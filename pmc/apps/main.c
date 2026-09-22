#include "includes.h"
#include "uart.h"
#include "debug.h"
#include "gpio.h"
#include "bank_switch.h"
#include "usr_timer.h"
#include "iic_api.h"
#include "main.h"
#include "bsp.h"
#include "sensor/driver/sensor_driver.h"
#include "vad_hw.h"
#if CONFIG_AEC_UART_EXPORT_ENABLE
#include "aec_uart_debug.h"
#endif
#include "generic/jiffies.h"
#include "init.h"
#include "wdt.h"

#define LOG_TAG_CONST       MAIN
#define LOG_TAG             "[main]"
#include "generic/log.h"

#pragma const_seg(".main.const.cache.L1")
#pragma code_seg(".main.text.cache.L1")
#pragma str_literal_override(".main.const.cache.L1")

/*
	function: m2p消息处理，主循环不会收到ack消息
	param index:等待返回ack的消息序号
	return val：1, 收到对应ack消息 0,未收到对应ack消息
 */
int q32f_m2p_msg_hdl(u32 index)
{
    struct lp_msg_handler *p;
    u8 msg[32];
    struct lp_msg_head head;

    int ret = q32f_m2p_get_msg(&head, (u8 *)msg, ARRAY_SIZE(msg));

    //成功读取到消息
    if (ret == MSG_NO_ERROR) {
        list_for_each_m2p_msg_handler(p) {
            if (p->type == head.type) {
                if (head.ack) {
                    config_post_ack_flag(0);
                }
                p->handler(p->priv, (u8 *)msg, head.len);
                if (head.ack) {
                    //若是应答消息则返回应答
                    q32f_pmc_ack_msg(head.ack_index);
                    config_post_ack_flag(1);
                }
            }
        }
    } else if (ret == MSG_NO_MSG) {
        //未读取到消息，请求进入低功耗了
#if 0
        low_power_enable();
        asm volatile("idle");
        low_power_disable();
#endif
    } else {
        ASSERT(0, "ret: %d, type: %d, ack: %d, index: %d, len: %d\n", ret, head.type, head.ack, head.index, head.len);

    }

    return 0;
}

static void callback(u32 *priv)
{
    log_info("function: callback, %d\n", (u32)priv);
}

static void func(void *priv)
{
    log_char('+');
}

// 临时接口
#define MSYS_RST            (PMC_SYSTEM->PMC_SYS_CON1 & BIT(12))
#define MSYS_KEY            (PMC_SYSTEM->PMC_SYS_CON1 & BIT(14))  // 注:旧sof是BIT(14), 新sof是BIT(15). 后续更新sof需要修改(TODO)
_NOINLINE_
static void p2m_key_ok()
{
    while (!MSYS_RST);
    while (!MSYS_KEY) {
        JL_PMU->P2M_KEY = 0xFF;
        JL_PMU->P2M_KEY = 0xEE;
        JL_PMU->P2M_KEY = 0xAB;
        JL_PMU->P2M_KEY = 0x0E;
        JL_PMU->P2M_KEY = 0x55;
        asm volatile("csync");
        asm volatile("nop");
        asm volatile("nop");
    };
}

AT(.flash_code)
static void flash_code_test_func(void)
{
    log_info("is flash code:0x%x \n", (int)flash_code_test_func);
}

static void app_task_loop(void *p)
{
    /* printf("func addr:0x%x \n", (int)flash_code_test_func);	 */
    /* flash_code_test_func(); */

    // 如果需要访问主系统的sram,则需要打key
    /* p2m_key_ok(); */
}

/* static int test_init(void) */
/* { */
/* return 0; */
/* } */
/* late_initcall(test_init); */

void app_main()
{
    do_early_initcall();

    bsp_init();
#if 0

    lptmr1_init();


#endif

    do_platform_initcall();
    do_initcall();

#ifndef LOWPOWER_TEST_TMP
    //usr_timer_add(NULL, func, 1000, 0);
#endif
#if 0 //关闭cache，禁止访问flash。用于检测主流程是否都在SRAM中
    void IcuFlushinvAll(void);
    void IcuDisable(void);
    IcuFlushinvAll();
    IcuDisable();
#endif

    do_module_initcall();
    do_late_initcall();

    while (1) {
        q32f_m2p_msg_hdl(0);
        wdt_clear();
#if CONFIG_AEC_UART_EXPORT_ENABLE
        aec_uart_send_loop();
#endif
    }
}

/*
	function: 主循环回调处理
 */
void app_handler_add(void (*callback)(u32 *priv), u32 *priv)
{
    u8 msg[9];
    u32 msg_cb = (u32)callback;
    u32 msg_priv = (u32)priv;

    msg[0] = MSG_APP_CALLBACK;
    memcpy(msg + 1, &msg_cb, 4);

    if (priv != NULL) {
        memcpy(msg + 5, &msg_priv, 4);
        m2p_post_msg_q32f(MSG_APP, 0, msg, 9);
    } else {
        m2p_post_msg_q32f(MSG_APP, 0, msg, 5);
    }
}

int task_post_msg(char *name, int argc, ...)
{
    int msg[16];
    va_list argptr;
    va_start(argptr, argc);
    int  param;
    ASSERT(argc < sizeof(msg) / sizeof(msg[0]))
    for (int i = 0; i < argc; ++i) {
        param = va_arg(argptr, int);
        msg[i] = param;
    }
    m2p_post_msg_q32f(MSG_APP, 0, (u8 *)msg, argc * sizeof(int));
    va_end(argptr);
    return 0;
}

static void app_msg_handler(void *priv, u8 *msg, u32 len)
{

    log_info(" app_msg_handler = %d\n", msg[0]);
    if (msg[0] == MSG_APP_CALLBACK) {
        u32 *msg_priv;
        void (*msg_cb)(u32 * priv);
        memcpy(&msg_cb, msg + 1, 4);
        memcpy(&msg_priv, msg + 5, 4);
        msg_cb(msg_priv);
    }


    switch (msg[0]) {
#if CONFIG_SENSOR_DRIVER_ENABLE
    case MSG_P11_SYS_KICK:
        log_info("MSG_P11_SYS_KICK\n");
        sensor_driver_run();
        break;
    case MSG_P11_SENSOR_SLEEP:
        log_info("MSG_P11_SENSOR_SLEEP\n");
        sensor_driver_sleep(msg[1], msg[2]);
        break;

    case MSG_P11_SENSOR_IRQ:
        log_info("MSG_P11_SENSOR_IRQ\n");
        sensor_driver_irq_handle();
        break;
    case MSG_P11_HEALTH_MANAGER_LOOP:
        printf("MSG_P11_HEALTH_MANAGER_LOOP\n");
        void sport_health_manager_task();
        sport_health_manager_task();
        break;
    case MSG_P11_HEALTH_MANAGER_DATA_P2P:
        printf(" MSG_P11_HEALTH_MANAGER_DATA_P2P\n");
        int sport_health_manager_msg_post_self(int module, int cmd, void *priv);
        int msg_priv;
        memcpy((u8 *)&msg_priv, &msg[3], 4);
        sport_health_manager_msg_post_self(msg[1], msg[2], (void *)&msg_priv);
        break;
    case MSG_P11_HEALTH_MANAGER_DATA_M2P:
        printf(" MSG_P11_HEALTH_MANAGER_DATA_M2P\n");
        {
            int module = msg[1];
            int type = msg[2];
            int sport_health_manager_get_value_m2p(int module, int type);
            sport_health_manager_get_value_m2p(module, type);
        }
#endif
        //test
        break;
    case MSG_P11_SENSOR_INIT: {
        log_info("m2p_post_msg  len=%d\n", len);
#if CONFIG_SENSOR_DRIVER_ENABLE
        sensor_driver_init(msg[1], msg[2], msg[3] << 8 | msg[4], msg[5]);
#endif
        break;
    }

    case MSG_P11_SOFF_EVENT: {
        //lptmr1_set_wkup_time(0, P11_LPTMR_WKUP_EVENT);
    }
    }

}

REGISTER_M2P_MSG_HANDLER(0, MSG_APP, app_msg_handler);


void __power_recover()
{
    u8 msg = MSG_P11_POWER_RECOVER;
    m2p_post_msg(MSG_APP, 0, &msg, 1);
}


