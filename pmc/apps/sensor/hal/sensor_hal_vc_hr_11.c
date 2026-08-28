#include "health_manager.h"
#include "vcHr11Hci.h"
#include "driver/driver_vc_hr_11/algo.h"
#include "driver/driver_vc_hr_11/spo2Algo.h"
#include "driver/driver_vc_hr_11/vcSportMotionIntAlgo.h"

#define LOG_TAG_CONST      	SENSOR_HAL
#define LOG_TAG     		"[SENSOR_HAL_HR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"

#if TCFG_SENSOR_HUB
#if TCFG_SENSOR_HAL_VCHR11_ENABLE

#define THIS_SENSOR_TYPE  SENSOR_DRV_HR_SPO2
static sensor_info_t *sensor_info(void)
{
    sensor_info_t *info = NULL;
    info = sensor_driver_info_get(THIS_SENSOR_TYPE);
    return info;
}


static s8 vc_11_start(u8 enable, vcHr11Mode_t mode)
{
    log_info("%s enable:%d mode:%d", __func__, enable, mode);
    sensor_driver_init(THIS_SENSOR_TYPE, enable, mode, 25);
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
    return sensor_driver_data_get(THIS_SENSOR_TYPE, data_buff, data_len) / 2;
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


