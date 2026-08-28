#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tone_table.data.bss")
#pragma data_seg(".tone_table.data")
#pragma const_seg(".tone_table.text.const")
#pragma code_seg(".tone_table.text")
#endif
#include "app_config.h"
#include "app_tone.h"
#include "fs/resfile.h"

static const struct tone_files chinese_tone_files = {
    .num = {
        "tone_zh/0.*",
        "tone_zh/1.*",
        "tone_zh/2.*",
        "tone_zh/3.*",
        "tone_zh/4.*",
        "tone_zh/5.*",
        "tone_zh/6.*",
        "tone_zh/7.*",
        "tone_zh/8.*",
        "tone_zh/9.*",
    },
    .power_on           = "tone_zh/power_on.*",
    .power_off          = "tone_zh/power_off.*",
    .bt_mode            = "tone_zh/bt.*",
    .pc_mode            = "tone_zh/pc.*",
    .bt_connect         = "tone_zh/bt_conn.*",
    .bt_disconnect      = "tone_zh/bt_dconn.*",
    .phone_in           = "tone_zh/ring.*",
    .phone_out          = "tone_zh/ring.*",
    .low_power          = "tone_zh/low_power.*",
    .max_vol            = "tone_zh/vol_max.*",
    .normal             = "tone_zh/normal.*",
    .key_tone           = "tone_zh/key_tone.*",
    .ring_volume        = "tone_zh/volume.*",
    .bt_discon      = {
        "tone_zh/bt_disc.*",
        "tone_zh/bt_con_fail.*",
        "tone_zh/disc_all_bt.*",
    },
    .bt_scan            = "tone_zh/bt_scan.*",
    .bt_conn         	= "tone_zh/bt_link.*",
    .factory_reset      = "tone_zh/factory_res.*",
    .reset_complete     = "tone_zh/reset_succ.*",
    .ota_start          = "tone_zh/ota_start.*",
    .ota_reminder       = "tone_zh/ota_notice.*",
    .low_battery        = "tone_zh/low_bat.*",
    .auto_power_off     = "tone_zh/auto_shutdo.*",
    .charging_on        = "tone_zh/charge_on.*",
    .charging_off       = "tone_zh/charge_off.*",
    .touch_tone         = "tone_zh/touch.*",
    .camera_shutter     = "tone_zh/shutter.*",
    .video_recording    = "tone_zh/video_rec.*",
    .battery_status     = "tone_zh/glass_bat.*",
    .battery_lvl        = {
        "tone_zh/level1.*",
        "tone_zh/level5.*",
        "tone_zh/level10.*",
        "tone_zh/level15.*",
        "tone_zh/level20.*",
        "tone_zh/level30.*",
        "tone_zh/level40.*",
        "tone_zh/level50.*",
        "tone_zh/level60.*",
        "tone_zh/level70.*",
        "tone_zh/level80.*",
        "tone_zh/level90.*",
        "tone_zh/level100.*",
    },
};

static const struct tone_files english_tone_files = {
    .num = {
        "tone_en/0.*",
        "tone_en/1.*",
        "tone_en/2.*",
        "tone_en/3.*",
        "tone_en/4.*",
        "tone_en/5.*",
        "tone_en/6.*",
        "tone_en/7.*",
        "tone_en/8.*",
        "tone_en/9.*",
    },
    .power_on           = "tone_en/power_on.*",
    .power_off          = "tone_en/power_off.*",
    .bt_mode            = "tone_en/bt.*",
    .pc_mode            = "tone_en/pc.*",
    .bt_connect         = "tone_en/bt_conn.*",
    .bt_disconnect      = "tone_en/bt_dconn.*",
    .phone_in           = "tone_en/ring.*",
    .phone_out          = "tone_en/ring.*",
    .low_power          = "tone_en/low_power.*",
    .max_vol            = "tone_en/vol_max.*",
    .normal             = "tone_en/normal.*",
    .key_tone           = "tone_en/key_tone.*",
    .ring_volume        = "tone_en/volume.*",
    .bt_discon      = {
        "tone_en/bt_disc.*",
        "tone_en/bt_con_fail.*",
        "tone_en/disc_all_bt.*",
    },
    .bt_scan            = "tone_en/bt_scan.*",
    .bt_conn         	= "tone_en/bt_link.*",
    .factory_reset      = "tone_en/factory_res.*",
    .reset_complete     = "tone_en/reset_succ.*",
    .ota_start          = "tone_en/ota_start.*",
    .ota_reminder       = "tone_en/ota_notice.*",
    .low_battery        = "tone_en/low_bat.*",
    .auto_power_off     = "tone_en/auto_shutdo.*",
    .charging_on        = "tone_en/charge_on.*",
    .charging_off       = "tone_en/charge_off.*",
    .touch_tone         = "tone_en/touch.*",
    .camera_shutter     = "tone_en/shutter.*",
    .video_recording    = "tone_en/video_rec.*",
    .battery_status     = "tone_en/glass_bat.*",
    .battery_lvl        = {
        "tone_en/level1.*",
        "tone_en/level5.*",
        "tone_en/level10.*",
        "tone_en/level15.*",
        "tone_en/level20.*",
        "tone_en/level30.*",
        "tone_en/level40.*",
        "tone_en/level50.*",
        "tone_en/level60.*",
        "tone_en/level70.*",
        "tone_en/level80.*",
        "tone_en/level90.*",
        "tone_en/level100.*",
    },
};

#if TCFG_TONE_EN_ENABLE
static enum tone_language g_lang_used = TONE_ENGLISH;
#else
static enum tone_language g_lang_used = TONE_CHINESE;
#endif

enum tone_language tone_language_get()
{
    return g_lang_used;
}

void tone_language_set(enum tone_language lang)
{
    g_lang_used = lang;
}

const struct tone_files *get_tone_files()
{
#if TCFG_TONE_ZH_ENABLE
    if (g_lang_used == TONE_CHINESE) {
        return &chinese_tone_files;
    }
#endif

    return &english_tone_files;
}



