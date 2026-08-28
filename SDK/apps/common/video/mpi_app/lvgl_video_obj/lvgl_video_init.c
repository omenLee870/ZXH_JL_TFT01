#include "system/includes.h"
#include "lvgl_video_init.h"

#if defined CONFIG_CPU_BR33

#if (defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE)
#include "lvgl.h"
#include "lv_port_video_disp.h"

extern int lvgl_rpc_post_func(void (*rpc_func)(void), int argc, ...);

static void *g_pending_handle = NULL;
static OS_SEM g_init_sem;
static OS_MUTEX g_init_mutex;

static int _lvgl_video_init_mutex_init(void)
{
    os_sem_create(&g_init_sem, 0);
    os_mutex_create(&g_init_mutex);
    return 0;
}
late_initcall(_lvgl_video_init_mutex_init);

/* RPC 回调：LVGL 线程执行创建 */
static void _lvgl_video_init_rpc(int32_t width, int32_t height, int32_t x, int32_t y, const char *fb_name)
{

#if (defined LV_USE_VIDEO_DISP && LV_USE_VIDEO_DISP)
    lv_video_disp_handle_t *handle = lv_port_video_disp_create(
                                         lv_screen_active(), width, height, x, y, LV_COLOR_FORMAT_NV12, fb_name);
    if (!handle) {
        printf("lv_port_video_disp_create fail\n");
    }
    g_pending_handle = (void *)handle;
    os_sem_post(&g_init_sem);

#else
    printf("error LV_USE_VIDEO_DISP \n");
#endif
}

void *lvgl_video_init(int32_t width, int32_t height, int32_t x, int32_t y, const char *fb_name)
{
    printf("mpi test lvgl_video_init");
    os_mutex_pend(&g_init_mutex, 0);

    void *handle = NULL;

    if (!strcmp(os_current_task(), "ui")) {
        _lvgl_video_init_rpc(width, height, x, y, fb_name);
        handle = g_pending_handle;
        g_pending_handle = NULL;
        os_mutex_post(&g_init_mutex);
    } else {
        os_sem_set(&g_init_sem, 0);
        lvgl_rpc_post_func((void *)_lvgl_video_init_rpc, 5, width, height, x, y,
                           fb_name);
        int sem_ret = os_sem_pend(&g_init_sem, 300);
        handle = g_pending_handle;
        g_pending_handle = NULL;
        os_mutex_post(&g_init_mutex);
        if (sem_ret != OS_ERR_NONE) {
            printf("lvgl_video_init: sem_pend timeout\n");
            return NULL;
        }
    }
    return handle;
}

/* RPC 回调：LVGL 线程执行销毁 */
static void _lvgl_video_exit_rpc(void *handle)
{
#if (defined LV_USE_VIDEO_DISP && LV_USE_VIDEO_DISP)
    if (handle) {
        lv_port_video_disp_del((lv_video_disp_handle_t *)handle);
    }
#else
    printf("error LV_USE_VIDEO_DISP \n");
#endif
}

void lvgl_video_exit(void *handle)
{
    if (handle) {
        printf("mpi test lvgl_video_exit");
        lvgl_rpc_post_func((void *)_lvgl_video_exit_rpc, 1, handle);
    }
}

void lvgl_video_exit_sync(void *handle)
{
    if (handle) {
        printf("mpi test lvgl_video_exit_sync");
        lv_port_video_disp_del((lv_video_disp_handle_t *)handle);
    }
}

#endif /* CONFIG_LVGL_UI_ENABLE */

#endif /* CONFIG_CPU_BR33 */
