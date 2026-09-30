#pragma once
#include "board_config.h"
#include "cpu.h"
#include "includes.h"
#include "sensor_driver.h"
#include "sensor_driver_p11.h"
#include "sensor_algorithm_jl_motion.h"



typedef struct {
    u8 dev_type;
    sensor_info_t *(*dev_info)(void);
    s8(*dev_init)(u8 enable);
    s8(*dev_cbuf)(u8 index, u8 enable);
    u16(*dev_get)(u8 index, void *buff, u16 buff_len);
} SENSOR_HAL_INTERFACE;

extern SENSOR_HAL_INTERFACE  sensor_hal_begin[];
extern SENSOR_HAL_INTERFACE sensor_hal_end[];

#define REGISTER_SENSOR_HAL(sensorhal) \
	static const SENSOR_HAL_INTERFACE sensorhal SEC_USED(.sensor_hal)

#define list_for_each_sensor_hal(c) \
	for (c=sensor_hal_begin; c<sensor_hal_end; c++)


sensor_info_t  *sensor_hub_get_info(u8 type);
sensor_state_t sensor_hub_get_sensor_state(u8 type);
s8 sensor_hub_enable(u8 type, u8 enable);
s8 sensor_hub_cbuf_enable(u8 type, u8 index, u8 enable);
u16 sensor_hub_get_data(u8 type, u8 index, void *buff, u16 buff_len);
void sensor_hub_run(void);

/* 抬腕检测闸门：占用方（罗盘页）置 1 期间开关变化只记意图不下发，避免抢 gsensor；
 * 释放时置 0 并自行发 ENABLE 恢复。链路未编译时为空实现。 */
void wrist_tilt_set_suspended(u8 suspended);



void sensor_service_motion_init();
void sensor_service_motion_get(struct algo_value *buf);
void sensor_service_motion_cfg_update();
