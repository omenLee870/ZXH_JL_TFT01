#include "sensor_driver_p11.h"
#include "power/p11_app_msg.h"
#include "health_manager.h"
#include "shm_info_storage.h"
#include "asm/power/pmc/pmc_mmap.h"
#include "power/p11_cbuf.h"
#if(TCFG_SENSOR_HUB&&TCFG_SENSOR_HUB_P11)
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".p11_cbuf.data.bss")
#pragma data_seg(".p11_cbuf.data")
#pragma const_seg(".p11_cbuf.text.const")
#pragma code_seg(".p11_cbuf.text")
#endif


#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HUB_P11]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sensor_hub.data.bss")
#pragma data_seg(".sensor_hub.data")
#pragma const_seg(".sensor_hub.text.const")
#pragma code_seg(".sensor_hub.text")
#endif
static u32  p11_sensor_dev_begin = 0;
static u32  p11_sensor_dev_end   = 0;


SENSOR_INTERFACE *sensor_driver_p11_find(sensor_type_t type)
{
    sensor_info_t *sensor_info = NULL;
    SENSOR_INTERFACE *sensor_driver = NULL;
    for (sensor_driver = (SENSOR_INTERFACE *) p11_sensor_dev_begin; (u32)sensor_driver < p11_sensor_dev_end; sensor_driver++) {
        sensor_info = (sensor_info_t *)((u32)sensor_driver->info + P11_RAM_BASE);
        // log_debug("type=%d sensor_driver=%x type=%d sensor_info=%x %x", type, sensor_driver, sensor_info->type, sensor_info, sensor_driver->info);
        if (sensor_info->type == type) {
            // log_info("type=%d sensor_driver=%x", type, sensor_driver);
            return sensor_driver;
        }
    }
    log_error("type=%d sensor_driver is null", type);
    return NULL;
}

sensor_info_t *sensor_driver_p11_info_get(sensor_type_t type)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_p11_find(type);
    if (sensor_driver != NULL) {
        return (sensor_info_t *)((u32)(sensor_driver->info) + P11_RAM_BASE);
    }
    return NULL;
}

u16 sensor_driver_p11_cbuf_mult_entry_enable(sensor_type_t type, u8 index, u8 enable)
{
    sensor_info_t *sensor_driver = sensor_driver_p11_info_get(type);
    if (sensor_driver == NULL) {
        return 0;
    }
    log_debug("type=%d cbuf_index=%x enable:%d", type, index, enable);
    p11cbuf_mult_entry_enable((p11_cbuffer_t *)sensor_driver->cbuffer, index, enable);
    return 0;
}

u16 sensor_driver_p11_data_get(sensor_type_t type, u8 index, void *data_buff, u16 data_len)
{
    sensor_info_t *sensor_driver = sensor_driver_p11_info_get(type);

    if (sensor_driver == NULL) {
        return 0;
    }

    int buff_len = p11_cbuf_mult_read_get_data_len((p11_cbuffer_t *)sensor_driver->cbuffer, index);
    log_debug("type=%d cbuf_index=%x len=%d", type, index, buff_len);
    if ((buff_len > 0) && (data_len > 0)) {
        if (buff_len < data_len) {
            data_len = buff_len;
        }
        data_len = p11_cbuf_mult_read_alloc_len((p11_cbuffer_t *)sensor_driver->cbuffer,  index, data_buff, data_len);
        if (data_len) {
            p11_cbuf_mult_read_alloc_len_updata((p11_cbuffer_t *)sensor_driver->cbuffer, index, data_len);
        }
    } else {
        data_len = 0;
    }
    return data_len;
}


void sensor_driver_p11_init(sensor_type_t type, u8 enable, u16 range, u8 odr)
{

    log_debug("type=%d range=%d odr=%d", type, range, odr);

    u8 msg[6];
    msg[0] = MSG_P11_SENSOR_INIT;
    msg[1] = type;
    msg[2] = enable;
    msg[3] = range >> 8;
    msg[4] = range & 0xFF;
    msg[5] = odr;
    m2p_post_msg_q32f(MSG_APP, 0, (u8 *)msg, sizeof(msg));
}


void sensor_driver_p11_timer_modify(u16 msec)
{
    u8 msg[3];
    msg[0] = MSG_P11_SENSOR_TIMER;
    msg[1] = msec >> 8;
    msg[2] = msec & 0xFF;
    log_debug("sensor_driver_p11_timer_modify: %d", msec);
    m2p_post_msg_q32f(MSG_APP, 0, (u8 *)msg, sizeof(msg));
}

void sensor_driver_p11_sleep(u8 type, u8 enable)
{
    u8 msg[3];
    msg[0] = MSG_P11_SENSOR_SLEEP;
    msg[1] = type;
    msg[2] = enable;
    printf("sensor_sleep: type=%d\n", type);
    m2p_post_msg_q32f(MSG_APP, 0, (u8 *)msg, sizeof(msg));
}

/* pmc_gsensor：大核向小核请求算法数据（请求-应答拉取模式，参考 br33_sensor） */
int sensor_driver_p11_get_data(u8 module, u8 type)
{
    u8 msg[3];
    msg[0] = MSG_P11_HEALTH_MANAGER_DATA_M2P;
    msg[1] = module;
    msg[2] = type;
    log_debug("%s module:%d type:%d len:(%d)", __func__, module, type, (u32)sizeof(health_data_msg_t));
    m2p_post_msg_q32f(MSG_APP, 0, (u8 *)msg, sizeof(msg));
    return 0;
}
/* pmc_gsensor：大核向小核发送控制命令（开启/关闭算法等） */
int sensor_driver_p11_ctrl(u8 module, u8 cmd)
{
    u8 msg[3];
    msg[0] = MSG_P11_HEALTH_MANAGER_DATA_M2P;
    msg[1] = module;
    msg[2] = cmd;
    /* pmc_hrsensor: 大核侧跨核控制下发埋点（msg15 = MSG_P11_HEALTH_MANAGER_DATA_M2P） */
    log_info("[pmc_hrsensor] %s M2P msg15 module:%d cmd:%d", __func__, module, cmd);
    m2p_post_msg_q32f(MSG_APP, 0, (u8 *)msg, sizeof(msg));
    return 0;
}

/* 解析小核回传算法数据并通知 health_manager 回填；返回 1 已处理、0 未识别。
 * 打印按模块节流（每 5 次回传一次），避免每秒刷屏。 */
static u8 sensor_driver_p11_data_deal(u8 module, u8 type, void *_data)
{
    static u32 cnt_gsensor = 0, cnt_hr = 0, cnt_spo2 = 0, cnt_sleep = 0;
    health_data_u *data = (health_data_u *)_data;
    switch (module) {
    case SHM_MOD_GSENSOR_ALGO:
        if (type == SHM_GET_TYPE_REAL_VALUE) {
            /* bedside 方向通道：小核回传方向值（存于 dir 字段），直接更新缓存 */
            shm_algo_gsensor_dir_update(data->dir);
            return 1;
        } else if (type == SHM_GET_TYPE_INFO) {
            /* 算法数据通道：回填 gsensor_algo_info（daily active/sport 用） */
            if (++cnt_gsensor % 5 == 0) {
                log_info("pmc_gsensor[gsensor] steps:%d sleep:%d cal:%d",
                         data->acceler.steps, data->acceler.sleep, data->acceler.calories);
            }
            sport_health_manager_msg_post_self(module, SHM_CMD_UPDATE_VAILE_P11, data);
            return 1;
        }
        break;
    case SHM_MOD_HEART_RATE_ALGO:
        if (type == SHM_GET_TYPE_SAMPLE_BUF) {
            /* 采样缓存整包：小核按请求时的 module 原样回传，这里转投到大核数据模块
             * （SHM_MOD_HEART_RATE）落盘——绕开 _ALGO 中转，回传当秒即可落盘 */
            log_info("pmc_hrsample[hr] size:%d int:%d", data->sample.data_size, data->sample.sample_int);
            sport_health_manager_msg_post_self(SHM_MOD_HEART_RATE, SHM_CMD_UPDATE_SAMPLE_P11, data);
            return 1;
        }
        if (type == SHM_GET_TYPE_INFO || type == SHM_GET_TYPE_REAL_VALUE) {
            if (++cnt_hr % 5 == 0) {
                log_info("pmc_gsensor[hr] real:%d", data->hr_data.real);
            }
            sport_health_manager_msg_post_self(module, SHM_CMD_UPDATE_VAILE_P11, data);
            return 1;
        }
        break;
    case SHM_MOD_SPO2_ALGO:
        if (type == SHM_GET_TYPE_SAMPLE_BUF) {
            /* 同心率：转投 SHM_MOD_BLOOD_OXYGEN 数据模块 */
            log_info("pmc_hrsample[spo2] size:%d int:%d", data->sample.data_size, data->sample.sample_int);
            sport_health_manager_msg_post_self(SHM_MOD_BLOOD_OXYGEN, SHM_CMD_UPDATE_SAMPLE_P11, data);
            return 1;
        }
        if (type == SHM_GET_TYPE_INFO || type == SHM_GET_TYPE_REAL_VALUE) {
            if (++cnt_spo2 % 5 == 0) {
                log_info("pmc_gsensor[spo2] real:%d", data->spo2_data.real);
            }
            sport_health_manager_msg_post_self(module, SHM_CMD_UPDATE_VAILE_P11, data);
            return 1;
        }
        break;
    case SHM_MOD_SLEEP:
        if (type == SHM_GET_TYPE_INFO || type == SHM_GET_TYPE_REAL_VALUE) {
            if (++cnt_sleep % 5 == 0) {
                log_info("pmc_gsensor[sleep] sleep:%d", data->acceler.sleep);
            }
            sport_health_manager_msg_post_self(module, SHM_CMD_UPDATE_VAILE_P11, data);
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

static void p11_msg_handler(void *priv, u8 *buf, u32 len)
{
    int msg[32];

    /* ASSERT 在 CONFIG_RELEASE_ENABLE 下编译为空，越界保护必须用运行时判断 */
    if (len > sizeof(msg)) {
        log_error("%s len:%d over buf:%d", __func__, len, (int)sizeof(msg));
        return;
    }
    memcpy(msg, buf, len);
    log_debug("p11_msg_handler: msg0=%d len=%d", msg[0], len);

    switch (msg[0]) {
    case   MSG_P11_SENSOR_INFO:
        p11_sensor_dev_begin = msg[1] + P11_RAM_BASE;
        p11_sensor_dev_end   = msg[2] + P11_RAM_BASE;
        log_debug("p11_sensor_dev_begin=%x end=%x", p11_sensor_dev_begin, p11_sensor_dev_end);
        /* PMC 每次启动都会发本消息：PMC 单独复位后它那边的 wrist_enable 归 0，
         * 这里补下发一次开关状态，否则抬腕亮屏永久失效。 */
        sport_info_raise_wrist_switch_apply();
        break;

    case   MSG_P11_SENSOR_EVENT:
        break;

    case   MSG_P11_ALGORITHM_EVENT: {
        /* post 返回非 0 = health_manager 尚未就绪（开机后 1 秒内），该次事件被丢弃 */
        int pret = sport_health_manager_msg_post(SHM_MOD_DET_WRIST, SHM_CMD_UPDATE, (void *)msg[1], 0);
        if (pret) {
            log_error("[wrist] p11->mcu algo_event:%d post fail:%d", msg[1], pret);
        }
        break;
    }

    case MSG_P11_HEALTH_MANAGER_DATA_P2M: {
        /* 载荷布局与 health_data_msg_t 一致：msg/module/type/time/data */
        health_data_msg_t *pmsg = (health_data_msg_t *)msg;
        log_debug("%s module:%d type:%d", __func__, pmsg->module, pmsg->type);
        if (!sensor_driver_p11_data_deal(pmsg->module, pmsg->type, &pmsg->data)) {
            log_error("%s unknown module:%d type:%d", __func__, pmsg->module, pmsg->type);
        }
        break;
    }
    }
}

REGISTER_P2M_MSG_HANDLER(NULL, MSG_APP, p11_msg_handler);

#endif //TCFG_SENSOR_HUB_P11
