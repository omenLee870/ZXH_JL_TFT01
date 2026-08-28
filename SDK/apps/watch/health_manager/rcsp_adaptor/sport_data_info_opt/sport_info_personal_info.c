#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sport_info_personal_info.data.bss")
#pragma data_seg(".sport_info_personal_info.data")
#pragma const_seg(".sport_info_personal_info.text.const")
#pragma code_seg(".sport_info_personal_info.text")
#endif
#include "rcsp_config.h"
#include "sport_info_opt.h"
#include "rcsp_event.h"
#include "rcsp_manage.h"
#include "sport_info_personal_info.h"
#include "health_manager/health_manager.h"
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

void sport_info_personal_info_attr_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    log_info("<%s>", __func__);
    personal_information info = {0};
    memcpy(&info, data, sizeof(personal_information));
    info.height = ((u8 *)&info.height)[0] << 8 | ((u8 *)&info.height)[1];
    info.weight = ((u8 *)&info.weight)[0] << 8 | ((u8 *)&info.weight)[1];
    info.birth_y = ((u8 *)&info.birth_y)[0] << 8 | ((u8 *)&info.birth_y)[1];
    //更新到vm
    sport_info_write_vm(VM_SPORT_INFO_PERSONAL_INFO_FLAG, (u8 *)&info, sizeof(personal_information));
    //通知算法更新参数
    sport_health_gsensor_algo_cfg_update();
}

u32 sport_info_personal_info_attr_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    log_info("<%s>", __func__);
    u32 rlen = 0;
    personal_information info = {0};
    if (sport_personal_info_get(&info)) {
        info.height = ((u8 *)&info.height)[0] << 8 | ((u8 *)&info.height)[1];
        info.weight = ((u8 *)&info.weight)[0] << 8 | ((u8 *)&info.weight)[1];
        info.birth_y = ((u8 *)&info.birth_y)[0] << 8 | ((u8 *)&info.birth_y)[1];
    }
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&info, sizeof(personal_information));
    return rlen;
}

#endif /* if JL_RCSP_SENSORS_DATA_OPT */

