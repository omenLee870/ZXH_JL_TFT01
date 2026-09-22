#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sport_info_mense.data.bss")
#pragma data_seg(".sport_info_mense.data")
#pragma const_seg(".sport_info_mense.text.const")
#pragma code_seg(".sport_info_mense.text")
#endif
#include "app_config.h"
#include "rcsp_config.h"
#include "sport_info_opt.h"
#include "rcsp_event.h"
#include "rcsp_manage.h"
#include "sport_info_mense.h"
#include "health_manager/shm_info_storage.h"
#include "mense_manage/mense_manage.h"

#define LOG_TAG_CONST       RCSP_ADAPTOR
#define LOG_TAG     		"[RCSP-SPORT-DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

/* 女性健康(经期)数据仅手表产品(SUB_CASE_TYPE_WATCH)实现(经期管理在
 * mense_manage / watch_320x385 ui_action_mense.c), 儿童相机等子项目不编译,
 * 避免引用未链接的 mense_personal_info_set/get。 */
#if JL_RCSP_SENSORS_DATA_OPT && (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH)

void sport_info_mense_attr_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    log_info("<%s>", __func__);
    sport_info_mense_t mense = {0};
    memcpy(&mense, data, sizeof(sport_info_mense_t));
    // u16 字段网络字节序(大端) -> 主机序，与 personal_info 的 height/weight 一致
    mense.start_year = ((u8 *)&mense.start_year)[0] << 8 | ((u8 *)&mense.start_year)[1];

    PERSONAL_MENSE info = {0};
    info.mense_period_days = mense.mense_period_days;
    info.physiological_days = mense.physiological_days;
    info.menstruation_sday.year = mense.start_year;
    info.menstruation_sday.month = mense.start_month;
    info.menstruation_sday.day = mense.start_day;
    info.resp.enable = mense.resp_enable ? 1 : 0;
    info.resp.mense_prev_day = mense.mense_prev_day;
    info.resp.ovulation_prev_day = mense.ovulation_prev_day;
    info.resp.easy_prev_day = mense.easy_prev_day;
    info.resp.resp_hour = mense.resp_hour;
    info.resp.resp_min = mense.resp_min;

    // 写回 mense 后端：更新内存 + 重算周期 + 延时落盘(VM)
    mense_personal_info_set(&info);
}

u32 sport_info_mense_attr_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    log_info("<%s>", __func__);
    u32 rlen = 0;
    sport_info_mense_t mense = {0};

    PERSONAL_MENSE info = {0};
    mense_personal_info_get(&info);

    mense.mense_period_days = info.mense_period_days;
    mense.physiological_days = info.physiological_days;
    mense.start_year = info.menstruation_sday.year;
    mense.start_month = info.menstruation_sday.month;
    mense.start_day = info.menstruation_sday.day;
    mense.resp_enable = info.resp.enable;
    mense.mense_prev_day = info.resp.mense_prev_day;
    mense.ovulation_prev_day = info.resp.ovulation_prev_day;
    mense.easy_prev_day = info.resp.easy_prev_day;
    mense.resp_hour = info.resp.resp_hour;
    mense.resp_min = info.resp.resp_min;
    // u16 字段主机序 -> 网络字节序(大端)
    mense.start_year = ((u8 *)&mense.start_year)[0] << 8 | ((u8 *)&mense.start_year)[1];

    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&mense, sizeof(sport_info_mense_t));

    return rlen;
}

#endif /* if JL_RCSP_SENSORS_DATA_OPT */
