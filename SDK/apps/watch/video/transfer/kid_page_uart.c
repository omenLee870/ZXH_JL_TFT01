/**
 * @file kid_page_uart.c
 * @brief 儿童对讲机串口配对模块 (产线组装时蓝牙配对)
 *
 * 该模块实现了两台对讲机之间通过串口(默认 USB_DP/USB_DM)进行配对的功能，
 * 主要用于产线组装时交换蓝牙 MAC 地址并建立配对。
 *
 * 移植自 br28 工程 cpu/periph_demo/kid_page_uart_demo.c, 适配 br33 kid_camera:
 *   - 全程代码由 TCFG_KID_PAGE_BY_UART_ENABLE 包裹;
 *   - 自建独立任务 "kid_page" 跑配对消息循环(与 br28 demo 一致), 不阻塞 app_core,
 *     从而 BT 协议栈 / UI 可正常初始化;
 *   - 配对成功后不复用独立的 BT_PAGE UI, 而是复用本工程 KWT 对讲链路
 *     (kwt_set_connet_addr_with_conn + kwt_connect_addr_rec_set), 连接成功由 SPP
 *     连接事件统一驱动跳转到视频对讲页(kwt_stream_start -> walkie_talkie_video_enter);
 *   - 开机 60s 超时后自动关闭串口配对功能。
 *
 * 核心功能：
 * 1. 串口初始化和配置
 * 2. 基于 CRC16 校验的数据帧传输
 * 3. 主机/从机角色协商
 * 4. 配对超时自动关闭
 */

#include "system/includes.h"
#include "app_config.h"
#include "uart.h"
#include "crc.h"
#include "kid_wt_app.h"
#include "user_cfg.h"          /* bt_get_mac_addr() */

#if TCFG_KID_PAGE_BY_UART_ENABLE

#define LOG_TAG             "[KID_PAGE_UART]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

/** @brief 配对任务名称(需在 task_info_table 中注册) */
#define KID_PAGE_TASK_NAME  "kid_page"

/** @brief 任务消息类型枚举 */
enum {
    KID_UART_TX_DATA = 1,  /**< 发送配对数据(发起配对) */
    KID_UART_RX_DATA = 2,  /**< 收到串口数据(处理配对帧) */
    KID_UART_CLOSE = 3,    /**< 关闭配对(超时或外部主动关闭) */
};

/** @brief 配对任务运行状态标志 */
static u8 kid_uart_page_run = 0;

/** @brief 本端是否按下了配对键(长按发起方) */
static u8 kid_uart_initiator = 0;

/** @brief 串口配对数据帧结构
 *
 * 数据帧格式：
 * +--------+--------+--------+
 * |  CRC   | Length |  Data  |
 * | 2 bytes| 2 bytes| N bytes|
 * +--------+--------+--------+
 */
struct kid_uart_frame {
    u16 crc;      /**< CRC16校验值 */
    u16 length;   /**< 数据长度 */
    u8 data[0];   /**< 可变长度数据（MAC地址） */
};

/** @brief 配对数据帧指针 */
static struct kid_uart_frame *frame = NULL;

/** @brief 串口设备编号 */
static int uart_num = -1;

/** @brief 接收缓冲区指针(DMA 要求) */
static void *uart_rx_ptr = NULL;

/** @brief 60s 超时定时器句柄 */
static u16 kid_uart_timeout_tid = 0;

/**
 * @brief 串口中断回调函数
 * @param num   串口编号
 * @param event 串口事件类型
 *
 * 收到数据时投递消息到 "kid_page" 任务, 由配对消息循环阻塞读取并解析配对帧。
 */
static void uart_irq_func(int num, enum uart_event event)
{
    (void)num;
    /* 与 br28 参考实现一致: 无条件投递。参考实现把 4 个 event 判断的函数体全部注释掉,
     * 只留下这一句 os_taskq_post_msg —— 即不依赖具体事件类型。
     * br33 移植时把 post 挪进了 `if (event & UART_EVENT_RX_DATA)`, 而 10 字节帧
     * 对 frame_size=32 永远凑不满 DMA 帧, 驱动只会发 RX_TIMEOUT(见 sdk.lst uart_isr
     * 8168a9e), 于是 post 被过滤掉、任务永不唤醒 —— 这是 RX 收不到的直接原因。 */
    os_taskq_post_msg(KID_PAGE_TASK_NAME, 1, KID_UART_RX_DATA);
}

/**
 * @brief 发送配对数据（实际发送函数）
 *
 * 组装并发送配对数据帧：清空帧缓冲 -> 填长度(6) -> 拷贝本地 MAC -> 计算 CRC
 * -> 串口阻塞发送。
 */
static void kid_uart_send_data(void)
{
    if (!kid_uart_page_run || !frame) {
        return;
    }

    memset(frame, 0, 64);
    frame->length = 6;                                       /* 数据长度：6字节MAC地址 */
    memcpy(frame->data, (u8 *)bt_get_mac_addr(), 6);         /* 复制本地MAC地址 */
    frame->crc = CRC16(frame->data, 6);                      /* 计算CRC16校验 */

    /* 总长度与参考实现一致: length 字段(2) + crc 字段(2) + MAC(6) = 10 */
    u8 send_len = frame->length + sizeof(frame->crc) + sizeof(frame->length);
    int send = uart_send_blocking(uart_num, frame, send_len, 20);
    if (send > 0) {
        log_info("%s sent=%d bytes\n", __func__, send);
    } else {
        log_error("%s send failed ret=%d\n", __func__, send);
    }
}

/**
 * @brief 关闭配对后统一清理串口与缓冲
 */
static void kid_uart_cleanup(void)
{
    if (kid_uart_timeout_tid) {
        sys_timeout_del(kid_uart_timeout_tid);
        kid_uart_timeout_tid = 0;
    }
    if (uart_num >= 0) {
        uart_deinit(uart_num);
        uart_num = -1;
    }
    if (frame) {
        dma_free(frame);
        frame = NULL;
    }
    if (uart_rx_ptr) {
        dma_free(uart_rx_ptr);
        uart_rx_ptr = NULL;
    }

#if TCFG_KID_PAGE_UART_USE_USB_IO
    /* 复位串口引脚(与 product_test 的 pt_check_ot 清理方式一致) */
    gpio_set_mode(IO_PORT_SPILT(IO_PORT_DP), PORT_HIGHZ);
    gpio_set_mode(IO_PORT_SPILT(IO_PORT_DM), PORT_HIGHZ);
#else
    gpio_set_mode(IO_PORT_SPILT(TCFG_KID_PAGE_UART_TX_IO), PORT_HIGHZ);
    gpio_set_mode(IO_PORT_SPILT(TCFG_KID_PAGE_UART_RX_IO), PORT_HIGHZ);
#endif
}

/**
 * @brief 配对消息循环任务
 *
 * 阻塞等待消息并处理配对帧:
 * - KID_UART_TX_DATA: 发送本地 MAC, 发起配对(仅主叫方);
 * - KID_UART_RX_DATA: 阻塞读取串口数据, CRC 校验通过后按角色处理:
 *     主机模式: 把对端 MAC 写入连接并发起连接(连接成功后由 SPP 事件跳对讲页);
 *     从机模式: 回复自己的 MAC 地址;
 * - KID_UART_CLOSE: 退出循环并清理。
 */
static void kid_uart_pairing_task(void *p)
{
    (void)p;
    int msg[4];
    int ret;
    int r;

    printf("%s\n", __func__);

    while (kid_uart_page_run) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            os_time_dly(1);
            continue;
        }

        /* 响应 task_kill("kid_page"): 与 vstream_sender/vstream_receiver/kid_wt_video
         * 的退出协议保持一致, 否则按名字杀不掉本任务。此处只置标志退出循环, 由循环
         * 之后的 kid_uart_cleanup() 完成清理, 再调 os_task_del_res() 交还 TCB
         * (该函数内部是 OSTimeDly 死循环, 不会返回)。 */
        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            kid_uart_page_run = 0;
            break;
        }

        switch (msg[1]) {
        case KID_UART_TX_DATA:
            /* 仅主叫方主动发送; 从机等待主叫先发 */
            kid_uart_send_data();
            break;

        case KID_UART_RX_DATA:
            if (!frame) {
                break;
            }
            /* 与参考实现一致: 一次阻塞读, 读到多少算多少, 不做凑帧。 */
            r = uart_recv_blocking(uart_num, frame, 64, 10);
            if (r > 0) {
                u16 recv_crc = frame->crc;
                u16 calc_crc = CRC16(frame->data, frame->length);
                if (recv_crc == calc_crc) {
                    log_info("%s crc ok len=%d\n", __func__, r);
                    /* 角色由按键时机决定, 不再读 get_kwt_master_flag():
                     * 该标志只在 edr_connect() 里置 1, 而 edr_connect() 恰好要
                     * 等配对成功才会被调用, 所以本函数执行时它必然还是 0 ——
                     * 双方都会走"从机回帧"分支, 谁也发不起连接。
                     * 本协议帧里只有 MAC 没有角色位, 故用长按发起方(先发方)
                     * 作主机: 先按下的一方 send 后再收到对方回帧 -> 主叫去连;
                     * 被叫方先收到帧(此时尚未 send) -> 回帧并等对方连入。 */
                    if (kid_uart_initiator) {
                        /* 主机模式：设置远端地址并立即发起连接(内部置主叫标志),
                         * 并持久化配对地址供断电回连; 完成后结束本次配对。 */
                        kwt_set_connet_addr_with_conn(frame->data);
                        kwt_connect_addr_rec_set(frame->data);
                        kid_uart_page_run = 0;
                    } else {
                        /* 从机模式：回复自己的MAC地址 */
                        kid_uart_send_data();
                    }
                } else {
                    log_error("%s crc fail recv=0x%04x calc=0x%04x\n",
                              __func__, recv_crc, calc_crc);
                }
            }
            break;

        case KID_UART_CLOSE:
            kid_uart_page_run = 0;
            break;

        default:
            break;
        }
        os_time_dly(1);
    }

    kid_uart_cleanup();

    /* 任务必须显式交还 TCB 再自删, 不能直接 return:
     * 本 SDK 的 OSTaskStkInit 从不写入返回地址槽(sdk.lst OSTaskStkInit 只写
     * 栈顶 +0x0/+0x8/+0xc, +0x4 保持 0), 而本函数序言是
     * `[--sp] = {rets, r14-r4}` / 尾声 `{pc, r14-r4} = [sp++]`。若直接 return,
     * pc 会从栈上取出 0, 即跳转到地址 0 —— 这正是 60s 超时(kid_uart_pt_close)
     * 触发关闭后崩溃的原因。os_task_del_res() 内部是 OSTimeDly 死循环, 不返回。 */
    os_task_del_res(OS_TASK_SELF);
}

/**
 * @brief 发送配对数据（非阻塞，投递消息到 kid_page 任务）
 *
 * 默认在未配对时长按触发, 仅主叫方调用; 开机 60s 后配对功能已关闭, 直接返回。
 */
void kid_uart_tx_data(void)
{
    log_info("%s\n", __func__);
    if (!kid_uart_page_run) {
        return;
    }
    /* 长按者即配对发起方(主机): 由本端先发 MAC, 对端收到后回帧, 本端再去连接。 */
    kid_uart_initiator = 1;
    os_taskq_post_msg(KID_PAGE_TASK_NAME, 1, KID_UART_TX_DATA);
}

/**
 * @brief 关闭串口配对功能(供外部主动关闭, 投递消息避免自删任务)
 */
void kid_uart_close(void)
{
    log_info("%s\n", __func__);
    if (!kid_uart_page_run) {
        return;
    }
    os_taskq_post_msg(KID_PAGE_TASK_NAME, 1, KID_UART_CLOSE);
}

/**
 * @brief 串口配对超时关闭（定时器回调）
 *
 * 开机 60s 后投递关闭消息, 由配对任务自行退出并清理。
 */
static void kid_uart_pt_close(void *p)
{
    (void)p;
    log_info("%s\n", __func__);
    /* 配对可能已因成功配对而提前结束(任务已退出并清理), 此时不必再投递关闭消息;
     * 消息投递到已不存在的任务只会被 SDK 静默丢弃, 这里显式挡掉便于排查。 */
    if (!kid_uart_page_run) {
        return;
    }
    os_taskq_post_msg(KID_PAGE_TASK_NAME, 1, KID_UART_CLOSE);
}

/**
 * @brief 启动串口配对任务
 *
 * 执行以下操作：
 * 1. 初始化串口(USB_DP/USB_DM, 115200)和 DMA 接收;
 * 2. 注册 60s 超时定时器(超时后自动关闭配对);
 * 3. 创建独立配对任务 "kid_page" 跑消息循环。
 *
 * 配对消息循环里主叫方收到对端 MAC 后会调用 kwt 连接接口, 因此本函数应在
 * BT 模式进入阶段调用(与 BLE 配对页扫描相同的前提: BT 协议栈已初始化)。
 */
void kid_uart_page_start(void)
{
    int err;

    log_info("%s\n", __func__);

    /* 幂等保护: 本函数会被 BT 模式每次进入时经
     * bt_status_init_ok -> multi_protocol_bt_init -> kwt_transfer_protocol_init 调用,
     * 而 BT 模式可反复进出(对讲页 LOAD 发 APP_MODE_BT, UNLOAD 发 APP_MODE_IDLE)。
     * 若重复进入, 下面的 task_create("kid_page") 会因任务已存在使 OSTaskCreate 返回
     * OS_TASK_EXIST, 而 task_create 对该返回值走的是 printf + cpu_assert_debug 死循环
     * (sdk.lst task_create+0x86), 即直接挂死 app_core。
     * 直接以任务是否还存在为准: 任务退出时先跑 kid_uart_cleanup() 才自删, 因此
     * kid_uart_page_run 可能已为 0 而任务尚未消亡, 只判该标志会漏掉这个窗口。 */
    if (kid_uart_page_run || os_task_get_handle(KID_PAGE_TASK_NAME)) {
        log_info("%s already running, skip\n", __func__);
        return;
    }

    struct uart_config config = {
        .baud_rate = 115200,
#if TCFG_KID_PAGE_UART_USE_USB_IO
        /* 配对串口走 USB_DP/USB_DM, 与 br28 demo 一致 */
        .tx_pin = IO_PORT_DP,
        .rx_pin = IO_PORT_DM,
#else
        /* 普通 IO 串口, 引脚由板级配置指定 */
        .tx_pin = TCFG_KID_PAGE_UART_TX_IO,
        .rx_pin = TCFG_KID_PAGE_UART_RX_IO,
#endif
        .parity = UART_PARITY_DISABLE,
    };

    uart_rx_ptr = dma_malloc(128);
    if (!uart_rx_ptr) {
        log_error("%s dma_malloc rx fail\n", __func__);
        return;
    }

    uart_num = uart_init(-1, &config);
    if (uart_num < 0) {
        log_error("%s uart_init err %d\n", __func__, uart_num);
        goto err_init;
    }

    struct uart_dma_config dma = {
        /* 与参考实现保持一致。参考的注释把 100 写成 "100ms" 是错的(单位是 us),
         * 但既然参考实现以此工作正常, 先不改动 —— 一处一个变量, 便于定位问题。 */
        .rx_timeout_thresh = 100,
        .frame_size = 32,
        .event_mask = UART_EVENT_RX_DATA | UART_EVENT_RX_FIFO_OVF | UART_EVENT_RX_TIMEOUT,
        .irq_priority = 3,
        .irq_callback = uart_irq_func,
        .rx_cbuffer = uart_rx_ptr,
        .rx_cbuffer_size = 128,
    };

    err = uart_dma_init(uart_num, &dma);
    if (err < 0) {
        log_error("%s dma init err %d\n", __func__, err);
        goto err_dma;
    }

    frame = (struct kid_uart_frame *)dma_malloc(64);
    if (!frame) {
        log_error("%s dma_malloc frame fail\n", __func__);
        goto err_frame;
    }

    kid_uart_page_run = 1;
    kid_uart_initiator = 0;

    /* 开机 60s 后关闭串口配对功能 */
    kid_uart_timeout_tid = sys_timeout_add(NULL, kid_uart_pt_close, 60 * 1000);

    err = task_create(kid_uart_pairing_task, NULL, KID_PAGE_TASK_NAME);
    if (err != OS_ERR_NONE) {
        log_error("%s task create fail %x\n", __func__, err);
        kid_uart_page_run = 0;
        goto err_task;
    }
    return;

err_task:
    if (kid_uart_timeout_tid) {
        sys_timeout_del(kid_uart_timeout_tid);
        kid_uart_timeout_tid = 0;
    }
err_frame:
    if (frame) {
        dma_free(frame);
        frame = NULL;
    }
    uart_deinit(uart_num);
    uart_num = -1;
err_dma:
    dma_free(uart_rx_ptr);
    uart_rx_ptr = NULL;
err_init:
    return;
}

#endif // TCFG_KID_PAGE_BY_UART_ENABLE
