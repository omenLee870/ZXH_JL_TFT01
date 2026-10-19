/**
 * @file camera_display_ui.c
 * @brief 摄像头推屏到LVGL的实现
 *
 * 参考 mpi_camera_demo.c 和 video_rec_ui.c，提供:
 * - camera_display_ui_init():  初始化摄像头推屏, LVGL线程安全
 * - camera_display_ui_deinit(): 卸载摄像头推屏, 释放所有资源
 *
 * 数据流: XS9950 -> VI -> VPSS -> VO -> fb5 -> LVGL视频控件 -> JSTC合成 -> LCD
 */
#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#include "camera_display_ui.h"
#include "system/includes.h"

#if defined CONFIG_CPU_BR33 && defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE
#include "lvgl.h"
#include "lvgl_video_init.h"
#endif
#include "app_camera_mpi.h"

/* ---- 默认配置 ---- */
#define CAM_DISP_SENSOR    "XS9950"
#define CAM_DISP_FB_NAME   "fb5"
#define CAM_DISP_FB_NUM    2

/* ---- 静态状态 ---- */
static app_camera_handle_t g_cam_handle      = NULL;
static int                 g_cam_active       = 0;

static int g_cam_w = 0;
static int g_cam_h = 0;
static int g_cam_x = 0;
static int g_cam_y = 0;

static void *g_cam_video_handle = NULL;

/* ---- 摄像头事件回调 (预留) ---- */
static void _cam_event_handler(void *priv, int argc, int *argv)
{
    (void)priv;
    if (argv == NULL || argc <= 0) {
        return;
    }
    switch (argv[0]) {
    case APP_CAMERA_EVENT_REC_STARTED:
        printf("[CAM_DISP] rec started\n");
        break;
    case APP_CAMERA_EVENT_REC_STOPPED:
        printf("[CAM_DISP] rec stopped\n");
        break;
    case APP_CAMERA_EVENT_REC_ERROR:
        printf("[CAM_DISP] rec error\n");
        break;
    default:
        break;
    }
}

/* ============ 公开接口 ============ */

int camera_display_ui_init(int width, int height, int x, int y)
{
    extern int app_meter_control_media_allowed(void);
    if (!app_meter_control_media_allowed()) return -1;
    app_camera_ret_t ret;

    if (g_cam_active) {
        printf("[CAM_DISP] already active, skip\n");
        return 0;
    }

    /* ---- 1. 打开摄像头 ---- */
    ret = app_camera_open(0, CAM_DISP_SENSOR, &g_cam_handle);
    if (ret != APP_CAMERA_OK) {
        printf("[CAM_DISP] open camera failed, ret=%d\n", ret);
        return -3;
    }

    app_camera_register_event_handler(g_cam_handle,
                                      g_cam_handle,
                                      _cam_event_handler);

    g_cam_w = width;
    g_cam_h = height;
    g_cam_x = x;
    g_cam_y = y;

    /* ---- 2. 启动摄像头推屏 (VI -> VPSS -> VO -> fb5) ----
     * 必须先建好 fb5 缓冲区, 后续 LVGL 视频控件才能读到有效帧 */
    app_camera_display_attr_t disp_attr = {
        .fb_name = CAM_DISP_FB_NAME,
        .fb_num  = CAM_DISP_FB_NUM,
        .width   = (uint16_t)g_cam_w,
        .height  = (uint16_t)g_cam_h,
        .left    = (uint16_t)g_cam_x,
        .top     = (uint16_t)g_cam_y,
    };
    ret = app_camera_display_start(g_cam_handle, &disp_attr);
    if (ret != APP_CAMERA_OK) {
        printf("[CAM_DISP] display start failed, ret=%d\n", ret);
        app_camera_close(&g_cam_handle);
        g_cam_handle = NULL;
        return -4;
    }

    /* ---- 3. 创建 LVGL 视频控件 (读 fb5 上屏) ----
     * 必须在 app_camera_display_start 之后创建, 否则 fb5 尚无 buffer 读不到帧 */
    g_cam_video_handle = lvgl_video_init(g_cam_w, g_cam_h, g_cam_x, g_cam_y, CAM_DISP_FB_NAME);
    if (!g_cam_video_handle) {
        printf("[CAM_DISP] video widget failed; release camera pipeline\n");
        app_camera_display_stop(g_cam_handle);
        app_camera_close(&g_cam_handle);
        g_cam_handle = NULL;
        return -5;
    }

    g_cam_active = 1;
    printf("[CAM_DISP] init ok: %dx%d at (%d,%d)\n", g_cam_w, g_cam_h, g_cam_x, g_cam_y);
    os_time_dly(8); //加延时是为了防止摄像头未出数据LVGL就渲染第一帧导致黑屏(25fps x 2)
    return 0;
}

void camera_display_ui_deinit(void)
{
    if (!g_cam_active) {
        return;
    }

    /*
     * 先销毁 LVGL 视频控件 (清掉 JLGPU 绘制任务及 attach_buf 对 fb5 的引用),
     * 再停止摄像头管道释放 fb5 大块缓冲区。
     *
     * 若反过来(先释放 fb5、后销毁控件), 控件销毁时清 attach_buf 会 free
     * 已随 fb5 一起失效的帧指针, 导致 free 野指针崩溃 (vPortFree)。
     *
     * 注意: 本函数在 LVGL 线程 (lv_timer 回调) 中被调用,
     * 必须用同步版本 lvgl_video_exit_sync() 而非 RPC 版本的 lvgl_video_exit()。
     */
#if defined CONFIG_CPU_BR33 && defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE
    lvgl_video_exit_sync(g_cam_video_handle);
    g_cam_video_handle = NULL;
#endif

    app_camera_display_stop(g_cam_handle);
    app_camera_close(&g_cam_handle);
    g_cam_handle = NULL;

    g_cam_active = 0;
    printf("[CAM_DISP] deinit ok\n");
}

int camera_display_ui_is_active(void)
{
    return g_cam_active;
}
#endif
