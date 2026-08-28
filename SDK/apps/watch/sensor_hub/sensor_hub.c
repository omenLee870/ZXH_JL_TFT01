#include "sensor_hub.h"
#include "sensor_driver_p11.h"

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HUB]"
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
static SENSOR_HAL_INTERFACE *sensor_hal = NULL;


s8 sensor_hub_find(u8 type)
{
    list_for_each_sensor_hal(sensor_hal) {
        if (sensor_hal->dev_type == type) {
            return RET_OK;
        }
    }

    log_error("%s type:%d not found", __func__, type);
    return RET_ERR;
}

sensor_info_t  *sensor_hub_get_info(u8 type)
{
    if ((sensor_hub_find(type) != RET_OK) || (sensor_hal->dev_info == NULL)) {
        log_error("%s type:%d ", __func__, type);
        return NULL;
    }
    return  sensor_hal->dev_info();
}

sensor_state_t sensor_hub_get_sensor_state(u8 type)
{
    sensor_info_t  *sensor_info = sensor_hub_get_info(type);
    if (sensor_info == NULL) {
        return 0;
    }
    return  sensor_info->state;
}


s8 sensor_hub_enable(u8 type, u8 enable)
{
    log_info("type=%d enable=%d", type, enable);
    if ((sensor_hub_find(type) != RET_OK) || (sensor_hal->dev_init == NULL)) {
        return RET_ERR;
    }
    return  sensor_hal->dev_init(enable);
}

u16 sensor_hub_get_data(u8 type, u8 index, void *buff, u16 buff_len)
{
    if ((sensor_hub_find(type) != RET_OK) || (sensor_hal->dev_get == NULL)) {
        log_error("%s type:%d index:%d ", __func__, type, index);
        return 0;
    }

    if (sensor_hub_get_sensor_state(type) != SENSOR_STATE_OPEN) {
        log_error("%s sensor state err,type:%d", __func__, type);
        return 0;
    }

    return  sensor_hal->dev_get(index, buff, buff_len);
}

s8 sensor_hub_cbuf_enable(u8 type, u8 index, u8 enable)
{
    if ((sensor_hub_find(type) != RET_OK) || (sensor_hal->dev_cbuf == NULL)) {
        log_error("type=%d cbuf index=%d enable=%d", type, index, enable);
        return RET_ERR;
    }
    return  sensor_hal->dev_cbuf(index, enable);
}
#endif//TCFG_SENSOR_HUB
