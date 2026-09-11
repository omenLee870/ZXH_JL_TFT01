#include "system/includes.h"
#include "lvgl_video_init.h"

#if defined CONFIG_CPU_BR33

#if (defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE)
#include "lvgl.h"
#include "lv_port_video_disp.h"

extern int lvgl_rpc_post_func(void (*rpc_func)(void), int argc, ...);

static void *g_pending_handle = NULL;
static OS_SEM g_init_sem;
static OS_SEM g_exit_sem;
static OS_MUTEX g_init_mutex;

static int _lvgl_video_init_mutex_init(void)
{
    os_sem_create(&g_init_sem, 0);
    os_sem_create(&g_exit_sem, 0);
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

#if LV_USE_DRAW_JLGPU_LIST == 2
extern void lv_jlgpu_run_disp_drain(void);
#endif

void lvgl_video_exit_sync(void *handle)
{
    /* 先排空在飞的异步渲染(LIST==2), 再删视频 img / dev_close fb5, 避免
     * 异步渲染线程读已释放 buffer (与 lvgl_video_exit_drain 同序)。 */
#if LV_USE_DRAW_JLGPU_LIST == 2
    lv_jlgpu_run_disp_drain();
#endif

    if (handle) {
        printf("mpi test lvgl_video_exit_sync");
        lv_port_video_disp_del((lv_video_disp_handle_t *)handle);
    }
}

/* RPC 回调: LVGL 线程执行销毁 + 排空异步渲染, 完成后 post g_exit_sem 通知调用方 */
static void _lvgl_video_exit_drain_rpc(void *handle)
{
    /* 先排空在飞的异步渲染(LIST==2), 再删视频 img / dev_close fb5:
     * 若反过来先关 fb5 释放 buffer, 异步渲染线程可能仍在读该 buffer, 导致
     * use-after-free(VO 复用后画面下半绿屏 / mmu 异常)。 */
#if LV_USE_DRAW_JLGPU_LIST == 2
    lv_jlgpu_run_disp_drain();
#endif

#if (defined LV_USE_VIDEO_DISP && LV_USE_VIDEO_DISP)
    if (handle) {
        lv_port_video_disp_del((lv_video_disp_handle_t *)handle);
    }
#else
    printf("error LV_USE_VIDEO_DISP \n");
#endif

    os_sem_post(&g_exit_sem);
}

/* 同步退出(任意线程): 在 LVGL 线程删除视频 img 并排空 LIST==2 异步渲染线程。
 * 与 lvgl_video_exit 的差异是阻塞等待完成后才返回, 调用方可在返回后安全释放
 * 视频 buffer / 拆显示通路, 避免双链表异步渲染与释放 buffer 的竞态。 */
void lvgl_video_exit_drain(void *handle)
{
    if (!handle) {
        return;
    }

    if (!strcmp(os_current_task(), "ui")) {
        _lvgl_video_exit_drain_rpc(handle);
    } else {
        os_sem_set(&g_exit_sem, 0);
        lvgl_rpc_post_func((void *)_lvgl_video_exit_drain_rpc, 1, (int)handle);
        if (os_sem_pend(&g_exit_sem, 300) != OS_ERR_NONE) {
            printf("lvgl_video_exit_drain: sem_pend timeout\n");
        }
    }
}

#endif /* CONFIG_LVGL_UI_ENABLE */

#endif /* CONFIG_CPU_BR33 */
