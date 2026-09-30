#include "sensor_driver.h"
#include "includes.h"
#include "usr_timer.h"
#include "iic_api.h"
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
#define LOG_TAG_CONST      	SENSOR_DRIVER
#define LOG_TAG     		"[SENSOR_DRIVER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_WARN_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"

#if CONFIG_SENSOR_DRIVER_ENABLE


#define IIC_HW_ENABLE 			TCFG_SENSOR_IIC_HW_EN

/* FIFO 取数定时器周期，可在 sdk_config.h 中调整（u8 上限 255ms） */
static u8 sensor_timer_msec = CONFIG_SENSOR_DRIVER_TIMER_PERIOD;
AT_SENSOR_FIFO u8 sensor_write(u8 iic_dev_addr, u8 register_address,  u8 *buf, u8 data_len)
{
    int ret = 0;
#if IIC_HW_ENABLE
    ret = hw_i2c_master_write_nbytes_to_device_reg(HW_IIC_0, iic_dev_addr, &register_address, 1, buf, data_len);
#else
    ret = soft_i2c_master_write_nbytes_to_device_reg(HW_IIC_0, iic_dev_addr, &register_address, 1, buf, data_len);
#endif
    log_debug("%s dev:0x%x reg:0x%x buf:0x%x len:%d ret:%d", __func__, iic_dev_addr, register_address, (u32)buf, data_len, ret);
    return ret == data_len ? RET_OK : RET_ERR;
}

AT_SENSOR_FIFO u8 sensor_read(u8 iic_dev_addr, u8 register_address, u8 *buf, u8 data_len, u8 ignore_len)
{
    int ret = 0;
#if IIC_HW_ENABLE
    ret = hw_i2c_master_read_nbytes_from_device_reg(HW_IIC_0, iic_dev_addr, &register_address, 1, buf, data_len);
#else
    ret = soft_i2c_master_read_nbytes_from_device_reg(HW_IIC_0, iic_dev_addr, &register_address, 1, buf, data_len);
#endif
    log_debug("%s dev:0x%x reg:0x%x buf:0x%x len:%d ret:%d", __func__, iic_dev_addr, register_address, (u32)buf, data_len, ret);
    return ret == data_len ? RET_OK : RET_ERR;
}

AT_SENSOR_FIFO SENSOR_INTERFACE *sensor_driver_find(sensor_type_t type)
{
    SENSOR_INTERFACE *sensor_driver = NULL;
    list_for_each_sensor(sensor_driver) {
        if (sensor_driver->info->type == type) {
            return sensor_driver;
        }
    }
    return NULL;
}


AT_SENSOR_COLD void sensor_info_printf(void)
{
    SENSOR_INTERFACE *driver = NULL;

    log_debug("******** sensor info ********");
    list_for_each_sensor(driver) {
        log_debug("name  : %s", driver->info->name);
        log_debug("type  : %d", driver->info->type);
        log_debug("state : %d", driver->info->state);
        log_debug("range : +-%d +-%d +-%d +-%d", driver->info->range[0], driver->info->range[1], driver->info->range[2], driver->info->range[3]);
        log_debug("odr   : %d %d %d %d", driver->info->odr[0], driver->info->odr[1], driver->info->odr[2], driver->info->odr[3]);
    }
    log_debug("****** sensor info end ******");
}

AT_SENSOR_COLD void sensor_driver_check(void)
{
    SENSOR_INTERFACE *sensor_driver = NULL;
    list_for_each_sensor(sensor_driver) {
        log_debug("[sensor_drv] check name=%s type=%d ...", sensor_driver->info->name, sensor_driver->info->type);
        if ((sensor_driver->online != NULL) && (sensor_driver->online() == RET_OK)) {
            sensor_driver->info->state = SENSOR_STATE_ONLINE;
        } else {
            sensor_driver->info->state = SENSOR_STATE_OFFLINE;
        }
        log_debug("[sensor_drv] check name=%s state=%d", sensor_driver->info->name, sensor_driver->info->state);
    }
    //发送sensor_drvier_info给大核
    int msg[3];
    msg[0] = MSG_P11_SENSOR_INFO;
    msg[1] = (int)&sensor_dev_begin;
    msg[2] = (int)&sensor_dev_end;
    int ret = p2m_post_msg(MSG_APP, 0, (u8 *)msg, sizeof(msg));
    log_info("%s begin=%x end=%x ret=%d ", __func__, (u32)sensor_dev_begin, (u32)sensor_dev_end, ret);
    sensor_info_printf();
}

AT_SENSOR_FIFO static void sensor_timer_irq_handle(void *priv)
{
    task_post_msg(NULL, 1, MSG_P11_SYS_KICK);
}


AT_SENSOR_FIFO void sensor_timer_enable(u8 enable)
{
    static u16 timer_handle = 0;

    if (enable && timer_handle == 0) {
        timer_handle = usr_timer_add(NULL, sensor_timer_irq_handle, sensor_timer_msec, 1);
        if (timer_handle == 0) {
            log_error("%s usr_timer_add failed: pool full (msec=%d)", __func__, sensor_timer_msec);
        }
        log_debug("%s handle=%d msec=%d", __func__, timer_handle, sensor_timer_msec);
    } else if ((!enable) && (timer_handle > 0)) {
        usr_timer_del(timer_handle);
        timer_handle = 0;
        log_debug("%s handle=%d", __func__, timer_handle);
    }
}


AT_SENSOR_FIFO void sensor_timer_modify(u32 msec)
{
    sensor_timer_msec = msec;
    sensor_timer_enable(0);
    sensor_timer_enable(1);
}

AT_SENSOR_FIFO void sensor_driver_sleep(u8 type, u8 enable)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    if (sensor_driver->sleep) {
        log_debug("%s type=%d enable=%d", __func__, type, enable);
        sensor_driver->sleep();
        sensor_timer_enable(!enable);
    }
}

AT_SENSOR_FIFO void sensor_driver_irq_handle(void)
{
    __attribute__((weak)) s8 driver_sc7a20_is_wake(void);
    if (driver_sc7a20_is_wake) {
        if (driver_sc7a20_is_wake() == RET_OK) {
            sensor_timer_enable(1);
        }
    }
}
AT_SENSOR_COLD void sensor_driver_cbuf_enable(u8 type, u8 index, u8 enable)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    p11_cbuffer_t *cbuffer = sensor_driver->info->cbuffer;
    log_debug("%s type:%d index:%d en:%d cbuf:0x%x", __func__, type, index, enable, (u32)cbuffer);
    cbuf_mult_entry_enable(cbuffer, index, enable);
}
AT_SENSOR_COLD void sensor_driver_init(u8 type, u8 enable, u16 range, u8 odr)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    log_debug("%s type=%d enable=%d range=%d odr=%d driver=%x open:0x%x", __func__, type, enable, range, odr, (u32)sensor_driver, (u32)sensor_driver->open);
    if ((sensor_driver != NULL) && (sensor_driver->open != NULL)) {
        if (enable) {
            if (sensor_driver->open(range, odr) == RET_OK) {
                sensor_driver->info->state = SENSOR_STATE_OPEN;
                sensor_timer_enable(1);
                log_info("type %d sucess!", type);
            } else {
                log_error("type %d failed!", type);
            }
        } else {
            if (sensor_driver->close() == RET_OK) {
                sensor_driver->info->state = SENSOR_STATE_CLOSE;
                log_info("type %d sucess!", type);
            } else {
                log_error("type %d failed!", type);
            }
        }

    } else {
        log_error("no driver for sensor %d", type);
    }
}


AT_SENSOR_FIFO void sensor_driver_run(void)
{
    u16 data[30 * 3];
    u16 size = sizeof(data);

    SENSOR_INTERFACE *sensor_driver     = NULL;
    SENSOR_INTERFACE *acceler_algorithm = NULL;

    acceler_algorithm = sensor_driver_find(SENSOR_ALGO_WRIST_TILT);
    list_for_each_sensor(sensor_driver) {
        if ((sensor_driver->info->type < SENSOR_DRV_MAX_NUM) &&
            (sensor_driver->info->state == SENSOR_STATE_OPEN) && (sensor_driver->run)) {
            sensor_driver->run(data, &size);

            if ((sensor_driver->info->type == SENSOR_DRV_ACCELER) &&
                (acceler_algorithm != NULL) && (acceler_algorithm->run)) {
                acceler_algorithm->run(data, &size);
            }
        }
    }
}
AT_SENSOR_FIFO sensor_info_t *sensor_driver_info_get(sensor_type_t type)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    if (sensor_driver != NULL) {
        return sensor_driver->info;
    }
    return NULL;
}
AT_SENSOR_FIFO u16 sensor_driver_data_get(sensor_type_t type, void *data_buff, u16 data_len)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    log_debug("type=%d driver=%x", type, (u32)sensor_driver);

    if ((sensor_driver != NULL) && (sensor_driver->run)) {
        sensor_driver->run(data_buff, &data_len);
    } else {
        data_len = 0;
    }
    log_debug("data_len=%d", data_len);
    return data_len;
}

#endif
