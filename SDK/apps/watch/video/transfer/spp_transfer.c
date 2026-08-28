#include "system/includes.h"
#include "app_config.h"
#include "app_msg.h"
#include "btstack/avctp_user.h"
#include "app_ble_spp_api.h"
#include "JL_rcsp_packet.h"
#include "vstream_common.h"
#include "spp_transfer.h"
#include "kid_wt_app.h"

#define LOG_TAG             "[KWT_SPP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_VSTREAM_TRANSFER
//蓝牙库 相关控制
unsigned int user_emitter_set_profile_judge = 0;

//蓝牙库函数声明
extern void rfcomm_change_credits_setting(u16 init_credits, u8 base);
extern void set_bt_tx_buf_max_size(int max_size);
extern int user_spp_data_packet_cache_get(bd_addr_t addr);
extern u32 get_bredr_tx_remain_size();
extern u8 get_role_type_by_addr(u8 *addr);
extern void spp_user_max_sending_packet_num_set(u8 max_num);
///---接收器还是发射器---///
#define ROLE_RECEIVER     0
#define ROLE_EMITTER      1
#define ROLE_UNACTIVE     2   //一拖二的时候不活跃设备


#define SPP_MAX_SENDING_PACKET_NUM  20  //蓝牙底层spp发送缓存
#define DATA_TRANSFER_TEST  0    //直接spp传输数据测试
#define PACKET_TRANS_TEST   0   //分包传输测试


static void *spp_hdl = NULL;


#if KID_WT_FPS_TEST
static int rx_data_len;
static u16 fps_tid;
static void fps_test_timer_open(void);
static void fps_test_timer_close(void);
// extern spinlock_t fps_test_lock;
#endif

#if DATA_TRANSFER_TEST
#define PACKAGE_SIZE 650    //目前spp最大有效数据量 byte
#define PACKET_PERIOD 7     //spp发送最短间隔7ms
static u16 send_tid;
static u8 send_test_data[PACKAGE_SIZE];
void data_transfer_test_open(void);
void data_transfer_test_close(void);
#endif

#if PACKET_TRANS_TEST
static u16 packet_tid;
#define PACKET_TEST_PERIOD 3*1000     //spp发送最短间隔7ms
void packet_transfer_test_open(void);
void packet_transfer_test_close(void);
#endif

u8 kwt_spp_connected = 0;

static void spp_state_callback(void *hdl, void *remote_addr, u8 state)
{
    printf("[KWT_SPP] state_cb: state=%d\n", state);
    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("kwt spp connect#########\n");
        // 将 custom_demo_spp_hdl 绑定到连接上的设备地址，否则后续会收到所有已连接设备地址的事件和数据
        app_spp_set_filter_remote_addr(spp_hdl, remote_addr);
        kwt_spp_connected = 1;

#if TCFG_VSTREAM_TRANSFER && !DATA_TRANSFER_TEST
        {
            int msg[3] = {0};
            extern void kwt_stream_start(void);
            msg[0] = (int)kwt_stream_start;
            os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        }
#endif
#if DATA_TRANSFER_TEST
        {
            int msg[3] = {0};
            msg[0] = (int)data_transfer_test_open;
            os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        }
#endif
#if PACKET_TRANS_TEST
        {
            int msg[3] = {0};
            msg[0] = (int)packet_transfer_test_open;
            os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        }
#endif//PACKET_TRANS_TEST
#if KID_WT_FPS_TEST
        {
            int msg_fps[3] = {0};
            msg_fps[0] = (int)fps_test_timer_open;
            os_taskq_post_type("app_core", Q_CALLBACK, 3, msg_fps);
        }
#endif
        break;
    case SPP_USER_ST_DISCONN:
        kwt_spp_connected = 0;
        log_info("kwt spp disconnect#########\n");

#if DATA_TRANSFER_TEST
        data_transfer_test_close();
#endif
#if PACKET_TRANS_TEST
        packet_transfer_test_close();
#endif
#if KID_WT_FPS_TEST
        fps_test_timer_close();
#endif
#if TCFG_VSTREAM_TRANSFER
        {
            /* 媒体停止不能在 btstack 线程同步执行: kwt_stream_stop 内部
             * thread_kill(KILL_WAIT) 会等待 kwt_video_cap 线程退出, 而该线程
             * vstream_send_frame 正阻塞在视频 lbuf 互斥锁上, 锁又被
             * vstream_pkg_send_task 持有, 而该任务在 user_spp_data_packet_count_get()
             * 里 post 到 btstack 并 sem_pend 等 btstack 响应 —— 若 btstack 卡在
             * thread_kill 里就形成 3 方循环死锁。故与 CONNECT 分支一致, 把 stop
             * 封送到 app_core, 让 btstack 线程立即返回去应答 pkg_send_task。 */
            int msg[3] = {0};
            extern void kwt_stream_stop(void);
            msg[0] = (int)kwt_stream_stop;
            os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        }
#endif
        break;
    };
}

static void spp_recieve_callback(void *hdl, void *remote_addr, u8 *buf, u16 len)
{
    wdt_clear();
    /* log_info("%s len=%d\n", __func__, len); */
    /* put_buf(buf, len); */

#if KID_WT_FPS_TEST
    // spin_lock(&fps_test_lock);
    rx_data_len += len;
    // spin_unlock(&fps_test_lock);
#endif
#if !DATA_TRANSFER_TEST
    vstream_spp_rx_handler(buf, len);
#endif
}

//主机数据发送
static int spp_emitter_send(u8 *data, u32 len)
{
    if (!(bt_emitter_get_curr_channel_state() & SPP_CH) || !spp_hdl) {
        log_error("[%s %d] %x %p", __func__, __LINE__, bt_emitter_get_curr_channel_state(), spp_hdl);
        return -1;
    }
    return bt_emitter_cmd_prepare(USER_CTRL_SPP_SEND_DATA, len, data);
}

//从机数据发送
static int spp_receiver_send(u8 *data, u32 len)
{
    if (!(bt_get_curr_channel_state() & SPP_CH) || !spp_hdl) {
        log_error("[%s %d] %x %p", __func__, __LINE__, bt_get_curr_channel_state(), spp_hdl);
        return -1;
    }
    return app_spp_data_send(spp_hdl, data, len);
}

int spp_send(u8 *data, u32 len)
{
    int ret;

    /* 1s 心跳: 打印 BR/EDR TX 缓冲剩余空间, 用于判断断线重连后 TX 是被对端
     * RFCOMM 信用失步"钉死"(值恒定 <1024) 还是缓慢排空(值在波动但排空速率
     * 远低于发送速率)。仅在发送路径(spp_send 被调用时)打印, 不影响静默期。
     *
     * 同时统计有界等待次数/等待时长/等不到失败的次数, 直接量化"连接刚建立时
     * TX 拥塞导致卡顿"这一假设: 若 wait_cnt/wait_ticks 在连接头几秒显著高、
     * 之后回落到 0, 则说明卡顿确由 TX 拥塞(信用流控爬坡)导致。 */
    static u32 tx_dump_ms = 0;
    static u32 tx_wait_cnt = 0;      /* 1s 内进入有界等待的次数 */
    static u32 tx_wait_ticks = 0;    /* 1s 内等待的总 tick 数(1 tick=10ms) */
    static u32 tx_wait_fail = 0;     /* 1s 内等不到排空而失败的次数 */
    u32 now_ms = jiffies_msec();
    if (jiffies_msec2offset(tx_dump_ms, now_ms) >= 1000) {
        log_info("%s tx_remain=%u wait_cnt=%u wait_ms=%u wait_fail=%u\n",
                 __func__, get_bredr_tx_remain_size(),
                 tx_wait_cnt, tx_wait_ticks * 10, tx_wait_fail);
        tx_dump_ms = now_ms;
        tx_wait_cnt = 0;
        tx_wait_ticks = 0;
        tx_wait_fail = 0;
    }

    // THIS IS BROKE AFTER BTSTACK UPDATE!!!!!
    //if (!(bt_get_curr_channel_state() & SPP_CH) || !spp_hdl) {
    if ((kwt_spp_connected == 0) || !spp_hdl) {
        log_error("%s %d %p", __func__, (bt_get_curr_channel_state() & SPP_CH), spp_hdl);
        ret = -1;
        return ret;
    }

    if (get_bredr_tx_remain_size() < SPP_TX_MIN_RAM_LIMIT) {
        /* TX 缓冲不足: 有界等待其排空, 而不是立即返回 -1 丢整帧。spp_send 只在
         * vstream_pkg_send_task 上下文调用, os_time_dly 安全。os_time_dly 参数是
         * OS tick(1 tick=10ms), 用 SPP_TX_WAIT_TIMEOUT_TICKS 限界。等不到才失败,
         * 把背压转移到生产者(按排空速率丢整帧)而不是消费者(把帧拆一半)。 */
        int wait_ticks = 0;
        tx_wait_cnt++;
        while (get_bredr_tx_remain_size() < SPP_TX_MIN_RAM_LIMIT
               && wait_ticks < SPP_TX_WAIT_TIMEOUT_TICKS) {
            os_time_dly(1);
            wait_ticks++;
        }
        tx_wait_ticks += wait_ticks;
        if (get_bredr_tx_remain_size() < SPP_TX_MIN_RAM_LIMIT) {
            tx_wait_fail++;
            log_error("%s %d err! %d %d", __func__, __LINE__, get_bredr_tx_remain_size(), user_spp_data_packet_count_get(kwt_get_connect_addr()));
            return -1;
        }
    }

    u8 role_type = get_role_type_by_addr(kwt_get_connect_addr());
    if (role_type == ROLE_RECEIVER) {
        ret = spp_receiver_send(data, len);
    } else if (role_type == ROLE_EMITTER) {
        ret = spp_emitter_send(data, len);
    } else {
        log_error("[%s %d] role_type:%d", __func__, __LINE__, role_type);
        ret = -1;
    }

    if (ret) {
        log_error("%s error! %d %d %s", __func__, len, ret, os_current_task());
    } else {
        /* log_debug("%s succ(len:%d)\m", __func__, len); */
    }
    return ret;
}

int user_spp_data_packet_total_get(void)
{
    return SPP_MAX_SENDING_PACKET_NUM;
}

#define LOOP_CNT_MAX    150
int user_spp_data_packet_count_get(bd_addr_t addr)
{
    if (strcmp(os_current_task(), "btstack") != 0) {
        OS_SEM *sem = malloc(sizeof(OS_SEM) + 4);
        int *count = (int *)(sem + 1);
        int msg[8];

        msg[0] = (int)user_spp_data_packet_cache_get;
        msg[1] = 1 | BIT(8) | BIT(9);
        msg[2] = (int)addr;
        msg[3] = (int)count;
        msg[4] = (int)sem;

        os_sem_create(sem, 0);
        int cur_count = 0;
        int ret_err;
        int loop_count = 0;
        while (loop_count < LOOP_CNT_MAX) {
            ret_err = os_taskq_post_type("btstack", Q_CALLBACK, 5, msg);
            if (ret_err == OS_ERR_NONE) {
                os_sem_pend(sem, 0);
                cur_count = *count;
                break;
            }

            if (ret_err != OS_Q_FULL) {
                log_error("%s %d ret_err:%d", __func__, __LINE__, ret_err);
                break;
            }
            ++loop_count;
            os_time_dly(2);
        }
        if (loop_count >= LOOP_CNT_MAX) {
            log_error("%s %d timeout error!!!", __func__, __LINE__);
        }
        free(sem);
        return cur_count;
    }
    return user_spp_data_packet_cache_get(addr);
}

void spp_protocol_init(void)
{
    if (spp_hdl == NULL) {
        spp_hdl = app_spp_hdl_alloc(0x0);
        if (spp_hdl == NULL) {
            log_error("%s spp_hdl alloc err !\n", __func__);
            return;
        }
        app_spp_recieve_callback_register(spp_hdl, spp_recieve_callback);
        app_spp_state_callback_register(spp_hdl, spp_state_callback);
        app_spp_wakeup_callback_register(spp_hdl, NULL);
        rfcomm_change_credits_setting(0x30, 0x15);  //只修改base, init_credits用的还是默认参数值
        /* Fix C: 不再把 TX FIFO 提到 15KB。该接口实为 flag 选择 5KB->15KB, 15KB 是
         * bufferbloat —— 只增加排队在途延迟(0.6-1.0s)而不增加吞吐。用默认 5KB FIFO,
         * 使延迟降约 3 倍(0.2-0.35s), 与源端背压(Fix A)配合。 */
        /* set_bt_tx_buf_max_size(15 * 1024); */
        log_info("%s succ!", __func__);
    }
}

void spp_protocol_deinit(void)
{
    if (NULL != app_spp_get_hdl_remote_addr(spp_hdl)) {
        app_spp_disconnect(spp_hdl);
    }
    app_spp_hdl_free(spp_hdl);
    spp_hdl = NULL;
}

#if KID_WT_FPS_TEST
int fps_win_counter_get_fps();
int fps_win_counter_get_fps_rx();
int fps_win_counter_get_fps_audio_rx();
static void fps_test(void *p)
{
    // spin_lock(&fps_test_lock);
    int rx_rate = rx_data_len * 8 / 1024;
    rx_data_len = 0;
    /*info: video_fps_tx | video_fps_rx | audio_fps_rx | rx_rate | rssi*/
    // printf("info: %3d|%3d|%3d|%3d|%3d|", fps_win_counter_get_fps(), fps_win_counter_get_fps_rx(), \
    //        fps_win_counter_get_fps_audio_rx(), rx_rate, bredr_get_rssi_for_address(kwt_get_connect_addr()));
    log_info("rx_rate:%d kbps", rx_rate);
    // spin_unlock(&fps_test_lock);

}

static void fps_test_timer_open(void)
{
    if (!fps_tid) {
        fps_tid = sys_timer_add(NULL, fps_test, 1000);
    }
}

static void fps_test_timer_close(void)
{
    if (fps_tid) {
        sys_timer_del(fps_tid);
        fps_tid = 0;
    }
}
#endif

#if DATA_TRANSFER_TEST

void data_transfer_test_cb(void *p)
{
    u32 get_bredr_tx_remain_size();
    if (get_bredr_tx_remain_size() < 1 * 1024) {
        log_error("%s %d return", __func__, get_bredr_tx_remain_size());
        return;
    }
    spp_send(send_test_data, sizeof(send_test_data));
}

extern void os_system_info_output(void);
void os_system_info_reset(void);
static void task_info(void *priv)
{
    os_system_info_output();
    os_system_info_reset();
}

void data_transfer_test_open(void)
{
    log_info("%s %d", __func__, __LINE__);
    /* sys_timer_add(NULL, task_info, 3 * 1000); */
    if (!send_tid) {
        send_tid = sys_timer_add(NULL, data_transfer_test_cb, PACKET_PERIOD);
    }
}

void data_transfer_test_close(void)
{
    if (send_tid) {
        sys_timer_del(send_tid);
        send_tid = 0;
    }
}
#endif
#if PACKET_TRANS_TEST
void packet_transfer_test_cb(void *p)
{
    int jpeg_data_len = 2 * 1024;
    u8 *jpeg_data = malloc(jpeg_data_len);
    for (int i = 0; i < jpeg_data_len; i++) {
        jpeg_data[i] = i;
    }
    vstream_send_frame(jpeg_data, jpeg_data_len, VSTREAM_JPEG_TYPE_VIDEO);
    free(jpeg_data);
    printf("%s %d", __func__, __LINE__);
}
void packet_transfer_test_open(void)
{
    printf("%s %d", __func__, __LINE__);
    if (!packet_tid) {
        printf("%s %d", __func__, __LINE__);
        vstream_sender_video_init();
        packet_tid = sys_timer_add(NULL, packet_transfer_test_cb, PACKET_TEST_PERIOD);
    }

}
void packet_transfer_test_close(void)
{
    printf("%s %d", __func__, __LINE__);
    if (packet_tid) {
        sys_timer_del(packet_tid);
        packet_tid = 0;
        vstream_sender_video_deinit();
    }
}
#endif//PACKET_TRANS_TEST
#endif /*#if TCFG_VSTREAM_TRANSFER*/

