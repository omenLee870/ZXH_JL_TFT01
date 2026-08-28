#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_dec.data.bss")
#pragma data_seg(".video_dec.data")
#pragma const_seg(".video_dec.text.const")
#pragma code_seg(".video_dec.text")
#endif
#include "system/includes.h"
#include "jiffies.h"
#include "app_main.h"
#include "app_config.h"
#include "app_action.h"
#include "key_driver.h"
#include "video_dec.h"
#include "video_dec_ui.h"
#include "app_camera_mpi.h"
#include "playback/app_playback_mpi.h"
#include "playback/app_playback_mpi_priv.h"
#include "fs/fs.h"
#include "file_lock.h"  /* br33_fs_lock/unlock/is_locked: 回放保护锁 */
#include "automute.h"
#include "audio_config.h"
#include "media_file_list.h"


#define LOG_TAG             "[APP_VIDEO_DEC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"


#ifdef CONFIG_ENABLE_VLIST

#if (defined TCFG_PSRAM_SIZE && TCFG_PSRAM_SIZE > 0)
#define kc_malloc(sz)       malloc_psram(sz)
#define kc_realloc(p, sz)   realloc_psram(p, sz)
#define kc_free(p)          free_psram(p)
#else
#define kc_malloc(sz)       malloc(sz)
#define kc_realloc(p, sz)   realloc(p, sz)
#define kc_free(p)          free(p)
#endif
/*==============================================================================
 * Constants
 *==============================================================================*/
#define VDEC_PB_FB_NAME        "fb5"
#define VDEC_PB_FB_NUM         2
#define VDEC_PB_WIDTH          320
#define VDEC_PB_HEIGHT         240
#define VDEC_VIDEO_BUF_SIZE    (128u * 1024u) /* 帧池实测41KB/JPEG 33~44KB, 256KB过量挤压物理池 */
#define VDEC_AUDIO_BUF_SIZE    (16u * 1024u)
/*==============================================================================
 * Global context
 *==============================================================================*/
static struct {
    u8               **thumb_data;    /* 缩略图指针数组 (由 video_dec 管理) */
    int                thumb_count;   /* thumb_data 已分配条目数(flm 计数清零后仍需按此释放) */
    int                current_index;
    u32                total_sec;      /* 当前播放视频总时长(秒), OPENED事件得到 */
    enum video_dec_state state;
    app_playback_handle_t pb_handle;
    app_camera_handle_t   cam_handle;
    u8   video_playing;
    u8   video_paused;
    u8   photo_showing;
    u8   stopping_for_switch;
    u8   first_frame_seen;
    u8   corruption_skip_count;
    u32  play_gen;   /* 播放代号: 每次 video_dec_play 递增, 用于丢弃快切后过期的 SKIP/RECOVER 事件(防重入崩溃) */
    u8   muted;
    u8   mute_before_enter;
    u8   mode_running;
    /* P1: 缩略图 LRU 缓存 — thumb_lru[0] 最近使用, 队尾最久未用。
     * 缓存上限 VIDEO_DEC_THUMB_CACHE_MAX (20张 ~640KB), 超限淘汰队尾。 */
    int  thumb_lru[VIDEO_DEC_THUMB_CACHE_MAX];
    int  thumb_lru_count;
} g_vd;
/*==============================================================================
 * Forward declarations
 *==============================================================================*/
/* 删除菜单状态机(app_core线程): 0=关 1=roller_1选择 2=roller_2二次确认 */
static u8 s_del_menu = 0;
static u8 s_del_sel = 0;      /* roller_1选中: 0=删除所有 1=取消 2=删除当前 */
static u8 s_del_pending = 0;  /* 二次确认待删动作: 1=删当前 2=删所有 */
static void vd_set_state(enum video_dec_state new_state);
static int  vd_scan_and_load(void);
static void vd_free_files(void);
static int  vd_open_playback(void);
static void vd_close_playback(void);
static void vd_playback_event_cb(void *priv, int argc, int *argv);
static void vd_dispatch_play_internal(void);
static void vd_close_photo_if_showing(void);
static int  vd_thumb_ensure(int index);  /* P1: 缩略图 LRU ensure */
static void vd_ui_refresh_grid(void);
int video_dec_play(int index);
#if TCFG_APP_VIDEO_REC_EN
int video_rec_repair_avi(const char *path);
#endif
/*==============================================================================
 * State machine
 *==============================================================================*/
static void vd_close_photo_if_showing(void)
{
    if (g_vd.photo_showing && g_vd.pb_handle) {
        app_playback_image_pipeline_close(g_vd.pb_handle);
        g_vd.photo_showing = 0;
    }
}
static void vd_set_state(enum video_dec_state new_state)
{
    enum video_dec_state old = g_vd.state;
    if (old == new_state) {
        return;
    }
    log_info("vd_set_state: %d -> %d\n", old, new_state);
    g_vd.state = new_state;
    video_dec_ui_on_state_change(old, new_state);
}
enum video_dec_state video_dec_get_state(void)
{
    return g_vd.state;
}
int video_dec_get_current_index(void)
{
    return g_vd.current_index;
}
int video_dec_get_file_count(void)
{
    return flm_get_count();
}
/*==============================================================================
 * UI helper
 *==============================================================================*/
static void vd_ui_refresh_grid(void)
{
    flm_file_t centry;
    const char *name = NULL;
    if (flm_get_entry(g_vd.current_index, &centry) == 0) {
        name = centry.name[0] ? centry.name : centry.path;
    }
    video_dec_ui_refresh_grid(name, flm_get_count(), g_vd.current_index);
}
/*==============================================================================
 * File scanning
 *==============================================================================*/
static int vd_scan_and_load(void)
{
    vd_free_files();
    g_vd.current_index = 0;
    s_del_menu = 0;
    if (flm_sd_online() != 0) {
        log_error("vd_scan_and_load: SD not ready\n");
        vd_set_state(VIDEO_DEC_STATE_LIST);
        video_dec_ui_show_alert(1);
        video_dec_ui_refresh_grid(NULL, 0, -1);
        return 0;
    }
    vd_set_state(VIDEO_DEC_STATE_SCANNING);

    flm_scan_and_load();
    int file_count = flm_get_count();

    log_info("vd_scan_and_load: total %d files\n", file_count);
    if (file_count == 0) {
        vd_set_state(VIDEO_DEC_STATE_LIST);
        /* 删空时隐藏 video disp img_obj, 否则残留最后一帧照片 */
        video_dec_ui_clear_video();
        video_dec_ui_refresh_grid(NULL, 0, -1);
        return 0;
    }

    /* 分配缩略图指针数组 */
    g_vd.thumb_data = (u8 **)kc_malloc(file_count * sizeof(u8 *));
    if (g_vd.thumb_data) {
        memset(g_vd.thumb_data, 0, file_count * sizeof(u8 *));
    }
    g_vd.thumb_count = file_count;

    int i;
    int page_start = 0;
    int page_end = page_start + VIDEO_DEC_GRID_COUNT;
    if (page_end > file_count) {
        page_end = file_count;
    }
    for (i = page_start; i < page_end; i++) {
        vd_thumb_ensure(i);
    }
    vd_set_state(VIDEO_DEC_STATE_LIST);
    vd_ui_refresh_grid();
    video_dec_storage_t info;
    if (video_dec_get_storage_info(&info) == 0) {
        video_dec_ui_update_storage(&info);
    }
    return file_count;
}

/* 插卡后重扫并自动显示第一个 (kid_camera 应用层 SD_EVENT 调用) */
void video_dec_refresh_after_insert(void)
{
    vd_scan_and_load();
    if (flm_get_count() > 0) {
        video_dec_play(0);
    }
}

/* 拔卡: 停播放/关照片, 清索引状态, 隐藏进度标签清显示回空列表 */
void video_dec_clear_on_card_out(void)
{
    if (g_vd.video_playing || g_vd.video_paused) {
        video_dec_stop_video();
    }
    vd_close_photo_if_showing();
    vd_free_files();  /* 先按旧 file_count 释放缩略图, flm_reset 会把计数清零 */
    flm_reset();
    g_vd.current_index = 0;
    s_del_menu = 0;
    vd_set_state(VIDEO_DEC_STATE_LIST);
    video_dec_ui_time_hide();
    video_dec_ui_clear_video();
    video_dec_ui_refresh_grid(NULL, 0, -1);
    video_dec_ui_show_alert(1);
}
static void vd_free_files(void)
{
    int i;
    /* 按自己记录的条目数释放, 不依赖 flm 计数(删除/拔卡路径会先清零 flm, 按 flm
     * 遍历会漏掉全部缩略图条目 -> 每删一批泄漏 ~294KB) */
    if (g_vd.thumb_data) {
        for (i = 0; i < g_vd.thumb_count; i++) {
            if (g_vd.thumb_data[i]) {
                kc_free(g_vd.thumb_data[i]);
                g_vd.thumb_data[i] = NULL;
            }
        }
        kc_free(g_vd.thumb_data);
        g_vd.thumb_data = NULL;
    }
    g_vd.thumb_count = 0;
    g_vd.thumb_lru_count = 0;
}
/*==============================================================================
 * Playback handle management
 *==============================================================================*/
static int vd_open_playback(void)
{
    app_playback_ret_t ret;
    app_playback_display_attr_t attr = {
        .fb_name = VDEC_PB_FB_NAME,
        .fb_num  = VDEC_PB_FB_NUM,
        .width   = VDEC_PB_WIDTH,
        .height  = VDEC_PB_HEIGHT,
    };
    if (g_vd.pb_handle) {
        log_info("vd_open_playback: already open\n");
        return 0;
    }
    ret = app_playback_open(&g_vd.pb_handle, &attr);
    if (ret != APP_PLAYBACK_OK) {
        log_error("vd_open_playback: app_playback_open failed, ret=%d\n", ret);
        g_vd.pb_handle = NULL;
        return -1;
    }
    app_playback_set_video_buffer_size(g_vd.pb_handle, VDEC_VIDEO_BUF_SIZE);
    app_playback_set_audio_buffer_size(g_vd.pb_handle, VDEC_AUDIO_BUF_SIZE);
    app_playback_register_event_handler(g_vd.pb_handle, NULL, vd_playback_event_cb);
    log_info("vd_open_playback: ok\n");
    return 0;
}
static void vd_close_playback(void)
{
    if (g_vd.pb_handle) {
        if (g_vd.video_playing) {
            video_dec_stop_video();
        }
        app_playback_close(&g_vd.pb_handle);
        g_vd.pb_handle = NULL;
        log_info("vd_close_playback: closed\n");
    }
}
/*==============================================================================
 * Playback event callback
 *==============================================================================*/
static void vd_playback_event_cb(void *priv, int argc, int *argv)
{
    if (argc < 1 || !argv) {
        return;
    }
    int event = argv[0];
    int file_count = flm_get_count();
    log_info("vd_playback_event_cb: event=%d\n", event);
    switch (event) {
    case APP_PLAYBACK_EVENT_OPENED:
        g_vd.total_sec = (argc >= 2) ? (u32)(argv[1] / 1000) : 0;
        log_info("event OPENED total_ms=%d idx=%d\n",
                 argc >= 2 ? argv[1] : 0, g_vd.current_index);
        break;
    case APP_PLAYBACK_EVENT_STARTED:
        log_info("event STARTED idx=%d\n", g_vd.current_index);
        break;
    case APP_PLAYBACK_EVENT_FIRST_FRAME:
        log_info("event FIRST_FRAME idx=%d (corruption_skip reset)\n", g_vd.current_index);
        g_vd.first_frame_seen = 1;
        g_vd.corruption_skip_count = 0;
        break;
    case APP_PLAYBACK_EVENT_CUR_TIME: {
        if (argc >= 2) {
            int pts_ms = argv[1];
            u32 cur_sec = (u32)(pts_ms / 1000);
            video_dec_ui_update_progress(cur_sec, g_vd.total_sec);
        }
        break;
    }
    case APP_PLAYBACK_EVENT_FINISHED:
        log_info("event FINISHED idx=%d first_frame_seen=%d\n",
                 g_vd.current_index, g_vd.first_frame_seen);
        if (g_vd.pb_handle) {
            app_playback_ret_t ret = app_playback_video_finish(g_vd.pb_handle);
            if (ret != APP_PLAYBACK_OK && ret != APP_PLAYBACK_ERR_NOT_READY) {
                log_error("event FINISHED: finish failed ret=%d\n", ret);
            }
        }
        g_vd.video_playing = 0;
        g_vd.video_paused = 0;
        video_dec_ui_time_hide();
        if (!g_vd.first_frame_seen && file_count > 0 &&
            g_vd.corruption_skip_count < file_count) {
            flm_file_t cur;
            if (flm_get_entry(g_vd.current_index, &cur) == 0) {
                log_error("CORRUPTED video: %s, request skip on app_core\n", cur.path);
            } else {
                log_error("CORRUPTED video: ?, request skip on app_core\n");
            }
            app_send_message(APP_MSG_VIDEO_DEC_SKIP_CORRUPT, 0);
        } else {
            g_vd.corruption_skip_count = 0;
            vd_set_state(VIDEO_DEC_STATE_VIDEO_PLAY);
            vd_ui_refresh_grid();
        }
        break;
    case APP_PLAYBACK_EVENT_STOPPED:
        log_info("event STOPPED idx=%d switch=%d\n",
                 g_vd.current_index, g_vd.stopping_for_switch);
        g_vd.stopping_for_switch = 0;
        break;
    case APP_PLAYBACK_EVENT_ERROR:
        log_error("event ERROR idx=%d first_frame_seen=%d\n",
                  g_vd.current_index, g_vd.first_frame_seen);
        /* 事件任务绝不碰 pb_handle: 坏视频解码线程已退, 再 stop 与其半死状态竞争 → MMU 崩溃 */

        g_vd.video_playing = 0;
        g_vd.video_paused = 0;
        if (!g_vd.first_frame_seen && file_count > 1 &&
            g_vd.corruption_skip_count < file_count) {
            /* 首帧就失败: 无活跃管线, 直接跳过(app_core) */
            log_error("[VDEC] corrupt(no first frame) idx=%d, skip on app_core (gen=%u)\n",
                      g_vd.current_index, g_vd.play_gen);
            app_send_message2(APP_MSG_VIDEO_DEC_SKIP_CORRUPT, (int)g_vd.play_gen, 0);
        } else {
            /* 中途损坏(管线在fault态)或跳过预算用尽: app_core 上安全停管线后恢复 */
            log_error("[VDEC] mid-stream error idx=%d, recover on app_core (gen=%u)\n",
                      g_vd.current_index, g_vd.play_gen);
            app_send_message2(APP_MSG_VIDEO_DEC_ERR_RECOVER, (int)g_vd.play_gen, 0);
        }
        break;
    }
}
/*==============================================================================
 * Play control: play / pause / resume / stop
 *
 * Design notes:
 *   - video_dec_play(index) opens file[index], photo uses one_image_path,
 *     video uses video_start
 *   - pause/resume: SDK playback has no native pause, use stop+restart
 *   - stop: close video and return to LIST state
 *==============================================================================*/
int video_dec_play(int index)
{
    app_playback_ret_t ret;
    int file_count = flm_get_count();
    /* 新一次播放: 递增代号。此刻之前旧播放的 play_avi 线程若已 post ERROR/FINISHED,
     * 那些过期事件仍带旧 play_gen, app_core 处理 SKIP/RECOVER 时据此丢弃。 */
    g_vd.play_gen++;
    if (index < 0 || index >= file_count) {
        log_error("video_dec_play: invalid index=%d, count=%d\n",
                  index, file_count);
        return -1;
    }
    if (!g_vd.pb_handle) {
        log_error("video_dec_play: pb_handle is NULL\n");
        return -1;
    }
    if (g_vd.video_playing || g_vd.video_paused) {
        g_vd.stopping_for_switch = 1;
        ret = app_playback_video_finish(g_vd.pb_handle);
        if (ret != APP_PLAYBACK_OK && ret != APP_PLAYBACK_ERR_NOT_READY) {
            log_error("video_dec_play: finish old video failed ret=%d\n", ret);
        }
        g_vd.video_playing = 0;
        g_vd.video_paused = 0;
    }
    vd_close_photo_if_showing();
    g_vd.current_index = index;
    flm_file_t entry;
    if (flm_get_entry(index, &entry) != 0) {
        return -1;
    }
    if (entry.type == VIDEO_DEC_TYPE_PHOTO) {
        /* 坏照片(JPEG头损坏 -> one_image_path 内部 probe 失败, 日志 "failed to probe
         * VDEC pic info ret=-5")会拆掉当前画面变黑屏且不恢复。用有限循环(非递归:
         * app_core 栈仅~3KB, 禁止按坏文件数深递归)自动跳到下一张可解码照片; 若跳到
         * 视频则落入下面的视频分支; 全部损坏才告警。 */
        int photo_tries = 0;
        for (;;) {
            log_info("video_dec_play: photo %s\n", entry.path);
            video_dec_ui_time_hide();  /* 视频切照片: 隐藏播放进度标签 */
            video_dec_ui_show_photo(entry.path);
            ret = app_playback_one_image_path(g_vd.pb_handle, entry.path);
            if (ret == APP_PLAYBACK_OK) {
                vd_set_state(VIDEO_DEC_STATE_PHOTO_VIEW);
                g_vd.photo_showing = 1;
                g_vd.corruption_skip_count = 0;
                break;
            }
            log_error("video_dec_play: one_image_path failed ret=%d (corrupt %s), skip\n",
                      ret, entry.path);
            if (file_count <= 1 || ++photo_tries >= file_count) {
                video_dec_ui_show_alert(2);   /* 全部无法解码 */
                return -1;
            }
            g_vd.current_index = (g_vd.current_index + 1) % file_count;
            if (flm_get_entry(g_vd.current_index, &entry) != 0) {
                log_error("video_dec_play: flm_get_entry failed idx=%d\n", g_vd.current_index);
                video_dec_ui_show_alert(2);
                return -1;
            }
            if (entry.type != VIDEO_DEC_TYPE_PHOTO) {
                break;  /* 跳到了视频, 交给下面视频分支 */
            }
        }
    }
    if (entry.type != VIDEO_DEC_TYPE_PHOTO) {
        log_info("video_dec_play: video %s\n", entry.path);
        g_vd.first_frame_seen = 0;
        if (!g_vd.muted) {
            app_audio_mute(AUDIO_UNMUTE_DEFAULT);
        }
        video_dec_ui_show_video(entry.path);
        ret = app_playback_video_start(g_vd.pb_handle, entry.path);
        if (ret == APP_PLAYBACK_ERR_BUSY) {
            /* [FIX] 上次FINISHED/ERROR未正确清理，底层video_started仍为1，强制stop后重试 */
            log_info("video_dec_play: BUSY, force stop and retry\n");
            app_playback_video_stop(g_vd.pb_handle);
            ret = app_playback_video_start(g_vd.pb_handle, entry.path);
        }
#if TCFG_APP_VIDEO_REC_EN
        if (ret == APP_PLAYBACK_ERR_IO || ret == APP_PLAYBACK_ERR_NOT_FOUND) {
            /* 仅对打不开/解析失败的损坏 AVI 修复一次, 其他错误类型不动文件 */
            log_error("video_dec_play: video_start failed ret=%d path=%s, repairing\n",
                      ret, entry.path);
            if (video_rec_repair_avi(entry.path) == 0) {
                ret = app_playback_video_start(g_vd.pb_handle, entry.path);
            }
        }
#endif
        if (ret != APP_PLAYBACK_OK) {
            log_error("video_dec_play: video_start failed, ret=%d path=%s\n", ret, entry.path);
            video_dec_ui_show_alert(2);
            return -1;
        }
        g_vd.video_playing = 1;
        g_vd.video_paused = 0;
        vd_set_state(VIDEO_DEC_STATE_VIDEO_PLAY);
        log_info("video_dec_play: video STARTED path=%s\n", entry.path);
    }
    vd_ui_refresh_grid();
    return 0;
}
int video_dec_pause_video(void)
{
    if (!g_vd.video_playing || g_vd.video_paused) {
        log_info("pause_video: skip (playing=%d paused=%d)\n",
                 g_vd.video_playing, g_vd.video_paused);
        return -1;
    }
    flm_file_t pf;
    if (flm_get_entry(g_vd.current_index, &pf) != 0) {
        log_error("pause_video: get_entry failed idx=%d\n", g_vd.current_index);
        return -1;
    }
    log_info("pause_video: finishing demux path=%s\n", pf.path);
    app_playback_ret_t ret = app_playback_video_finish(g_vd.pb_handle);
    if (ret != APP_PLAYBACK_OK && ret != APP_PLAYBACK_ERR_NOT_READY) {
        log_error("pause_video: finish failed ret=%d\n", ret);
        return -1;
    }
    g_vd.video_paused = 1;
    g_vd.video_playing = 0;
    return 0;
}
int video_dec_resume_video(void)
{
    if (!g_vd.video_paused) {
        log_info("resume_video: skip (not paused)\n");
        return -1;
    }
    if (g_vd.current_index < 0 || g_vd.current_index >= flm_get_count()) {
        log_error("resume_video: invalid index=%d\n", g_vd.current_index);
        g_vd.video_paused = 0;
        return -1;
    }
    flm_file_t pf;
    if (flm_get_entry(g_vd.current_index, &pf) != 0) {
        log_error("resume_video: get_entry failed idx=%d\n", g_vd.current_index);
        g_vd.video_paused = 0;
        return -1;
    }
    log_info("resume_video: restarting (no native seek, replaying from 0) path=%s\n", pf.path);
    g_vd.video_paused = 0;
    g_vd.first_frame_seen = 0;
    app_playback_ret_t ret = app_playback_video_start(g_vd.pb_handle, pf.path);
    if (ret == APP_PLAYBACK_ERR_BUSY) {
        log_info("resume_video: BUSY, force stop and retry\n");
        app_playback_video_stop(g_vd.pb_handle);
        ret = app_playback_video_start(g_vd.pb_handle, pf.path);
    }
    if (ret != APP_PLAYBACK_OK) {
        log_error("resume_video: video_start failed ret=%d\n", ret);
        video_dec_ui_show_alert(2);
        return -1;
    }
    g_vd.video_playing = 1;
    return 0;
}
int video_dec_stop_video(void)
{
    if (!g_vd.video_playing && !g_vd.video_paused &&
        g_vd.state != VIDEO_DEC_STATE_VIDEO_PLAY) {
        return -1;
    }
    log_info("video_dec_stop_video\n");
    if (g_vd.pb_handle) {
        app_playback_video_stop(g_vd.pb_handle);
    }
    g_vd.video_playing = 0;
    g_vd.video_paused = 0;
    video_dec_ui_time_hide();
    video_dec_ui_close_video();
    return 0;
}
int video_dec_video_progress(u32 *current_sec, u32 *total_sec)
{
    if (current_sec) {
        *current_sec = 0;
    }
    if (total_sec) {
        *total_sec = 0;
    }
    return 0;
}
/*==============================================================================
 * Mute control
 *==============================================================================*/
int video_dec_set_mute(int mute)
{
    if (mute) {
        app_audio_mute(AUDIO_MUTE_DEFAULT);
        g_vd.muted = 1;
        log_info("video_dec_set_mute: ON\n");
    } else {
        app_audio_mute(AUDIO_UNMUTE_DEFAULT);
        g_vd.muted = 0;
        log_info("video_dec_set_mute: OFF\n");
    }
    return 0;
}
void video_dec_mode_exit_mute_restore(void)
{
    if (g_vd.mute_before_enter) {
        app_audio_mute(AUDIO_MUTE_DEFAULT);
        log_info("video_dec_mode_exit_mute_restore: restore mute ON\n");
    } else {
        app_audio_mute(AUDIO_UNMUTE_DEFAULT);
        log_info("video_dec_mode_exit_mute_restore: restore mute OFF\n");
    }
    g_vd.muted = g_vd.mute_before_enter;
}
/*==============================================================================
 * Legacy photo/video view interfaces
 *==============================================================================*/
static int vd_find_index_by_path(const char *path, u8 type_filter, int filter_enable)
{
    int i;
    int file_count = flm_get_count();
    if (!path || !path[0]) {
        return -1;
    }
    for (i = 0; i < file_count; i++) {
        flm_file_t f;
        if (flm_get_entry(i, &f) != 0) {
            continue;
        }
        if (strcmp(f.path, path) != 0) {
            continue;
        }
        if (filter_enable && f.type != type_filter) {
            continue;
        }
        return i;
    }
    return -1;
}
int video_dec_view_photo(const char *path)
{
    int idx = vd_find_index_by_path(path, VIDEO_DEC_TYPE_PHOTO, 1);
    if (idx < 0) {
        log_error("video_dec_view_photo: not found path=%s\n", path ? path : "<null>");
        return -1;
    }
    return video_dec_play(idx);
}
int video_dec_play_video(const char *path)
{
    int idx = vd_find_index_by_path(path, VIDEO_DEC_TYPE_VIDEO, 1);
    if (idx < 0) {
        log_error("video_dec_play_video: not found path=%s\n", path ? path : "<null>");
        return -1;
    }
    return video_dec_play(idx);
}
/*==============================================================================
 * Dispatch functions (called by app_msg_handler)
 *==============================================================================*/
/* 坏视频跳过: 由 APP_MSG_VIDEO_DEC_SKIP_CORRUPT 在 app_core 触发(见事件回调注释),
 * 保证所有 pb_handle 操作都在 app_core 单线程完成。 */
void video_dec_skip_corrupt(u32 gen)
{
    int file_count = flm_get_count();

    if (gen != g_vd.play_gen) {
        /* 过期事件: 快切后当前已是别的播放, 丢弃防止重入 video_dec_play。 */
        log_info("skip_corrupt: stale gen=%u cur=%u, drop\n", gen, g_vd.play_gen);
        return;
    }
    if (file_count <= 0) {
        return;
    }
    g_vd.corruption_skip_count++;
    g_vd.current_index = (g_vd.current_index + 1) % file_count;
    log_info("skip_corrupt: -> idx=%d (skip_cnt=%d)\n",
             g_vd.current_index, g_vd.corruption_skip_count);
    video_dec_play(g_vd.current_index);
}
/* 坏视频中途损坏错误恢复: 由 APP_MSG_VIDEO_DEC_ERR_RECOVER 在 app_core 触发。所有
 * pb_handle 拆除都在 app_core 单线程(事件任务里停半死管线会崩)。先安全停, 再过滤跳
 * 下一个可播文件; 预算用尽则回列表告警。 */
void video_dec_err_recover(u32 gen)
{
    int file_count = flm_get_count();

    if (gen != g_vd.play_gen) {
        /* 过期事件: 快切后当前已是别的播放, 丢弃防止重入。 */
        log_info("err_recover: stale gen=%u cur=%u, drop\n", gen, g_vd.play_gen);
        return;
    }
    if (g_vd.pb_handle) {
        /* 用 finish 不用 stop: stop 会 app_playback_image_pipeline_close 拆刚才还在流式
         * 解码的活跃 VDEC/VPSS/VO 管线(拆除顺序VO->VPSS->VDEC, VDEC最后停, worker/DSP还
         * 有活时先拆下游 -> MMU/DSP崩溃)。finish 只拆 play_avi/demux 线程, 不关管线, 留给
         * 下一个同分辨率文件(全640x480)复用(与正常切换视频同一条不崩的路径)。 */
        app_playback_video_finish(g_vd.pb_handle);
    }
    g_vd.video_playing = 0;
    g_vd.video_paused = 0;
    video_dec_ui_close_video();
    if (file_count > 1 && g_vd.corruption_skip_count < file_count) {
        g_vd.corruption_skip_count++;
        g_vd.current_index = (g_vd.current_index + 1) % file_count;
        log_info("err_recover: skip to idx=%d (skip_cnt=%d)\n",
                 g_vd.current_index, g_vd.corruption_skip_count);
        video_dec_play(g_vd.current_index);
    } else {
        g_vd.corruption_skip_count = 0;
        video_dec_ui_show_alert(2);
        vd_set_state(VIDEO_DEC_STATE_LIST);
        vd_ui_refresh_grid();
    }
}
/* 长按OK: 弹出/收起删除菜单(roller_1)。视频播放中不弹, 防误删正在播放的文件。 */
void video_dec_dispatch_show_del_menu(void)
{
    if (s_del_menu) {
        s_del_menu = 0;
        s_del_pending = 0;
        video_dec_ui_del_menu_hide();
        log_info("show_del_menu: toggle off\n");
        return;
    }
    if (flm_get_count() <= 0) {
        log_info("show_del_menu: no files\n");
        return;
    }
    s_del_menu = 1;
    s_del_sel = 2;  /* 默认停"删除当前"(最常用) */
    video_dec_ui_del_menu_show(s_del_sel);
    log_info("show_del_menu: open sel=%d\n", s_del_sel);
}
void video_dec_dispatch_play(void)
{
    int file_count = flm_get_count();
    if (s_del_menu == 1) {
        if (s_del_sel == 0) {          /* 删除所有 -> 二次确认 */
            s_del_pending = 2;
            s_del_menu = 2;
            video_dec_ui_del_menu_confirm();
        } else if (s_del_sel == 2) {   /* 删除当前 -> 二次确认 */
            s_del_pending = 1;
            s_del_menu = 2;
            video_dec_ui_del_menu_confirm();
        } else {                       /* 取消 */
            s_del_menu = 0;
            video_dec_ui_del_menu_hide();
        }
        return;
    }
    if (s_del_menu == 2) {
        if (s_del_pending == 1) {
            video_dec_delete_selected();
        } else if (s_del_pending == 2) {
            video_dec_delete_all();
        }
        s_del_pending = 0;
        s_del_menu = 0;
        video_dec_ui_del_menu_hide();
        return;
    }
    flm_file_t centry;
    const char *log_path = "<none>";
    if (file_count > 0 && g_vd.current_index >= 0 &&
        g_vd.current_index < file_count &&
        flm_get_entry(g_vd.current_index, &centry) == 0) {
        log_path = centry.path;
    }
    log_info("dispatch_play: state=%d index=%d/%d path=%s\n",
             g_vd.state, g_vd.current_index, file_count, log_path);

    switch (g_vd.state) {
    case VIDEO_DEC_STATE_LIST:
        log_info("dispatch_play: LIST -> open current focus\n");
        vd_dispatch_play_internal();
        break;
    case VIDEO_DEC_STATE_PHOTO_VIEW:
        if (file_count > 0) {
            g_vd.current_index++;
            if (g_vd.current_index >= file_count) {
                g_vd.current_index = 0;
            }
            log_info("dispatch_play: PHOTO_VIEW -> next photo, new_index=%d\n",
                     g_vd.current_index);
            video_dec_play(g_vd.current_index);
        }
        break;
    case VIDEO_DEC_STATE_VIDEO_PLAY:
        if (g_vd.video_paused) {
            log_info("dispatch_play: VIDEO_PLAY -> resume\n");
            video_dec_resume_video();
        } else if (g_vd.video_playing) {
            log_info("dispatch_play: VIDEO_PLAY -> pause\n");
            video_dec_pause_video();
        } else {
            log_info("dispatch_play: VIDEO_PLAY -> replay current\n");
            video_dec_play(g_vd.current_index);
        }
        break;
    default:
        log_info("dispatch_play: state=%d unhandled\n", g_vd.state);
        break;
    }
}
static void vd_dispatch_play_internal(void)
{
    if (flm_get_count() <= 0 || g_vd.current_index < 0) {
        return;
    }
    video_dec_play(g_vd.current_index);
}
void video_dec_dispatch_next(void)
{
    int file_count = flm_get_count();
    if (s_del_menu == 1) {
        s_del_sel = (u8)((s_del_sel + 1) % 3);
        video_dec_ui_del_menu_set_sel(s_del_sel);
        return;
    }
    if (s_del_menu == 2) {
        return;
    }
    flm_file_t centry;
    const char *log_path = "<none>";
    if (file_count > 0 && g_vd.current_index >= 0 &&
        g_vd.current_index < file_count &&
        flm_get_entry(g_vd.current_index, &centry) == 0) {
        log_path = centry.path;
    }
    log_info("dispatch_next: state=%d index=%d/%d path=%s\n",
             g_vd.state, g_vd.current_index, file_count, log_path);

    switch (g_vd.state) {
    case VIDEO_DEC_STATE_LIST:
        if (file_count > 0) {
            g_vd.current_index++;
            if (g_vd.current_index >= file_count) {
                g_vd.current_index = 0;
            }
            {
                int page = g_vd.current_index / VIDEO_DEC_GRID_COUNT;
                int page_start = page * VIDEO_DEC_GRID_COUNT;
                int page_end = page_start + VIDEO_DEC_GRID_COUNT;
                if (page_end > file_count) {
                    page_end = file_count;
                }
                int i;
                for (i = page_start; i < page_end; i++) {
                    vd_thumb_ensure(i);
                }
            }
            vd_ui_refresh_grid();
        }
        break;
    case VIDEO_DEC_STATE_PHOTO_VIEW:
    case VIDEO_DEC_STATE_VIDEO_PLAY:
        if (file_count > 0) {
            g_vd.current_index++;
            if (g_vd.current_index >= file_count) {
                g_vd.current_index = 0;
            }
            video_dec_play(g_vd.current_index);
        }
        break;
    default:
        break;
    }
}
void video_dec_dispatch_prev(void)
{
    int file_count = flm_get_count();
    if (s_del_menu == 1) {
        s_del_sel = (u8)((s_del_sel + 2) % 3);
        video_dec_ui_del_menu_set_sel(s_del_sel);
        return;
    }
    if (s_del_menu == 2) {
        return;
    }
    flm_file_t centry;
    const char *log_path = "<none>";
    if (file_count > 0 && g_vd.current_index >= 0 &&
        g_vd.current_index < file_count &&
        flm_get_entry(g_vd.current_index, &centry) == 0) {
        log_path = centry.path;
    }
    log_info("dispatch_prev: state=%d index=%d/%d path=%s\n",
             g_vd.state, g_vd.current_index, file_count, log_path);

    switch (g_vd.state) {
    case VIDEO_DEC_STATE_LIST:
        if (file_count > 0) {
            g_vd.current_index--;
            if (g_vd.current_index < 0) {
                g_vd.current_index = file_count - 1;
            }
            {
                int page = g_vd.current_index / VIDEO_DEC_GRID_COUNT;
                int page_start = page * VIDEO_DEC_GRID_COUNT;
                int page_end = page_start + VIDEO_DEC_GRID_COUNT;
                if (page_end > file_count) {
                    page_end = file_count;
                }
                int i;
                for (i = page_start; i < page_end; i++) {
                    vd_thumb_ensure(i);
                }
            }
            vd_ui_refresh_grid();
        }
        break;
    case VIDEO_DEC_STATE_PHOTO_VIEW:
    case VIDEO_DEC_STATE_VIDEO_PLAY:
        if (file_count > 0) {
            g_vd.current_index--;
            if (g_vd.current_index < 0) {
                g_vd.current_index = file_count - 1;
            }
            video_dec_play(g_vd.current_index);
        }
        break;
    default:
        break;
    }
}
void video_dec_dispatch_back(void)
{
    if (s_del_menu == 2) {
        s_del_menu = 1;
        s_del_pending = 0;
        video_dec_ui_del_menu_show(s_del_sel);
        return;
    }
    if (s_del_menu == 1) {
        s_del_menu = 0;
        video_dec_ui_del_menu_hide();
        return;
    }
    log_info("dispatch_back: state=%d index=%d\n", g_vd.state, g_vd.current_index);
    switch (g_vd.state) {
    case VIDEO_DEC_STATE_PHOTO_VIEW:
        /* 回放为纯全屏浏览(LIST无缩略图渲染=白屏), 看图时BACK直接回主页不落空列表 */
        vd_close_photo_if_showing();
        video_dec_ui_time_hide();
        video_dec_ui_back_to_home();
        break;
    case VIDEO_DEC_STATE_VIDEO_PLAY:
        video_dec_stop_video();
        video_dec_ui_back_to_home();
        break;
    case VIDEO_DEC_STATE_LIST:
    default:
        video_dec_ui_back_to_home();
        break;
    }
}
/*==============================================================================
 * P2: Volume control (仅 VIDEO_PLAY 状态有效)
 * 显式操作 APP_AUDIO_STATE_MUSIC 通道 (视频回放音量), 不依赖"当前通道"。
 *==============================================================================*/
#define VDEC_VOL_STEP       1
int video_dec_volume_up(void)
{
    if (g_vd.state != VIDEO_DEC_STATE_VIDEO_PLAY) {
        log_info("volume_up: skip (not in VIDEO_PLAY, state=%d)\n", g_vd.state);
        return -1;
    }
    s16 vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, vol + VDEC_VOL_STEP, 1);
    log_info("volume_up: %d -> %d\n", vol, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
    return 0;
}
int video_dec_volume_down(void)
{
    if (g_vd.state != VIDEO_DEC_STATE_VIDEO_PLAY) {
        log_info("volume_down: skip (not in VIDEO_PLAY, state=%d)\n", g_vd.state);
        return -1;
    }
    s16 vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    if (vol > 0) {
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, vol - VDEC_VOL_STEP, 1);
    }
    log_info("volume_down: %d -> %d\n", vol, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
    return 0;
}
int video_dec_volume_get(void)
{
    return (int)app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
}

/*==============================================================================
 * P2: 批量删除 / 一键清空
 *  - 加锁(保护)文件跳过 (video_dec_delete_file 返回 -2)
 *  - 删除后重新扫描刷新列表与缩略图 LRU
 *==============================================================================*/
/* 删除当前选中文件; 返回 0=成功, -2=加锁跳过, -1=失败。 */
int video_dec_delete_selected(void)
{
    int file_count = flm_get_count();
    if (g_vd.current_index < 0 || g_vd.current_index >= file_count) {
        log_error("delete_selected: invalid index=%d\n", g_vd.current_index);
        return -1;
    }
    /* 删除前先关闭当前显示管线(照片/视频), 否则管线仍持有已删文件的解码资源且残留
     * 在屏上(对抗审查确认的 PHOTO_VIEW 删除残留 bug)。 */
    if (g_vd.photo_showing) {
        vd_close_photo_if_showing();
    }
    if (g_vd.video_playing || g_vd.video_paused) {
        video_dec_stop_video();
    }
    flm_file_t f;
    if (flm_get_entry(g_vd.current_index, &f) != 0) {
        return -1;
    }
    /* 删后停在原位: 同 index 即原下一张, 删最后一张落到新最后一张 */
    int next_index = g_vd.current_index;
    int ret = video_dec_delete_file(f.path);
    if (ret == 0) {
        log_info("delete_selected: deleted %s, rescanning\n", f.path);
        vd_scan_and_load();
        if (flm_get_count() > 0) {
            if (next_index >= flm_get_count()) {
                next_index = flm_get_count() - 1;
            }
            video_dec_play(next_index);  /* 停在原位, 不留黑屏 */
        } else {
            video_dec_ui_refresh_grid(NULL, 0, -1);
        }
    }
    return ret;
}
/* 一键清空: 删除全部非加锁文件, 返回成功删除数。 */
int video_dec_delete_all(void)
{
    /* 删除前关闭当前显示管线(见 delete_selected 注释) */
    if (g_vd.photo_showing) {
        vd_close_photo_if_showing();
    }
    if (g_vd.video_playing || g_vd.video_paused) {
        video_dec_stop_video();
    }
    /* 逐个删 index 0 因 B+ 树遍历顺序错乱不可靠, 改用 flm_free_all 物理删除全部(写保护跳过) */
    int deleted = flm_free_all();
    log_info("delete_all: deleted=%d\n", deleted);
    vd_scan_and_load();
    if (flm_get_count() > 0) {
        video_dec_play(g_vd.current_index);  /* 仍有(加锁)文件则显示首个, 不留黑屏 */
    } else {
        video_dec_ui_refresh_grid(NULL, 0, -1);
    }
    return deleted;
}

/*==============================================================================
 * File operations
 *==============================================================================*/
int video_dec_scan_files(video_dec_file_t **list, int *count)
{
    if (list) {
        *list = NULL;
    }
    if (count) {
        *count = flm_get_count();
    }
    return 0;
}
void video_dec_free_file_list(video_dec_file_t *list, int count)
{
    (void)list;
    (void)count;
}
/*==============================================================================
 * 缩略图 LRU 缓存管理 (P1)
 *
 * 缩略图每张 128x128x2 = 32KB。无淘汰时浏览多页累积撑爆内存。
 * LRU 上限 VIDEO_DEC_THUMB_CACHE_MAX(20), 淘汰最久未用。
 *==============================================================================*/
/* 将 index 移到 LRU 队首 (最近使用); 若已在队列先移除再入首。 */
static void vd_thumb_lru_touch(int index)
{
    int i, j;
    int file_count = flm_get_count();
    if (index < 0 || index >= file_count) {
        return;
    }
    /* 已在队列: 移到队首 */
    for (i = 0; i < g_vd.thumb_lru_count; i++) {
        if (g_vd.thumb_lru[i] == index) {
            for (j = i; j > 0; j--) {
                g_vd.thumb_lru[j] = g_vd.thumb_lru[j - 1];
            }
            g_vd.thumb_lru[0] = index;
            return;
        }
    }
    /* 不在队列: 若满, 淘汰队尾(最久未用)的缩略图 */
    if (g_vd.thumb_lru_count >= VIDEO_DEC_THUMB_CACHE_MAX) {
        int victim = g_vd.thumb_lru[VIDEO_DEC_THUMB_CACHE_MAX - 1];
        if (victim >= 0 && victim < file_count && g_vd.thumb_data && g_vd.thumb_data[victim]) {
            kc_free(g_vd.thumb_data[victim]);
            g_vd.thumb_data[victim] = NULL;
            log_info("vd_thumb LRU evict index=%d\n", victim);
        }
        g_vd.thumb_lru_count = VIDEO_DEC_THUMB_CACHE_MAX - 1;
    }
    /* 入首: 整体后移 */
    for (j = g_vd.thumb_lru_count; j > 0; j--) {
        g_vd.thumb_lru[j] = g_vd.thumb_lru[j - 1];
    }
    g_vd.thumb_lru[0] = index;
    g_vd.thumb_lru_count++;
}

/* 确保 index 的缩略图已缓存 (生成 + LRU touch + 淘汰)。替代裸 generate_thumbnail。 */
static int vd_thumb_ensure(int index)
{
    int file_count = flm_get_count();
    if (index < 0 || index >= file_count) {
        return -1;
    }
    if (!g_vd.thumb_data || !g_vd.thumb_data[index]) {
        flm_file_t f;
        if (flm_get_entry(index, &f) != 0) {
            return -1;
        }
        if (video_dec_generate_thumbnail(&f) != 0) {
            return -1;
        }
        if (g_vd.thumb_data) {
            g_vd.thumb_data[index] = f.thumbnail;
        }
    }
    vd_thumb_lru_touch(index);
    return 0;
}

int video_dec_generate_thumbnail(video_dec_file_t *file)
{
    /* 缩略图功能未启用(LIST 不渲染缩略图, UI 只显示文件名), 原占位实现每张
     * malloc 32KB 灰块却从不显示, 只带来 294KB 峰值与删除路径泄漏;
     * 此处不分配, 将来实现真缩略图时在此恢复 malloc。 */
    (void)file;
    return 0;
}
int video_dec_get_storage_info(video_dec_storage_t *info)
{
    if (flm_sd_online() != 0) {
        return -1;
    }
    return flm_get_storage_info(
               info ? &info->total_kb : NULL,
               info ? &info->free_kb : NULL,
               info ? &info->photo_count : NULL,
               info ? &info->video_count : NULL);
}
int video_dec_delete_file(const char *path)
{
    return flm_delete_file(path);
}
int video_dec_lock_file(const char *path)
{
    return flm_lock_file(path);
}
int video_dec_unlock_file(const char *path)
{
    return flm_unlock_file(path);
}
int video_dec_is_locked(const char *path)
{
    return flm_is_locked(path);
}
/*==============================================================================
 * Mode init / exit
 *==============================================================================*/
int video_dec_open(void)
{
    return vd_open_playback();
}
int video_dec_init(void)
{
    log_info("video_dec_init\n");
    memset(&g_vd, 0, sizeof(g_vd));
    g_vd.mute_before_enter = app_audio_get_dac_digital_mute();
    if (vd_open_playback() != 0) {
        log_error("video_dec_init: vd_open_playback failed\n");
        return -1;
    }
    video_dec_ui_init(VDEC_PB_WIDTH, VDEC_PB_HEIGHT);
    vd_scan_and_load();
    /* 进回放模式自动解码显示第一个文件(用户无需按OK)。LIST态只有文件名文本无
     * 缩略图渲染, 停在LIST=白屏; 有文件则直接全屏解码显示首张(照片/视频)。 */
    if (flm_get_count() > 0) {
        video_dec_play(g_vd.current_index);
    }
    return 0;
}
void video_dec_exit(void)
{
    if (g_vd.pb_handle) {
        app_playback_video_stop(g_vd.pb_handle);
    }
    g_vd.video_playing = 0;
    g_vd.video_paused = 0;
    video_dec_ui_close_video();
    vd_close_photo_if_showing();
    video_dec_ui_time_hide();
    video_dec_ui_deinit();
    vd_close_playback();
    video_dec_mode_exit_mute_restore();
    vd_free_files();
    g_vd.mode_running = 0;
    g_vd.state = VIDEO_DEC_STATE_IDLE;
}
#endif // CONFIG_ENABLE_VLIST

/*==============================================================================
 * Mode entry function
 *==============================================================================*/
struct app_mode *app_enter_video_dec_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

#ifdef CONFIG_ENABLE_VLIST
    log_info("app_enter_video_dec_mode, arg=%d\n", arg);
    {
        u8 _mode = APP_MODE_VIDEO_DEC;
        syscfg_write(VM_ID_MODE_MEMORY, &_mode, sizeof(_mode));
    }
    if (video_dec_init() != 0) {
        log_error("app_enter_video_dec_mode: init failed\n");
        app_task_switch_to(APP_MODE_VIDEO_REC, 0);
        return NULL;
    }

    g_vd.mode_running = 1;
    while (g_vd.mode_running) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), video_dec_mode_key_table)) {
            os_time_dly(1);
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }
        switch (msg[0]) {
        case MSG_FROM_APP:
            video_dec_app_msg_handler(msg + 1);
            break;
        default:
            break;
        }
        app_default_msg_handler(msg);
    }
    video_dec_ui_on_state_change(g_vd.state, VIDEO_DEC_STATE_IDLE);
    video_dec_exit();
    return next_mode;

#else
    log_error("app_enter_video_dec_mode: failed: CONFIG_ENABLE_VLIST is not defined\n");
    app_task_switch_to(APP_MODE_VIDEO_REC, 0);
    return NULL;

#endif // CONFIG_ENABLE_VLIST

}
/*==============================================================================
 * Mode registration
 *==============================================================================*/
static int video_dec_mode_try_enter(int arg)
{
    log_info("video_dec_mode_try_enter\n");
    return 0;
}
static int video_dec_mode_try_exit(void)
{
    log_info("video_dec_mode_try_exit\n");
    return 0;
}
static const struct app_mode_ops video_dec_mode_ops = {
    .try_enter  = video_dec_mode_try_enter,
    .try_exit   = video_dec_mode_try_exit,
};
REGISTER_APP_MODE(video_dec_mode) = {
    .name   = APP_MODE_VIDEO_DEC,
    .index  = APP_MODE_VIDEO_DEC_INDEX,
    .ops    = &video_dec_mode_ops,
};
struct app_mode __video_dec_mode;

