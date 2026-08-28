/**
 * @brief   Kids Walkie-Talkie Video/Audio Transmission Module.
 */

#include "system/includes.h"
#include "app_config.h"
#include "app_msg.h"
#include "bt.h"
#include "btstack/avctp_user.h"
#include "clock_manager/clock_manager.h"
#include "spp_transfer.h"
#include "kid_wt_app.h"
#include "vstream_common.h"
#include "generic/list.h"
#include "system/spinlock.h"
#include "ui/ui_api.h"
#include "transfer_stubs.h"
#include "kid_wt_video.h"
#include "vstream_cmd.h"


/* CFG keys for KWT pairing storage — source project defines these in system/cfg.h */
#ifndef CFG_KWT_REMOTE_ADDR_INFO
#define CFG_KWT_REMOTE_ADDR_INFO    105
#endif
#ifndef CFG_KWT_MASTER_ROLE_INFO
#define CFG_KWT_MASTER_ROLE_INFO    106
#endif

/* BR33 平台：clock_add_set / clock_remove_set 适配 clock_manager。
 * 频率使用 clk_get_max_frequency() 动态获取，不硬编码避免不同芯片主频差异导致的问题。 */
#define VIDEO_TRANSFER_CLK          "video_transfer"
#define clock_add_set(name)         clock_lock(name, clk_get_max_frequency())
#define clock_remove_set(name)      clock_unlock(name)
#define LOG_TAG             "[KWT_APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_VSTREAM_TRANSFER

#if (TCFG_USER_EMITTER_ENABLE != 1) || (TCFG_APP_BT_EN != 1)
#error "TCFG_USER_EMITTER_ENABLE TCFG_APP_BT_EN must be 1"
#endif

#define KWT_SPP_CONNECT_BY_NAME			0	//搜到蓝牙名即连接
#define KWT_SPP_CONNECT_BY_ADDR			1	//主动选择远端地址连接
#define TCFG_KWT_SPP_CONNECT_SEL 		KWT_SPP_CONNECT_BY_ADDR

extern u16 bt_get_curr_channel_state();
static u8 scan_flag;    //判断哪方发起扫描
static u8 kwt_master_flag;		//主动发起方
static u8 kwt_pending_conn;	//用户已点"连接": 等待 INQUIRY_COMPLETE 后由 kwt_conn_handler 发起连接
static u8 kwt_inquiry_active;	//当前是否有查询(inquiry)在进行
static u8  kwt_reconn_retry;	//断线重连重试计数(仅主叫方)
static u16 kwt_reconn_tid;		//断线重连定时器 id
static u8  kwt_outbound_pending;	//本端已发起 outbound page, 等待 SPP connect 确认方向
#define KWT_RECONN_INTERVAL_MS   8000    /* 断线后主叫方每 8s 重连一次 */
#define KWT_RECONN_RETRY_MAX     2       /* 最多重连 2 次, 之后放弃 */
#define BT_SCAN_FILTER_RSSI_ENABLE		0
#define BT_SCAN_FILTER_RSSI_VAL			-50 //0~-128，数值越小，搜索范围越窄

extern void delete_mac_device_form_vm(bd_addr_t mac_addr);
static u8 connect_addr[6];
static u8 bd_name_filt[5][64] = {
    KID_WT_NAME
};
struct remote_dev_info {
    struct list_head list;
    u8 addr[6];
};
struct remote_dev_head {
    struct list_head list;
    spinlock_t lock;
    int dev_num;
    int init;
};
static struct remote_dev_head dev_list_head = {0};
u8 kwt_master_flag_set(u8 flag)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_info("%s %x %d\n", __func__, rets, flag);
    kwt_master_flag = flag;
    if (flag == 0) {
        kwt_outbound_pending = 0;   /* 清除主叫标志时, 一并清除待定的 outbound 标记 */
    }
    return kwt_master_flag;
}


u8 get_kwt_master_flag()
{
    return kwt_master_flag;
}

#if TCFG_KID_TALKIES_BT_CONNECT_ENABLE

/**
 * @brief 读取KWT主设备角色信息
 *
 * 该函数从系统配置中读取KWT（Kid Watch Talk）主设备角色信息，
 * 并根据读取到的标志位返回相应的角色状态。
 *
 * @return u8 返回主设备角色状态
 *         - 0xFF: 未定义或错误状态
 *         - 1:   设备为主设备
 *         - 0:   设备为从设备
 *
 * @note 该函数会输出调试信息到控制台，显示读取到的原始标志值和最终返回值
 */
u8 kwt_master_role_info_read(void)
{
    u8 flag = 0;  // 存储从系统配置读取的原始标志值
    // 从系统配置中读取KWT主设备角色信息
    syscfg_read(CFG_KWT_MASTER_ROLE_INFO, &flag, 1);
    u8 ret = 0xff;  // 初始化返回值为无效状态
    // 根据读取到的标志值确定返回值
    if (flag == 1) {
        ret = 1;  // 如果标志为1，则返回1表示设备为主设备
    } else {
        ret = 0;  // 否则返回0表示设备为从设备
    }
    // 输出调试信息，显示原始标志值和最终返回值
    printf("kwt_master_role_info_read : %d.ret = %d\n", flag, ret);
    return ret;  // 返回主设备角色状态
}

/**
 * @brief 写入KWT主设备角色信息到系统配置
 *
 * 该函数将指定的主设备角色标志写入系统配置中，用于保存
 * KWT（Kid Watch Talk）设备的角色状态（主设备或从设备）。
 *
 * @param flag 要写入的主设备角色标志
 *             - 1: 表示设备作为主设备
 *             - 0: 表示设备作为从设备
 *             - 其他值: 未定义行为
 *
 * @return void 无返回值
 *
 * @note 该函数会输出调试信息到控制台，并将角色信息持久化存储到系统配置中
 */
void kwt_master_role_info_write(u8 flag)
{
    u8 cur_flag = 0;  // 临时变量，用于存储要写入的标志值

    cur_flag = flag;  // 将输入参数赋值给临时变量

    // 输出调试信息，显示即将写入的标志值
    printf("kwt_master_role_info_write : %d\n", cur_flag);

    // 将标志值写入系统配置，持久化保存主设备角色信息
    syscfg_write(CFG_KWT_MASTER_ROLE_INFO, &cur_flag, 1);
}


/**
 * @brief 检查是否需要在开机时发起回连
 *
 * 该函数用于判断设备在开机时是否应该主动发起连接。
 * 判断条件包括：存在已记录的配对地址且当前设备是主设备。
 *
 * @return u8 返回是否需要发起回连
 *         - 1: 需要发起回连（设备是主设备且有配对记录）
 *         - 0: 不需要发起回连（非主设备或无配对记录）
 *
 * @note 此函数用于KWT（Kid Watch Talk）设备的自动重连逻辑，
 *       确保主设备在开机后能自动连接到之前配对的设备
 */
u8 power_on_check_master_flag(void)
{
    u8 remote_addr[6];  // 存储远程设备地址的缓冲区（蓝牙MAC地址格式）

    // 检查两个条件：
    // 1. 是否成功获取到已记录的配对设备地址（返回值为6表示成功获取完整地址）
    // 2. 当前设备是否为主设备角色（通过读取系统配置确认）
    if ((kwt_connect_addr_rec_get(remote_addr) == 6) && (kwt_master_role_info_read())) {
        // 如果同时满足：存在配对记录 且 当前设备是主设备
        return 1;  // 发起回连
    } else {
        // 如果不满足任一条件：无配对记录 或 当前设备不是主设备
        return 0;  // 不是主机，不发起回连
    }
}
#endif // endif TCFG_KID_TALKIES_BT_CONNECT_ENABLE
int kwt_dev_list_init()
{
    if (!dev_list_head.init) {
        INIT_LIST_HEAD(&dev_list_head.list);
        spin_lock_init(&dev_list_head.lock);
        dev_list_head.init = 1;
    }
    return 0;
}
int kwt_dev_list_add(u8 *addr)
{
    if (!dev_list_head.init) {
        kwt_dev_list_init();
    }
    struct  remote_dev_info *dev = (struct remote_dev_info *)malloc(sizeof(struct remote_dev_info));
    if (!dev) {
        return -1;
    }
    memcpy(dev->addr, addr, 6);
    spin_lock(&dev_list_head.lock);
    list_add_tail(&dev->list, &dev_list_head.list);
    dev_list_head.dev_num++;
    spin_unlock(&dev_list_head.lock);
    UI_MSG_POST("scan_update:event=%4", 1);
    return 0;
}
int kwt_dev_list_clr()
{
    if (!dev_list_head.init) {
        return -1;
    }
    struct  remote_dev_info *p, *n;
    spin_lock(&dev_list_head.lock);
    list_for_each_entry_safe(p, n, &dev_list_head.list, list) {
        list_del(&p->list);
        free(p);
    }
    dev_list_head.dev_num = 0;
    spin_unlock(&dev_list_head.lock);
    return 0;
}
int kwt_dev_list_get_addr_by_index(int index, u8 *addr)
{
    if (!dev_list_head.init) {
        return -1;
    }
    if (index >= dev_list_head.dev_num) {
        return -1;
    }

    struct remote_dev_info *p;
    int list_idx = 0;
    int ret = -2;
    spin_lock(&dev_list_head.lock);
    list_for_each_entry(p, &dev_list_head.list, list) {
        if (list_idx == index) {
            memcpy(addr, p->addr, 6);
            ret = 0;
            break;
        }
        list_idx++;
    }
    spin_unlock(&dev_list_head.lock);
    return ret;
}
int kwt_dev_list_get_num()
{
    return dev_list_head.dev_num;
}

int kwt_connect_addr_rec_set(u8 *addr)
{
    printf("%s :", __func__);
    put_buf(addr, 6);
    return syscfg_write(CFG_KWT_REMOTE_ADDR_INFO, addr, 6);
}
int kwt_connect_addr_rec_get(u8 *addr)
{
    int ret = syscfg_read(CFG_KWT_REMOTE_ADDR_INFO, addr, 6);
    if (ret == 6) {
        //无效数据
        int sum  = 0;
        for (int i = 0; i < 6; i++) {
            sum += addr[i];
        }
        if (sum == 0) {
            return 0;
        }
        printf("%s :", __func__);
        put_buf(addr, 6);
    }
    return ret;
}
int kwt_connect_addr_rec_clr()
{
    u8 addr[6] = {0};
    u8 test = 0;
    syscfg_write(CFG_KWT_MASTER_ROLE_INFO, &test, 1);
    return syscfg_write(CFG_KWT_REMOTE_ADDR_INFO, addr, 6);
}
int kwt_disconnect_delete_dev()
{
    u8 addr[6];
    kwt_connect_addr_rec_get(addr);
    if (kwt_get_spp_connect()) {
        printf("%s %d", __func__, __LINE__);
        bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 6, addr);
    }
    // (void)delete_last_device_from_vm();
    /* delete_mac_device_form_vm(addr); */
    return 0;
}

static int is_addr_valid(void)
{
    for (int i = 0; i < ARRAY_SIZE(connect_addr); ++i) {
        if (connect_addr[i] != 0) {
            return 1;
        }
    }
    return 0;
}

u8 *kwt_get_connect_addr(void)
{
    return connect_addr;
}

static int edr_scan(void)
{
    if (kwt_get_spp_connect()) {
        log_info("%s SPP is connected!", __func__);
        return FALSE;
    }
    /* 扫描真正发起的条件: 协议栈已开始初始化(init_start) 且 发射模式就绪
     * (emitter_or_receiver == BT_EMITTER_EN, 在 BT_STATUS_INIT_OK 时才置位)。
     * init_start 在初始化开始即置 1, 若只判它, 在 init_start=1 与
     * BT_STATUS_INIT_OK 之间的窗口内 bt_emitter_start_search_device() 会静默
     * 返回而不发起查询, 却误报扫描成功(scan_flag=1), 导致上层不再重试。 */
    extern int bt_emitter_is_search_ready(void);
    if (!bt_emitter_is_search_ready()) {
        log_info("%s BT not ready, skip scan", __func__);
        return FALSE;
    }
    bt_emitter_start_search_device();
    return TRUE;
}

static void edr_stop_scan(void)
{
    bt_search_stop();
}

static void edr_connect(void)
{
    // BY_ADDR 模式：扫描完成(kwt_conn_handler)时若用户尚未选择设备,
    // connect_addr 为全 0, 不能发起连接。必须有有效地址才连接。
    if (is_addr_valid() == 1) {
        log_debug("%s connect_addr:\n", __func__);
        log_debug_hexdump(kwt_get_connect_addr(), 6);
        /* 本端主动 page(outbound): 标记本端为主叫方, 断线后由本端主动重连,
         * 被叫方仅等待接受, 仲裁避免双方同时 page 导致"连上即断"。 */
        kwt_master_flag_set(1);
        kwt_outbound_pending = 1;
        bt_cmd_prepare(USER_CTRL_START_CONNEC_SPP_VIA_ADDR, 6, kwt_get_connect_addr());
    }
}

void kwt_set_connet_addr(u8 *addr)
{
    log_info("%s", __func__);
    log_info_hexdump(addr, 6);
    memcpy(connect_addr, addr, 6);
}
void kwt_set_connet_addr_with_conn(u8 *addr)
{
    log_info("%s", __func__);
    log_info_hexdump(addr, 6);
    memcpy(connect_addr, addr, 6);
    edr_connect();
}

/* "连接"按钮专用: 设置远端地址并决定立即连接还是延迟到 INQUIRY_COMPLETE 后再连接。
 * 若查询仍在进行, 立即发 START_CONNEC_SPP_VIA_ADDR 会被随后
 * emitter_search_stop_handle() 的 USER_CTRL_PAGE_CANCEL 取消, 导致一次点击连不上;
 * 改为置 kwt_pending_conn, 由 kwt_conn_handler() 在 INQUIRY_COMPLETE(且已发出
 * PAGE_CANCEL)之后发起连接。若查询已结束, 则直接连接。 */
void kwt_set_connet_addr_defer(u8 *addr)
{
    log_info("%s", __func__);
    log_info_hexdump(addr, 6);
    memcpy(connect_addr, addr, 6);
    if (kwt_inquiry_active) {
        kwt_pending_conn = 1;
    } else {
        edr_connect();
    }
}

int kwt_get_spp_connect(void)
{
    /* log_debug("%s %x", __func__, bt_get_curr_channel_state()); */
    // THIS IS BROKE AFTER BTSTACK UPDATE!!!!!!
    //return (bt_get_curr_channel_state() & SPP_CH);

    extern u8 kwt_spp_connected;
    return kwt_spp_connected;
}

/* 断线重连定时器(前向声明, 定义见 kwt_conn_handler 之后) */
void kwt_reconn_stop(void);
static void kwt_reconn_start(void);

void kwt_stream_start(void)
{
    /* 连接方向仲裁: outbound(本端主动 page)保持主叫标志; inbound(对端连入)则清除
     * 主叫标志(本端为被叫方)。否则角色反转时双方都会 master=1, 下次断线双方同时
     * page 导致"连上即断"。 */
    if (kwt_outbound_pending) {
        kwt_outbound_pending = 0;
    } else {
        kwt_master_flag_set(0);
    }

#if KID_WT_VSTREAM_START_ON_CONNECT
    /* 重连成功: 停止断线重连定时器, 恢复远端画面(清除白屏)。 */
    kwt_reconn_stop();

    /* 复位 vstream 发送端序号, 使重连后与首次上电一致(避免断线期间残留的
     * 序号状态)。SPP TX 缓冲是 btctrler 全局, 应用层无法 flush(见 workflow 结论),
     * 发送卡死的实际缓解是 spp_send 里的有界等待(背压)。 */
    vstream_sender_reset();

    kid_wt_video_xsmt_start();
    kid_wt_video_recv_start();

    extern void audio_app_open(void);
    audio_app_open();

    /* 连接建立后主叫/被叫双方都进入视频通话页。此前只有主叫(ble_search_poll_cb)
     * 跳转, 被叫只启动媒体不跳页, 画面不显示(WT-009/WT-004 被动侧)。统一由
     * SPP 连接事件驱动跳转, 两侧一致。 */
    walkie_talkie_video_enter();
    walkie_talkie_unblank_remote();
#endif
}

void kwt_stream_stop(void)
{
    /* Disconnect (对端意外掉线/断电/超距): 不再跳回配对页, 而是
     * (1) 隐藏语音 overlay + 把远端画面置白(停留在对讲页);
     * (2) 进入可被发现/可连接状态, 接受对端回连;
     * (3) 主叫方启动定时重连, 被叫方仅等待接受(仲裁, 避免双方同时 page)。
     * 三者都经异步接口立即返回, 再做可能阻塞的媒体拆除。 */
    walkie_talkie_talk_voice_pic_hide();
    walkie_talkie_blank_remote();

    /* 断线后进入可被发现+可连接状态(接受对端按正常流程回连)。
     * 用 bt_cmd_prepare 排队命令(不直接写控制器, 可从 app_core 安全调用),
     * 避免 bt_discovery_and_connectable_using_loca_mac_addr 里的
     * lmp_hci_write_local_address 在 app_core 线程直接写控制器。 */
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);

    /* 主叫方定时重连 */
    kwt_reconn_start();

#if KID_WT_VSTREAM_START_ON_CONNECT
    kid_wt_video_recv_stop();
    kid_wt_video_xsmt_stop();

    extern void audio_app_stop(void);
    audio_app_stop();
#endif
}

void kwt_conn_handler(void)
{
    log_debug("%s channel state:0x%x scan_flag:%d master_flag:%x \n", __func__, bt_get_curr_channel_state(), scan_flag, kwt_master_flag);
    kwt_inquiry_active = 0;   /* 查询(inquiry)已结束 */
    if (bt_get_curr_channel_state() & SPP_CH) {
        kwt_pending_conn = 0;
        return;
    }
    if (kwt_pending_conn) {
        /* 用户已点"连接": 此处运行在 HCI_EVENT_INQUIRY_COMPLETE 回调里,
         * 且位于 emitter_search_stop_handle() 之后(其已发出 USER_CTRL_PAGE_CANCEL),
         * 此时再发起 START_CONNEC_SPP_VIA_ADDR 不会被 PAGE_CANCEL 取消, 一次点击即可连上。 */
        kwt_pending_conn = 0;
        edr_connect();
    } /*else if (scan_flag == 1) {
        edr_connect();
    }*/
    scan_flag = 0;
}

/* 断线重连: 主叫方(kwt_master_flag==1)在断线后每 8s 尝试重连一次, 最多 2 次;
 * 被叫方不重连, 仅接受连接, 以此仲裁避免双方同时 page 导致"连上即断"。 */
void kwt_reconn_stop(void)
{
    if (kwt_reconn_tid) {
        sys_timer_del(kwt_reconn_tid);
        kwt_reconn_tid = 0;
    }
    kwt_reconn_retry = 0;
}

static void kwt_reconn_timer_cb(void *p)
{
    (void)p;
    if (!kwt_master_flag) {
        kwt_reconn_stop();   /* 主叫标志已清(如用户显式退出), 停止重连 */
        return;
    }
    if (kwt_get_spp_connect()) {
        kwt_reconn_stop();   /* 已重连 */
        return;
    }
    if (kwt_reconn_retry >= KWT_RECONN_RETRY_MAX) {
        /* 重连次数用尽, 放弃: 清除主叫标志, 让设备回归被叫方角色, 之后由对端主动
         * 连入(对端此时进入配对页点击"连接"成为主叫)。 */
        kwt_master_flag_set(0);
        kwt_outbound_pending = 0;
        kwt_reconn_stop();
        return;
    }
    u8 remote_addr[6];
    if (kwt_connect_addr_rec_get(remote_addr) == 6) {
        kwt_set_connet_addr(remote_addr);
        kwt_reconn_retry++;
        edr_connect();
    } else {
        kwt_reconn_stop();   /* 无配对地址, 无法重连 */
    }
}

static void kwt_reconn_start(void)
{
    if (!kwt_master_flag) {
        return;   /* 仅主叫方主动重连 */
    }
    kwt_reconn_stop();
    kwt_reconn_tid = sys_timer_add(NULL, kwt_reconn_timer_cb, KWT_RECONN_INTERVAL_MS);
}

void kwt_disconn_handler(void)
{
    log_info("%s channel:0x%x scan_flag:%d\n", __func__,
             bt_get_curr_channel_state(), scan_flag);

    /* HCI link dropped — link-level reset only. Media teardown is handled by
     * spp_state_callback (spp_transfer.c) -> kwt_stream_stop(); protocol
     * deinit stays session-level (multi_protocol_main.c, BT-mode exit). */
    scan_flag = 0;          /* else stale flag re-triggers edr_connect() on
                             * next INQUIRY_COMPLETE (kwt_conn_handler) */
    kwt_outbound_pending = 0; /* 链路断开, 清除待定的 outbound 标记 */
    kwt_stop_scan();        /* stop any in-flight inquiry */

#if TCFG_KWT_SPP_CONNECT_SEL == KWT_SPP_CONNECT_BY_ADDR
    kwt_dev_list_clr();     /* free scanned-device list (kid_bt_search_bd_name_filt) */
#endif

    /* Reset in-call command status so a stale flag can't block the next call
     * (vstream_cmd_ask_video_start ignores calls while video_status is set). */
    vstream_cmd_video_status_set(0);
    vstream_cmd_video_break_set(0);
    vstream_cmd_voice_status_set(0);
}

int kwt_scan_devs(void)
{
    int flag;
    flag = edr_scan();

    if (flag == TRUE) {
        log_info("%s begin!", __func__);
        scan_flag = 1;
        kwt_inquiry_active = 1;
        return 0;
    } else {
        log_info("%s error!", __func__);
        scan_flag = 0;
        return -1;
    }
}

void kwt_stop_scan(void)
{
    log_info("%s", __func__);
#if (TCFG_KWT_SPP_CONNECT_SEL == KWT_SPP_CONNECT_BY_ADDR)
    /* BY_ADDR 模式: 连接由用户点击 "连接" 显式发起(kwt_set_connet_addr_with_conn),
     * 停止扫描后若 scan_flag 仍为 1, INQUIRY_CANCEL 触发的 HCI_EVENT_INQUIRY_COMPLETE
     * 会进入 kwt_conn_handler() 再次 edr_connect(), 造成重复连接。故停止扫描即清
     * scan_flag。BY_NAME 模式(编译未启用)依赖 scan_flag 在 stop->connect 间保持,
     * 所以仅 BY_ADDR 分支清除。 */
    scan_flag = 0;
#endif
    edr_stop_scan();
}

void kwt_transfer_protocol_init(void)
{
    log_info("%s", __func__);
    spp_protocol_init();


    struct vstream_info s_info = {0};
#if 0
    s_info.sample_rate = AUDIO_SEND_SAMPLE_RATE;
#endif
    s_info.abr_kbps = KWT_CAMERA_ABR_KBPS;
    s_info.fps = KWT_CAMERA_FPS;
    vstream_sender_open(&s_info);

    vstream_receiver_open();
    clock_add_set(VIDEO_TRANSFER_CLK);

}

void kwt_transfer_protocol_deinit(void)
{
    log_info("%s", __func__);

    vstream_receiver_close();
    vstream_sender_close();

    spp_protocol_deinit();
    clock_remove_set(VIDEO_TRANSFER_CLK);
}


void kwt_bt_pa_init(void)
{
//     log_info("%s %d", __func__, __LINE__);

//     /**根据具体的pa芯片和匹配料，具体调整该参数
//       *公版环境CB2401, AT2401 使用0x08
//       */
//     extern void balun_interface(u8 recfg_en, u8 txbalun);
//     balun_interface(1, 0x8);

//     void bt_set_rxtx_status_enable(u8 en);
//     void bt_rf_PA_control_io_remap(u16 tx_io, u16 rx_io);
//     bt_set_rxtx_status_enable(1);
//     bt_rf_PA_control_io_remap(IO_PORTG_00, IO_PORTG_01);

//     gpio_set_die(IO_PORTG_02, 1);
//     gpio_set_hd(IO_PORTG_02, 0);
//     gpio_set_hd0(IO_PORTG_02, 1);
//     gpio_direction_output(IO_PORTG_02, 1);
}

u8 kid_bt_search_bd_name_filt(char *name_data, u8 name_len, u8 *addr, int rssi)
{
    char bd_name[64] = {0};
    int i;
    if ((name_len > ARRAY_SIZE(bd_name_filt[0])) || (name_len == 0)) {
        log_error("bd_name_len error:%d\n", name_len);
        return FALSE;
    }
#if BT_SCAN_FILTER_RSSI_ENABLE
    if (rssi < BT_SCAN_FILTER_RSSI_VAL) {
        return FALSE;
    }
#endif
    for (i = 0; i < (sizeof(bd_name_filt) / sizeof(bd_name_filt[0])); i++) {
        if (memcmp(name_data, bd_name_filt[i], name_len) == 0) {
#if (TCFG_KWT_SPP_CONNECT_SEL == KWT_SPP_CONNECT_BY_NAME)
            puts("\n*****find dev ok******\n");
            printf("%s add_name:%s rssi:%d", __func__, name_data, rssi);
            put_buf(addr, 6);
            kwt_stop_scan();
            kwt_set_connet_addr(addr);
#else
            printf("%s add_name:%s rssi:%d", __func__, name_data, rssi);
            kwt_dev_list_add(addr);
#endif
            return TRUE;
        }
    }
    return FALSE;

}

#endif /*#if TCFG_VSTREAM_TRANSFER*/

