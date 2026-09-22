#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video.data.bss")
#pragma data_seg(".video.data")
#pragma const_seg(".video.text.const")
#pragma code_seg(".video.text")
#endif

#include "system/includes.h"
#include "os/os_api.h"
#include "jiffies.h"
#include "app_main.h"
#include "app_msg.h"
#include "app_video.h"
#include "app_config.h"
#include "app_action.h"
#include "app_tone.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "video_rec.h"
#include "video_rec_internal.h"
#include "key_driver.h"
#include "video_rec_ui.h"
#include "app_power_manage.h"  /* get_vbat_percent: 低电量检测 */
#include "media_file_list.h"

#include "fs/fs.h"

/* 摄像头源二选一: DVP(内部 sensor) 或 USB Host UVC(外接摄像头)。
 * UVC 打开要求 TCFG_HOST_UVC_ENABLE 与 TCFG_USB_HOST_ENABLE 同时为 1, 缺一不可。 */
#if TCFG_CAMERA_DVP_ENABLE
#include "app_camera_mpi.h"
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
#include "os/os_api.h"
#include "usb_host.h"
#include "host_uvc.h"
#include "uvc_device.h"
#include "mpi_common.h"
#include "mpi_vdec.h"
#include "mpi_vpss.h"
#include "mpi_venc.h"
#include "mpi_vo.h"
#include "mpi_sys.h"
#include "fb.h"
#include "avilib.h"
#endif

/* 将 sys_timer 绑定到指定线程（如 app_core），回调在该线程 taskq 中执行 */
extern u16 sys_timer_add_2_task(void *priv, void (*func)(void *priv),
                                u32 msec, const char *task_name);

#if TCFG_APP_VIDEO_REC_EN

#define LOG_TAG             "[APP_VIDEO_REC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

/* 弱默认函数: 当 kid_camera LVGL UI 未被编译时提供默认值 */
_WEAK_
u8 kid_settings_get_resolution(void)
{
    return 1;  /* 默认非 QVGA (VGA) */
}

_WEAK_
u8 kid_settings_get_rec_recyle_sec(void)
{
    return 0;  /* 默认使用 LOOP_TIME_DEFAULT (60s) */
}

extern void rtc_read_time(struct sys_time *time);
extern void fat_set_datetime_info(u16 year, u8 month, u8 day, u8 hour, u8 minute, u8 second);

/* 显示属性 (LCD preview): 摄像头预览保持 320x240(传感器 4:3 等比画面),
 * 预览 fb 与 LVGL 视频控件同尺寸 */
#if TCFG_CAMERA_DVP_ENABLE
static const app_camera_display_attr_t g_rec_disp_attr = {
    .fb_name = "fb5",
    .fb_num = 2,
    .width = 320,
    .height = 240,
};
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
typedef struct {
    const char *fb_name;
    u8  fb_num;
    u16 width;
    u16 height;
} video_rec_uvc_disp_attr_t;
static const video_rec_uvc_disp_attr_t g_rec_disp_attr = {
    .fb_name = "fb5",
    .fb_num = 2,
    .width = 320,
    .height = 240,
};
#endif

/* 录像实际文件名, AVI_close 前填, 供上层缓存/回显 */
static char g_last_video_file[128];

/*==============================================================================
 * 全局上下文
 *==============================================================================*/

video_rec_ctx_t g_rec_ctx = {0};

/* SD IN 自动开录串行化: 重建进行中挂起, 等 APP_MSG_FLM_DB_CHANGED; 5s 超时兜底防重建挂死 */
#define VIDEO_REC_SD_AUTO_START_TIMEOUT_MS 5000
static u8  g_sd_auto_start_pending;
static u32 g_sd_auto_start_pending_ms;

/* GC0308 source dimensions used by the camera zoom crop. */
#define VIDEO_REC_SENSOR_WIDTH   640u
#define VIDEO_REC_SENSOR_HEIGHT   480u

/* Recording zoom uses the same center crop as preview and photo. */
static u8 g_rec_zoom_tenths = 10;
static int video_rec_zoom_apply(u8 zoom_tenths);
float video_rec_zoom_get(void)
{
    return g_rec_zoom_tenths / 10.0f;
}

/* 慢动作: 影响下次录像启动帧率(启动前配置, 安全)。慢动作15fps, 正常20fps。 */
static u8 g_rec_slowmo = 0;
u8   video_rec_slowmo_get(void)
{
    return g_rec_slowmo;
}
void video_rec_slowmo_toggle(void)
{
    g_rec_slowmo = !g_rec_slowmo;
}
#define VIDEO_REC_FPS_SLOWMO   15
/* 录像中定期检查剩余空间: 不足时删除最旧未加锁录像继续录, 删无可删才停止 */
#define VIDEO_REC_SPACE_CHECK_INTERVAL_MS   5000
float video_rec_zoom_in(void)
{
    if (g_rec_zoom_tenths < 40) {
        u8 zoom_tenths = g_rec_zoom_tenths + 1u;

        if (video_rec_zoom_apply(zoom_tenths) == 0) {
            g_rec_zoom_tenths = zoom_tenths;
        }
    }
    return video_rec_zoom_get();
}

float video_rec_zoom_out(void)
{
    if (g_rec_zoom_tenths > 10) {
        u8 zoom_tenths = g_rec_zoom_tenths - 1u;

        if (video_rec_zoom_apply(zoom_tenths) == 0) {
            g_rec_zoom_tenths = zoom_tenths;
        }
    }
    return video_rec_zoom_get();
}

/* 摄像头UI内部状态 (已从video_rec.h移入) */
#if TCFG_CAMERA_DVP_ENABLE
static app_camera_handle_t g_camera_handle = NULL;
#endif
static u8  g_display_ready         = 0;
static u8  g_recording_ui_active   = 0;
static u32 g_last_ui_update_time   = 0;
static char g_rec_time_str[16]     = {0};

static int video_rec_zoom_apply(u8 zoom_tenths)
{
#if TCFG_CAMERA_DVP_ENABLE
    app_camera_zoom_param_t param;
    app_camera_ret_t ret;

    if (!g_camera_handle) {
        log_error("[VREC] camera is not ready for zoom\n");
        return -1;
    }
    if (g_rec_ctx.recording) {
        log_error("[VREC] zoom change while recording is not supported\n");
        return -1;
    }

    if (zoom_tenths == 10) {
        ret = app_camera_disable_digital_zoom(g_camera_handle);
    } else {
        param.crop_w = (VIDEO_REC_SENSOR_WIDTH * 10u / zoom_tenths) & ~3u;
        param.crop_h = (VIDEO_REC_SENSOR_HEIGHT * 10u / zoom_tenths) & ~3u;
        param.x_start = ((VIDEO_REC_SENSOR_WIDTH - param.crop_w) / 2u) & ~3u;
        param.y_start = ((VIDEO_REC_SENSOR_HEIGHT - param.crop_h) / 2u) & ~3u;
        ret = app_camera_set_digital_zoom(g_camera_handle, &param);
    }

    if (ret != APP_CAMERA_OK) {
        log_error("[VREC] apply zoom %.1fx failed: %d\n",
                  zoom_tenths / 10.0f, ret);
        return -1;
    }

    return 0;
#else
    /* UVC 摄像头不做数字变焦 */
    (void)zoom_tenths;
    return -1;
#endif
}
/* UI 计时 500ms 定时器, 仅 RECORDING 状态下由 transition 启停 */
static u16 s_rec_ui_timer          = 0;

/* sys_timer 可能被 worker 线程和 app_core 同时 add/del（frame_timer、ui_timer），
 * SDK 定时器链表未确认多核安全，这里用互斥锁串行化本模块的定时器操作，避免链表写坏。 */
static OS_MUTEX s_vrec_timer_mutex;
static volatile u8 s_vrec_timer_mutex_inited = 0;

static void vrec_timer_lock(void)
{
    if (!s_vrec_timer_mutex_inited) {
        os_mutex_create(&s_vrec_timer_mutex);
        s_vrec_timer_mutex_inited = 1;
    }
    os_mutex_pend(&s_vrec_timer_mutex, 0);
}

static void vrec_timer_unlock(void)
{
    os_mutex_post(&s_vrec_timer_mutex);
}

static void video_rec_enable_osd(void)
{
#if TCFG_CAMERA_DVP_ENABLE
    static const app_camera_osd_attr_t osd = {
        .x = 0,
        .y = 64,
        .text = "yyyy-nn-dd hh:mm:ss",
    };
    app_camera_ret_t ret;

    ret = app_camera_set_record_text_osd(g_camera_handle, &osd);
    if (ret != APP_CAMERA_OK) {
        log_error("[VREC] set record OSD failed: %d\n", ret);
        return;
    }
    ret = app_camera_enable_record_osd(g_camera_handle);
    if (ret != APP_CAMERA_OK && ret != APP_CAMERA_ERR_EXIST) {
        log_error("[VREC] enable record OSD failed: %d\n", ret);
    }
#else
    /* UVC 录像不做硬件 OSD */
#endif
}

/* RTC 无效(未接入/掉电)时 rtc_read_time 全 0 文件名变 00000000, 用 jiffies 上电秒数派生 boot-time fallback 时间 */
static u8 g_rtc_available     = 0;   // 1=RTC 时间有效, 0=fallback 到 boot time
static u8 g_rtc_diag_logged   = 0;   // mode_init 自检只 log 一次

static int video_rec_check_rtc(void)
{
    struct sys_time t;
    memset(&t, 0, sizeof(t));
    rtc_read_time(&t);
    int valid = (t.year >= 2020 && t.year <= 2099
                 && t.month >= 1 && t.month <= 12
                 && t.day   >= 1 && t.day   <= 31
                 && t.hour  <= 23 && t.min   <= 59 && t.sec <= 59);
    g_rtc_available = valid ? 1 : 0;
    if (!g_rtc_diag_logged) {
        g_rtc_diag_logged = 1;
        if (valid) {
            log_info("[VREC] RTC diag: OK  year=%u mon=%u day=%u %02u:%02u:%02u\n",
                     t.year, t.month, t.day, t.hour, t.min, t.sec);
        } else {
            log_error("[VREC] RTC diag: INVALID year=%u mon=%u day=%u %02u:%02u:%02u"
                      " -> fallback to boot-time jiffies\n",
                      t.year, t.month, t.day, t.hour, t.min, t.sec);
        }
    }
    return valid;
}

static void video_rec_log_mem(const char *tag)
{
    u32 ram0 = (u32)memory_get_size(P_MEMORY_UNUSED);
    u32 ram1 = (u32)memory_get_size(P_VLT_MEMORY_UNUSED);
    log_info("[VREC][MEM] %s: ram0_free=%uB(%uKB), ram1_free=%uB(%uKB), total_free=%uB(%uKB)\n",
             tag, ram0, ram0 / 1024, ram1, ram1 / 1024,
             ram0 + ram1, (ram0 + ram1) / 1024);
}

static void video_rec_log_task_stack(const char *tag, const char *task_name)
{
    SIMPLE_TASK_INFO info[24];
    u16 n = simple_task_info_list_get(info, 24);
    for (u16 i = 0; i < n; i++) {
        if (strncmp(info[i].task_name, task_name, SIMPLE_TASK_INFO_NAME_MAX_LEN) == 0) {
            u32 max_words = info[i].stack_max;
            u32 used_words = info[i].stack_used;
            u32 remain_words = (max_words > used_words) ? (max_words - used_words) : 0;
            log_info("[VREC][STK] %s: name=%s, stack_used=%u/%u words (remain=%u, bytes_used=%u, bytes_max=%u)\n",
                     tag, info[i].task_name,
                     used_words, max_words, remain_words,
                     used_words * 4, max_words * 4);
            return;
        }
    }
    log_info("[VREC][STK] %s: task '%s' not in list (n=%u)\n", tag, task_name, n);
}

/* 状态查询 */
int video_rec_is_recording(void)
{
    return g_rec_ctx.recording;
}

u32 video_rec_get_elapsed_sec(void)
{
    /* 用 segment_start_time(每段重置)而非 record_start_time(全程累加): 循环录像分段后计时归零.
     * 暂停时长要扣除: 真暂停期间不写帧, 文件时长不包含暂停, UI 计时也不能包含 */
    if (!g_rec_ctx.recording) {
        return 0;
    }
    u32 eff_ms = sys_timer_get_ms() - g_rec_ctx.pause_accum_ms;
    return (eff_ms / 1000) - g_rec_ctx.segment_start_time;
}

void video_rec_build_file_path(char *out_path, int out_size);

/* 录像中巡检/边界删除在 video_rec_worker.c 实现; 头文件不改, 这里文件内声明 */
extern int video_rec_check_storage_no_delete(void);
extern int video_rec_try_delete_old(u32 *out_free_kb);
extern volatile u8 g_rec_need_delete_old;
extern volatile u8 g_rec_cyc_boundary;

/* 分段边界待维护: CYC_FILE 回调不再交下一段路径(MPI 停在 wait_next_file 等),
 * 置位后由 worker 主循环停录→删最旧→重启(重启自带新路径); 超时未服务则看门狗强制交路径 */
#define VREC_MAINT_PENDING_TIMEOUT_MS   300
static volatile u8  g_rec_maint_pending    = 0;
static volatile u32 g_rec_maint_pending_ms = 0;

/* MPI cyc_sec 驱动分段: close→reopen→post CYC_FILE, 回调 frename 占位路径为时间戳名 */
static volatile u32 g_cyc_file_count = 0;   /* CYC_FILE 事件累计次数 */
/* 本次录像会话开始前的段数基数: 维护性停录重开新段后段号继续递增(MPI cyc_count 会从 1 重数) */
static u32 g_rec_cyc_base = 0;

#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
/* UVC 录像管线前置声明 (video_rec_start/stop 在本文件前部, 定义在 mode_init 之前) */
static int  vrec_uvc_rec_start(const char *path, u16 w, u16 h);
static void vrec_uvc_rec_stop(void);
static u16 g_uvc_width;
static u16 g_uvc_height;
#endif

#if TCFG_CAMERA_DVP_ENABLE
/* 帧计数定时器回调 (30fps) */
static void video_rec_frame_tick(void *priv)
{
    if (g_rec_ctx.recording && !g_rec_ctx.paused) {
        g_rec_ctx.total_frames++;
        g_rec_ctx.avi_frame_count++;
    }
}

static void video_rec_event_handler(void *priv, int argc, int *argv)
{
    (void)priv;
    if (argv == NULL || argc <= 0) {
        return;
    }

    int evt = argv[0];
    switch (evt) {
    case APP_CAMERA_EVENT_REC_STARTED:
        log_info("[VREC] MPI event: REC_STARTED\n");
        break;
    case APP_CAMERA_EVENT_REC_TIME:
        /* argv[1] = 当前录制时长 ms (MPI cyc_sec 线程 1s 上报一次) */
        if (argc > 1) {
            log_debug("[VREC] MPI event: REC_TIME=%dms\n", argv[1]);
        }
        break;
    case APP_CAMERA_EVENT_REC_CYC_FILE: {
        /* argv[1] = cyc_count (1-based, MPI 自增) */
        int cyc_count = (argc > 1) ? argv[1] : 0;
        g_cyc_file_count++;
        /* 本回调在 MPI camera event 线程: 只置边界标志, FS 删除交给 rec_worker */
        g_rec_cyc_boundary = 1;
        /* Mode0 时间戳命名走 frename+file_index 段号; Mode1 固定 VID_****.AVI 由 JLFS 撞名自增 */
        if (VIDEO_REC_NAMING_MODE == 1) {
            /* Mode 1: 跳过 frename + 跳过 file_index, 仅刷新 ctx 字段 */
            char new_path[APP_CAMERA_REC_PATH_MAX];
            video_rec_build_file_path(new_path, sizeof(new_path));
            g_rec_ctx.total_segments = g_rec_cyc_base + (u32)cyc_count;
            snprintf(g_rec_ctx.current_file_path,
                     sizeof(g_rec_ctx.current_file_path), "%s", new_path);
            g_rec_ctx.segment_start_time = sys_timer_get_ms() / 1000;
            log_info("[VREC] CYC_FILE #%d (FIXED mode=1, skip frename/file_index):"
                     " next=%s\n", cyc_count, g_rec_ctx.current_file_path);
            /* 有待删文件时不交路径: 交了 MPI 立刻开新段, 随后维护停录会掐断它留 0 帧碎片.
             * 置 pending 交 worker: 停录→删最旧→重启, 重启自带新路径解开 MPI 的等待 */
            if (g_rec_need_delete_old) {
                g_rec_maint_pending_ms = jiffies_msec();
                g_rec_maint_pending    = 1;
                log_info("[VREC] CYC_FILE #%d: maint pending, hold path\n", cyc_count);
            } else {
                /* 必须 set_path 交下一段路径, 否则 wait_next_file 永久 pend 阻塞(VENC 缓冲不排空死循环) */
                app_camera_rec_set_path(g_camera_handle, new_path);
            }
            break;
        }

        /* Mode 0: file_index 推进到 基数+cyc_count, 用作路径中 _NNN 段序号 */
        g_rec_ctx.file_index = g_rec_cyc_base + (u32)cyc_count;

        /* 复用 build_file_path 统一命名: 之前直接 rtc_read_time, RTC 无效拼出 00000000 名与 fallback 源名不重合 → frename 失败 */
        char new_path[APP_CAMERA_REC_PATH_MAX];
        video_rec_build_file_path(new_path, sizeof(new_path));

        /* frename(FILE *file, const char *path) 需要一个 FILE*;
         * 用 fopen("r") 拿到一个句柄, 然后 frename 移走原文件. */
        FILE *fp = fopen(g_rec_ctx.current_file_path, "r");
        if (fp) {
            int ret = frename(fp, new_path);
            fclose(fp);
            log_info("[VREC] CYC_FILE #%d: frename(%s -> %s) ret=%d\n",
                     cyc_count, g_rec_ctx.current_file_path, new_path, ret);
        } else {
            /* 文件可能因 AVI 关闭→重开顺序被截断/替换, fopen 失败正常 */
            log_info("[VREC] CYC_FILE #%d: fopen(%s) failed (cyc_sec truncate),"
                     " target=%s\n", cyc_count,
                     g_rec_ctx.current_file_path, new_path);
        }

        g_rec_ctx.total_segments = g_rec_cyc_base + (u32)cyc_count;
        snprintf(g_rec_ctx.current_file_path,
                 sizeof(g_rec_ctx.current_file_path), "%s", new_path);
        g_rec_ctx.segment_start_time = sys_timer_get_ms() / 1000;
        break;
    }
    case APP_CAMERA_EVENT_REC_STOPPED:
        log_info("[VREC] MPI event: REC_STOPPED\n");
        break;
    case APP_CAMERA_EVENT_REC_ERROR:
        log_error("[VREC] MPI event: REC_ERROR -> stop recording\n");
        /* 写入错误(如拔卡): 投递停止消息, 触发 video_rec_stop 正常结束录制 */
        app_send_message(APP_MSG_VIDEO_REC_STOP, 0);
        break;
    default:
        log_info("[VREC] MPI event: unknown %d\n", evt);
        break;
    }
}

/* 统一填充录像参数(含音频): worker 与 resume 共用, 避免重复遗漏 */
static void video_rec_fill_rec_attr(app_camera_rec_attr_t *ra)
{
    memset(ra, 0, sizeof(*ra));
    strncpy(ra->path, g_rec_ctx.current_file_path, sizeof(ra->path) - 1);
    if (kid_settings_get_resolution() == 0 /* KID_RES_QVGA */) {
        ra->width  = 320;
        ra->height = 240;
    } else {
        ra->width  = VIDEO_REC_WIDTH;
        ra->height = VIDEO_REC_HEIGHT;
    }
    ra->fps             = g_rec_slowmo ? VIDEO_REC_FPS_SLOWMO : VIDEO_REC_FPS;
    ra->quality         = VIDEO_REC_QUALITY;
    ra->abr_kbps        = VIDEO_REC_VENC_ABR_KBPS;
    ra->stream_buf_size = VIDEO_REC_STREAM_BUF_SIZE;
    /* MPI cyc_sec 写满 cyc_frame 帧自动 close+reopen 并 post CYC_FILE, 回调 frename 时间戳名 */
    ra->cyc_sec         = (u32)g_rec_ctx.loop_time_sec;
    /* 录音走 app_video_audio_rec_open, PCM 16kHz 免编码器电脑可放 */
    ra->audio_enable            = 1;
    ra->audio_codec_type        = APP_CAMERA_AUDIO_CODEC_PCM;  /* 0=PCM */
    ra->audio_sample_rate       = 16000;   /* 16kHz, 优于电话8kHz, 环境音更清晰 */
    ra->audio_buf_size          = 32 * 1024;   /* 探针实证: 32KB够用 */
    ra->audio_frame_interval_ms = 500;     /* PCM有效: 每500ms打包一个音频块写入AVI */
    ra->audio_bit_rate          = 0;       /* PCM时无效(仅MP3用) */
}

/* 维护驱动: 分段边界"停录 -> 删最旧 -> 恢复录新段".
 * 停录用 app_camera_rec_stop 只关录制通道(VO 预览通道不动), 目的是回收录像占用的
 * 连续物理页: 录像期池子被切碎, 直接 fdelete 取不到连续块会触发 P_MEM 风暴死机.
 * 在 worker 线程内联执行, 不走 video_rec_stop 的 worker 自杀路径, 也不经 app_core 投递. */
static void video_rec_maint_pause_delete_resume(void)
{
    if (!g_rec_ctx.recording || g_rec_ctx.paused || g_camera_handle == NULL) {
        g_rec_cyc_boundary = 0;
        g_rec_maint_pending = 0;
        return;
    }

    log_info("[VREC] maint: pause rec for delete\n");
    if (app_camera_rec_stop(g_camera_handle) != APP_CAMERA_OK) {
        log_error("[VREC] maint: pause rec failed\n");
        g_rec_cyc_boundary = 0;
        g_rec_maint_pending = 0;
        return;
    }
    u32 free_kb = 0;
    int deleted = video_rec_try_delete_old(&free_kb);

    /* 维护窗口内 app_core 已请求停止: 不恢复, 剩余清理交给 worker 退出路径 */
    if (g_rec_ctx.worker_stop) {
        log_info("[VREC] maint: worker_stop set, skip resume\n");
        g_rec_maint_pending = 0;
        return;
    }

    if (deleted < 0) {
        log_error("[VREC] maint: no file to delete\n");
        video_rec_ui_show_alert((deleted == -2) ? 1 : 0);
        app_send_message(APP_MSG_VIDEO_REC_STOP, 0);
        g_rec_ctx.worker_stop = 1;
        g_rec_maint_pending = 0;
        return;
    }

    /* 恢复: 新起一段(段号在 g_rec_cyc_base 上继续), 预览通道全程未动 */
    video_rec_build_file_path(g_rec_ctx.current_file_path, sizeof(g_rec_ctx.current_file_path));
    video_rec_enable_osd();
    app_camera_rec_attr_t ra;
    video_rec_fill_rec_attr(&ra);
    if (app_camera_rec_start(g_camera_handle, &ra) != APP_CAMERA_OK) {
        log_error("[VREC] maint: resume failed\n");
        video_rec_ui_show_alert(2);
        app_send_message(APP_MSG_VIDEO_REC_STOP, 0);
        g_rec_ctx.worker_stop = 1;
        g_rec_maint_pending = 0;
        return;
    }

    g_rec_cyc_base = g_rec_ctx.total_segments;
    if (VIDEO_REC_NAMING_MODE == 0) {
        g_rec_ctx.file_index++;
    }
    g_rec_ctx.total_segments++;
    g_rec_ctx.segment_start_time = jiffies_msec() / 1000;
    log_info("[VREC] maint: resume rec deleted=%d free=%uKB\n", deleted, free_kb);
    g_rec_maint_pending = 0;
}

/* 录制工作者线程 */
static void video_rec_worker(void *arg)
{
    (void)arg;
    log_info("[VREC] worker started\n");

    app_camera_rec_attr_t ra;
    video_rec_fill_rec_attr(&ra);
    log_info("[VREC] worker: rec_start (loop_time=%us, cyc_sec=%us, mpicyc-driven)\n",
             g_rec_ctx.loop_time_sec, ra.cyc_sec);
    video_rec_enable_osd();
    app_camera_ret_t ret = app_camera_rec_start(g_camera_handle, &ra);
    if (ret != APP_CAMERA_OK) {
        log_error("[VREC] worker: rec_start failed: %d\n", ret);
        g_rec_ctx.recording = 0;
        if (video_rec_check_storage() == -2) {
            log_error("[VREC] rec_start failed but SD not online -> alert type=1\n");
            video_rec_ui_show_alert(1);  /* 请插入SD卡 */
        } else {
            video_rec_ui_show_alert(2);  /* 系统错误 */
        }
        return;
    }
    g_rec_ctx.record_start_time = sys_timer_get_ms() / 1000;
    /* frame_timer 统一由 app_core 在 video_rec_start() 创建，worker 不再 add */
    /* Mode0 _NNN 由 file_index 派生(启动+1 首段 001); Mode1 不写入路径 */
    if (VIDEO_REC_NAMING_MODE == 0) {
        g_rec_ctx.file_index++;
    }
    g_rec_ctx.total_segments++;

    /* 必须先置 record_start_time 再 transition, UI 回调 elapsed 才正确 */
    video_rec_transition_to(VIDEO_STATE_RECORDING);

    log_info("[VREC] worker: recording started, loop_time=%us, path=%s\n",
             g_rec_ctx.loop_time_sec, g_rec_ctx.current_file_path);

    /* 段计时基线: seg_start_ms 每段重置, print_last 用于 1s FPS 打印 */
    u32 seg_start_ms   = jiffies_msec();
    u32 print_last_ms  = seg_start_ms;
    u32 print_last_frm = g_rec_ctx.total_frames;
    u32 last_storage_check_ms = seg_start_ms;
    g_rec_ctx.segment_start_time = seg_start_ms / 1000;

    while (!g_rec_ctx.worker_stop) {
        /* worker 常驻; 暂停由 app_camera_rec 层丢帧实现, 本循环不参与, 退出条件只有 stop */
        u32 now_ms = jiffies_msec();

        /* 分段边界待维护: 回调没交下一段路径(MPI 停在 wait_next_file), 这里停录→删最旧→重启.
         * 服务条件不满足时不消费 pending(否则早退清标志, 看门狗永远等不到) */
        if (g_rec_maint_pending) {
            if ((u32)jiffies_msec2offset(g_rec_maint_pending_ms, now_ms) >=
                VREC_MAINT_PENDING_TIMEOUT_MS) {
                /* 看门狗: worker 迟迟没服务, MPI 会永久 pend(VENC 缓冲不排空).
                 * 宁可留一个碎片段, 也不能让录像卡死 */
                char wd_path[APP_CAMERA_REC_PATH_MAX];
                video_rec_build_file_path(wd_path, sizeof(wd_path));
                log_error("[VREC] maint watchdog: force set_path\n");
                app_camera_rec_set_path(g_camera_handle, wd_path);
                g_rec_maint_pending = 0;
            } else if (g_rec_ctx.recording && !g_rec_ctx.paused && g_camera_handle != NULL) {
                video_rec_maint_pause_delete_resume();
            }
        }

        /* ① 每秒打印一次当前录制状态 (累计帧数 / 1s 内的瞬时 FPS) */
        if (g_rec_ctx.recording && (u32)jiffies_msec2offset(print_last_ms, now_ms) >= 1000) {
            u32 frames_now  = g_rec_ctx.total_frames;
            u32 frames_dlt  = frames_now - print_last_frm;
            u32 seg_dur_ms  = jiffies_msec2offset(seg_start_ms, now_ms);
            u32 seg_dur_sec = seg_dur_ms / 1000;
            log_info("[VREC] rec=1 seg=%u/%us t=%us frames=%u (+%u/1s fps=%u) path=%s\n",
                     g_rec_ctx.total_segments,
                     g_rec_ctx.loop_time_sec,
                     seg_dur_sec,
                     frames_now,
                     frames_dlt,
                     frames_dlt,                            /* 1s delta ≈ 瞬时 FPS */
                     g_rec_ctx.current_file_path);
            print_last_ms  = now_ms;
            print_last_frm = frames_now;
        }

        /* ② 录像中定期检查剩余空间: 只置待删标志, 不在录像满负荷时做 FS 删除
         *   (此时物理池近枯竭, fdelete 取不到物理页会打死系统); 删无可删/拔卡才停录 */
        if (g_rec_ctx.recording &&
            (u32)jiffies_msec2offset(last_storage_check_ms, now_ms) >= VIDEO_REC_SPACE_CHECK_INTERVAL_MS) {
            last_storage_check_ms = now_ms;
            int stor_ret = video_rec_check_storage_no_delete();
            if (stor_ret == -2) {
                log_error("[VREC] SD offline during recording, stopping\n");
                video_rec_ui_show_alert(1);
                app_send_message(APP_MSG_VIDEO_REC_STOP, 0);
                g_rec_ctx.worker_stop = 1;
                break;
            } else if (stor_ret != 0) {
                log_error("[VREC] free space below danger line, stopping\n");
                video_rec_ui_show_alert(0);
                app_send_message(APP_MSG_VIDEO_REC_STOP, 0);
                g_rec_ctx.worker_stop = 1;
                break;
            }
        }

        /* 分段边界维护已上移到循环开头: 判据由"段内时间余量"改为回调置的 g_rec_maint_pending,
         * 边界一到即停录删最旧(那时下一段还没建, 不会掐断它), 每段都是完整 cyc_sec */

        /* ③ MPI cyc_sec 驱动的自动分段: 由 app_camera_rec_thread 内部
         *   完成 close+reopen, 然后 post APP_CAMERA_EVENT_REC_CYC_FILE.
         *   video_rec_event_handler() 收到事件后做 frename, 同步
         *   g_rec_ctx.total_segments / segment_start_time.
         *   worker 主循环不参与 stop+start 轮转, 仅打印日志. */

        /* 排空 taskq: frame_tick/ui_tick 等 sys_timer 到期事件与 jlstream
         * 音频回调都投递到本线程 taskq (timer.h: 在哪个线程 add 事件投给
         * 哪个线程), 不取则回调永不执行 (时间卡 0000), 退出时遗留触发
         * "timer func no del" assert. */
        int tq[8];
        os_taskq_accept(8, tq);
        os_time_dly(10);
    }

    log_info("[VREC] worker: pre-stop diag: recording=%d worker_stop=%d paused=%d"
             " frame_timer=0x%x cam_handle=0x%x total_frames=%u seg=%u\n",
             g_rec_ctx.recording, g_rec_ctx.worker_stop, g_rec_ctx.paused,
             g_rec_ctx.frame_timer, (u32)g_camera_handle,
             g_rec_ctx.total_frames, g_rec_ctx.total_segments);

    log_info("[VREC] worker: stop requested, stopping...\n");

    /* frame_timer 由 app_core 在 video_rec_stop() 中统一删除，worker 不再删除 */
    /* 录制中或暂停中都要停 camera (真暂停时 AVI 仍打开, 不能漏关) */
    if (g_camera_handle != NULL && (g_rec_ctx.recording || g_rec_ctx.paused)) {
        app_camera_rec_stop(g_camera_handle);
    }
    //停止后重建媒体索引: 目录 diff 全量入库, app_camera_rec_stop 已同步关 AVI(管线完全停止)
    flm_rebuild_async();
    g_rec_ctx.recording = 0;
    g_rec_ctx.worker_stop = 0;

    /* 500ms UI 定时器由 worker 线程内 transition(RECORDING) 添加, 回调投递到
     * worker 线程 taskq。必须在 worker 线程退出前删除, 否则线程销毁时残留 timer
     * 触发 "timer func no del" assert (timer.c:681)。stop/pause 里的 transition
     * 只负责切换状态, 不再重复删除 (此时 s_rec_ui_timer 已清零)。 */
    if (s_rec_ui_timer) {
        sys_timer_del(s_rec_ui_timer);
        log_info("[VREC] ui_timer stopped, id=%d\n", s_rec_ui_timer);
        s_rec_ui_timer = 0;
    }

    /* 最终状态(停止→IDLE / 暂停→PAUSED)由调用方 stop()/pause() 在 thread_kill
     * 返回后通过 transition 决定, worker 退出路径只做底层清理, 不再自行切状态。 */

    /* 停止录像不再弹"保存完成"提示(用户要求静默停止)。AVI 文件已由
     * app_camera_rec_stop() 关闭保存, 无需 UI 打扰。 */

    log_info("[VREC] worker: stopped, total_frames=%u, segments=%u\n",
             g_rec_ctx.total_frames, g_rec_ctx.total_segments);


    /* 退出前排空 taskq, 防遗留事件触发 assert */
    {
        int drain[8];
        int n = 0;
        while (os_taskq_accept(8, drain) == OS_TASKQ && n++ < 64) {
        }
    }
}
#endif /* TCFG_CAMERA_DVP_ENABLE */

#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
/* tentative definition: 下方 UVC 块内有初始化定义(=0)。这里前置声明供本文件前部的
 * video_rec_build_file_path() 按 H264/JPEG 区分录像文件后缀(.MP4/.AVI)。 */
static u8 g_uvc_is_h264;
#endif

/* 生成录像文件名: Mode0=RTC/boot-time 时间+file_index 段号, Mode1=固定 VID_****.AVI */
void video_rec_build_file_path(char *out_path, int out_size)
{

    struct sys_time t;
    u32 fy = 2025, fm = 1, fd = 1, fh = 0, fmn = 0, fs = 0;
    u32 boot_sec = jiffies_msec() / 1000u;
    u32 yy, mo, dd, hh, mm, ss;

    if (g_rtc_available) {
        rtc_read_time(&t);
        yy = t.year;
        mo = t.month;
        dd = t.day;
        hh = t.hour;
        mm = t.min;
        ss = t.sec;
    } else {
        yy = fy;
        mo = fm;
        dd = fd;
        hh = (boot_sec / 3600u) % 24u;
        mm = (boot_sec / 60u)   % 60u;
        ss =  boot_sec          % 60u;
    }

    if (VIDEO_REC_NAMING_MODE == 1) {
        /* H264 直通录像封装成 MP4, 固定名后缀用 .MP4; JPEG/DVP 用 .AVI */
        const char *fixed_name = VIDEO_REC_FIXED_NAME_FMT;
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
        if (g_uvc_is_h264) {
            fixed_name = "VID_****.MP4";
        }
#endif
        snprintf(out_path, out_size, "%s%s", CONFIG_REC_PATH, fixed_name);
        g_rec_ctx.current_file_create_time = jiffies_msec() / 1000u;
        log_info("[VREC] build path (FIXED mode=1): %s\n", out_path);
    } else {
        const char *ext = VIDEO_REC_FILE_EXT;
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
        if (g_uvc_is_h264) {
            ext = ".MP4";
        }
#endif
        snprintf(out_path, out_size,
                 CONFIG_REC_PATH VIDEO_REC_FILE_PREFIX "%04d%02d%02d_%02d%02d%02d_%03d%s",
                 (int)yy, (int)mo, (int)dd, (int)hh, (int)mm, (int)ss,
                 g_rec_ctx.file_index % 1000, ext);
        g_rec_ctx.current_file_create_time = hh * 3600 + mm * 60 + ss;
        log_info("[VREC] build path: %s (index=%u, rtc=%s, boot_sec=%u)\n",
                 out_path, g_rec_ctx.file_index,
                 g_rtc_available ? "OK" : "FALLBACK", boot_sec);
    }

    /* FILE_TIME_USER_DEFINE_ENABLE=1 时, f_GetTimeDate() 从 datetime_info 取值.
     * 每次创建新文件前更新 datetime_info, 使 FAT 目录项时间戳与当前时间一致. */
    fat_set_datetime_info((u16)yy, (u8)mo, (u8)dd, (u8)hh, (u8)mm, (u8)ss);
}

#if TCFG_CAMERA_DVP_ENABLE
/* 提取 worker 启动: 注册分段事件回调 + fork rec_worker (start 与 resume 共用) */
static int video_rec_launch_worker(void)
{
    app_camera_ret_t ehr;

    /* 分段回调须先于 app_camera_rec_start 注册, 防首段 CYC_FILE 丢失 */
    ehr = app_camera_register_event_handler(g_camera_handle, NULL, video_rec_event_handler);
    if (ehr != APP_CAMERA_OK) {
        log_error("[VREC] register event handler failed: %d (continue, cyc may not work)\n", ehr);
    } else {
        log_info("[VREC] MPI event handler registered\n");
    }

    g_rec_ctx.worker_stop = 0;
    g_rec_ctx.worker_pid = 0;
    /* 新一次录像: 清上次会话残留的待删标志与边界标志; 段号基数=已有段数, 段号跨维护/重开继续 */
    g_rec_need_delete_old = 0;
    g_rec_cyc_boundary = 0;
    g_rec_maint_pending = 0;
    g_rec_cyc_base = g_rec_ctx.total_segments;

    video_rec_log_mem("before_worker_create");

    int err = thread_fork("rec_worker",
                          VIDEO_REC_WORKER_PRIO,
                          VIDEO_REC_WORKER_STACK,
                          64,
                          &g_rec_ctx.worker_pid,
                          video_rec_worker,
                          NULL);
    if (err != 0) {
        log_error("[VREC] failed to create worker thread: %d\n", err);
        g_rec_ctx.worker_pid = 0;  /* thread_fork 失败时不写 pid, 这里兜底 */
        video_rec_log_mem("worker_create_FAIL");
        return -1;
    }

    os_time_dly(20);
    video_rec_log_mem("after_worker_create");
    video_rec_log_task_stack("after_worker_create", "rec_worker");

    log_info("[VREC] worker thread forked\n");
    return 0;
}
#endif /* TCFG_CAMERA_DVP_ENABLE */

int video_rec_start(void)
{
    log_info("[VREC] video_rec_start\n");
    g_sd_auto_start_pending = 0;  /* 任意路径开录: 清 pending 防 DONE 重触自动开录 */
    g_rec_ctx.low_bat_alert_shown = 0;  /* 新录像: 复位低电量弹窗标志 */
    /* 新一段录像必须清累计暂停时长, 否则 elapsed 计算会减出负值 -> UI 时间溢出 */
    g_rec_ctx.pause_start_ms = 0;
    g_rec_ctx.pause_accum_ms = 0;

    if (g_rec_ctx.recording || g_rec_ctx.paused) {
        log_error("[VREC] already recording/paused\n");
        video_rec_ui_show_alert(2);
        return -1;
    }
#if TCFG_CAMERA_DVP_ENABLE
    if (!g_camera_handle) {
        log_error("[VREC] camera not ready\n");
        video_rec_ui_show_alert(2);
        return -1;
    }
#endif

    if (g_rec_ctx.loop_time_sec == 0) {
        g_rec_ctx.loop_time_sec = LOOP_TIME_DEFAULT;
    }

    if (flm_sd_is_exfat()) {
        log_error("[VREC] exFAT card not supported, please use FAT32\n");
        video_rec_ui_show_alert(3);  /* 请使用FAT32格式的SD卡 */
        return -1;
    }

    int stor_ret = video_rec_check_storage();
    if (stor_ret == -2) {
        log_error("[VREC] SD card not online\n");
        video_rec_ui_show_alert(1);
        return -1;
    }
    if (stor_ret != 0) {
        log_error("[VREC] storage full\n");
        video_rec_ui_show_alert(0);
        return -1;
    }


    /* Phase 1: 文件数量限制 (循环覆盖) */
    if (video_rec_check_file_count() != 0) {
        log_error("[VREC] file count limit reached, cannot start\n");
        video_rec_ui_show_alert(0);
        return -1;
    }

    video_rec_build_file_path(g_rec_ctx.current_file_path,
                              sizeof(g_rec_ctx.current_file_path));

#if TCFG_CAMERA_DVP_ENABLE
    if (video_rec_launch_worker() != 0) {
        video_rec_ui_show_alert(2);
        return -1;
    }

    /* frame_timer 统一挂在 app_core，回调在 app_core 执行，避免 worker/app_core 跨线程操作定时器链表 */
    vrec_timer_lock();
    if (g_rec_ctx.frame_timer == 0) {
        g_rec_ctx.frame_timer = sys_timer_add_2_task(NULL, video_rec_frame_tick, 1000 / VIDEO_REC_FPS, "app_core");
        log_info("[VREC] frame_timer started on app_core, id=%d\n", g_rec_ctx.frame_timer);
    }
    vrec_timer_unlock();

    return 0;
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    /* UVC 直通录像: 直接用摄像头实际分辨率封装原始 JPEG 帧, 忽略菜单档位。
     * 原始 JPEG 帧就是摄像头输出的尺寸, AVI 头必须与帧数据一致。 */
    u16 rec_w = g_uvc_width;
    u16 rec_h = g_uvc_height;
    if (vrec_uvc_rec_start(g_rec_ctx.current_file_path, rec_w, rec_h) != 0) {
        log_error("[VREC] uvc rec start failed\n");
        video_rec_ui_show_alert(2);
        return -1;
    }
    g_rec_ctx.record_start_time = sys_timer_get_ms() / 1000;
    g_rec_ctx.total_segments++;
    video_rec_transition_to(VIDEO_STATE_RECORDING);
    log_info("[VREC] uvc recording started: %s %dx%d\n",
             g_rec_ctx.current_file_path, rec_w, rec_h);
    return 0;
#endif
}

int video_rec_stop(void)
{
    log_info("[VREC] video_rec_stop\n");

    /* frame_timer 统一在 app_core 删除；即使状态已因异常提前置 IDLE，也确保不泄漏 */
    if (g_rec_ctx.frame_timer) {
        vrec_timer_lock();
        sys_timer_del(g_rec_ctx.frame_timer);
        g_rec_ctx.frame_timer = 0;
        vrec_timer_unlock();
    }

    if (!g_rec_ctx.recording && !g_rec_ctx.paused) {
        return 0;
    }

#if TCFG_CAMERA_DVP_ENABLE
    g_rec_ctx.worker_stop = 1;

    if (g_rec_ctx.worker_pid) {
        log_info("[VREC] thread_kill(rec_worker, KILL_WAIT) pid=%d\n",
                 g_rec_ctx.worker_pid);
        thread_kill(&g_rec_ctx.worker_pid, KILL_WAIT);
        /* thread_kill 成功回收后会把 pid 清零; 兜底再清一次. */
        g_rec_ctx.worker_pid = 0;
        log_info("[VREC] worker exited, total_frames=%u, segments=%u\n",
                 g_rec_ctx.total_frames, g_rec_ctx.total_segments);
    } else {
        /* 异常分支: recording/paused=1 但 worker_pid=0, 说明上一次清理未完成
         * 或从未成功 start. 不调 thread_kill (pid=0 传给 OS 会出问题), 直接
         * 进入 IDLE 状态机恢复. */
        log_error("[VREC] video_rec_stop: worker_pid=0 (orphaned), skip thread_kill\n");
    }

    /* 清零 worker_stop (worker 自己也会清，这里做兜底) */
    g_rec_ctx.worker_stop = 0;
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    vrec_uvc_rec_stop();
    //停止后重建媒体索引: 目录 diff 全量入库, vrec_uvc_rec_stop 已同步 AVI_close(管线完全停止)
    flm_rebuild_async();
#endif

    video_rec_transition_to(VIDEO_STATE_IDLE);
    return 0;
}

int video_rec_pause(void)
{
#if TCFG_CAMERA_DVP_ENABLE
    log_info("[VREC] pause\n");
    if (!g_rec_ctx.recording) {
        log_error("[VREC] pause: not recording\n");
        return -1;
    }
    if (g_rec_ctx.paused) {
        log_info("[VREC] already paused\n");
        return 0;
    }
    if (!g_camera_handle) {
        log_error("[VREC] pause: camera not ready\n");
        return -1;
    }
    /* 真暂停: 编码链路与 AVI 文件保持打开, 只丢弃暂停期帧; 恢复后同一文件续写 */
    if (app_camera_rec_pause(g_camera_handle) != APP_CAMERA_OK) {
        log_error("[VREC] pause: app_camera_rec_pause failed\n");
        video_rec_ui_show_alert(2);
        return -1;
    }
    g_rec_ctx.pause_start_ms = sys_timer_get_ms();

    if (g_rec_ctx.frame_timer) {
        vrec_timer_lock();
        sys_timer_del(g_rec_ctx.frame_timer);
        g_rec_ctx.frame_timer = 0;
        vrec_timer_unlock();
    }
    g_rec_ctx.worker_stop = 0;

    video_rec_transition_to(VIDEO_STATE_PAUSED);

    log_info("[VREC] paused at frame %u, same file continues on resume\n", g_rec_ctx.total_frames);
    return 0;
#else
    /* UVC 基本录像不支持暂停 */
    log_error("[VREC] pause not supported\n");
    return -1;
#endif
}

int video_rec_resume(void)
{
#if TCFG_CAMERA_DVP_ENABLE
    log_info("[VREC] resume\n");
    if (g_rec_ctx.recording) {
        log_info("[VREC] already recording\n");
        return 0;
    }
    if (!g_rec_ctx.paused) {
        log_error("[VREC] resume: not paused\n");
        return -1;
    }
    if (!g_camera_handle) {
        log_error("[VREC] resume: camera not ready\n");
        return -1;
    }
    /* 同一文件续录: AVI 从未关闭, 这里只恢复写帧, 不重开编码链路 */
    if (app_camera_rec_resume(g_camera_handle) != APP_CAMERA_OK) {
        log_error("[VREC] resume: app_camera_rec_resume failed\n");
        video_rec_ui_show_alert(2);
        return -1;
    }

    /* 累计暂停时长: UI 计时扣除, 恢复后时间从暂停处继续, 不跳变 */
    g_rec_ctx.pause_accum_ms += sys_timer_get_ms() - g_rec_ctx.pause_start_ms;

    vrec_timer_lock();
    g_rec_ctx.frame_timer = sys_timer_add_2_task(NULL, video_rec_frame_tick, 1000 / VIDEO_REC_FPS, "app_core");
    vrec_timer_unlock();

    video_rec_transition_to(VIDEO_STATE_RECORDING);

    /* 恢复后立即把计时刷新到暂停前位置, 避免 500ms 首跳后才更新 */
    video_rec_ui_update_timer(video_rec_get_elapsed_sec());

    log_info("[VREC] resumed, same file continues\n");
    return 0;
#else
    log_error("[VREC] resume not supported\n");
    return -1;
#endif
}

enum video_rec_state video_rec_get_state(void)
{
    if (g_rec_ctx.current_state != 0) {
        return (enum video_rec_state)g_rec_ctx.current_state;
    }
    if (g_rec_ctx.paused) {
        return VIDEO_STATE_PAUSED;
    }
    if (g_rec_ctx.recording) {
        return VIDEO_STATE_RECORDING;
    }
    return VIDEO_STATE_IDLE;
}

u32 video_rec_get_total_frames(void)
{
    return g_rec_ctx.total_frames;
}

u32 video_rec_get_total_segments(void)
{
    return g_rec_ctx.total_segments;
}

/* 状态切换统一走 video_rec_transition_to(), 防分散赋值漏 UI 更新/计时器启停 */

/* 500ms 周期回调 - 仅在 RECORDING 状态下被 sys_timer 调度 */
void video_rec_ui_tick_500ms(void *priv)
{
    (void)priv;
    /* 防御性检查：若 state 在 timer 触发前已切走 (race window)，
     * 不再推送 elapsed_sec 给 UI。 */
    if ((enum video_rec_state)g_rec_ctx.current_state == VIDEO_STATE_RECORDING) {
        u32 elapsed = video_rec_get_elapsed_sec();
        video_rec_ui_update_timer(elapsed);

        /* 电量<10% 自动停录+弹窗, 复用 500ms timer, low_bat_alert_shown 防重复 */
#if 0
        if (!g_rec_ctx.low_bat_alert_shown && get_vbat_percent() < VIDEO_REC_LOW_BAT_PERCENT) {
            log_error("[VREC] low battery %d%% < %d%%, auto stop\n",
                      get_vbat_percent(), VIDEO_REC_LOW_BAT_PERCENT);
            g_rec_ctx.low_bat_alert_shown = 1;
            video_rec_stop();
            video_rec_transition_to(VIDEO_STATE_ERROR);
            video_rec_ui_show_alert(2);  /* type=2: 电量低于10% */
        }
#endif
    }

}

void video_rec_transition_to(enum video_rec_state new_state)
{
    enum video_rec_state old_state =
        (g_rec_ctx.current_state != 0)
        ? (enum video_rec_state)g_rec_ctx.current_state
        : VIDEO_STATE_IDLE;

    if (new_state == old_state) {
        return;  /* 同一状态无需切换，幂等 */
    }

    /* 1. 原子地更新权威字段 */
    g_rec_ctx.current_state = (u32)new_state;

    /* 2. 同步 recording/paused 位 (旧代码读这两个位仍可工作) */
    switch (new_state) {
    case VIDEO_STATE_IDLE:
        g_rec_ctx.recording = 0;
        g_rec_ctx.paused    = 0;
        break;
    case VIDEO_STATE_RECORDING:
        g_rec_ctx.recording = 1;
        g_rec_ctx.paused    = 0;
        break;
    case VIDEO_STATE_PAUSED:
        g_rec_ctx.recording = 0;
        g_rec_ctx.paused    = 1;
        break;
    case VIDEO_STATE_SAVING:
    case VIDEO_STATE_ERROR:
    default:
        g_rec_ctx.recording = 0;
        g_rec_ctx.paused    = 0;
        break;
    }

    log_info("[VREC] state %d -> %d\n", (int)old_state, (int)new_state);

    /* 3. 通知 UI 层 */
    video_rec_ui_on_state_change(old_state, new_state);

    /* 4. 启停 500ms 计时器 - 统一投递到 app_core 执行，确保 timer 只挂在 app_core */
    app_send_message(APP_MSG_VIDEO_REC_UI_TIMER_SYNC, new_state);
}

/* app_core 线程执行：录像 UI 500ms 定时器启停 */
void video_rec_ui_timer_sync(int new_state)
{
    vrec_timer_lock();
    if (new_state == VIDEO_STATE_RECORDING) {
        if (s_rec_ui_timer == 0) {
            s_rec_ui_timer = sys_timer_add_2_task(NULL, video_rec_ui_tick_500ms, 500, "app_core");
            log_info("[VREC] ui_timer started on app_core, id=%d\n", s_rec_ui_timer);
        }
    } else {
        if (s_rec_ui_timer) {
            sys_timer_del(s_rec_ui_timer);
            log_info("[VREC] ui_timer stopped on app_core, id=%d\n", s_rec_ui_timer);
            s_rec_ui_timer = 0;
        }
    }
    vrec_timer_unlock();
}

/*==============================================================================
 * 消息处理
 *==============================================================================
 */
static void video_app_msg_handler(int *msg)
{
    int evt = msg[0];
    int arg = msg[1];
    log_info("APP msg: 0x%x, 0x%x\n", evt, arg);

    /* SD0 插卡(空闲)自动开始录像, 拔卡停录+ERROR 等待态.
     * 必须确认录像模式仍是当前 app 模式: 左键返回后 UI 已弹回主页,
     * 若模式切换消息尚未处理完, 本循环还可能短暂收到 SD 事件, 不能在此自动录像 */
    if (evt == APP_MSG_SD_EVENT) {
        if (!app_in_mode(APP_MODE_VIDEO_REC)) {
            log_info("[VREC] SD event ignored: rec mode not active\n");
            return;
        }
        enum video_rec_state st = video_rec_get_state();
        if (arg == DEVICE_EVENT_IN) {
            if (st == VIDEO_STATE_ERROR) {
                /* 拔卡等待态: 重插卡回 IDLE, 与空闲插卡同路径 */
                video_rec_transition_to(VIDEO_STATE_IDLE);
                st = VIDEO_STATE_IDLE;
            }
            if (st == VIDEO_STATE_IDLE) {
                /* 拔卡抖动会来假 IN, 查卡在线才 auto start */
                if (flm_sd_online() != 0) {
                    log_info("[VREC] SD IN but card not ready, skip auto start\n");
                } else if (flm_rebuild_in_progress()) {
                    /* 重建进行中: 挂起自动开录, 等 APP_MSG_FLM_DB_CHANGED, 防并发抢 SD 读与内存 */
                    g_sd_auto_start_pending = 1;
                    g_sd_auto_start_pending_ms = sys_timer_get_ms();
                    log_info("[VREC] SD IN -> rebuild in progress, defer auto start\n");
                } else {
                    log_info("[VREC] SD IN -> auto start\n");
                    video_rec_start();
                }
            } else {
                log_info("[VREC] SD IN: state=%d, skip auto start\n", (int)st);
            }
        } else if (arg == DEVICE_EVENT_OUT) {
            log_info("[VREC] SD OUT -> stop + ERROR + alert\n");
            g_sd_auto_start_pending = 0;  /* 拔卡: 清 pending 防后续 DONE 误触开录 */
            if (g_rec_ctx.recording || g_rec_ctx.paused) {
                video_rec_stop();
            }
            /* 等待态: ERROR. 不再 auto-start, 直到 SD 重新插入触发 DEVICE_EVENT_IN. */
            video_rec_transition_to(VIDEO_STATE_ERROR);
            /* 弹窗: type=1 = "请插入SD卡" */
            video_rec_ui_show_alert(1);
        } else {
            log_info("[VREC] SD_EVENT: unknown arg=%d\n", arg);
        }
        return;
    }
}

/* SD IN 自动开录统一出口: 清 pending 并复检模式/卡在线/状态后才开录 */
static void video_rec_sd_auto_start_flush(void)
{
    g_sd_auto_start_pending = 0;
    if (!app_in_mode(APP_MODE_VIDEO_REC)) {
        return;
    }
    if (flm_sd_online() != 0) {
        log_info("[VREC] SD gone before auto start, cancel pending\n");
        return;
    }
    if (video_rec_get_state() != VIDEO_STATE_IDLE) {
        log_info("[VREC] state not idle, cancel pending auto start\n");
        return;
    }
    log_info("[VREC] SD IN -> auto start\n");
    video_rec_start();
}

/* APP_MSG_FLM_DB_CHANGED 补开录: 仅 pending 时生效, 停录触发的重建 DONE 不误触 */
int video_rec_flm_rebuild_done(void)
{
    if (!g_sd_auto_start_pending) {
        return 0;
    }
    log_info("[VREC] APP_MSG_FLM_DB_CHANGED: flush pending auto start\n");
    video_rec_sd_auto_start_flush();
    return 0;
}

/*==============================================================================
 * UVC Camera 路径 (USB Host 外接摄像头) — JPEG 录像
 *
 *   帧回调 -> 双帧缓冲池 -> pipeline 线程:
 *     VDEC(JPEG) 解码 -> VPSS(display chn 预览 + rec chn 录像) -> VENC(JPEG) -> AVI
 *   参考 apps/watch/wifi/stream/mpi_video_net.c 的 JPEG 录像 + DVP 录像的
 *   VPSS rec chn(ONLINE) bind VENC(ONLINE) 做法。
 * ============================================================================ */
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)

#include "mp4_muxer.h"
#include "mp4_file_io.h"

/* libmov keyframe flag (from mov-format.h) */
#ifndef MOV_AV_FLAG_KEYFRAME
#define MOV_AV_FLAG_KEYFRAME  0x0001
#endif

#define VREC_UVC_BUF_SIZE        (128 * 1024)
#define VREC_UVC_BUF_POOL_COUNT  2
#define VREC_UVC_VDEC_TIMEOUT_MS 5000
#define VREC_UVC_VPSS_TIMEOUT_MS 5000

extern u8   uvc_host_is_h264_format(void);
extern void uvc_host_set_target_reso(u16 width, u16 height);

/* ---- 双帧缓冲池 ---- */
typedef struct {
    u8              *data;
    volatile u32    len;
    u32             capacity;
    volatile u8     locked;
    volatile u8     ready;
    volatile u8     is_h264;
} vrec_uvc_frame_buf_t;

typedef struct {
    vrec_uvc_frame_buf_t bufs[VREC_UVC_BUF_POOL_COUNT];
    OS_SEM          frame_sem;
    volatile u32    frame_cnt;
    volatile u32    drop_cnt;
} vrec_uvc_buf_pool_t;

/* ---- VDEC ---- */
typedef struct {
    mpi_vdec_chn_t  chn;
    u8              created, started;
    OS_SEM          sem;
    volatile int    done;
} vrec_uvc_vdec_t;

/* ---- VPSS(display chn bind VO) ---- */
typedef struct {
    mpi_vpss_grp_t  grp;
    mpi_vpss_chn_t  disp_chn;
    mpi_vo_chn_t    vo_chn;
    u8              created, started;
    u8              disp_enabled;
} vrec_uvc_vpss_t;

static vrec_uvc_buf_pool_t *g_uvc_pool = NULL;
static vrec_uvc_vdec_t      g_uvc_vdec;
static vrec_uvc_vpss_t      g_uvc_vpss;
static avi_t               *g_uvc_avi = NULL;
static int    g_uvc_pipeline_pid = 0;
static volatile u8 g_uvc_exit       = 0;
static volatile u8 g_uvc_running    = 0;
static volatile u8 g_uvc_rec_active = 0;
static u16   g_uvc_rec_w  = 640;
static u16   g_uvc_rec_h  = 480;

/* H264 直通状态 (UVC H264 输入源时: 不显示, 录像直通 MP4, 拍照软解码转 JPEG) */
static u8    g_uvc_is_h264 = 0;
static mp4_muxer_t *g_uvc_mov  = NULL;
static u32   g_uvc_mov_frames  = 0;
static u32   g_uvc_mov_fps     = 30;  /* 固定 30fps (mov_rec_init 里赋值 VIDEO_REC_FPS) */
static u8    g_uvc_mov_avcc[256 + 128 + 16];  /* SPS+PPS concatenated for avcC */
static u32   g_uvc_mov_avcc_len = 0;
static u8    g_uvc_h264_wait_idr = 0;   /* 0=idle, 1=waiting IDR, 2=writing */

/* ---- 帧回调 (UVC ISO 接收线程上下文) ---- */
static void vrec_uvc_frame_cb(const u8 *data, u32 len, u8 is_h264)
{
    vrec_uvc_buf_pool_t *p = g_uvc_pool;
    vrec_uvc_frame_buf_t *t = NULL;
    int i;

    if (!p || len == 0) {
        return;
    }
    p->frame_cnt++;

    for (i = 0; i < VREC_UVC_BUF_POOL_COUNT; i++) {
        if (!p->bufs[i].locked) {
            t = &p->bufs[i];
            break;
        }
    }
    if (!t) {
        p->drop_cnt++;
        return;
    }
    if (len > t->capacity) {
        u8 *n = (u8 *)realloc(t->data, len);
        if (!n) {
            p->drop_cnt++;
            return;
        }
        t->data = n;
        t->capacity = len;
    }
    memcpy(t->data, data, len);
    t->len = len;
    t->is_h264 = is_h264;
    t->ready = 1;
    os_sem_post(&p->frame_sem);
}

static int vrec_uvc_buf_pool_init(vrec_uvc_buf_pool_t **pp)
{
    vrec_uvc_buf_pool_t *p = (vrec_uvc_buf_pool_t *)calloc(1, sizeof(*p));
    int i;

    if (!p) {
        return -1;
    }
    os_sem_create(&p->frame_sem, 0);
    for (i = 0; i < VREC_UVC_BUF_POOL_COUNT; i++) {
        p->bufs[i].data = (u8 *)malloc(VREC_UVC_BUF_SIZE);
        if (!p->bufs[i].data) {
            goto err;
        }
        p->bufs[i].capacity = VREC_UVC_BUF_SIZE;
    }
    g_uvc_pool = p;
    *pp = p;
    uvc_host_frame_cb_register(vrec_uvc_frame_cb);
    printf("[VREC_UVC] buf pool: %d x %d bytes\n", VREC_UVC_BUF_POOL_COUNT, VREC_UVC_BUF_SIZE);
    return 0;
err:
    for (i = 0; i < VREC_UVC_BUF_POOL_COUNT; i++) if (p->bufs[i].data) {
            free(p->bufs[i].data);
        }
    os_sem_del(&p->frame_sem, OS_DEL_ALWAYS);
    free(p);
    return -1;
}

static void vrec_uvc_buf_pool_deinit(vrec_uvc_buf_pool_t *p)
{
    int i;
    if (!p) {
        return;
    }
    uvc_host_frame_cb_register(NULL);
    os_sem_del(&p->frame_sem, OS_DEL_ALWAYS);
    for (i = 0; i < VREC_UVC_BUF_POOL_COUNT; i++) if (p->bufs[i].data) {
            free(p->bufs[i].data);
        }
    free(p);
    g_uvc_pool = NULL;
}

static int vrec_uvc_buf_pool_get(vrec_uvc_buf_pool_t *p, u8 **data, u32 *len,
                                 u8 *is_h264, int to_ms)
{
    int i;
    if (os_sem_pend(&p->frame_sem, to_ms) != 0) {
        return -1;
    }
    for (i = 0; i < VREC_UVC_BUF_POOL_COUNT; i++) {
        if (p->bufs[i].ready && !p->bufs[i].locked) {
            p->bufs[i].locked = 1;
            p->bufs[i].ready = 0;
            *data = p->bufs[i].data;
            *len = p->bufs[i].len;
            if (is_h264) {
                *is_h264 = p->bufs[i].is_h264;
            }
            return 0;
        }
    }
    return -1;
}

static void vrec_uvc_buf_pool_unlock(vrec_uvc_buf_pool_t *p, u8 *data)
{
    int i;
    for (i = 0; i < VREC_UVC_BUF_POOL_COUNT; i++)
        if (p->bufs[i].data == data) {
            p->bufs[i].locked = 0;
            return;
        }
}

/* ---- VDEC (JPEG, OFFLINE 非阻塞) ---- */
static void vrec_uvc_vdec_notify(void *priv, const mpi_stream_t *s, mpi_ret_t ret)
{
    vrec_uvc_vdec_t *c = (vrec_uvc_vdec_t *)priv;
    (void)s;
    c->done = (ret == MPI_SUCCESS) ? 1 : -1;
    os_sem_post(&c->sem);
}

static int vrec_uvc_vdec_create(vrec_uvc_vdec_t *c)
{
    mpi_ret_t r;
    mpi_vdec_chn_attr_t a;

    memset(c, 0, sizeof(*c));
    os_sem_create(&c->sem, 0);
    r = mpi_vdec_find_free_chn(&c->chn);
    if (r) {
        return -1;
    }
    memset(&a, 0, sizeof(a));
    a.hw_dev = 0;
    a.type = MPI_VDEC_TYPE_JPEG;
    a.out_yuv_order = MPI_YUV_ORDER_YUYV;
    a.output_mode = MPI_VDEC_OUTPUT_FRAME;
    a.frame_buf_size = (u32)g_uvc_width * g_uvc_height * 2 + 1024;
    r = mpi_vdec_create_chn(c->chn, &a);
    if (r) {
        return -1;
    }
    c->created = 1;
    mpi_vdec_set_block_mode(c->chn, 0, vrec_uvc_vdec_notify, c);
    r = mpi_vdec_start_chn(c->chn);
    if (r) {
        return -1;
    }
    c->started = 1;
    printf("[VREC_UVC] vdec chn=%d JPEG\n", c->chn);
    return 0;
}

static int vrec_uvc_vdec_decode(vrec_uvc_vdec_t *c, u8 *data, u32 len,
                                mpi_video_frame_info_t *f, int to_ms)
{
    mpi_stream_t s;
    int ret;

    c->done = 0;
    os_sem_set(&c->sem, 0);
    memset(&s, 0, sizeof(s));
    s.addr = data;
    s.len = len;
    s.type = MPI_STREAM_TYPE_JPEG;
    ret = mpi_vdec_send_stream(c->chn, &s);
    if (ret) {
        return -1;
    }
    ret = os_sem_pend(&c->sem, to_ms);
    if (ret || c->done <= 0) {
        return -1;
    }
    memset(f, 0, sizeof(*f));
    ret = mpi_vdec_get_frame(c->chn, f, VREC_UVC_VDEC_TIMEOUT_MS);
    if (ret) {
        return -1;
    }
    return 0;
}

static void vrec_uvc_vdec_destroy(vrec_uvc_vdec_t *c)
{
    if (c->started) {
        mpi_vdec_stop_chn(c->chn);
    }
    if (c->created) {
        mpi_vdec_destroy_chn(c->chn);
    }
    os_sem_del(&c->sem, OS_DEL_ALWAYS);
    memset(c, 0, sizeof(*c));
}

/* ---- VPSS(display chn bind VO) + VO ---- */
static int vrec_uvc_vpss_create(vrec_uvc_vpss_t *c, u16 w, u16 h, u16 disp_w, u16 disp_h)
{
    mpi_ret_t r;
    mpi_vpss_grp_attr_t ga;
    mpi_vpss_chn_attr_t ca;
    mpi_vo_chn_attr_t vo_attr;
    mpi_chn_t vpss_src, vo_dst;

    memset(c, 0, sizeof(*c));
    c->grp = -1;
    c->disp_chn = -1;

    r = mpi_vpss_find_free_grp(&c->grp);
    if (r) {
        return -1;
    }
    memset(&ga, 0, sizeof(ga));
    ga.src_width = w;
    ga.src_height = h;
    ga.src_format = MPI_VPSS_SRC_FMT_YUV;
    ga.src_pixel_format = MPI_PIXEL_FMT_YUV422_PACKED;
    ga.src_yuv_order = MPI_YUV_ORDER_YUYV;
    r = mpi_vpss_create_grp(c->grp, &ga);
    if (r) {
        return -1;
    }
    c->created = 1;
    r = mpi_vpss_start_grp(c->grp);
    if (r) {
        return -1;
    }
    c->started = 1;

    /* display chn -> VO -> fb5 */
    r = mpi_vpss_find_free_chn(c->grp, &c->disp_chn);
    if (r) {
        return -1;
    }
    memset(&ca, 0, sizeof(ca));
    ca.width = disp_w;
    ca.height = disp_h;
    ca.format = MPI_PIXEL_FMT_YUV420SP;
    ca.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    ca.hw_id = MPI_VPSS_HW_IMC_DST1;
    r = mpi_vpss_create_chn(c->grp, c->disp_chn, &ca);
    if (r) {
        return -1;
    }
    r = mpi_vpss_enable_chn(c->grp, c->disp_chn);
    if (r) {
        return -1;
    }
    c->disp_enabled = 1;

    r = mpi_vo_find_free_chn(&c->vo_chn);
    if (r) {
        return -1;
    }
    memset(&vo_attr, 0, sizeof(vo_attr));
    strncpy(vo_attr.fb_name, "fb5", sizeof(vo_attr.fb_name) - 1);
    vo_attr.fb_num = 2;
    vo_attr.fb_width = disp_w;
    vo_attr.fb_height = disp_h;
    vo_attr.window_width = disp_w;
    vo_attr.window_height = disp_h;
    vo_attr.fb_format = FB_COLOR_FORMAT_YUV420P;
    vo_attr.combine = 1;
    r = mpi_vo_create_chn(c->vo_chn, &vo_attr);
    if (r) {
        return -1;
    }
    r = mpi_vo_start_chn(c->vo_chn);
    if (r) {
        return -1;
    }
    vpss_src.mod_id = MPI_MOD_ID_VPSS;
    vpss_src.dev_id = c->grp;
    vpss_src.chn_id = c->disp_chn;
    vo_dst.mod_id = MPI_MOD_ID_VO;
    vo_dst.dev_id = 0;
    vo_dst.chn_id = c->vo_chn;
    r = mpi_sys_bind(&vpss_src, &vo_dst);
    if (r) {
        return -1;
    }

    printf("[VREC_UVC] vpss grp=%d disp=%d %dx%d->%dx%d\n",
           c->grp, c->disp_chn, w, h, disp_w, disp_h);
    return 0;
}

static void vrec_uvc_vpss_destroy(vrec_uvc_vpss_t *c)
{
    mpi_chn_t vpss_src, vo_dst;

    if (c->disp_enabled) {
        vpss_src.mod_id = MPI_MOD_ID_VPSS;
        vpss_src.dev_id = c->grp;
        vpss_src.chn_id = c->disp_chn;
        vo_dst.mod_id = MPI_MOD_ID_VO;
        vo_dst.dev_id = 0;
        vo_dst.chn_id = c->vo_chn;
        mpi_sys_unbind(&vpss_src, &vo_dst);
        mpi_vo_stop_chn(c->vo_chn);
        mpi_vo_destroy_chn(c->vo_chn);
        mpi_vpss_disable_chn(c->grp, c->disp_chn);
        mpi_vpss_destroy_chn(c->grp, c->disp_chn);
    }
    if (c->started) {
        mpi_vpss_stop_grp(c->grp);
    }
    if (c->created) {
        mpi_vpss_destroy_grp(c->grp);
    }
    memset(c, 0, sizeof(*c));
}

/* ---- libmov MP4 muxer (UVC H264 直通) ---- */

/* mp4_muxer_t 首个字段是 mp4_file_io_ctx_t file_buffer (见 mp4_muxer.c 的 struct 定义),
 * 用于 mp4_muxer_close 前取 JLFS 占位符解析后的实际文件名入库。 */
typedef struct {
    mp4_file_io_ctx_t file_buffer;
} vrec_mov_muxer_layout_t;

static void mov_rec_init(const char *path)
{
    if (g_uvc_mov) {
        return;
    }
    g_uvc_mov_frames  = 0;
    g_uvc_mov_avcc_len = 0;
    g_uvc_h264_wait_idr = 0;

    /* 禁用 SD 低功耗: 否则写帧间隙 SD 会进低功耗, 下次写要唤醒, 造成丢帧(花屏)
     * 和数据不落盘。对齐 mpi_video_net.c 的做法 */
    {
        char sd_dev[4] = {0};
        const char *sd = strstr(path, "storage/");
        if (sd) {
            sd += 8;
            const char *end = strchr(sd, '/');
            int len = end ? (int)(end - sd) : (int)strlen(sd);
            if (len > 0 && len < 4) {
                memcpy(sd_dev, sd, len);
            }
        }
        if (sd_dev[0]) {
            extern void sdmmc_dev_lowpower_disable(char *sdx, u32 disable);
            sdmmc_dev_lowpower_disable(sd_dev, 1);
        }
    }

    /* 固定 30fps (对齐 VIDEO_REC_FPS)。不再读 uvc_host_get_fps_val():
     * H264 摄像头的 UVC frame interval 描述符不可靠, f_interval=7 时会算出
     * 10000000/7=1428571 这种荒谬值, 使 MP4 时间戳全错 → 花屏 + 时长不对。 */
    g_uvc_mov_fps = VIDEO_REC_FPS;

    g_uvc_mov = mp4_muxer_open(path, 0);
    if (g_uvc_mov) {
        printf("[VREC_UVC][MOV] opened %s @%ufps\n", path, g_uvc_mov_fps);
    } else {
        printf("[VREC_UVC][MOV] open failed: %s\n", path);
    }
}

/* 从 Annex-B H264 提取 SPS(NAL 7) 和 PPS(NAL 8) */
static void mov_extract_sps_pps(const u8 *data, u32 len,
                                u8 *sps, u32 *sps_len,
                                u8 *pps, u32 *pps_len)
{
    u32 off = 0;
    *sps_len = *pps_len = 0;
    while (off + 4 < len) {
        u8 sl;
        if (data[off] == 0 && data[off + 1] == 0 && data[off + 2] == 0 && data[off + 3] == 1) {
            sl = 4;
        } else if (data[off] == 0 && data[off + 1] == 0 && data[off + 2] == 1) {
            sl = 3;
        } else {
            off++;
            continue;
        }
        u8 nt = data[off + sl] & 0x1f;
        u32 nal_start = off + sl;
        u32 nal_end = nal_start;
        while (nal_end + 2 < len &&
               !(data[nal_end] == 0 && data[nal_end + 1] == 0 &&
                 (data[nal_end + 2] == 1 || (nal_end + 3 < len && data[nal_end + 2] == 0 && data[nal_end + 3] == 1)))) {
            nal_end++;
        }
        u32 nal_len = nal_end - nal_start;
        if (nt == 7 && nal_len <= 256) {
            memcpy(sps, data + nal_start, nal_len);
            *sps_len = nal_len;
        }
        if (nt == 8 && nal_len <= 128) {
            memcpy(pps, data + nal_start, nal_len);
            *pps_len = nal_len;
        }
        off = nal_end;
    }
}

/* 由 SPS+PPS 构建 avcC extra_data */
static u32 mov_build_avcc(u8 *out, const u8 *sps, u32 sps_len, const u8 *pps, u32 pps_len)
{
    u8 *p = out;
    *p++ = 1;
    *p++ = sps_len >= 4 ? sps[1] : 66;
    *p++ = sps_len >= 4 ? sps[2] : 0;
    *p++ = sps_len >= 4 ? sps[3] : 30;
    *p++ = 0xFF;
    *p++ = 0xE1;
    *p++ = (u8)(sps_len >> 8);
    *p++ = (u8)(sps_len & 0xFF);
    memcpy(p, sps, sps_len);
    p += sps_len;
    *p++ = 1;
    *p++ = (u8)(pps_len >> 8);
    *p++ = (u8)(pps_len & 0xFF);
    memcpy(p, pps, pps_len);
    p += pps_len;
    return (u32)(p - out);
}

static void mov_rec_write(const u8 *data, u32 len, int flags)
{
    if (!g_uvc_mov || !data || !len) {
        return;
    }

    /* lazy init: 从首帧提取 SPS/PPS, add video track */
    if (g_uvc_mov_avcc_len == 0) {
        u8 sps[256], pps[128];
        u32 sps_len, pps_len;
        mov_extract_sps_pps(data, len, sps, &sps_len, pps, &pps_len);
        if (sps_len && pps_len) {
            g_uvc_mov_avcc_len = mov_build_avcc(g_uvc_mov_avcc, sps, sps_len, pps, pps_len);
            mp4_muxer_add_video(g_uvc_mov, MP4_MUXER_OBJECT_H264,
                                g_uvc_rec_w, g_uvc_rec_h, g_uvc_mov_avcc, g_uvc_mov_avcc_len);
        } else {
            mp4_muxer_add_video(g_uvc_mov, MP4_MUXER_OBJECT_H264,
                                g_uvc_rec_w, g_uvc_rec_h, NULL, 0);
            g_uvc_mov_avcc_len = 1; /* 标记已初始化 */
        }
    }

    u32 ts = (u32)((u64)g_uvc_mov_frames * 1000 / g_uvc_mov_fps);
    /* 直接写 Annex-B 流 (同 mpi_video_net.c 的 mov_rec_write 做法) */
    mp4_muxer_write_video(g_uvc_mov, data, len, ts, ts, flags);
    g_uvc_mov_frames++;
}

static void mov_rec_finish(void)
{
    if (!g_uvc_mov) {
        return;
    }
    FILE *fp = (FILE *)((vrec_mov_muxer_layout_t *)g_uvc_mov)->file_buffer.file;
    if (fp) {
        fget_name(fp, (u8 *)g_last_video_file, sizeof(g_last_video_file));
    }
    mp4_muxer_close(g_uvc_mov);
    g_uvc_mov = NULL;
    printf("[VREC_UVC][MOV] done: %u frames\n", g_uvc_mov_frames);
}

/* ---- 录像 chn(ONLINE) + VENC(JPEG) + AVI ---- */
/* 直通录像: JPEG 时只开 AVI 文件(MJPG), H264 时只开 MP4 muxer。均不解码不编码。 */
static int vrec_uvc_rec_start(const char *path, u16 w, u16 h)
{
    g_uvc_rec_w = w;
    g_uvc_rec_h = h;

    if (g_uvc_is_h264) {
        /* 后缀已由 build_file_path 按 H264 生成 .MP4, 这里直接用 path */
        mov_rec_init(path);
        if (!g_uvc_mov) {
            return -1;
        }
        printf("[VREC_UVC] rec start (H264 passthrough): %s %dx%d\n", path, w, h);
    } else {
        g_uvc_avi = AVI_open_output_file(path);
        if (!g_uvc_avi) {
            printf("[VREC_UVC] AVI open fail: %s\n", path);
            return -1;
        }
        AVI_set_video(g_uvc_avi, w, h, (double)VIDEO_REC_FPS, "MJPG");
        printf("[VREC_UVC] rec start (JPEG passthrough): %s %dx%d\n", path, w, h);
    }

    g_uvc_rec_active = 1;
    return 0;
}

static void vrec_uvc_rec_stop(void)
{
    g_uvc_rec_active = 0;

    if (g_uvc_is_h264) {
        mov_rec_finish();
    } else if (g_uvc_avi) {
        FILE *fp = (FILE *)(long)g_uvc_avi->fdes;
        fget_name(fp, (u8 *)g_last_video_file, sizeof(g_last_video_file));
        AVI_close(g_uvc_avi);
        g_uvc_avi = NULL;
    }

    printf("[VREC_UVC] rec stop\n");
}

/* ---- pipeline 线程 ---- */

/* H264 Annex-B 流里检测 IDR 帧 (NAL type 5) */
static int vrec_uvc_is_idr(const u8 *data, u32 len)
{
    u32 off = 0;
    while (off + 4 < len) {
        u8 sl;
        if (data[off] == 0 && data[off + 1] == 0 && data[off + 2] == 0 && data[off + 3] == 1) {
            sl = 4;
        } else if (data[off] == 0 && data[off + 1] == 0 && data[off + 2] == 1) {
            sl = 3;
        } else {
            off++;
            continue;
        }
        if ((data[off + sl] & 0x1f) == 5) {
            return 1;
        }
        off += sl + 1;
    }
    return 0;
}

static void vrec_uvc_pipeline_thread(void *arg)
{
    vrec_uvc_buf_pool_t *pool = NULL;
    mpi_video_frame_info_t yuv = {0};
    u8  *raw = NULL;
    u32 raw_len = 0;
    int ret;

    (void)arg;

    ret = vrec_uvc_buf_pool_init(&pool);
    if (ret) {
        printf("[VREC_UVC] buf pool init failed\n");
        goto out;
    }

    if (!g_uvc_is_h264) {
        if (vrec_uvc_vdec_create(&g_uvc_vdec)) {
            printf("[VREC_UVC] vdec create failed\n");
            goto out_pool;
        }
        if (vrec_uvc_vpss_create(&g_uvc_vpss, g_uvc_width, g_uvc_height,
                                 g_rec_disp_attr.width, g_rec_disp_attr.height)) {
            printf("[VREC_UVC] vpss create failed\n");
            goto out_vdec;
        }
    }

    g_uvc_running = 1;
    printf("[VREC_UVC] pipeline running (%s %dx%d)\n",
           g_uvc_is_h264 ? "H264 passthrough no-display" : "JPEG",
           g_uvc_width, g_uvc_height);

    while (!g_uvc_exit) {
        if (vrec_uvc_buf_pool_get(pool, &raw, &raw_len, NULL, 500)) {
            os_time_dly(1);
            continue;
        }

        if (g_uvc_is_h264) {
            /* H264 直通: 等 IDR 后写 MP4 (同 mpi_video_net.c) */
            if (g_uvc_rec_active && g_uvc_h264_wait_idr == 0) {
                g_uvc_h264_wait_idr = 1;
            }
            if (!g_uvc_rec_active) {
                g_uvc_h264_wait_idr = 0;
            }

            if (g_uvc_rec_active && g_uvc_h264_wait_idr == 1) {
                if (vrec_uvc_is_idr(raw, raw_len)) {
                    g_uvc_h264_wait_idr = 2;
                    mov_rec_write(raw, raw_len, MOV_AV_FLAG_KEYFRAME);
                }
            } else if (g_uvc_rec_active && g_uvc_h264_wait_idr == 2) {
                mov_rec_write(raw, raw_len,
                              vrec_uvc_is_idr(raw, raw_len) ? MOV_AV_FLAG_KEYFRAME : 0);
            }
        } else {
            /* JPEG: VDEC 解码 -> VPSS 显示(预览) + 直通 MJPG AVI */
            if (vrec_uvc_vdec_decode(&g_uvc_vdec, raw, raw_len, &yuv,
                                     VREC_UVC_VDEC_TIMEOUT_MS) == 0) {
                mpi_vpss_send_frame(g_uvc_vpss.grp, &yuv, VREC_UVC_VPSS_TIMEOUT_MS);
                mpi_vdec_release_frame(g_uvc_vdec.chn, &yuv);
            }
            if (g_uvc_rec_active && g_uvc_avi && raw_len > 0) {
                AVI_write_frame(g_uvc_avi, (const char *)raw, (long)raw_len, 1);
            }
        }

        vrec_uvc_buf_pool_unlock(pool, raw);
    }

    printf("[VREC_UVC] pipeline exit\n");

    if (g_uvc_rec_active) {
        vrec_uvc_rec_stop();
    }
    if (!g_uvc_is_h264) {
        vrec_uvc_vpss_destroy(&g_uvc_vpss);
    }
out_vdec:
    if (!g_uvc_is_h264) {
        vrec_uvc_vdec_destroy(&g_uvc_vdec);
    }
out_pool:
    vrec_uvc_buf_pool_deinit(pool);
out:
    g_uvc_running = 0;
    g_uvc_pipeline_pid = 0;
}

/* ---- 打开 / 关闭 ---- */
static int vrec_uvc_open(void)
{
    int rw = 640, rh = 480;

    g_uvc_is_h264 = uvc_host_is_h264_format() ? 1 : 0;
    uvc_host_get_cur_resolution(&rw, &rh);
    if (rw > 0 && rh > 0) {
        g_uvc_width = (u16)rw;
        g_uvc_height = (u16)rh;
    }

    g_uvc_exit = 0;
    if (thread_fork("vrec_uvc_pipe", 20, 8192, 20,
                    &g_uvc_pipeline_pid, vrec_uvc_pipeline_thread, NULL)) {
        printf("[VREC_UVC] pipeline fork fail\n");
        return -1;
    }

    uvc_host_set_target_reso(g_uvc_width, g_uvc_height);
    uvc_host_start_camera();

    {
        int wait = 0;
        while (!g_uvc_running && wait++ < 500) {
            os_time_dly(1);
        }
    }
    if (!g_uvc_running) {
        printf("[VREC_UVC] pipeline not ready\n");
        return -1;
    }

    printf("[VREC_UVC] camera started %dx%d\n", g_uvc_width, g_uvc_height);
    return 0;
}

static void vrec_uvc_close(void)
{
    g_uvc_exit = 1;
    if (g_uvc_pipeline_pid) {
        thread_kill(&g_uvc_pipeline_pid, KILL_WAIT);
        g_uvc_pipeline_pid = 0;
    }
    g_uvc_running = 0;
    g_uvc_rec_active = 0;
    printf("[VREC_UVC] closed\n");
}

#endif /* TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE */

/*==============================================================================
 * 模式入口/退出
 *==============================================================================
 */
static int video_rec_mode_init(int param)
{
    putchar('6');
    log_info("video_rec_mode_init, param=%d\n", param);

    video_rec_check_rtc();

    memset(&g_rec_ctx, 0, sizeof(g_rec_ctx));
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    /* H264 检测提前到 UI init 之前: H264 不显示预览, 不创建 fb5 视频显示对象 */
    g_uvc_is_h264 = uvc_host_is_h264_format() ? 1 : 0;
#endif
    int show_video = 1;
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    show_video = g_uvc_is_h264 ? 0 : 1;
#endif
    video_rec_ui_init(g_rec_disp_attr.width, g_rec_disp_attr.height, show_video);
    {
        u8 recyle = kid_settings_get_rec_recyle_sec();  /* 菜单循环录像秒 30/60/90 */
        g_rec_ctx.loop_time_sec = recyle ? recyle : LOOP_TIME_DEFAULT;  /* 0退化默认60秒 */
    }
    g_cyc_file_count = 0;

    /* 打开摄像头并启动 LCD 预览 */
#if TCFG_CAMERA_DVP_ENABLE
    if (1) {
        app_camera_ret_t ret = app_camera_open(0, "GC0308", &g_camera_handle);
        if (ret == APP_CAMERA_OK) {
            app_camera_ret_t ret2 = app_camera_display_start(g_camera_handle, &g_rec_disp_attr);
            if (ret2 != APP_CAMERA_OK) {
                log_error("[VREC] display_start failed: %d", ret2);
                app_camera_close(&g_camera_handle);
                video_rec_ui_show_alert(2);
                return -1;
            }
            log_info("[VREC] camera preview started\n");
            app_camera_set_last_video_file_buffer(g_camera_handle, g_last_video_file);
        } else {
            log_error("[VREC] camera open failed: %d\n", ret);
            /* sensor 不可用等同无 SD, 复用 SD 卡弹窗 */
            video_rec_ui_show_alert(1);
            return -1;
        }
    }
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    if (vrec_uvc_open() != 0) {
        log_error("[VREC] uvc open failed\n");
        video_rec_ui_show_alert(1);
    }
#endif

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_VIDEO_REC);

    return 0;
}

void video_rec_mode_exit(void)
{
    log_info("video_rec_mode_exit\n");
    g_sd_auto_start_pending = 0;  /* 退出模式: 清 pending 防退出后 DONE 误触开录 */
    /* 先停录再拆 UI: stop 会触发 state->IDLE 的 UI 回调, 必须在 UI 仍存活时执行 */
    if (g_rec_ctx.recording || g_rec_ctx.paused) {
        video_rec_stop();
    }
    video_rec_ui_deinit();

    /* 关闭摄像头预览 */
#if TCFG_CAMERA_DVP_ENABLE
    if (g_camera_handle) {
        app_camera_display_stop(g_camera_handle);
        app_camera_close(&g_camera_handle);
        log_info("[VREC] camera preview stopped\n");
    }
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    vrec_uvc_close();
#endif

    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_VIDEO_REC);
}
/*==============================================================================
 * 模式入口函数
 *==============================================================================
 */
struct app_mode *app_enter_video_rec_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;
    enum video_rec_state prev_state = VIDEO_STATE_IDLE, curr_state;

    /* 进录像(内存吃紧模式)前释放回放遗留的全库名字缓存(87.8KB): 该缓存跨"多宫格↔全屏"
     * 保持是为了避免每次进回放重建(实测重建 2.3s, 是 FS 缓存冷填充 365 扇区所致),
     * 但不能带进录像/拍照这类吃内存的模式。 */
    flm_names_cache_release();

    {
        u8 _mode = APP_MODE_VIDEO_REC;
        syscfg_write(VM_ID_MODE_MEMORY, &_mode, sizeof(_mode));
    }

    if (video_rec_mode_init(arg) != 0) {
        log_error("app_enter_video_rec_mode: init failed, back to idle\n");
        video_rec_mode_exit();
        app_task_switch_to(APP_MODE_IDLE, 0);
        return NULL;
    }

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), video_rec_mode_key_table)) {
            os_time_dly(1);
            /* 状态变化 -> UI 通知 (兜底刷新, 处理后台线程改变 recording 位) */
            curr_state = video_rec_get_state();
            if (curr_state != prev_state) {
                video_rec_ui_on_state_change(prev_state, curr_state);
                prev_state = curr_state;
            }
            /* SD IN 自动开录超时兜底: 重建异常挂死也不能永不开录 */
            if (g_sd_auto_start_pending &&
                (u32)(sys_timer_get_ms() - g_sd_auto_start_pending_ms) >= VIDEO_REC_SD_AUTO_START_TIMEOUT_MS) {
                log_info("[VREC] SD IN -> rebuild wait timeout(5s), auto start anyway\n");
                video_rec_sd_auto_start_flush();
            }
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            video_rec_app_msg_handler(msg + 1);
            video_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE: {
            /* MSG_FROM_DEVICE: 0xF3=MODE_SWITCH 直接处理, 其他(SD/USB)走 app_default_msg_handler */
            u16 dev_evt = (u16)msg[1];
            if (dev_evt == 0xF3) {
                log_info("[VREC] MSG_FROM_DEVICE MODE_SWITCH (0xF3) -> goto PHOTO\n");
                if (g_rec_ctx.recording || g_rec_ctx.paused) {
                    video_rec_stop();
                }
                app_task_switch_to(APP_MODE_PHOTO, 0);
            }
            break;
        }
        }

        app_default_msg_handler(msg);
    }

    /* 退出前通知 UI 进入 IDLE */
    video_rec_ui_on_state_change(video_rec_get_state(), VIDEO_STATE_IDLE);
    video_rec_mode_exit();

    return next_mode;
}


/*==============================================================================
 * 兼容性别名
 *==============================================================================
 */
int video_rec_init(int param)
{
    return video_rec_mode_init(param);
}

void video_rec_exit(void)
{
    video_rec_mode_exit();
}

/*==============================================================================
 * 模式try_enter/try_exit
 *==============================================================================
 */
static int video_rec_mode_try_enter(int arg)
{
    log_info("video_rec_mode_try_enter\n");
    return 0;
}

static int video_rec_mode_try_exit(void)
{
    log_info("video_rec_mode_try_exit\n");
    return 0;
}

static const struct app_mode_ops video_rec_mode_ops = {
    .try_enter  = video_rec_mode_try_enter,
    .try_exit   = video_rec_mode_try_exit,
};

REGISTER_APP_MODE(video_rec_mode) = {
    .name   = APP_MODE_VIDEO_REC,
    .index = APP_MODE_VIDEO_REC_INDEX,
    .ops    = &video_rec_mode_ops,
};

#endif /* #if TCFG_APP_VIDEO_REC_EN */

