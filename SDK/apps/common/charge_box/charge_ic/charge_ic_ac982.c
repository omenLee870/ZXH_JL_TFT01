#include "chargeIc_manage.h"
#include "device/device.h"
#include "app_config.h"
#include "app_main.h"
#include "user_cfg.h"
#include "chgbox_det.h"
#include "chgbox_ctrl.h"
#include "chgbox_wireless.h"
#include "smartbox_info_manager.h"
#include "uart.h"

#if (defined TCFG_CHARGE_IC_AC982 &&  TCFG_CHARGE_IC_AC982)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHG_IC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
/************************************************************************************
 * macro define
 ************************************************************************************/
#define PMU_CH_VALUE_ARRAY_SIZE   (16 + 1)
#define PMU_CH_SAMPLE_FREQ    500 //ms

#define UART_MAGIC_NUM_1      0X55
#define UART_MAGIC_NUM_2      0XAA
#define SBOX_UART_RX_DAT		0
#define SBOX_UART_TX_DAT		1
/************************************************************************************
 * static global variables
 ************************************************************************************/
typedef enum {
    SALVE_2_MASTER,
    MASTER_2_SALVE,
} UART_DIRECTION;
typedef enum {
    UART_CMD_BATTERY,
    UART_CMD_CHARGING,
    UART_CMD_HALL,
    UART_CMD_MAC,
    CMD_BATTERY_F98,
    UART_CMD_BUTTON,
    UART_USER_ADD,
    CMD_PRODUCT_MODE,

    UART_CMD_EAR_HW_ST = 0x09,
    UART_CMDD_TEST,

    UART_CMD_NULL = 0xFF,
} UART_CMD_TYPE;

struct msg_hall {
    u8 bar_full;
    u8 eal_power_l;
    u8 eal_power_r;
    u8 ear_sta_charge;
    u8 sta_hall;
};

struct msg_charge {
    u8 power_vol_h;
    u8 power_vol_l;
    u8 local_charge;
    u8 power_current_h;
    u8 power_current_l;
};

typedef struct {
    u8 magic_1;
    u8 magic_2;
    u8 type;
    u8 direct;
    u8 length;
    u8 payload[7];
} uart_packet_t;//12 bytes

static struct sbox_uart_ctrl {
    uart_packet_t packet[1];
    uart_packet_t txbuf[1];
    u8 uart_dev;
    bool get_vbat_value_from_f98 ;
    bool get_vbat_a_from_f98;
    u8 uart_into_new_sleep;
    u16 tx_ack_timer;
    u16 vbat_voltage_array_f98[PMU_CH_VALUE_ARRAY_SIZE];
    u16 vbat_voltage_array[PMU_CH_VALUE_ARRAY_SIZE];
    u32 uart_rx_ptr[16];
} __ctrl = {
    .uart_into_new_sleep = 1,
};

#define __this (&__ctrl)

/************************************************************************************
 * extern functions
 ************************************************************************************/
extern void vbat_voltage_array_f98_init(void);
extern void vbat_voltage_array_f98_uninit(void);
extern void f98_vbat_to_array(u16 vbat_f98);
extern void user_check_into_scan(void *p);
void vbat_voltage_array_f98_init(void)
{
    memset(__this->vbat_voltage_array_f98, 0, PMU_CH_VALUE_ARRAY_SIZE * sizeof(u16));
}

void vbat_voltage_array_f98_uninit(void)
{
    memset(__this->vbat_voltage_array, 0, PMU_CH_VALUE_ARRAY_SIZE * sizeof(u16));
}

u32 adc_get_value_f98(void)
{
    y_printf("DDD-vbat_voltage_array_f98:");
    put_buf((u8 *)__this->vbat_voltage_array_f98, PMU_CH_VALUE_ARRAY_SIZE * sizeof(u16));
    u32 i, j, sum = 0;
    for (i = 1, j = 0; i < PMU_CH_VALUE_ARRAY_SIZE; i++) {
        if (__this->vbat_voltage_array[i]) {
            sum += __this->vbat_voltage_array[i];
            j += 1;
        }
    }
    if (sum) {
        return (sum / j);
    }
    return 1;
}

// 8s更新一次
void f98_vbat_to_array(u16 vbat_f98)
{
    u16 temp_buf[PMU_CH_VALUE_ARRAY_SIZE - 1] = {0};
    memcpy(temp_buf, __this->vbat_voltage_array_f98, (PMU_CH_VALUE_ARRAY_SIZE - 1) * 2);
    memcpy(&__this->vbat_voltage_array_f98[1], temp_buf, (PMU_CH_VALUE_ARRAY_SIZE - 1) * 2);
    __this->vbat_voltage_array_f98[0] = vbat_f98;
    r_printf("F98_new_avg_buf:");
    put_buf((u8 *)__this->vbat_voltage_array_f98, PMU_CH_VALUE_ARRAY_SIZE * sizeof(u16));
    r_printf("F98_avg_vbat: 0x%x", adc_get_value_f98());
}

static void uart_irq_func(int uart_num, enum uart_event event)
{
    u32 rx_flag = 0;
    if (event & UART_EVENT_TX_DONE) {
        printf("uart[%d] tx done", uart_num);
    }
    if (event & UART_EVENT_RX_TIMEOUT) {
        printf("uart[%d] rx timerout data", uart_num);
    }
    if (event & UART_EVENT_RX_FIFO_OVF) {
        printf("uart[%d] rx fifo ovf", uart_num);
    }
    if (event & UART_EVENT_RX_DATA) {
        rx_flag = 1;
        printf("uart[%d] UART_EVENT_RX_DATA", uart_num);
    }
    if (rx_flag) {
        int ret = os_taskq_post_msg("sbox_uart_task", 1, SBOX_UART_RX_DAT);
        if (ret) {
            log_error("%s ret:%d", __func__, ret);
        }
    }
}
void packet_dump(uart_packet_t *packet)
{
    printf("magic1:0x%x magic2:0x%x type:0x%x dir:0x%x len:0x%x \n",
           packet->magic_1, packet->magic_2, packet->type, packet->direct, packet->length);
    printf("payload:");
    put_buf((u8 *)packet->payload, 7);
}
static void uart_ack_timeout_cb()
{
    static u8 ack_timeout_cnt = 0;
    log_info("uart ack timeout!!!!\n");
    ack_timeout_cnt++;
    if (ack_timeout_cnt > 3) {
        log_info("ack_timeout_cnt > MAX !!!!\n");
        ack_timeout_cnt = 0;
        return;
    }
    uart_send_blocking(__this->uart_dev, __this->txbuf, sizeof(uart_packet_t), 20);
    log_info("ot tx_data[%d]:\n", sizeof(uart_packet_t));
    put_buf((u8 *)__this->txbuf, sizeof(uart_packet_t));
}
static void sbox_uart_tx_data_process(int *msg)
{
    u8 type = msg[2];
    uart_packet_t tx_msg;
    uart_packet_t *uart_txbuf = &tx_msg;
    switch (msg[2]) {
    case UART_CMD_CHARGING:
        log_info("UART_CMD_SCREEN\n");
        uart_txbuf->magic_1 = UART_MAGIC_NUM_1;
        uart_txbuf->magic_2 = UART_MAGIC_NUM_2;
        uart_txbuf->type = UART_CMD_CHARGING;
        uart_txbuf->direct = MASTER_2_SALVE;
        uart_txbuf->length = 0X01;
        uart_txbuf->payload[0] = msg[3];
        break;
    case UART_CMD_MAC:
        log_info("UART_CMD_MAC\n");
        uart_txbuf->magic_1 = UART_MAGIC_NUM_1;
        uart_txbuf->magic_2 = UART_MAGIC_NUM_2;
        uart_txbuf->type = UART_USER_ADD;
        uart_txbuf->direct = MASTER_2_SALVE;
        uart_txbuf->length = 0X01;
        uart_txbuf->payload[0] = 0xac;//msg[3];
        break;
    case CMD_PRODUCT_MODE:
        log_info("CMD_PRODUCT_MODE\n");
        uart_txbuf->magic_1 = UART_MAGIC_NUM_1;
        uart_txbuf->magic_2 = UART_MAGIC_NUM_2;
        uart_txbuf->type = CMD_PRODUCT_MODE;
        uart_txbuf->direct = MASTER_2_SALVE;
        uart_txbuf->length = 0X01;
        uart_txbuf->payload[0] = 0x01;//msg[3];
        break;
    default:
        break;
    }
    if (__this->tx_ack_timer) {
        sys_timeout_del(__this->tx_ack_timer);
        __this->tx_ack_timer = 0;
    }
    memcpy(__this->txbuf, uart_txbuf, sizeof(uart_packet_t));
    uart_send_blocking(__this->uart_dev, uart_txbuf, sizeof(uart_packet_t), 1);
    log_info("tx_data[%d]:\n", sizeof(uart_packet_t));
    put_buf((u8 *)__this->txbuf, sizeof(uart_packet_t));
    __this->tx_ack_timer = sys_timeout_add(NULL, uart_ack_timeout_cb, 200);
}

void sbox_uart_rx_data_process()
{
    int msg[3];
    memset(__this->packet, 0, sizeof(uart_packet_t));
    uart_recv_bytes(__this->uart_dev, __this->packet, sizeof(uart_packet_t));
    packet_dump(__this->packet);
    // 校验
    if (__this->packet->magic_1 != UART_MAGIC_NUM_1 ||
        __this->packet->magic_2 != UART_MAGIC_NUM_2) {
        log_info("magic incorrect :%x %x!\n", __this->packet->magic_1, __this->packet->magic_2);
        return;
    }
    // 过滤
    if (__this->packet->direct != SALVE_2_MASTER) {
        log_info("direct incorrect :%x!\n", __this->packet->direct);
        return;
    }
    // 应答过滤
    if (__this->packet->length == 0x00) {
        log_info("ack correct0!\n");
        return;
    }
    switch (__this->packet->type) {
    case UART_CMD_HALL:     //舱门状态
    case UART_CMD_CHARGING:
        struct msg_hall *sub = (struct msg_hall *)__this->packet->payload;
        if (__this->get_vbat_value_from_f98 != (sub->ear_sta_charge & BIT(2))) {
            if (!__this->get_vbat_value_from_f98) {
                __this->get_vbat_value_from_f98 = 1;
                vbat_voltage_array_f98_init();
            } else {
                __this->get_vbat_value_from_f98 = 0;
                vbat_voltage_array_f98_uninit();
            }
        }
        if (sub->eal_power_l <= 100 && sub->eal_power_l >= 0) {
            sbox_battery_left_set(sub->eal_power_l);
        }
        if (sub->eal_power_r <= 100 && sub->eal_power_r >= 0) {
            sbox_battery_right_set(sub->eal_power_r);
        }
        if (sub->ear_sta_charge <= 7 && sub->ear_sta_charge >= 0) {
            if (sbox_box_charging_get() != (sub->ear_sta_charge & BIT(2))) {
                /* ui_screen_recover(1); */
            }
            sbox_box_charging_set(sub->ear_sta_charge & BIT(2));
            sbox_left_charging_set(sub->ear_sta_charge & BIT(1));
            sbox_right_charging_set(sub->ear_sta_charge & BIT(0));
        }
        if (__this->packet->type == UART_CMD_HALL) {
            sbox_box_clid_status_set(sub->sta_hall);
            msg[0] = (int)user_check_into_scan;
            msg[1] = 1;
            os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
        }
        break;
    case UART_CMD_MAC:      //MAC地址
        log_info("UART_CMD_MAC:");
        put_buf((u8 *)__this->packet->payload, 6);
#if TCFG_EARPHONE_PROTOCOL
        sbox_update_earphone_mac();
#endif
        break;
    case UART_CMD_EAR_HW_ST:
        log_info("UART_CMD_EAR_HW_ST : %x\n", __this->packet->payload[0]);
        extern void ear_inbox_state_deal(u8 status);
        ear_inbox_state_deal(__this->packet->payload[0]);
        break;
    case CMD_BATTERY_F98:
        struct msg_charge *sub_charge = (struct msg_charge *)__this->packet->payload;
        log_info("CMD_BATTERY_F98");
        u16 temp_adc_value = 0;
        temp_adc_value = (u16)((sub_charge->power_vol_h << 8) | sub_charge->power_vol_l);
        __this->get_vbat_value_from_f98 = sub_charge->local_charge;
        g_printf("DDD - temp_adc_value: 0x%x", temp_adc_value);
        __this->get_vbat_a_from_f98 = (u16)((sub_charge->power_current_h << 8) | sub_charge->power_current_l);
        g_printf("DDD - a_f98: 0x%x", __this->get_vbat_a_from_f98);
        if (__this->get_vbat_value_from_f98) {
            f98_vbat_to_array(temp_adc_value);
        }
    case UART_CMD_BATTERY:  //电量交互
        log_info("UART_CMD_BATTERY : %x\n", __this->packet->payload[0]);
        break;
    case UART_CMD_BUTTON:   //按键事件
        log_info("UART_CMD_BUTTON : %x\n", __this->packet->payload[0]);
        break;
    case UART_USER_ADD:
        log_info("UART_USER_ADD : %x\n", __this->packet->payload[0]);
        break;
    default:
        break;
    }
}
void test(void *p)
{
    printf("test uart cmd charging\n");
    int msg[2];
    int err = 0;
    msg[0] =  SBOX_UART_TX_DAT;
    msg[1] = CMD_PRODUCT_MODE;
    err = os_taskq_post_type("sbox_uart_task", Q_MSG, 2, msg);
    if (err) {
        printf("sbox tx post fail :%d \n", err);
        return;
    }
}
static void sbox_uart_task(void *priv)
{
    int msg[32];
    int ret;
    test(NULL);
    while (1) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        log_info("%s msg:%d", __func__, msg[1]);
        if (msg[1] == SBOX_UART_RX_DAT) {
            sbox_uart_rx_data_process();
        } else if (msg[1] == SBOX_UART_TX_DAT) {
            sbox_uart_tx_data_process(msg);
        }
    }
}


void sbox_uart_init()
{
    struct uart_config config = {
        .baud_rate = TCFG_SCREEN_CHARGE_IC_BAUD_RATE,
        .tx_pin = TCFG_SCREEN_CHARGE_IC_TX_PIN,
        .rx_pin = TCFG_SCREEN_CHARGE_IC_RX_PIN,
        .parity = UART_PARITY_DISABLE,
        .tx_wait_mutex = 0,
    };
    struct uart_dma_config dma = {0};

    dma.rx_timeout_thresh = 25 * 1000000 / 115200;
    dma.frame_size = 1;
    dma.event_mask = UART_EVENT_RX_DATA | UART_EVENT_RX_TIMEOUT | UART_EVENT_TX_DONE;
    dma.irq_priority = 2;
    dma.irq_callback = uart_irq_func;
    dma.rx_cbuffer = __this->uart_rx_ptr;
    dma.rx_cbuffer_size = 64;

    __this->uart_dev = uart_init(-1, &config);
    if (__this->uart_dev < 0) {
        printf("uart_init error :%d", __this->uart_dev);
        return;
    }
    int ret = uart_dma_init(__this->uart_dev, &dma);
    if (ret < 0) {
        printf("uart_dma_init error %d", ret);
        return;
    }
    uart_dump();
    printf("%s ok\n", __func__);
}

static u8 uart_idle_query(void)
{
    /* return 0; */
    return __this->uart_into_new_sleep;
}

REGISTER_LP_TARGET(charge_outside_target) = {
    .name = "uart",
    .is_idle = uart_idle_query,
};

void sbox_uart_module_init(void)
{
    sbox_uart_init();
    int err = task_create(sbox_uart_task, NULL, "sbox_uart_task");
    if (err != OS_NO_ERR) {
        printf("sbox_uart_rx_task creat fail %x\n", err);
    }

}


static int charge_ic_init()
{
    sbox_uart_module_init();
    return 0;
}
static int charge_ic_uninit()
{
    return 0;
}
static int charge_ic_charge_start()
{
    return 0;
}
static int charge_ic_charge_stop()
{
    return 0;
}
static int charge_ic_boost_ctrl(u32 en)
{
    return 0;
}
static int charge_ic_pwr_crtl(u32 en)
{
    return 0;
}


static int charge_ic_ioctrl(u32 cmd, u32 arg)
{
    int ret = 0;
    switch (cmd) {
    case CHARGE_IC_CMD_INIT:
        if (arg) {
            ret = charge_ic_init();
        } else {
            ret = charge_ic_uninit();
        }
        break;
    case CHARGE_IC_CMD_CHARGE:
        if (arg) {
            ret = charge_ic_charge_start();
        } else {
            ret = charge_ic_charge_stop();
        }
        break;
    case CHARGE_IC_CMD_BOOST:
        ret = charge_ic_boost_ctrl(arg);
        break;
    case CHARGE_IC_CMD_PWR:
        ret = charge_ic_pwr_crtl(arg);
        break;
    default:
        break;
    }
    return ret;
}

REGISTER_CHAREG_IC_MODULE(ac982)
{
    .io_ctrl = charge_ic_ioctrl,
};

#endif//
