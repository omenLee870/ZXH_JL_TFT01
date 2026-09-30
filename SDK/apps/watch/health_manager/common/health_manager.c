
#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"
#include "clock_manager/clock_manager.h"
#include "health_manager/health_manager.h"
#include "health_manager/health_manager_test.h"
#include "sensor_hub.h"
#include "rtc/rtc_dev.h"
#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-ALGO_GSENSOR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".health_manager.data.bss")
#pragma data_seg(".health_manager.data")
#pragma const_seg(".health_manager.text.const")
#pragma code_seg(".health_manager.text")
#endif


#define	SPORT_HEALTH_MANAGER_TASK   "health_manager"

/* 关机前落盘的等待预算。
 * 预算用尽即放弃剩余模块继续关机，保证关机不被落盘拖死；
 * 剩余预算按剩余模块数均分（单模块上限 SHM_POWEROFF_SAVE_ONE_MS），
 * 使清单内每个模块都能分到等待窗口，不会被前面的慢模块饿死。 */
#define SHM_POWEROFF_SAVE_BUDGET_MS     (200)
#define SHM_POWEROFF_SAVE_ONE_MS        (100)



/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_value_get 获取模块数据
 *
 * @param module		模块
 * @param type			获取数据类型
 * @param priv
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_health_manager_value_get(int module, int type, void *priv)
{
    int rets;
    __asm__ volatile("%0 = rets":"=r"(rets));

    int ret = -SHM_ERR_MOD_NOT_FIND;
    if (module >= SHM_MOD_END) {
        goto __ret;
    }
    if (type >= SHM_GET_TYPE_MAX) {
        ret =  -SHM_ERR_TYPE_NOT_DEFINE;
        goto __ret;
    }

    struct sport_health_module *mod;
    list_for_each_sport_health_module(mod) {
        if (mod->module == module) {
            if (mod->get_value) {
                ret = mod->get_value(type, priv);
            } else {
                ret = -	SHM_ERR_MOD_CB_NOT_DEFINE;
                goto __ret;
            }
        }
    }

__ret:
    if (ret) {
        /* SHM_ERR_DATA_ERR(12)：数据尚未收敛/暂不可用（如血氧模块 status 未到
         * SHM_MOD_STA_DATA_UPDATE_SUCC），属轮询期间的正常中间态，每秒必现，
         * 降为 debug 抑制刷屏；其余错误码保持 error 级别。 */
        if (ret == -SHM_ERR_DATA_ERR || ret == SHM_ERR_DATA_ERR) {
            log_debug("%s err:%d rets:0x%x", __func__, ret, rets);
        } else {
            log_error("%s err:%d rets:0x%x", __func__, ret, rets);
        }
    }
    return ret;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_alloc 申请内存接口
 *
 * @param size
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void *sport_health_alloc(u32 size)
{
//debug info
    return zalloc(size);
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_free 释放内存
 *
 * @param p
 */
/* ------------------------------------------------------------------------------------*/
void sport_health_free(void *p)
{
    free(p);
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_msg_post_self 发送消息给其他模块执行
 *
 * @param module
 * @param cmd
 * @param priv
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_health_manager_msg_post_self(int module, int cmd, void *priv)
{
    int rets;
    __asm__ volatile("%0 = rets":"=r"(rets));

    int ret = SHM_ERR_OK;
    struct sport_health_module *mod;
    list_for_each_sport_health_module(mod) {
        if (mod->module == module) {
            if (mod->io_ctrl) {
                mod->io_ctrl(cmd, priv);
            }
            continue;
        }
    }

    return ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_msg_post post消息给健康模块
 *
 * @param module
 * @param cmd
 * @param priv
 * @param need_pend			是否阻塞
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_health_manager_msg_post(int module, int cmd, void *priv, int need_pend)
{
    int rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    int pnd = need_pend;
    if (cpu_in_irq()) {
        pnd = 0;
    } else {
        if (!strcmp(os_current_task(), SPORT_HEALTH_MANAGER_TASK)) {
            pnd = 0;
        }
    }
    int ret = SHM_ERR_OK;
    static OS_SEM sem;
    int msg[4] = {0};
    msg[0] = cmd;
    msg[1] = (int)priv;
    if (pnd) {
        os_sem_create(&sem, 0);
        msg[2] = (int)&sem;
    } else {
        msg[2] = 0;
    }
    log_debug("%s mod:%d cmd:%d pnd:%d", __func__, module, cmd, need_pend);
    int err = os_taskq_post_type(SPORT_HEALTH_MANAGER_TASK, module, 3, msg);
    if (err) {
        log_warn("%s cmd:0x%x os_err:%d rets:0x%x\n", __func__, cmd, err, rets);
        ret = -SHM_ERR_OS_ERR;
    } else {
        if (pnd) {
            os_sem_pend(&sem, 0);
        }
    }
    return ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_module_init 初始化所有模块
 */
/* ------------------------------------------------------------------------------------*/
static void sport_health_module_init()
{

    log_debug("%s %d 0x%x 0x%x", __func__, __LINE__, sport_health_module_begin, sport_health_module_end);
    struct sport_health_module *mod;
    list_for_each_sport_health_module(mod) {
        log_debug("%s %d", __func__, mod->module);
        if (mod->io_ctrl) {
            mod->io_ctrl(SHM_CMD_INIT, NULL);
        }
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_module_release 注销所有模块
 */
/* ------------------------------------------------------------------------------------*/
static void sport_health_module_release()
{

    log_debug("%s %d 0x%x 0x%x", __func__, __LINE__, sport_health_module_begin, sport_health_module_end);
    struct sport_health_module *mod;
    list_for_each_sport_health_module(mod) {
        log_debug("%s %d", __func__, mod->module);
        if (mod->io_ctrl) {
            mod->io_ctrl(SHM_CMD_RELEASE, NULL);
        }
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_update_sec_all 每秒更新模块
 */
/* ------------------------------------------------------------------------------------*/
static void sport_health_manager_update_sec_all()
{
    for (int module = SHM_MOD_BEGIN + 1; module < SHM_MOD_END; module++) {
        struct sport_health_module *mod;
        list_for_each_sport_health_module(mod) {
            if (mod->module == module) {
                if (mod->io_ctrl) {
                    mod->io_ctrl(SHM_CMD_UPDATE_SEC, NULL);
                }
                break;
            }
        }
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_update_timer_cb timer回调
 */
/* ------------------------------------------------------------------------------------*/
static void sport_health_manager_update_timer_cb()
{
    sport_health_manager_msg_post(SHM_MOD_BEGIN, SHM_CMD_UPDATE_ALL, NULL, 0);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 整点强制唤醒
 *
 * @note 主 1s 节拍走 sys_s_hi_timer_add，展开就是 usr_timer_add(..., priority=0)：
 *       低功耗下该节拍「不会丢失，但定时周期会变」，节拍被拉长时整点可能落在
 *       某一个变长的周期里，整点落盘/整点拉取都会被推迟甚至跳过。
 *       sys_timer 由 systimer 线程提供时基，低功耗不丢节拍、到期唤醒系统，
 *       用来兜住这个问题。
 *       调用频率：周期恒为「距下一个整点的毫秒数」（最长 1 小时），回调内重新
 *       对齐到再下一个整点 —— 即每小时只唤醒一次，不做秒级轮询。
 *       时刻校验：排定唤醒时记下「应到的整点」完整时刻，唤醒后与 RTC 求绝对秒差，
 *       偏差超过 SHM_HOUR_DRIFT_TOLERATE_MIN 视为时钟被调整过（人为改时间 / 校时 /
 *       改日期），本段采样数据作废、不驱动整点动作 —— 否则改过的时钟会把这一小时
 *       的采样挂到错误的整点槽位上。
 *       用绝对秒差而不是逐字段比对：正常跨整点时日期本来就会变（23:00 排定、
 *       次日 00:00 唤醒），逐字段判「不等」会把每个跨零点的整点都误杀；秒差则
 *       天然覆盖分钟/小时/日期/年份任一维度的改动。
 */
/* ------------------------------------------------------------------------------------*/
/* 整点唤醒允许偏差(分钟)：1 分钟内的抖动视为定时器正常误差，超过即认定时钟被调整过。
 * 与 module/shm_mod_heart_rate.c、module/shm_mod_blood_oxygen.c 中的同名宏保持一致。 */
#define SHM_HOUR_DRIFT_TOLERATE_MIN     (1)

static u16 shm_hour_wake_timer_id = 0;
static struct sys_time shm_hour_wake_expect = {0};/* 本次定时唤醒应到的整点时刻(完整) */

/* 主 1s 节拍（usr_timer，回调在 ISR）。与整点 timer 同理不会随模块 release 自动
 * 消亡，必须留句柄以配对删除：否则 release 之后它仍每秒投递 SHM_CMD_UPDATE_ALL，
 * 唤醒系统去跑已释放的模块，系统无法进入稳定低功耗。 */
static u16 shm_update_tick_timer_id = 0;

static u32 shm_msec_to_next_hour(void)
{
    struct sys_time time = {0};
    u32 left;

    rtc_read_time(&time);
    left = 3600000u - (((u32)time.min * 60u + (u32)time.sec) * 1000u);

    /* 记下本次排定要唤醒的整点时刻：截到整点后再进一小时。
     * 完整记年月日时分秒，唤醒时才能用绝对秒差判出「只改日期不改钟点」这类调整 */
    memcpy(&shm_hour_wake_expect, &time, sizeof(struct sys_time));
    shm_hour_wake_expect.min = 0;
    shm_hour_wake_expect.sec = 0;
    datetime_add_seconds(&shm_hour_wake_expect, 3600);

    if (left < 1000u) {
        /* 已经落在整点这一秒内，本轮来不及，顺延到再下一个整点 */
        left = 3600000u;
        datetime_add_seconds(&shm_hour_wake_expect, 3600);
    }
    return left;
}

static void shm_hour_wake_timer_cb(void *priv)
{
    struct sys_time time = {0};
    int drift;

    rtc_read_time(&time);
    /* 唤醒时刻与排定整点的绝对偏差(秒)：时钟被整体平移时偏差随之放大，
     * 正常按点唤醒只有定时器抖动的几秒 */
    drift = get_seconds_delta(&shm_hour_wake_expect, &time);
    if (drift < 0) {
        drift = -drift;
    }

    if (drift > SHM_HOUR_DRIFT_TOLERATE_MIN * 60) {
        /* 时钟被调整过：这一段采样数据的时间归属已不可信，整段作废、不落盘。
         * 小核环内的点由环形缓存自然覆盖，不需要额外排空。 */
        log_error("%s wake drift %ds, now %02d-%02d %02d:%02d, drop this hour",
                  __func__, drift, (int)time.month, (int)time.day, (int)time.hour, (int)time.min);
    } else {
        /* 复用 1s 节拍同一个入口，整点判断仍由各模块按 RTC 做，避免两套整点逻辑 */
        sport_health_manager_msg_post(SHM_MOD_BEGIN, SHM_CMD_UPDATE_ALL, NULL, 0);
    }

    if (shm_hour_wake_timer_id) {
        sys_timer_modify(shm_hour_wake_timer_id, shm_msec_to_next_hour());
    }
}

static void shm_hour_wake_timer_start(void)
{
    u32 msec = shm_msec_to_next_hour();

    shm_hour_wake_timer_id = sys_timer_add(NULL, shm_hour_wake_timer_cb, msec);
    if (!shm_hour_wake_timer_id) {
        log_error("%s sys_timer_add fail", __func__);
    } else {
        log_info("%s first in %dms", __func__, msec);
    }
}

#if TCFG_ACCELER_SLEEP_ENABLE
void sensor_driver_irq_handle(P33_IO_WKUP_EDGE edge)
{
    sport_health_manager_msg_post(SHM_MOD_GSENSOR_ALGO, SHM_CMD_INIT, NULL, 0);
}

//只读配置，用户后续新增io唤醒、ad唤醒注意使用const修饰，节约静态ram
static const struct _p33_io_wakeup_config sensor_irq = {
    .pullup_down_mode  = PORT_INPUT_PULLUP_10K,
    .filter      		= PORT_FLT_DISABLE,
    .edge               = RISING_EDGE,
    .gpio              = IO_PORTB_03,
    .callback          = sensor_driver_irq_handle,
};

void sensor_irq_init(void)
{
    p33_io_wakeup_port_init(&sensor_irq);
    p33_io_wakeup_enable(IO_PORTB_03, 1);
}

#endif

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_task 运动健康管理主线程
 */
/* ------------------------------------------------------------------------------------*/
void sport_health_manager_task()
{

    log_debug("%s %d", __func__, __LINE__);
//todo
    int ret;
    int msg[32];

#if TCFG_SENSOR_HUB
    sensor_driver_check();
#endif
#if TCFG_ACCELER_SLEEP_ENABLE
    sensor_irq_init();
#endif

    sport_health_module_init();

#if TCFG_SPORT_HEALTH_TEST_ENABLE
    /* 测试钩子：跑一次自检并设定测试用 RTC。
     * 必须在 health_manager 任务上下文调用——模块回调依赖本任务的消息队列，
     * 且 rtc_write_time 要求 RTC 驱动已初始化（app_main.c:478 早于本任务创建）。
     * 交付态 TCFG_SPORT_HEALTH_TEST_ENABLE=0，本段不编译。 */
    health_manager_test_run();
#endif

    shm_update_tick_timer_id = sys_s_hi_timer_add(NULL, sport_health_manager_update_timer_cb,
                               TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS);
    if (!shm_update_tick_timer_id) {
        log_error("%s sys_s_hi_timer_add fail", __func__);
    }

    /* 整点兜底唤醒：1s 节拍在低功耗下周期会漂，整点可能被推后甚至跳过。
     * 本 timer 周期为「距下一整点」，每小时只唤醒一次（详见 shm_hour_wake_timer_cb 注释）。 */
    shm_hour_wake_timer_start();

#if TCFG_SENSOR_HUB_P11
    /* 大核周期向小核拉取 gsensor 算法数据（请求-应答 P2M 回填） */
    sensor_driver_p11_get_data(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO);
#endif
    while (1) {
        /* memset(msg, 0, 32); */
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }

        int module = msg[0];
        int module_cmd = msg[1];
        int module_priv = msg[2];
        int msg_pnd = msg[3];
        log_debug("taskloop module:%d cmd:%d", module, module_cmd);
        ASSERT(module < SHM_MOD_END);
        ASSERT(module_cmd < SHM_CMD_MAX);

        if (module_cmd == SHM_CMD_UPDATE_ALL) {
            sport_health_manager_update_sec_all();
#if TCFG_SENSOR_HUB_P11
            /* 周期拉取 HR/SPO2/sleep 算法数据（这三个模块的 UPDATE_SEC 被 __info.status
             * 门控挡住，故在此显式补触发一次跨核请求） */
            sensor_driver_p11_get_data(SHM_MOD_HEART_RATE_ALGO, SHM_GET_TYPE_REAL_VALUE);
            sensor_driver_p11_get_data(SHM_MOD_SPO2_ALGO, SHM_GET_TYPE_REAL_VALUE);
            sensor_driver_p11_get_data(SHM_MOD_SLEEP, SHM_GET_TYPE_REAL_VALUE);
#endif
            if (msg_pnd) {
                os_sem_post((OS_SEM *)msg_pnd);
            }
            continue;
        }

        struct sport_health_module *mod;
        list_for_each_sport_health_module(mod) {
            if (mod->module == module) {
                if (mod->io_ctrl) {
                    mod->io_ctrl(module_cmd, (void *)module_priv);
                }
                break;
            }
        }
        if (msg_pnd) {
            os_sem_post((OS_SEM *)msg_pnd);
        }
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_init 运动健康管理初始化
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_health_manager_init()
{

#if TCFG_SPORT_HEALTH_ENABLE
    log_debug("%s %d", __func__, __LINE__);
    clock_alloc("health", 8 * MHz);
    //init probe todo
    int ret = SHM_ERR_OK;
    int err = task_create(sport_health_manager_task, NULL, SPORT_HEALTH_MANAGER_TASK);
    if (err) {
        log_warn("%s os_err:%d \n", __func__, err);
        ret = -SHM_ERR_OS_ERR;
    }
    return ret;
#else
    return  0;
#endif
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_release
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_health_manager_release()
{
    int ret = SHM_ERR_OK;
#if TCFG_SPORT_HEALTH_ENABLE
    /* 两个定时器都不会随模块释放自动消亡，必须显式删除：否则 release 之后
     * 它们仍按周期唤醒系统、向已 release 的模块投递 SHM_CMD_UPDATE_ALL，
     * 系统无法进入稳定低功耗。1s 节拍频率最高，先停它。 */
    if (shm_update_tick_timer_id) {
        sys_s_hi_timer_del(shm_update_tick_timer_id);
        shm_update_tick_timer_id = 0;
    }
    if (shm_hour_wake_timer_id) {
        sys_timer_del(shm_hour_wake_timer_id);
        shm_hour_wake_timer_id = 0;
    }
    sport_health_module_release();
    clock_free("health");
    /* #if 0 */
    /* int err	= task_kill(SPORT_HEALTH_MANAGER_TASK); */
    /* if (err) { */
    /* log_warn("%s os_err:%d \n", __func__, err); */
    /* ret = -SHM_ERR_OS_ERR; */
    /* } */
    /* #endif */

#endif
    return ret;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_save_one_with_timeout 请求单模块落盘并限时等待
 *
 * @param module        模块
 * @param cmd           落盘命令（SHM_CMD_SAVE_SINGLE / SHM_CMD_SAVE_CONTINUE）
 * @param timeout_tick  等待上限，单位 tick（msecs_to_jiffies 换算，1 tick = 10ms）
 *
 * @return SHM_ERR_OK，或负错误码（任务不存在 / 等待超时）
 * @note 与 sport_health_manager_msg_post(..., need_pend=1) 的差别：后者 os_sem_pend
 *       超时传 0 表示永久等待，关机路径不可用；此处带超时上限。
 */
/* ------------------------------------------------------------------------------------*/
#if TCFG_SPORT_HEALTH_ENABLE
static int sport_health_manager_save_one_with_timeout(int module, int cmd, int timeout_tick)
{
    /* 必须是 static：等待超时后管理任务仍可能对同一个 sem 执行 os_sem_post，
     * 若放在栈上则 post 会写到已失效的地址。static 复用最坏只是多一次无人
     * 等待的 post，下次 os_sem_create 会重置计数。关机路径为单线程顺序调用。 */
    static OS_SEM sem;
    int msg[4] = {0};

    os_sem_create(&sem, 0);
    /* 上一次调用超时返回后，管理任务仍可能对同一个 sem 补一次 os_sem_post（任务队列
     * 里排着，只是晚到）。OS_SEM 的 create 不清等待链表/残留计数，若不清掉这次迟到
     * 的 post，本次 os_sem_pend 会立即返回成功，把未完成的落盘误判为已完成。
     * os_sem_pend 超时 -1 走非阻塞语义，返回 OS_TIMEOUT 即表示无残留计数。 */
    if (os_sem_pend(&sem, -1) != OS_TIMEOUT) {
        log_warn("%s mod:%d drop stale post", __func__, module);
    }
    msg[0] = cmd;
    msg[1] = 0;
    msg[2] = (int)&sem;

    int err = os_taskq_post_type(SPORT_HEALTH_MANAGER_TASK, module, 3, msg);
    if (err) {
        /* 管理任务未创建（task_create 失败或 TCFG_SPORT_HEALTH_ENABLE=0） */
        log_warn("%s mod:%d cmd:%d os_err:%d", __func__, module, cmd, err);
        return -SHM_ERR_OS_ERR;
    }
    if (os_sem_pend(&sem, timeout_tick) == OS_TIMEOUT) {
        log_warn("%s mod:%d cmd:%d timeout(%dtick)", __func__, module, cmd, timeout_tick);
        return -SHM_ERR_OS_ERR;
    }
    return SHM_ERR_OK;
}
#endif

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_manager_save_all_on_poweroff 关机前统一落盘入口
 *
 * @return SHM_ERR_OK 表示清单内模块全部落盘完成
 * @note 落盘清单对齐需求书【btsdk】第 2 / 4 条：日常活动量、睡眠、心率、血氧。
 *       总等待上限 SHM_POWEROFF_SAVE_BUDGET_MS，超出即放弃剩余模块，优先保证
 *       关机不被拖死（超时则用已缓存的值落盘后继续关机）。
 *       不在此处重新向 pmc 拉取数据：跨核取数是异步应答，关机窗口内等不到回传，
 *       落盘用的是最近一次回传的算法值——需求书第 6 条允许秒级误差。
 */
/* ------------------------------------------------------------------------------------*/
int sport_health_manager_save_all_on_poweroff(void)
{
    int ret = SHM_ERR_OK;
#if TCFG_SPORT_HEALTH_ENABLE
    static const struct {
        u8 module;
        u8 cmd;
    } save_list[] = {
        { SHM_MOD_DAILY_ACTIVE, SHM_CMD_SAVE_SINGLE },
        { SHM_MOD_SLEEP,        SHM_CMD_SAVE_SINGLE },
        { SHM_MOD_HEART_RATE,   SHM_CMD_SAVE_SINGLE },
        { SHM_MOD_BLOOD_OXYGEN, SHM_CMD_SAVE_SINGLE },
    };

    unsigned long start = jiffies_msec();
    for (unsigned int i = 0; i < ARRAY_SIZE(save_list); i++) {
        int used = jiffies_msec2offset(start, jiffies_msec());
        int left = SHM_POWEROFF_SAVE_BUDGET_MS - used;
        int rest = (int)(ARRAY_SIZE(save_list) - i);
        if (left <= 0) {
            log_warn("%s budget(%dms) used up, skip %d module(s)",
                     __func__, SHM_POWEROFF_SAVE_BUDGET_MS, rest);
            ret = -SHM_ERR_OS_ERR;
            break;
        }
        /* 剩余预算按剩余模块数均分，保证每个模块都至少拿到一次命令投递机会；
         * 单模块仍不超过 SHM_POWEROFF_SAVE_ONE_MS，避免一个慢模块吃掉全部预算。 */
        int one = left / rest;
        if (one > SHM_POWEROFF_SAVE_ONE_MS) {
            one = SHM_POWEROFF_SAVE_ONE_MS;
        }
        int tick = msecs_to_jiffies(one);
        if (tick < 1) {
            tick = 1;/* os_sem_pend 超时 0 表示永久等待，至少留 1 tick */
        }
        if (sport_health_manager_save_one_with_timeout(save_list[i].module, save_list[i].cmd, tick)) {
            ret = -SHM_ERR_OS_ERR;
        }
    }
    log_info("%s cost:%dms ret:%d", __func__, jiffies_msec2offset(start, jiffies_msec()), ret);
#endif
    return ret;
}


