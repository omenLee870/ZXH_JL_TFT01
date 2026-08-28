#ifndef __POWER_MANAGE_H__
#define __POWER_MANAGE_H__

#include "list.h"

//******************************************************************************************
/*
 *   模块注册进入sleep等级
 */
typedef u8(*idle_handler_t)(void);
typedef enum LOW_POWER_LEVEL(*level_handler_t)(void);
struct lp_target {
    char *name;
    level_handler_t level;
    idle_handler_t is_idle;
};

#define REGISTER_LP_TARGET(target) \
        const struct lp_target target SEC_USED(.lp_target)

extern const struct lp_target lp_target_begin[];
extern const struct lp_target lp_target_end[];

#define list_for_each_lp_target(p) \
    for (p = lp_target_begin; p < lp_target_end; p++)

u32 low_power_sys_not_idle_cnt(void);
u8 low_power_sys_is_idle(void);

enum LOW_POWER_LEVEL {
    LOW_POWER_MODE_LIGHT_SLEEP = 1,
    LOW_POWER_MODE_SLEEP,
    LOW_POWER_MODE_DEEP_SLEEP,
    LOW_POWER_MODE_SOFF,
};
u8 is_low_power_mode(enum LOW_POWER_LEVEL level);
enum LOW_POWER_LEVEL low_power_level(enum LOW_POWER_LEVEL level);

u32 get_low_power_mode();

//******************************************************************************************
/*
 *	get_timeout->suspend->resume
 */
struct low_power_operation {
    const char *name;
    u32(*get_timeout)(void *priv);
    void (*suspend_probe)(void *priv);
    void (*suspend_post)(void *priv, u32 usec);
    void (*resume)(void *priv, u32 usec);
    void (*resume_post)(void *priv, u32 usec);
};

struct low_power_ops {
    struct list_head entry;
    void *priv;
    const struct low_power_operation *ops;
};

void *low_power_get(void *priv, const struct low_power_operation *ops);
void low_power_put(void *priv);
void *low_power_sys_get(void *priv, const struct low_power_operation *ops);
void low_power_sys_put(void *priv);
void low_power_on();
void low_power_request();
u32 low_power_group_query(struct list_head *request_head);
u32 lower_power_bt_group_query();
u32 lower_power_sys_group_query();
u32 is_low_power_bt_mode();
void low_power_suspend(u32 usec);
void low_power_suspend_post(u32 usec);
void low_power_resume(u32 usec);
void low_power_resume_post(u32 usec);
void low_power_sys_resume(u32 usec);
void low_power_sys_resume_post(u32 usec);

//******************************************************************************************
/*低功耗线程请求进入低功耗, 主动发出
 */
struct lp_request {
    char *name;
    u8(*request_enter)(u32 timeout);
    u8(*request_exit)(u32 timeout);
};

#define REGISTER_LP_REQUEST(target) \
        const struct lp_request target SEC_USED(.lp_request)

extern const struct lp_request lp_request_begin[];
extern const struct lp_request lp_request_end[];

#define list_for_each_lp_request(p) \
    for (p = lp_request_begin; p < lp_request_end; p++)

u8 low_power_sys_request_enter(u32 timeout);
u8 low_power_sys_request_exit(u32 timeout);

//******************************************************************************************
/*
 * deepsleep register
 */
struct deepsleep_target {
    char *name;
    u8(*enter)(void);
    u8(*exit)(void);
    u8(*post)(void);
};

#define DEEPSLEEP_TARGET_REGISTER(target) \
        const struct deepsleep_target target SEC_USED(.deepsleep_target)

extern const struct deepsleep_target deepsleep_target_begin[];
extern const struct deepsleep_target deepsleep_target_end[];

#define list_for_each_deepsleep_target(p) \
    for (p = deepsleep_target_begin; p < deepsleep_target_end; p++)

void deepsleep_target_enter();

void deepsleep_target_exit();

void deepsleep_target_post();

void deepsleep_target_dump();

//******************************************************************************************
void power_manage_init(void *priv);

extern const u32 pdrv_text_cache_l2_begin;
extern const u32 pdrv_text_cache_l2_end;

#endif
