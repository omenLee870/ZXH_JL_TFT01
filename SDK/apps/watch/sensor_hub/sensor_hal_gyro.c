#include "sensor_hub.h"
#include "sensor_driver_p11.h"
#include "syscfg_id.h"

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HAL_GYRO]"
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
#if TCFG_GYRO_P11_ENABLE | TCFG_GYRO_MASTER_ENABLE

#define GYRO_CBUF_BYTE    (1024)

static sensor_calibrate_t     calibrate = {0};
static sensor_hal_t           gyroscope = {0};

static s8 gyro_init(void)
{
    gyroscope.odr   = 25;
    gyroscope.range = 250;

    if (gyroscope.cbuffer == NULL) {
        gyroscope.cbuffer   = zalloc(sizeof(cbuffer_t));
        short   *data_buf = zalloc(GYRO_CBUF_BYTE);
        if (data_buf == NULL || gyroscope.cbuffer == NULL) {
            log_error("cbuf_error!");
            return RET_FAILED;
        }
        cbuf_init(gyroscope.cbuffer, data_buf, GYRO_CBUF_BYTE);
    }

    int ret = syscfg_read(VM_GYRO_CALIBRATE_DATA, &calibrate, sizeof(sensor_calibrate_t));
    log_debug("calibrate:%d,%d,%d,%d,%d", ret, calibrate.state, calibrate.x, calibrate.y, calibrate.z);
    return RET_SUCCESS;
}

static s8 gyro_info_get(sensor_drv_info *sensor_info)
{
    sensor_info_t *info = NULL;
#if TCFG_GYRO_P33_ENABLE
    info = sensor_driver_info_get(SENSOR_HW_GYRO);
#endif
#if TCFG_GYRO_P11_ENABLE
    info = sensor_driver_info_p11_get(SENSOR_HW_GYRO);
#endif
    if (info != NULL) {
        memcpy(sensor_info, info, sizeof(sensor_drv_info));
        return RET_SUCCESS;
    }
    return RET_FAILED;
}

static s8 gyro_enable(u8 enable)
{
    gyroscope.enable = enable;
#if TCFG_GYRO_P33_ENABLE
    sensor_driver_open(SENSOR_HW_GYRO, enable, gyroscope.range, gyroscope.odr);
#endif
#if TCFG_GYRO_P11_ENABLE
    sensor_driver_p11_open(SENSOR_HW_GYRO, enable, gyroscope.range, gyroscope.odr);
#endif
    return RET_SUCCESS;
}

static s8 gyro_set_param(u8 odr, u32 time_out)
{
    gyroscope.odr = odr;
    gyroscope.timeout_ms = time_out;
    return RET_SUCCESS;
}


static u16 gyro_raw_data_get(u8 index, void *data_buff, u16 data_len)
{
#if TCFG_GYRO_P33_ENABLE
    return sensor_driver_data_get(SENSOR_HW_GYRO, data_buff, data_len);
#endif
#if TCFG_GYRO_P11_ENABLE
    return sensor_driver_p11_data_get(SENSOR_HW_GYRO, data_buff, data_len);
#endif
}

static s8 gyro_calibrae_start(void)
{
    calibrate.state = CALIBRATE_GOING;
    return RET_OK;
}

static s8 gyro_calibrae(axis_data_t *gyro, u16 point)
{
    if (calibrate.state == CALIBRATE_GOING) {
        if (gyro->x > -10 && gyro->x < 10 && gyro->y > -10 && gyro->y < 10 && gyro->x > -10 && gyro->x < 10) {
            calibrate.x = -(gyro->x);
            calibrate.y = -(gyro->y);
            calibrate.z = -(gyro->z);
            calibrate.state = CALIBRATE_SUCCESS;

            int ret = syscfg_write(VM_GYRO_CALIBRATE_DATA, &calibrate, sizeof(sensor_calibrate_data_t));
            log_debug("calibrate:%d,%d,%d,%d,%d", ret, calibrate.state, calibrate.x, calibrate.y, calibrate.z);
        } else {
            calibrate.state = CALIBRATE_FAILED;
        }

        {
            struct sensor_event sensor_update = {0};
            short cal_x = gyro->x + calibrate.x;
            short cal_y = gyro->y + calibrate.y;
            short cal_z = gyro->z + calibrate.z;
            short cali_status =  calibrate.state == CALIBRATE_SUCCESS ? 0 : -1;

            sensor_update.sensor_handle = GYROSCOPE;
            sensor_update.timestamp     = gyroscope.time_tamp;
            sensor_update.data[0] = GYRO_FLOAT_CONVER(cal_x);
            sensor_update.data[1] = GYRO_FLOAT_CONVER(cal_y);
            sensor_update.data[2] = GYRO_FLOAT_CONVER(cal_z);

            send_cali_data(cali_status, &sensor_update);
        }
    }
    return 0;

}


static u16 gyro_calibration_data_get(void *data_buff, u16 data_len)
{
    axis_data_t *gyro = data_buff;
    u16 gyro_len = gyro_raw_data_get(data_buff, data_len);
    gyro_calibrae(gyro, gyro_len / 6);

    if (calibrate.state == CALIBRATE_SUCCESS) {
        for (u16 i = 0; i < gyro_len / 6; i++) {
            gyro[i].x +=  calibrate.x;
            gyro[i].y +=  calibrate.y;
            gyro[i].z +=  calibrate.z;
            log_debug("gyro[%d]:%d,%d,%d", i, gyro[i].x, gyro[i].y, gyro[i].z);
        }
    }
    return gyro_len;
}

static s8 gyro_clear_cbuf(void)
{
    if (gyroscope.cbuffer == NULL) {
        return RET_FAILED;
    }
    cbuf_clear(gyroscope.cbuffer);

    struct sensor_event sensor_event_data;
    sensor_event_data.sensor_handle = 0;
    sensor_event_data.timestamp     = gyroscope.time_tamp;
    sensor_event_data.event.what    = 1;
    sensor_event_data.event.sensor  = GYROSCOPE;
    send_flush_data(&sensor_event_data);
    return RET_SUCCESS;
}


static void gyro_data_send(void)
{
    // LOG("low_power_state=%d wake=%d",low_power_state,gyroscope.wake);
    if (low_power_state == 1 && gyroscope.wake == 0) {
        return;
    }
    u32 gyro_len = cbuf_get_data_size(gyroscope.cbuffer);
    // LOG("gyro_len=%d",gyro_len);
    if (gyro_len == 0) {
        log_error("gsensor_cbuf_empty");
        return;
    }

    axis_data_t *data_buff =  zalloc(gyro_len);
    if (data_buff == 0) {
        log_error("malloc faile");
        return;
    }

    cbuf_read(gyroscope.cbuffer, data_buff, gyro_len);

    struct sensor_event sensor_update = {0};
    sensor_update.sensor_handle = GYROSCOPE;
    sensor_update.timestamp     = gyroscope.time_tamp;
    for (u16 i = 0; i < gyro_len / 6; i++) {
        sensor_update.data[0] = GYRO_FLOAT_CONVER(data_buff[i].x);
        sensor_update.data[1] = GYRO_FLOAT_CONVER(data_buff[i].y);
        sensor_update.data[2] = GYRO_FLOAT_CONVER(data_buff[i].z);
        log_debug("[%d]:%f,%f,%f", i, sensor_update.data[0], sensor_update.data[1], sensor_update.data[2]);
        send_sensor_event(&sensor_update);
    }
    free(data_buff);
}



static void gyro_data_process(void)
{
    if (gyroscope.enable) {
        axis_data_t data_buff[50];
        u16 gyro_len = gyro_calibration_data_get(data_buff, sizeof(data_buff));

        if (gyroscope.cbuffer && gyro_len) {
            u32 wlen = cbuf_write(gyroscope.cbuffer, data_buff, gyro_len);
            if (wlen == 0) {
                log_error("cbuf_full");
            }

            u64 cur_time = jiffies_msec();
            u64 diff_time = cur_time - gyroscope.time_tamp;
            if (diff_time >= gyroscope.timeout_ms) {
                gyroscope.time_tamp = cur_time;
                gyro_data_send();
            }
        }
    }

    REGISTER_SENSOR_HAL(gyro_hal) = {
        .handle    = GYROSCOPE,
        .init      = gyro_init,
        .enable    = gyro_enable,
        .calibrate = gyro_calibrae_start,
        .flush     = gyro_clear_cbuf,
        .get_info  = gyro_info_get,
        .set_param = gyro_set_param,
        .run       = gyro_data_process,
    };

#endif
#endif
