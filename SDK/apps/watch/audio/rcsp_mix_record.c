#include "typedef.h"
#include "app_main.h"
#include "app_config.h"
#include "mix_record_api.h"
#include "rcsp_mix_record.h"
#include "system/includes.h"
#include "bt.h"
#include "dev_manager.h"
#include "file_recorder.h"
#include "audio_video_control.h"
#include "rcsp_browser.h"
#include "rtc/rtc_dev.h"

#define LOG_TAG_CONST       RCSP_MIX_RECORD
#define LOG_TAG             "[RCSP-REC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#if RCSP_MIX_RECORD_ENABLE

#define RCSP_MIX_RECORD_STOP_RSP_TIMEOUT_MS    1000
#define RCSP_MIX_RECORD_START_TIMEOUT_MS       2000

struct rcsp_mix_record_file_handle {
    struct file_recorder *recorder;
    FILE *old_file;
    FILE *new_file;
    u8 call_record;
    u8 status;
    char logo[20];
    const char *suffix;
    char file_path[64];
    char new_file_path[64];
};

struct rcsp_mix_record_handle {
    u8 session_id;
    u8 active;
    u8 start_pending;
    u8 stop_pending;
    u8 stream_started;
    u8 start_response_sent;
    u8 recording_notified;
    u8 defer_stop_notify;
    u8 resource_pending;
    u8 device_stop_pending;
    u8 device_stop_reason;
    u8 discard_resource;
    u16 timer;
    u16 start_timer;
    u16 device_stop_timer;
    u32 start_ms;
    u32 last_duration_sec;
    u32 min_duration_sec;
    u32 max_duration_sec;
    u32 resource_storage_handle;
    u32 resource_cluster;
    u32 resource_file_size;
};

static struct rcsp_mix_record_file_handle g_rcsp_file;
static struct rcsp_mix_record_handle g_rcsp_rec;
static const u8 rcsp_mix_record_resource_path[] = "/" TCFG_REC_FOLDER_NAME;

static u32 rcsp_mix_record_duration(void);
static void rcsp_mix_record_stop_from_device(u8 reason, u8 discard_resource);
static void rcsp_mix_record_start_timeout(void *priv);

static u8 rcsp_mix_record_take_resource(struct rcsp_audio_video_resource *resource)
{
    if (!resource || !g_rcsp_rec.resource_pending) {
        return RCSP_AV_RESULT_UNSUPPORTED;
    }

    resource->operation = RCSP_AV_OP_AUDIO_START;
    resource->session_id = g_rcsp_rec.session_id;
    resource->file_type = 0;
    resource->storage_handle = g_rcsp_rec.resource_storage_handle;
    resource->cluster = g_rcsp_rec.resource_cluster;
    resource->file_size = g_rcsp_rec.resource_file_size;
    resource->crc = 0;
    resource->file_path = rcsp_mix_record_resource_path;
    resource->file_path_len = sizeof(rcsp_mix_record_resource_path) - 1;
    g_rcsp_rec.resource_pending = 0;
    return RCSP_AV_RESULT_SUCCESS;
}

static void rcsp_mix_record_notify_resource(void)
{
    struct rcsp_audio_video_resource resource;

    if (rcsp_mix_record_take_resource(&resource) == RCSP_AV_RESULT_SUCCESS) {
        rcsp_audio_video_notify_resource(&resource);
    }
}

static u32 rcsp_mix_record_get_storage_handle(const char *logo)
{
    if (!logo) {
        return (u32) -1;
    }
    if (!strncmp(logo, "udisk", 5)) {
        return RCSPDevMapUDISK;
    }
    if (!strcmp(logo, "sd0")) {
        return RCSPDevMapSD0;
    }
    if (!strcmp(logo, "sd1")) {
        return RCSPDevMapSD1;
    }
    if (!strcmp(logo, "virfat_flash") || !strcmp(logo, "rec_sdfile")) {
        return RCSPDevMapFLASH;
    }
    if (!strcmp(logo, "fat_nor") || !strcmp(logo, "rec_nor")) {
        return RCSPDevMapFLASH_2;
    }
    return (u32) -1;
}

static void rcsp_mix_record_mark_resource_error(void)
{
    g_rcsp_rec.resource_pending = 0;
    g_rcsp_rec.discard_resource = 1;
    if (g_rcsp_rec.device_stop_pending &&
        g_rcsp_rec.device_stop_reason != RCSP_AV_STOP_STORAGE_OFFLINE) {
        g_rcsp_rec.device_stop_reason = RCSP_AV_STOP_IO_ERROR;
    }
}

static void rcsp_mix_record_prepare_resource(void)
{
    FILE *file;
    u32 storage_handle;
    u32 duration;
    struct vfs_attr attr = {0};

    g_rcsp_rec.resource_pending = 0;
    if (!g_rcsp_file.file_path[0]) {
        return;
    }

    storage_handle = rcsp_mix_record_get_storage_handle(g_rcsp_file.logo);
    if (storage_handle == (u32) -1) {
        log_error("record resource unknown storage: %s\n", g_rcsp_file.logo);
        rcsp_mix_record_mark_resource_error();
        return;
    }

    file = fopen(g_rcsp_file.file_path, "r");
    if (!file) {
        log_error("record resource open fail: %s\n", g_rcsp_file.file_path);
        rcsp_mix_record_mark_resource_error();
        return;
    }
    if (fget_attrs(file, &attr)) {
        log_error("record resource attr fail: %s\n", g_rcsp_file.file_path);
        fclose(file);
        rcsp_mix_record_mark_resource_error();
        return;
    }

    duration = rcsp_mix_record_duration();
    if (!attr.sclust || !attr.fsize || duration < g_rcsp_rec.min_duration_sec) {
        log_info("delete invalid record: cluster=0x%x, size=%u, duration=%u, path=%s\n",
                 attr.sclust, attr.fsize, duration, g_rcsp_file.file_path);
        fclose(file);
        if (!attr.sclust || !attr.fsize) {
            rcsp_mix_record_mark_resource_error();
        }
        if (fdelete_by_name(g_rcsp_file.file_path)) {
            log_error("delete invalid record fail: %s\n", g_rcsp_file.file_path);
        }
        g_rcsp_file.file_path[0] = '\0';
        return;
    }
    fclose(file);

    g_rcsp_rec.resource_storage_handle = storage_handle;
    g_rcsp_rec.resource_cluster = attr.sclust;
    g_rcsp_rec.resource_file_size = attr.fsize;
    g_rcsp_rec.resource_pending = 1;
    log_info("record resource: handle=%u, cluster=0x%x, size=%u, path=%s\n",
             storage_handle, attr.sclust, attr.fsize,
             (const char *)rcsp_mix_record_resource_path);
}

static u32 rcsp_mix_record_duration(void)
{
    if (!g_rcsp_rec.active) {
        return g_rcsp_rec.last_duration_sec;
    }
    if (!g_rcsp_rec.recording_notified) {
        return 0;
    }
    return (sys_timer_get_ms() - g_rcsp_rec.start_ms) / 1000;
}

static void rcsp_mix_record_notify_state(u8 state)
{
    struct rcsp_audio_video_state av_state = {
        .session_id = g_rcsp_rec.session_id,
        .state = state,
        .duration_sec = rcsp_mix_record_duration(),
    };

    rcsp_audio_video_notify_audio_state(&av_state);
}

static void rcsp_mix_record_timer(void *priv)
{
    u32 duration;

    (void)priv;
    if (!g_rcsp_rec.active || !g_rcsp_file.status) {
        return;
    }

    duration = rcsp_mix_record_duration();
    rcsp_mix_record_notify_state(RCSP_AUDIO_STATE_RECORDING);
    if (g_rcsp_rec.max_duration_sec &&
        duration >= g_rcsp_rec.max_duration_sec &&
        !g_rcsp_rec.stop_pending &&
        !g_rcsp_rec.device_stop_pending) {
        rcsp_mix_record_stop_from_device(RCSP_AV_STOP_TIMEOUT, 0);
    }
}

static void rcsp_mix_record_finish(void)
{
    u32 duration;
    u8 notify_state;
    struct rcsp_audio_video_state av_state;

    if (!g_rcsp_rec.active) {
        if (g_rcsp_rec.start_timer) {
            sys_timeout_del(g_rcsp_rec.start_timer);
            g_rcsp_rec.start_timer = 0;
        }
        g_rcsp_rec.start_pending = 0;
        g_rcsp_rec.stop_pending = 0;
        g_rcsp_rec.stream_started = 0;
        g_rcsp_rec.start_response_sent = 0;
        g_rcsp_rec.recording_notified = 0;
        g_rcsp_rec.defer_stop_notify = 0;
        return;
    }

    duration = rcsp_mix_record_duration();
    notify_state = !g_rcsp_rec.defer_stop_notify;
    if (g_rcsp_rec.timer) {
        sys_timer_del(g_rcsp_rec.timer);
        g_rcsp_rec.timer = 0;
    }
    if (g_rcsp_rec.start_timer) {
        sys_timeout_del(g_rcsp_rec.start_timer);
        g_rcsp_rec.start_timer = 0;
    }

    av_state.session_id = g_rcsp_rec.session_id;
    av_state.state = RCSP_AUDIO_STATE_STOPPED;
    av_state.duration_sec = duration;
    if (notify_state) {
        rcsp_audio_video_notify_audio_state(&av_state);
        if (!g_rcsp_rec.discard_resource) {
            rcsp_mix_record_notify_resource();
        } else {
            g_rcsp_rec.resource_pending = 0;
        }
    }

    g_rcsp_rec.active = 0;
    g_rcsp_rec.last_duration_sec = duration;
    g_rcsp_rec.start_pending = 0;
    g_rcsp_rec.stop_pending = 0;
    g_rcsp_rec.stream_started = 0;
    g_rcsp_rec.start_response_sent = 0;
    g_rcsp_rec.recording_notified = 0;
    g_rcsp_rec.defer_stop_notify = 0;
}

static void rcsp_mix_record_complete_device_stop(void)
{
    u8 notify_resource;

    if (!g_rcsp_rec.device_stop_pending) {
        return;
    }
    if (g_rcsp_rec.device_stop_timer) {
        sys_timeout_del(g_rcsp_rec.device_stop_timer);
        g_rcsp_rec.device_stop_timer = 0;
    }

    notify_resource = !g_rcsp_rec.discard_resource;
    g_rcsp_rec.device_stop_pending = 0;
    g_rcsp_rec.device_stop_reason = RCSP_AV_STOP_ACTIVE;
    g_rcsp_rec.discard_resource = 0;
    rcsp_mix_record_notify_state(RCSP_AUDIO_STATE_STOPPED);
    if (notify_resource) {
        rcsp_mix_record_notify_resource();
    } else {
        g_rcsp_rec.resource_pending = 0;
    }
}

static void rcsp_mix_record_device_stop_timeout(void *priv)
{
    (void)priv;
    g_rcsp_rec.device_stop_timer = 0;
    log_error("rcsp record stop response timeout\n");
    rcsp_mix_record_complete_device_stop();
}

static void rcsp_mix_record_send_device_stop(void)
{
    JL_ERR err;

    if (!g_rcsp_rec.device_stop_pending) {
        return;
    }
    if (!g_rcsp_rec.device_stop_timer) {
        g_rcsp_rec.device_stop_timer = sys_timeout_add(
                                           NULL,
                                           rcsp_mix_record_device_stop_timeout,
                                           RCSP_MIX_RECORD_STOP_RSP_TIMEOUT_MS);
    }

    err = rcsp_audio_video_request_audio_stop(g_rcsp_rec.session_id,
          g_rcsp_rec.device_stop_reason);
    log_info("rcsp record stop request: session=%u, reason=%u, err=%d\n",
             g_rcsp_rec.session_id, g_rcsp_rec.device_stop_reason, err);
    if (err != JL_ERR_NONE) {
        log_error("rcsp record stop request send fail: %d\n", err);
        rcsp_mix_record_complete_device_stop();
    }
}

static u32 rcsp_mix_record_get_file_timestamp(void)
{
    struct sys_time time = {0};

    rtc_read_time(&time);
    if (time.year < 2010 || time.year > (2010 + 0x3f) ||
        time.month < 1 || time.month > 12 ||
        time.day < 1 || time.day > 31 ||
        time.hour > 23 || time.min > 59 || time.sec > 59) {
        memset(&time, 0, sizeof(time));
        time.year = 2010;
        time.month = 1;
        time.day = 1;
        datetime_add_seconds(&time, sys_timer_get_ms() / 1000);
    }

    return ((u32)(time.sec & 0x3f)) |
           ((u32)(time.min & 0x3f) << 6) |
           ((u32)(time.hour & 0x1f) << 12) |
           ((u32)(time.day & 0x1f) << 17) |
           ((u32)(time.month & 0x0f) << 22) |
           ((u32)((time.year - 2010) & 0x3f) << 26);
}

static void rcsp_mix_record_timestamp_to_base36(u32 timestamp, char out[8])
{
    static const char base36_alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;

    for (i = 6; i >= 0; i--) {
        out[i] = base36_alphabet[timestamp % 36u];
        timestamp /= 36u;
    }
    out[7] = '\0';
}

static FILE *rcsp_mix_recorder_open_file(char *record_path, u16 path_size)
{
    FILE *file;
    char folder[] = {TCFG_REC_FOLDER_NAME};
    char filename[9];
    char path[64] = {0};
    char *root_path;
    u32 timestamp = rcsp_mix_record_get_file_timestamp();

#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));
#endif

    if (!logo) {
        if (record_path && path_size) {
            record_path[0] = '\0';
        }
        log_error("rcsp recorder storage not found\n");
        return NULL;
    }
    snprintf(g_rcsp_file.logo, sizeof(g_rcsp_file.logo), "%s", logo);

    root_path = dev_manager_get_root_path_by_logo(logo);
    if (!root_path) {
        if (record_path && path_size) {
            record_path[0] = '\0';
        }
        log_error("rcsp recorder root path not found: %s\n", logo);
        return NULL;
    }

    filename[0] = g_rcsp_file.call_record ? 'C' : 'L';
    rcsp_mix_record_timestamp_to_base36(timestamp, filename + 1);
    snprintf(path, sizeof(path), "%s%s/%s.%s", root_path, folder,
             filename, g_rcsp_file.suffix);
    if (record_path && path_size) {
        snprintf(record_path, path_size, "%s", path);
    }

    file = fopen(path, "w+");
    if (!file) {
        if (record_path && path_size) {
            record_path[0] = '\0';
        }
        log_error("rcsp_recorder_open_file_faild: %s\n", path);
    } else {
        fget_name(file, (u8 *)path, sizeof(path));
        log_info("rcsp_recorder_open_file_suss: %s\n", path);
    }
    return file;
}

static void rcsp_mix_record_delete_current_file(void)
{
    if (!g_rcsp_file.file_path[0]) {
        return;
    }
    if (fdelete_by_name(g_rcsp_file.file_path)) {
        log_error("delete failed record fail: %s\n", g_rcsp_file.file_path);
    }
    g_rcsp_file.file_path[0] = '\0';
}

static void rcsp_mix_record_file_ops_in_app_task(void *p)
{
    switch ((int)p) {
    case SEAMLESS_OPEN_FILE:
        g_rcsp_file.new_file = rcsp_mix_recorder_open_file(
                                   g_rcsp_file.new_file_path,
                                   sizeof(g_rcsp_file.new_file_path));
        break;
    case SEAMLESS_CHANGE_FILE:
        if (g_rcsp_file.old_file) {
            fclose(g_rcsp_file.old_file);
            g_rcsp_file.old_file = NULL;
        }
        break;
    case -ENOENT:
        rcsp_mix_record_stop_from_device(RCSP_AV_STOP_IO_ERROR, 1);
        break;
    }
}

static int rcsp_mix_record_change_file_in_stream_task(void *priv,
        enum change_file_step step)
{
    int msg[4] = {(int)rcsp_mix_record_file_ops_in_app_task, 1, step};

    if (step == SEAMLESS_CHANGE_FILE) {
        g_rcsp_file.old_file = file_recorder_change_file(
                                   g_rcsp_file.recorder,
                                   g_rcsp_file.new_file);
        if (g_rcsp_file.new_file) {
            snprintf(g_rcsp_file.file_path, sizeof(g_rcsp_file.file_path),
                     "%s", g_rcsp_file.new_file_path);
            g_rcsp_file.new_file_path[0] = '\0';
            g_rcsp_file.new_file = NULL;
        } else {
            msg[2] = -ENOENT;
        }
    }
    return os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

static void rcsp_mix_record_notify_started_if_ready(void)
{
    if (!g_rcsp_rec.active || !g_rcsp_rec.stream_started ||
        g_rcsp_rec.recording_notified) {
        return;
    }
#if RCSP_AUDIO_VIDEO_RESPONSE_FIRST
    if (!g_rcsp_rec.start_response_sent) {
        return;
    }
#endif

    g_rcsp_rec.recording_notified = 1;
    g_rcsp_rec.start_ms = sys_timer_get_ms();
    rcsp_mix_record_notify_state(RCSP_AUDIO_STATE_RECORDING);
    if (!g_rcsp_rec.timer) {
        g_rcsp_rec.timer = sys_timer_add(NULL, rcsp_mix_record_timer, 1000);
    }
}

static void rcsp_mix_recorder_stream_started_in_app_task(void *priv)
{
    struct file_recorder *recorder = (struct file_recorder *)priv;

    if (!recorder || g_rcsp_file.recorder != recorder) {
        return;
    }
    if (g_rcsp_rec.start_timer) {
        sys_timeout_del(g_rcsp_rec.start_timer);
        g_rcsp_rec.start_timer = 0;
    }
    g_rcsp_rec.stream_started = 1;
    rcsp_mix_record_notify_started_if_ready();
}

static void rcsp_mix_recorder_stream_fail_in_app_task(void *priv)
{
    struct file_recorder *recorder = (struct file_recorder *)priv;
    u8 report_failure;
    u8 start_response_sent;

    if (!recorder || g_rcsp_file.recorder != recorder) {
        return;
    }

    log_error("rcsp mix recorder stream start fail\n");
    if (g_rcsp_rec.start_timer) {
        sys_timeout_del(g_rcsp_rec.start_timer);
        g_rcsp_rec.start_timer = 0;
    }
    report_failure = g_rcsp_rec.active;
    start_response_sent = g_rcsp_rec.start_response_sent;
    if (report_failure) {
        g_rcsp_rec.device_stop_pending = 1;
        g_rcsp_rec.device_stop_reason = RCSP_AV_STOP_IO_ERROR;
        g_rcsp_rec.discard_resource = 1;
        g_rcsp_rec.defer_stop_notify = 1;
    }

    g_rcsp_file.status = 0;
    g_rcsp_file.recorder = NULL;
    file_recorder_close(recorder, 1);
    if (g_rcsp_file.old_file) {
        fclose(g_rcsp_file.old_file);
        g_rcsp_file.old_file = NULL;
    }
    if (g_rcsp_file.new_file) {
        fdelete(g_rcsp_file.new_file);
        g_rcsp_file.new_file = NULL;
        g_rcsp_file.new_file_path[0] = '\0';
    }
    rcsp_mix_record_delete_current_file();
    g_rcsp_rec.resource_pending = 0;
    rcsp_mix_record_finish();
    if (report_failure && start_response_sent) {
        rcsp_mix_record_send_device_stop();
    }
}

static void rcsp_mix_record_start_timeout(void *priv)
{
    struct file_recorder *recorder = g_rcsp_file.recorder;
    int msg[3];

    (void)priv;
    g_rcsp_rec.start_timer = 0;
    if (!recorder || !g_rcsp_rec.active || g_rcsp_rec.stream_started) {
        return;
    }

    log_error("rcsp mix recorder stream start timeout\n");
    msg[0] = (int)rcsp_mix_recorder_stream_fail_in_app_task;
    msg[1] = 1;
    msg[2] = (int)recorder;
    if (os_taskq_post_type("app_core", Q_CALLBACK, 3, msg)) {
        log_error("rcsp mix recorder start timeout post error\n");
        g_rcsp_rec.start_timer = sys_timeout_add(
                                     NULL,
                                     rcsp_mix_record_start_timeout,
                                     100);
    }
}

static void rcsp_mix_recorder_stream_callback(void *priv,
        enum stream_state state)
{
    int msg[3];

    if (state == STREAM_EVENT_START) {
        msg[0] = (int)rcsp_mix_recorder_stream_started_in_app_task;
    } else if (state == STREAM_EVENT_NEGOTIATE_FAILD) {
        msg[0] = (int)rcsp_mix_recorder_stream_fail_in_app_task;
    } else if ((state == STREAM_EVENT_STOP || state == STREAM_EVENT_PREEMPTED) &&
               !g_rcsp_rec.stop_pending &&
               !g_rcsp_rec.device_stop_pending) {
        msg[0] = (int)rcsp_mix_recorder_stream_fail_in_app_task;
    } else {
        return;
    }

    msg[1] = 1;
    msg[2] = (int)priv;
    if (os_taskq_post_type("app_core", Q_CALLBACK, 3, msg)) {
        log_error("rcsp mix recorder stream event post error\n");
    }
}

static int rcsp_mix_recorder_start_internal(void)
{
    int err;
    u16 pipeline_uuid;
    u16 node_uuid;
    FILE *file;
    struct file_recorder *recorder;
    struct stream_enc_fmt fmt;
    struct seamless_recording seamless;

    if (g_rcsp_file.recorder) {
        return -EBUSY;
    }

    pipeline_uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID,
                                          (int)"mix_recorder");
    g_rcsp_file.call_record = (bt_get_call_status() != BT_CALL_HANGUP);
    g_rcsp_file.file_path[0] = '\0';
    g_rcsp_file.new_file_path[0] = '\0';
    node_uuid = g_rcsp_file.call_record ? NODE_UUID_ZERO_ACTIVE : NODE_UUID_ADC;
    recorder = file_recorder_open(pipeline_uuid, node_uuid);
    if (!recorder) {
        log_error("rcsp file_recorder_open_faild\n");
        return -ENODEV;
    }

    err = file_recorder_get_fmt(recorder, &fmt);
    if (err) {
        file_recorder_close(recorder, 0);
        return err;
    }
    /*RCSP 默认用OPUS 使用代码强制固定*/
    fmt.sample_rate = 16000;
    fmt.bit_rate = 16000;
    fmt.frame_dms = 20 * 10;
    fmt.coding_type = AUDIO_CODING_OPUS;
    err = file_recorder_set_fmt(recorder, &fmt);
    if (err) {
        log_error("rcsp file_recorder_set_fmt_err: %d\n", err);
        file_recorder_close(recorder, 0);
        return err;
    }

    if (fmt.coding_type == AUDIO_CODING_OPUS) {
        struct encoder_fmt enc_fmt = {0};
        enc_fmt.ops_type = OPUS_TYPE_SILK;
        enc_fmt.frame_dms = 20 * 10;
        err = file_recorder_set_priv_fmt(recorder, &enc_fmt);
        if (err) {
            log_error("rcsp file_recorder_set_priv_fmt_err: %d\n", err);
            file_recorder_close(recorder, 0);
            return err;
        }
    }

    log_info("rcsp recorder_fmt: coding 0x%x, ch %d, sample rate %d, bit rate %d, frame dms %d\n",
             fmt.coding_type, fmt.channel, fmt.sample_rate,
             fmt.bit_rate, fmt.frame_dms);
    switch (fmt.coding_type) {
    case AUDIO_CODING_PCM:
        g_rcsp_file.suffix = "pcm";
        break;
    case AUDIO_CODING_WAV:
        g_rcsp_file.suffix = "wav";
        break;
    case AUDIO_CODING_MP3:
        g_rcsp_file.suffix = "mp3";
        break;
    case AUDIO_CODING_AMR:
        g_rcsp_file.suffix = "amr";
        break;
    case AUDIO_CODING_OPUS:
        g_rcsp_file.suffix = "ops";
        break;
    default:
        g_rcsp_file.suffix = "bin";
        break;
    }

    file = rcsp_mix_recorder_open_file(g_rcsp_file.file_path,
                                       sizeof(g_rcsp_file.file_path));
    if (!file) {
        file_recorder_close(recorder, 0);
        return -ENOENT;
    }
    file_recorder_set_file(recorder, file, NULL);

    memset(&seamless, 0, sizeof(seamless));
    seamless.advance_time = 10;
    seamless.time = 3600;
    seamless.priv = recorder;
    seamless.change_file = rcsp_mix_record_change_file_in_stream_task;
    file_recorder_seamless_set(recorder, &seamless);
    file_recorder_set_callback(recorder, recorder,
                               rcsp_mix_recorder_stream_callback);

    g_rcsp_file.recorder = recorder;
    err = file_recorder_start(recorder);
    if (err) {
        log_error("rcsp file_recorder_err: %d\n", err);
        rcsp_mix_recorder_stream_fail_in_app_task(recorder);
        return err;
    }
    if (g_rcsp_file.recorder != recorder) {
        return -EIO;
    }
    g_rcsp_file.status = 1;
    mem_stats();
    return 0;
}

static void rcsp_mix_recorder_stop_internal(void)
{
    int stop_err;
    struct file_recorder *recorder = g_rcsp_file.recorder;

    if (!recorder) {
        rcsp_mix_record_finish();
        return;
    }

    stop_err = file_recorder_stop(recorder, 1);
    g_rcsp_file.status = 0;
    os_time_dly(1);
    if (g_rcsp_file.old_file) {
        fclose(g_rcsp_file.old_file);
        g_rcsp_file.old_file = NULL;
    }
    if (g_rcsp_file.new_file) {
        fdelete(g_rcsp_file.new_file);
        g_rcsp_file.new_file = NULL;
        g_rcsp_file.new_file_path[0] = '\0';
    }
    file_recorder_release(recorder);
    g_rcsp_file.recorder = NULL;

    if (stop_err) {
        log_error("rcsp mix recorder close file error: %d\n", stop_err);
        rcsp_mix_record_mark_resource_error();
        rcsp_mix_record_delete_current_file();
    } else {
        rcsp_mix_record_prepare_resource();
    }
    rcsp_mix_record_finish();
}

static void rcsp_mix_record_stop_from_device(u8 reason, u8 discard_resource)
{
    if (!g_rcsp_rec.active || !g_rcsp_file.recorder) {
        rcsp_mix_recorder_stop_internal();
        return;
    }
    if (g_rcsp_rec.device_stop_pending) {
        return;
    }

    g_rcsp_rec.device_stop_pending = 1;
    g_rcsp_rec.device_stop_reason = reason;
    g_rcsp_rec.discard_resource = discard_resource;
    g_rcsp_rec.defer_stop_notify = 1;
    rcsp_mix_recorder_stop_internal();
    rcsp_mix_record_send_device_stop();
}

int rcsp_mix_recorder_get_status(void)
{
    return g_rcsp_file.status ? 1 : 0;
}

void rcsp_mix_recorder_stop(void)
{
    if (g_rcsp_rec.active && !g_rcsp_rec.stop_pending &&
        !g_rcsp_rec.device_stop_pending) {
        rcsp_mix_record_stop_from_device(RCSP_AV_STOP_ACTIVE, 0);
    } else if (g_rcsp_file.recorder) {
        rcsp_mix_recorder_stop_internal();
    }
}

int rcsp_mix_record_device_msg_handler(int *msg)
{
    const char *logo = NULL;

    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        logo = (char *)msg[2];
    /* fall through */
    case DEVICE_EVENT_FROM_USB_HOST:
        if (!strncmp((char *)msg[2], "udisk", 5)) {
            logo = (char *)msg[2];
        }
        if (msg[1] == DEVICE_EVENT_OUT && logo && g_rcsp_file.recorder &&
            !strcmp(g_rcsp_file.logo, logo)) {
            rcsp_mix_record_stop_from_device(RCSP_AV_STOP_STORAGE_OFFLINE, 1);
        }
        break;
    default:
        break;
    }
    return false;
}

static u8 rcsp_mix_record_get_config(u8 *buf, u16 buf_size, u16 *out_len)
{
    if (!buf || !out_len || buf_size < 10) {
        return RCSP_AV_RESULT_UNSUPPORTED;
    }

    buf[0] = 9;
    buf[1] = RCSP_AV_CFG_RECORD_DURATION;
    WRITE_BIG_U32(buf + 2, g_rcsp_rec.min_duration_sec);
    WRITE_BIG_U32(buf + 6, g_rcsp_rec.max_duration_sec);
    *out_len = 10;
    return RCSP_AV_RESULT_SUCCESS;
}

static u8 rcsp_mix_record_set_config(const u8 *ltv, u16 len)
{
    u16 offset = 0;
    u32 min_duration = g_rcsp_rec.min_duration_sec;
    u32 max_duration = g_rcsp_rec.max_duration_sec;

    if (g_rcsp_rec.active || g_rcsp_rec.start_pending ||
        g_rcsp_rec.device_stop_pending) {
        return RCSP_AV_RESULT_BUSY;
    }

    while (offset < len) {
        u8 ltv_len = ltv[offset];
        u8 type = ltv[offset + 1];

        if (type != RCSP_AV_CFG_RECORD_DURATION || ltv_len != 9) {
            return RCSP_AV_RESULT_UNSUPPORTED;
        }
        min_duration = READ_BIG_U32(ltv + offset + 2);
        max_duration = READ_BIG_U32(ltv + offset + 6);
        offset += 1 + ltv_len;
    }

    if (max_duration && min_duration > max_duration) {
        return RCSP_AV_RESULT_UNSUPPORTED;
    }
    g_rcsp_rec.min_duration_sec = min_duration;
    g_rcsp_rec.max_duration_sec = max_duration;
    return RCSP_AV_RESULT_SUCCESS;
}

static u8 rcsp_mix_record_start_request(u8 session_id, u8 device_number)
{
    u8 mode = app_get_current_mode_name();

    if (device_number != RCSP_AV_DEVICE_NUMBER_DEFAULT && device_number != 0) {
        return RCSP_AV_RESULT_UNSUPPORTED;
    }
    if (mode == APP_MODE_MUSIC || mode == APP_MODE_PC) {
        return RCSP_AV_RESULT_BUSY;
    }
    if (g_rcsp_rec.start_pending || g_rcsp_rec.active ||
        g_rcsp_rec.device_stop_pending || g_rcsp_file.status ||
        get_mix_recorder_status()) {
        return RCSP_AV_RESULT_IN_PROGRESS;
    }

    g_rcsp_rec.session_id = session_id;
    g_rcsp_rec.start_pending = 1;
    g_rcsp_rec.stream_started = 0;
    g_rcsp_rec.start_response_sent = 0;
    g_rcsp_rec.recording_notified = 0;
    g_rcsp_rec.stop_pending = 0;
    g_rcsp_rec.defer_stop_notify = 0;
    g_rcsp_rec.resource_pending = 0;
    g_rcsp_rec.device_stop_reason = RCSP_AV_STOP_ACTIVE;
    g_rcsp_rec.discard_resource = 0;
    g_rcsp_rec.last_duration_sec = 0;
    if (rcsp_mix_recorder_start_internal()) {
        g_rcsp_rec.start_pending = 0;
        g_rcsp_file.file_path[0] = '\0';
        g_rcsp_file.new_file_path[0] = '\0';
        return RCSP_AV_RESULT_BUSY;
    }
    g_rcsp_rec.start_pending = 0;
    g_rcsp_rec.active = 1;
    return RCSP_AV_RESULT_SUCCESS;
}

static void rcsp_mix_record_start_response_sent(u8 result)
{
    if (result != RCSP_AV_RESULT_SUCCESS) {
        return;
    }
    g_rcsp_rec.start_response_sent = 1;
    if (g_rcsp_rec.device_stop_pending) {
        rcsp_mix_record_send_device_stop();
        return;
    }
    if (!g_rcsp_rec.active) {
        return;
    }
    if (!g_rcsp_rec.stream_started && !g_rcsp_rec.start_timer) {
        g_rcsp_rec.start_timer = sys_timeout_add(
                                     NULL,
                                     rcsp_mix_record_start_timeout,
                                     RCSP_MIX_RECORD_START_TIMEOUT_MS);
    }
    rcsp_mix_record_notify_started_if_ready();
}

static void rcsp_mix_record_stop_response(u8 status, u8 result)
{
    if (!g_rcsp_rec.device_stop_pending) {
        return;
    }
    log_info("rcsp record stop response: status=%u, result=%u\n", status, result);
    rcsp_mix_record_complete_device_stop();
}

static u8 rcsp_mix_record_stop_request(u8 session_id, u8 reason)
{
    if (session_id != g_rcsp_rec.session_id) {
        return RCSP_AV_RESULT_UNSUPPORTED;
    }
    if (g_rcsp_rec.stop_pending || g_rcsp_rec.device_stop_pending) {
        return RCSP_AV_RESULT_IN_PROGRESS;
    }
    if (!g_rcsp_rec.active || !g_rcsp_file.status) {
        return RCSP_AV_RESULT_IN_PROGRESS;
    }

    g_rcsp_rec.stop_pending = 1;
    if (reason == RCSP_AV_STOP_STORAGE_OFFLINE ||
        reason == RCSP_AV_STOP_IO_ERROR) {
        g_rcsp_rec.discard_resource = 1;
    }
#if RCSP_AUDIO_VIDEO_RESPONSE_FIRST
    g_rcsp_rec.defer_stop_notify = 1;
#endif
    rcsp_mix_recorder_stop_internal();
    if (reason == RCSP_AV_STOP_STORAGE_OFFLINE ||
        reason == RCSP_AV_STOP_IO_ERROR) {
        g_rcsp_rec.resource_pending = 0;
    }
    if (reason == RCSP_AV_STOP_IO_ERROR) {
        rcsp_mix_record_delete_current_file();
    }
    return RCSP_AV_RESULT_SUCCESS;
}

static u8 rcsp_mix_record_get_state(struct rcsp_audio_video_state *state)
{
    if (!state) {
        return RCSP_AV_RESULT_UNSUPPORTED;
    }

    state->session_id = g_rcsp_rec.session_id;
    state->state = (g_rcsp_rec.active && g_rcsp_rec.recording_notified &&
                    g_rcsp_file.status) ?
                   RCSP_AUDIO_STATE_RECORDING : RCSP_AUDIO_STATE_STOPPED;
    state->duration_sec = rcsp_mix_record_duration();
    return RCSP_AV_RESULT_SUCCESS;
}

static const struct rcsp_audio_video_ops rcsp_mix_record_ops = {
    .get_audio_config = rcsp_mix_record_get_config,
    .set_audio_config = rcsp_mix_record_set_config,
    .start_audio = rcsp_mix_record_start_request,
    .stop_audio = rcsp_mix_record_stop_request,
    .get_audio_state = rcsp_mix_record_get_state,
    .take_audio_resource = rcsp_mix_record_take_resource,
    .audio_start_response_sent = rcsp_mix_record_start_response_sent,
    .audio_stop_response = rcsp_mix_record_stop_response,
};

static int rcsp_mix_record_init(void)
{
    rcsp_audio_video_register_ops(&rcsp_mix_record_ops);
    return 0;
}

late_initcall(rcsp_mix_record_init);

static u8 rcsp_mix_record_idle_query(void)
{
    return g_rcsp_file.status ? 0 : 1;
}

REGISTER_LP_TARGET(rcsp_mix_record_lp_target) = {
    .name = "rcsp_mix_record",
    .is_idle = rcsp_mix_record_idle_query,
};

#endif /* TCFG_MIX_RECORD_ENABLE */
