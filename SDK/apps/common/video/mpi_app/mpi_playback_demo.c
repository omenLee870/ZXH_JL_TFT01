#include "system/includes.h"
#include "app_playback_mpi.h"
#include "lvgl_video_init.h"

static void app_playback_test_event_handler(void *priv, int argc, int *argv)
{
    (void)priv;

    if (argv == NULL || argc <= 0) {
        return;
    }

    switch (argv[0]) {
    case APP_PLAYBACK_EVENT_OPENED:
        if (argc > 1) {
            printf("playback opened, total=%d ms\n", argv[1]);
        }
        break;
    case APP_PLAYBACK_EVENT_STARTED:
        printf("playback started\n");
        break;
    case APP_PLAYBACK_EVENT_FIRST_FRAME:
        printf("playback first frame\n");
        break;
    case APP_PLAYBACK_EVENT_CUR_TIME:
        if (argc > 1) {
            printf("playback cur time: %d ms\n", argv[1]);
        }
        break;
    case APP_PLAYBACK_EVENT_FINISHED:
        printf("playback finished\n");
        break;
    case APP_PLAYBACK_EVENT_STOPPED:
        printf("playback stopped\n");
        break;
    case APP_PLAYBACK_EVENT_ERROR:
        printf("playback error\n");
        break;
    default:
        break;
    }
}

void app_playback_mpi_test(void)
{
    printf("app_playback_mpi_test\n");

    app_playback_handle_t handle;
    app_playback_ret_t ret;
    app_playback_display_attr_t display_attr = {
        .fb_name = "fb5",
        .fb_num = 2,
        .width = 320,
        .height = 386,
    };

    //BR33 需要创建lvgl视频控件才能显示视频,不使用时需要销毁lvgl视频控件:lvgl_video_exit
#if defined CONFIG_CPU_BR33 && defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE
    lvgl_video_init(display_attr.width, display_attr.height, display_attr.left, display_attr.top, display_attr.fb_name);
#endif

    ret = app_playback_open(&handle, &display_attr);
    if (ret != APP_PLAYBACK_OK) {
        printf("failed to open playback handle, ret=%d\n", ret);
        return;
    }

    ret = app_playback_register_event_handler(handle,
        NULL,
        app_playback_test_event_handler);
    if (ret != APP_PLAYBACK_OK) {
        printf("failed to register playback event handler, ret=%d\n", ret);
    }

    // app_playback_one_image_path(handle, "storage/sd0/C/jpg/IMG_0000.jpg");
    // os_time_dly(100 * 2);
    // app_playback_one_image_path(handle, "storage/sd0/C/jpg/IMG_0001.jpg");
    // os_time_dly(100 * 2);
    // app_playback_one_image_path(handle, "storage/sd0/C/jpg/IMG_0002.jpg");
    // os_time_dly(100 * 2);
    // app_playback_one_image_path(handle, "storage/sd0/C/jpg/IMG_0003.jpg");
    // os_time_dly(100 * 2);


    printf("start video playback\n");
    app_playback_video_start(handle, "storage/sd0/C/avi/VID_0000.avi");
}

