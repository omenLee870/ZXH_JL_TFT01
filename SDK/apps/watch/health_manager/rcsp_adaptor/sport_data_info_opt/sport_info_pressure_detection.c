#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sport_info_pressure_detection.data.bss")
#pragma data_seg(".sport_info_pressure_detection.data")
#pragma const_seg(".sport_info_pressure_detection.text.const")
#pragma code_seg(".sport_info_pressure_detection.text")
#endif
#include "rcsp_config.h"
#include "sport_info_opt.h"
#include "rcsp_event.h"
#include "rcsp_manage.h"
#include "sport_info_pressure_detection.h"
#include "health_manager/shm_info_storage.h"

#define LOG_TAG_CONST       RCSP_ADAPTOR
#define LOG_TAG     		"[RCSP-SPORT-DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if JL_RCSP_SENSORS_DATA_OPT

void sport_info_pressure_detection_attr_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    log_info("<%s>", __func__);
    u8 press_switch = data[0];
    u8 press_mode = data[1];
    sport_info_switch_record_update(SPORT_INFO_SWTICH_TYPE_PRESSURE_DETECTION, press_switch, 1);
    sport_info_mode_record_update(SPORT_INFO_MODE_TYPE_PRESSURE_DETECTION, press_mode);
}

u32 sport_info_pressure_detection_attr_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    log_info("<%s>", __func__);
    u32 rlen = 0;

    u8 press_data[2] = {0};
    press_data[0] = !!sport_info_swtich_record_get(SPORT_INFO_SWTICH_TYPE_PRESSURE_DETECTION);

    u8 *mode_data = NULL;
    u16 mode_len = sport_info_record_get(SPORT_INFO_MODE_TYPE_PRESSURE_DETECTION, &mode_data);
    if (mode_data && mode_len) {
        press_data[1] = mode_data[0];
    }
    rlen = add_one_attr(buf, buf_size, offset, attr, press_data, sizeof(press_data));

    return rlen;
}

#endif /* if JL_RCSP_SENSORS_DATA_OPT */

