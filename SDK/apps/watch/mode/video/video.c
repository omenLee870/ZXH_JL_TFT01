#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video.data.bss")
#pragma data_seg(".video.data")
#pragma const_seg(".video.text.const")
#pragma code_seg(".video.text")
#endif
#include "system/includes.h"
#include "app_main.h"
#include "app_mode.h"
#include "app_video.h"
#include "ui/ui_api.h"

#define LOG_TAG             "[APP_VIDEO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_APP_VIDEO_EN

static int app_video_init()
{
    log_info("%s", __func__);

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_VIDEO);

    return 0;
}

static void app_video_exit()
{
    log_info("%s", __func__);

    UI_MSG_POST("video_show_exit");
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_VIDEO);
}

struct app_mode *app_enter_video_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_video_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), video_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_video_exit();

    return next_mode;
}

static int video_mode_try_enter(int arg)
{
    log_info("%s", __func__);
    return 0;
}

static int video_mode_try_exit()
{
    log_info("%s", __func__);
    return 0;
}

static const struct app_mode_ops video_mode_ops = {
    .try_enter          = video_mode_try_enter,
    .try_exit           = video_mode_try_exit,
};

/*
 * 注册video模式
 */
REGISTER_APP_MODE(video_mode) = {
    .name 	= APP_MODE_VIDEO,
    .index  = 0xff,
    .ops 	= &video_mode_ops,
};

#endif /*TCFG_APP_VIDEO_EN*/
