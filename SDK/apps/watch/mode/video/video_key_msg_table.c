/**
 * @file video_key_msg_table.c
 * @brief APP_MODE_VIDEO 按键消息映射表
 *
 * 视频模式按键由 UI 框架处理，此处仅提供空映射表供 app_get_message 使用。
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_key_msg_table.data.bss")
#pragma data_seg(".video_key_msg_table.data")
#pragma const_seg(".video_key_msg_table.text.const")
#pragma code_seg(".video_key_msg_table.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"

#if TCFG_APP_VIDEO_EN

const struct key_remap_table video_mode_key_table[] = {
    { .key_value = 0xff }
};

#endif /* TCFG_APP_VIDEO_EN */
