/**
 * @file video_rec_worker.c
 * @brief 录像后台线程 + 存储管理 + 文件操作
 *
 * 包含:
 *   - 存储管理 (video_rec_get_storage_info, video_rec_check_storage)
 *   - 文件数量限制 (video_rec_count_files, video_rec_check_file_count)  // Phase 1
 *   - 文件加锁 (委托 file_lock.h)
 *   - 循环录像配置 (video_rec_set_loop_time, video_rec_get_current_file)
 *   - AVI 修复 (video_rec_repair_avi)
 *
 * UI 回调统一在 video_rec_ui.h → Coder2 实现
 * 文件加锁统一在 file_lock.h
 *
 * ## 单位说明 (fget_free_space)
 *   SDK 的 fget_free_space() 返回值已经是 KB (FAT 源码:
 *     *space = fat->space.free << (fat->clustSize - 10);
 *   即 clusters × (1 << (clustSize - 10)) KB/cluster).
 *   本模块直接以 KB 解读，禁止再 /1024，避免和 photo.c 的错误做法混淆。
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video.data.bss")
#pragma data_seg(".video.data")
#pragma const_seg(".video.text.const")
#pragma code_seg(".video.text")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "video_rec.h"
#include "video_rec_internal.h"
#include "file_lock.h"
#include "app_camera_mpi.h"
#include "os/os_api.h"
#include "system/timer.h"
#include "stdlib.h"
#include "string.h"
#include "avilib.h"
#include "fs/fs.h"
#include "media_file_list.h"

#if TCFG_APP_VIDEO_REC_EN


#define LOG_TAG             "[VREC_WORKER]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

#if (defined TCFG_PSRAM_SIZE && TCFG_PSRAM_SIZE > 0)
#define kc_malloc(sz)       malloc_psram(sz)
#define kc_free(p)          free_psram(p)
#else
#define kc_malloc(sz)       malloc(sz)
#define kc_free(p)          free(p)
#endif


/*==============================================================================
 * 存储管理
 *
 * fget_free_space() 返回值单位: KB (见文件头注释)
 *==============================================================================
 */
int video_rec_get_storage_info(u32 *total_kb, u32 *free_kb)
{
    if (!total_kb || !free_kb) {
        return -1;
    }
    if (flm_sd_online() != 0) {
        return -2;
    }
    u32 free_space_kb = 0;
    struct vfs_partition *part = fget_partition(CONFIG_ROOT_PATH);
    if (!part) {
        part = fget_partition("mnt/"SDX_DEV);
        if (!part) {
            return -2;
        }
    }
    /* part->total_size 由 FAT 层填入, 单位 KB (见 fat_compact.c:289) */
    *total_kb = part->total_size;
    if (fget_free_space(CONFIG_ROOT_PATH, &free_space_kb) != 0) {
        fget_free_space("mnt/"SDX_DEV, &free_space_kb);
    }
    *free_kb = free_space_kb;
    return 0;
}
/*==============================================================================
 * 异步容量计算
 *
 * fget_free_space 遍历 FAT 首次耗时数秒, 不能跑在 LVGL ui 任务(否则卡住画面
 * 渲染, 录像屏长时间背景/白屏)。计算放 app_core 线程(APP_MSG 消息),
 * 结果经共享变量回读。
 *==============================================================================
 */
static volatile u32 g_rec_free_sec_async = 0;  /* 剩余可录秒(后台算好, UI 只读) */
static volatile u8  g_rec_has_card_async = 1;  /* SD 在位(后台探测) */
static volatile u8  g_rec_calc_done = 0;       /* 后台算完标志(UI 取后清) */

/* 剩余秒 = free_kb * 8(kbit/KB) / 码率kbps */
static u32 video_rec_free_kb_to_sec(u32 free_kb)
{
    return (u32)(((u64)free_kb * 8u) / VIDEO_REC_VENC_ABR_KBPS);
}

/* app_core 线程执行: 慢遍历 FAT, 不阻塞 LVGL ui 任务 */
void video_rec_storage_recalc(void)
{
    u32 total_kb = 0, free_kb = 0;
    /* 录像中跳过 FAT 遍历: 抢 SD 总线且占 app_core;
     * 录像中的剩余容量由 worker 的 video_rec_check_storage() 更新共享缓存。
     * 这里不能清 g_rec_calc_done, 否则会盖掉 worker 刚发布的结果。 */
    if (video_rec_is_recording()) {
        return;
    }
    if (flm_sd_online() != 0) {
        g_rec_has_card_async = 0;
        g_rec_free_sec_async = 0;
    } else {
        g_rec_has_card_async = 1;
        if (video_rec_get_storage_info(&total_kb, &free_kb) == 0) {
            g_rec_free_sec_async = video_rec_free_kb_to_sec(free_kb);
        }
    }
    g_rec_calc_done = 1;
}

u32 video_rec_storage_get_free_sec(void)
{
    return g_rec_free_sec_async;
}
u8 video_rec_storage_get_has_card(void)
{
    return g_rec_has_card_async;
}

/* 后台是否已算完 (UI 取后清标志, 避免反复用旧值) */
u8 video_rec_storage_take_done(void)
{
    u8 done = g_rec_calc_done;
    g_rec_calc_done = 0;
    return done;
}

/*==============================================================================
 * 文件数量计数 (Phase 1)
 *
 * 扫描 VIDEO_REC_FILE_PATH 下所有 .avi/.AVI 文件, 返回数量.
 * 跳过当前正在录像的 g_rec_ctx.current_file_path (避免 worker 写入时计数波动).
 *==============================================================================
 */
int video_rec_count_files(void)
{
    int count = 0;
    struct vfscan *fs = fscan(VIDEO_REC_FILE_PATH, "-tAVI -sn", 1);
    if (!fs) {
        log_info("[VREC] no VIDEO directory, count=0");
        return 0;
    }

    FILE *fp;
    /* fselect 返回的 FILE* 循环尾必须 fclose, 否则 VFS_FILE_POOL_NUM_CONFIG=1 时 P_MEM 耗尽 */

    for (fp = fselect(fs, FSEL_FIRST_FILE, 0); fp != NULL;
         fp = fselect(fs, FSEL_NEXT_FILE, 0)) {
        char name[64] = {0};
        fget_name(fp, (u8 *)name, sizeof(name));
        /* 过滤大小写 .avi 扩展名 */
        if (strstr(name, ".avi") == NULL && strstr(name, ".AVI") == NULL) {
            fclose(fp);
            continue;
        }
        /* 跳过当前正在录像的文件 (避免被 worker 写入时影响计数) */
        if (g_rec_ctx.current_file_path[0] != '\0' &&
            strstr(g_rec_ctx.current_file_path, name) != NULL) {
            fclose(fp);
            continue;
        }
        count++;
        fclose(fp);
    }
    fscan_release(fs);
    return count;
}

/*==============================================================================
 * 删除最旧未加锁文件
 *
 * 策略:
 *   1. 扫描 VIDEO_REC_FILE_PATH 下的 AVI 文件
 *   2. 按文件名字典序排序, 找出最小的（JLFS 按 VID_**** 顺序生成，
 *      文件名顺序即创建顺序；不依赖设备 RTC，避免未联网时间不准）
 *   3. 跳过已加锁 (F_ATTR_RO) 的文件
 *   4. 跳过当前正在录像的 g_rec_ctx.current_file_path
 *   5. flm_delete_file()（删除文件并同步更新媒体数据库）
 *
 * 返回 0=成功删除, -1=无可删文件 (目录为空或全部加锁)
 *==============================================================================
 */
static int video_rec_delete_oldest_file(void)
{
    struct vfscan *fs = fscan(VIDEO_REC_FILE_PATH, "-tAVI -sn", 1);
    if (!fs) {
        fs = fscan("mnt/sd1/DCIM/VIDEO", "-tAVI -sn", 1);
    }
    if (!fs) {
        log_info("[VREC] cannot scan VIDEO directory\n");
        return 0;
    }

    struct vfs_attr attr = {0};
    char oldest_name[64] = {0};

    FILE *fp;
    for (fp = fselect(fs, FSEL_FIRST_FILE, 0); fp != NULL;
         fp = fselect(fs, FSEL_NEXT_FILE, 0)) {
        char name[64] = {0};
        fget_name(fp, (u8 *)name, sizeof(name));
        if (strstr(name, ".avi") == NULL && strstr(name, ".AVI") == NULL) {
            fclose(fp);
            continue;
        }

        if (fget_attrs(fp, &attr) != 0) {
            fclose(fp);
            continue;
        }
        /* 加锁保护: 跳过只读属性的文件 */
        if (attr.attr & F_ATTR_RO) {
            fclose(fp);
            continue;
        }
        /* 跳过当前正在录像的文件 */
        if (g_rec_ctx.current_file_path[0] != '\0' &&
            strstr(g_rec_ctx.current_file_path, name) != NULL) {
            fclose(fp);
            continue;
        }

        /* 设备时间可能未联网同步，创建时间不可靠；
         * 文件名由 JLFS 按 VID_**** 顺序自动生成，
         * 因此按文件名字典序选最旧（最小）的一个。 */
        if (oldest_name[0] == '\0' || strcmp(name, oldest_name) < 0) {
            strncpy(oldest_name, name, sizeof(oldest_name) - 1);
        }
        fclose(fp);
    }
    fscan_release(fs);

    if (oldest_name[0] == '\0') {
        log_info("[VREC] no unlocked file to delete\n");
        return -1;
    }

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", VIDEO_REC_FILE_PATH, oldest_name);
    log_info("[VREC] deleting oldest file: %s\n", full_path);

    if (flm_delete_file(full_path) != 0) {
        log_error("[VREC] delete file failed: %s\n", full_path);
        return -1;
    }
    log_info("[VREC] deleted: %s\n", full_path);
    return 0;
}

/*==============================================================================
 * 存储空间检查 (容量维度)
 *
 * 当 free < MIN_FREE_SPACE_MB*1024 KB 时, 循环删除最旧未加锁文件直到满足.
 *==============================================================================
 */
int video_rec_check_storage(void)
{
    u32 total_kb = 0, free_kb = 0;
    int ret = video_rec_get_storage_info(&total_kb, &free_kb);
    if (ret == -2) {
        log_error("[VREC] SD card not online\n");
        g_rec_has_card_async = 0;
        g_rec_free_sec_async = 0;
        g_rec_calc_done = 1;
        return -2;
    }
    if (ret != 0) {
        return -1;
    }

    log_info("[VREC] storage: total=%uKB, free=%uKB\n", total_kb, free_kb);

    if (free_kb < (u32)MIN_FREE_SPACE_MB * 1024) {
        log_info("[VREC] free space < %dMB, deleting oldest unlocked file\n", MIN_FREE_SPACE_MB);
        int retries = VIDEO_DELETE_MAX_FILES;
        while (free_kb < (u32)MIN_FREE_SPACE_MB * 1024 && retries-- > 0) {
            if (video_rec_delete_oldest_file() != 0) {
                log_error("[VREC] no more files to delete\n");
                /* 即使无可删文件也把当前容量同步给 UI */
                g_rec_has_card_async = 1;
                g_rec_free_sec_async = video_rec_free_kb_to_sec(free_kb);
                g_rec_calc_done = 1;
                return -1;
            }
            video_rec_get_storage_info(&total_kb, &free_kb);
        }
    }

    /* worker 每 5s 已拿到最新 free_kb; 录像中 app_core 的 recalc 会跳过,
     * 这里直接把结果发布给 UI, 否则删旧文件后剩余时间不刷新 */
    g_rec_has_card_async = 1;
    g_rec_free_sec_async = video_rec_free_kb_to_sec(free_kb);
    g_rec_calc_done = 1;
    return 0;
}

/*==============================================================================
 * 文件数量检查 (数量维度, Phase 1)
 *
 * 当 count >= VIDEO_REC_MAX_FILES 时, 循环删除最旧未加锁文件直到 count < MAX.
 * 与 video_rec_check_storage() 独立, 两个都要满足才能开始录像.
 *==============================================================================
 */
int video_rec_check_file_count(void)
{
    int count = video_rec_count_files();
    if (count < 0) {
        return -1;
    }
    log_info("[VREC] current AVI count: %d (max=%d)\n", count, VIDEO_REC_MAX_FILES);

    if (count >= VIDEO_REC_MAX_FILES) {
        log_info("[VREC] file count >= %d, deleting oldest unlocked file\n", VIDEO_REC_MAX_FILES);
        int retries = VIDEO_DELETE_MAX_FILES;
        while (count >= VIDEO_REC_MAX_FILES && retries-- > 0) {
            if (video_rec_delete_oldest_file() != 0) {
                log_error("[VREC] no more files to delete (file count limit)\n");
                return -1;
            }
            count = video_rec_count_files();
            if (count < 0) {
                return -1;
            }
        }
    }
    return 0;
}

/*==============================================================================
 * 文件加锁 (委托 file_lock.h)
 *==============================================================================
 */
int video_rec_lock_file_by_path(const char *file_path)
{
    return br33_fs_lock_file(file_path);
}

int video_rec_unlock_file_by_path(const char *file_path)
{
    return br33_fs_unlock_file(file_path);
}

int video_rec_is_file_locked_by_path(const char *file_path)
{
    return br33_fs_is_locked(file_path);
}

int video_rec_lock_current_file(void)
{
    return video_rec_lock_file_by_path(g_rec_ctx.current_file_path);
}

/*==============================================================================
 * 循环录像配置
 *==============================================================================
 */
int video_rec_set_loop_time(u8 seconds)
{
    if (seconds != 30 && seconds != 60 && seconds != 90) {
        log_error("[VREC] invalid loop time: %u\n", seconds);
        return -1;
    }
    g_rec_ctx.loop_time_sec = seconds;
    log_info("[VREC] loop time set to %u seconds\n", seconds);
    return 0;
}

const char *video_rec_get_current_file(void)
{
    /* 返回空字符串而非 NULL, 避免调用方 fopen(NULL) 崩溃 */
    return g_rec_ctx.current_file_path[0] ? g_rec_ctx.current_file_path : "";
}

/*==============================================================================
 * AVI 修复 (P1)
 *
 * 当 worker 异常退出 (断电/复位) 时, AVI 头可能未刷新, 播放器无法识别.
 * 策略: 重写整个 AVI, 复用源文件的所有帧.
 *==============================================================================
 */
int video_rec_repair_avi(const char *path)
{
    if (!path || !*path) {
        return -1;
    }
    log_info("[VREC] repairing AVI: %s\n", path);

    void *src = AVI_open_input_file(path, 1);
    if (!src) {
        log_error("[VREC] repair: cannot open source\n");
        return -2;
    }

    char tmp_path[136];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    void *fixed = AVI_open_output_file(tmp_path);
    if (!fixed) {
        log_error("[VREC] repair: cannot create temp\n");
        AVI_close(src);
        return -5;
    }
    AVI_set_video(fixed, AVI_video_width(src), AVI_video_height(src),
                  AVI_frame_rate(src), AVI_video_compressor(src));
    if (AVI_can_read_audio(src) && AVI_audio_chunks(src) > 0) {
        AVI_set_audio(fixed, AVI_audio_channels(src), AVI_audio_rate(src),
                      AVI_audio_bits(src), AVI_audio_format(src), AVI_audio_mp3rate(src));
    }

    long max_frame = AVI_max_video_chunk(src);
    if (max_frame < 64 * 1024) {
        max_frame = 64 * 1024;
    }
    /* 视频与音频共用缓冲, 音频每段按 4KB 读; 任一帧超上限则中止, 保留源文件 */
    char *buf = kc_malloc(max_frame + 4 * 1024);
    if (!buf) {
        log_error("[VREC] repair: malloc(%ld) FAIL\n", max_frame + 4 * 1024);
        AVI_close(fixed);
        AVI_close(src);
        fdelete_by_name(tmp_path);
        return -3;
    }
    char *abuf = buf + max_frame;

    long written = 0;
    long audio_written = 0;
    int copy_failed = 0;
    for (;;) {
        int key = 0;
        long len = AVI_read_frame(src, buf, &key);
        if (len <= 0) {
            break;
        }
        if (len > max_frame) {
            log_error("[VREC] repair: frame too big %ld > %ld\n", len, max_frame);
            copy_failed = 1;
            break;
        }
        if (AVI_write_frame(fixed, buf, len, key) == 0) {
            written++;
        }
    }

    while (!copy_failed && AVI_can_read_audio(src)) {
        long alen = AVI_read_audio(src, abuf, 4 * 1024);
        if (alen <= 0) {
            break;
        }
        if (AVI_write_audio(fixed, abuf, alen) != 0) {
            log_error("[VREC] repair: write audio failed at %ld bytes\n", audio_written);
            copy_failed = 1;
            break;
        }
        audio_written += alen;
    }

    AVI_close(src);
    kc_free(buf);
    int cret = AVI_close(fixed);
    if (written <= 0 || copy_failed || cret != 0) {
        log_error("[VREC] repair: aborted written=%ld audio=%ld fail=%d close=%d\n",
                  written, audio_written, copy_failed, cret);
        fdelete_by_name(tmp_path);
        return -8;
    }

    /* 源先改名 .bak, 修复件再改回原名, 任一步失败都尝试恢复, 不直接删源 */
    char bak_path[140];
    snprintf(bak_path, sizeof(bak_path), "%s.bak", path);
    int bak_moved = 0;
    int rn_ret = -1;

    /* 旧 .bak 若存在会挡住 rename, 先清掉 */
    fdelete_by_name(bak_path);

    FILE *orig = fopen(path, "r");
    if (orig) {
        rn_ret = frename(orig, bak_path);
        if (rn_ret == 0) {
            bak_moved = 1;
        } else {
            /* 部分 JLFS 场景第一次 rename 失败, 关掉重开再试一次 */
            fclose(orig);
            orig = fopen(path, "r");
            if (orig) {
                rn_ret = frename(orig, bak_path);
                if (rn_ret == 0) {
                    bak_moved = 1;
                }
            }
        }
        if (orig) {
            fclose(orig);
        }
        if (!bak_moved) {
            /* rename 失败(目录枚举/文件属性等): 退化为原路径原地重写; 修复内容已在 tmp 完整落盘 */
            log_error("[VREC] repair: cannot move source aside (frename ret=%d), fallback in-place rewrite\n",
                      rn_ret);
            FILE *in = fopen(tmp_path, "r");
            FILE *out = fopen(path, "r+");
            if (!in || !out) {
                if (in) {
                    fclose(in);
                }
                if (out) {
                    fclose(out);
                }
                log_error("[VREC] repair: in-place open failed (path=%s), keep tmp\n", path);
                return -11;
            }
            char *cbuf = kc_malloc(4096);
            int copy_ok = 1;
            u32 total = 0;
            if (cbuf) {
                size_t n;
                while ((n = fread(cbuf, 1, 4096, in)) > 0) {
                    if (fwrite(cbuf, 1, n, out) != n) {
                        copy_ok = 0;
                        break;
                    }
                    total += (u32)n;
                }
                kc_free(cbuf);
            } else {
                copy_ok = 0;
            }
            fclose(in);
            if (copy_ok && ftruncate(out, total) != 0) {
                copy_ok = 0;
            }
            fclose(out);
            if (copy_ok) {
                fdelete_by_name(tmp_path);
                log_info("[VREC] repair done (in-place rewrite): %ld frames, %ld audio bytes\n",
                         written, audio_written);
                return 0;
            }
            log_error("[VREC] repair: in-place rewrite failed, keep tmp %s\n", tmp_path);
            return -12;
        }
    }

    FILE *tmp = fopen(tmp_path, "r");
    int rename_ok = 0;
    if (tmp) {
        rename_ok = (frename(tmp, path) == 0);
        fclose(tmp);
    }
    if (!rename_ok) {
        log_error("[VREC] repair: final rename failed, restoring source\n");
        if (bak_moved) {
            FILE *rb = fopen(bak_path, "r");
            if (rb) {
                frename(rb, path);
                fclose(rb);
            }
        }
        fdelete_by_name(tmp_path);
        return -10;
    }
    if (bak_moved) {
        FILE *bak = fopen(bak_path, "r");
        if (bak) {
            fdelete(bak);
        }
    }

    log_info("[VREC] repair done: %ld frames, %ld audio bytes\n", written, audio_written);
    return 0;
}
#endif /* #if TCFG_APP_VIDEO_REC_EN */
