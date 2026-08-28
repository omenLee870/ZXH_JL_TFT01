#include "health_manager.h"
#include "sensor_driver.h"
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
#define LOG_TAG_CONST      	SENSOR_HAL
#define LOG_TAG     		"[SENSOR_HAL_ACC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"

#if TCFG_SENSOR_HUB
#if TCFG_SENSOR_HAL_ACCELER_ENABLE

#define THIS_SENSOR_TYPE  SENSOR_DRV_ACCELER

static sensor_info_t *sensor_info(void)
{
    static sensor_info_t *info = NULL;

    if (info == NULL) {
        info = sensor_driver_info_get(THIS_SENSOR_TYPE);
    }
    return info;
}

static s8 sensor_enable(u8 enable)
{
    sensor_driver_init(THIS_SENSOR_TYPE, enable, 4, 25);
    return RET_OK;
}

static s8 sensor_cbuf_enable(u8 index, u8 enable)
{
    void sensor_driver_cbuf_enable(u8 type, u8 index, u8 enable);
    sensor_driver_cbuf_enable(THIS_SENSOR_TYPE, index, enable);
    return RET_OK;
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
            sensor_driver_sleep(THIS_SENSOR_TYPE, 1);
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
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(THIS_SENSOR_TYPE);
    p11_cbuffer_t *cbuffer = sensor_driver->info->cbuffer;
    u16 get_len  = cbuf_mult_read_get_data_len(cbuffer, index);
    len = cbuf_mult_read_alloc_len(cbuffer, index, data_buff, get_len);
    if (len) {
        cbuf_mult_read_alloc_len_updata(cbuffer, index, len);
    }
    /* len = sensor_driver_data_get(THIS_SENSOR_TYPE, data_buff, data_len) ; */
    log_debug("%s type:%d get_data_len:%d cbuflen:%d", __func__, THIS_SENSOR_TYPE, len, cbuf_mult_read_get_data_len(cbuffer, index));

    len /= 6;
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

