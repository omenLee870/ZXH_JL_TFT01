#include "health_manager.h"
#include "usr_timer.h"
#include "sdk_config.h"
#include "init.h"
#if 1
#pragma bss_seg(".sensor_bss.L1")
#pragma data_seg(".sensor_data.L1")
#pragma code_seg(".sensor_code.L1")
#pragma const_seg(".sensor_const")
#else
#pragma bss_seg(".sensor_bss.L2")
#pragma data_seg(".sensor_data.L2")
#pragma code_seg(".sensor_code.L2")
#pragma const_seg(".sensor_const")
#endif
#define LOG_TAG_CONST       SENSOR_MANAGER
#define LOG_TAG     		"[SENSOR_MANAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"

#if CONFIG_SPORT_HEALTH_MANAGER_ENABLE

#define SENSOR_MANAGER_TEST_ENABLE			1

static int test_steps = 0;
static int test_hr  = 0;
static int tets_spo2 = 0;


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
        log_error("%s err:%d rets:0x%x module:0x%x type:0x%x\n", __func__, ret, rets, module, type);
    } else {
        log_debug("%s module:0x%x type:0x%x succ\n", __func__, module, type);
    }
    return ret;
}

int sport_health_manager_get_value_m2p(int module, int type)
{
    int ret = -1;
    health_data_msg_t msg = {0};
    switch (module) {
    case SHM_MOD_GSENSOR_ALGO:
        if (CONFIG_SPORT_HEALTH_MANAGER_DEBUG & BIT(0)) {
            test_steps ++;
            msg.data.acceler.steps = test_steps;
            ret = 0;
        }
        break;
    case SHM_MOD_HEART_RATE_ALGO:
        if (CONFIG_SPORT_HEALTH_MANAGER_DEBUG & BIT(1)) {
            test_hr++;
            test_hr %= 20;
            msg.data.hr_data.real = 90 + test_hr;
            ret = 0;
        }
        break;
    case SHM_MOD_SPO2_ALGO:
        if (CONFIG_SPORT_HEALTH_MANAGER_DEBUG & BIT(2)) {
            tets_spo2++;
            tets_spo2 %= 10;
            msg.data.spo2_data.real = 90 + tets_spo2;
            ret = 0;
        }
        break;
    default:
        break;
    }
    if (ret) {
        ret = sport_health_manager_value_get(module, type, (void *)&msg.data);
    }

    if (!ret) {
        msg.msg = MSG_P11_HEALTH_MANAGER_DATA_P2M;
        msg.module = module;
        msg.type = type;

        p2m_post_msg(MSG_APP, 0, (u8 *)&msg, sizeof(health_data_msg_t));
        /* log_debug("%s module:%d type:%d (len:%d)", __func__, module, type, (u32)sizeof(health_data_msg_t)); */
        /* put_buf((u8 *)&msg, sizeof(health_data_msg_t)); */
    }
    return ret;
}
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
int sport_health_manager_msg_post(int module, int cmd, int priv_len, void *priv)
{
    int rets;
    __asm__ volatile("%0 = rets":"=r"(rets));

    int ret = SHM_ERR_OK;
#define MSG_LEN 7//3+sizeof(addr)
    u8 msg[MSG_LEN];
    msg[0] = MSG_P11_HEALTH_MANAGER_DATA_P2P;
    msg[1] = module;
    msg[2] = cmd;
    ASSERT(priv_len <= (MSG_LEN - 3));
    memcpy(msg + 3, (u8 *)priv, priv_len);
    m2p_post_msg_q32f(MSG_APP, 0, msg, MSG_LEN);
    return ret;
}
static void sport_health_module_init()
{
    log_debug("%s %d 0x%x 0x%x", __func__, __LINE__, (u32)sport_health_module_begin, (u32)sport_health_module_end);
    struct sport_health_module *mod;
    list_for_each_sport_health_module(mod) {
        log_debug("%s %d", __func__, mod->module);
        if (mod->io_ctrl) {
            mod->io_ctrl(SHM_CMD_INIT, NULL);
        }
    }
}

static void sport_health_manager_update_sec_all()
{
    /* printf("%s %d", __func__, __LINE__); */

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

void sport_health_manager_task()
{
    sport_health_manager_update_sec_all();
}
void sport_health_manager_sec_update(void *priv)
{
    printf("%s\n", __func__);
    task_post_msg(NULL,  MSG_SENSOR, MSG_P11_HEALTH_MANAGER_LOOP);
}
int sport_health_manager_init()
{
    printf("%s\n", __func__);
    sensor_driver_check();
    sport_health_module_init();
    usr_timer_add(NULL,  sport_health_manager_sec_update, 1000, 0);
    return 0;
}
module_initcall(sport_health_manager_init);

/* static void health_manager_msg_handler(void *priv, u8 *msg, u32 len) */
/* { */
/* switch (msg[0]) { */

/* } */
/* } */

/* REGISTER_M2P_MSG_HANDLER(0, MSG_SENSOR,  health_manager_msg_handler); */

#endif

