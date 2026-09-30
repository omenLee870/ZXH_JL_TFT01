#include "typedef.h"
#include "app_main.h"
#include "app_config.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "app_tone.h"
#include "volume_node.h"
#include "mix_record_api.h"
#include "system/includes.h"
#include "app_action.h"
#include "default_event_handler.h"
#include "bt.h"
#include "tone_player.h"
#include "dev_manager.h"
#include "file_recorder.h"

#if TCFG_MIX_RECORD_ENABLE

/* 录音最少需要的空闲空间(KB)，低于此值不允许开始录音 */
#define MIN_FREE_SPACE_KB_FOR_RECORD       (1024)  /* 1M */
/* 录音过程中低于此值立即停止，预留空间完成文件封头保存 */
#define MIN_FREE_SPACE_KB_RECORD_CRITICAL  (256)   /* 256K */
/* 录音中剩余空间检查间隔(ms) */
#define RECORD_SPACE_CHECK_INTERVAL_MS     5000

struct mix_recode_handle {
    void *recorder;
    FILE *old_file;
    FILE *new_file;
    char logo[20];
    char path[64];		//当前录音文件的完整路径, 用于停止时清理空文件
    u16 space_timer;	//剩余空间检查定时器ID
    int status;
    const char *suffix;
};

static struct mix_recode_handle g_rec_hdl = {
    .status = 0,
};

int mix_record_device_msg_handler(int *msg)
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
        if (msg[1] == DEVICE_EVENT_OUT && logo && g_rec_hdl.recorder &&
            !strcmp(g_rec_hdl.logo, logo)) {
            mix_recorder_stop();
        }
        break;
    default:
        break;
    }
    return false;
}

//检查录音目标设备的剩余空间是否充足
static bool mix_recorder_check_free_space(const char *root_path, u32 need_kb)
{
    u32 free_kb = 0;

    if (!root_path) {
        printf("[MIX_REC] no active device, cannot get free space\n");
        return false;
    }
    if (fget_free_space(root_path, &free_kb) != 0) {
        printf("[MIX_REC] failed to get free space from %s\n", root_path);
        return false;
    }
    if (free_kb < need_kb) {
        printf("[MIX_REC] low storage: free=%u KB, need=%u KB\n", free_kb, need_kb);
        return false;
    }
    return true;
}

//删除停止录音后残留的0字节文件(SD卡满时编码器一帧都没写进去, 只留下一个空目录项)
static void mix_recorder_remove_empty_file(const char *path)
{
    FILE *file;
    u32 size;

    if (!path || !path[0]) {
        return;
    }
    file = fopen(path, "r");
    if (!file) {
        return;
    }
    size = flen(file);
    fclose(file);

    if (size == 0) {
        printf("[MIX_REC] remove empty file: %s\n", path);
        fdelete_by_name(path);
    }
}

static FILE *mix_recorder_open_file(void)
{
    FILE *file;
    char folder[] = {TCFG_REC_FOLDER_NAME};
    char filename[] = {TCFG_REC_FILE_NAME};
    char path[64] = {0};
    char *root_path;

#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));
#endif

    if (!logo) {
        puts("recorder storage not found\n");
        return NULL;
    }
    snprintf(g_rec_hdl.logo, sizeof(g_rec_hdl.logo), "%s", logo);
    root_path = dev_manager_get_root_path_by_logo(logo);
    if (!root_path) {
        printf("recorder root path not found: %s\n", logo);
        return NULL;
    }

    //空间不足时不要创建文件: fopen("w+")能建出一个0字节文件(不占簇),
    //但编码器第一次写就分配不到簇, 文件既不能播放又占着目录项, 所以先判断再创建
    if (!mix_recorder_check_free_space(root_path, MIN_FREE_SPACE_KB_FOR_RECORD)) {
        printf("recorder no space: %s\n", root_path);
        g_rec_hdl.path[0] = '\0';
        return NULL;
    }

    snprintf(path, sizeof(path), "%s%s/%s.%s", root_path, folder,
             filename, g_rec_hdl.suffix);
    file = fopen(path, "w+");
    if (!file) {
        printf("recorder_open_file_faild: %s\n", path);
        g_rec_hdl.path[0] = '\0';
    } else {
        //先存完整路径(下面 fget_name 会把 path 覆盖成纯文件名)
        snprintf(g_rec_hdl.path, sizeof(g_rec_hdl.path), "%s", path);
        fget_name(file, (u8 *)path, sizeof(path));
        printf("recorder_open_file_suss: %s\n", path);
    }
    return file;
}

//录音中定时检查剩余空间，不足则立即停止录音(保证文件正常封头保存)
static void mix_record_free_space_timer(void *priv)
{
    char *root_path;

    if (!g_rec_hdl.recorder) {
        //录音已停止(如设备离线)，删除自身定时器
        if (g_rec_hdl.space_timer) {
            sys_timer_del(g_rec_hdl.space_timer);
            g_rec_hdl.space_timer = 0;
        }
        return;
    }
    root_path = dev_manager_get_root_path_by_logo(g_rec_hdl.logo);
    if (!mix_recorder_check_free_space(root_path, MIN_FREE_SPACE_KB_RECORD_CRITICAL)) {
        printf("[MIX_REC] critically low storage, stopping record\n");
        mix_recorder_stop();
    }
}

static void file_ops_in_app_task(void *p)
{
    switch ((int)p) {
    case SEAMLESS_OPEN_FILE:
        g_rec_hdl.new_file = mix_recorder_open_file();
        break;
    case SEAMLESS_CHANGE_FILE:
        if (g_rec_hdl.old_file) {
            fclose(g_rec_hdl.old_file);
            g_rec_hdl.old_file = NULL;
        }
        break;
    case -ENOENT:
        mix_recorder_stop();
        break;
    }
}

/* This callback runs in the audio stream task and must not block. */
static int change_file_in_stream_task(void *priv, enum change_file_step step)
{
    int msg[4] = {(int)file_ops_in_app_task, 1, step};

    if (step == SEAMLESS_CHANGE_FILE) {
        g_rec_hdl.old_file = file_recorder_change_file(g_rec_hdl.recorder,
                             g_rec_hdl.new_file);
        if (g_rec_hdl.new_file) {
            g_rec_hdl.new_file = NULL;
        } else {
            msg[2] = -ENOENT;
        }
    }
    return os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

void mix_recorder_start(void)
{
    int err;
    u16 pipeline_uuid;
    u16 node_uuid = NODE_UUID_ADC;
    FILE *file;
    void *recorder;
    struct stream_enc_fmt fmt;
    struct seamless_recording seamless;

    pipeline_uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID,
                                          (int)"mix_recorder");
    if (bt_get_call_status() != BT_CALL_HANGUP) {
        node_uuid = NODE_UUID_ZERO_ACTIVE;
    }
    recorder = file_recorder_open(pipeline_uuid, node_uuid);
    if (!recorder) {
        puts("file_recorder_open_faild\n");
        return;
    }

    err = file_recorder_get_fmt(recorder, &fmt);
    if (err) {
        file_recorder_close(recorder, 0);
        return;
    }

    printf("recorder_fmt: ch %d, sample rate %d\n",
           fmt.channel, fmt.sample_rate);

    switch (fmt.coding_type) {
    case AUDIO_CODING_PCM:
        g_rec_hdl.suffix = "pcm";
        break;
    case AUDIO_CODING_WAV:
        g_rec_hdl.suffix = "wav";
        break;
    case AUDIO_CODING_MP3:
        g_rec_hdl.suffix = "mp3";
        break;
    case AUDIO_CODING_AMR:
        g_rec_hdl.suffix = "amr";
        break;
    default:
        g_rec_hdl.suffix = "bin";
        break;
    }

    file = mix_recorder_open_file();
    if (!file) {
        file_recorder_close(recorder, 0);
        return;
    }
    file_recorder_set_file(recorder, file, NULL);

    memset(&seamless, 0, sizeof(seamless));
    seamless.advance_time = 10;
    seamless.time = 3600;
    seamless.priv = recorder;
    seamless.change_file = change_file_in_stream_task;
    file_recorder_seamless_set(recorder, &seamless);

    err = file_recorder_start(recorder);
    if (err) {
        printf("file_recorder_err: %d\n", err);
        file_recorder_close(recorder, 1);
        mix_recorder_remove_empty_file(g_rec_hdl.path);
        g_rec_hdl.path[0] = '\0';
        return;
    }
    g_rec_hdl.status = 1;
    g_rec_hdl.recorder = recorder;

    //启动剩余空间定时检查，录音过程中空间不足立即停止
    if (g_rec_hdl.space_timer == 0) {
        g_rec_hdl.space_timer = sys_timer_add(NULL, mix_record_free_space_timer,
                                              RECORD_SPACE_CHECK_INTERVAL_MS);
    }
    mem_stats();
}

void mix_recorder_stop(void)
{
    void *recorder = g_rec_hdl.recorder;

    if (g_rec_hdl.space_timer) {
        sys_timer_del(g_rec_hdl.space_timer);
        g_rec_hdl.space_timer = 0;
    }
    file_recorder_stop(recorder, 1);
    g_rec_hdl.status = 0;
    os_time_dly(1);

    if (g_rec_hdl.old_file) {
        fclose(g_rec_hdl.old_file);
        g_rec_hdl.old_file = NULL;
    }
    if (g_rec_hdl.new_file) {
        fdelete(g_rec_hdl.new_file);
        g_rec_hdl.new_file = NULL;
    }
    file_recorder_release(recorder);
    g_rec_hdl.recorder = NULL;

    //录到一半空间不足直接停止时, 一帧都没写进去的文件要清掉
    if (recorder) {
        mix_recorder_remove_empty_file(g_rec_hdl.path);
    }
    g_rec_hdl.path[0] = '\0';
}

int get_mix_recorder_status(void)
{
    return g_rec_hdl.status ? 1 : 0;
}

static u8 mix_record_idle_query(void)
{
    return g_rec_hdl.status ? 0 : 1;
}

REGISTER_LP_TARGET(mix_record_lp_target) = {
    .name = "mix_record",
    .is_idle = mix_record_idle_query,
};

#endif /* TCFG_MIX_RECORD_ENABLE */
