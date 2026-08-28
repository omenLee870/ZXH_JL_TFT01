#include "sensor_hub.h"
#include "sensor_driver_p11.h"
#include "vcHr11Hci.h"
#include "syscfg_id.h"

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HAL_HR]"
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
#if TCFG_VCHR11_P11_ENABLE | TCFG_VCHR11_MASTER_ENABLE

#define THIS_SENSOR_TYPE  SENSOR_DRV_HR_SPO2
static sensor_info_t *sensor_info(void)
{
    sensor_info_t *info = NULL;
#if TCFG_VCHR11_MASTER_ENABLE
    info = sensor_driver_info_get(THIS_SENSOR_TYPE);
#endif
#if TCFG_VCHR11_P11_ENABLE
    info = sensor_driver_p11_info_get(THIS_SENSOR_TYPE);
#endif
    return info;
}


static s8 vc_11_start(u8 enable, vcHr11Mode_t mode)
{
    log_info("%s enable:%d mode:%d", __func__, enable, mode);
#if TCFG_VCHR11_MASTER_ENABLE
    sensor_driver_init(THIS_SENSOR_TYPE, enable, mode, 25);
#endif
#if TCFG_VCHR11_P11_ENABLE
    sensor_driver_p11_init(THIS_SENSOR_TYPE, enable, mode, 25);
    sensor_driver_p11_cbuf_mult_entry_enable(THIS_SENSOR_TYPE, 0, enable);
#endif
    return RET_OK;
}

static s8 hrm_enable(u8 enable)
{
    return vc_11_start(enable, VCWORK_MODE_HRWORK);
}

static s8 spo2_enable(u8 enable)
{
    return vc_11_start(enable, VCWORK_MODE_SPO2WORK);
}

static s8 wear_detect_enable(u8 enable)
{
    return vc_11_start(enable, VCWORK_MODE_LPDETECTION);
}

static u16 hrm_spo2_raw_data_get(u8 index, void *data_buff, u16 data_len)
{
    ASSERT(data_buff);
#if TCFG_VCHR11_MASTER_ENABLE
    return sensor_driver_data_get(THIS_SENSOR_TYPE, data_buff, data_len) / 2;
#endif
#if TCFG_VCHR11_P11_ENABLE
    return sensor_driver_p11_data_get(THIS_SENSOR_TYPE, index, data_buff, data_len) / 2;
#endif
}


REGISTER_SENSOR_HAL(sensor_hrm) = {
    .dev_type    = SENSOR_DRV_HR,
    .dev_info    = sensor_info,
    .dev_init    = hrm_enable,
    .dev_get     = hrm_spo2_raw_data_get,
};

REGISTER_SENSOR_HAL(sensor_spo2) = {
    .dev_type    = SENSOR_DRV_SPO2,
    .dev_info    = sensor_info,
    .dev_init    = spo2_enable,
    .dev_get     = hrm_spo2_raw_data_get,
};

REGISTER_SENSOR_HAL(sensor_wear_detect) = {
    .dev_type    = SENSOR_DRV_WEAR_DETECTION,
    .dev_info    = sensor_info,
    .dev_init    = wear_detect_enable,
    .dev_get     = hrm_spo2_raw_data_get,
};

#endif
#endif
