#include "sensor_hub.h"
#include "sensor_driver_p11.h"
#include "syscfg_id.h"

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HAL_ACC]"
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
#if TCFG_SENSOR_HUB
#if TCFG_ACCELER_P11_ENABLE || TCFG_ACCELER_MASTER_ENABLE

#define THIS_SENSOR_TYPE  SENSOR_DRV_ACCELER

static sensor_info_t *sensor_info(void)
{
    static sensor_info_t *info = NULL;

    if (info == NULL) {
#if TCFG_ACCELER_MASTER_ENABLE
        info = sensor_driver_info_get(THIS_SENSOR_TYPE);
#endif
#if TCFG_ACCELER_P11_ENABLE
        info = sensor_driver_p11_info_get(THIS_SENSOR_TYPE);
#endif
    }
    return info;
}

static s8 sensor_enable(u8 enable)
{
#if TCFG_ACCELER_MASTER_ENABLE
    sensor_driver_init(THIS_SENSOR_TYPE, enable, 4, 25);
#endif
#if TCFG_ACCELER_P11_ENABLE
    sensor_driver_p11_init(THIS_SENSOR_TYPE, enable, 4, 25);
#endif
    return RET_OK;
}


static s8 sensor_cbuf_enable(u8 index, u8 enable)
{
#if TCFG_ACCELER_MASTER_ENABLE
    return RET_ERR;
#endif
#if TCFG_ACCELER_P11_ENABLE
    sensor_driver_p11_cbuf_mult_entry_enable(THIS_SENSOR_TYPE, index, enable);
    return RET_OK;
#endif
}

#if TCFG_ACCELER_SLEEP_ENABLE

#define SLEEP_THRES  (150)
#define SLEEP_TIME   (30*25)

static short last_x = 0, last_y = 0, last_z = 0, last_len = 0;
static bool sensor_sleep(axis_data_t *axis, u16 len)
{
    static u16 still_count = 0;
    static u8  sleep_mode  = 0;

    int diff_x = 0, diff_y = 0, diff_z = 0;
    for (int i = 1; i < len; i++) {
        diff_x = axis[i].x - axis[i - 1].x;
        diff_y = axis[i].y - axis[i - 1].y;
        diff_z = axis[i].z - axis[i - 1].z;
        // printf("diff_xyz:%d,%d,%d,%d\n", diff_x,diff_y,diff_z, still_count);
        if (diff_x > SLEEP_THRES || diff_x < -SLEEP_THRES || diff_y > SLEEP_THRES
            || diff_y < -SLEEP_THRES || diff_z > SLEEP_THRES || diff_z < -SLEEP_THRES) {
            still_count = 0;
            sleep_mode  = 0;
            break;
        }
        still_count++;
        if (still_count == SLEEP_TIME) {
#if TCFG_ACCELER_MASTER_ENABLE
            sensor_driver_sleep(THIS_SENSOR_TYPE, 1);
#endif
#if TCFG_ACCELER_P11_ENABLE
            sensor_driver_p11_sleep(THIS_SENSOR_TYPE, 1);
#endif
            still_count = 0;
            sleep_mode  = 1;
            last_x = axis[i].x;
            last_y = axis[i].y;
            last_z = axis[i].z;
            last_len = len;
        }
    }
    return sleep_mode;
}

static u16 sensor_last_data_get(axis_data_t *axis, u16 len)
{
    if (len == 0) {
        for (u16 i = 0; i < last_len; i++) {
            axis[i].x = last_x;
            axis[i].y = last_y;
            axis[i].z = last_z;
        }
        return last_len;
    }
    return len;
}
#endif
static u16 sensor_raw_data_get(u8 index, void *data_buff, u16 data_len)
{
    u16 len = 0;
#if TCFG_ACCELER_MASTER_ENABLE
    len = sensor_driver_data_get(THIS_SENSOR_TYPE, data_buff, data_len) / 6;
#endif
#if TCFG_ACCELER_P11_ENABLE
    len = sensor_driver_p11_data_get(THIS_SENSOR_TYPE, index, data_buff, data_len) / 6;
#endif
#if TCFG_ACCELER_SLEEP_ENABLE
    if (sensor_sleep(data_buff, len)) {
#if TCFG_SPORT_HEALTH_SLEEP//支持睡眠算法时，需要持续喂数据给算法
        len = sensor_last_data_get(data_buff, len);
#endif
    }
#endif
    return len;
}



REGISTER_SENSOR_HAL(sensor_hal) = {
    .dev_type    = THIS_SENSOR_TYPE,
    .dev_info    = sensor_info,
    .dev_init    = sensor_enable,
    .dev_cbuf    = sensor_cbuf_enable,
    .dev_get     = sensor_raw_data_get,
};

#endif
#endif
