#include "app_config.h"
#include "pwm_interface.h"
#include "gpio.h"
#include "clock.h"
#include "init.h"

#define LOG_TAG_CONST   PWM
#define LOG_TAG         "[pwm]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define ADVTMR_CLOCK			(48000000) // advtmr模块时钟
#define ADVPWM_HZ				(10*1000) // pwm周期

#define GPPWM_HZ				(1000) // gp_pwm周期
#define GPPWM_DUTY				(5000) // gp占空比

static const u8 advpwm_channel[] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};
static const u8 advpwm_ocp[] = {
    PORT_FUNC_TMR0_OC1P, PORT_FUNC_TMR0_OC2P, PORT_FUNC_TMR0_OC3P, PORT_FUNC_TMR0_OC4P
};
static const u8 advpwm_ocn[] = {
    PORT_FUNC_TMR0_OC1N, PORT_FUNC_TMR0_OC2N, PORT_FUNC_TMR0_OC3N, PORT_FUNC_TMR0_OC4N
};
static u8 advpwm_ch_used = 0; // 标记通道已经使用。不可使用的通道需要在此处标记

static u8 gptimer_ch_used = 0;  // 标记gptimer通道已使用

static u8 gptimer_ids[] = {
    TIMER0, TIMER1, TIMER2, TIMER3, TIMER4, TIMER5, TIMERx
};

static TIM_HandleTypeDef advpwm_hdl;

static int advpwm_init()
{
    adv_timer_rst();

    advpwm_hdl.Instance               = JL_ADV_TIMER0;
    advpwm_hdl.Init.Prescaler         = (clk_get("lsb") / ADVTMR_CLOCK) - 1;
    advpwm_hdl.Init.Period            = ADVTMR_CLOCK / (ADVPWM_HZ) - 1;
    advpwm_hdl.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    advpwm_hdl.Init.CounterMode       = TIM_COUNTERMODE_UP;

    HAL_TIM_PWM_Init(&advpwm_hdl);

    return 0;
}
early_initcall(advpwm_init);


int pwm_if_open(struct pwm_if_handle *hdl, int gpio, int duty, struct pwm_if_param *param)
{
    if (!hdl) {
        return PWM_IF_ERR_HDL;
    }
    u8 src_type = -1;
    if (param) {
        src_type = param->src_type;
    }
    if (src_type & BIT(PWM_IF_SRC_TYPE_ADV)) {
        for (int i = 0; i < ARRAY_SIZE(advpwm_channel); i++) {
            if (advpwm_ch_used & BIT(i)) {
                continue;
            }
            if (param && (param->channel >= 0)) {
                if (i != param->channel) {
                    continue;
                }
            }
            advpwm_ch_used |= BIT(i);
            hdl->src_type = PWM_IF_SRC_TYPE_ADV;
            hdl->channel = i;

            TIM_OC_InitTypeDef normal_cfg;
            TIM_OC_InitTypeDef *cfg;

            if (param && param->adv.cfg) {
                cfg = param->adv.cfg;
            } else {
                cfg = &normal_cfg;
                normal_cfg.OCMode         = TIM_OCMODE_PWM1;      // 输出比较模式
                normal_cfg.OCPolarity     = TIM_OCPOLARITY_HIGH;     // 输出有效电平极性
                normal_cfg.OCFastMode     = TIM_OCFAST_DISABLE;
                normal_cfg.OCNPolarity    = TIM_OCPOLARITY_HIGH;     // 互补输出有效电平极性
                normal_cfg.OCNIdleState   = TIM_OCNIDLESTATE_RESET;
                normal_cfg.OCIdleState    = TIM_OCIDLESTATE_RESET;
                normal_cfg.Pulse 			= 0;
            }
            cfg->Pulse = advpwm_hdl.Init.Period * duty / 100;

            HAL_TIM_PWM_ConfigChannel(&advpwm_hdl, cfg, advpwm_channel[hdl->channel]);

            HAL_TIM_PWM_Start(&advpwm_hdl, advpwm_channel[hdl->channel]);
            TIM_CCxNChannelCmd(advpwm_hdl.Instance, advpwm_channel[hdl->channel], TIM_CCxN_ENABLE);

            gpio_set_function(IO_PORT_SPILT(gpio), advpwm_ocp[hdl->channel]);

            if (param && param->adv.output_n_en) {
                gpio_set_function(IO_PORT_SPILT(param->adv.gpio_n), advpwm_ocn[hdl->channel]);
            }
            log_info("advpwm open ch:%d \n", hdl->channel);

            return 0;
        }
    }
    if (src_type & BIT(PWM_IF_SRC_TYPE_TMR)) {
        for (int i = 0; i < ARRAY_SIZE(gptimer_ids); i++) {
            if (gptimer_ch_used & BIT(i)) {
                continue;
            }
            if (param && (param->channel >= 0)) {
                if (i != param->channel) {
                    continue;
                }
            }

            struct gptimer_config *tmr_cfg;
            struct gptimer_config default_cfg;

            if (param && param->tmr.cfg) {
                tmr_cfg = param->tmr.cfg;
            } else {
                memset(&default_cfg, 0, sizeof(default_cfg));
                default_cfg.mode = GPTIMER_MODE_PWM;
                default_cfg.pwm.freq = GPPWM_HZ;
                default_cfg.pwm.duty = GPPWM_DUTY;  // 默认50%占空比
                default_cfg.pwm.port = (enum gpio_port)((u32)gpio / IO_GROUP_NUM);
                default_cfg.pwm.pin  = BIT((u32)gpio % IO_GROUP_NUM);
                default_cfg.irq_priority = 1;
                tmr_cfg = &default_cfg;
            }
            timer_dev tmr_dev = TIMERx;  // 默认
            if (param && param->tmr.tmr_dev) {
                tmr_dev = param->tmr.tmr_dev;
            }

            u32 tid = gptimer_init(tmr_dev, tmr_cfg);
            if (tid == (u32) - 1) {
                log_error("gptimer init failed!");
                continue;
            }
            gpio_set_mode(tmr_cfg->pwm.port, tmr_cfg->pwm.pin, PORT_OUTPUT_LOW);
            gptimer_start(tid);

            gptimer_ch_used |= BIT(i);
            gptimer_ids[i] = tid;
            hdl->src_type = PWM_IF_SRC_TYPE_TMR;
            hdl->channel = i;  // 使用索引作为channel

            log_info("gptimer pwm open ch:%d, tid:%d\n", i, tid);
            return 0;
        }
    }
    return PWM_IF_ERR_NO_VALID;
}

int pwm_if_set_duty(struct pwm_if_handle *hdl, u16 duty)
{
    if (!hdl) {
        return PWM_IF_ERR_HDL;
    }
    if (hdl->src_type == PWM_IF_SRC_TYPE_ADV) {
        if (!(advpwm_ch_used & BIT(hdl->channel))) {
            return PWM_IF_ERR_CHANNEL;
        }
        u32 pulse = __HAL_TIM_GET_AUTORELOAD(&advpwm_hdl) * duty / 100;
        log_info("advpwm[%d] duty set %d, %d \n", hdl->channel, duty, pulse);
        __HAL_TIM_SET_COMPARE(&advpwm_hdl, advpwm_channel[hdl->channel], pulse);
        return 0;
    }
    if (hdl->src_type == PWM_IF_SRC_TYPE_TMR) {
        if (!(gptimer_ch_used & BIT(hdl->channel))) {
            return PWM_IF_ERR_CHANNEL;
        }

        u32 tid = gptimer_ids[hdl->channel];

        log_info("gptimer[%d] duty set %d\n", hdl->channel, duty);
        gptimer_set_pwm_duty(tid, duty);
        return 0;
    }
    return PWM_IF_ERR_HDL;
}

int pwm_if_set_freq(struct pwm_if_handle *hdl, u32 freq)
{
    if (!hdl) {
        return PWM_IF_ERR_HDL;
    }

    if (hdl->src_type == PWM_IF_SRC_TYPE_TMR) {
        if (!(gptimer_ch_used & BIT(hdl->channel))) {
            return PWM_IF_ERR_CHANNEL;
        }

        u32 tid = gptimer_ids[hdl->channel];
        log_info("gptimer[%d] freq set %d\n", hdl->channel, freq);
        gptimer_set_pwm_freq(tid, freq);
        return 0;
    }

    return PWM_IF_ERR_HDL;
}

int pwm_if_close(struct pwm_if_handle *hdl)
{
    if (!hdl) {
        return PWM_IF_ERR_HDL;
    }
    if (hdl->src_type == PWM_IF_SRC_TYPE_ADV) {
        if (!(advpwm_ch_used & BIT(hdl->channel))) {
            return PWM_IF_ERR_CHANNEL;
        }
        HAL_TIM_PWM_Stop(&advpwm_hdl, advpwm_channel[hdl->channel]);
        advpwm_ch_used &= ~BIT(hdl->channel);
        hdl->src_type = PWM_IF_SRC_TYPE_NULL;
        log_info("advpwm close ch:%d \n", hdl->channel);
        return 0;
    }
    if (hdl->src_type == PWM_IF_SRC_TYPE_TMR) {
        if (!(gptimer_ch_used & BIT(hdl->channel))) {
            return PWM_IF_ERR_CHANNEL;
        }
        u32 tid = gptimer_ids[hdl->channel];
        gptimer_pause(tid);
        gptimer_deinit(tid);

        gptimer_ch_used &= ~BIT(hdl->channel);
        gptimer_ids[hdl->channel] = 0;
        hdl->src_type = PWM_IF_SRC_TYPE_NULL;

        log_info("gptimer close ch:%d\n", hdl->channel);
        return 0;
    }
    return 0;
}

