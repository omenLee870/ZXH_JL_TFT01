#include "app_config.h"

#if TCFG_BT_NET_ENABLE || TCFG_EXT_WIFI_ENABLE || TCFG_UART_NET_ENABLE || TCFG_SPI_NET_ENABLE || TCFG_LTE_PHY_ENABLE || TCFG_RTL8733BS_ENABLE
#include "server/server_core.h"
#include "server/net_server.h"
#include "server/ctp_server.h"
#include "http/http_server.h"
#include "lwip/sockets.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "ctp_cmd.h"
#endif
#include "system/includes.h"
#include "os/os_api.h"
#include "fs/fs.h"
#include "dev_manager.h"
#include "time.h"
#include "sys/time.h"
#include "simple_mov_unpkg.h"
#include "simple_avi_unpkg.h"
//#include "rt_stream_pkg.h"
/* #include "action.h" */
/* #include "net_config.h" */
#include "app_config.h"
#include "app_msg.h"
#include "media_file_list.h"
#include "bplus.h"

extern u8 sd1_driver_online(void);
extern u8 sd0_driver_online(void);
#if TCFG_APP_VIDEO_REC_EN
int video_rec_repair_avi(const char *path);
#endif

int flm_sd_online(void)
{
#if (TCFG_APP_VIDEO_REC_EN || TCFG_APP_VIDEO_DEC_EN || TCFG_APP_PHOTO_EN)
#if TCFG_SD0_ENABLE
    if (!sd0_driver_online()) {
        return -1;
    }
#elif TCFG_SD1_ENABLE
    if (!sd1_driver_online()) {
        return -1;
    }
#endif
#endif
    if (dev_manager_check_by_logo((char *)SDX_DEV) == NULL) {
        return -1;
    }
    return 0;
}

/* 当前 SDK/JLFS 只完整支持 FAT32；exFAT 卡会引发文件名/回放异常。
 * dev_update.c 注释: fs_type 1=fat12 2=fat16 3=fat32 4=exfat */
int flm_sd_is_exfat(void)
{
    struct vfs_partition *part = fget_partition(CONFIG_ROOT_PATH);
    if (!part) {
        part = fget_partition("mnt/sd1");
    }
    if (!part) {
        return 0;
    }
    return part->fs_type == 4;
}

#ifdef CONFIG_ENABLE_VLIST

/* --------------------------- MARCO DEFINITIONS --------------------------- */
#define VF_LIST_VIDEO_FORMAT   "{\"y\":%d,\"f\":\"%s%s\",\"t\":\"%04d%02d%02d%02d%02d%02d\",\"d\":\"%d\",\"h\":%d,\"w\":%d,\"p\":%d,\"s\":\"%d\",\"c\":\"%d\",\"e\":\"%ld\"},"
#define VF_LIST_PHOTO_FORMAT   "{\"f\":\"%s%s\",\"t\":\"%04d%02d%02d%02d%02d%02d\",\"h\":%d,\"w\":%d,\"s\":\"%d\",\"c\":\"%d\",\"e\":\"%ld\"},"
// #define VF_LIST_VIDEO_FORMAT  "{\"y\":%d,\"f\":\"%s%s\",\"t\":\"%04d%02d%02d%02d%02d%02d\",\"d\":\"%d\",\"h\":%d,\"w\":%d,\"p\":%d,\"s\":\"%d\"},"
// #define VF_LIST_PHOTO_FORMAT "{\"f\":\"%s%s\",\"t\":\"%04d%02d%02d%02d%02d%02d\",\"h\":%d,\"w\":%d,\"s\":\"%d\"},"
#define VF_LIST_JSON_HEAD  	"{\"file_list\":["
#define VF_LIST_JSON_END  	"]}"

/* --------------------------- GLOBAL VARIABLES --------------------------- */
typedef struct {
    OS_MUTEX database_mutex;
    bool     mutex_created;
    bool     is_initialized;
    char     json_buf[512];
    int      file_count;
    char   (*names)[64];      /* 文件名缓存: flm_get_entry O(1) */
    int      names_count;
} flm_ctx_t;

static flm_ctx_t g_flm;

extern int cal_days(int, int);

static const char *get_extension(const char *path)
{
    const char *p = &path[strlen(path) - 1];
    for (; p > path && *p != '.'; p--);
    return *p == '.' ? p + 1 : path;
}

static const char *get_filename(const char *path)
{
    const char *p = &path[strlen(path) - 1];
    for (; p > path && *p != '/'; p--);
    return *p == '/' ? p + 1 : path;
}

/* JLFS 对非 UTF-8/中文短名会返回 "\U" + UTF-16LE 内部名（含 NUL）。
 * 这里把内部名转换回普通 UTF-8/ASCII 文件名，保证 B+ tree 里存的是
 * 可 fopen 的真实文件名（如 JPG_0150.JPG），而不是截断后的 "\UJ"。
 *
 * name_len > 0 时按 fget_name 返回的字节长度解析；
 * name_len <= 0 时从 name+2 开始按 UTF-16LE 扫描，遇到 0x0000 结束
 * （调用方传入的是 128 字节清零 buffer，截断后的 "\UJ" 后面仍保留完整 UTF-16LE 数据）。 */
static int flm_utf16le_to_utf8(const u8 *src, int src_len, char *dst, int dst_len)
{
    int in = 0;
    int out = 0;

    while (1) {
        if (src_len >= 0) {
            if (in + 1 >= src_len) {
                break;
            }
        } else if (in + 1 >= 126) {
            break;
        }

        u16 uc = (u16)(src[in] | (src[in + 1] << 8));
        if (src_len < 0 && uc == 0) {
            break;
        }
        /* kid-camera 自身生成的文件名均为 ASCII；遇到非 ASCII 的 \U 内部名暂不支持 */
        if (uc > 0x7F) {
            return -1;
        }
        if (out + 1 >= dst_len) {
            return -1;
        }
        dst[out++] = (char)uc;
        in += 2;
    }

    if (out >= dst_len) {
        return -1;
    }
    dst[out] = '\0';
    return out;
}

static int flm_normalize_name(const char *name, int name_len, char *out, int out_len)
{
    if (!name || !out || out_len <= 0) {
        return -1;
    }

    if (name[0] == '\\' && name[1] == 'U') {
        return flm_utf16le_to_utf8((const u8 *)name + 2,
                                   name_len > 0 ? name_len - 2 : -1,
                                   out, out_len);
    }

    int len = strlen(name);
    if (len >= out_len) {
        return -1;
    }
    memcpy(out, name, len + 1);
    return len;
}

static void incr_date_time(struct tm *tm_time, int incr)
{
    do {
        if (incr >= 60) {
            tm_time->tm_sec += 60;
            incr -= 60;
        } else {
            tm_time->tm_sec += incr;
            incr = 0;
        }

        if (tm_time->tm_sec > 59) {
            tm_time->tm_sec -= 60;
            tm_time->tm_min += 1;

            if (tm_time->tm_min > 59) {
                tm_time->tm_min -= 60;
                tm_time->tm_hour += 1;

                if (tm_time->tm_hour > 23) {
                    tm_time->tm_hour -= 24;
                    tm_time->tm_mday += 1;

                    if (tm_time->tm_mday > cal_days(tm_time->tm_year, tm_time->tm_mon)) {
                        tm_time->tm_mday = 1;
                        tm_time->tm_mon += 1;

                        if (tm_time->tm_mon > 12) {
                            tm_time->tm_mon = 1;
                            ++tm_time->tm_year;
                        }
                    }
                }
            }
        }
    } while (incr);
}

static int add_avi_file_to_database(
    bp_db_t         *database_handle,
    FILE            *fd,
    const char      *name,
    struct vfs_attr *attr,
    int              _attr)
{
    struct __mov_unpkg_info info = {0};
    u8 state = 0;

    avi_net_preview_unpkg_init(fd, state);
    if (!is_vaild_avi_file(fd, state)) {
        printf("Not a valid AVI file");
        return -1;
    }
    if (avi_get_width_height(fd, (void *)&info, state)) {
        printf("Failed to get width and height\n");
        return -1;
    }

    u8 valid = (_attr & F_ATTR_RO) ? 2 : 1;

    info.scale = 1;
    info.durition = avi_get_file_time(fd, state);
    info.sample_duration = avi_get_fps(fd, state);

    avi_net_unpkg_exit(fd, state);

    struct tm _tm;
    _tm.tm_year  = attr->crt_time.year - 1900;
    _tm.tm_mon  = attr->crt_time.month - 1;
    _tm.tm_mday  = attr->crt_time.day;
    _tm.tm_hour  = attr->crt_time.hour;
    _tm.tm_min  = attr->crt_time.min;
    _tm.tm_sec  = attr->crt_time.sec;

    incr_date_time(&_tm, info.durition / info.scale);

    int bytes_written = sprintf(g_flm.json_buf, VF_LIST_VIDEO_FORMAT
                                , valid
                                , MEDIA_VIDEO_DIR
                                , name
                                , attr->crt_time.year
                                , attr->crt_time.month
                                , attr->crt_time.day
                                , attr->crt_time.hour
                                , attr->crt_time.min
                                , attr->crt_time.sec
                                , info.durition / info.scale
                                , info.height
                                , info.length
                                , info.sample_duration
                                , flen(fd)
                                , 0
                                , mktime(&_tm));

    bp_key_t key = { .value = (char *)name, .length = strlen(name) + 1 };
    bp_value_t value = { .value = g_flm.json_buf, .length = bytes_written };
    if (bp_set(database_handle, &key, &value) != BP_OK) {
        printf("Failed to add file to database");
        return -2;
    }

    return 0;
}

/* JPEG 宽高: 跳段按 SOF marker 读, 兼容 EXIF/APPn; 失败返回 -1 */
static int jpg_get_dimensions(FILE *fd, u16 *width, u16 *height)
{
    u8 buf[5];
    long fsize = flen(fd);

    if (fsize < 12) {
        return -1;
    }
    fseek(fd, 0, SEEK_SET);
    if (fread(buf, 1, 2, fd) != 2 || buf[0] != 0xFF || buf[1] != 0xD8) {
        return -1;
    }
    while (ftell(fd) < fsize - 4) {
        if (fread(buf, 1, 2, fd) != 2 || buf[0] != 0xFF) {
            return -1;
        }
        u8 marker = buf[1];
        if (marker == 0xFF || marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7)) {
            continue;
        }
        if (marker == 0xD9 || marker == 0xDA) {
            return -1;
        }
        if (fread(buf, 1, 2, fd) != 2) {
            return -1;
        }
        u16 seg_len = ((u16)buf[0] << 8) | buf[1];
        if (seg_len < 2) {
            return -1;
        }
        if ((marker >= 0xC0 && marker <= 0xCF) && marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
            if (fread(buf, 1, 5, fd) != 5) {
                return -1;
            }
            *height = ((u16)buf[1] << 8) | buf[2];
            *width  = ((u16)buf[3] << 8) | buf[4];
            return 0;
        }
        fseek(fd, seg_len - 2, SEEK_CUR);
    }
    return -1;
}

static int add_jpg_file_to_database(
    bp_db_t         *database_handle,
    FILE            *fd,
    const char      *name,
    struct vfs_attr *attr,
    int              _attr)
{
    u16 height = 0, width = 0;
    if (jpg_get_dimensions(fd, &width, &height) != 0) {
        width  = 320;  /* 头异常按屏幕分辨率兜底, 不拒收文件 */
        height = 240;
    }

    struct tm _tm;
    _tm.tm_year  = attr->crt_time.year - 1900;
    _tm.tm_mon  = attr->crt_time.month - 1;
    _tm.tm_mday  = attr->crt_time.day;
    _tm.tm_hour  = attr->crt_time.hour;
    _tm.tm_min  = attr->crt_time.min;
    _tm.tm_sec  = attr->crt_time.sec;

    int bytes_written = sprintf(g_flm.json_buf, VF_LIST_PHOTO_FORMAT
                                , MEDIA_PHOTO_DIR
                                , name
                                , attr->crt_time.year
                                , attr->crt_time.month
                                , attr->crt_time.day
                                , attr->crt_time.hour
                                , attr->crt_time.min
                                , attr->crt_time.sec
                                , height
                                , width
                                , flen(fd)
                                , 0
                                , mktime(&_tm));

    bp_key_t key = { .value = (char *)name, .length = strlen(name) + 1 };
    bp_value_t value = { .value = g_flm.json_buf, .length = bytes_written };
    if (bp_set(database_handle, &key, &value) != BP_OK) {
        printf("Failed to add file to database");
        return -2;
    }

    return 0;
}

/* 重建线程在跑标志: 防重入, 外部扫等待复用结果 */
static volatile bool flm_rebuild_busy;

static int add_files_in_path_to_database(
    bp_db_t *database_handle,
    const char *path,
    const char *extension)
{
    char file_name[128] = {0};
    char media_name[128] = {0};
    char options[32] = {0};
    int file_count = 0;
#if TCFG_APP_VIDEO_REC_EN
    /* 损坏 AVI 先记录, 目录扫描句柄释放后再修; 扫描中 frename 源文件会失败 */
    char repair_paths[4][160];
    int repair_count = 0;
#endif

    snprintf(options, 32, "-t%s -sn", extension);

    if (fdir_exist(path) != 0) {
        return 0;   /* 目录不存在: 跳过 (fscan 库内会对不存在路径反复打印 no free files) */
    }

    struct vfscan *scan_result = fscan(path, options, 9);
    if (scan_result == NULL) {
        printf("Failed to scan path %s\n", path);
        return 0;
    }
    FILE *file_handle = fselect(scan_result, FSEL_FIRST_FILE, 0);
    if (file_handle == NULL) {
        printf("Directory %s is empty\n", path);
        fscan_release(scan_result);
        return 0;
    }

    while (1) {
        int name_len = fget_name(file_handle, (u8 *)file_name, sizeof(file_name));
        struct vfs_attr attr = {0};
        int _attr = 0;
        fget_attrs(file_handle, &attr);
        fget_attr(file_handle, &_attr);

        if (flm_normalize_name(file_name, name_len, media_name, sizeof(media_name)) < 0) {
            printf("Skip invalid media name: %s%s\n", path, file_name);
        } else {
            printf("Discovered file %s%s\n", path, media_name);

            if (!memcmp(extension, "AVI", 3)) {
                if (add_avi_file_to_database(database_handle, file_handle, media_name, &attr, _attr) == 0) {
                    file_count++;
#if TCFG_APP_VIDEO_REC_EN
                } else if (flm_rebuild_busy) {
                    /* 热拔卡未收尾 AVI: 有 RIFF 头才记录待修, 非 AVI 文件不动; 目录扫描完再统一修 */
                    u8 hdr[4] = {0};
                    fseek(file_handle, 0, SEEK_SET);
                    int has_riff = (fread(hdr, 1, 4, file_handle) == 4 && !memcmp(hdr, "RIFF", 4));
                    if (has_riff && repair_count < (int)(sizeof(repair_paths) / sizeof(repair_paths[0]))) {
                        snprintf(repair_paths[repair_count], sizeof(repair_paths[repair_count]),
                                 "%s%s", path, media_name);
                        repair_count++;
                    }
#endif
                }
            } else if (!memcmp(extension, "JPG", 3)) {
                if (add_jpg_file_to_database(database_handle, file_handle, media_name, &attr, _attr) == 0) {
                    file_count++;
                }
            } else {
                printf("Unknown file type, skipping...");
            }
        }

        if (file_handle) {
            fclose(file_handle);
        }
        file_handle = fselect(scan_result, FSEL_NEXT_FILE, 0);
        if (file_handle == NULL) {
            break;
        }
    }

    fscan_release(scan_result);

#if TCFG_APP_VIDEO_REC_EN
    /* 目录句柄已释放, 逐个修复; 修复成功后重新打开入库 */
    for (int i = 0; i < repair_count; i++) {
        if (video_rec_repair_avi(repair_paths[i]) == 0) {
            FILE *rfh = fopen(repair_paths[i], "r");
            if (rfh) {
                struct vfs_attr rattr = {0};
                int r_attr = 0;
                fget_attrs(rfh, &rattr);
                fget_attr(rfh, &r_attr);
                const char *rname = strrchr(repair_paths[i], '/');
                rname = rname ? (rname + 1) : repair_paths[i];
                if (add_avi_file_to_database(database_handle, rfh, rname, &rattr, r_attr) == 0) {
                    file_count++;
                }
                fclose(rfh);
            }
        }
    }
#endif

    return file_count;
}

static int accumulate(void *arg, const bp_key_t *key, const bp_value_t *value)
{
    (*(int *)arg)++;
    return 0;
}

static void flm_mutex_ensure(void)
{
    if (!g_flm.mutex_created) {
        os_mutex_create(&g_flm.database_mutex);
        g_flm.mutex_created = true;
    }
}

struct flm_name_ctx {
    char (*names)[64];
    int  cap;
    int  count;
};

static int flm_name_cb(void *arg, const bp_key_t *key, const bp_value_t *value)
{
    (void)value;
    struct flm_name_ctx *ctx = (struct flm_name_ctx *)arg;
    if (ctx->count < ctx->cap) {
        snprintf(ctx->names[ctx->count], 64, "%s", key->value);
    }
    ctx->count++;
    return 0;
}

/* 需已持 database_mutex; >1500 文件不缓存, 走慢路径兜底 */
static void flm_names_cache_build_locked(void)
{
    if (g_flm.names) {
        free(g_flm.names);
        g_flm.names = NULL;
        g_flm.names_count = 0;
    }
    if (g_flm.file_count <= 0 || (size_t)g_flm.file_count * 64 > 96 * 1024) {
        return;
    }
    bp_db_t db;
    if (bp_open(&db, MEDIA_DB_PATH) != BP_OK) {
        return;
    }
    g_flm.names = (char (*)[64])malloc((size_t)g_flm.file_count * 64);
    if (g_flm.names) {
        struct flm_name_ctx ctx = { .names = g_flm.names, .cap = g_flm.file_count, .count = 0 };
        bp_get_ranges(&db, "/", "z", flm_name_cb, &ctx);
        g_flm.names_count = ctx.count;
    }
    bp_close(&db);
}

/* 任何索引变更都必须显式失效缓存: 同数量换文件时 names_count==file_count 会误命中旧路径 */
static void flm_names_cache_invalidate(void)
{
    if (g_flm.names) {
        free(g_flm.names);
        g_flm.names = NULL;
    }
    g_flm.names_count = 0;
}

static int flm_do_scan_and_load(void)
{
    flm_mutex_ensure();
    if (g_flm.is_initialized) {
        return 0;
    }

    /* 每次扫描前删除旧数据库, 重建干净索引 (INDEX.DAT 持久化,
     * 旧固件残留 key 会让 flm_get_entry 索引错乱导致回放卡住)。 */
    FILE *db_old = fopen(MEDIA_DB_PATH, "r");
    if (db_old) {
        fdelete(db_old);
        db_old = NULL;
    }

    bp_db_t database_handle = {0};
    if (bp_open(&database_handle, MEDIA_DB_PATH) != BP_OK) {
        printf("Failed to open database %s\n", MEDIA_DB_PATH);
        /* 写不进即停: 不再重试重扫, 清索引防 dec 读旧数据 (重插卡重建恢复) */
        g_flm.is_initialized = true;
        g_flm.file_count = 0;
        flm_names_cache_invalidate();
        return -1;
    }

    int file_count = 0;
    file_count += add_files_in_path_to_database(&database_handle, MEDIA_VIDEO_DIR, "AVI");
    file_count += add_files_in_path_to_database(&database_handle, MEDIA_PHOTO_DIR, "JPG");

    bp_close(&database_handle);

    g_flm.is_initialized = true;
    g_flm.file_count = file_count;
    flm_names_cache_invalidate();

    return file_count;
}

int flm_scan_and_load(void)
{
    flm_mutex_ensure();
    if (g_flm.is_initialized) {
        return 0;
    }
    /* 重建线程在跑: 等其完成复用结果, 避免双跑 (app_core 只等待, 深调用在重建线程栈) */
    while (flm_rebuild_busy) {
        os_time_dly(20);
    }
    if (g_flm.is_initialized) {
        return 0;
    }
    // 需要重建时走异步线程(8KB栈, FATFS深链不压app_core 3KB栈)
    flm_rebuild_async();
    while (flm_rebuild_busy) {
        os_time_dly(20);
    }
    return g_flm.is_initialized ? 0 : flm_do_scan_and_load();
}

/* 插卡等场景: 强制重建媒体索引 (与当前卡内容同步)。
 * 只清 is_initialized 挡板让下次 scan 重建, 不删除任何媒体文件
 * (勿用 flm_free_all: 那是删除全部照片/视频的接口)。 */
void flm_rebuild(void)
{
    g_flm.is_initialized = false;
    flm_do_scan_and_load();
}

/* 拔卡清媒体索引状态(不删文件), 插卡后由 flm_rebuild_async 重建 */
void flm_reset(void)
{
    flm_mutex_ensure();
    os_mutex_pend(&g_flm.database_mutex, 0);
    g_flm.is_initialized = false;
    g_flm.file_count = 0;
    flm_names_cache_invalidate();
    os_mutex_post(&g_flm.database_mutex);
}

/* 重建放独立线程: app_core 同步扫深调用链会栈溢出 (开机必经路径, app_core 栈仅 768) */
static void flm_rebuild_thread(void *arg)
{
    int i;
    for (i = 0; i < 30; i++) {
        if (fget_partition(CONFIG_ROOT_PATH)) {
            break;
        }
        os_time_dly(100);
    }
    flm_mutex_ensure();
    os_mutex_pend(&g_flm.database_mutex, 0);
    flm_rebuild();
    os_mutex_post(&g_flm.database_mutex);
    flm_rebuild_busy = false;
    /* 完成即通知: dec 插卡刷新由此驱动, 不忙等 */
    app_send_message(APP_MSG_FLM_REBUILD_DONE, 0);
}

/* 插卡等场景: 异步重建 (app_core 只投递, 重建在独立大栈线程) */
void flm_rebuild_async(void)
{
    if (flm_rebuild_busy) {
        return;
    }
    flm_rebuild_busy = true;
    // FATFS深链+normalize局部256B, 4KB栈删除后重建会溢出

    if (thread_fork("flm_rebuild", 20, 8192, 0, NULL, flm_rebuild_thread, NULL) != OS_NO_ERR) {
        flm_rebuild_busy = false;
    }
}

int flm_get_count(void)
{
    return g_flm.file_count;
}

struct flm_find_ctx {
    int target;
    int current;
    flm_file_t *out;
};

static int flm_find_by_idx_cb(void *arg, const bp_key_t *key, const bp_value_t *value)
{
    struct flm_find_ctx *ctx = (struct flm_find_ctx *)arg;
    (void)value;
    if (ctx->current == ctx->target) {
        strncpy(ctx->out->name, key->value, sizeof(ctx->out->name) - 1);
        ctx->out->name[sizeof(ctx->out->name) - 1] = '\0';

        const char *ext = strrchr(ctx->out->name, '.');
        if (ext && (!memcmp(ext + 1, "JPG", 3) || !memcmp(ext + 1, "jpg", 3))) {
            ctx->out->type = 0;
            snprintf(ctx->out->path, sizeof(ctx->out->path), "%s%s", MEDIA_PHOTO_DIR, ctx->out->name);
        } else {
            ctx->out->type = 1;
            snprintf(ctx->out->path, sizeof(ctx->out->path), "%s%s", MEDIA_VIDEO_DIR, ctx->out->name);
        }
        ctx->out->thumbnail = NULL;
        /* bp 库约定回调返回 <0 才停止遍历 (见 bp__page_get_range).
         * 原 return 1 不停止: current 卡在 target, 后续所有 key 都匹配
         * 覆盖填 out, 最终返回最后一个 key (回放所有 index 都指向末张照片). */
        return -1;
    }
    ctx->current++;
    return 0;
}

static int flm_get_entry_slow(int index, flm_file_t *out)
{
    bp_db_t db;
    struct flm_find_ctx ctx;

    if (index < 0 || index >= g_flm.file_count || !out) {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    ctx.target = index;
    ctx.current = 0;
    ctx.out = out;

    os_mutex_pend(&g_flm.database_mutex, 0);
    if (bp_open(&db, MEDIA_DB_PATH) != BP_OK) {
        os_mutex_post(&g_flm.database_mutex);
        return -1;
    }
    bp_get_ranges(&db, "/", "z", flm_find_by_idx_cb, &ctx);
    bp_close(&db);
    os_mutex_post(&g_flm.database_mutex);

    if (out->name[0] == '\0') {
        return -1;
    }
    return 0;
}

int flm_get_entry(int index, flm_file_t *out)
{
    if (index < 0 || index >= g_flm.file_count || !out) {
        return -1;
    }
    memset(out, 0, sizeof(*out));

    os_mutex_pend(&g_flm.database_mutex, 0);
    if (!g_flm.names || g_flm.names_count != g_flm.file_count) {
        flm_names_cache_build_locked();
    }
    if (g_flm.names && index < g_flm.names_count) {
        snprintf(out->name, sizeof(out->name), "%s", g_flm.names[index]);
        os_mutex_post(&g_flm.database_mutex);
        const char *ext = strrchr(out->name, '.');
        if (ext && (!memcmp(ext + 1, "JPG", 3) || !memcmp(ext + 1, "jpg", 3))) {
            out->type = 0;
            snprintf(out->path, sizeof(out->path), "%s%s", MEDIA_PHOTO_DIR, out->name);
        } else {
            out->type = 1;
            snprintf(out->path, sizeof(out->path), "%s%s", MEDIA_VIDEO_DIR, out->name);
        }
        out->thumbnail = NULL;
        return 0;
    }
    os_mutex_post(&g_flm.database_mutex);
    return flm_get_entry_slow(index, out);
}

int flm_delete_file(const char *path)
{
    FILE *fp;
    int ret;

    if (!path || !path[0]) {
        return -1;
    }

    if (flm_is_locked(path)) {
        printf("flm_delete_file: %s is locked, skip\n", path);
        return -2;
    }

    fp = fopen(path, "r");
    if (!fp) {
        printf("flm_delete_file: fopen(%s) failed\n", path);
        return -1;
    }
    ret = fdelete(fp);
    if (ret != 0) {
        printf("flm_delete_file: fdelete(%s) failed\n", path);
        fclose(fp);
        return -1;
    }

    /* Remove from database by filename */
    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;

    flm_mutex_ensure();
    os_mutex_pend(&g_flm.database_mutex, 0);
    bp_db_t db;
    if (bp_open(&db, MEDIA_DB_PATH) == BP_OK) {
        bp_removes(&db, name);
        bp_close(&db);
    }
    flm_names_cache_invalidate();
    os_mutex_post(&g_flm.database_mutex);

    g_flm.file_count--;
    printf("flm_delete_file: deleted %s\n", path);
    return 0;
}

int flm_lock_file(const char *path)
{
    FILE *fp;
    if (!path || !path[0]) {
        return -1;
    }
    fp = fopen(path, "r+");
    if (!fp) {
        return -1;
    }
    int ret = fset_attr(fp, F_ATTR_RO);
    fclose(fp);
    if (ret == 0) {
        /* Re-emit DB entry with updated "y"=2 (locked) */
        fp = fopen(path, "r");
        if (fp) {
            struct vfs_attr attr = {0};
            int _attr = 0;
            fget_attrs(fp, &attr);
            fget_attr(fp, &_attr);
            bp_db_t db;
            os_mutex_pend(&g_flm.database_mutex, 0);
            if (bp_open(&db, MEDIA_DB_PATH) == BP_OK) {
                if (!memcmp(get_extension(path), "JPG", 3)) {
                    add_jpg_file_to_database(&db, fp, get_filename(path), &attr, _attr);
                } else {
                    add_avi_file_to_database(&db, fp, get_filename(path), &attr, _attr);
                }
                bp_close(&db);
            }
            os_mutex_post(&g_flm.database_mutex);
            fclose(fp);
        }
    }
    return ret;
}

int flm_unlock_file(const char *path)
{
    FILE *fp;
    if (!path || !path[0]) {
        return -1;
    }
    fp = fopen(path, "r+");
    if (!fp) {
        return -1;
    }
    int ret = fset_attr(fp, 0);
    fclose(fp);
    if (ret == 0) {
        /* Re-emit DB entry with updated "y"=1 (unlocked) */
        fp = fopen(path, "r");
        if (fp) {
            struct vfs_attr attr = {0};
            int _attr = 0;
            fget_attrs(fp, &attr);
            fget_attr(fp, &_attr);
            bp_db_t db;
            os_mutex_pend(&g_flm.database_mutex, 0);
            if (bp_open(&db, MEDIA_DB_PATH) == BP_OK) {
                if (!memcmp(get_extension(path), "JPG", 3)) {
                    add_jpg_file_to_database(&db, fp, get_filename(path), &attr, _attr);
                } else {
                    add_avi_file_to_database(&db, fp, get_filename(path), &attr, _attr);
                }
                bp_close(&db);
            }
            os_mutex_post(&g_flm.database_mutex);
            fclose(fp);
        }
    }
    return ret;
}

int flm_is_locked(const char *path)
{
    FILE *fp;
    struct vfs_attr attr;
    if (!path || !path[0]) {
        return 0;
    }
    fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }
    memset(&attr, 0, sizeof(attr));
    int ret = fget_attrs(fp, &attr);
    fclose(fp);
    return (ret == 0 && (attr.attr & F_ATTR_RO)) ? 1 : 0;
}

int flm_get_storage_info(u32 *total_kb, u32 *free_kb, u32 *photo_cnt, u32 *video_cnt)
{
    struct vfs_partition *p = fget_partition(CONFIG_ROOT_PATH);

    if (!p) {
        p = fget_partition(CONFIG_STORAGE_PATH);
    }
    if (p && total_kb) {
        *total_kb = p->total_size;
    }

    u32 fs = 0;
    if (fget_free_space(CONFIG_ROOT_PATH, &fs) != 0) {
        fget_free_space(CONFIG_STORAGE_PATH, &fs);
    }
    if (free_kb) {
        *free_kb = fs;
    }

    /* Count by type from DB entries */
    bp_db_t db;
    os_mutex_pend(&g_flm.database_mutex, 0);
    if (bp_open(&db, MEDIA_DB_PATH) == BP_OK) {
        int total = 0;
        bp_get_ranges(&db, "/", "z", accumulate, &total);
        bp_close(&db);
        /* Approximation: we don't track per-type count in DB;
         * g_flm.file_count is total. Caller can count from entries. */
    }
    os_mutex_post(&g_flm.database_mutex);

    /* Count by iterating entries via flm_get_entry */
    int i;
    if (photo_cnt) {
        *photo_cnt = 0;
        for (i = 0; i < g_flm.file_count; i++) {
            flm_file_t f;
            if (flm_get_entry(i, &f) == 0 && f.type == 0) {
                (*photo_cnt)++;
            }
        }
    }
    if (video_cnt) {
        *video_cnt = g_flm.file_count - (photo_cnt ? *photo_cnt : 0);
    }

    return 0;
}

int flm_free_all(void)
{
    struct vfscan *fs;
    FILE *fp;
    int deleted = 0;

    /* Delete all photo files */
    fs = fscan(MEDIA_PHOTO_DIR, "-tJPG -sn", 1);
    if (fs) {
        fp = fselect(fs, FSEL_FIRST_FILE, 0);
        while (fp != NULL) {
            int _attr = 0;
            fget_attr(fp, &_attr);
            if (_attr & F_ATTR_RO) {
                fclose(fp);  /* 只读跳过: 反复 FSEL_FIRST 会死循环 */
            } else {
                fdelete(fp);
                deleted++;
            }
            fp = fselect(fs, FSEL_NEXT_FILE, 0);
        }
        fscan_release(fs);
    }

    /* Delete all video files */
    fs = fscan(MEDIA_VIDEO_DIR, "-tAVI -sn", 1);
    if (fs) {
        fp = fselect(fs, FSEL_FIRST_FILE, 0);
        while (fp != NULL) {
            int _attr = 0;
            fget_attr(fp, &_attr);
            if (_attr & F_ATTR_RO) {
                fclose(fp);
            } else {
                fdelete(fp);
                deleted++;
            }
            fp = fselect(fs, FSEL_NEXT_FILE, 0);
        }
        fscan_release(fs);
    }

    /* Delete the database file */
    fp = fopen(MEDIA_DB_PATH, "r");
    if (fp) {
        fdelete(fp);
    }

    flm_mutex_ensure();
    os_mutex_pend(&g_flm.database_mutex, 0);
    g_flm.file_count = 0;
    flm_names_cache_invalidate();
    os_mutex_post(&g_flm.database_mutex);
    /* 库与数据库文件已清空, 重置初始化标记: 否则下次 flm_scan_and_load
     * 被 g_flm.is_initialized 挡板直接 return 0, 空库不重建。 */
    g_flm.is_initialized = false;
    return deleted;
}

struct json_writer_context {
    FILE *file_handle;
    u32   file_size;
};

static int json_writer(void *arg, const bp_key_t *key, const bp_value_t *value)
{
    struct json_writer_context *ctx = (struct json_writer_context *)arg;

    // printf("%s: key=%s, value=%.*s\n", __FUNCTION__, key->value, (int)value->length, value->value);

    fwrite(value->value, value->length, 1, ctx->file_handle);

    ctx->file_size += value->length;

    return 0;
}

int flm_dump_json(void)
{
    int ret = 0;
    bp_db_t database_handle = {0};

    os_mutex_pend(&g_flm.database_mutex, 0);
    if (bp_open(&database_handle, MEDIA_DB_PATH) != BP_OK) {
        ret = -1;
        goto exit0;
    }

    FILE *file_handle = fopen(MEDIA_VF_LIST, "w+");
    if (file_handle == NULL) {
        ret = -1;
        goto exit1;
    }

    int header_size = fwrite(VF_LIST_JSON_HEAD, strlen(VF_LIST_JSON_HEAD), 1, file_handle);

    struct json_writer_context ctx = { .file_handle = file_handle, .file_size = header_size };

    if (bp_get_ranges(&database_handle, "/", "z", json_writer, &ctx) != BP_OK) {
        printf("Failed to dump json\n");
        ret = -1;
        goto exit2;
    }

    /* Remove trailing comma */
    if (ctx.file_size > 0) {
        fseek(file_handle, ctx.file_size - 1, SEEK_SET);
    }

    fwrite(VF_LIST_JSON_END, strlen(VF_LIST_JSON_END), 1, file_handle);

exit2:
    fclose(file_handle);
exit1:
    bp_close(&database_handle);
exit0:
    os_mutex_post(&g_flm.database_mutex);
    return ret;
}

int flm_add(void *fp, const char *dir_path)
{
    flm_mutex_ensure();
    FILE *file_handle = (FILE *)fp;
    char filename[128] = {0};
    char media_name[128] = {0};
    int name_len = fget_name(file_handle, (u8 *)filename, sizeof(filename));
    if (flm_normalize_name(filename, name_len, media_name, sizeof(media_name)) < 0) {
        printf("flm_add: invalid media name: %s\n", filename);
        return -1;
    }
    printf("%s: dir=%s, fname=%s\n", __FUNCTION__, dir_path, media_name);

    struct vfs_attr attr = {0};
    int _attr = 0;
    fget_attrs(file_handle, &attr);
    fget_attr(file_handle, &_attr);

    os_mutex_pend(&g_flm.database_mutex, 0);
    bp_db_t database_handle = {0};
    int ret = -1;
    if (bp_open(&database_handle, MEDIA_DB_PATH) == BP_OK) {
        char *qval = NULL;
        if (bp_gets(&database_handle, media_name, &qval) == BP_OK) {
            ret = 0;  /* 已入库: 幂等, 不重复计数 */
        } else {
            if (!memcmp(get_extension(media_name), "JPG", 3)) {
                ret = add_jpg_file_to_database(&database_handle, file_handle, media_name, &attr, _attr);
            } else if (!memcmp(get_extension(media_name), "AVI", 3)) {
                ret = add_avi_file_to_database(&database_handle, file_handle, media_name, &attr, _attr);
            } else {
                printf("Unknown file type\n");
            }
            if (ret == 0) {
                printf("Successfully added %s to database\n", media_name);
                g_flm.file_count++;
                flm_names_cache_invalidate();
            }
        }
        bp_close(&database_handle);
    }
    os_mutex_post(&g_flm.database_mutex);

    return ret;
}

void FILE_LIST_ADD(u32 status, const char *__path, u8 create_file)
{
}

void FILE_DELETE(char *__fname, u8 create_file)
{
}

void FILE_GEN()
{
    flm_dump_json();
}

void FILE_CHANGE_ATTR(const char *fname, char attr)
{
}

/* 按文件名入库 (事件延迟, JLFS 写句柄已落盘, 避免立即重开读破坏缓冲) */
int flm_add_by_name(const char *path, const char *filename)
{
#if (TCFG_APP_VIDEO_REC_EN || TCFG_APP_VIDEO_DEC_EN || TCFG_APP_PHOTO_EN)
#if TCFG_SD0_ENABLE
    if (!sd0_driver_online()) {
        return -1;
    }
#elif TCFG_SD1_ENABLE
    if (!sd1_driver_online()) {
        return -1;
    }
#endif
#endif
    char media_name[128] = {0};
    if (flm_normalize_name(filename, 0, media_name, sizeof(media_name)) < 0) {
        printf("flm_add_by_name: invalid media name: %s\n", filename);
        return -1;
    }

    flm_mutex_ensure();
    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s%s", path, media_name);
    FILE *file_handle = fopen(full_path, "r");
    if (file_handle == NULL) {
        printf("Failed to open file %s\n", full_path);
        return -1;
    }

    struct vfs_attr attr = {0};
    int _attr = 0;
    fget_attrs(file_handle, &attr);
    fget_attr(file_handle, &_attr);

    os_mutex_pend(&g_flm.database_mutex, 0);
    bp_db_t database_handle = {0};
    int ret = -1;
    if (bp_open(&database_handle, MEDIA_DB_PATH) == BP_OK) {
        char *qval = NULL;
        if (bp_gets(&database_handle, media_name, &qval) == BP_OK) {
            ret = 0;  /* 已入库: 幂等, 不重复计数 */
        } else {
            if (!memcmp(get_extension(media_name), "JPG", 3)) {
                ret = add_jpg_file_to_database(&database_handle, file_handle, media_name, &attr, _attr);
            } else if (!memcmp(get_extension(media_name), "AVI", 3)) {
                ret = add_avi_file_to_database(&database_handle, file_handle, media_name, &attr, _attr);
            } else {
                printf("Unknown file type\n");
            }
            if (ret == 0) {
                printf("Successfully added %s to database\n", media_name);
                g_flm.file_count++;
                flm_names_cache_invalidate();
            }
        }
        bp_close(&database_handle);
    }
    os_mutex_post(&g_flm.database_mutex);
    fclose(file_handle);
    return ret;
}

#else // !CONFIG_ENABLE_VLIST

__attribute__((weak))
int flm_add(void *fp, const char *dir_path)
{
    (void)fp;
    (void)dir_path;
    return 0;
}

__attribute__((weak))
int flm_add_by_name(const char *path, const char *filename)
{
    (void)path;
    (void)filename;
    return 0;
}

#endif // CONFIG_ENABLE_VLIST

__attribute__((weak))
const char *get_rec_path_1(void)
{
    return "";
}

__attribute__((weak))
const char *get_rec_path_2(void)
{
    return "";
}

__attribute__((weak))
const char *get_rec_path_3(void)
{
    return "";
}


