#pragma once
#include "includes.h"
#include "sensor_hub.h"
#include "circular_buf.h"


typedef enum {
    SENSOR_GET_INFO,
    SENSOR_GET_RAW,
} sensor_get_cmd;


typedef enum {
    ALGO_NOTHING,           //没有相关动作
    ALGO_WRIST_UP,          //抬腕动作
    ALGO_WRIST_DOWN,        //落腕动作
    ALGO_DOUBLE_CLICK,      //双击屏幕
    ALGO_HITTING,           //击球动作
    ALGO_ON_DESK,           //放置桌面
} p11_algo_gesture;



void sensor_driver_p11_list(u8 sensor_number, void *data);
sensor_info_t *sensor_driver_p11_info_get(sensor_type_t type);
u16 sensor_driver_p11_data_get(sensor_type_t type, u8 index, void *data_buff, u16 data_len);
void sensor_driver_p11_init(sensor_type_t type, u8 enable, u16 range, u8 odr);
void sensor_driver_p11_timer_modify(u16 msec);
u16 sensor_driver_p11_cbuf_mult_entry_enable(sensor_type_t type, u8 index, u8 enable);
void sensor_driver_p11_sleep(u8 type, u8 enable);