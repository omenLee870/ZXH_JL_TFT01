#include "app_config.h"
//*********MY INCLUDE*********//
#include "amap_canvas.h"
#if LV_USE_DEMO_AMAP
#if AMAP_USE_CANVAS
#include "list.h"
#include "awk.h"
#include "fs/fs.h"
#include "generic/printf.h"
#include "awk_map.h"
#include "awk_defines.h"
#include "awk_map_defines.h"
#include "string.h"
#include "os/os_api.h"
#include "asm/psram_api.h"
// ==================== 可调配置（集中在此，方便开关） ====================

// ---- 屏幕尺寸 ----
#define LCD_W 1024//840  // 可选 1024
#define LCD_H 600//480   // 可选 600

// ---- 在线激活 license：1=联网激活 / 0=关闭 ----
#define GET_LICENSE 0
#if GET_LICENSE
#include "http_cli.h"
#include "wifi/wifi_connect.h"
#endif

// ---- 路名/POI 文字字体 ----
// 1 = 用 SD 卡 ttf（freetype，完整字库）；0 = 内置 CJK 字体（仅 1338 常用字）
#define AMAP_USE_FREETYPE_FONT   1
#define AMAP_FREETYPE_FONT_PATH  "storage/sd1/C/font.ttf"
#define AMAP_FREETYPE_FONT_SIZE  16
#define AMAP_FT_FONT_CACHE_MAX   8   // freetype 字体缓存条目数（仅 AMAP_USE_FREETYPE_FONT=1 生效）

// ---- 调试打印：1=打印 / 0=关闭（由 AMAP_OPEN_INFO 控制） ----
#if AMAP_OPEN_INFO
#define OPEN_INFO 1
#define INFO_PRINTF(...)    printf(__VA_ARGS__)
#else
#define OPEN_INFO 0
#define INFO_PRINTF(...)
#endif

// ==================== SVG 瓦片光栅化缓存（加速拖动） ====================
// 拖动/缩放地图时，SDK 每帧都会把可见瓦片的 SVG 重新解析(nsvgParse)+光栅化(nsvgRasterize)，
// 这是地图卡顿的主要瓶颈。开启缓存后，把每张瓦片「整块」的 RGB565 光栅结果保存下来，
// 之后再次出现同一张瓦片（拖动时只是换了屏幕坐标）就直接把可见区域拷出来上屏，
// 完全跳过重复的解析+光栅化。
#define AMAP_TILE_CACHE_EN    0     // 1=开启瓦片缓存 / 0=关闭（关闭则回退到原来每帧全解析）
#define AMAP_TILE_CACHE_SLOT  8     // 瓦片缓存槽位数（每槽缓存一张整瓦片 RGB565，按需分配 PSRAM）
// 内存占用 ≈ 槽位数 × 瓦片像素数 × 2 字节：256x256 瓦片 ≈ 128KB/槽
// 请按剩余 PSRAM 调节（建议 4~16，至少 ≥1）

// ==================== 解析/光栅化耗时打印 ====================
// 1=开启：直接用 printf 打印每帧瓦片的解析/光栅化累计耗时（不受 AMAP_OPEN_INFO 限制，方便单独测速）
// 0=关闭：无任何额外打印
#define AMAP_TIME_PROFILE     1
// ====================================================================

// ---- 对象池大小（每帧最大瓦片/POI/文字数量） ----
#define AMAP_MAX_TEXT_LABEL 128
#define AMAP_POI_MAX 128
#define AMAP_SVG_TILE_MAX 128

// ---- 文件读取中转 buffer 大小（SD DMA 先读到内部 SRAM 中转，再 memcpy 到 PSRAM） ----
#define AMAP_READ_STAGE_SIZE 512

// ---- SVG 瓦片尺寸（svg_op 模式） ----
#define SVG_TILE_W 256
#define SVG_TILE_H 256

// SVG 复杂度阈值：光栅化前统计所有 shape/path 的总点数。nanosvg 内部用标准
// malloc/realloc 维护点/边池（NSVGpoint 32B、NSVGedge 24B），复杂瓦片会让池无限膨胀，
// internal SRAM 耗尽后 realloc 返回 NULL，nanosvg 的错误处理会把 NULL 当有效地址继续写
// （nsvg__alloc 里 &curpage->mem[...] 落在地址 ~4），破坏另一个核的 OS 数据，即双核连锁
// 崩溃。超过阈值直接跳过该瓦片，从源头封死内存膨胀。数值可按实际 internal SRAM 调。
#define SVG_MAX_TOTAL_PTS 5000
//****************************//
#if CONFIG_LV9_ENABLE

#include "lvgl.h"

// 在包含nanosvg.h之前，先包含这些：
#include <string.h> // 字符串操作函数
#include <math.h> // 数学函数
#include <stdlib.h> // malloc/realloc/free/qsort/strtod，必须在下面重定向前包含，避免宏污染其声明
//包含nanoSVG解析器和栅格化器
// 【修复】nanosvg 内部用标准 malloc/realloc/free 维护点/边/页池，标准堆是本平台紧张的
// internal SRAM（~1MB，与 OS 栈/任务共用）。复杂瓦片光栅化时池子翻倍扩容会耗尽 SRAM，
// realloc 返回 NULL 后 nanosvg 把 NULL 当有效地址继续写，破坏另一核 OS 数据（双核连锁
// 崩溃：bypass=1 / OPEN_INFO 下高频光栅化就崩，正是这个）。这里在包含前把 nanosvg 的
// malloc/realloc/free 重定向到 PSRAM 版本（8MB），从根上消除 SRAM 耗尽。
// realloc_psram 由 SDK 提供（LVGL 的 lv_mem_core_clib.c 亦在用，可靠）。
#if TCFG_PSRAM_DEV_ENABLE
extern void *malloc_psram(size_t size);
extern void *realloc_psram(void *pv, size_t size);
extern void free_psram(void *ptr);
#define malloc   malloc_psram
#define realloc  realloc_psram
#define free     free_psram
#endif
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#if TCFG_PSRAM_DEV_ENABLE
#undef malloc
#undef realloc
#undef free
#endif
extern void psram_heap_stats(void);
/**
 * @brief 文件扫描信息
 */
// 全局变量保存 mapId
static uint32_t mapId = 0;
#if AMAP_TIME_PROFILE
// 本帧解析/光栅化耗时统计（由 awk_render_commit_drawing_adapter 汇总打印）
static uint32_t g_prof_tile_n = 0;     // 本帧进入解析流程的瓦片数
static uint32_t g_prof_tile_hit = 0;   // 本帧缓存命中(免解析)的瓦片数
static uint32_t g_prof_parse_ms = 0;   // 本帧累计 nsvgParse 耗时(ms)
static uint32_t g_prof_raster_ms = 0;  // 本帧累计 nsvgRasterize 耗时(ms)
#endif
static awk_context_t context;
static int i2590 = 1;
static bool first = 1;
static int disp_hor = 1024;//840;
static int disp_ver = 600;//480;
static int map_hor = 1024;//840;
static int map_ver = 600;//480;

// 网络请求追踪结构（仅在线激活 GET_LICENSE 时使用）
#if GET_LICENSE
typedef struct _network_request_t {
    uint64_t id;                                    // 请求唯一ID
    httpcli_ctx *http_ctx;                          // HTTP客户端上下文
    awk_http_response_callback_t *callback;         // 高德SDK回调接口
    awk_http_response_t response;                   // 响应数据结构
    awk_http_buffer_t resp_body_buf;                // 响应body缓冲区
    uint8_t *resp_body_data;                        // 响应body数据
    int thread_pid;                                 // 工作线程PID
    volatile bool cancelled;                        // 取消标记
    struct list_head list;                          // 链表节点
} network_request_t;
static LIST_HEAD(g_network_list);     // 活跃请求链表
static uint64_t g_next_request_id = 1; // 请求ID自增计数器
#endif // GET_LICENSE

// ==================== PSRAM 内存适配 ====================
// 本平台 SRAM 紧张，地图相关大块内存放 PSRAM（参考 map/graph_tile.c）。
// malloc_psram/realloc_psram/free_psram 由系统提供（interface/system/malloc.h），
// 无 calloc_psram，calloc 用 malloc_psram + memset 实现。
#if TCFG_PSRAM_DEV_ENABLE
extern void *malloc_psram(size_t size);
extern void *realloc_psram(void *pv, size_t size);
extern void free_psram(void *ptr);
#define AMAP_MALLOC(sz)     malloc_psram(sz)
#define AMAP_REALLOC(p, sz) realloc_psram(p, sz)
#define AMAP_FREE(p)        free_psram(p)
#else
#define AMAP_MALLOC(sz)     malloc(sz)
#define AMAP_REALLOC(p, sz) realloc(p, sz)
#define AMAP_FREE(p)        free(p)
#endif

extern uint32_t get_system_ms(void);  // 系统毫秒计时（perf_counter）

static void awk_mem_free_adapter(void *ptr)
{
    AMAP_FREE(ptr);
    ptr = NULL;
}
static void *awk_mem_malloc_adapter(size_t size)
{
    return AMAP_MALLOC(size);
}
static void *awk_mem_calloc_adapter(size_t count, size_t size)
{
    // 系统无 calloc_psram，用 malloc_psram + memset 实现
    void *ptr = AMAP_MALLOC(count * size);
    if (ptr) {
        memset(ptr, 0, count * size);
    }
    return ptr;
}
static void *awk_mem_realloc_adapter(void *ptr, size_t size)
{
    // 系统提供 realloc_psram，直接透传
    return AMAP_REALLOC(ptr, size);
}
static void *awk_file_open_adapter(const char *filename, const char *mode)
{
    INFO_PRINTF("openfile filename=[%s], mode=[%s]\n", filename, mode);
    char mode1[3];
    mode1[0] = mode[0];
    if (mode[1] == 'b') {
        mode1[1] = '\0';
    } else {
        mode1[1] = mode[1];
    }
    INFO_PRINTF("turn mode=[%s]\n", mode1);
    FILE *file = fopen(filename, mode1);

    if (file == NULL) {
        INFO_PRINTF("openfile fail!\n");
    } else {
        INFO_PRINTF("openfile suss!\n");
    }

    return (void *)file;
}
static int awk_file_close_adapter(void *handler)
{
    FILE *file = (FILE *)handler;
    INFO_PRINTF("closefile start\n");
    return fclose(file);
}
static int awk_file_seek_adapter(void *handler, long offset, int where)
{
    INFO_PRINTF("seek start\n");
    FILE *file = (FILE *)handler;
    return fseek(file, offset, where);//有改动
}
static int awk_file_flush_adapter(void *handler)
{
    INFO_PRINTF("flush start\n");
    FILE *file = (FILE *)handler;
    return 0;  // 本平台 VFS 无 fflush，写入即时落盘，无需 flush
}
static size_t awk_file_read_adapter(void *ptr, size_t size, void *handler)
{
    // SD DMA 直接写 PSRAM 会绕过 CPU cache 导致数据错乱，因此先 fread 到内部 SRAM
    // 的中转 buffer（static，默认落在内部 SRAM 的 .bss），再 memcpy 到目标 PSRAM。
    static uint32_t stage[AMAP_READ_STAGE_SIZE / 4] __attribute__((aligned(32)));
    uint8_t *s = (uint8_t *)stage;
    uint8_t *d = (uint8_t *)ptr;
    size_t left = size;
    size_t total = 0;
    while (left > 0) {
        size_t n = left < AMAP_READ_STAGE_SIZE ? left : AMAP_READ_STAGE_SIZE;
        size_t r = fread(s, 1, n, (FILE *)handler);
        if (r == 0) {
            break;
        }
        memcpy(d, s, r);
        d += r;
        left -= r;
        total += r;
    }
    return total;
}
static size_t awk_file_write_adapter(void *ptr, size_t size, void *handler)
{
#if OPEN_INFO
    char *name = (char *)AMAP_MALLOC(256);
    fget_name((FILE *)handler, (u8 *)name, 256);
    INFO_PRINTF("write file %s start\n", name);
    AMAP_FREE(name);
    name = NULL;
#endif
    return fwrite(ptr, sizeof(char), size, (FILE *)handler);
}
static bool awk_file_exists_adapter(const char *path)
{
    INFO_PRINTF("ask file is exists start\n");
    INFO_PRINTF("path = [%s]\n", path);

    FILE *file = fopen(path, "r");

    if (file) {
        INFO_PRINTF("fopen success: [%s]\n", path);
        fclose(file);
        return true;
    }

    INFO_PRINTF("fopen FAILED: [%s]\n", path);
    return false;
}
static bool awk_file_dir_exists_adapter(const char *path)
{
    if (!path || !path[0]) {
        return false;
    }
    // 判断是否目录：唯一可靠依据是 FAT 属性位 F_ATTR_DIR(0x10)。
    // fdir_exist / fopen_dir_info 对普通文件也会返回「存在/句柄」，无法区分文件和目录，
    // 会把 14.pbf 这类文件误判成目录，导致 SDK 反复往下扫（无限递归）。
    // 所以这里不管用什么方式拿到句柄，都必须再用 fget_attr 读属性、测 F_ATTR_DIR 位。
    FILE *fp = fopen(path, "r");
    if (!fp) {
        // fopen 打不开目录(目录不能按普通文件打开)：退回 fopen_dir_info 拿句柄再查属性。
        fp = NULL;
        fopen_dir_info(path, &fp, 0);
        if (!fp) {
            INFO_PRINTF("%s -> not openable, not a dir\n", path);
            return false;
        }
    }

    int attr = 0;
    int ret = fget_attr(fp, &attr);
    fclose(fp);

    bool is_dir = (ret == 0) && ((attr & F_ATTR_DIR) != 0);
    INFO_PRINTF("%s -> attr=0x%02x %s\n", path, attr, is_dir ? "is dir" : "not a dir");
    return is_dir;
}
static int awk_file_remove_adapter(const char *path)
{
    INFO_PRINTF("rmfile start\n");
    return fdelete_by_name(path);
}
static int awk_file_mkdir_adapter(const char *path, uint16_t model)
{
    char *pathtemp = (char *)AMAP_MALLOC(strlen(path) + 5);
    strcpy(pathtemp, path);
    if (pathtemp[strlen(pathtemp) - 1] != '/') {
        strcat(pathtemp, "/");
    }
    FILE *ret = fopen(pathtemp, "w+");
#if OPEN_INFO
    if (ret != NULL) {
        INFO_PRINTF("mkdir %s suss!\n", pathtemp);
    } else {
        INFO_PRINTF("mkdir %s fail!", pathtemp);
    }
#endif
    AMAP_FREE(pathtemp);
    pathtemp = NULL;
    if (ret) {
        fclose(ret);
    }
    return ret ? 0 : -1;
}
static int awk_file_rmdir_adapter(const char *path)
{
    char *pathtemp = (char *)AMAP_MALLOC(strlen(path) + 5);
    strcpy(pathtemp, path);
    if (pathtemp[strlen(pathtemp) - 1] != '/') {
        strcat(pathtemp, "/");
    }
    int ret = -1; // 本平台 fs 无 fdelete_dir，删除目录未实现（离线地图不调用 rmdir）
#if OPEN_INFO
    if (ret == 0) {
        INFO_PRINTF("rmdir %s suss!\n", pathtemp);
    } else {
        INFO_PRINTF("rmdir %s fail!\n", pathtemp);
    }
#endif
    AMAP_FREE(pathtemp);
    pathtemp = NULL;
    return ret;
}
static void *awk_file_opendir_adapter(const char *path)
{
    if (!path) {
        return NULL;  // 路径为空
    }
    INFO_PRINTF("opendir start\n");
    char *pathtemp = (char *)AMAP_MALLOC(strlen(path) + 5);
    strcpy(pathtemp, path);
    // 使用 vfscan 打开目录
    if (pathtemp[strlen(pathtemp) - 1] != '/') {
        strcat(pathtemp, "/");
    }
    struct vfscan *scan = fscan(pathtemp, "-d -tALL -sn", 100); // 100 是最大目录+文件项数，可根据需要调整
    if (!scan) {
        INFO_PRINTF("fscan failed for path: %s\n", pathtemp);
        AMAP_FREE(pathtemp);
        return NULL;
    }
#if OPEN_INFO
    INFO_PRINTF("fscan suss for path: %s\n", pathtemp);
#endif
    AMAP_FREE(pathtemp);
    pathtemp = NULL;
    return (void *)scan;  // 返回结构体句柄，供 closedir 和 readdir 使用
}
static int awk_file_closedir_adapter(void *dir)
{
    if (!dir) {
        return -1;  // 参数错误或目录已经关闭
    }

    fscan_release((struct vfscan *)dir);  // 释放目录扫描结构体资源
    INFO_PRINTF("closedir start\n");
    return 0;  // 成功关闭目录
}
static bool awk_file_readdir_adapter(void *dir, awk_readdir_result *result)
{
    if (!dir || !result) {//如果dir和result为空直接返回
#if OPEN_INFO
        INFO_PRINTF("Invalid input to readdir_adapter\n");
#endif
        return 0;
    }
    struct vfscan *scan = (struct vfscan *)dir;
    int count = scan->file_number;//文件总数

    result->nodes = (awk_file_node *)AMAP_MALLOC(count * sizeof(awk_file_node));
    if (result->nodes) {
        memset(result->nodes, 0, count * sizeof(awk_file_node));
    }
    if (!result->nodes) {
        INFO_PRINTF("Failed to allocate memory for readdir result\n");
        result->size = 0;
        return 0;
    }
    result->size = count;
    INFO_PRINTF("有%d个文件\n", count);
    FILE *file = fselect(scan, 0, 0);
    for (int i = 0; i < count && file; i++) {
        // 获取文件名
        fget_name(file, (u8 *)result->nodes[i].file_name, sizeof(result->nodes[i].file_name));
        INFO_PRINTF("文件%d：%s\n", i + 1, result->nodes[i].file_name);
        INFO_PRINTF("文件大小：%d\n", flen(file));
        // 获取文件大小（可选，如果你不需要可以不写）
        result->nodes[i].file_size = flen(file);
        // 关闭当前项
        fclose(file);
        // 选择下一个
        file = fselect(scan, 2, 0);
    }
    return 1;
}
static size_t awk_file_get_size_adapter(const char *path)
{
    INFO_PRINTF("getsize start\n");

    // BR33 VFS：以 "rb" 打开时 flen() 会返回错误值（参考 simple_avi_unpkg.c 注释
    // "文件系统BUG，读取方式是rb时候flen = 32768"），导致 SDK 把几 MB 的瓦片文件
    // 读成 0/32768 字节、解码失败。统一用 "r" 打开 + flen 取大小。
    FILE *fp = fopen(path, "r");
    if (!fp) {
        INFO_PRINTF("getsize fopen failed\n");
        return 0;
    }

    size_t size = flen(fp);

    INFO_PRINTF("flen size = %u\n", (unsigned int)size);

    fclose(fp);

    return size;
}
static long awk_file_get_last_access_adapter(const char *path)
{
    INFO_PRINTF("last_access start\n");//待实现
    return 0;
}
static int awk_file_rename_adapter(const char *old_name, const char *new_name)
{
    INFO_PRINTF("rename start\n");
    FILE *temp = fopen(old_name, "w+");
    int ret = frename(temp, new_name);
    fclose(temp);
    return ret;
}
static bool awk_file_unzip_adapter(const char *zip_file, const char *out_dir)
{
    INFO_PRINTF("unzip start\n");//待实现
    return false;
}
static long awk_file_tell_adapter(void *handler)
{
    if (handler == NULL) {
        return -1;
    }

    // file_tell 语义 = 返回当前文件指针位置（对应 ftell）。取文件大小 SDK 走的是
    // awk_file_get_size_adapter（flen），不是 seek(SEEK_END)+tell。这里必须保持 ftell。
    return (long)ftell((FILE *)handler);
}
lv_obj_t *canvas = NULL;
uint16_t *canvas_buf = NULL;

// 文字标签（POI/路名）对象池：与瓦片 img_obj 类似的按帧复用机制
lv_obj_t *text_label[AMAP_MAX_TEXT_LABEL] = {0};
char *text_label_last_text[AMAP_MAX_TEXT_LABEL] = {0};  // 每个 label 上次文字内容，跳过重复 set_text
int text_label_count = 0;

// POI 图标对象池（矢量模式下 draw_bitmap 专用于画 POI 图标，独立于 SVG 瓦片的 img_obj 数组）
lv_obj_t *poi_img_obj[AMAP_POI_MAX] = {0};
lv_img_dsc_t *poi_img_dsc[AMAP_POI_MAX] = {0};
uint8_t *poi_temp_buffer[AMAP_POI_MAX] = {0};
int poi_buf_size[AMAP_POI_MAX] = {0};  // 每个槽位已分配 buffer 大小，用于复用
int poi_mapi = 0;

// SVG 瓦片对象池（HIDDEN 复用模式，独立于 POI 图标对象池）
lv_obj_t *svg_img_obj[AMAP_SVG_TILE_MAX] = {0};
lv_img_dsc_t *svg_img_dsc[AMAP_SVG_TILE_MAX] = {0};
uint8_t *svg_temp_buffer[AMAP_SVG_TILE_MAX] = {0};
int svg_temp_buffer_size[AMAP_SVG_TILE_MAX] = {0};
int svg_mapi = 0;

/* JLGPU 矢量模式下实际生效的 SVG 瓦片槽位（awk_render_svg_bitmap_adapter 使用）。
 * 槽位数必须与 AMAP_SVG_TILE_MAX 一致：视口 1024x600 / 瓦片 256x256 时可见瓦片
 * 约 20~30 块，拖动到某些偏移会超过原来的 25，越界写这些数组会直接破坏相邻的
 * PSRAM 全局变量（表现为拖动中随机崩溃，崩溃点每次都不一样）。 */
lv_obj_t *svg_jlgpu_img[AMAP_SVG_TILE_MAX] = {0};
uint8_t *svg_jlgpu_buffer[AMAP_SVG_TILE_MAX] = {0};
uint32_t svg_jlgpu_buffer_cap[AMAP_SVG_TILE_MAX] = {0};   /* 各槽位 buffer 已分配容量，"只增不减"用 */
lv_img_dsc_t svg_jlgpu_dsc[AMAP_SVG_TILE_MAX] = {0};

static void awk_render_begin_drawing_adapter(uint32_t map_id, awk_render_context_t status)
{
    text_label_count = 0;  // 每帧开始重置文字标签计数
    poi_mapi = 0;          // 每帧开始重置 POI 图标计数
    svg_mapi = 0;          // 每帧开始重置 SVG 瓦片计数
    if (first) {
        canvas_buf = AMAP_MALLOC(LV_CANVAS_BUF_SIZE(map_hor, map_ver, 16, 1));
        if (!canvas_buf) {
            // 申请失败，打印错误或返回
            return;
        }
        // 创建canvas对象，父对象为当前屏幕
        canvas = lv_canvas_create(lv_scr_act());
        lv_obj_remove_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
        for (uint32_t i = 0; i < map_hor * map_ver; i++) {
            canvas_buf[i] = 0xFFFF;
        }
        lv_canvas_set_buffer(canvas, canvas_buf, map_hor, map_ver, LV_COLOR_FORMAT_RGB565);
        lv_obj_set_pos(canvas, 0, 0);
        first = 0;
    } else {
        return;
    }
    INFO_PRINTF("render begin drawing start\n");
    return;//2590
}
static void awk_render_commit_drawing_adapter(uint32_t map_id)
{
    INFO_PRINTF("render commit drawing start\n");

    // 隐藏本帧没用到（上一帧残留）的文字标签（不删除，避免渲染回调里 lv_obj_del 引发 OS 锁异常/崩溃）
    for (int i = text_label_count; i < AMAP_MAX_TEXT_LABEL; i++) {
        if (text_label[i] != NULL) {
            lv_obj_add_flag(text_label[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // 隐藏本帧没用到（上一帧残留）的 POI 图标
    for (int i = poi_mapi; i < AMAP_POI_MAX; i++) {
        if (poi_img_obj[i] != NULL) {
            lv_obj_add_flag(poi_img_obj[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // 隐藏本帧没用到（上一帧残留）的 SVG 瓦片。
    // 注意：实际渲染用的是 svg_jlgpu_img 池（awk_render_svg_bitmap_adapter），
    // svg_img_obj 是未注册的 adapter1 用的池，恒为 NULL，这里必须查对数组。
    for (int i = svg_mapi; i < AMAP_SVG_TILE_MAX; i++) {
        if (svg_jlgpu_img[i] != NULL) {
            lv_obj_add_flag(svg_jlgpu_img[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    lv_disp_t *disp = lv_disp_get_default();
    if (disp == NULL) {
        INFO_PRINTF("no default display\n");
        return;
    }
    lv_refr_now(disp);

#if AMAP_TIME_PROFILE
    if (g_prof_tile_n || g_prof_tile_hit) {
        printf("[amap] render: parse_tile=%u, cache_hit=%u, parse=%u ms, raster=%u ms\n",
               g_prof_tile_n, g_prof_tile_hit, g_prof_parse_ms, g_prof_raster_ms);
        /* 清零，避免无瓦片的那一帧把上一帧的旧统计重复打印 */
        g_prof_tile_n = 0;
        g_prof_tile_hit = 0;
        g_prof_parse_ms = 0;
        g_prof_raster_ms = 0;
    }
#endif
}

// 根据 amap 给的字号选择字体：AMAP_USE_FREETYPE_FONT=1 时用 SD 卡 ttf（完整字库，按字号缓存），否则用内置 CJK 字体
#if AMAP_USE_FREETYPE_FONT
static struct {
    uint32_t size;
    lv_font_t *font;
} g_amap_ft_font_cache[AMAP_FT_FONT_CACHE_MAX] = {{0}};
#endif

static const lv_font_t *amap_get_text_font(const awk_paint_style_t *style)
{
#if AMAP_USE_FREETYPE_FONT
    uint32_t size = (style && style->text_style.font_size > 0) ? style->text_style.font_size : AMAP_FREETYPE_FONT_SIZE;

    // 单循环：命中已缓存字号就返回，同时记住第一个空位
    int free_slot = -1;
    for (int i = 0; i < AMAP_FT_FONT_CACHE_MAX; i++) {
        if (g_amap_ft_font_cache[i].font == NULL) {
            if (free_slot < 0) {
                free_slot = i;
            }
        } else if (g_amap_ft_font_cache[i].size == size) {
            return g_amap_ft_font_cache[i].font;
        }
    }

    // 未命中：有空位就按字号创建并缓存
    if (free_slot >= 0) {
        lv_font_t *font = lv_freetype_font_create(AMAP_FREETYPE_FONT_PATH,
                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                          size,
                          LV_FREETYPE_FONT_STYLE_NORMAL);
        if (font) {
            g_amap_ft_font_cache[free_slot].font = font;
            g_amap_ft_font_cache[free_slot].size = size;
            return font;
        }
    }

    // 缓存满或 ttf 加载失败时回退内置字体，避免传 NULL 导致崩溃
    return &lv_font_montserrat_16;
#else
    if (style && style->text_style.font_size <= 14) {
        return &lv_font_montserrat_14;
    }
    return &lv_font_montserrat_16;
#endif
}

static bool awk_render_measure_text_adapter(uint32_t map_id, const char *text, const awk_paint_style_t *style, int32_t *width, int32_t *ascender, int32_t *descender)
{
    (void)map_id;
    if (!text || !style) {
        return false;
    }
    const lv_font_t *font = amap_get_text_font(style);
    lv_point_t size;
    lv_text_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (width) {
        *width = size.x;
    }
    if (ascender) {
        *ascender = font->line_height;
    }
    if (descender) {
        *descender = 0;
    }
    return true;
}

static void awk_render_draw_text_adapter(uint32_t map_id, awk_point_t center, const char *text, const awk_paint_style_t *style)
{
    (void)map_id;
    if (!text || !style || text_label_count >= AMAP_MAX_TEXT_LABEL) {
        return;
    }

    // 复用/创建 label（用 HIDDEN 复用，不反复 create/delete，避免渲染回调里删对象引发 OS 锁异常）
    int label_slot = text_label_count;
    lv_obj_t *label = text_label[label_slot];
    if (label == NULL) {
        label = lv_label_create(lv_scr_act());
        lv_obj_remove_flag(label, LV_OBJ_FLAG_SCROLLABLE);
        text_label[label_slot] = label;
    }
    lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(label);  // 移到最前，避免被每帧重建的瓦片 img 覆盖
    text_label_count++;

    // 文字样式
    const lv_font_t *font = amap_get_text_font(style);
    uint32_t argb = style->color;
    uint8_t a = (argb >> 24) & 0xFF;
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8) & 0xFF;
    uint8_t b = argb & 0xFF;

    // 文字内容没变化时不重复 set_text（拖动时大量文字内容不变，省掉重复的文字重排/重绘）
    bool text_changed = (text_label_last_text[label_slot] == NULL ||
                         strcmp(text_label_last_text[label_slot], text) != 0);
    if (text_changed) {
        lv_label_set_text(label, text);
        char *tmp = (char *)AMAP_REALLOC(text_label_last_text[label_slot], strlen(text) + 1);
        if (tmp != NULL) {
            text_label_last_text[label_slot] = tmp;
            strcpy(tmp, text);
        }
    }
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_CLIP);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_make(r, g, b), LV_PART_MAIN);
    lv_obj_set_style_text_opa(label, a, LV_PART_MAIN);

    // 量文字尺寸，按 align 把中心点换算成左上角
    lv_point_t size;
    lv_text_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

    int32_t x = center.x;
    int32_t y = center.y - size.y / 2;
    switch (style->text_style.align) {
    case AWK_ALIGN_STYLE_LEFT:
        x = center.x;
        break;
    case AWK_ALIGN_STYLE_RIGHT:
        x = center.x - size.x;
        break;
    case AWK_ALIGN_STYLE_CENTER:
    default:
        x = center.x - size.x / 2;
        break;
    }

    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, size.x, size.y);
}
static void awk_render_point_adapter(uint32_t map_id, awk_point_t *point, uint32_t point_size, const awk_paint_style_t *style)
{
    // 因为还未用到，所以还未实现该功能（v9 中 lv_draw_rect 需要 layer 上下文，见 polyline 实现）
    (void)map_id;
    (void)point;
    (void)point_size;
    (void)style;
    INFO_PRINTF("render point start (not implemented)\n");
}
bool first_polyline = 1;
int linenum = 0;
static void awk_render_polyline_adapter(uint32_t map_id, awk_point_t *points, uint32_t point_size, const awk_paint_style_t *style)
{
    if (!first_polyline) {
        return;
    }
    linenum++;
    INFO_PRINTF("linenum = %d\n", linenum);
    if (!canvas || !style || point_size < 2) {
        return;
    }
    // 输出颜色信息
    uint32_t argb = style->color;
    uint8_t a = (argb >> 24) & 0xFF;
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8) & 0xFF;
    uint8_t b = argb & 0xFF;

    INFO_PRINTF("🎨 color = 0x%08X → (A=%d, R=%d, G=%d, B=%d), width = %d\n", argb, a, r, g, b, style->width);
    // 设置线条样式
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_make(r, g, b);
    line_dsc.opa = a;
    line_dsc.width = style->width;
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;

    // v9 中 lv_canvas_draw_line 已移除，canvas 画线需通过 layer + lv_draw_line
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    for (uint32_t i = 0; i + 1 < point_size; i++) {
        line_dsc.p1.x = points[i].x;
        line_dsc.p1.y = points[i].y;
        line_dsc.p2.x = points[i + 1].x;
        line_dsc.p2.y = points[i + 1].y;
        lv_draw_line(&layer, &line_dsc);
    }
    lv_canvas_finish_layer(canvas, &layer);
    INFO_PRINTF("✅ polyline drawn on canvas\n");
}
static void awk_render_polygon_adapter(uint32_t map_id, awk_point_t *points, uint32_t point_size, const awk_paint_style_t *style)
{
    // 目前没用到，所以此处还未写功能，要完善详情请见 polyline
    (void)map_id;
    (void)points;
    (void)point_size;
    (void)style;
    INFO_PRINTF("render polygon start (not implemented)\n");
}

static void awk_render_bitmap_adapter(uint32_t map_id, awk_rect_area_t area, awk_bitmap_t bitmap, const awk_paint_style_t *style)
{
    (void)map_id;
    if (poi_mapi >= AMAP_POI_MAX) {
        INFO_PRINTF("⚠️ POI 图标数量超过上限 %d，丢弃\n", AMAP_POI_MAX);
        return;
    }

    // 根据 bitmap.pixel_mode 确定 LVGL 颜色格式和每像素字节数（POI 图标通常是 RGBA8888 带 alpha）
    lv_color_format_t cf;
    int bpp;
    switch (bitmap.pixel_mode) {
    case AWK_PIXEL_MODE_RGB_565:
        cf = LV_COLOR_FORMAT_RGB565;
        bpp = 2;
        break;
    case AWK_PIXEL_MODE_RGB_888:
        cf = LV_COLOR_FORMAT_RGB888;
        bpp = 3;
        break;
    case AWK_PIXEL_MODE_ARGB_8888:   // [A,R,G,B]
    case AWK_PIXEL_MODE_RGBA_8888:   // [R,G,B,A]
    case AWK_PIXEL_MODE_BGRA_8888:   // [B,G,R,A] 与 LVGL ARGB8888 字节序一致
        cf = LV_COLOR_FORMAT_ARGB8888;   // LVGL 内存字节序 [B,G,R,A]
        bpp = 4;
        break;
    default:
        INFO_PRINTF("❌ 不支持的 bitmap pixel_mode=%d\n", bitmap.pixel_mode);
        return;
    }

    int slot = poi_mapi;
    poi_mapi++;

    // 复用像素 buffer（只有当前槽位容量不够时才重新分配）
    int buf_size = bitmap.width * bitmap.height * bpp;
    if (poi_buf_size[slot] < buf_size) {
        AMAP_FREE(poi_temp_buffer[slot]);
        poi_temp_buffer[slot] = AMAP_MALLOC(buf_size);
        poi_buf_size[slot] = buf_size;
    }
    uint8_t *dst = poi_temp_buffer[slot];
    if (!dst) {
        INFO_PRINTF("❌ POI buffer malloc fail\n");
        poi_buf_size[slot] = 0;
        return;
    }

    // RGBA8888([R,G,B,A]) / ARGB8888([A,R,G,B]) → LVGL ARGB8888([B,G,R,A])，交换 R/B 字节
    if (bitmap.pixel_mode == AWK_PIXEL_MODE_RGBA_8888 ||
        bitmap.pixel_mode == AWK_PIXEL_MODE_ARGB_8888) {
        for (uint32_t i = 0; i < bitmap.width * bitmap.height; i++) {
            const uint8_t *s = bitmap.buffer + i * 4;
            uint8_t *d = dst + i * 4;
            if (bitmap.pixel_mode == AWK_PIXEL_MODE_RGBA_8888) {
                d[0] = s[2];  // B
                d[1] = s[1];  // G
                d[2] = s[0];  // R
                d[3] = s[3];  // A
            } else {  // ARGB_8888
                d[0] = s[3];  // B
                d[1] = s[2];  // G
                d[2] = s[1];  // R
                d[3] = s[0];  // A
            }
        }
    } else {
        memcpy(dst, bitmap.buffer, buf_size);
    }

    // 复用 img_dsc 和 img 对象（HIDDEN 复用，不在渲染回调里 lv_obj_del 引发 OS 锁异常）
    if (poi_img_dsc[slot] == NULL) {
        poi_img_dsc[slot] = AMAP_MALLOC(sizeof(lv_img_dsc_t));
    }
    lv_img_dsc_t *dsc = poi_img_dsc[slot];
    if (!dsc) {
        return;
    }
    memset(dsc, 0, sizeof(lv_img_dsc_t));
    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->header.cf = cf;
    dsc->header.w = bitmap.width;
    dsc->header.h = bitmap.height;
    dsc->header.stride = bitmap.width * bpp;
    dsc->data_size = buf_size;
    dsc->data = dst;

    if (poi_img_obj[slot] == NULL) {
        poi_img_obj[slot] = lv_img_create(lv_scr_act());
        lv_obj_remove_flag(poi_img_obj[slot], LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_t *img = poi_img_obj[slot];
    lv_obj_remove_flag(img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(img);  // 移到最前，避免被每帧重建的瓦片 img 覆盖
    lv_img_set_src(img, dsc);
    lv_obj_set_pos(img, area.x, area.y);
    lv_obj_set_size(img, area.width, area.height);
    lv_img_set_angle(img, style->angle * 10);
}
float scale = 0;
NSVGrasterizer *rast;

// SVG 瓦片固定 256x256（svg_op 模式），中间缓冲静态复用，避免每帧 malloc/free 抖动。
static unsigned char *s_svg_rgba = NULL;  // 光栅化中间 RGBA 缓冲，按实际 raster_w*raster_h*4 动态分配
static int s_svg_rgba_cap = 0;            // rgba 缓冲已分配大小（字节）
static char *s_svg_xml = NULL;            // SVG 字符串缓冲（需以 '\0' 结尾供 nsvgParse）
static int s_svg_xml_cap = 0;             // xml 缓冲已分配大小

// 地图背景色（由 awk_render_color_adapter 填充）。SVG 瓦片的透明像素（alpha=0）
// 在 RGBA→RGB565 转换时若直接丢弃 alpha 会变黑，正是瓦片黑线根因。
// 这里把透明像素混合到地图背景色，黑线就消失了。
static uint8_t g_map_bg_r = 255, g_map_bg_g = 255, g_map_bg_b = 255;

// ==================== SVG 瓦片光栅结果缓存 ====================
#if AMAP_TILE_CACHE_EN
typedef struct {
    bool     valid;      // 槽位是否有效（rgb565 分配成功后才置 1）
    uint32_t key;        // SVG 内容 hash，作为瓦片标识
    uint32_t size;       // SVG buffer_size，配合 key 进一步防 hash 碰撞
    int      raster_w;   // 整张瓦片光栅宽(px)
    int      raster_h;   // 整张瓦片光栅高(px)
    int      cap;        // rgb565 已分配字节数（只增不减，按槽位复用）
    uint8_t *rgb565;     // 整张瓦片光栅化并转成 RGB565 的完整数据（不含裁剪）
    uint32_t seq;        // 最近使用序号（用于 LRU 淘汰）
} amap_svg_cache_t;
static amap_svg_cache_t s_svg_cache[AMAP_TILE_CACHE_SLOT];
static uint32_t s_svg_cache_seq = 0;

// FNV-1a：轻量内容 hash，用来标识“同一张瓦片”。同一地理瓦片在拖动时 SVG 内容不变。
static uint32_t amap_svg_content_hash(const char *buf, uint32_t len)
{
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < len; i++) {
        h ^= (uint8_t)buf[i];
        h *= 16777619u;
    }
    return h;
}

// 查找缓存：命中(内容 key + buffer_size + 光栅尺寸都一致)则更新 LRU 序号并返回
static amap_svg_cache_t *amap_svg_cache_find(uint32_t key, uint32_t size, int rw, int rh)
{
    for (int i = 0; i < AMAP_TILE_CACHE_SLOT; i++) {
        amap_svg_cache_t *e = &s_svg_cache[i];
        if (e->valid && e->key == key && e->size == size && e->raster_w == rw && e->raster_h == rh) {
            e->seq = ++s_svg_cache_seq;
            return e;
        }
    }
    return NULL;
}

// 取一个槽位用来写入新瓦片：优先空槽，没有则淘汰最久未用的(LRU)。
// 被淘汰槽位的 rgb565 大块 buffer 不 free（只增不减），由调用方按需扩容。
static amap_svg_cache_t *amap_svg_cache_get_free(uint32_t key, uint32_t size, int rw, int rh)
{
    amap_svg_cache_t *oldest = &s_svg_cache[0];
    for (int i = 0; i < AMAP_TILE_CACHE_SLOT; i++) {
        amap_svg_cache_t *e = &s_svg_cache[i];
        if (!e->valid) {
            e->valid = true;
            e->key = key;
            e->size = size;
            e->raster_w = rw;
            e->raster_h = rh;
            e->seq = ++s_svg_cache_seq;
            return e;
        }
        if (e->seq < oldest->seq) {
            oldest = e;
        }
    }
    oldest->key = key;
    oldest->size = size;
    oldest->raster_w = rw;
    oldest->raster_h = rh;
    oldest->seq = ++s_svg_cache_seq;
    return oldest;
}
#endif /* AMAP_TILE_CACHE_EN */

// 把 RGBA(整块瓦片) 中 [src_x,src_y,out_w,out_h) 区域转成 RGB565 写到 out（连续 out_w*out_h）
// 透明像素与地图背景色混合，逻辑与原内联转换完全一致。
static void amap_svg_rgba_to_rgb565(const uint8_t *rgba, int raster_w, int raster_h,
                                    int src_x, int src_y, int out_w, int out_h,
                                    uint8_t *out)
{
    uint16_t *d = (uint16_t *)out;
    for (int y = 0; y < out_h; y++) {
        for (int x = 0; x < out_w; x++) {
            int idx = (src_y + y) * raster_w + (src_x + x);
            uint32_t a = rgba[idx * 4 + 3];
            uint8_t r;
            uint8_t g;
            uint8_t b;

            if (a == 255) {
                r = rgba[idx * 4 + 0];
                g = rgba[idx * 4 + 1];
                b = rgba[idx * 4 + 2];
            } else if (a == 0) {
                r = g_map_bg_r;
                g = g_map_bg_g;
                b = g_map_bg_b;
            } else {
                r = (uint8_t)((rgba[idx * 4 + 0] * a + g_map_bg_r * (255u - a)) / 255u);
                g = (uint8_t)((rgba[idx * 4 + 1] * a + g_map_bg_g * (255u - a)) / 255u);
                b = (uint8_t)((rgba[idx * 4 + 2] * a + g_map_bg_b * (255u - a)) / 255u);
            }

            d[(size_t)y * out_w + x] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
        }
    }
}

// 从“整瓦片 RGB565”里把可见区域（连续 out_w*out_h）按行拷到 dst
static void amap_svg_copy_visible(const uint8_t *full, int raster_w, int raster_h,
                                  int src_x, int src_y, int out_w, int out_h,
                                  uint8_t *dst)
{
    for (int y = 0; y < out_h; y++) {
        memcpy(dst + (size_t)y * out_w * 2,
               full + ((size_t)(src_y + y) * raster_w + src_x) * 2,
               (size_t)out_w * 2);
    }
}

static void awk_render_svg_bitmap_adapter(uint32_t map_id, awk_rect_area_t area, awk_bitmap_t bitmap, const awk_paint_style_t *style)
{
    (void)map_id;
    (void)style;
    os_time_dly(1);//喂狗
    printf("svg_mapi:%d\n", svg_mapi);
    /* 每帧开头清理上一帧的 lv_image 对象（LVGL 堆上的小对象，直接删）。
     * svg_jlgpu_buffer[i] 不释放：按槽位只增不减，避免拖动时反复 free/malloc
     * 大块 PSRAM 导致碎片化（这正是之前内存一路下降的根因）。 */
    if (svg_mapi == 0) {
        for (int i = 0; i < AMAP_SVG_TILE_MAX; i++) {
            if (svg_jlgpu_img[i]) {
                lv_obj_del(svg_jlgpu_img[i]);
                svg_jlgpu_img[i] = NULL;
            }
        }
    }
    /* 兜底保护：超出槽位上限时丢弃该瓦片，绝不越界写数组。 */
    if (svg_mapi >= AMAP_SVG_TILE_MAX) {
        return;
    }
    /* draw_svg_bitmap 回调里的 bitmap.buffer 是 SVG XML 字符串，不是解码后的像素。
     * 参考 awk_render_svg_bitmap_adapter1 里就是直接拿它喂 nsvgParse 的。
     * 这里把它拷进持久 buffer（+1 补 '\0'），因为 SDK 可能在回调返回后复用/释放它。 */
    if (svg_jlgpu_buffer[svg_mapi] == NULL || svg_jlgpu_buffer_cap[svg_mapi] < bitmap.buffer_size + 1) {
        uint8_t *new_buf = malloc_psram(bitmap.buffer_size + 1);
        if (new_buf == NULL) {
            /* PSRAM 紧张时直接丢弃该瓦片：保留旧 buffer 会因容量不足被 memcpy 越界写。 */
            printf("svg tile: malloc_psram %u failed, drop tile\n", bitmap.buffer_size + 1);
            return;
        }
        if (svg_jlgpu_buffer[svg_mapi]) {
            free_psram(svg_jlgpu_buffer[svg_mapi]);
        }
        svg_jlgpu_buffer[svg_mapi] = new_buf;
        svg_jlgpu_buffer_cap[svg_mapi] = bitmap.buffer_size + 1;
    }
    memcpy(svg_jlgpu_buffer[svg_mapi], bitmap.buffer, bitmap.buffer_size);
    svg_jlgpu_buffer[svg_mapi][bitmap.buffer_size] = '\0';
    memset(&svg_jlgpu_dsc[svg_mapi], 0, sizeof(lv_img_dsc_t));
    svg_jlgpu_dsc[svg_mapi].header.magic = LV_IMAGE_HEADER_MAGIC;
    //svg_jlgpu_dsc[svg_mapi].header.cf = LV_COLOR_FORMAT_RGB8888; /* 会被 svg 解码器覆盖，占位 */
    svg_jlgpu_dsc[svg_mapi].header.w = area.width;
    svg_jlgpu_dsc[svg_mapi].header.h = area.height;
    svg_jlgpu_dsc[svg_mapi].data_size = bitmap.buffer_size;       /* XML 字节数，不是 w*h*2 */
    svg_jlgpu_dsc[svg_mapi].data = svg_jlgpu_buffer[svg_mapi];
    svg_jlgpu_img[svg_mapi] = lv_image_create(lv_scr_act());
    lv_image_set_src(svg_jlgpu_img[svg_mapi], &svg_jlgpu_dsc[svg_mapi]);
    lv_obj_set_pos(svg_jlgpu_img[svg_mapi], area.x, area.y);
    svg_mapi++;
}
static void awk_render_svg_bitmap_adapter1(uint32_t map_id, awk_rect_area_t area, awk_bitmap_t bitmap, const awk_paint_style_t *style)
{
    (void)map_id;
    (void)style;
    os_time_dly(1);//喂狗
    /* 每帧开头重建 LVGL 图片对象：直接复用 img 对象会让上一帧的旧图/旧尺寸/旧位置残留在
     * 屏幕上，拖动时出现花屏。lv_img_create/lv_obj_del 都是 LVGL 堆上的小对象，不占 PSRAM。
     * 大块 svg_temp_buffer 保持按槽位只增不减（不 free），消除拖动时的 PSRAM 碎片化——
     * 这才是之前内存一路下降的根因，跟对象复用无关。 */
    if (svg_mapi == 0) {
#if AMAP_TIME_PROFILE
        /* 新的一帧开始，清零本帧耗时统计（等 commit 时打印汇总） */
        g_prof_tile_n = 0;
        g_prof_tile_hit = 0;
        g_prof_parse_ms = 0;
        g_prof_raster_ms = 0;
#endif
        for (int i = 0; i < AMAP_SVG_TILE_MAX; i++) {
            if (svg_img_obj[i]) {
                lv_obj_del(svg_img_obj[i]);
                svg_img_obj[i] = NULL;
            }

            if (svg_img_dsc[i]) {
                AMAP_FREE(svg_img_dsc[i]);
                svg_img_dsc[i] = NULL;
            }

            /* svg_temp_buffer[i] 不释放：只增不减，避免拖动时反复 free/malloc 大块导致碎片化 */
        }
    }
    INFO_PRINTF("svg_mapi:%d\n", svg_mapi);
    INFO_PRINTF("svg_bitmap\n");
    INFO_PRINTF("📐 Render Area: x=%d, y=%d, width=%d, height=%d\n", area.x, area.y, area.width, area.height);
    INFO_PRINTF("🧱 buffer_size=%u\n", bitmap.buffer_size);

    int malloc_width = 0;
    int malloc_height = 0;
    int posx1 = 0;
    int posy1 = 0;

    /* 瓦片裁剪到地图窗口范围 */
    int clip_left = area.x < 0 ? -area.x : 0;
    int clip_top = area.y < 0 ? -area.y : 0;
    int clip_right = (area.x + area.width > map_hor) ? (area.x + area.width - map_hor) : 0;
    int clip_bottom = (area.y + area.height > map_ver) ? (area.y + area.height - map_ver) : 0;

    malloc_width = area.width - clip_left - clip_right;
    malloc_height = area.height - clip_top - clip_bottom;

    /* 可见区域左上角屏幕坐标 */
    posx1 = area.x + clip_left;
    posy1 = area.y + clip_top;

    if (malloc_width <= 0 || malloc_height <= 0) {
        return;
    }

    INFO_PRINTF("clip L%d T%d R%d B%d, posx1: %d, posy1: %d\n", clip_left, clip_top, clip_right, clip_bottom, posx1, posy1);
    INFO_PRINTF("malloc_width : %d , malloc_height : %d\n", malloc_width, malloc_height);

    if (first_polyline) {
        rast = nsvgCreateRasterizer();
        linenum = 0;
        first_polyline = 0;
    }

    int raster_w = area.width;
    int raster_h = area.height;
    int r_malloc_w = malloc_width;
    int r_malloc_h = malloc_height;

    int buf_size = r_malloc_w * r_malloc_h * 2;

    /* 当前瓦片显示 buffer */
    if (svg_temp_buffer[svg_mapi] == NULL || svg_temp_buffer_size[svg_mapi] < buf_size) {
        if (svg_temp_buffer[svg_mapi]) {
            AMAP_FREE(svg_temp_buffer[svg_mapi]);
        }

        svg_temp_buffer[svg_mapi] = AMAP_MALLOC(buf_size);
        svg_temp_buffer_size[svg_mapi] = buf_size;
    }

    uint8_t *dst = svg_temp_buffer[svg_mapi];

    if (!dst) {
        svg_temp_buffer_size[svg_mapi] = 0;
        INFO_PRINTF("❌ svg temp_buffer malloc fail\n");
        return;
    }

    int src_x = area.x < 0 ? -area.x : 0;
    int src_y = area.y < 0 ? -area.y : 0;

#if AMAP_TILE_CACHE_EN
    /* ---- 瓦片缓存查找：命中则跳过整段 nsvgParse + nsvgRasterize，只拷可见区域上屏 ---- */
    amap_svg_cache_t *cache_ent = NULL;
    uint32_t svg_key = 0;
    if (bitmap.buffer && bitmap.buffer_size > 0) {
        svg_key = amap_svg_content_hash((const char *)bitmap.buffer, bitmap.buffer_size);
        cache_ent = amap_svg_cache_find(svg_key, bitmap.buffer_size, raster_w, raster_h);
        if (cache_ent) {
#if AMAP_TIME_PROFILE
            g_prof_tile_hit++;
#endif
            amap_svg_copy_visible(cache_ent->rgb565, raster_w, raster_h,
                                  src_x, src_y, r_malloc_w, r_malloc_h, dst);
            goto svg_img_tail;
        }
    }
#endif /* AMAP_TILE_CACHE_EN */

#if AMAP_TIME_PROFILE
    g_prof_tile_n++;   // 走到这里 = 缓存未命中（或缓存关闭），本瓦片需要完整解析
#endif

    /* ---- 解析 + 光栅化（只对缓存未命中的瓦片执行） ---- */
    {
        /* 准备 SVG XML buffer */
        if (s_svg_xml_cap < (int)bitmap.buffer_size + 1) {
            char *n = (char *)AMAP_MALLOC(bitmap.buffer_size + 1);

            if (!n) {
                INFO_PRINTF("❌ xml buffer realloc fail\n");
                return;
            }

            if (s_svg_xml) {
                AMAP_FREE(s_svg_xml);
            }

            s_svg_xml = n;
            s_svg_xml_cap = bitmap.buffer_size + 1;
        }

        memcpy(s_svg_xml, bitmap.buffer, bitmap.buffer_size);
        s_svg_xml[bitmap.buffer_size] = '\0';

        uint32_t t_parse0 = get_system_ms();

        NSVGimage *image = nsvgParse(s_svg_xml, "px", 96.0f);

        uint32_t t_parse1 = get_system_ms();

#if AMAP_TIME_PROFILE
        g_prof_parse_ms += (uint32_t)(t_parse1 - t_parse0);
#endif
        INFO_PRINTF("svg parse time=%u ms\n", t_parse1 - t_parse0);

        if (!image) {
            INFO_PRINTF("❌ nsvgParse fail\n");
            return;
        }

        if (image->width <= 0.0f || image->height <= 0.0f) {
            INFO_PRINTF("❌ invalid svg size %.2f x %.2f\n", image->width, image->height);
            nsvgDelete(image);
            return;
        }

        /* 检查 SVG 几何数据是否异常 */
        int total_pts = 0;
        int bad_bounds = 0;

        for (NSVGshape *sh = image->shapes; sh; sh = sh->next) {
            if (!(sh->bounds[0] > -1e7f && sh->bounds[0] < 1e7f &&
                  sh->bounds[1] > -1e7f && sh->bounds[1] < 1e7f &&
                  sh->bounds[2] > -1e7f && sh->bounds[2] < 1e7f &&
                  sh->bounds[3] > -1e7f && sh->bounds[3] < 1e7f)) {
                bad_bounds = 1;
                break;
            }

            for (NSVGpath *ph = sh->paths; ph; ph = ph->next) {
                total_pts += ph->npts;
            }
        }

        if (bad_bounds || total_pts > SVG_MAX_TOTAL_PTS) {
            INFO_PRINTF("❌ SVG invalid: bad_bounds=%d pts=%d\n", bad_bounds, total_pts);
            nsvgDelete(image);
            return;
        }

        /* 根据 SVG 尺寸计算光栅化缩放比例 */
        float scale_w = (float)raster_w / image->width;
        float scale_h = (float)raster_h / image->height;

        scale = scale_w < scale_h ? scale_w : scale_h;

        if (!(scale > 0.0f) || !(scale < 1e6f)) {
            INFO_PRINTF("❌ invalid scale=%f\n", scale);
            nsvgDelete(image);
            return;
        }

        /* RGBA 临时 buffer */
        int rgba_size = raster_w * raster_h * 4;

        if (s_svg_rgba_cap < rgba_size) {
            if (s_svg_rgba) {
                AMAP_FREE(s_svg_rgba);
                s_svg_rgba = NULL;
            }

            s_svg_rgba = (unsigned char *)AMAP_MALLOC(rgba_size);

            if (!s_svg_rgba) {
                s_svg_rgba_cap = 0;
                INFO_PRINTF("❌ rgba malloc fail\n");
                nsvgDelete(image);
                return;
            }

            s_svg_rgba_cap = rgba_size;
        }

        unsigned char *rgba = s_svg_rgba;

        /* SVG 光栅化 */
#if AMAP_TIME_PROFILE
        uint32_t t_rast0 = get_system_ms();
        nsvgRasterize(rast, image, 0, 0, scale, rgba, raster_w, raster_h, raster_w * 4);
        uint32_t t_rast1 = get_system_ms();
        g_prof_raster_ms += (uint32_t)(t_rast1 - t_rast0);
#else
        nsvgRasterize(rast, image, 0, 0, scale, rgba, raster_w, raster_h, raster_w * 4);
#endif

        /* RGBA → RGB565 */
#if AMAP_TILE_CACHE_EN
        if (bitmap.buffer && bitmap.buffer_size > 0) {
            /* 缓存未命中：先把整张瓦片转成 RGB565 存进缓存槽(LRU)，再拷可见区域给本帧显示 buffer。
             * 之后同一瓦片再出现就 cache hit，直接省掉解析+光栅化。 */
            cache_ent = amap_svg_cache_get_free(svg_key, bitmap.buffer_size, raster_w, raster_h);
            int full_need = raster_w * raster_h * 2;
            if (cache_ent->cap < full_need) {
                uint8_t *nb = (uint8_t *)AMAP_MALLOC(full_need);
                if (nb) {
                    if (cache_ent->rgb565) {
                        AMAP_FREE(cache_ent->rgb565);
                    }
                    cache_ent->rgb565 = nb;
                    cache_ent->cap = full_need;
                }
            }
            if (cache_ent->rgb565 && cache_ent->cap >= full_need) {
                /* 缓存成功：整瓦片转 RGB565 存槽，再拷可见区域给本帧显示 buffer */
                amap_svg_rgba_to_rgb565(rgba, raster_w, raster_h, 0, 0, raster_w, raster_h, cache_ent->rgb565);
                amap_svg_copy_visible(cache_ent->rgb565, raster_w, raster_h,
                                      src_x, src_y, r_malloc_w, r_malloc_h, dst);
            } else {
                /* 缓存 buffer 不足/分配失败：退回不缓存路径（本瓦片仍正常显示） */
                cache_ent->valid = false;
                amap_svg_rgba_to_rgb565(rgba, raster_w, raster_h, src_x, src_y, r_malloc_w, r_malloc_h, dst);
            }
        } else
#endif
        {
            amap_svg_rgba_to_rgb565(rgba, raster_w, raster_h, src_x, src_y, r_malloc_w, r_malloc_h, dst);
        }

        nsvgDelete(image);
    }   /* ---- 解析+光栅化块结束 ---- */

#if AMAP_TILE_CACHE_EN
svg_img_tail:
#endif
    /* 创建 LVGL 图片描述符 */
    if (svg_img_dsc[svg_mapi] == NULL) {
        svg_img_dsc[svg_mapi] = AMAP_MALLOC(sizeof(lv_img_dsc_t));
    }

    if (!svg_img_dsc[svg_mapi]) {
        INFO_PRINTF("❌ lv_img_dsc malloc fail\n");
        return;
    }

    lv_img_dsc_t *dsc = svg_img_dsc[svg_mapi];

    memset(dsc, 0, sizeof(lv_img_dsc_t));

    dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    dsc->header.w = r_malloc_w;
    dsc->header.h = r_malloc_h;
    dsc->data_size = buf_size;
    dsc->data = dst;

    /* 创建 LVGL 图片对象 */
    if (svg_img_obj[svg_mapi] == NULL) {
        svg_img_obj[svg_mapi] = lv_img_create(lv_scr_act());
        lv_obj_remove_flag(svg_img_obj[svg_mapi], LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t *img = svg_img_obj[svg_mapi];

    lv_obj_remove_flag(img, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(img, dsc);
    lv_obj_set_pos(img, posx1, posy1);

    svg_mapi++;

    //psram_heap_stats();
}
static void awk_render_color_adapter(uint32_t map_id, awk_rect_area_t area, const awk_paint_style_t *style)
{
    if (first) {
        first = 0;
    } else {
        return;
    }
    if (!style) {
        return;
    }
    INFO_PRINTF("🖌️ render color start\n");

    // 提取颜色（ARGB）
    uint32_t argb = style->color;
    uint8_t a = (argb >> 24) & 0xFF;
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8) & 0xFF;
    uint8_t b = argb & 0xFF;

    // 生成 lv_color_t（假设使用 RGB565 显存）
    lv_color_t lv_color = lv_color_make(r, g, b);
    lv_canvas_fill_bg(canvas, lv_color, LV_OPA_COVER);
    // 记录地图背景色，供 SVG 瓦片透明像素混合使用（否则透明边变黑）
    g_map_bg_r = r;
    g_map_bg_g = g;
    g_map_bg_b = b;
    INFO_PRINTF("✅ blend_fill done: color R=%d G=%d B=%d\n", r, g, b);
}
static uint64_t awk_get_system_time_adapter(void)
{
    INFO_PRINTF("get system time start\n");
    uint32_t tick = get_system_ms();
    // 如果1 tick = 1ms，直接返回tick即可
    // 如果1 tick = x ms，需要乘以x

    return (uint64_t)tick; // 单位：毫秒
}
static int awk_printf_adapter(const char *__fmt, ...)
{
#if OPEN_INFO
    INFO_PRINTF("printf start\n");
    va_list args;
    va_start(args, __fmt);
    int ret = vprintf(__fmt, args);
    va_end(args);
    return ret;
#endif
    return 0;
}
#if GET_LICENSE
static int http_bridge_cb(void *ctx, void *buf, unsigned int size, void *priv, httpin_status status)
{
    network_request_t *req = (network_request_t *)priv;
    httpcli_ctx *http_ctx = (httpcli_ctx *)ctx;

    if (!req || !req->callback) {
        return 0;
    }

    switch (status) {
    case HTTPIN_HEADER: {
        req->response.status_code = 200;
        if (http_ctx && http_ctx->content_length > 0) {
            req->resp_body_data = (uint8_t *)malloc(http_ctx->content_length + 1);
            if (req->resp_body_data) {
                memset(req->resp_body_data, 0, http_ctx->content_length + 1);
                req->resp_body_buf.buffer = req->resp_body_data;
                req->resp_body_buf.length = 0;
                req->response.body = &req->resp_body_buf;
            }
        } else {
            req->response.body = NULL;
        }
        req->response.url = http_ctx ? (char *)http_ctx->url : NULL;
        if (req->callback->on_receive_header) {
            req->callback->on_receive_header(req->callback, &req->response);
        }
        break;
    }

    case HTTPIN_PROGRESS: {
        if (!buf || size == 0) {
            break;
        }

        // 动态分配 / 扩容缓冲区
        if (!req->resp_body_data) {
            req->resp_body_data = (uint8_t *)malloc(size + 1);
            if (!req->resp_body_data) {
                break;
            }
            memset(req->resp_body_data, 0, size + 1);
            req->resp_body_buf.buffer = req->resp_body_data;
            req->resp_body_buf.length = 0;
        }

        size_t new_len = req->resp_body_buf.length + size;
        if (http_ctx && http_ctx->content_length > 0) {
            if (new_len > http_ctx->content_length) {
                size = http_ctx->content_length - req->resp_body_buf.length;
                new_len = http_ctx->content_length;
            }
        } else {
            uint8_t *new_buf = (uint8_t *)realloc(req->resp_body_data, new_len + 1);
            if (!new_buf) {
                break;
            }
            req->resp_body_data = new_buf;
            req->resp_body_buf.buffer = req->resp_body_data;
        }

        memcpy(req->resp_body_data + req->resp_body_buf.length, buf, size);
        req->resp_body_buf.length += size;
        req->resp_body_data[req->resp_body_buf.length] = '\0';
        req->response.body = &req->resp_body_buf;

        // ⚠️ 不在这里调用 on_receive_body，等 FINISHED 时 dechunk 后再回调
        break;
    }

    case HTTPIN_FINISHED: {
        req->response.body = &req->resp_body_buf;

        // ---------- chunked 解包 ----------
        if (req->resp_body_data && req->resp_body_buf.length > 0) {
            char *src = (char *)req->resp_body_data;
            size_t src_len = req->resp_body_buf.length;

            // 检测是否是 chunked 编码（以 hex 开头）
            if (src_len > 0) {
                char c0 = src[0];
                int is_hex = (c0 >= '0' && c0 <= '9')
                             || (c0 >= 'a' && c0 <= 'f')
                             || (c0 >= 'A' && c0 <= 'F');
                if (is_hex) {
                    size_t si = 0, di = 0;
                    while (si < src_len) {
                        // 跳过 \r\n
                        while (si < src_len && (src[si] == '\r' || src[si] == '\n')) {
                            si++;
                        }
                        if (si >= src_len) {
                            break;
                        }
                        // 读 chunk size (hex)
                        size_t csize = 0;
                        while (si < src_len) {
                            char c = src[si];
                            if (c >= '0' && c <= '9') {
                                csize = (csize << 4) | (c - '0');
                            } else if (c >= 'a' && c <= 'f') {
                                csize = (csize << 4) | (c - 'a' + 10);
                            } else if (c >= 'A' && c <= 'F') {
                                csize = (csize << 4) | (c - 'A' + 10);
                            } else {
                                break;
                            }
                            si++;
                        }
                        if (csize == 0) {
                            break;
                        }
                        // 跳过 \r\n
                        while (si < src_len && (src[si] == '\r' || src[si] == '\n')) {
                            si++;
                        }
                        // 拷贝 chunk data
                        if (si + csize <= src_len) {
                            memmove(&src[di], &src[si], csize);
                            di += csize;
                            si += csize;
                        } else {
                            break;
                        }
                    }
                    req->resp_body_buf.length = di;
                    if (di < src_len) {
                        src[di] = '\0';
                    }
                }
            }
        }
        // ---------------------------------

        INFO_PRINTF("\n========== HTTP FINISHED ==========\n");

        if (req->response.body) {
            INFO_PRINTF("body length = %d\n", req->response.body->length);
            INFO_PRINTF("body content = %.*s\n", req->response.body->length, req->response.body->buffer);
        } else {
            INFO_PRINTF("response.body NULL\n");
        }

        INFO_PRINTF("===================================\n");

        // 先通知 SDK 收到 body（已 dechunk）
        if (req->callback->on_receive_body) {
            req->callback->on_receive_body(req->callback, &req->response);
        }
        // 再通知成功
        if (req->callback->on_success) {
            req->callback->on_success(req->callback, &req->response);
        }
        break;
    }

    case HTTPIN_ERROR:
        if (req->cancelled) {
            if (req->callback->on_canceled) {
                req->callback->on_canceled(req->callback, &req->response);
            }
        } else {
            if (req->callback->on_fail) {
                req->callback->on_fail(req->callback, &req->response, -1);
            }
        }
        break;

    case HTTPIN_ABORT:
        if (req->cancelled) {
            if (req->callback->on_canceled) {
                req->callback->on_canceled(req->callback, &req->response);
            }
        }
        break;

    default:
        break;
    }

    if (req->cancelled) {
        return -1;
    }
    return 0;
}

static void network_worker_thread(void *arg)
{
    network_request_t *req = (network_request_t *)arg;
    if (!req || !req->http_ctx) {
        return;
    }

    httpcli_ctx *ctx = req->http_ctx;
    httpin_error ret;

    INFO_PRINTF("[network] start request id=%llu url=%s\n", req->id, ctx->url ? ctx->url : "NULL");

    if (ctx->post_data && ctx->data_len > 0) {
        ret = httpcli_post(ctx);
    } else {
        ret = httpcli_get(ctx);
    }

    INFO_PRINTF("[network] request id=%llu finished, ret=%d\n", req->id, ret);

    // 如果 callbacks 还未被 bridge callback 调用（部分场景），补充处理
    if (ret != HERROR_OK && !req->cancelled) {
        if (req->callback && req->callback->on_fail) {
            req->callback->on_fail(req->callback, &req->response, ret);
        }
    }

    // 清理资源
    httpcli_close(ctx);
    if (ctx) {
        free(ctx);
        req->http_ctx = NULL;
    }
    if (req->resp_body_data) {
        free(req->resp_body_data);
        req->resp_body_data = NULL;
    }

    // 从链表中移除
    list_del(&req->list);
    free(req);
}
// 同步执行一次 HTTP 请求（在当前线程上），用于激活等必须在 init 线程执行的场景
static void network_do_request_sync(network_request_t *req)
{
    if (!req || !req->http_ctx) {
        return;
    }

    httpcli_ctx *ctx = req->http_ctx;
    httpin_error ret;

    INFO_PRINTF("[network] sync start id=%llu url=%s\n", req->id, ctx->url ? ctx->url : "NULL");

    if (ctx->post_data && ctx->data_len > 0) {
        ret = httpcli_post(ctx);
    } else {
        ret = httpcli_get(ctx);
    }

    INFO_PRINTF("[network] sync request id=%llu finished, ret=%d\n", req->id, ret);

    if (ret != HERROR_OK && !req->cancelled) {
        if (req->callback && req->callback->on_fail) {
            req->callback->on_fail(req->callback, &req->response, ret);
        }
    }

    // 清理
    httpcli_close(ctx);
    if (ctx) {
        free(ctx);
        req->http_ctx = NULL;
    }
    if (req->resp_body_data) {
        free(req->resp_body_data);
        req->resp_body_data = NULL;
    }
    list_del(&req->list);
    free(req);
}
#endif // GET_LICENSE
static uint64_t awk_aos_network_adapter_send_adapter(awk_http_request_t *request, awk_http_response_callback_t *callback)
{
#if GET_LICENSE
    INFO_PRINTF("network send start, url=%s\n", request->url ? request->url : "NULL");

    if (!request || !request->url || !callback) {
        INFO_PRINTF("network send: invalid param\n");
        return 0;
    }

    // 分配请求追踪结构
    network_request_t *req = (network_request_t *)calloc(1, sizeof(network_request_t));
    if (!req) {
        INFO_PRINTF("network send: alloc req failed\n");
        return 0;
    }

    // 分配 httpcli_ctx
    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        free(req);
        INFO_PRINTF("network send: alloc ctx failed\n");
        return 0;
    }

    // 初始化
    req->id = g_next_request_id++;
    req->http_ctx = ctx;
    req->callback = callback;
    req->cancelled = false;
    INIT_LIST_HEAD(&req->list);
    memset(&req->response, 0, sizeof(awk_http_response_t));
    req->response.request_id = req->id;
    req->response.url = request->url;

    // 设置 httpcli_ctx
    ctx->url = request->url;
    ctx->cb = http_bridge_cb;
    ctx->priv = req;
    ctx->connection = "close";
    ctx->timeout_millsec = 10000;  // 10秒超时

    // 设置请求方法
    if (request->method == AWK_HTTP_METHOD_POST) {
        ctx->data_format = "application/json";
        if (request->body && request->body->buffer && request->body->length > 0) {
            ctx->post_data = (const char *)request->body->buffer;
            ctx->data_len = (int)request->body->length;
        }
    }

    // 处理自定义 headers（通过 user_http_header 拼接）
    // 注：httpcli 的 user_http_header 最后必须以 \0 结尾，多个 header 用 \r\n 分隔
    // 此处简化处理，如有 headers 则忽略（高德 SDK 内部会自动拼接基础 headers）

    // 加入链表
    list_add_tail(&req->list, &g_network_list);

    // 在当前线程上同步执行 HTTP 请求（这样 SDK 的线程检查才能通过）
    network_do_request_sync(req);

    return req->id;
#else
    (void)request;
    (void)callback;
    return 0;
#endif
}

static void awk_aos_network_adapter_cancel_adapter(uint64_t request_id)
{
#if GET_LICENSE
    INFO_PRINTF("network cancel start, id=%llu\n", request_id);

    network_request_t *pos, *n;
    list_for_each_entry_safe(pos, n, &g_network_list, list) {
        if (pos->id == request_id) {
            pos->cancelled = true;

            // 尝试取消 HTTP 传输
            if (pos->http_ctx) {
                httpcli_cancel(pos->http_ctx);
            }

            // 回调 on_canceled
            if (pos->callback && pos->callback->on_canceled) {
                pos->response.request_id = request_id;
                pos->callback->on_canceled(pos->callback, &pos->response);
            }

            INFO_PRINTF("network cancel: request %llu cancelled\n", request_id);
            return;
        }
    }

    INFO_PRINTF("network cancel: request %llu not found\n", request_id);
#else
    (void)request_id;
#endif
}
void *your_thread_create_function(void (*func)(void *), void *arg)
{
    INFO_PRINTF("thread create start\n");
    // 如果无多线程，直接调用func，返回NULL
    func(arg);
    return NULL;
}
static uint64_t my_get_thread_id(void)
{
    // 本平台无 get_cur_thread_pid，离线地图单线程，线程 ID 直接返回 0
    return 0;
}
static void touch_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t point;
    lv_indev_get_point(lv_indev_get_act(), &point);

    switch (code) {
    case LV_EVENT_PRESSED:
        awk_map_touch_begin(mapId, point.x, point.y);
        INFO_PRINTF("awk_map_touch_begin\n");
        break;
    case LV_EVENT_PRESSING:
        awk_map_touch_update(mapId, point.x, point.y);
        INFO_PRINTF("awk_map_touch_update\n");
        break;
    case LV_EVENT_RELEASED:
    case LV_EVENT_PRESS_LOST:
        awk_map_touch_end(mapId, point.x, point.y);
        INFO_PRINTF("awk_map_touch_end\n");
        break;
    default:
        return;
    }

#if OPEN_INFO
    int a = get_system_ms();
#endif
    awk_map_do_render();
#if OPEN_INFO
    INFO_PRINTF("运行时间：%dms\n", get_system_ms() - a);
#endif
}
/*空函数，用来查看链接前的资源消耗时打开
int32_t awk_init(awk_context_t *context){}
int32_t awk_map_create_view(awk_map_view_param_t param){}
int awk_map_set_level(uint32_t a,float b){}
int32_t awk_map_set_center(uint32_t map_id, awk_map_coord2d_t coord2d){}
int32_t awk_map_do_render(void){};*/
static void amap_render_timer_cb(lv_timer_t *timer)
{
    // awk_map_do_render 是高德 SDK 提供的 int32_t(void)，适配 lv_timer_cb_t 需要包一层
    (void)timer;
    awk_map_do_render();
}
static void activate_on_success(const char *device_id, const char *license_id)
{
    INFO_PRINTF("\n========== 设备激活成功 ==========\n");
    INFO_PRINTF("✅ device_id : %s\n", device_id ? device_id : "NULL");
    INFO_PRINTF("✅ license_id: %s\n", license_id ? license_id : "NULL");
    INFO_PRINTF("===================================\n\n");

    // 步骤1：创建地图窗口
    awk_map_view_param_t awkParam;
    awkParam.port.width = map_hor;
    awkParam.port.height = map_ver;
    mapId = awk_map_create_view(awkParam);
    INFO_PRINTF("📍 地图创建成功，mapId = %u\n", mapId);
    // 步骤2：设置缩放等级和中心点
    awk_map_set_level(mapId, 14);
    awk_map_coord2d_t map_center;
    map_center.lat = 39.995273;
    map_center.lon = 116.473362;
    awk_map_set_center(mapId, map_center);
    // 挪动事件设置
    lv_obj_add_event_cb(lv_scr_act(), touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(lv_scr_act(), touch_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(lv_scr_act(), touch_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(lv_scr_act(), touch_cb, LV_EVENT_PRESS_LOST, NULL);
    // 步骤4：开始绘制
    INFO_PRINTF("开始绘制地图\n");
    lv_timer_t *t1 = lv_timer_create(amap_render_timer_cb, 500, NULL);
    lv_timer_set_repeat_count(t1, 1);
}
static void activate_on_fail(int code, const char *msg)
{
    INFO_PRINTF("\n========== 设备激活失败 ==========\n");
    INFO_PRINTF("❌ code: %d\n", code);
    INFO_PRINTF("❌ msg : %s\n", msg ? msg : "NULL");
    INFO_PRINTF("===================================\n\n");
}
/*static void test_header(awk_http_response_callback_t *cb,awk_http_response_t *response)
{
    INFO_PRINTF("header received\n");
    INFO_PRINTF("status=%d\n",response->status_code);
}
static void test_body(awk_http_response_callback_t *cb,awk_http_response_t *response)
{
    if(response->body)
    {
        INFO_PRINTF("body len=%d\n",response->body->length);
        INFO_PRINTF("%s\n",response->body->buffer);
    }
}
static void test_success(awk_http_response_callback_t *cb,awk_http_response_t *response)
{
    INFO_PRINTF("http success\n");
    if(response->body)
    {
        INFO_PRINTF("final len=%d\n",response->body->length);
    }
}
static void test_fail(awk_http_response_callback_t *cb,awk_http_response_t *response,int error)
{
    INFO_PRINTF("http fail=%d\n",error);
}
static void test_cancel(awk_http_response_callback_t *cb,awk_http_response_t *response)
{
    INFO_PRINTF("http canceled\n");
}
void test_baidu_http(void)
{
    awk_http_request_t request;
    awk_http_response_callback_t callback;


    memset(&request,0,sizeof(request));
    memset(&callback,0,sizeof(callback));


    request.url = "https://www.baidu.com";

    // GET
    request.method = AWK_HTTP_METHOD_GET;


    // 设置回调
    callback.on_receive_header = test_header;
    callback.on_receive_body   = test_body;
    callback.on_success        = test_success;
    callback.on_fail           = test_fail;
    callback.on_canceled       = test_cancel;


    uint64_t id =awk_aos_network_adapter_send_adapter(&request,&callback);


    INFO_PRINTF("request id=%llu\n",id);
}*/

/*============================================================================
 * JLGPU 矢量 / SVG 渲染测试
 *----------------------------------------------------------------------------
 * 目的：验证 Phase 1 的 lv_draw_jlgpu_vector 硬件加速链路是否真正跑通：
 *   SVG 解析 -> lv_draw_vector_dsc_t -> LV_DRAW_TASK_TYPE_VECTOR
 *   -> _jlgpu_evaluate 抢占(score=80) -> lv_draw_jlgpu_vector -> JLGPU 硬件
 *
 * 注意：Phase 1 的 lv_draw_jlgpu_vector.c 只实现了 SOLID 填充，不支持
 *       stroke / 渐变。所以测试图形必须只用 fill，不能带 stroke。
 *
 * 用法：将下方 AMAP_JLGPU_VECTOR_TEST 置 1，AMAP_USE_TILE_DEMO 开头会自动调用。
 *       默认 0 关闭，不影响地图渲染。
 *==========================================================================*/
#define AMAP_JLGPU_VECTOR_TEST 0

void amap_jlgpu_vector_test(void)
{
#if 0
    /* ---------- 测试1：内联 SVG，走完整 SVG -> JLGPU 硬件链路（已关闭） ---------- */
    /* 复杂场景：覆盖 JLGPU 后端支持的每种路径指令，全部 SOLID 填充、无 stroke。
     * 第1行：圆环(evenodd)、椭圆、圆角矩形、五角星(nonzero)、凹多边形
     * 第2行：心形(三次贝塞尔)、胶囊(圆弧A)、半透明叠加、旋转组、二次贝塞尔 */
    static const char svg_data[] =
        "<svg viewBox=\"0 0 1000 320\">"
        /* 圆环：evenodd 挖洞 */
        "<g fill-rule=\"evenodd\" fill=\"#ff0000\">"
        "<circle cx=\"80\" cy=\"80\" r=\"70\"/><circle cx=\"80\" cy=\"80\" r=\"28\"/>"
        "</g>"
        /* 椭圆 */
        "<ellipse cx=\"250\" cy=\"80\" rx=\"85\" ry=\"50\" fill=\"#00aa00\"/>"
        /* 圆角矩形 */
        "<rect x=\"380\" y=\"30\" width=\"170\" height=\"100\" rx=\"35\" fill=\"#0000ff\"/>"
        /* 五角星：自交多边形，默认 nonzero 全实心 */
        "<polygon fill=\"#ffaa00\" points=\"640,20 660,72 714,72 674,100 690,150 640,120 590,150 606,100 566,72 620,72\"/>"
        /* 凹多边形（向右箭头） */
        "<polygon fill=\"#8000ff\" points=\"840,30 920,30 920,90 840,90 880,60\"/>"
        /* 心形：三次贝塞尔 C */
        "<path fill=\"#ff69b4\" d=\"M80,320 C60,300 30,280 30,240 C30,205 60,190 80,165 C100,190 130,205 130,240 C130,280 100,300 80,320 Z\"/>"
        /* 胶囊：圆弧 A -> 三次贝塞尔扁平化 */
        "<path fill=\"#00cccc\" d=\"M220,200 L340,200 A50,50 0 0 1 340,300 L220,300 A50,50 0 0 1 220,200 Z\"/>"
        /* 半透明叠加：fill-opacity 叠加效果 */
        "<circle cx=\"470\" cy=\"250\" r=\"70\" fill=\"#ff0000\" fill-opacity=\"0.5\"/>"
        "<circle cx=\"540\" cy=\"250\" r=\"70\" fill=\"#00ff00\" fill-opacity=\"0.5\"/>"
        /* 旋转组：translate + rotate 矩阵 */
        "<g transform=\"translate(700,200) rotate(30)\">"
        "<rect x=\"0\" y=\"0\" width=\"130\" height=\"60\" fill=\"#ff8800\"/>"
        "</g>"
        /* 二次贝塞尔 Q */
        "<path fill=\"#00ff88\" d=\"M840,190 Q880,150 920,190 Q960,230 1000,190 L1000,310 L840,310 Z\"/>"
        "</svg>";

    static lv_image_dsc_t svg_dsc;
    svg_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    svg_dsc.header.w = 1000;
    svg_dsc.header.h = 320;
    svg_dsc.data_size = sizeof(svg_data) - 1;
    svg_dsc.data = (const uint8_t *)svg_data;

    /* lv_image_set_src 会触发 SVG 解码器 svg_decoder_info 设置 CUSTOM_DRAW，
     * 刷新时 svg_draw 构造 vector dsc -> lv_draw_vector -> JLGPU */
    lv_obj_t *svg = lv_image_create(lv_scr_act());
    lv_image_set_src(svg, &svg_dsc);
    lv_obj_set_pos(svg, 0, 0);
    INFO_PRINTF("jlgpu vector test: svg image created (%dx%d)\n",
                (int)svg_dsc.header.w, (int)svg_dsc.header.h);

    return;
#endif

    /* ---------- 测试2：文件 SVG，从 norflash 读取并走 SVG -> JLGPU 硬件链路 ---------- */
    /* 路径 "storage/res_nor_mode/C/lvgl/svg.svg" 的首字符 's' 同时是 LVGL FS 驱动字母
     * （lv_port_fs_init 注册 's'），lv_fs_resolve_path 剥离首字符后 fs_open 用 --path
     * 恢复完整路径，最终经 VFS fopen 打开该文件。SVG 解码器按扩展名 .svg 走 FILE 分支。 */
    lv_obj_t *svg = lv_image_create(lv_scr_act());
    lv_image_set_src(svg, "storage/res_nor_mode/C/lvgl/svg.svg");
    lv_obj_set_pos(svg, 0, 0);
    lv_obj_t *svg1 = lv_image_create(lv_scr_act());
    lv_image_set_src(svg1, "storage/res_nor_mode/C/lvgl/svg.svg");
    lv_obj_set_pos(svg1, 256, 0);
    lv_obj_t *svg2 = lv_image_create(lv_scr_act());
    lv_image_set_src(svg2, "storage/res_nor_mode/C/lvgl/svg.svg");
    lv_obj_set_pos(svg2, 512, 0);
    lv_obj_t *svg3 = lv_image_create(lv_scr_act());
    lv_image_set_src(svg3, "storage/res_nor_mode/C/lvgl/svg.svg");
    lv_obj_set_pos(svg3, 0, 256);
    uint32_t t_parse2 = get_system_ms();
    lv_disp_t *disp = lv_disp_get_default();
    lv_refr_now(disp);
    uint32_t t_parse1 = get_system_ms();
    printf("JLGPU SVG解析渲染时间：%d", t_parse1 - t_parse2);
    INFO_PRINTF("jlgpu vector test: file svg loaded from storage/res_nor_mode/C/lvgl/svg.svg\n");
    return;
    /* ---------- 测试3：直接矢量路径，绕过 SVG 解析，走 lv_draw_vector -> JLGPU（已废弃，return 之后不可达） ---------- */
    /* 用一个独立小 canvas 作为绘制目标，finish_layer 会立即派发 vector 任务 */
    static uint8_t test_canvas_buf[240 * 120 * 2];  /* RGB565 */
    lv_obj_t *test_canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(test_canvas, test_canvas_buf, 240, 120, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(test_canvas, 0, 100);

    lv_layer_t layer;
    lv_canvas_init_layer(test_canvas, &layer);

    lv_draw_vector_dsc_t *dsc = lv_draw_vector_dsc_create(&layer);
    lv_draw_vector_dsc_set_fill_color32(dsc, lv_color32_make(0, 255, 0, LV_OPA_COVER));

    /* 一个实心圆 */
    lv_vector_path_t *path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_fpoint_t c = { 60, 60 };
    lv_vector_path_append_circle(path, &c, 50, 50);
    lv_draw_vector_dsc_add_path(dsc, path);
    lv_vector_path_delete(path);

    /* 一个实心矩形（多路径复用同一 dsc） */
    path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_append_rectangle(path, 130, 10, 100, 100, 0, 0);
    lv_draw_vector_dsc_add_path(dsc, path);
    lv_vector_path_delete(path);

    lv_draw_vector(dsc);
    lv_draw_vector_dsc_delete(dsc);

    lv_canvas_finish_layer(test_canvas, &layer);
    INFO_PRINTF("jlgpu vector test: direct circle + rect drawn\n");
}

void AMAP_USE_TILE_DEMO(void)
{
    psram_heap_stats();
    INFO_PRINTF("LCD Width: %d, Height: %d\n", LCD_W, LCD_H);

#if AMAP_JLGPU_VECTOR_TEST
    /* JLGPU 矢量/SVG 渲染自测：默认关闭，需验证时置 AMAP_JLGPU_VECTOR_TEST 为 1 */
    amap_jlgpu_vector_test();
    return;
#endif
    // 本平台无 storage_device_ready()，改用 VFS 判断 SD 卡目录是否已挂载
    // （根目录 storage/sd1/C/SVG 存在即表示 SD 卡已就绪，与 SDK 读图路径一致）
    while (fdir_exist("storage/sd1/C/SVG")) {
        os_time_dly(3);
    }
#if GET_LICENSE
    // wifi_camera 已负责 WiFi 初始化，这里只等待连接成功拿到 IP
    INFO_PRINTF("[amap] 等待 WiFi 连接...\n");
    while (wifi_get_sta_connect_state() != WIFI_STA_NETWORK_STACK_DHCP_SUCC) {
        os_time_dly(10);  // 每 100ms 检查一次
    }
    INFO_PRINTF("[amap] WiFi 已就绪，进入地图\n");
#endif
    /*test_baidu_http();
    return;*/
    lv_obj_remove_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);  // 屏幕不可滚动
    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);  // 关闭滚动条
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_TRANSP, LV_PART_MAIN);
    memset(&context, 0, sizeof(awk_context_t)); //务必先memset
    context.tile_pixel_mode = AWK_PIXEL_MODE_RGB_565;
    context.device_id = "ac792amap";
    context.key = "6953e70e8ce4789125e3dcb6bafa3e09";
    context.root_dir = "storage/sd1/C/SVG";            //SDK内部文件夹根路径
    context.offline_map_dir = "storage/sd1/C/svgoff"; // 离线文件路径
    context.tile_mem_cache_max_size = 0;     //栅格图内存缓存限值，单位：KB
    context.tile_disk_cache_max_size = 200;  //栅格图磁盘缓存限值，单位：MB
    //context.poi_tile_mem_cache_max_size = 4 * 1024;  // POI 瓦片内存缓存，单位：KB（避免拖动时反复解析 .pbf，可调大）
    //context.poi_tile_disk_cache_max_size = 200;      // POI 瓦片磁盘缓存，单位：MB（落盘不占 RAM）
    context.tile_cache_decoded_bitmap = true;  // 缓存解码后的位图，减少重复解析/光栅化
    context.tile_clip_load = true;             // 加载瓦片时就裁剪（tile_mem_cache_max_size=0 时才有效）
    context.tile_load_mode = AWK_MAP_TILE_LOAD_OFFLINE;    //离线模式
    context.tile_style = AWK_MAP_TILE_STYLE_VECTOR_OP;//AWK_MAP_TILE_STYLE_VECTOR;
    //内存相关适配
    context.memory_adapter.mem_malloc = awk_mem_malloc_adapter;
    context.memory_adapter.mem_free = awk_mem_free_adapter;
    context.memory_adapter.mem_calloc = awk_mem_calloc_adapter;
    context.memory_adapter.mem_realloc = awk_mem_realloc_adapter;

    //文件相关适配
    context.file_adapter.file_open = awk_file_open_adapter;
    context.file_adapter.file_close = awk_file_close_adapter;
    context.file_adapter.file_read = awk_file_read_adapter;
    context.file_adapter.file_write = awk_file_write_adapter;
    context.file_adapter.file_mkdir = awk_file_mkdir_adapter;
    context.file_adapter.file_exists = awk_file_exists_adapter;
    context.file_adapter.file_remove = awk_file_remove_adapter;
    context.file_adapter.file_opendir = awk_file_opendir_adapter;
    context.file_adapter.file_closedir = awk_file_closedir_adapter;
    context.file_adapter.file_readdir = awk_file_readdir_adapter;
    context.file_adapter.file_seek = awk_file_seek_adapter;
    context.file_adapter.file_flush = awk_file_flush_adapter;
    context.file_adapter.file_rmdir = awk_file_rmdir_adapter;
    context.file_adapter.file_dir_exists = awk_file_dir_exists_adapter;
    context.file_adapter.file_get_size = awk_file_get_size_adapter;
    context.file_adapter.file_get_last_access = awk_file_get_last_access_adapter;
    context.file_adapter.file_rename = awk_file_rename_adapter;
    context.file_adapter.file_unzip = awk_file_unzip_adapter;
    context.file_adapter.file_tell = awk_file_tell_adapter;

    //网络相关适配
    context.network_adapter.send = awk_aos_network_adapter_send_adapter;
    context.network_adapter.cancel = awk_aos_network_adapter_cancel_adapter;

    //渲染绘制相关适配
    context.render_adapter.begin_drawing = awk_render_begin_drawing_adapter;
    context.render_adapter.commit_drawing = awk_render_commit_drawing_adapter;
    context.render_adapter.draw_point = awk_render_point_adapter;
    context.render_adapter.draw_polyline = awk_render_polyline_adapter;
    context.render_adapter.draw_polygon = awk_render_polygon_adapter;
    context.render_adapter.draw_bitmap = awk_render_bitmap_adapter;
    context.render_adapter.draw_color = awk_render_color_adapter;
    context.render_adapter.draw_svg_bitmap = awk_render_svg_bitmap_adapter;
    context.render_adapter.draw_text = awk_render_draw_text_adapter;
    context.render_adapter.measure_text = awk_render_measure_text_adapter;

    //其他系统相关适配
    context.system_adapter.get_system_time = awk_get_system_time_adapter;
    context.system_adapter.log_printf = awk_printf_adapter;
    //线程
    context.thread_adapter.get_thread_id = my_get_thread_id;
    int isOK = awk_init(&context);
    INFO_PRINTF("2590_awk_init : %d\n", isOK);
    if (isOK == 0) {
        awk_device_activate_param_t activate_param = {
            .area      = "mainland",
            .country   = "cn",
            .type      = 0,        // 0 = 新激活
            .data_type = "vector",  // 矢量地图
        };
#if GET_LICENSE
        // 在线激活：需要网络，回调异步返回
        awk_device_active_callback activate_cb = {
            .awk_device_active_on_success = activate_on_success,
            .awk_device_active_on_fail    = activate_on_fail,
        };
        INFO_PRINTF("📱 设备激活已提交，等待回调...\n");
        awk_activate_device(&activate_param, activate_cb);
        // 回调 activate_on_success / activate_on_fail 会在主线程中被调用
        // 主线程必须返回，不能阻塞在这里，否则回调无法送达
#else
        // 离线模式：不能走 awk_activate_device（它需要网络，而 GET_LICENSE=0 时网络适配
        // 是空实现，send 直接返回 0 从不回调 → 激活成功/失败回调永不触发，表现为既无成功也无失败）。
        // 改用本地 license 校验：awk_check_device_activated 检查 root_dir 下的 license 文件，
        // 已激活则直接建图渲染。
        bool activated = awk_check_device_activated(&activate_param, &context);
        INFO_PRINTF("📱 离线激活检查：%s\n", activated ? "已激活" : "未激活");
        if (activated) {
            activate_on_success(context.device_id, NULL);
        } else {
            activate_on_fail(-1, "offline: device not activated (no valid local license)");
        }
#endif
    } else {
        INFO_PRINTF("❌ awk_init 失败，跳过激活\n");
    }

}
#endif //CONFIG_LV9_ENABLE
#endif //AMAP_USE_CANVAS
#endif //LV_USE_DEMO_AMAP

