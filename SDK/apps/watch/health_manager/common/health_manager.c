
#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"
#include "clock_manager/clock_manager.h"
#include "health_manager/health_manager.h"
#include "sensor_hub.h"
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
        log_error("%s err:%d rets:0x%x", __func__, ret, rets);
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

    sys_s_hi_timer_add(NULL, sport_health_manager_update_timer_cb, 1000);
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


