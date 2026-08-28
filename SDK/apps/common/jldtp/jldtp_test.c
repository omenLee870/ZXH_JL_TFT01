#include "jldtp/jldtp_manager.h"
#include "system/task.h"
#include "system/init.h"
#include "system/timer.h"
#include "generic/jiffies.h"


#define JLDTP_SPEED_TEST 0
#define JLDTP_CHANNEL_CLOSE_TEST 0




#if JLDTP_SPEED_TEST
static jldtp_channel_handle_t test_channel;
static u8 test_buf[2048] __attribute__((aligned(4)));
static u32 tx_seqn = 1;
static u32 rx_seqn = 1;
static u32 start_time = 0;
static u16 tx_timer;
static u8 rx_frame_cnt = 0;
static struct list_head rx_frame_head = LIST_HEAD_INIT(rx_frame_head);

extern u32 jldtp_usec;

static jldtp_result_t test_channel_data_handler(void *user_data, jldtp_frame_t *frame, u8 *data, u16 len)
{
    u32 *value = (u32 *)data;
    if (value[0] != rx_seqn) {
        r_printf("jldtp_rx_err: seqn %d, %d\n", value[0], rx_seqn);
    } else {
        if ((value[0] % 100) == 0) {
            int time = jiffies_offset_to_msec(start_time, jiffies);
            int speed = (100 * len * 8) / time;
            start_time = jiffies;
            g_printf("jldtp_rx_suss: %d, %dkbps\n", value[0], speed);
        }
        /*if ((value[0] % 20) == 0) {
            printf("usec: %d\n", jldtp_usec);
        }*/
    }
    rx_seqn = value[0] + 1;

    // 延时释放测试
    list_add_tail(&frame->entry, &rx_frame_head);
    if (++rx_frame_cnt >= 5) {
        rx_frame_cnt--;
        frame = list_first_entry(&rx_frame_head, jldtp_frame_t, entry);
        list_del(&frame->entry);
        free(frame);
    }
    return JLDTP_FRAME_BUSY;
}

void jldtp_channel_test_send_data()
{
    u32 *buf = (u32 *)test_buf;

    if (start_time == 0) {
        start_time = jiffies;
    }
    while (1) {
        buf[0] = tx_seqn;
        jldtp_result_t err = jldtp_mc_send_data(test_channel, test_buf, 1920);
        if (err == JLDTP_SUCCESS) {
            tx_seqn++;
        } else {
            /*putchar('F');*/
            break;
        }
    }
}

int jldtp_channel_test_init()
{
    jldtp_mc_handle_t mc_handle = jldtp_manager_get_mc_handle(JLDTP_WITH_CHIP0);

    jldtp_channel_config_t config = {
        .channel_id = 50,
        .priority = 0,
        .max_tx_buf_size = 6 * 1024,
        .task_name = "app_core",
    };
    test_channel = jldtp_mc_open_channel(mc_handle, &config);

    jldtp_mc_register_data_callback(test_channel, NULL, test_channel_data_handler);

    if (jldtp_mc_get_role(mc_handle) == JLDTP_MASTER) {
        sys_hi_timer_add(NULL, jldtp_channel_test_send_data, 5);
    }
    return 0;
}
late_initcall(jldtp_channel_test_init);

#endif



#if JLDTP_CHANNEL_CLOSE_TEST
static u32 close_test_tx_seqn = 0;
static jldtp_channel_handle_t channel_a;
static void timer_close_channel(void *p);

static jldtp_result_t channel_close_callback(void *user_data, jldtp_frame_t *frame, u8 *data, u16 len)
{
    printf("jldtp_rx: %d\n", *(u32 *)data);
    return JLDTP_SUCCESS;
}

void channel_close_test_send_data()
{
    jldtp_mc_send_data(channel_a, (u8 *)&close_test_tx_seqn, 4);
    close_test_tx_seqn++;
}

static void timer_open_channel(void *p)
{
    jldtp_mc_handle_t mc_handle = jldtp_manager_get_mc_handle(JLDTP_WITH_CHIP0);
    jldtp_channel_config_t config = {
        .channel_id = 100,
        .priority = 0,
        .max_tx_buf_size = 128,
        .task_name = "app_core",
    };
    channel_a = jldtp_mc_open_channel(mc_handle, &config);

    jldtp_mc_register_data_callback(channel_a, NULL, channel_close_callback);
    puts("--test_channel_open\n");

    sys_timeout_add(NULL, timer_close_channel, 190);
}

static void timer_close_channel(void *p)
{
    puts("test_channel_close\n");
    jldtp_mc_close_channel(channel_a);
    sys_timeout_add(NULL, timer_open_channel, 180);
}

int jldtp_channel_close_test_init()
{
    jldtp_mc_handle_t mc_handle = jldtp_manager_get_mc_handle(JLDTP_WITH_CHIP0);

    jldtp_channel_config_t config = {
        .channel_id = 100,
        .priority = 0,
        .max_tx_buf_size = 128,
        .task_name = "app_core",
    };
    channel_a = jldtp_mc_open_channel(mc_handle, &config);

    jldtp_mc_register_data_callback(channel_a, NULL, channel_close_callback);

    if (jldtp_mc_get_role(mc_handle) == JLDTP_MASTER) {
        sys_hi_timer_add(NULL, channel_close_test_send_data, 55);
    } else {
        sys_timeout_add(NULL, timer_close_channel, 120);
    }
    return 0;
}
late_initcall(jldtp_channel_close_test_init);
#endif

