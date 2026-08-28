#include "app_config.h"
#include "timer.h"
#include "gpio.h"
#include "asm/gptimer_hw.h"
#include <math.h> // 需要包含math.h以使用sqrt函数
#include "chgbox_ctrl.h"
#include "chargeIc_manage.h"

#if TCFG_CHARGE_BOX_ENABLE

#define USER_TIMER_ID   1
#define USER_TIMER      (TIMER_BASE_ADDR + USER_TIMER_ID * TIMER_OFFSET)
#define DATA_COUNT      20
#define DIFFERENCE_THRESHOLD 30 // 自定义差值阈值
#define CONFIRMATION_COUNT 3 // 确认状态所需的连续检测次数

static u8 is_full_busy = false;
static u32 pulse_data[DATA_COUNT];
static int data_index = 0;
static bool is_charging = false; // 初始状态为未满电
static int charge_counter = 0;
static int full_charge_counter = 0;
static bool is_func_gpio_init = 0;

void user_pulse_detec_init(u16 gpio)
{
    if (is_func_gpio_init) {
        return;
    }
    USER_TIMER->CON = ((0b1 << 14) | (0b1111 << 10) | (0b0000 << 4));
    USER_TIMER->CNT = 0;
    USER_TIMER->PRD = (u16) - 1;
    gpio_set_function(IO_PORT_SPILT(gpio), PORT_FUNC_TIMER0_CIN + USER_TIMER_ID);
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_INPUT_FLOATING);
    is_func_gpio_init = 1;
}

void user_pulse_detec_uninit(u16 gpio)
{
    USER_TIMER->CON = ((0b1 << 14) | (0b1111 << 10) | (0b0000 << 4));
    USER_TIMER->CNT = 0;
    USER_TIMER->PRD = (u16) - 1;
    gpio_disable_function(IO_PORT_SPILT(gpio), PORT_FUNC_TIMER0_CIN + USER_TIMER_ID);
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_INPUT_FLOATING);
    is_func_gpio_init = 0;
}

void user_pulse_detec_start()
{
    USER_TIMER->CON |= BIT(0);
}

void user_pulse_detec_pause()
{
    USER_TIMER->CON &= ~BIT(0);
}

void user_clear_pluse_cnt()
{
    USER_TIMER->CNT = 0;
}

u32 user_get_pluse_cnt()
{
    USER_TIMER->CON &= ~BIT(0);
    u32 cnt = USER_TIMER->CNT;
    USER_TIMER->CNT = 0;
    USER_TIMER->CON |= BIT(0);
    return cnt;
}

static void user_pluse_detect_timer_func(void *priv)
{
    u32 cnt = user_get_pluse_cnt();
    pulse_data[data_index] = cnt;
    data_index = (data_index + 1) % DATA_COUNT;
    if ((sys_info.status[USB_DET] == STATUS_ONLINE) && chargebox_get_charge_en()) {
        if (data_index == 0) {
            u32 max_value = 0;
            u32 total_difference = 0;

            // 找到最大值
            for (int i = 0; i < DATA_COUNT; i++) {
                if (pulse_data[i] > max_value) {
                    max_value = pulse_data[i];
                }
            }

            // 计算所有值与最大值的差值之和
            for (int i = 0; i < DATA_COUNT; i++) {
                total_difference += (pulse_data[i] - max_value);
            }

            printf("Pulse data for the last second:\n");
            for (int i = 0; i < DATA_COUNT; i++) {
                printf("%d ", pulse_data[i]);
            }
            printf("\n");
            printf("Total difference: %d\n", total_difference);

            // 判断状态
            if (total_difference > DIFFERENCE_THRESHOLD) {
                charge_counter++;
                full_charge_counter = 0; // 重置满电计数器
                if (charge_counter >= CONFIRMATION_COUNT) {
                    is_charging = false;
                    printf("Confirmed: Charging...\n");
                } else {
                    printf("Checking Charging...\n");
                }
            } else {
                full_charge_counter++;
                charge_counter = 0; // 重置充电计数器
                if (full_charge_counter >= CONFIRMATION_COUNT) {
                    is_charging = true;
                    printf("Confirmed: Fully Charged\n");
                } else {
                    printf("Checking Fully Charged...\n");
                }
            }
        } else {
            is_charging = false;
        }
    } else {
        is_charging = false;
    }
}

bool is_device_charging()
{
    return is_charging;
}

uint16_t charge_full_detect_timer = 0;
void user_charge_insert_detect()
{
    printf("func:%s(), line:%d\n", __func__, __LINE__);
    if (charge_full_detect_timer) {
        return;
    }
    // 检测充电插入
    if (sys_info.status[USB_DET] == STATUS_ONLINE) {
        user_pulse_detec_init(TCFG_CHARGE_FULL_DET_IO);
        if (charge_full_detect_timer) {
            sys_timer_del(charge_full_detect_timer);
            charge_full_detect_timer = 0;
        }
        charge_full_detect_timer = sys_timer_add(NULL, user_pluse_detect_timer_func, 100);
        user_pulse_detec_start();
        is_full_busy = true;
        printf("Charging detected, timer started.\n");
    }
}

void user_charge_remove_detect()
{
    // 检测充电拔出
    if (sys_info.status[USB_DET] == STATUS_OFFLINE) {
        if (charge_full_detect_timer) {
            sys_timer_del(charge_full_detect_timer);
            charge_full_detect_timer = 0;
        }
        user_pulse_detec_pause();
        // 清除 pulse_data 数据
        for (int i = 0; i < DATA_COUNT; i++) {
            pulse_data[i] = 0;
        }
        data_index = 0;
        printf("Charging removed, timer stopped.\n");
        is_charging = false;
        is_full_busy = false;
    }
}

static u8 full_idle_query(void)
{
    return !is_full_busy;
}

REGISTER_LP_TARGET(full_lp_target) = {
    .name = "full_timer",
    .is_idle = full_idle_query,
};

#endif //#if TCFG_CHARGE_BOX_ENABLE

