#include "sensor_driver_p11.h"
#include "power/p11_app_msg.h"
#include "health_manager.h"
#include "asm/power/pmc/pmc_mmap.h"
#include "power/p11_cbuf.h"
#if(TCFG_SENSOR_HUB&&TCFG_SENSOR_HUB_P11)
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".p11_cbuf.data.bss")
#pragma data_seg(".p11_cbuf.data")
#pragma const_seg(".p11_cbuf.text.const")
#pragma code_seg(".p11_cbuf.text")
#endif


#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HUB_P11]"
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
static u32  p11_sensor_dev_begin = 0;
static u32  p11_sensor_dev_end   = 0;


SENSOR_INTERFACE *sensor_driver_p11_find(sensor_type_t type)
{
    sensor_info_t *sensor_info = NULL;
    SENSOR_INTERFACE *sensor_driver = NULL;
    for (sensor_driver = (SENSOR_INTERFACE *) p11_sensor_dev_begin; (u32)sensor_driver < p11_sensor_dev_end; sensor_driver++) {
        sensor_info = (sensor_info_t *)((u32)sensor_driver->info + P11_RAM_BASE);
        // log_debug("type=%d sensor_driver=%x type=%d sensor_info=%x %x", type, sensor_driver, sensor_info->type, sensor_info, sensor_driver->info);
        if (sensor_info->type == type) {
            // log_info("type=%d sensor_driver=%x", type, sensor_driver);
            return sensor_driver;
        }
    }
    log_error("type=%d sensor_driver is null", type);
    return NULL;
}

sensor_info_t *sensor_driver_p11_info_get(sensor_type_t type)
{
    SENSOR_INTERFACE *sensor_driver = sensor_driver_p11_find(type);
    if (sensor_driver != NULL) {
        return (sensor_info_t *)((u32)(sensor_driver->info) + P11_RAM_BASE);
    }
    return NULL;
}

u16 sensor_driver_p11_cbuf_mult_entry_enable(sensor_type_t type, u8 index, u8 enable)
{
    sensor_info_t *sensor_driver = sensor_driver_p11_info_get(type);
    if (sensor_driver == NULL) {
        return 0;
    }
    log_debug("type=%d cbuf_index=%x enable:%d", type, index, enable);
    p11cbuf_mult_entry_enable((p11_cbuffer_t *)sensor_driver->cbuffer, index, enable);
    return 0;
}

u16 sensor_driver_p11_data_get(sensor_type_t type, u8 index, void *data_buff, u16 data_len)
{
    sensor_info_t *sensor_driver = sensor_driver_p11_info_get(type);

    if (sensor_driver == NULL) {
        return 0;
    }

    int buff_len = p11_cbuf_mult_read_get_data_len((p11_cbuffer_t *)sensor_driver->cbuffer, index);
    log_debug("type=%d cbuf_index=%x len=%d", type, index, buff_len);
    if ((buff_len > 0) && (data_len > 0)) {
        if (buff_len < data_len) {
            data_len = buff_len;
        }
        data_len = p11_cbuf_mult_read_alloc_len((p11_cbuffer_t *)sensor_driver->cbuffer,  index, data_buff, data_len);
        if (data_len) {
            p11_cbuf_mult_read_alloc_len_updata((p11_cbuffer_t *)sensor_driver->cbuffer, index, data_len);
        }
    } else {
        data_len = 0;
    }
    return data_len;
}


void sensor_driver_p11_init(sensor_type_t type, u8 enable, u16 range, u8 odr)
{

    log_debug("type=%d range=%d odr=%d", type, range, odr);

    u8 msg[6];
    msg[0] = MSG_P11_SENSOR_INIT;
    msg[1] = type;
    msg[2] = enable;
    msg[3] = range >> 8;
    msg[4] = range & 0xFF;
    msg[5] = odr;
    m2p_post_msg(MSG_APP, 0, (u8 *)msg, sizeof(msg));
}


void sensor_driver_p11_timer_modify(u16 msec)
{
    u8 msg[3];
    msg[0] = MSG_P11_SENSOR_TIMER;
    msg[1] = msec >> 8;
    msg[2] = msec & 0xFF;
    log_debug("sensor_driver_p11_timer_modify: %d", msec);
    m2p_post_msg(MSG_APP, 0, (u8 *)msg, sizeof(msg));
}

void sensor_driver_p11_sleep(u8 type, u8 enable)
{
    u8 msg[3];
    msg[0] = MSG_P11_SENSOR_SLEEP;
    msg[1] = type;
    msg[2] = enable;
    printf("sensor_sleep: type=%d\n", type);
    m2p_post_msg(MSG_APP, 0, (u8 *)msg, sizeof(msg));
}

static void p11_msg_handler(void *priv, u8 *buf, u32 len)
{
    int msg[16];
    ASSERT(len <= sizeof(msg));
    memcpy(msg, buf, len);
    log_debug("p11_msg_handler: %d, len: %d\n", msg[0], len);

    switch (msg[0]) {
    case   MSG_P11_SENSOR_INFO:
        p11_sensor_dev_begin = msg[1] + P11_RAM_BASE;
        p11_sensor_dev_end   = msg[2] + P11_RAM_BASE;
        log_debug("p11_sensor_dev_begin=%x end=%x", p11_sensor_dev_begin, p11_sensor_dev_end);
        break;

    case   MSG_P11_SENSOR_EVENT:
        break;

    case   MSG_P11_ALGORITHM_EVENT:
        sport_health_manager_msg_post(SHM_MOD_DET_WRIST, SHM_CMD_UPDATE, (void *)msg[1], 0);
        break;
    }
}

REGISTER_P2M_MSG_HANDLER(NULL, MSG_APP, p11_msg_handler);

#endif //TCFG_SENSOR_HUB_P11
