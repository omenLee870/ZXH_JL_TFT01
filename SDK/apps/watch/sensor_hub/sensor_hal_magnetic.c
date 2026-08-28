#include "sensor_hub.h"
#include "sensor_driver_p11.h"
#include "syscfg_id.h"

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HAL_MAG]"
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
#if TCFG_MAGNETIC_P11_ENABLE | TCFG_MAGNETIC_MASTER_ENABLE

#define THIS_SENSOR_TYPE  SENSOR_DRV_MAGNETIC

static sensor_info_t *sensor_info(void)
{
    sensor_info_t *info = NULL;
#if TCFG_MAGNETIC_MASTER_ENABLE
    info = sensor_driver_info_get(THIS_SENSOR_TYPE);
#endif
#if TCFG_MAGNETIC_P11_ENABLE
    info = sensor_driver_p11_info_get(THIS_SENSOR_TYPE);
#endif
    return info;
}

static s8 sensor_enable(u8 enable)
{
#if TCFG_MAGNETIC_MASTER_ENABLE
    sensor_driver_init(THIS_SENSOR_TYPE, enable, 4, 25);
#endif
#if TCFG_MAGNETIC_P11_ENABLE
    sensor_driver_p11_init(THIS_SENSOR_TYPE, enable, 4, 25);
#endif
    return RET_OK;
}

static u16 sensor_raw_data_get(u8 index, void *data_buff, u16 data_len)
{
#if TCFG_MAGNETIC_MASTER_ENABLE
    return sensor_driver_data_get(THIS_SENSOR_TYPE, data_buff, data_len) / 6;
#endif
#if TCFG_MAGNETIC_P11_ENABLE
    return sensor_driver_p11_data_get(THIS_SENSOR_TYPE, index, data_buff, data_len) / 6;
#endif
}


REGISTER_SENSOR_HAL(sensor_hal) = {
    .dev_type    = THIS_SENSOR_TYPE,
    .dev_info    = sensor_info,
    .dev_init    = sensor_enable,
    .dev_get     = sensor_raw_data_get,
};

#endif
#endif
