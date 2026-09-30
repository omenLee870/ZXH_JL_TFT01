
#include "health_manager.h"
#include "sensor_hub.h"
#include "sensor_seg.h"
/* L0/L1/L2 → RAM；L3 → Flash（FIFO 类同向，函数级 AT 覆盖） */
#if TCFG_SENSOR_GSENSOR_LEVEL <= 2
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
#define LOG_TAG     		"[SENSOR_HUB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"

#if TCFG_SENSOR_HUB
static SENSOR_HAL_INTERFACE *sensor_hal = NULL;


AT_SENSOR_FIFO s8 sensor_hub_find(u8 type)
{
    list_for_each_sensor_hal(sensor_hal) {
        if (sensor_hal->dev_type == type) {
            return RET_OK;
        }
    }

    log_error("%s type:%d not found", __func__, type);
    return RET_ERR;
}

AT_SENSOR_FIFO sensor_info_t  *sensor_hub_get_info(u8 type)
{
    if ((sensor_hub_find(type) != RET_OK) || (sensor_hal->dev_info == NULL)) {
        log_error("%s type:%d ", __func__, type);
        return NULL;
    }
    return  sensor_hal->dev_info();
}

AT_SENSOR_FIFO sensor_state_t sensor_hub_get_sensor_state(u8 type)
{
    sensor_info_t  *sensor_info = sensor_hub_get_info(type);
    if (sensor_info == NULL) {
        return 0;
    }
    return  sensor_info->state;
}


AT_SENSOR_COLD s8 sensor_hub_enable(u8 type, u8 enable)
{
    log_info("type=%d enable=%d", type, enable);
    if ((sensor_hub_find(type) != RET_OK) || (sensor_hal->dev_init == NULL)) {
        return RET_ERR;
    }
    return  sensor_hal->dev_init(enable);
}

AT_SENSOR_FIFO u16 sensor_hub_get_data(u8 type, u8 index, void *buff, u16 buff_len)
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

AT_SENSOR_COLD s8 sensor_hub_cbuf_enable(u8 type, u8 index, u8 enable)
{
    if ((sensor_hub_find(type) != RET_OK) || (sensor_hal->dev_cbuf == NULL)) {
        log_error("type=%d cbuf index=%d enable=%d", type, index, enable);
        return RET_ERR;
    }
    return  sensor_hal->dev_cbuf(index, enable);
}
#endif//TCFG_SENSOR_HUB

