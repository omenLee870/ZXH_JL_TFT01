#include "system/includes.h"
#include "app_camera_mpi.h"
#include "lvgl_video_init.h"

static int rec_path_index = 0;

static void app_camera_test_event_handler(void *priv, int argc, int *argv)
{
    app_camera_handle_t handle = (app_camera_handle_t)priv;
    char path[APP_CAMERA_REC_PATH_MAX];

    if (argv == NULL || argc <= 0) {
        return;
    }

    switch (argv[0]) {
    case APP_CAMERA_EVENT_REC_STARTED:
        printf("camera record started\n");
        break;
    case APP_CAMERA_EVENT_REC_TIME:
        if (argc > 1) {
            printf("camera record time: %d ms\n", argv[1]);
        }
        break;
    case APP_CAMERA_EVENT_REC_CYC_FILE:
        if (argc > 1) {
            snprintf(path, sizeof(path), "storage/sd1/C/avi/VID_%04d.AVI", rec_path_index++);
            printf("camera record cycle file, cyc_count=%d, next_path=%s\n", argv[1], path);
            app_camera_rec_set_path(handle, path);
        }
        break;
    case APP_CAMERA_EVENT_REC_STOPPED:
        printf("camera record stopped\n");
        break;
    case APP_CAMERA_EVENT_REC_ERROR:
        printf("camera record error\n");
        break;
    default:
        break;
    }
}

void app_camera_mpi_test(void)
{
    u8 camera_id = 0;
    const char *sensor_name = "NT99141";
    app_camera_handle_t handle;
    app_camera_ret_t ret;

    ret = app_camera_open(camera_id, sensor_name, &handle);
    if (ret != APP_CAMERA_OK) {
        printf("failed to open camera handle, ret=%d\n", ret);
        return;
    }

    ret = app_camera_register_event_handler(handle,
                                            handle,
                                            app_camera_test_event_handler);
    if (ret != APP_CAMERA_OK) {
        printf("failed to register camera event handler, ret=%d\n", ret);
    }

    app_camera_display_attr_t disp_attr = {
        .fb_name = "fb5",
        .fb_num = 2,
        .width = 320,
        .height = 386,
    };

    //BR33 需要创建lvgl视频控件才能显示视频,不使用时需要销毁lvgl视频控件:lvgl_video_exit
#if defined CONFIG_CPU_BR33 && defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE
    lvgl_video_init(disp_attr.width, disp_attr.height, disp_attr.left, disp_attr.top, disp_attr.fb_name);
#endif
    app_camera_display_start(handle, &disp_attr);

#if 0
    app_camera_rec_attr_t rec_attr = {
        .width = 1280,
        .height = 720,
        .fps = 30,
        .abr_kbps = 6000,
        .cyc_sec =  30,
        .stream_buf_size = 300 * 1024,

        //audio
        .audio_enable = 1,
        .audio_buf_size = 64 * 1024,
        .audio_sample_rate = 16000,
        .audio_codec_type = APP_CAMERA_AUDIO_CODEC_MP3, //APP_CAMERA_AUDIO_CODEC_MP3,
        .audio_bit_rate = 64000,            //pcm时无效
        .audio_frame_interval_ms = 500,    //仅在pcm时有效
    };
    snprintf(rec_attr.path, sizeof(rec_attr.path), "storage/sd1/C/avi/VID_%04d.AVI", rec_path_index++);

    app_camera_osd_attr_t osd = {
        .x = 0,
        .y = 64,
        .text = "粤A 12345 Aa JL jl\nyyyy-nn-dd hh:mm:ss",
    };
    app_camera_set_record_text_osd(handle, &osd);
    app_camera_enable_record_osd(handle);
    app_camera_rec_start(handle, &rec_attr);

    os_time_dly(100 * 200);

    app_camera_rec_stop(handle);
    app_camera_close(&handle);

    // osd.x = 0;
    // osd.y = 0;
    // osd.text = "粤A 12345";
    // app_camera_set_record_text_osd(handle, &osd);
    // app_camera_enable_record_osd(handle);
    // app_camera_rec_start(handle, &rec_attr);
    // os_time_dly(100 * 30);
#endif
#if 0
    os_time_dly(100);
    app_camera_photo_attr_t photo_attr = {
        .width = 640,
        .height = 480,
        .quality = 20,
        .stream_buf_size = 500 * 1024,
    };
    snprintf(photo_attr.path, sizeof(photo_attr.path), "storage/sd1/C/jpg/IMG_0000.JPG");
    app_camera_take_photo(handle, &photo_attr);

    photo_attr.width = 1280;
    photo_attr.height = 720;
    snprintf(photo_attr.path, sizeof(photo_attr.path), "storage/sd1/C/jpg/IMG_0001.JPG");
    app_camera_take_photo(handle, &photo_attr);

    photo_attr.width = 3840;
    photo_attr.height = 2160;
    snprintf(photo_attr.path, sizeof(photo_attr.path), "storage/sd1/C/jpg/IMG_0002.JPG");
    app_camera_take_photo(handle, &photo_attr);

    photo_attr.width = 8000;
    photo_attr.height = 6000;
    snprintf(photo_attr.path, sizeof(photo_attr.path), "storage/sd1/C/jpg/IMG_0003.JPG");
    app_camera_take_photo(handle, &photo_attr);

    photo_attr.width = 1920;
    photo_attr.height = 1088;
    snprintf(photo_attr.path, sizeof(photo_attr.path), "storage/sd1/C/jpg/IMG_0004.JPG");
    app_camera_take_photo(handle, &photo_attr);

    photo_attr.width = 2560;
    photo_attr.height = 1440;
    snprintf(photo_attr.path, sizeof(photo_attr.path), "storage/sd1/C/jpg/IMG_0005.JPG");
    app_camera_take_photo(handle, &photo_attr);


    app_camera_close(&handle);
#endif
}

/* Sensor input resolution */
#define SENSOR_NATIVE_W    1280
#define SENSOR_NATIVE_H    720
/* Display (screen) resolution */
#define DISPLAY_W          320
#define DISPLAY_H          386
#define ZOOM_STEP_MAX      10

static void zoom_level_to_crop(int level, app_camera_zoom_param_t *param)
{
    int eff_w, eff_h, crop_w, crop_h, margin_x, margin_y;

    /* Effective area: largest display-ratio region that fits in sensor */
    if (SENSOR_NATIVE_W * DISPLAY_H <= SENSOR_NATIVE_H * DISPLAY_W) {
        eff_w = SENSOR_NATIVE_W;
        eff_h = SENSOR_NATIVE_W * DISPLAY_H / DISPLAY_W;
    } else {
        eff_h = SENSOR_NATIVE_H;
        eff_w = SENSOR_NATIVE_H * DISPLAY_W / DISPLAY_H;
    }

    if (level <= 0) {
        crop_w = eff_w;
        crop_h = eff_h;
    } else {
        crop_w = eff_w / (level + 1);
        crop_h = eff_h / (level + 1);
    }

    /* crop width/height: 2-aligned */
    crop_w = (crop_w / 2) * 2;
    crop_h = (crop_h / 2) * 2;

    /* x_start, y_start: centered, 4-aligned (round down) */
    margin_x = ((SENSOR_NATIVE_W - crop_w) / 2 / 4) * 4;
    margin_y = ((SENSOR_NATIVE_H - crop_h) / 2 / 4) * 4;

    param->x_start = margin_x;
    param->y_start = margin_y;
    param->crop_w  = crop_w;
    param->crop_h  = crop_h;
}

/*
 * Demo: preview + iterative zoom & photo
 *
 * Simulates a user pressing zoom key then shutter key repeatedly:
 *   zoom to level N → take photo → zoom to level N+1 → take photo → ...
 * Each photo captures the current zoomed view.
 */
void app_camera_zoom_photo_demo(void)
{
    u8 camera_id = 0;
    const char *sensor_name = "NT99141";
    app_camera_handle_t handle;
    app_camera_ret_t ret;
    app_camera_zoom_param_t zoom_param;
    int level;

    ret = app_camera_open(camera_id, sensor_name, &handle);
    if (ret != APP_CAMERA_OK) {
        printf("failed to open camera, ret=%d\n", ret);
        return;
    }

    app_camera_display_attr_t disp_attr = {
        .fb_name = "fb5",
        .fb_num  = 2,
        .width   = DISPLAY_W,
        .height  = DISPLAY_H,
    };

    //BR33 需要创建lvgl视频控件才能显示视频,不使用时需要销毁lvgl视频控件:lvgl_video_exit
#if defined CONFIG_CPU_BR33 && defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE
    lvgl_video_init(disp_attr.width, disp_attr.height, disp_attr.left, disp_attr.top, "fb5");
#endif
    ret = app_camera_display_start(handle, &disp_attr);
    if (ret != APP_CAMERA_OK) {
        printf("failed to start display, ret=%d\n", ret);
        app_camera_close(&handle);
        return;
    }

    printf("=== zoom + photo demo ===\n");

    for (level = 1; level <= ZOOM_STEP_MAX / 2; level++) {
        /* Press zoom key → preview updates immediately */
        zoom_level_to_crop(level, &zoom_param);
        ret = app_camera_set_digital_zoom(handle, &zoom_param);
        printf("[zoom ] level=%d crop(%d,%d,%d,%d) ret=%d\n",
               level, zoom_param.x_start, zoom_param.y_start,
               zoom_param.crop_w, zoom_param.crop_h, ret);

        os_time_dly(500); /* short delay for preview to settle */

        /* Press shutter → zoom auto-applies to photo VPSS channel */
        {
            app_camera_photo_attr_t photo_attr = {
                .width  = SENSOR_NATIVE_W,
                .height = SENSOR_NATIVE_H,
                .quality = 20,
                .stream_buf_size = 1024 * 1024,
            };
            snprintf(photo_attr.path, sizeof(photo_attr.path),
                     "storage/sd1/C/jpg/IMG_Z%02d.JPG", level);
            ret = app_camera_take_photo(handle, &photo_attr);
            printf("[photo] level=%d ret=%d\n", level, ret);
        }
    }

    /* Reset zoom */
    app_camera_disable_digital_zoom(handle);
    printf("zoom disabled, back to full view\n");

    app_camera_display_stop(handle);
    app_camera_close(&handle);
    printf("=== zoom + photo demo finished ===\n");
}

/*
 * Demo: preview + iterative zoom & record
 *
 * Simulates a user pressing zoom key then record key repeatedly:
 *   zoom to level N → record short clip → stop → zoom to level N+1 → record → ...
 * Each clip captures the current zoomed view.
 */
void app_camera_zoom_record_demo(void)
{
    u8 camera_id = 0;
    const char *sensor_name = "NT99141";
    app_camera_handle_t handle;
    app_camera_ret_t ret;
    app_camera_zoom_param_t zoom_param;
    int level;

    ret = app_camera_open(camera_id, sensor_name, &handle);
    if (ret != APP_CAMERA_OK) {
        printf("failed to open camera, ret=%d\n", ret);
        return;
    }

    ret = app_camera_register_event_handler(handle,
                                            handle,
                                            app_camera_test_event_handler);
    if (ret != APP_CAMERA_OK) {
        printf("failed to register camera event handler, ret=%d\n", ret);
    }

    app_camera_display_attr_t disp_attr = {
        .fb_name = "fb5",
        .fb_num  = 2,
        .width   = DISPLAY_W,
        .height  = DISPLAY_H,
    };

#if defined CONFIG_CPU_BR33 && defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE
    lvgl_video_init(disp_attr.width, disp_attr.height, disp_attr.left, disp_attr.top, "fb5");
#endif
    ret = app_camera_display_start(handle, &disp_attr);
    if (ret != APP_CAMERA_OK) {
        printf("failed to start display, ret=%d\n", ret);
        app_camera_close(&handle);
        return;
    }

    printf("=== zoom + record demo ===\n");

    for (level = 1; level <= ZOOM_STEP_MAX / 2; level++) {
        /* Press zoom key → preview updates immediately */
        zoom_level_to_crop(level, &zoom_param);
        ret = app_camera_set_digital_zoom(handle, &zoom_param);
        printf("[zoom  ] level=%d crop(%d,%d,%d,%d) ret=%d\n",
               level, zoom_param.x_start, zoom_param.y_start,
               zoom_param.crop_w, zoom_param.crop_h, ret);

        os_time_dly(500); /* short delay for preview to settle */

        /* Press record key → zoom auto-applies to encode via rec_start */
        {
            app_camera_rec_attr_t rec_attr = {
                .width  = SENSOR_NATIVE_W,
                .height = SENSOR_NATIVE_H,
                .fps = 30,
                .abr_kbps = 10000,
                .cyc_sec = 30,
                .stream_buf_size = 1024 * 1024,
                .audio_enable = 1,
                .audio_buf_size = 64 * 1024,
                .audio_sample_rate = 16000,
                .audio_codec_type = APP_CAMERA_AUDIO_CODEC_MP3,
                .audio_bit_rate = 64000,
                .audio_frame_interval_ms = 500,
            };
            snprintf(rec_attr.path, sizeof(rec_attr.path),
                     "storage/sd1/C/avi/VID_Z%02d.AVI", level);

            ret = app_camera_rec_start(handle, &rec_attr);
            if (ret != APP_CAMERA_OK) {
                printf("failed to start recording at level %d, ret=%d\n", level, ret);
                continue;
            }
            printf("[record] level=%d started\n", level);

            os_time_dly(100 * 50); /* 5 seconds per clip */

            app_camera_rec_stop(handle);
            printf("[record] level=%d stopped\n", level);
        }
    }

    /* Reset zoom */
    app_camera_disable_digital_zoom(handle);
    printf("zoom disabled\n");

    app_camera_display_stop(handle);
    app_camera_close(&handle);
    printf("=== zoom + record demo finished ===\n");
}





