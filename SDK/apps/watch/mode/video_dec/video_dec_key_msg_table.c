#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_dec_key_msg_table.data.bss")
#pragma data_seg(".video_dec_key_msg_table.data")
#pragma const_seg(".video_dec_key_msg_table.text.const")
#pragma code_seg(".video_dec_key_msg_table.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "system/includes.h"
#include "app_config.h"

#if TCFG_APP_VIDEO_DEC_EN

#if TCFG_IOKEY_ENABLE
//CLICK=0     LONG=1      HOLD=2       UP=3                DOUBLE=4              TRIPLE=5
const int key_video_dec_io_num0_msg_table[KEY_ACTION_MAX] = {
    /* CLICK */ APP_MSG_VIDEO_DEC_BACK,
    /* LONG  */ APP_MSG_NULL,//APP_MSG_CHANGE_MODE,
    /* HOLD  */ APP_MSG_NULL,
    /* UP    */ APP_MSG_NULL,
    /* DOUBLE*/ APP_MSG_NULL,
    /* TRIPLE*/ APP_MSG_NULL,
};
const int key_video_dec_io_num1_msg_table[KEY_ACTION_MAX] = {
    /* CLICK */ APP_MSG_VIDEO_DEC_PLAY,
    /* LONG  */ APP_MSG_VIDEO_DEC_SHOW_DEL_MENU,
    /* HOLD  */ APP_MSG_NULL,
    /* UP    */ APP_MSG_NULL,
    /* DOUBLE*/ APP_MSG_VIDEO_DEC_NEXT,
    /* TRIPLE*/ APP_MSG_NULL,
};
const int key_video_dec_io_num2_msg_table[KEY_ACTION_MAX] = {
    /* CLICK */ APP_MSG_VIDEO_DEC_NEXT,
    /* LONG  */ APP_MSG_VIDEO_DEC_BACK,
    /* HOLD  */ APP_MSG_NULL,
    /* UP    */ APP_MSG_NULL,
    /* DOUBLE*/ APP_MSG_VIDEO_DEC_PREV,
    /* TRIPLE*/ APP_MSG_NULL,
};
#endif

#if TCFG_ADKEY_ENABLE
const int key_video_dec_ad_num0_msg_table[KEY_ACTION_MAX] = {
    /* CLICK */ APP_MSG_VIDEO_DEC_BACK,
    /* LONG  */ APP_MSG_NULL,
    /* HOLD  */ APP_MSG_NULL,
    /* UP    */ APP_MSG_NULL,
    /* DOUBLE*/ APP_MSG_NULL,
    /* TRIPLE*/ APP_MSG_NULL,
};
const int key_video_dec_ad_num1_msg_table[KEY_ACTION_MAX] = {
    /* CLICK */ APP_MSG_VIDEO_DEC_PLAY,
    /* LONG  */ APP_MSG_VIDEO_DEC_SHOW_DEL_MENU,
    /* HOLD  */ APP_MSG_NULL,
    /* UP    */ APP_MSG_NULL,
    /* DOUBLE*/ APP_MSG_VIDEO_DEC_NEXT,
    /* TRIPLE*/ APP_MSG_NULL,
};
const int key_video_dec_ad_num2_msg_table[KEY_ACTION_MAX] = {
    /* CLICK */ APP_MSG_VIDEO_DEC_NEXT,
    /* LONG  */ APP_MSG_NULL,
    /* HOLD  */ APP_MSG_NULL,
    /* UP    */ APP_MSG_NULL,
    /* DOUBLE*/ APP_MSG_VIDEO_DEC_PREV,
    /* TRIPLE*/ APP_MSG_NULL,
};
#endif

const struct key_remap_table video_dec_mode_key_table[] = {
#if TCFG_IOKEY_ENABLE
    { .key_value = KEY_IO_NUM0, .remap_table = key_video_dec_io_num0_msg_table },
    { .key_value = KEY_IO_NUM1, .remap_table = key_video_dec_io_num1_msg_table },
    { .key_value = KEY_IO_NUM2, .remap_table = key_video_dec_io_num2_msg_table },
#endif
#if TCFG_ADKEY_ENABLE
    { .key_value = KEY_AD_NUM0, .remap_table = key_video_dec_ad_num0_msg_table },
    { .key_value = KEY_AD_NUM1, .remap_table = key_video_dec_ad_num1_msg_table },
    { .key_value = KEY_AD_NUM2, .remap_table = key_video_dec_ad_num2_msg_table },
#endif
    { .key_value = 0xff }
};

#endif /* #if TCFG_APP_VIDEO_DEC_EN */
