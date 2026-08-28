#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_photo_key_msg_table.data.bss")
#pragma data_seg(".video_photo_key_msg_table.data")
#pragma const_seg(".video_photo_key_msg_table.text.const")
#pragma code_seg(".video_photo_key_msg_table.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "system/includes.h"
#include "app_config.h"

#if TCFG_APP_PHOTO_EN

#if TCFG_IOKEY_ENABLE
//CLICK=0     LONG=1      HOLD=2       UP=3                DOUBLE=4              TRIPLE=5
const int key_video_photo_io_num0_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_VIDEO_PHOTO_BACK, APP_MSG_ZOOM_OUT, APP_MSG_NULL, APP_MSG_VIDEO_PHOTO_ZOOM_STOP, APP_MSG_NULL /*EFFECT_PREV 贴纸待测, 暂置空*/, APP_MSG_NULL,
};
const int key_video_photo_io_num1_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_VIDEO_PHOTO_CAPTURE, APP_MSG_NULL, APP_MSG_NULL, APP_MSG_NULL, APP_MSG_NULL, APP_MSG_NULL,
};
const int key_video_photo_io_num2_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL /*EFFECT_NEXT 贴纸待测, 暂置空*/, APP_MSG_ZOOM_IN, APP_MSG_NULL, APP_MSG_VIDEO_PHOTO_ZOOM_STOP, APP_MSG_NULL, APP_MSG_NULL,
};
#endif


#if TCFG_ADKEY_ENABLE
// ad_num0(220R): 左键, 返回/变焦减 (对齐 io_num0)
const int key_video_photo_ad_num0_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_VIDEO_PHOTO_BACK, APP_MSG_ZOOM_OUT, APP_MSG_NULL, APP_MSG_VIDEO_PHOTO_ZOOM_STOP, APP_MSG_NULL /*EFFECT_PREV 贴纸待测, 暂置空*/, APP_MSG_NULL,
};
// ad_num1(3.3K): 中键, 拍照 (对齐 io_num1)
const int key_video_photo_ad_num1_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_VIDEO_PHOTO_CAPTURE, APP_MSG_NULL, APP_MSG_NULL, APP_MSG_NULL, APP_MSG_NULL, APP_MSG_NULL,
};
// ad_num2(10K): 右键, 效果/变焦加 (对齐 io_num2)
const int key_video_photo_ad_num2_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL /*EFFECT_NEXT 贴纸待测, 暂置空*/, APP_MSG_ZOOM_IN, APP_MSG_NULL, APP_MSG_VIDEO_PHOTO_ZOOM_STOP, APP_MSG_NULL, APP_MSG_NULL,
};
#endif

const struct key_remap_table video_photo_mode_key_table[] = {
#if TCFG_IOKEY_ENABLE
    { .key_value = KEY_IO_NUM0, .remap_table = key_video_photo_io_num0_msg_table },
    { .key_value = KEY_IO_NUM1, .remap_table = key_video_photo_io_num1_msg_table },
    { .key_value = KEY_IO_NUM2, .remap_table = key_video_photo_io_num2_msg_table },
#endif
#if TCFG_ADKEY_ENABLE
    { .key_value = KEY_AD_NUM0, .remap_table = key_video_photo_ad_num0_msg_table },
    { .key_value = KEY_AD_NUM1, .remap_table = key_video_photo_ad_num1_msg_table },
    { .key_value = KEY_AD_NUM2, .remap_table = key_video_photo_ad_num2_msg_table },
#endif
    { .key_value = 0xff }
};
#endif /* #if TCFG_APP_PHOTO_EN */
