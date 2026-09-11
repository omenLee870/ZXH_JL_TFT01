#pragma once

#include "app_config.h"
#include "includes.h"
#include "circular_buf.h"



#define RET_OK                 (0)
#define RET_ERR                ((u8)-1)

typedef enum {

    SENSOR_DRV_ACCELER      = 0x00,
    SENSOR_DRV_GYRO,
    SENSOR_DRV_MAGNETIC,
    SENSOR_DRV_HR,
    SENSOR_DRV_SPO2,
    SENSOR_DRV_HR_SPO2,
    SENSOR_DRV_WEAR_DETECTION,
    SENSOR_DRV_TEMP,
    SENSOR_DRV_MAX_NUM,

    SENSOR_ALGO_WRIST_TILT  = 0x20,
    SENSOR_ALGO_HR,
    SENSOR_ALGO_SPO2,
    SENSOR_ALGO_STEP_COUNTER,
} sensor_type_t;


typedef enum {
    SENSOR_STATE_OFFLINE  = 0,
    SENSOR_STATE_ONLINE   = 1,
    SENSOR_STATE_CLOSE    = 1,
    SENSOR_STATE_OPEN     = 2,
} sensor_state_t;

typedef struct {
    u8 addr;
    u8 value;
} sensor_regs_t;

typedef struct {
    u8   type;
    u8   state;
    u8   name[20];
    u8   odr[4];
    u16  range[4];
    cbuffer_t *cbuffer;
} sensor_info_t;


typedef struct {
    u8   type;
    u8   enable;
    u8   odr;
    u16  range;
} p11_sensor_t;

typedef struct {
    sensor_info_t *info;
    s8(*open)(u16  range, u16  odr);
    s8(*close)(void);
    s8(*run)(void *arg, u16 *len);
    s8(*online)(void);
    s8(*sleep)(void);
} SENSOR_INTERFACE;

extern SENSOR_INTERFACE  sensor_dev_begin[];
extern SENSOR_INTERFACE  sensor_dev_end[];

#define REGISTER_SENSOR(Sensor) \
	static const SENSOR_INTERFACE Sensor SEC_USED(.sensor_dev)

#define list_for_each_sensor(c) \
	for (c=sensor_dev_begin; c<sensor_dev_end; c++)


typedef struct {
    short x;
    short y;
    short z;
} axis_data_t;


int sensor_driver_bus_init(void);
u8 sensor_write(u8 w_chip_id, u8 register_address, const u8 *buf, u8 data_len);
u8 sensor_read(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len, u8 ignore_len);

void sensor_driver_check(void);
void sensor_driver_init(u8 type, u8 enable, u16 range, u8 odr);
sensor_info_t *sensor_driver_info_get(sensor_type_t type);
u16 sensor_driver_data_get(sensor_type_t type, void *data_buff, u16 data_len);
void sensor_driver_sleep(u8 type, u8 enable);
void sensor_irq_init(void);
__attribute__((weak)) s8 driver_sc7a20_reinit(void);
