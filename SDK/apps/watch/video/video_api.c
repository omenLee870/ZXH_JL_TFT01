#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_api.data.bss")
#pragma data_seg(".video_api.data")
#pragma const_seg(".video_api.text.const")
#pragma code_seg(".video_api.text")
#endif

#include "app_camera_mpi.h"
#include "app_config.h"
#include "generic/includes.h"
#include "avctp_user.h"
#include "utils/sys_time.h"
#include "rtc/rtc_dev.h"
#include "btstack/le/ble_api.h"
#include "fs/fs.h"
#include "device/device.h"
#include "video_api.h"
#include "record.h"
#include "mix_record_api.h"
#include "rcsp_mix_record.h"
#if TCFG_APP_MUSIC_EN
#include "file_player.h"
#endif
#include "system/malloc.h"
#include "asm/sdmmc.h"

#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)

#if defined(TCFG_SD0_ENABLE) && (TCFG_SD0_ENABLE == 1)
#define REC_DIR         "storage/sd0/C/video/"
#define REC_BASE_PATH   "storage/sd0/C/"
#define REC_MOUNT_PATH  "mnt/sd0"
#define REC_FOLDER      "video"
#define SD_DEV_NAME     "sd0"
#elif defined(TCFG_SD1_ENABLE) && (TCFG_SD1_ENABLE == 1)
#define REC_DIR         "storage/sd1/C/video/"
#define REC_BASE_PATH   "storage/sd1/C/"
#define REC_MOUNT_PATH  "mnt/sd1"
#define REC_FOLDER      "video"
#define SD_DEV_NAME     "sd1"
#else
#error "Please enable sd0 or sd1 !!!"
#endif

#define VIDEO_FILE_FMT  "VID_%04d.AVI"
#define PHOTO_FILE_FMT  "IMG_%04d.JPG"
#define MAX_FILE_INDEX  9999
/* 目录扫描失败时的重试次数与间隔(os_time_dly 单位: tick) */
#define FILE_SCAN_RETRY_TIMES   3
#define FILE_SCAN_RETRY_DELAY   1
#define VIDEO_CAMERA_ID                0
#ifndef TCFG_VIDEO_SENSOR_NAME
#define TCFG_VIDEO_SENSOR_NAME         "NT99141"
#endif
#define VIDEO_SENSOR_NAME              TCFG_VIDEO_SENSOR_NAME
#define VIDEO_PHOTO_QUALITY            80
#define VIDEO_PHOTO_BUF_EXTRA          (1 * 1024)
#define VIDEO_MEM_ESTIMATE_MARGIN       (1 * 1024)
#define VIDEO_REC_CYC_SEC              30
#define VIDEO_AUDIO_BUF_SIZE           (64 * 1024)
#define VIDEO_AUDIO_FRAME_INTERVAL_MS  500
#define VIDEO_AUDIO_SAMPLE_RATE        16000
#define VIDEO_AUDIO_BIT_RATE           64000
#define VIDEO_SPACE_CHECK_INTERVAL_SEC  5
#define MIN_FREE_SPACE_KB_FOR_REC      (50 * 1024)  /* 50M，录像至少需要的空闲空间,低于这个限制不进行录像 */
#define MIN_FREE_SPACE_KB_CRITICAL     (20 * 1024)  /* 20M，录像过程中低于此值立即停止，预留空间写AVI索引 */

/* 分辨率 + 录像参数 */
typedef struct {
    u16 width;
    u16 height;
    u32 fps;
    u16 abr_kbps;
    u32 stream_buf_size;
} video_res_entry_t;

/* 录像分辨率表（从高到低） */
static const video_res_entry_t video_res_table[] = {
    {1280, 720, 30, 6000, 300 * 1024},
    {864,  480, 25, 3000, 160 * 1024},
    {640,  480, 25, 2000, 100 * 1024},
    {320,  240, 15,  600,  35 * 1024},
};

/* 仅分辨率（拍照用） */
typedef struct {
    u16 width;
    u16 height;
} video_resolution_t;

/* 拍照分辨率表（从高到低） */
static const video_resolution_t photo_res_table[] = {
    {1280, 720},
    {640,  480},
    {320,  240},
};

static u8 camera_status = CAMERA_STATE_IDLE;
static int rec_path_index = 0;
static int photo_path_index = 0;
static bool file_index_inited = false;
static app_camera_handle_t photo_handle;
static app_camera_handle_t rec_handle;
static u32 rec_last_space_check_sec;

u8 get_camera_status()
{
    return camera_status;
}

static void reset_camera_status()
{
    camera_status = CAMERA_STATE_IDLE;
    rec_last_space_check_sec = 0;
}

static void video_api_close_and_reset(app_camera_handle_t *handle)
{
    if (handle && *handle) {
        app_camera_close(handle);
    }
    reset_camera_status();
}

static bool video_api_is_call_active(int call_status)
{
    return call_status == BT_CALL_INCOMING ||
           call_status == BT_CALL_OUTGOING ||
           call_status == BT_CALL_ACTIVE;
}

/**
 * @brief 检查 SD 卡设备是否在线
 * @return true=在线, false=不在线
 */
static bool video_api_sd_is_online(void)
{
    if (!dev_online(SD_DEV_NAME)) {
        printf("[VIDEO_API] SD card (%s) not online\n", SD_DEV_NAME);
        return false;
    }
    return true;
}

/**
 * @brief 检查存储空间是否充足
 * @param need_kb 需要的空间，单位 KB
 * @return true=空间充足, false=空间不足
 */
static bool video_api_check_free_space(u32 need_kb)
{
    u32 free_kb = 0;

    if (fget_free_space(REC_BASE_PATH, &free_kb) != 0 &&
        fget_free_space(REC_MOUNT_PATH, &free_kb) != 0) {
        printf("[VIDEO_API] failed to get free space from %s/%s\n",
               REC_BASE_PATH, REC_MOUNT_PATH);
        return false;
    }

    if (free_kb < need_kb) {
        printf("[VIDEO_API] low storage: free=%u KB, need=%u KB\n", free_kb, need_kb);
        return false;
    }
    return true;
}

/**
 * @brief 从 filename 的指定偏移量解析4位十进制数字
 * @param s 指向4位数字起始位置的指针
 * @return 解析出的数字 (0~9999), 非数字字符返回 -1
 */
static int parse_four_digits(const char *s)
{
    if (s[0] < '0' || s[0] > '9') {
        return -1;
    }
    if (s[1] < '0' || s[1] > '9') {
        return -1;
    }
    if (s[2] < '0' || s[2] > '9') {
        return -1;
    }
    if (s[3] < '0' || s[3] > '9') {
        return -1;
    }
    return (s[0] - '0') * 1000 +
           (s[1] - '0') * 100 +
           (s[2] - '0') * 10 +
           (s[3] - '0');
}

static bool video_api_char_equal_nocase(char a, char b)
{
    if (a >= 'a' && a <= 'z') {
        a -= 'a' - 'A';
    }
    if (b >= 'a' && b <= 'z') {
        b -= 'a' - 'A';
    }
    return a == b;
}

static int video_api_parse_index_with_prefix(const char *name, const char *prefix)
{
    int i = 0;

    while (prefix[i] != '\0') {
        if (!video_api_char_equal_nocase(name[i], prefix[i])) {
            return -1;
        }
        i++;
    }

    if (name[i] == '\0' || name[i + 1] == '\0' ||
        name[i + 2] == '\0' || name[i + 3] == '\0') {
        return -1;
    }

    return parse_four_digits(&name[i]);
}

/**
 * @brief 判断文件是否已存在
 *
 * 录像文件名必须保证不撞已有文件: AVI_open_output_file() 内部用
 * O_CREAT|O_TRUNC 打开, 同名文件会被直接截断, 旧录像就没了。
 *
 * @return true=已存在, false=不存在
 */
static bool video_api_file_exist(const char *path)
{
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        return false;
    }
    fclose(fp);     /* 必须关: VFS 文件池只有 1 个槽位 */
    return true;
}

/**
 * @brief 扫描录像目录，统计 AVI/JPG 的最大编号
 *
 * 遍历方式用 fs->file_number + FSEL_BY_NUMBER(SDK 通用写法, 见 record.c),
 * 不用 FSEL_NEXT_FILE 起步 —— 该模式下首次调用拿不到文件时整个循环一次都不进,
 * 会一个文件都扫不到, 索引直接归零并从 VID_0000 覆盖旧录像。
 *
 * @param max_video  输出: AVI 最大编号, 无文件为 -1
 * @param max_photo  输出: JPG 最大编号, 无文件为 -1
 * @param video_cnt  输出: AVI 文件数
 * @param photo_cnt  输出: JPG 文件数
 * @return 0=成功, -1=扫描失败(fscan 返回 NULL)
 */
static int video_api_scan_index(int *max_video, int *max_photo,
                                int *video_cnt, int *photo_cnt)
{
    /* -tAVIJPG: 只扫 AVI 和 JPG(驱动层过滤, 避免遍历无关文件); 不排序, 只需要最大编号 */
    struct vfscan *fs = fscan(REC_DIR, "-tAVIJPG -sn", 1);

    *max_video = -1;
    *max_photo = -1;
    *video_cnt = 0;
    *photo_cnt = 0;

    if (fs == NULL) {
        return -1;
    }

    /* fscan 失败和目录为空都会走"没有文件"分支, 这里把 file_number 打出来区分 */
    printf("[VIDEO_API] scan %s: file_number=%u\n", REC_DIR, fs->file_number);

    for (u16 i = 1; i <= fs->file_number; i++) {
        FILE *fp = fselect(fs, FSEL_BY_NUMBER, (int)i);
        char name[64] = {0};
        int num;

        if (fp == NULL) {
            break;
        }

        fget_name(fp, (u8 *)name, sizeof(name));

        num = video_api_parse_index_with_prefix(name, "VID_");
        if (num >= 0) {
            if (num > *max_video) {
                *max_video = num;
            }
            (*video_cnt)++;
            fclose(fp);
            continue;
        }

        num = video_api_parse_index_with_prefix(name, "IMG_");
        if (num >= 0) {
            if (num > *max_photo) {
                *max_photo = num;
            }
            (*photo_cnt)++;
        }
        fclose(fp);
    }

    fscan_release(fs);
    return 0;
}

/**
 * @brief 初始化文件索引
 *
 * 扫描目录查找 .AVI 和 .JPG 文件的最大编号，如果目录不存在则创建。
 * 扫描失败时重试若干次(设备刚上电/SD 刚退出低功耗时文件系统可能未就绪),
 * 仍失败则返回 -1 由调用方放弃本次录像/拍照, 而不是静默从 0 开始覆盖旧文件。
 *
 * @return 0=成功, -1=失败
 */
static int video_api_file_init(void)
{
    int ret;

    if (file_index_inited) {
        return 0;
    }

    /* 1. 检查并创建目录 */
    if (fdir_exist(REC_DIR) != 0) {
        printf("[VIDEO_API] directory not exist, creating: %s\n", REC_DIR);
        ret = fmk_dir(REC_BASE_PATH, REC_FOLDER, 0);
        if (ret != 0) {
            printf("[VIDEO_API] failed to create directory: %s, ret=%d\n", REC_DIR, ret);
            return -1;
        }
        /* 新目录，索引从 0 开始 */
        rec_path_index = 0;
        photo_path_index = 0;
        file_index_inited = true;
        printf("[VIDEO_API] new directory created, indices start from 0\n");
        return 0;
    }

    /* 2. 扫描目录，统计 AVI/JPG 的最大编号 */
    {
        int max_video = -1;
        int max_photo = -1;
        int video_cnt = 0;
        int photo_cnt = 0;
        int scan_ok = 0;

        for (int i = 0; i < FILE_SCAN_RETRY_TIMES; i++) {
            if (video_api_scan_index(&max_video, &max_photo,
                                     &video_cnt, &photo_cnt) == 0) {
                scan_ok = 1;
                break;
            }
            printf("[VIDEO_API] fscan %s failed, retry %d/%d\n",
                   REC_DIR, i + 1, FILE_SCAN_RETRY_TIMES);
            os_time_dly(FILE_SCAN_RETRY_DELAY);
        }

        if (!scan_ok) {
            printf("[VIDEO_API] cannot scan %s, refuse to record/photo\n", REC_DIR);
            return -1;
        }

        /* 设置视频文件索引 */
        if (max_video >= 0) {
            rec_path_index = max_video + 1;
            if (rec_path_index > MAX_FILE_INDEX) {
                rec_path_index = 0;  /* 索引溢出，从头循环 */
            }
            printf("[VIDEO_API] found %d existing video files, last=%d, next_index=%d\n",
                   video_cnt, max_video, rec_path_index);
        } else {
            rec_path_index = 0;
            printf("[VIDEO_API] no existing video files, video index start from 0\n");
        }

        /* 设置照片文件索引 */
        if (max_photo >= 0) {
            photo_path_index = max_photo + 1;
            if (photo_path_index > MAX_FILE_INDEX) {
                photo_path_index = 0;
            }
            printf("[VIDEO_API] found %d existing photo files, last=%d, next_index=%d\n",
                   photo_cnt, max_photo, photo_path_index);
        } else {
            photo_path_index = 0;
            printf("[VIDEO_API] no existing photo files, photo index start from 0\n");
        }
    }

    file_index_inited = true;
    return 0;
}

static int video_api_format_media_path(char *path,
                                       size_t path_size,
                                       const char *fmt,
                                       int index,
                                       const char *tag)
{
    int written = snprintf(path, path_size, fmt, index);

    if (written < 0 || (size_t)written >= path_size) {
        printf("[VIDEO_API] %s path truncated! need=%d, buf=%zu\n",
               tag, written, path_size);
        return -1;
    }
    return 0;
}

/**
 * @brief 生成下一个视频文件的完整路径
 * @param path      输出缓冲区
 * @param path_size 缓冲区大小
 * @return 0=成功, -1=找不到空闲文件名, -2=路径过长
 */
static int video_api_get_video_path(char *path, size_t path_size)
{
    /* 索引可能被其它流程占用(或扫描漏了文件), 逐个往后找第一个不存在的名字,
     * 保证绝不截断已有录像; 全满时绕回 0 再找一圈 */
    for (int tries = 0; tries <= MAX_FILE_INDEX; tries++) {
        if (rec_path_index > MAX_FILE_INDEX) {
            rec_path_index = 0;
        }

        if (video_api_format_media_path(path, path_size,
                                        REC_DIR VIDEO_FILE_FMT,
                                        rec_path_index, "video") != 0) {
            return -2;
        }

        rec_path_index++;

        if (!video_api_file_exist(path)) {
            return 0;
        }
        printf("[VIDEO_API] %s already exists, skip\n", path);
    }

    printf("[VIDEO_API] no free video file name in %s\n", REC_DIR);
    return -1;
}

/**
 * @brief 生成下一个照片文件的完整路径
 * @param path      输出缓冲区
 * @param path_size 缓冲区大小
 * @return 0=成功, -1=找不到空闲文件名, -2=路径过长
 */
static int video_api_get_photo_path(char *path, size_t path_size)
{
    for (int tries = 0; tries <= MAX_FILE_INDEX; tries++) {
        if (photo_path_index > MAX_FILE_INDEX) {
            photo_path_index = 0;
        }

        if (video_api_format_media_path(path, path_size,
                                        REC_DIR PHOTO_FILE_FMT,
                                        photo_path_index, "photo") != 0) {
            return -2;
        }

        photo_path_index++;

        if (!video_api_file_exist(path)) {
            return 0;
        }
        printf("[VIDEO_API] %s already exists, skip\n", path);
    }

    printf("[VIDEO_API] no free photo file name in %s\n", REC_DIR);
    return -1;
}

/**
 * @brief 获取当前录像文件的序号(用于循环录像回调中的路径设置)
 *
 * 注意：video_api_get_video_path() 内部会自增 rec_path_index，
 * 而事件回调 APP_CAMERA_EVENT_REC_CYC_FILE 中需要同步设置新路径。
 * 这里直接使用 rec_path_index 当前值生成路径。
 *
 * @param path      输出缓冲区
 * @param path_size 缓冲区大小
 * @return 0=成功, 非0=失败
 */
static int video_api_get_cycle_video_path(char *path, size_t path_size)
{
    /* 同 video_api_get_video_path(): 循环录像换段同样不能撞已有文件 */
    for (int tries = 0; tries <= MAX_FILE_INDEX; tries++) {
        if (rec_path_index > MAX_FILE_INDEX) {
            rec_path_index = 0;
        }

        if (video_api_format_media_path(path, path_size,
                                        REC_DIR VIDEO_FILE_FMT,
                                        rec_path_index, "cycle") != 0) {
            return -1;
        }

        rec_path_index++;

        if (!video_api_file_exist(path)) {
            return 0;
        }
        printf("[VIDEO_API] %s already exists, skip\n", path);
    }

    printf("[VIDEO_API] no free video file name for cycle in %s\n", REC_DIR);
    return -1;
}

/*============================================================================*/
/*  内存估算 & 分辨率选择                                                     */
/*============================================================================*/

/**
 * @brief 估算录像所需内存
 * @param res 分辨率+编码参数
 * @param audio_buf_size 音频缓冲区大小
 * @return 估算的内存需求（字节）
 *
 * 视频管线内部分配: YUV 帧缓冲 + 音频缓冲 + 1k安全边界
 */
static u32 video_api_estimate_video_mem(const video_res_entry_t *res, u32 audio_buf_size)
{
    return (u32)res->width * res->height * 3 / 2 + audio_buf_size + VIDEO_MEM_ESTIMATE_MARGIN;
}

/**
 * @brief 估算拍照所需内存
 * @param res 分辨率
 * @return 估算的内存需求（字节）
 *
 * 拍照管线: 抓拍帧
 * */
static u32 video_api_estimate_photo_mem(const video_resolution_t *res)
{
    return (u32)res->width * res->height * 3;
}

/**
 * @brief 探测 PSRAM 是否有足够剩余空间
 * @param size 需要的字节数
 * @return true=足够, false=不足或 PSRAM 未启用
 *
 * SDK 没有直接查询 PSRAM 剩余量的 API，通过分配+释放来探测
 */
static bool video_api_psram_has_enough(u32 size)
{
#if TCFG_PSRAM_DEV_ENABLE
    void *p = malloc_psram(size);
    if (p) {
        free_psram(p);
        return true;
    }
    return false;
#else
    return false;
#endif
}

/**
 * @brief 检查 RAM 是否有足够剩余空间
 * @param size 需要的字节数
 * @return true=足够, false=不足
 */
static bool video_api_ram_has_enough(u32 size)
{
    return memory_get_size(P_MEMORY_UNUSED) >= size;
}

/**
 * @brief 选择最佳录像分辨率
 *
 * 从高到低遍历录像分辨率表:
 * - PSRAM 启用: 探测 PSRAM，选第一个满足条件的
 * - PSRAM 未启用: 检查 RAM，选第一个满足条件的
 * - 都不满足则降级到最低分辨率
 *
 * @return 选中的分辨率参数（永不为 NULL）
 */
static const video_res_entry_t *video_api_select_video_res(void)
{
    const u32 audio_buf_size = VIDEO_AUDIO_BUF_SIZE;
    const int count = ARRAY_SIZE(video_res_table);

    for (int i = 0; i < count; i++) {
        u32 need = video_api_estimate_video_mem(&video_res_table[i], audio_buf_size);

#if TCFG_PSRAM_DEV_ENABLE
        if (video_api_psram_has_enough(need)) {
            printf("[VIDEO_API] vid: PSRAM ok  %dx%d  need=%u\n",
                   video_res_table[i].width, video_res_table[i].height, need);
            return &video_res_table[i];
        }
        printf("[VIDEO_API] vid: PSRAM fail %dx%d  need=%u\n",
               video_res_table[i].width, video_res_table[i].height, need);
#else
        if (video_api_ram_has_enough(need)) {
            printf("[VIDEO_API] vid: RAM ok   %dx%d  need=%u free=%d\n",
                   video_res_table[i].width, video_res_table[i].height,
                   need, (int)memory_get_size(P_MEMORY_UNUSED));
            return &video_res_table[i];
        }
        printf("[VIDEO_API] vid: RAM fail %dx%d  need=%u free=%d\n",
               video_res_table[i].width, video_res_table[i].height,
               need, (int)memory_get_size(P_MEMORY_UNUSED));
#endif
    }

    /* 所有分辨率都不满足，降级到最低 */
    printf("[VIDEO_API] vid: fallback to %dx%d\n",
           video_res_table[count - 1].width, video_res_table[count - 1].height);
    return &video_res_table[count - 1];
}

/**
 * @brief 选择最佳拍照分辨率
 *
 * 逻辑同 video_api_select_video_res()，遍历 photo_res_table
 *
 * @return 选中的分辨率（永不为 NULL）
 */
static const video_resolution_t *video_api_select_photo_res(void)
{
    const int count = ARRAY_SIZE(photo_res_table);

    for (int i = 0; i < count; i++) {
        u32 need = video_api_estimate_photo_mem(&photo_res_table[i]);

#if TCFG_PSRAM_DEV_ENABLE
        if (video_api_psram_has_enough(need)) {
            printf("[VIDEO_API] photo: PSRAM ok  %dx%d  need=%u\n",
                   photo_res_table[i].width, photo_res_table[i].height, need);
            return &photo_res_table[i];
        }
        printf("[VIDEO_API] photo: PSRAM fail %dx%d  need=%u\n",
               photo_res_table[i].width, photo_res_table[i].height, need);
#else
        if (video_api_ram_has_enough(need)) {
            printf("[VIDEO_API] photo: RAM ok   %dx%d  need=%u  free=%d\n",
                   photo_res_table[i].width, photo_res_table[i].height,
                   need, (int)memory_get_size(P_MEMORY_UNUSED));
            return &photo_res_table[i];
        }
        printf("[VIDEO_API] photo: RAM fail %dx%d  need=%u  free=%d\n",
               photo_res_table[i].width, photo_res_table[i].height,
               need, (int)memory_get_size(P_MEMORY_UNUSED));
#endif
    }

    printf("[VIDEO_API] photo: fallback to %dx%d\n",
           photo_res_table[count - 1].width, photo_res_table[count - 1].height);
    return &photo_res_table[count - 1];
}

static void camera_stop_record_video(void);

/*
 * 内部拍照实现。done_cb 为 NULL 表示普通拍照（调用方不关心结果）；
 * 非 NULL 时在拍照结束（成功或失败）后回调，供 AI 识图流程决定后续回报。
 * result: 0 成功，-1 失败；成功时 path 为落盘路径，失败时为 NULL。
 */
static void camera_take_photo_internal(void (*done_cb)(int result, const char *path))
{
    const u8 camera_id = VIDEO_CAMERA_ID;
    const char *sensor_name = VIDEO_SENSOR_NAME;
    app_camera_ret_t ret;

    camera_status = CAMERA_STATE_PHOTO;

    /* 先检查 SD 卡状态，避免离线时触发目录扫描/创建 */
    if (!video_api_sd_is_online()) {
        printf("[VIDEO_API] SD card not ready, cannot take photo\n");
        reset_camera_status();
        if (done_cb) {
            done_cb(-1, NULL);
        }
        return;
    }

    /* 确保文件索引已初始化 */
    if (video_api_file_init() != 0) {
        printf("[VIDEO_API] file init failed, cannot take photo\n");
        reset_camera_status();
        if (done_cb) {
            done_cb(-1, NULL);
        }
        return;
    }

    ret = app_camera_open(camera_id, sensor_name, &photo_handle);
    if (ret != APP_CAMERA_OK) {
        printf("[VIDEO_API] failed to open camera for photo, ret=%d\n", ret);
        reset_camera_status();
        if (done_cb) {
            done_cb(-1, NULL);
        }
        return;
    }

    const video_resolution_t *res = video_api_select_photo_res();
    u16 width = res->width, height = res->height;
    const u8 quality = VIDEO_PHOTO_QUALITY;

    app_camera_photo_attr_t photo_attr = {
        .width = width,
        .height = height,
        .quality = quality,
        .stream_buf_size = (uint32_t)width * height * quality / 100 + VIDEO_PHOTO_BUF_EXTRA,		//加1k的安全边界
        .thumbnail_enable = 1,
    };

    /* 生成照片文件路径 */
    if (video_api_get_photo_path(photo_attr.path, sizeof(photo_attr.path)) != 0) {
        printf("[VIDEO_API] failed to generate photo path\n");
        video_api_close_and_reset(&photo_handle);
        if (done_cb) {
            done_cb(-1, NULL);
        }
        return;
    }

    printf("[VIDEO_API] taking photo: %s\n", photo_attr.path);
    ret = app_camera_take_photo(photo_handle, &photo_attr);
    if (ret != APP_CAMERA_OK) {
        printf("[VIDEO_API] take photo failed, ret=%d\n", ret);
    }
    video_api_close_and_reset(&photo_handle);

    if (done_cb) {
        /* 拍照文件已完整落盘并关闭，此时才通知调用者 */
        done_cb(ret == APP_CAMERA_OK ? 0 : -1,
                ret == APP_CAMERA_OK ? photo_attr.path : NULL);
    }
}

static void camera_take_photo(void)
{
    camera_take_photo_internal(NULL);
}

/* AI 识图专用入口：只负责拍照落盘，完成后回调通知 AI 对话流程 */
void camera_api_take_photo_ai(void (*on_done)(int result, const char *path))
{
    if (get_camera_status() != CAMERA_STATE_IDLE) {
        printf("[VIDEO_API] camera busy, cannot take AI photo\n");
        if (on_done) {
            on_done(-1, NULL);
        }
        return;
    }
    camera_take_photo_internal(on_done);
}

static void app_camera_test_event_handler(void *priv, int argc, int *argv)
{
    app_camera_handle_t handle = (app_camera_handle_t)priv;

    if (argv == NULL || argc <= 0) {
        return;
    }

    switch (argv[0]) {
    case APP_CAMERA_EVENT_REC_STARTED:
        printf("[VIDEO_API] camera record started\n");
        break;

    case APP_CAMERA_EVENT_REC_TIME:
        if (argc > 1) {
            printf("[VIDEO_API] camera record time: %d ms\n", argv[1]);
        }

        /* 检测到来电/去电/通话中，立即停止录像，避免音频通路切换(SCO)导致 CVP 帧大小不匹配死机 */
        {
            int call_status = bt_get_call_status();
            if (video_api_is_call_active(call_status)) {
                printf("[VIDEO_API] call detected (status=%d), stopping record\n", call_status);
                camera_stop_record_video();
                break;
            }
        }

        /* 每5秒检查一次剩余空间，低于临界值立即停止录像 */
        {
            u32 current_sec = (argc > 1) ? (u32)argv[1] / 1000 : 0;

            if (current_sec - rec_last_space_check_sec >= VIDEO_SPACE_CHECK_INTERVAL_SEC) {
                rec_last_space_check_sec = current_sec;
                if (!video_api_check_free_space(MIN_FREE_SPACE_KB_CRITICAL)) {
                    printf("[VIDEO_API] critically low storage, stopping record\n");
                    camera_stop_record_video();
                }
            }
        }
        break;

    case APP_CAMERA_EVENT_REC_CYC_FILE:
        if (argc > 1) {
            /* 切换循环文件前检查剩余空间，不足则停止录像（当前文件已正常关闭不会损坏） */
            if (!video_api_check_free_space(MIN_FREE_SPACE_KB_CRITICAL)) {
                printf("[VIDEO_API] critically low storage at cycle, stopping record\n");
                camera_stop_record_video();
                break;
            }
            {
                char path[APP_CAMERA_REC_PATH_MAX];
                if (video_api_get_cycle_video_path(path, sizeof(path)) == 0) {
                    printf("[VIDEO_API] record cycle file, cyc_count=%d, next_path=%s\n",
                           argv[1], path);
                    app_camera_rec_set_path(handle, path);
                } else {
                    printf("[VIDEO_API] failed to generate cycle file path\n");
                }
            }
        }
        break;

    case APP_CAMERA_EVENT_REC_STOPPED:
        printf("[VIDEO_API] camera record stopped\n");
        break;

    case APP_CAMERA_EVENT_REC_ERROR:
        /* SD卡写满或I/O错误时，立即停止录像清理管线，避免死机和文件损坏 */
        printf("[VIDEO_API] camera record error, stopping\n");
        camera_stop_record_video();
#if (defined(TCFG_SD0_ENABLE) && (TCFG_SD0_ENABLE))
        sdmmc_dev_enter_lowpower("sd0");
#endif
#if (defined(TCFG_SD1_ENABLE) && (TCFG_SD1_ENABLE))
        sdmmc_dev_enter_lowpower("sd1");
#endif
        break;

    default:
        break;
    }
}

/**
 * @brief 启动录像前，先停止并保存正在进行的录音
 *
 * 录像本身带音频（.audio_enable = 1），音频源同样是 ADC/mic，
 * 与录音模式、混合录音、RCSP 混合录音会抢占同一条 mic 通路。
 * 若录音还在进行就直接开录像，正在录的音频文件不会被正常封头保存，
 * 可能产生损坏的录音文件甚至管线冲突，所以这里先停止保存。
 */
static void video_api_stop_active_record(void)
{
#if TCFG_APP_RECORD_EN
    if (is_recorder_runing()) {
        printf("[VIDEO_API] recorder active, stopping\n");
        app_recorder_stop_public();
    }
#endif
#if TCFG_MIX_RECORD_ENABLE
    if (get_mix_recorder_status()) {
        printf("[VIDEO_API] mix recorder active, stopping\n");
        mix_recorder_stop();
    }
    if (rcsp_mix_recorder_get_status()) {
        printf("[VIDEO_API] rcsp mix recorder active, stopping\n");
        rcsp_mix_recorder_stop();
    }
#endif
}

void camera_api_record_video()
{
    const u8 camera_id = VIDEO_CAMERA_ID;
    const char *sensor_name = VIDEO_SENSOR_NAME;
    app_camera_ret_t ret;

    /* 状态检查 */
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        printf("[VIDEO_API] already recording, skip\n");
        return;
    }
    if (BT_STATUS_TAKEING_PHONE == bt_get_connect_status()) {
        printf("[VIDEO_API] phone calling, cannot record\n");
        return;
    }
    {
        int call_status = bt_get_call_status();
        if (video_api_is_call_active(call_status)) {
            printf("[VIDEO_API] call active, cannot record\n");
            return;
        }
    }

#if TCFG_BT_SUPPORT_MAP
    bt_cmd_prepare(USER_CTRL_MAP_READ_TIME, 0, NULL);
#endif

#if TCFG_APP_RTC_EN
    {
        struct sys_time time;
        rtc_read_time(&time);
        /* printf("%s year %d month %d day %d hour %d min %d sec %d \n",
                  __FUNCTION__, time.year, time.month, time.day,
                  time.hour, time.min, time.sec); */
    }
#endif

    /* 先停止正在进行的录音，避免与录像抢占 mic 通路 */
    video_api_stop_active_record();

    /* 先检查 SD 卡状态，避免离线时触发目录扫描/创建 */
    if (!video_api_sd_is_online()) {
        printf("[VIDEO_API] SD card not ready, cannot record\n");
        return;
    }

    /* 确保文件索引已初始化 */
    if (video_api_file_init() != 0) {
        printf("[VIDEO_API] file init failed, cannot record\n");
        return;
    }

    /* 检查剩余空间 */
    if (!video_api_check_free_space(MIN_FREE_SPACE_KB_FOR_REC)) {
        printf("[VIDEO_API] insufficient storage, cannot record\n");
        return;
    }

    camera_status = CAMERA_STATE_VIDEO;

    ret = app_camera_open(camera_id, sensor_name, &rec_handle);
    if (ret != APP_CAMERA_OK) {
        printf("[VIDEO_API] failed to open camera for record, ret=%d\n", ret);
        reset_camera_status();
        return;
    }

    ret = app_camera_register_event_handler(rec_handle,
                                            rec_handle,
                                            app_camera_test_event_handler);
    if (ret != APP_CAMERA_OK) {
        printf("[VIDEO_API] failed to register event handler, ret=%d\n", ret);
        video_api_close_and_reset(&rec_handle);
        return;
    }

    const video_res_entry_t *res = video_api_select_video_res();

    app_camera_rec_attr_t rec_attr = {
        .width = res->width,
        .height = res->height,
        .fps = res->fps,
        .abr_kbps = res->abr_kbps,
        .cyc_sec = VIDEO_REC_CYC_SEC,
        .stream_buf_size = res->stream_buf_size,
        .audio_enable = 1,
        .audio_buf_size = VIDEO_AUDIO_BUF_SIZE,
        .audio_frame_interval_ms = VIDEO_AUDIO_FRAME_INTERVAL_MS,
        .audio_codec_type = APP_CAMERA_AUDIO_CODEC_MP3,
        .audio_sample_rate = VIDEO_AUDIO_SAMPLE_RATE,
        .audio_bit_rate = VIDEO_AUDIO_BIT_RATE,
    };

    /* 生成录像文件路径 */
    if (video_api_get_video_path(rec_attr.path, sizeof(rec_attr.path)) != 0) {
        printf("[VIDEO_API] failed to generate record path\n");
        video_api_close_and_reset(&rec_handle);
        return;
    }

    printf("[VIDEO_API] starting record: %s\n", rec_attr.path);

#if TCFG_APP_MUSIC_EN
    /* 正在播放本地音乐时先暂停，避免录像抢占音频通路后音乐无法暂停 */
    {
        struct file_player *file_player = get_music_file_player();
        if (file_player && music_file_get_player_status(file_player) == FILE_PLAYER_START) {
            printf("[VIDEO_API] local music playing, pause before record\n");
            music_file_player_pp(file_player);
        }
    }
#endif

    ret = app_camera_rec_start(rec_handle, &rec_attr);
    if (ret != APP_CAMERA_OK) {
        printf("[VIDEO_API] rec_start failed, ret=%d\n", ret);
        video_api_close_and_reset(&rec_handle);
    }
}

static void camera_stop_record_video()
{
    printf("[VIDEO_API] stopping record\n");
    video_api_close_and_reset(&rec_handle);
}

void camera_api_take_photo_or_stop_rec()
{
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        camera_stop_record_video();
    } else {
        camera_take_photo();
    }
}

void camera_api_stop_rec_photo()
{
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        video_api_close_and_reset(&rec_handle);
    } else if (get_camera_status() == CAMERA_STATE_PHOTO) {
        video_api_close_and_reset(&photo_handle);
    } else {
        reset_camera_status();
    }
}

#endif
