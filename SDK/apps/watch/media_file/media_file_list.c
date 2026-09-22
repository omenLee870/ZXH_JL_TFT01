#include "app_config.h"

#if TCFG_NETWORK_ENABLE
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

/* 重建进行中查询: 录像 SD IN 自动开录先等重建完成, 避免并发抢 SD 读与内存 */
int flm_rebuild_in_progress(void)
{
    return flm_rebuild_busy ? 1 : 0;
}

/* 坏名目录项限流: 只打前几条示例, 扫描结束再补一条汇总, 防刷屏堵串口 */
#define FLM_BAD_NAME_SHOW_MAX   3

/* 坏文件(空/截断)不入库, 但不动卡上文件。
 * 第一层: fget_attrs 取到的目录项大小 fsize, 零额外 IO;
 * 第二层: 仅尺寸可疑(< 结构下限)才读头确认, 只对少数文件产生 IO。
 * 阈值取结构下限: AVI = RIFF(4)+size(4)+"AVI "(4); JPG = SOI(2)+最小段头。 */
#define FLM_AVI_MIN_SIZE    12u
#define FLM_JPG_MIN_SIZE    12u

static int flm_media_bad_file(FILE *fd, u32 file_size, u8 type, int *empty)
{
    u8 hdr[4] = {0};

    if (file_size == 0) {
        (*empty)++;
        return 1;
    }
    if (file_size >= (type ? FLM_AVI_MIN_SIZE : FLM_JPG_MIN_SIZE)) {
        return 0;
    }
    fseek(fd, 0, SEEK_SET);
    if (type) {
        return !(fread(hdr, 1, 4, fd) == 4 && !memcmp(hdr, "RIFF", 4));
    }
    return !(fread(hdr, 1, 2, fd) == 2 && hdr[0] == 0xFF && hdr[1] == 0xD8);
}

static int add_files_in_path_to_database(
    bp_db_t *database_handle,
    const char *path,
    const char *extension,
    int *bad_cnt,
    int *empty_cnt)
{
    char file_name[128] = {0};
    char media_name[128] = {0};
    char options[32] = {0};
    int file_count = 0;
    int invalid_name_cnt = 0;
    int invalid_name_shown = 0;
    char invalid_name_first[64] = {0};
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
        int attrs_ok = (fget_attrs(file_handle, &attr) == 0);
        fget_attr(file_handle, &_attr);

        if (flm_normalize_name(file_name, name_len, media_name, sizeof(media_name)) < 0) {
            /* 名字无效分支: file_name 未必 NUL 结尾, 一律以 name_len 为上界打印 */
            int bad_len = (name_len > 0) ? name_len : 0;

            if (bad_len > (int)sizeof(file_name)) {
                bad_len = (int)sizeof(file_name);
            }
            if (invalid_name_cnt == 0) {
                snprintf(invalid_name_first, sizeof(invalid_name_first), "%.*s", bad_len, file_name);
            }
            if (invalid_name_shown < FLM_BAD_NAME_SHOW_MAX) {
                printf("Skip invalid media name: %s%.*s\n", path, bad_len, file_name);
                invalid_name_shown++;
            }
            invalid_name_cnt++;
        } else {
            printf("Discovered file %s%s\n", path, media_name);

            if (attrs_ok && flm_media_bad_file(file_handle, attr.fsize,
                                               !memcmp(extension, "AVI", 3) ? 1 : 0,
                                               empty_cnt)) {
                (*bad_cnt)++;
            } else if (!memcmp(extension, "AVI", 3)) {
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

    if (invalid_name_cnt > 0) {
        printf("[FLM] invalid media name: total=%d (shown=%d, first=\"%s\")\n",
               invalid_name_cnt, invalid_name_shown, invalid_name_first);
    }

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

/* 过滤回调版的名字收集: 在库【加载 value 之前】执行, 只拿 key, 恒返回 0 =>
 * 一个 value 都不加载 => 省掉遍历里每 key 一次的 malloc/free + 后续 free。
 * 实测这是"建缓存要 2.1 秒 + 期间刷 [P_MEM]"的主因(遍历 1450 条)。 */
static int flm_name_filter(void *arg, const bp_key_t *key)
{
    struct flm_name_ctx *ctx = (struct flm_name_ctx *)arg;
    if (ctx->count < ctx->cap) {
        snprintf(ctx->names[ctx->count], 64, "%s", key->value);
    }
    ctx->count++;
    return 0;               /* 恒 0: 不加载 value、不调 range cb */
}

/* range 回调: filter 恒返回 0, 本回调不会被调用; 仅为满足 API 形参 */
static int flm_name_noop_cb(void *arg, const bp_key_t *key, const bp_value_t *value)
{
    (void)arg;
    (void)key;
    (void)value;
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
        printf("[FLM] names cache SKIP: bp_open fail\n");
        return;
    }
    g_flm.names = (char (*)[64])malloc((size_t)g_flm.file_count * 64);
    if (g_flm.names) {
        struct flm_name_ctx ctx = { .names = g_flm.names, .cap = g_flm.file_count, .count = 0 };
        bp_get_filtered_ranges(&db, "/", "z", flm_name_filter, flm_name_noop_cb, &ctx);
        g_flm.names_count = ctx.count;
    }
    bp_close(&db);
}

/* 文件名小 LRU: 只缓存最近访问的 N 条, 替代原全库 1372x64=87.8KB 常驻缓存 */
#define FLM_NAME_LRU_N   64
/* 慢路径全库遍历时顺手预取的邻居条数(预取目标前后各 N 条)。
 * 必须 < FLM_NAME_LRU_N/2, 否则预取会把自己挤出 LRU。 */
#define FLM_LRU_PREFETCH_N   8
static struct {
    int  idx[FLM_NAME_LRU_N];      /* -1 = 空槽 */
    char name[FLM_NAME_LRU_N][64];
    u32  stamp[FLM_NAME_LRU_N];    /* 最近使用序号, 越小越旧 */
    u32  tick;
} g_name_lru;

static u8 g_name_lru_ready;

static void flm_name_lru_reset(void)
{
    int i;

    for (i = 0; i < FLM_NAME_LRU_N; i++) {
        g_name_lru.idx[i] = -1;
    }
    g_name_lru.tick = 0;
    g_name_lru_ready = 1;
}

// 静态区零初值时 idx[] 全为 0, 会把 0 号文件误判成命中, 故首次访问前先初始化
static void flm_name_lru_ensure(void)
{
    if (!g_name_lru_ready) {
        flm_name_lru_reset();
    }
}

// 命中返回 name 指针(调用方须持 database_mutex), 未命中返回 NULL
static const char *flm_name_lru_get(int index)
{
    int i;

    flm_name_lru_ensure();
    for (i = 0; i < FLM_NAME_LRU_N; i++) {
        if (g_name_lru.idx[i] == index) {
            g_name_lru.stamp[i] = ++g_name_lru.tick;
            return g_name_lru.name[i];
        }
    }
    return NULL;
}

// 调用方须持 database_mutex; 无空槽时淘汰 stamp 最小的一条
static void flm_name_lru_put(int index, const char *name)
{
    int i;
    int victim = -1;

    flm_name_lru_ensure();
    for (i = 0; i < FLM_NAME_LRU_N; i++) {
        if (g_name_lru.idx[i] == index || g_name_lru.idx[i] == -1) {
            victim = i;
            break;
        }
    }
    if (victim < 0) {
        victim = 0;
        for (i = 1; i < FLM_NAME_LRU_N; i++) {
            if (g_name_lru.stamp[i] < g_name_lru.stamp[victim]) {
                victim = i;
            }
        }
    }
    g_name_lru.idx[victim] = index;
    snprintf(g_name_lru.name[victim], sizeof(g_name_lru.name[victim]), "%s", name);
    g_name_lru.stamp[victim] = ++g_name_lru.tick;
}

/* 任何索引变更都必须显式失效缓存: 同数量换文件时 names_count==file_count 会误命中旧路径 */
static void flm_names_cache_invalidate(void)
{
    if (g_flm.names) {
        free(g_flm.names);
        g_flm.names = NULL;
    }
    g_flm.names_count = 0;
    flm_name_lru_reset();
}

/* 全库名字缓存: 进多宫格页面前建(此刻池子最宽), 离开页面【或开始播放】时释放。
 * 目的: 浏览全程不再 miss, 从而不触发全库遍历 —— 那在碎片化物理池里每次 open
 * 都要一块 ~160KB 连续内存(pages.c 页结构), 拿不到就刷 P_MEM 风暴。
 * 常驻不放会白占 87.8KB(1372 文件 x 64B), 故必须与 release 成对。 */
void flm_names_cache_prepare(void)
{
    os_mutex_pend(&g_flm.database_mutex, 0);
    if (!g_flm.names || g_flm.names_count != g_flm.file_count) {
        flm_names_cache_build_locked();
    }
    os_mutex_post(&g_flm.database_mutex);
}

void flm_names_cache_release(void)
{
    os_mutex_pend(&g_flm.database_mutex, 0);
    flm_names_cache_invalidate();
    os_mutex_post(&g_flm.database_mutex);
}

/* ---- 开机重建: 流式目录diff(免解析头) + 分段bulk写库; 内存只随 DB key 数增长 ---- */
#define FLM_SCAN_BATCH      64u

typedef struct {
    char name[64];
    u8  type;      //0=PHOTO 1=VIDEO
    u8  to_write;
    u32 fsize;
    struct vfs_attr attr;
    int _attr;
} flm_scan_file_t;

struct flm_key_ctx {
    char (*names)[64];
    int count;
    int cap;
    int overflow;
};

static int flm_key_collect_cb(void *arg, const bp_key_t *key, const bp_value_t *value)
{
    struct flm_key_ctx *ctx = (struct flm_key_ctx *)arg;

    (void)value;
    if (ctx->count >= ctx->cap) {
        ctx->overflow = 1;
        return -1;
    }
    strncpy(ctx->names[ctx->count], key->value, 63);
    ctx->names[ctx->count][63] = '\0';
    ctx->count++;
    return 0;
}

/* 名字 → 下标: 开放寻址(FNV-1a + 线性探测), 槽存"下标+1"(0=空) */
struct flm_scan_idx {
    char (*keys)[64];   /* DB key 名字表(所有权仍归 flm_key_ctx) */
    u8  *seen;          /* 本次扫描命中的 key 位图 */
    u32 *slots;         /* 槽数取 >= 2×key_cnt 的 2 的幂 */
    int key_cnt;
    int slot_cnt;
};

static u32 flm_name_hash(const char *s)
{
    u32 h = 2166136261u;

    while (*s) {
        h ^= (u8) * s++;
        h *= 16777619u;
    }
    return h;
}

/* key_cnt==0 时 slots==NULL, 查询恒不命中(全部按新增处理) */
static int flm_scan_idx_build(struct flm_scan_idx *idx, char (*keys)[64], int key_cnt)
{
    int i;

    memset(idx, 0, sizeof(*idx));
    idx->keys = keys;
    idx->key_cnt = key_cnt;
    if (key_cnt <= 0) {
        return 0;
    }
    idx->seen = malloc((size_t)key_cnt);
    idx->slot_cnt = 2;
    while (idx->slot_cnt < key_cnt * 2) {
        idx->slot_cnt <<= 1;
    }
    idx->slots = malloc((size_t)idx->slot_cnt * sizeof(u32));
    if (idx->seen == NULL || idx->slots == NULL) {
        free(idx->seen);
        free(idx->slots);
        memset(idx, 0, sizeof(*idx));
        return -1;
    }
    memset(idx->seen, 0, (size_t)key_cnt);
    memset(idx->slots, 0, (size_t)idx->slot_cnt * sizeof(u32));
    for (i = 0; i < key_cnt; i++) {
        u32 h = flm_name_hash(keys[i]) & (u32)(idx->slot_cnt - 1);

        while (idx->slots[h] != 0) {
            h = (h + 1) & (u32)(idx->slot_cnt - 1);
        }
        idx->slots[h] = (u32)(i + 1);
    }
    return 0;
}

static void flm_scan_idx_free(struct flm_scan_idx *idx)
{
    free(idx->seen);
    free(idx->slots);
    memset(idx, 0, sizeof(*idx));
}

/* 命中返回 key 下标, 未命中/空表返回 -1 */
static int flm_key_idx(const struct flm_scan_idx *idx, const char *name)
{
    u32 h;
    int probe;

    if (idx->slots == NULL || idx->slot_cnt <= 0) {
        return -1;
    }
    h = flm_name_hash(name) & (u32)(idx->slot_cnt - 1);
    for (probe = 0; probe < idx->slot_cnt; probe++) {
        u32 i = (h + (u32)probe) & (u32)(idx->slot_cnt - 1);

        if (idx->slots[i] == 0) {
            return -1;
        }
        if (strcmp(idx->keys[idx->slots[i] - 1], name) == 0) {
            return (int)(idx->slots[i] - 1);
        }
    }
    return -1;
}

//固定元数据 json(宽高/fps/时长无人消费, 本机录制格式固定): 返回 json 长度
static int flm_fill_json(const flm_scan_file_t *e, char *buf, int buf_len)
{
    struct tm _tm;
    int bytes;

    _tm.tm_year = e->attr.crt_time.year - 1900;
    _tm.tm_mon  = e->attr.crt_time.month - 1;
    _tm.tm_mday = e->attr.crt_time.day;
    _tm.tm_hour = e->attr.crt_time.hour;
    _tm.tm_min  = e->attr.crt_time.min;
    _tm.tm_sec  = e->attr.crt_time.sec;
    incr_date_time(&_tm, e->type ? 60 : 0);

    if (e->type) {
        bytes = snprintf(buf, buf_len, VF_LIST_VIDEO_FORMAT,
                         1, MEDIA_VIDEO_DIR, e->name,
                         e->attr.crt_time.year, e->attr.crt_time.month, e->attr.crt_time.day,
                         e->attr.crt_time.hour, e->attr.crt_time.min, e->attr.crt_time.sec,
                         60, 480, 640, 20, (int)e->fsize, 0, (long)mktime(&_tm));
    } else {
        bytes = snprintf(buf, buf_len, VF_LIST_PHOTO_FORMAT,
                         MEDIA_PHOTO_DIR, e->name,
                         e->attr.crt_time.year, e->attr.crt_time.month, e->attr.crt_time.day,
                         e->attr.crt_time.hour, e->attr.crt_time.min, e->attr.crt_time.sec,
                         480, 640, (int)e->fsize, 0, (long)mktime(&_tm));
    }
    if (bytes < 0 || bytes >= buf_len) {
        return -1;
    }
    return bytes;
}

/* 流式单扫目录: 命中哈希置 seen; 未命中填批缓冲, 满批 bulk 落库。
 * AVI 先验 RIFF(4B), 非 RIFF 且正在重建时记入 repair_paths。
 * 空/截断文件计数经 bad_cnt/empty_cnt 汇总, 不入库也不置 seen(残留 key 由调用方删除)。
 * 恒返回 0: 目录不存在或无匹配文件均视作该目录 0 个文件 */
static int flm_scan_diff_dir(bp_db_t *db, struct flm_scan_idx *idx,
                             const char *path, const char *extension, u8 type,
                             int *added, int *scanned,
                             char repair_paths[][160], int *repair_count,
                             int *bad_cnt, int *empty_cnt)
{
    char file_name[128] = {0};
    char media_name[128] = {0};
    char options[32] = {0};
    flm_scan_file_t entry;
    static bp_key_t bkeys[FLM_SCAN_BATCH];
    static bp_value_t bvals[FLM_SCAN_BATCH];
    static char bnames[FLM_SCAN_BATCH][64];
    static char bjsons[FLM_SCAN_BATCH][256];
    struct vfscan *scan_result;
    FILE *file_handle;
    int n = 0;
    int invalid_name_cnt = 0;
    int invalid_name_shown = 0;
    char invalid_name_first[64] = {0};

    snprintf(options, 32, "-t%s -sn", extension);
    if (fdir_exist(path) != 0) {
        return 0;
    }
    scan_result = fscan(path, options, 9);
    if (scan_result == NULL) {
        printf("Failed to scan path %s\n", path);
        return 0;
    }
    file_handle = fselect(scan_result, FSEL_FIRST_FILE, 0);
    while (file_handle != NULL) {
        int name_len = fget_name(file_handle, (u8 *)file_name, sizeof(file_name));
        struct vfs_attr attr = {0};
        int attrs_ok = (fget_attrs(file_handle, &attr) == 0);
        int has_riff = 1;
        int len;

        if (flm_normalize_name(file_name, name_len, media_name, sizeof(media_name)) < 0) {
            /* 名字无效分支: file_name 未必 NUL 结尾, 一律以 name_len 为上界打印 */
            int bad_len = (name_len > 0) ? name_len : 0;

            if (bad_len > (int)sizeof(file_name)) {
                bad_len = (int)sizeof(file_name);
            }
            if (invalid_name_cnt == 0) {
                snprintf(invalid_name_first, sizeof(invalid_name_first), "%.*s", bad_len, file_name);
            }
            if (invalid_name_shown < FLM_BAD_NAME_SHOW_MAX) {
                printf("Skip invalid media name: %s%.*s\n", path, bad_len, file_name);
                invalid_name_shown++;
            }
            invalid_name_cnt++;
        } else if (attrs_ok && flm_media_bad_file(file_handle, attr.fsize, type, empty_cnt)) {
            (*bad_cnt)++;
        } else {
            if (type == 1) {
                u8 hdr[4] = {0};

                fseek(file_handle, 0, SEEK_SET);
                has_riff = (fread(hdr, 1, 4, file_handle) == 4 && !memcmp(hdr, "RIFF", 4));
                /* 热拔卡未收尾 AVI: 记待修, 扫完统一 video_rec_repair_avi */
                if (!has_riff && flm_rebuild_busy && repair_count != NULL && *repair_count < 4) {
                    snprintf(repair_paths[*repair_count], 160, "%s%s", path, media_name);
                    (*repair_count)++;
                }
            }
            if (has_riff) {
                int hit;

                (*scanned)++;
                hit = flm_key_idx(idx, media_name);
                if (hit >= 0) {
                    idx->seen[hit] = 1;
                } else {
                    memset(&entry, 0, sizeof(entry));
                    strncpy(entry.name, media_name, 63);
                    entry.name[63] = '\0';
                    entry.type = type;
                    entry.attr = attr;
                    fget_attr(file_handle, &entry._attr);
                    entry.fsize = (u32)flen(file_handle);
                    strncpy(bnames[n], entry.name, 63);
                    bnames[n][63] = '\0';
                    len = flm_fill_json(&entry, bjsons[n], 256);
                    if (len > 0) {
                        bkeys[n].value = bnames[n];
                        bkeys[n].length = strlen(bnames[n]) + 1;
                        bvals[n].value = bjsons[n];
                        bvals[n].length = len;
                        n++;
                        (*added)++;
                        if (n == (int)FLM_SCAN_BATCH) {
                            const bp_key_t *pk = bkeys;
                            const bp_value_t *pv = bvals;

                            bp_bulk_set(db, n, &pk, &pv);
                            n = 0;
                        }
                    }
                }
            }
        }
        fclose(file_handle);
        file_handle = fselect(scan_result, FSEL_NEXT_FILE, 0);
    }
    if (n > 0) {
        const bp_key_t *pk = bkeys;
        const bp_value_t *pv = bvals;

        bp_bulk_set(db, n, &pk, &pv);
    }
    fscan_release(scan_result);

    if (invalid_name_cnt > 0) {
        printf("[FLM] invalid media name: total=%d (shown=%d, first=\"%s\")\n",
               invalid_name_cnt, invalid_name_shown, invalid_name_first);
    }

    return 0;
}

/* 流式增量重建: 单次目录扫描 + 名字哈希 diff; 内存只随 DB key 数增长。
 * 返回卡上文件总数(失败 -1 ⇒ 调用方退删库全扫) */
static int flm_rebuild_stream(bp_db_t *db, char repair_paths[][160], int *repair_count)
{
    struct flm_key_ctx *kctx;
    struct flm_scan_idx idx;
    int i;
    int key_cnt = 0;
    int added = 0, removed = 0, scanned = 0;
    int bad_cnt = 0, empty_cnt = 0;
    u32 t0 = sys_timer_get_ms();

    //先数 DB 键数再按需分配名字表, 不再按 1024 满额预占 64KB
    bp_get_ranges(db, "/", "z", accumulate, &key_cnt);
    kctx = malloc(sizeof(struct flm_key_ctx));
    if (kctx == NULL) {
        return -1;
    }
    memset(kctx, 0, sizeof(*kctx));
    kctx->cap = key_cnt;
    if (key_cnt > 0) {
        kctx->names = malloc((size_t)key_cnt * 64);
        if (kctx->names == NULL) {
            free(kctx);
            return -1;
        }
    }
    bp_get_ranges(db, "/", "z", flm_key_collect_cb, kctx);
    if (kctx->overflow || flm_scan_idx_build(&idx, kctx->names, kctx->count) != 0) {
        free(kctx->names);
        free(kctx);
        return -1;
    }
    if (flm_scan_diff_dir(db, &idx, MEDIA_VIDEO_DIR, "AVI", 1, &added, &scanned,
                          repair_paths, repair_count, &bad_cnt, &empty_cnt) != 0 ||
        flm_scan_diff_dir(db, &idx, MEDIA_PHOTO_DIR, "JPG", 0, &added, &scanned,
                          repair_paths, repair_count, &bad_cnt, &empty_cnt) != 0) {
        flm_scan_idx_free(&idx);
        free(kctx->names);
        free(kctx);
        return -1;
    }
    //DB 残留 key(卡上已无文件) → 删除, 索引错位自愈
    for (i = 0; i < idx.key_cnt; i++) {
        if (!idx.seen[i]) {
            bp_key_t k = { .value = idx.keys[i], .length = strlen(idx.keys[i]) + 1 };

            bp_remove(db, &k);
            removed++;
        }
    }
    //坏/空文件限流留痕: 每次重建合计一条, 无坏文件不打
    if (bad_cnt > 0) {
        printf("[FLM] skip empty media: total=%d (empty=%d)\n", bad_cnt, empty_cnt);
    }
    printf("[FLM] rebuild diff: scan=%d new=%d removed=%d ms=%u\n",
           scanned, added, removed, sys_timer_get_ms() - t0);
    flm_scan_idx_free(&idx);
    free(kctx->names);
    free(kctx);
    return scanned;
}

static int flm_do_scan_and_load(void)
{
    char repair_paths[4][160];
    int repair_count = 0;
    int file_count = -1;

    flm_mutex_ensure();
    if (g_flm.is_initialized) {
        return 0;
    }

    bp_db_t database_handle = {0};
    if (bp_open(&database_handle, MEDIA_DB_PATH) != BP_OK) {
        printf("Failed to open database %s\n", MEDIA_DB_PATH);
        g_flm.is_initialized = true;
        g_flm.file_count = 0;
        flm_names_cache_invalidate();
        return -1;
    }

    //stream: 流式增量重建(单扫目录+免解析+bulk), 失败退删库全扫老路径
    file_count = flm_rebuild_stream(&database_handle, repair_paths, &repair_count);

#if TCFG_APP_VIDEO_REC_EN
    //热拔卡收尾 AVI: 修复成功重新入库(数量少, 走老解析路径)
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
                if (add_avi_file_to_database(&database_handle, rfh, rname, &rattr, r_attr) == 0 &&
                    file_count >= 0) {
                    file_count++;
                }
                fclose(rfh);
            }
        }
    }
#endif

    if (file_count < 0) {
        //退化: 删库全扫(千级文件/DB key 超限场景, 保证正确性优先)
        u32 t1 = sys_timer_get_ms();
        FILE *db_old = fopen(MEDIA_DB_PATH, "r");
        if (db_old) {
            fdelete(db_old);
        }
        bp_close(&database_handle);
        if (bp_open(&database_handle, MEDIA_DB_PATH) != BP_OK) {
            g_flm.is_initialized = true;
            g_flm.file_count = 0;
            flm_names_cache_invalidate();
            return -1;
        }
        int bad_cnt = 0, empty_cnt = 0;

        file_count = 0;
        file_count += add_files_in_path_to_database(&database_handle, MEDIA_VIDEO_DIR, "AVI",
                      &bad_cnt, &empty_cnt);
        file_count += add_files_in_path_to_database(&database_handle, MEDIA_PHOTO_DIR, "JPG",
                      &bad_cnt, &empty_cnt);
        //坏/空文件限流留痕: 与流式路径同格式, 每次重建合计一条
        if (bad_cnt > 0) {
            printf("[FLM] skip empty media: total=%d (empty=%d)\n", bad_cnt, empty_cnt);
        }
        printf("[FLM] rebuild full: scan=%d ms=%u\n", file_count, sys_timer_get_ms() - t1);
    }

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
    /* 完成即通知: 录像停录入库/dec 插卡刷新由此驱动, 不忙等 */
    app_send_message(APP_MSG_FLM_DB_CHANGED, 0);
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

/* 过滤回调: 在库【加载 value 之前】执行, 只拿到 key。用它完成"按序号取名字":
 * 全程返回 0 => 一个 value 都不加载 => 省掉遍历里每 key 一次的 malloc/free
 * (实测那是 P_MEM 风暴的剩余来源: 1385 个 key => 1385 次分配)。
 * 名字/类型/路径的填充由调用方 flm_get_entry 按 name 完成, 这里只要 name。 */
static int flm_find_by_idx_filter(void *arg, const bp_key_t *key)
{
    struct flm_find_ctx *ctx = (struct flm_find_ctx *)arg;

    /* 顺序遍历顺手把目标前后各 N 条填进名字 LRU(零额外成本) */
    if (ctx->current + FLM_LRU_PREFETCH_N >= ctx->target &&
        ctx->current <= ctx->target + FLM_LRU_PREFETCH_N) {
        flm_name_lru_put(ctx->current, key->value);
    }
    if (ctx->current == ctx->target) {
        strncpy(ctx->out->name, key->value, sizeof(ctx->out->name) - 1);
        ctx->out->name[sizeof(ctx->out->name) - 1] = '\0';
    }
    ctx->current++;
    return 0;
}

/* 范围回调: filter 恒返回 0, 本回调不会被调用; 仅为满足 API 形参 */
static int flm_find_noop_cb(void *arg, const bp_key_t *key, const bp_value_t *value)
{
    (void)arg;
    (void)key;
    (void)value;
    return 0;
}

// 慢路径自身也要内存, 池子紧时直接失败, 不与回放管线并发撞车
#define FLM_SLOW_MIN_PMM   (96u * 1024u)

static int flm_get_entry_slow(int index, flm_file_t *out)
{
    bp_db_t db;
    struct flm_find_ctx ctx;

    if (index < 0 || index >= g_flm.file_count || !out) {
        return -1;
    }

    if (memory_get_size(P_MEMORY_UNUSED) < FLM_SLOW_MIN_PMM) {
        printf("[FLM] slow skipped: pmm low\n");
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
    bp_get_filtered_ranges(&db, "/", "z", flm_find_by_idx_filter, flm_find_noop_cb, &ctx);
    bp_close(&db);
    os_mutex_post(&g_flm.database_mutex);

    if (out->name[0] == '\0') {
        return -1;
    }
    return 0;
}

int flm_get_entry(int index, flm_file_t *out)
{
    const char *nm;

    if (index < 0 || index >= g_flm.file_count || !out) {
        return -1;
    }
    memset(out, 0, sizeof(*out));

    os_mutex_pend(&g_flm.database_mutex, 0);
    /* 全库缓存优先: 回放模式内由 flm_names_cache_prepare 建好, 命中则零遍历、零 LRU 访问 */
    if (g_flm.names != NULL && g_flm.names_count == g_flm.file_count && index < g_flm.names_count) {
        snprintf(out->name, sizeof(out->name), "%s", g_flm.names[index]);
        os_mutex_post(&g_flm.database_mutex);
        const char *cext = strrchr(out->name, '.');
        if (cext && (!memcmp(cext + 1, "JPG", 3) || !memcmp(cext + 1, "jpg", 3))) {
            out->type = 0;
            snprintf(out->path, sizeof(out->path), "%s%s", MEDIA_PHOTO_DIR, out->name);
        } else {
            out->type = 1;
            snprintf(out->path, sizeof(out->path), "%s%s", MEDIA_VIDEO_DIR, out->name);
        }
        out->thumbnail = NULL;
        return 0;
    }
    nm = flm_name_lru_get(index);
    if (nm != NULL) {
        snprintf(out->name, sizeof(out->name), "%s", nm);
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
    // 未命中走慢路径(自己加锁, 已填 name/type/path/thumbnail), 拿到名字后回填 LRU
    if (flm_get_entry_slow(index, out) != 0) {
        return -1;
    }
    os_mutex_pend(&g_flm.database_mutex, 0);
    flm_name_lru_put(index, out->name);
    os_mutex_post(&g_flm.database_mutex);
    return 0;
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

    // 起播路径 flm_get_entry 取同一把 database_mutex; 此处持锁做无人读的全库遍历会把它挡在锁外约 27s
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


