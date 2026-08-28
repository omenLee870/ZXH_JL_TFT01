#include "app_config.h"
#include "jldtp/jldtp_manager.h"
#include "jldtp/uart_transport.h"
#include "jldtp/spi_transport.h"
#include "cpu/periph/gpio.h"
#include "system/task.h"
#include "app_main.h"


#if TCFG_MULTI_CHIP_CONN_ENABLE


#ifndef JLDTP_MC_CHANNEL_NUM
#define JLDTP_MC_CHANNEL_NUM    4
#endif

#ifndef JLDTP_POLL_INTERVAL
#define JLDTP_POLL_INTERVAL     100
#endif


typedef struct {
    jldtp_handle_t       dtp_handle;
    jldtp_mc_handle_t    mc_handle;
    jldtp_channel_t      cmd_channel;
} jldtp_manager_t;

static u8 manager_num;
static jldtp_manager_t *manager;


extern int audio_timer_init(void);

static void jldtp_event_handler(int event)
{
    app_send_message_from(MSG_FROM_JLDTP, 4, (int *)&event);
}

static jldtp_result_t cmd_channel_data_handler(void *user_data, jldtp_frame_t *frame,
        u8 *data, u16 len)
{
    app_send_message_from(MSG_FROM_JLDTP, len, (int *)data);
    return JLDTP_SUCCESS;
}


static int get_jldtp_role(int config_role)
{
#if 0
    int role = JLDTP_MASTER;
    gpio_set_mode(IO_PORT_SPILT(IO_PORTB_00), PORT_INPUT_PULLDOWN_10K);
    os_time_dly(1);
    if (gpio_read(IO_PORTB_00)) {
        role = JLDTP_MASTER;
        r_printf("jldtp_role: master\n");
    } else {
        role = JLDTP_SLAVE;
        r_printf("jldtp_role: slave\n");
    }
    gpio_set_mode(IO_PORT_SPILT(IO_PORTB_00), PORT_HIGHZ);
    return role;
#endif
    return config_role;
}


int jldtp_manager_init(const struct multi_chip_conn_config *configs, u8 config_num)
{
    jldtp_config_t dtp_config;

    manager_num = config_num;
    manager = zalloc(sizeof(jldtp_manager_t) * config_num);
    ASSERT(manager, "jldtp_manager zalloc failed, num=%d", config_num);
    for (int i = 0; i < config_num; i++) {
        dtp_config.role           = get_jldtp_role(configs[i].role);
        dtp_config.poll_interval  = JLDTP_POLL_INTERVAL;
        dtp_config.max_retries    = 4;
        dtp_config.mtu_size       = configs[i].mtu_size;
        dtp_config.response_delay = 0;

        ASSERT(configs[i].remote_chip_id < config_num, "%d, %d\n",
               configs[i].remote_chip_id, config_num);
        jldtp_manager_t *mgr = &manager[configs[i].remote_chip_id];

        /*
         * ★ 先调用 transport_get 填充 dtp_config (full_duplex/max_trans_time),
         *   再调用 jldtp_create, 因为 create 内部会 copy config 到 handler
         */
#if (TCFG_MULTI_CHIP_CONN_TYPE == CHIP_CONN_TYPE_SPI)
        const jldtp_transport_t *dtp = jldtp_spi_transport_get(&dtp_config, configs[i].periph_cfg, configs[i].role);
#else

        const jldtp_transport_t *dtp = jldtp_uart_transport_get(&dtp_config, configs[i].periph_cfg, configs[i].role);
#endif

        mgr->dtp_handle = jldtp_create(&dtp_config);
        jldtp_register_transport(mgr->dtp_handle, dtp, configs[i].periph_cfg);

        mgr->mc_handle = jldtp_mc_create(mgr->dtp_handle, JLDTP_MC_CHANNEL_NUM);
        jldtp_mc_register_event_callback(mgr->mc_handle, jldtp_event_handler);

        jldtp_start(mgr->dtp_handle);

#if defined(JLDTP_TEST_ENABLE) && JLDTP_TEST_ENABLE
        jldtp_test_init(mgr->dtp_handle, configs[i].role, configs[i].mtu_size);
#endif
        // 打开命令传输channel
        jldtp_channel_config_t channel_config = {
            .channel_id = JLDTP_ID_CMD,
            .priority = JLDTP_MAX_PRIORITY_NUM - 1,
            .max_tx_buf_size = 128,
        };
        mgr->cmd_channel = jldtp_mc_open_channel(mgr->mc_handle, &channel_config);
        jldtp_mc_register_data_callback(mgr->cmd_channel, (void *)i, cmd_channel_data_handler);
    }

    audio_timer_init();

    r_printf("jldtp_start\n");

    return 0;
}


void jldtp_manager_deinit(void)
{
    for (int i = 0; i < manager_num; i++) {
        if (manager[i].mc_handle != NULL) {
            jldtp_mc_destroy(manager[i].mc_handle);
            manager[i].mc_handle = NULL;
        }
        if (manager[i].dtp_handle != NULL) {
            jldtp_destroy(manager[i].dtp_handle);
            manager[i].dtp_handle = NULL;
        }
    }
}

jldtp_mc_handle_t jldtp_manager_get_mc_handle(u8 chip_id)
{
    if (chip_id >= manager_num) {
        return NULL;
    }
    return manager[chip_id].mc_handle;
}

jldtp_handle_t jldtp_manager_get_dtp_handle(u8 chip_id)
{
    if (chip_id >= manager_num) {
        return NULL;
    }
    return manager[chip_id].dtp_handle;
}

jldtp_channel_t jldtp_manager_open_channel(u8 chip_id, jldtp_channel_config_t *config)
{
    jldtp_mc_handle_t mc_handle = jldtp_manager_get_mc_handle(chip_id);
    if (mc_handle) {
        return jldtp_mc_open_channel(mc_handle, config);
    }
    return NULL;
}

int jldtp_manager_get_role(u8 chip_id)
{
    if (chip_id >= manager_num) {
        return -EINVAL;
    }
    if (manager[chip_id].dtp_handle) {
        return jldtp_get_role(manager[chip_id].dtp_handle);
    }
    return JLDTP_MASTER;
}

void jldtp_manager_set_role(u8 chip_id, u8 role)
{
    if (chip_id >= manager_num) {
        return;
    }
    if (manager[chip_id].dtp_handle) {
        jldtp_set_role(manager[chip_id].dtp_handle, role);
    }
}

int jldtp_manager_send_msg(u8 chip_id, enum jldtp_cmd cmd, u32 arg)
{
    int buf[2] = {cmd, arg};
    return jldtp_mc_send_data(manager[chip_id].cmd_channel, (u8 *)buf, 8);
}

int jldtp_manager_send_data(u8 chip_id, enum jldtp_cmd cmd, u8 *data, u16 len)
{
    return jldtp_mc_send_data_with_head(manager[chip_id].cmd_channel, (u8 *)&cmd, 4, data, len);
}



/* ============================ JLDTP 统一测试模块 ============================
 *
 * 测试方式（停等协议,仅数据帧传输,无心跳/控制帧干扰）:
 *   主机: 发包 → 等待从机回传 → 收到回包后记录往返时间 → 发下一包
 *   从机: 收到数据 → 原样回传
 *
 * 包大小从 512 到最大 MTU,均分为 TEST_SIZE_GROUPS 组,每组测 TEST_PER_SIZE 次
 *
 * 关键设计:
 *   1. 在 data_callback 中排队下一包,使 tx_packet_num 恒 > 0,
 *      协议层 response 路径始终发数据帧而非 ctrl_frame
 *   2. 主机不进入 IDLE 态 → tick 不触发
 *   3. 所有组测完后不再排队数据包 → 协议自动恢复心跳
 *   4. 统一测试，不区分 SPI/UART
 *
 * 启用：在 jldtp_manager.h 中设置
 *   #define JLDTP_TEST_ENABLE 1
 */

#if defined(JLDTP_TEST_ENABLE) && JLDTP_TEST_ENABLE

#include "generic/jiffies.h"
#include "string.h"
#include "clock.h"


/* ======================== GPIO 翻转测试 (逻辑分析仪时序测量) ========================
 *
 * 引脚: PA7, PA8, PA9  →  初始化输出低，每次事件翻转一次
 *
 * 测量点 (角色通过 g_test.role 区分):
 *   ┌──────┬─────────────────────────────────┬────────┐
 *   │ 序号 │ 事件                            │ 引脚   │
 *   ├──────┼─────────────────────────────────┼────────┤
 *   │  1   │ 主机开始发数据 (write entry)     │ PA8,PA9│
 *   │  2   │ 主机 SPI 中断 (TX_FINISH)        │ PA8    │
 *   │  3   │ 从机 CS 中断 (CS ISR entry)      │ PA8    │
 *   │  4   │ 从机解析完数据 (rx_ready 返回)    │ PA10    │
 *   │  5   │ 从机调用发送 (write entry)        │ PA9*3    │
 *   │  6   │ 从机 SPI 中断 (TX_FINISH)        │ PA8    │
 *   │  7   │ 主机 CS 中断 (CS ISR entry)      │ PA8    │
 *   │  8   │ 主机解析完数据 (rx_ready 返回)    │ PA10    │
 *   └──────┴─────────────────────────────────┴────────┘
 */
#define GPIO_TEST_PA8  IO_PORTA_08
#define GPIO_TEST_PA9  IO_PORTA_09
#define GPIO_TEST_PA10  IO_PORTA_10


static void gpio_test_pins_init(void)
{
    gpio_set_mode(IO_PORT_SPILT(GPIO_TEST_PA8), PORT_OUTPUT_LOW);
    gpio_set_mode(IO_PORT_SPILT(GPIO_TEST_PA9), PORT_OUTPUT_LOW);
    gpio_set_mode(IO_PORT_SPILT(GPIO_TEST_PA10), PORT_OUTPUT_LOW);
}


#define TEST_PKT_SIZE_MIN   512          // 最小测试包长(字节)
#define TEST_PER_SIZE       10           // 每种包长测试次数
#define TEST_SIZE_GROUPS    5            // 测试包长分组数

struct jldtp_test {
    jldtp_handle_t handle;
    u8  role;
    u16 mtu;                             // 实际最大数据载荷(不含帧头)

    /* 当前测试进度 */
    u16 curr_size;                       // 当前测试的包大小
    u8  curr_count;                      // 当前大小已测次数 (0 ~ TEST_PER_SIZE-1)
    u8  curr_group;                      // 当前大小组索引 (0 ~ TEST_SIZE_GROUPS-1)

    /* 当前迭代计时 */
    u32 iter_start_ts;                   // 发送前时间戳 (us)

    /* 从机: 收/发字节累计 */
    u32 slave_rx_bytes;
    u32 slave_tx_bytes;

    /* 当前大小组的延迟采样 */
    u32 latency[TEST_PER_SIZE];          // 往返延迟 (us)

    /* 各组汇总 */
    struct {
        u16 size;                        // 包大小
        u32 avg_bw;                      // 平均带宽 (bps)
        u32 min_bw;                      // 最小带宽 (bps)
        u32 max_bw;                      // 最大带宽 (bps)
        u32 avg_latency;                 // 平均延迟 (us)
        u32 min_latency;                 // 最小延迟 (us)
        u32 max_latency;                 // 最大延迟 (us)
    } summary[TEST_SIZE_GROUPS];
};

static struct jldtp_test g_test;

/* --------------------------------------------------------------------------
 * 打印当前大小组的统计
 * -------------------------------------------------------------------------- */
static void test_print_size_stats(struct jldtp_test *t)
{
    u32 sum_latency = 0, min_lat = 0xFFFFFFFF, max_lat = 0;
    for (int i = 0; i < TEST_PER_SIZE; i++) {
        sum_latency += t->latency[i];
        if (t->latency[i] < min_lat) {
            min_lat = t->latency[i];
        }
        if (t->latency[i] > max_lat) {
            max_lat = t->latency[i];
        }
    }
    u32 avg_lat = sum_latency / TEST_PER_SIZE;

    /*
     * 带宽计算: 单向有效数据吞吐
     *   bw = data_bits / time_s
     *       = (curr_size * 8) / (latency_us / 1000000)
     *       = curr_size * 8000000 / latency_us
     */
    u64 size_bits = (u64)(t->curr_size + 8) * 2 * 8000000;
    u64 sum_bw = 0;
    u32 min_bw = 0xFFFFFFFF, max_bw = 0;
    for (int i = 0; i < TEST_PER_SIZE; i++) {
        u64 bw = t->latency[i] ? (u64)(size_bits / t->latency[i]) : 0;
        sum_bw += bw;
        if (bw < min_bw) {
            min_bw = bw;
        }
        if (bw > max_bw) {
            max_bw = bw;
        }
    }
    u32 avg_bw = (u32)(sum_bw / TEST_PER_SIZE);

    printf("========== [TEST] size=%d(+8)*2 bytes (%d tests) ==========\n",
           t->curr_size, TEST_PER_SIZE);
    printf("  Bandwidth:  avg=%d bps, min=%d bps, max=%d bps\n",
           avg_bw, min_bw, max_bw);
    printf("  Latency:    avg=%d us,  min=%d us,  max=%d us\n",
           avg_lat, min_lat, max_lat);

    /* 保存到汇总表 */
    int g = t->curr_group;
    if (g < TEST_SIZE_GROUPS) {
        t->summary[g].size        = t->curr_size + 8;
        t->summary[g].avg_bw      = avg_bw;
        t->summary[g].min_bw      = min_bw;
        t->summary[g].max_bw      = max_bw;
        t->summary[g].avg_latency = avg_lat;
        t->summary[g].min_latency = min_lat;
        t->summary[g].max_latency = max_lat;
    }
}

/* --------------------------------------------------------------------------
 * 打印最终汇总表
 * -------------------------------------------------------------------------- */
static void test_print_summary(struct jldtp_test *t)
{
    printf("========== [TEST] FINAL SUMMARY ==========\n");
#if (TCFG_MULTI_CHIP_CONN_TYPE == CHIP_CONN_TYPE_SPI)

    printf("spi freq:%d (%d/%d), SPI_CON:0x%x\n", clk_get("spi") / (JL_SPI1->BAUD + 1), clk_get("spi"), JL_SPI1->BAUD + 1, JL_SPI1->CON);
#else

    u8 divs_v = (JL_UART1->RX_CON0 & BIT(4)) ? 3 : 4;
    printf("uart freq:%d(%d/%d)\n", clk_get("uart") / (JL_UART1->BAUD + 1) / divs_v, clk_get("uart"), JL_UART1->BAUD + 1);
#endif
    printf("  MTU payload: %d bytes, groups: %d, per_group: %d\n",
           t->mtu, TEST_SIZE_GROUPS, TEST_PER_SIZE);
    printf("  Size(B) | Avg BW(bps) | Min BW(bps) | Max BW(bps) | "
           "Avg Lat(us) | Min Lat(us) | Max Lat(us)\n");
    printf("  --------+--------------+--------------+--------------+"
           "--------------+--------------+-------------\n");
    for (int i = 0; i < TEST_SIZE_GROUPS; i++) {
        if (t->summary[i].size == 0) {
            break;
        }
        printf("%7d*2 | %11d | %12d | %12d | "
               "%11d | %11d | %11d\n",
               t->summary[i].size,
               t->summary[i].avg_bw, t->summary[i].min_bw, t->summary[i].max_bw,
               t->summary[i].avg_latency, t->summary[i].min_latency, t->summary[i].max_latency);
    }
    if (t->role == JLDTP_SLAVE) {
        printf("  Slave total: rx=%d bytes, tx=%d bytes\n",
               t->slave_rx_bytes, t->slave_tx_bytes);
    }
    printf("==========================================\n");
}

/* --------------------------------------------------------------------------
 * 启动一次测试迭代: 分配缓冲区 → 记录时间戳 → 提交发送
 * -------------------------------------------------------------------------- */
static void test_start_iteration(struct jldtp_test *t)
{
    u16 len = t->curr_size;
    u8 *buf = jldtp_alloc_tx_buffer(t->handle, len);
    if (!buf) {
        printf("[TEST] alloc tx buffer(%d) failed\n", len);
        return;
    }

    /* 填充递增模式,方便从机校验 */
    for (u16 i = 0; i < len; i++) {
        buf[i] = (u8)(i & 0xFF);
    }

    /*
     * ★ 计时刻: 调应用层发送接口前记录时间戳
     *    止时刻: 收到从机回包时 (data_callback)
     */
    t->iter_start_ts = jiffies_usec();
    JLDTP_TEST_GPIO_TOGGLE_PA8();  /* 点1: 主机开始发数据,计时刻 */
    JLDTP_TEST_GPIO_TOGGLE_PA9();  /* 点1: 主机开始发数据,计时刻 */
    jldtp_commit_tx_buffer(t->handle, buf, len, 0);
}

/* --------------------------------------------------------------------------
 * 数据回调
 *   主机: 记录往返延迟 → 排队下一包 (保持 tx_packet_num > 0, 抑制 ctrl 帧)
 *   从机: 记录 rx 字节 → 原样回传
 * -------------------------------------------------------------------------- */
static int test_data_callback(void *user_data, jldtp_frame_t *frame)
{
    struct jldtp_test *t = (struct jldtp_test *)user_data;

    if (frame->data_len == 0) {
        return 0;
    }

    if (t->role == JLDTP_MASTER) {
        /* —— 计时刻终止: 记录往返延迟 —— */
        if (t->curr_count < TEST_PER_SIZE) {
            u32 now = jiffies_usec();
            t->latency[t->curr_count] = (now >= t->iter_start_ts)
                                        ? (now - t->iter_start_ts) : 0;
            t->curr_count++;
        }
        /* —— 当前大小组是否测完 —— */
        if (t->curr_count >= TEST_PER_SIZE) {
            test_print_size_stats(t);
            if (t->curr_group >= TEST_SIZE_GROUPS - 1) {
                /* 全部分组完成 → 打印汇总 → 不再排队数据包 → 协议恢复 IDLE/心跳 */
                test_print_summary(t);
                return 0;
            }
            /* 切换到下一大小组 */
            t->curr_group++;
            u16 step = (t->mtu - TEST_PKT_SIZE_MIN) / (TEST_SIZE_GROUPS - 1);
            if (t->curr_group < TEST_SIZE_GROUPS - 1) {
                t->curr_size = TEST_PKT_SIZE_MIN + step * t->curr_group;
            } else {
                t->curr_size = t->mtu;
            }
            t->curr_count = 0;
            memset(t->latency, 0, sizeof(t->latency));
            printf("[TEST] start group %d/%d, size=%d bytes\n",
                   t->curr_group + 1, TEST_SIZE_GROUPS, t->curr_size);
        }

        /*
         * 排队下一包。
         * data_callback 在 jldtp_notify_rx_ready 中调用,早于 IDLE 判定:
         *   此时 tx_pending==NULL(已被 TX_SUSS 清除)，排队后 tx_packet_num>0,
         *   协议层 response 路径会发数据帧而非 ctrl_frame,主机不进入 IDLE → tick 不触发。
         */
        test_start_iteration(t);
    } else {
        /* —— 从机: 记录接收字节并原样回传 —— */
        t->slave_rx_bytes += frame->data_len;
        u8 *buf = jldtp_alloc_tx_buffer(t->handle, frame->data_len);
        if (buf) {
            memcpy(buf, frame->data, frame->data_len);
            JLDTP_TEST_GPIO_TOGGLE_PA9();  /* 点5: 从机调用发送 */
            JLDTP_TEST_GPIO_TOGGLE_PA9();  /* 点5: 从机调用发送 */
            JLDTP_TEST_GPIO_TOGGLE_PA9();  /* 点5: 从机调用发送 */
            jldtp_commit_tx_buffer(t->handle, buf, frame->data_len, 0);
            t->slave_tx_bytes += frame->data_len;
        } else {
            printf("[TEST] slave alloc echo buf(%d) failed\n", frame->data_len);
        }
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * 事件回调
 * -------------------------------------------------------------------------- */
static void test_event_callback(void *user_data, jldtp_event_t event,
                                int arg1, int arg2)
{
    struct jldtp_test *t = (struct jldtp_test *)user_data;
    switch (event) {
    case JLDTP_EVENT_CONNECTED:
        printf("[TEST] connected, mtu=%d\n", jldtp_get_mtu_size(t->handle));
        t->mtu = (u16)jldtp_get_mtu_size(t->handle);
        if (t->role == JLDTP_MASTER) {
            /* 初始化: 从 512 字节开始 */
            t->curr_group = 0;
            t->curr_size  = TEST_PKT_SIZE_MIN;
            t->curr_count = 0;
            memset(t->latency, 0, sizeof(t->latency));
            memset(t->summary, 0, sizeof(t->summary));
            printf("[TEST] start group 1/%d, size=%d bytes\n",
                   TEST_SIZE_GROUPS, t->curr_size);
            test_start_iteration(t);
        }
        break;
    case JLDTP_EVENT_DISCONNECTED:
        printf("[TEST] disconnected, group=%d, size=%d, count=%d\n",
               t->curr_group + 1, t->curr_size, t->curr_count);
        break;
    case JLDTP_EVENT_TX_SUSS:
        /* TX_SUSS 仅表示 ACK 确认,往返时间在 data_callback 记录 */
        break;
    case JLDTP_EVENT_TX_TIMEOUT:
        printf("[TEST] tx timeout (group=%d, size=%d, count=%d)\n",
               t->curr_group + 1, t->curr_size, t->curr_count);
        /* 超时不阻塞测试: 尝试继续下一轮 */
        if (t->role == JLDTP_MASTER && t->curr_count < TEST_PER_SIZE) {
            t->curr_count++; /* 跳过本次 */
            test_start_iteration(t);
        }
        break;
    }
}

/* --------------------------------------------------------------------------
 * 初始化测试模块
 *   handle: jldtp 句柄
 *   role  : JLDTP_MASTER / JLDTP_SLAVE
 *   mtu   : (保留参数,实际使用 jldtp_get_mtu_size 获取)
 * -------------------------------------------------------------------------- */
void jldtp_test_init(jldtp_handle_t handle, u8 role, u16 mtu)
{
    // irq_unmask_set(IRQ_SPI1_IDX, 3, 0);
    // irq_unmask_set(IRQ_PORT_IDX, 3, 0);
    memset(&g_test, 0, sizeof(g_test));
    g_test.handle = handle;
    g_test.role   = role;
    g_test.mtu    = (u16)jldtp_get_mtu_size(handle);
    if (g_test.mtu < TEST_PKT_SIZE_MIN) {
        printf("[TEST] mtu(%d) < min(%d), test disabled\n",
               g_test.mtu, TEST_PKT_SIZE_MIN);
        return;
    }
    jldtp_register_callback(handle, &g_test, test_data_callback, test_event_callback);

    gpio_test_pins_init();

    printf("[TEST] init done, role=%s, mtu=%d, groups=%d, per_group=%d\n",
           role == JLDTP_MASTER ? "MASTER" : "SLAVE",
           g_test.mtu, TEST_SIZE_GROUPS, TEST_PER_SIZE);
}

/*
 * 停止测试模块
 */
void jldtp_test_deinit(void)
{
    memset(&g_test, 0, sizeof(g_test));
    printf("[TEST] deinit\n");
}

#endif /* JLDTP_TEST_ENABLE */

#endif
