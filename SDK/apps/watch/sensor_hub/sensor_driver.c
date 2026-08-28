#include "board_config.h"
#include "sensor_driver.h"
#include "circular_buf.h"

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HUB_DRIVER]"
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

#define _IIC_USE_HW
#include "iic_api.h"
#if TCFG_SENSOR_HUB
const u8 sensor_iic_hdl   = 1;

u8 sensor_write(u8 iic_dev_addr, u8 register_address, const u8 *buf, u8 data_len)
{
    return system_iic_write_nbytes(sensor_iic_hdl, iic_dev_addr >> 1, &register_address, 1, (u8 *) buf, data_len) == data_len ? RET_OK : RET_ERR;
}

u8 sensor_read(u8 iic_dev_addr, u8 register_address, u8 *buf, u8 data_len, u8 ignore_len)
{
    return system_iic_read_nbytes(sensor_iic_hdl, iic_dev_addr >> 1, &register_address, 1, buf, data_len) == data_len ? RET_OK : RET_ERR;
}

SENSOR_INTERFACE *sensor_driver_find(sensor_type_t type)
{
    SENSOR_INTERFACE *sensor_driver = NULL;
    list_for_each_sensor(sensor_driver) {
        if (sensor_driver->info->type == type) {
            return sensor_driver;
        }
    }
    return NULL;
}


void sensor_info_printf(void)
{
    printf("\n\n******** sensor info ********\n");
    SENSOR_INTERFACE *driver = NULL;

    list_for_each_sensor(driver) {
        printf("\nname  : %s\n", driver->info->name);
        printf("type  : %d\n", driver->info->type);
        printf("state : %d\n", driver->info->state);
        printf("range : +-%d +-%d +-%d +-%d\n", driver->info->range[0], driver->info->range[1], driver->info->range[2], driver->info->range[3]);
        printf("odr   : %d %d %d %d\n", driver->info->odr[0], driver->info->odr[1], driver->info->odr[2], driver->info->odr[3]);
    }
    printf("\n****** sensor info end ******\n\n");
}

void sensor_driver_check(void)
{
    SENSOR_INTERFACE *sensor_driver = NULL;

    // int ret = system_iic_init(sensor_iic_hdl, get_iic_config(sensor_iic_hdl));
    // LOG("system_iic_init ret=%d",ret);
    // gpio_set_mode(IO_PORTB_00/16, BIT(IO_PORTB_00%16), PORT_OUTPUT_LOW);

    list_for_each_sensor(sensor_driver) {
        if ((sensor_driver->online != NULL) && (sensor_driver->online() == RET_OK)) {
            sensor_driver->info->state = SENSOR_STATE_ONLINE;
        } else {
            sensor_driver->info->state = SENSOR_STATE_OFFLINE;
        }
    }
    sensor_info_printf();
}


sensor_info_t *sensor_driver_info_get(sensor_type_t type)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    if (sensor_driver != NULL) {
        return sensor_driver->info;
    }
    return NULL;
}

void sensor_driver_init(u8 type, u8 enable, u16 range, u8 odr)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    log_debug("type=%d range=%d odr=%d driver=%x", type, range, odr, sensor_driver);

    if ((sensor_driver != NULL) && (sensor_driver->open != NULL)) {
        if (enable) {
            if (sensor_driver->open(range, odr) == RET_OK) {
                sensor_driver->info->state = SENSOR_STATE_OPEN;
                log_info("en=%d type %d sucess!", enable, type);
            } else {
                log_error("en=%d type %d failed!", enable, type);
            }
        } else {
            if (sensor_driver->close() == RET_OK) {
                sensor_driver->info->state = SENSOR_STATE_CLOSE;
                log_info("en=%d type %d sucess!", enable, type);
            } else {
                log_error("en=%d type %d failed!", enable, type);
            }
        }

    } else {
        log_error("no driver for sensor %d", type);
    }
}

u16 sensor_driver_data_get(sensor_type_t type, void *data_buff, u16 data_len)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    log_debug("type=%d driver=%x", type, sensor_driver);

    if ((sensor_driver != NULL) && (sensor_driver->run)) {
        sensor_driver->run(data_buff, &data_len);
    } else {
        data_len = 0;
    }
    log_debug("data_len=%d", data_len);
    return data_len;
}

#if TCFG_ACCELER_SLEEP_ENABLE
void sensor_driver_sleep(u8 type, u8 enable)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_find(type);
    if (sensor_driver->sleep) {
        log_debug("type=%d enable=%d", type, enable);
        sensor_driver->sleep();
    }
}
#endif
#endif
