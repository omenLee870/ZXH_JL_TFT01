#ifndef  __PWM_INTERFACE_H__
#define  __PWM_INTERFACE_H__

#include "asm/adv_timer.h"
#include "gptimer.h"
#include "asm/gptimer_hw.h"

#define PWM_IF_NO_ERR					0
#define PWM_IF_ERR_HDL					-1 // 句柄错误
#define PWM_IF_ERR_NO_VALID				-2 // 没有适配有效的通道
#define PWM_IF_ERR_CHANNEL				-3 // channel错误

enum {
    PWM_IF_SRC_TYPE_NULL = 0,
    PWM_IF_SRC_TYPE_ADV,
    PWM_IF_SRC_TYPE_TMR,
    PWM_IF_SRC_TYPE_MAX,
};

/**
  * @brief  pwm interface handle
  */
struct pwm_if_handle {
    u8 src_type;
    u8 channel;
};

/**
  * @brief  pwm interface init parameters
  */
struct pwm_if_param {
    u8 src_type;	// pwm类型。按BIT(X)配置。类推(-1)就是自动分配
    s8 channel;		// <0 自动分配；>=0 指定通道
    union {
        struct {
            TIM_OC_InitTypeDef *cfg;	// adv相关参数。NULL用默认参数
            u8 output_n_en;	// 是否输出N端
            int gpio_n; 	// N端GPIO
        } adv;
        struct {
            struct gptimer_config *cfg;
            timer_dev tmr_dev;             // 指定定时器硬件，如TIMER0, TIMER1等
        } tmr;
    };
};

int pwm_if_open(struct pwm_if_handle *hdl, int gpio, int duty, struct pwm_if_param *param);

int pwm_if_set_duty(struct pwm_if_handle *hdl, u16 duty);

int pwm_if_close(struct pwm_if_handle *hdl);

#endif

