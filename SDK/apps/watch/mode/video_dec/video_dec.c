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
#include "os/os_api.h"
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
#include "avilib.h"        /* AVI 首帧封面(阶段二): 读侧 API */

/* 帧队列预建入口(app_playback_demux.c): 返回 0 成功, 非 0 失败走原路径 */
extern int app_playback_demux_frame_queue_prealloc(uint32_t size);
/* 帧队列常驻块释放(app_playback_demux.c): 回放退出时归还 128KB 物理池 */
extern void app_playback_demux_frame_queue_pool_release(void);


#define LOG_TAG             "[APP_VIDEO_DEC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"


#ifdef CONFIG_ENABLE_VLIST

/* 内存申请封装: 按"是否实际使能 PSRAM"(TCFG_PSRAM_DEV_ENABLE)选择 psram 堆。
 * 不要改用 TCFG_PSRAM_SIZE 判断: 该容量宏在未使能 PSRAM 的方案里仍 > 0,
 * 会误走 malloc_psram 去初始化 0x14000000 的未映射 PSRAM 区, 触发 hmem 写异常死机 */
#if TCFG_PSRAM_DEV_ENABLE
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
/* 回放预览源尺寸。必须与"屏的方向"一致(与 video_rec/video_photo 同源同规则):
 *   儿童照相机(宏=0): 屏 320x240 横 -> 240x320 竖, 旋转 90° 正好等比铺满
 *   儿童对讲机(宏=1): 屏 240x320 竖 -> 320x240 横, 旋转 90° 正好等比铺满 */
#if defined(TCFG_KID_CAMERA_UI_INTERCOM) && TCFG_KID_CAMERA_UI_INTERCOM
#define VDEC_PB_WIDTH          320
#define VDEC_PB_HEIGHT         240
#else
#define VDEC_PB_WIDTH          240
#define VDEC_PB_HEIGHT         320
#endif
#define VDEC_VIDEO_BUF_SIZE    (128u * 1024u) /* 帧池实测41KB/JPEG 33~44KB, 256KB过量挤压物理池 */
#define VDEC_AUDIO_BUF_SIZE    (16u * 1024u)
/*==============================================================================
 * Playback fb buffer pool
 *==============================================================================*/
#define VDEC_FB_POOL_SLOTS     2    /* 与 VDEC_PB_FB_NUM 一致: 双缓冲两块不能同址 */
#define VDEC_FB_POOL_ALIGN     8

/* 回放期固定 fb5 缓冲: 每轮导航重建管线都会 fb_draw_open/fb_close 一次,
 * 默认路径每轮 malloc/free 两块 115200 字节, 堆扩页在物理池上留下永久切痕,
 * 最终 pmalloc_continue 拿不到连续页死机。池不随 g_vd 走(memset 会清零)。 */
static struct {
    u8    enable;                          /* 仅回放模式内置位, 其余场景不接管 */
    u8    used[VDEC_FB_POOL_SLOTS];        /* 已交给 fb 层的槽 */
    u32   size[VDEC_FB_POOL_SLOTS];        /* 实际分配大小, 复用判据 */
    void *raw[VDEC_FB_POOL_SLOTS];         /* 申请到的原始指针, 供释放 */
    void *aligned[VDEC_FB_POOL_SLOTS];     /* 8 字节对齐后交给 fb 层 */
} g_vd_fb_pool;

/* 释放全部槽并清标志: 回放退出时调用, 缓冲还给物理池(录像还要用) */
static void vd_fb_pool_release(void)
{
    int i;

    g_vd_fb_pool.enable = 0;
    for (i = 0; i < VDEC_FB_POOL_SLOTS; i++) {
        if (g_vd_fb_pool.raw[i]) {
            phy_free(g_vd_fb_pool.raw[i]);   /* 走物理池: kc_free 只还给堆, 堆不把页还 pmm => 每轮永久少 230KB */
            g_vd_fb_pool.raw[i] = NULL;
        }
        g_vd_fb_pool.aligned[i] = NULL;
        g_vd_fb_pool.size[i] = 0;
        g_vd_fb_pool.used[i] = 0;
    }
}

/* fbvirtual.c 弱函数的强定义: 回放期复用固定缓冲, 池满/失败一律返回 NULL
 * 交回默认路径; 返回指针必须 8 字节对齐(fbvirtual.c 对用户指针不补对齐)。 */
void *fb_virtual_user_alloc_buffer(const char *fb_name, u32 size)
{
    int i;
    void *buf;

    if (!g_vd_fb_pool.enable || !fb_name || strcmp(fb_name, VDEC_PB_FB_NAME)) {
        return NULL;
    }
    for (i = 0; i < VDEC_FB_POOL_SLOTS; i++) {
        if (!g_vd_fb_pool.used[i] && g_vd_fb_pool.size[i] >= size) {
            g_vd_fb_pool.used[i] = 1;
            return g_vd_fb_pool.aligned[i];
        }
    }
    for (i = 0; i < VDEC_FB_POOL_SLOTS; i++) {
        if (!g_vd_fb_pool.used[i]) {
            break;
        }
    }
    if (i >= VDEC_FB_POOL_SLOTS) {
        return NULL;
    }
    buf = phy_malloc(size + VDEC_FB_POOL_ALIGN);   /* 物理池: 与 phy_free 配对, 真取真还 */
    if (!buf) {
        return NULL;
    }
    if (g_vd_fb_pool.raw[i]) {
        phy_free(g_vd_fb_pool.raw[i]);   /* 旧块不够大: 换块 */
    }
    g_vd_fb_pool.raw[i] = buf;
    g_vd_fb_pool.size[i] = size;
    g_vd_fb_pool.aligned[i] = buf;
    if ((u32)buf % VDEC_FB_POOL_ALIGN) {
        g_vd_fb_pool.aligned[i] = (void *)(((u32)buf + VDEC_FB_POOL_ALIGN - 1) & ~(VDEC_FB_POOL_ALIGN - 1));
    }
    g_vd_fb_pool.used[i] = 1;
    return g_vd_fb_pool.aligned[i];
}

/* 命中池内缓冲返回 1(已由用户处理, fb 层不要再 free), 其余返回 0 */
int fb_virtual_user_free_buffer(const char *fb_name, void *buf)
{
    int i;

    if (!g_vd_fb_pool.enable || !buf) {
        return 0;
    }
    for (i = 0; i < VDEC_FB_POOL_SLOTS; i++) {
        if (g_vd_fb_pool.used[i] && g_vd_fb_pool.aligned[i] == buf) {
            g_vd_fb_pool.used[i] = 0;
            return 1;
        }
    }
    log_info("[FBP] slot free MISS buf=%p\n", buf);   // 取证: 指针不在池里 => 槽位永不释放
    return 0;
}

/* 峰值前移: 进模式时(缩略图刚释放, 池子最干净)先把管线骨架的堆成本取走——
 * fb5 两块 115200 与帧队列 128KB 同抢内部物理池, 首播时再取会拿不到连续块。
 * 只入池不占位(used=0), fb5 通道仍由首播 pipeline_open 建一次并命中池块。 */
static int vd_fb_pool_prime(void)
{
    int i;
    int ready = 0;
    /* fbvirtual.c 按 s_xres*s_yres*bpp/8 请求, YUV420SP bpp=12 ⇒ 320x240 = 115200 */
    u32 size = (u32)VDEC_PB_WIDTH * VDEC_PB_HEIGHT * 12 / 8;

    for (i = 0; i < VDEC_FB_POOL_SLOTS; i++) {
        if (g_vd_fb_pool.raw[i] != NULL) {
            ready++;                /* 已在池中或已交给 fb 层 */
            continue;
        }
        g_vd_fb_pool.raw[i] = phy_malloc(size + VDEC_FB_POOL_ALIGN);   /* 物理池: 与 phy_free 配对 */
        if (g_vd_fb_pool.raw[i] == NULL) {
            break;
        }
        g_vd_fb_pool.aligned[i] = g_vd_fb_pool.raw[i];
        if ((u32)g_vd_fb_pool.raw[i] % VDEC_FB_POOL_ALIGN) {
            g_vd_fb_pool.aligned[i] = (void *)(((u32)g_vd_fb_pool.raw[i] + VDEC_FB_POOL_ALIGN - 1) & ~(VDEC_FB_POOL_ALIGN - 1));
        }
        g_vd_fb_pool.size[i] = size;
        g_vd_fb_pool.used[i] = 0;
        ready++;
    }

    return ready;
}

/* 预建失败回滚: 只释放还没交给 fb 层的槽, 已占用的槽留给 fb 层正常归还 */
static void vd_fb_pool_unprime(void)
{
    int i;

    for (i = 0; i < VDEC_FB_POOL_SLOTS; i++) {
        if (g_vd_fb_pool.raw[i] && !g_vd_fb_pool.used[i]) {
            phy_free(g_vd_fb_pool.raw[i]);   /* 与 phy_malloc 配对(原 kc_free 是配对错) */
            g_vd_fb_pool.raw[i] = NULL;
            g_vd_fb_pool.aligned[i] = NULL;
            g_vd_fb_pool.size[i] = 0;
        }
    }
}
/*==============================================================================
 * Global context
 *==============================================================================*/
static struct {
    /* 遗留簿记, 留待未来模式内 LIST UI 使用, 当前不 prefetch */
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
    u16  corruption_skip_count;   /* 跳过预算: 与 file_count 同量级(实测>255), u8 会回绕 */
    // 最近一次导航方向: +1=下一个, -1=上一个(跳过失败项时按此方向走)
    s8   nav_dir;
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
/* 相册列表指定起始播放文件下标: app_enter_video_dec_mode 从 arg(index+1) 解析写入,
 * video_dec_init 消费后复位(-1=不指定, 维持默认首文件) */
static int s_pending_start_index = -1;
/* 是否从相册六宫格列表进入回放: 是则 BACK 时 UI 层已把屏弹回列表,
 * app 层不再 video_dec_ui_back_to_home 二次弹屏(否则会越过列表直接回 home/聊天) */
static bool s_entered_from_grid = false;
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
    // 以句柄为准关闭: 显示失败时 photo_showing 仍为 0, 但管线可能已打开
    if (g_vd.pb_handle) {
        app_playback_image_pipeline_close(g_vd.pb_handle);
        g_vd.photo_showing = 0;
    }
}
// 归一化导航方向: 非法值(memset 后的 0)按 +1 处理
static int vd_nav_dir_norm(void)
{
    if (g_vd.nav_dir != -1) {
        g_vd.nav_dir = 1;
    }
    return g_vd.nav_dir;
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
 * Async storage info
 *
 * fget_free_space 遍历 FAT 首次耗时数秒(既有结论, 见 video_rec_worker.c):
 * 1371 文件/30GB 分区实测 ~14s, 不能跑在进屏同步路径。改独立低优先级线程
 * 后台算, 进屏只投递; 重扫/拔卡按代际作废在飞结果。
 *==============================================================================*/
static volatile u32 g_vdec_storage_total_kb = 0;
static volatile u32 g_vdec_storage_free_kb = 0;
static volatile u8  g_vdec_storage_done = 0;   /* 后台已算出有效值 */
static volatile u8  g_vdec_storage_busy = 0;   /* 计算线程在飞, 防重入 */
static volatile u32 g_vdec_storage_gen = 0;    /* 代际: 重扫/拔卡使在飞结果作废 */

static void vd_storage_invalidate(void)
{
    g_vdec_storage_gen++;
    g_vdec_storage_done = 0;
    g_vdec_storage_total_kb = 0;
    g_vdec_storage_free_kb = 0;
}

/* 后台线程: 慢遍历 FAT; 代际已变则丢弃结果不发布 */
static void vd_storage_worker(void *arg)
{
    u32 gen = g_vdec_storage_gen;
    video_dec_storage_t info;

    (void)arg;
    if (video_dec_get_storage_info(&info) == 0 && gen == g_vdec_storage_gen) {
        video_dec_storage_t cached;

        g_vdec_storage_total_kb = info.total_kb;
        g_vdec_storage_free_kb = info.free_kb;
        g_vdec_storage_done = 1;
        cached.total_kb = g_vdec_storage_total_kb;
        cached.free_kb = g_vdec_storage_free_kb;
        cached.photo_count = 0;
        cached.video_count = 0;
        video_dec_ui_update_storage(&cached);
    }
    g_vdec_storage_busy = 0;
}

/* 进屏/重扫路径: 只投递线程, 不做 FAT 遍历 */
static void vd_storage_recalc_async(void)
{
    int pid = 0;

    if (g_vdec_storage_busy || g_vdec_storage_done) {
        return;
    }
    g_vdec_storage_busy = 1;
    if (thread_fork("vd_storage_calc", VIDEO_DEC_THUMB_WORKER_PRIO, 2048, 0, &pid,
                    vd_storage_worker, NULL) != OS_NO_ERR) {
        log_error("vd_storage_recalc_async: fork fail\n");
        g_vdec_storage_busy = 0;
    }
}
/*==============================================================================
 * File scanning
 *==============================================================================*/
static int vd_scan_and_load(void)
{
    vd_storage_invalidate();   /* 重扫后旧容量作废, 在飞结果丢弃 */
    /* 模式内重扫前先停六宫格缩略图 worker(防与回放抢 VDEC/VPSS, 双保险) */
    video_dec_thumb_cancel_wait();
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
    g_vd.thumb_data = (u8 **)phy_malloc(file_count * sizeof(u8 *));   /* 物理池: 与 phy_free(arr) 配对 */
    if (g_vd.thumb_data) {
        memset(g_vd.thumb_data, 0, file_count * sizeof(u8 *));
    }
    g_vd.thumb_count = file_count;

    /* 遗留: 原 9 格页 prefetch 已删(真缩略图走六宫格页异步引擎, 不再模式内白解码) */
    vd_set_state(VIDEO_DEC_STATE_LIST);
    vd_ui_refresh_grid();
    vd_storage_recalc_async();   /* 容量后台算, 进屏不等 FAT 遍历 */
    return file_count;
}

/* 插卡后重扫并自动显示第一个 (kid_camera 应用层 SD_EVENT 调用) */
void video_dec_refresh_after_insert(void)
{
    video_dec_thumb_invalidate();   /* 同名文件内容可能已变, 旧缓存作废 */
    vd_scan_and_load();
    if (flm_get_count() > 0) {
        video_dec_play(0);
    }
}

/* 拔卡: 停播放/关照片, 清索引状态, 隐藏进度标签清显示回空列表 */
void video_dec_clear_on_card_out(void)
{
    video_dec_thumb_invalidate();   /* 拔卡: 旧 path 全部失效 */
    if (g_vd.video_playing || g_vd.video_paused) {
        video_dec_stop_video();
    }
    vd_close_photo_if_showing();
    vd_free_files();  /* 先按旧 file_count 释放缩略图, flm_reset 会把计数清零 */
    flm_reset();
    vd_storage_invalidate();   /* 拔卡: 旧容量作废 */
    g_vd.current_index = 0;
    s_del_menu = 0;
    vd_set_state(VIDEO_DEC_STATE_LIST);
    video_dec_ui_time_hide();
    video_dec_ui_clear_video();
    video_dec_ui_refresh_grid(NULL, 0, -1);
    video_dec_ui_show_alert(1);
}
/* 只按入参释放、不读 g_vd: 退出异步线程复用同一份释放逻辑, 入参是已摘除所有权的旧数组 */
static void vd_free_files_ptr(u8 **arr, int count)
{
    int i;

    if (!arr) {
        return;
    }
    for (i = 0; i < count; i++) {
        if (arr[i]) {
            kc_free(arr[i]);
            arr[i] = NULL;
        }
    }
    phy_free(arr);
}
static void vd_free_files(void)
{
    /* 遗留簿记: thumb_data 仅服务未来模式内 LIST UI, 当前不再有生成方 */
    /* 按自己记录的条目数释放, 不依赖 flm 计数(删除/拔卡路径会先清零 flm, 按 flm
     * 遍历会漏掉全部缩略图条目 -> 每删一批泄漏 ~294KB) */
    vd_free_files_ptr(g_vd.thumb_data, g_vd.thumb_count);
    g_vd.thumb_data = NULL;
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
    log_info("[VDEC] play_enter idx=%d\n", index);
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
    g_vd.current_index = index;
    flm_file_t entry = {0};
    int entry_ok = (flm_get_entry(index, &entry) == 0);
    // 照片解码成功: 保留现有照片管线不关
    int photo_keep = 0;
    if (entry_ok) {
        log_info("[VDEC] play_type=%d idx=%d\n", entry.type, index);
    }
    if (entry_ok && entry.type == VIDEO_DEC_TYPE_PHOTO) {
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
            if (ret == APP_PLAYBACK_ERR_BUSY) {
                // 视频 ERROR 事件不碰 pb_handle, video_started 残留 1 会把后续照片全判 busy
                log_info("video_dec_play: photo BUSY, force stop video and retry\n");
                app_playback_video_stop(g_vd.pb_handle);
                g_vd.video_playing = 0;
                g_vd.video_paused = 0;
                ret = app_playback_one_image_path(g_vd.pb_handle, entry.path);
            }
            if (ret == APP_PLAYBACK_OK) {
                vd_set_state(VIDEO_DEC_STATE_PHOTO_VIEW);
                g_vd.photo_showing = 1;
                g_vd.corruption_skip_count = 0;
                photo_keep = 1;
                break;
            }
            if (ret == APP_PLAYBACK_ERR_BUSY) {
                log_error("video_dec_play: photo BUSY after force stop ret=%d (%s), skip\n",
                          ret, entry.path);
            } else {
                log_error("video_dec_play: one_image_path failed ret=%d (corrupt %s), skip\n",
                          ret, entry.path);
            }
            if (file_count <= 1 || ++photo_tries >= file_count) {
                video_dec_ui_show_alert(2);   /* 全部无法解码 */
                entry_ok = 0;
                break;
            }
            g_vd.current_index = (g_vd.current_index + vd_nav_dir_norm() + file_count) % file_count;
            if (flm_get_entry(g_vd.current_index, &entry) != 0) {
                log_error("video_dec_play: flm_get_entry failed idx=%d\n", g_vd.current_index);
                video_dec_ui_show_alert(2);
                entry_ok = 0;
                break;
            }
            if (entry.type != VIDEO_DEC_TYPE_PHOTO) {
                break;  /* 跳到了视频, 交给下面视频分支 */
            }
        }
    }
    // 视频目标或照片解码失败: 关照片管线; 照片→照片复用时不关(免 VO fb 反复 alloc/free)
    if (!photo_keep) {
        vd_close_photo_if_showing();
    }
    if (!entry_ok) {
        return -1;
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
            /* 单个文件起播失败: 复用跳过预算, 经消息队列静默跳下一个(不递归);
             * 预算用尽=整段都放不了, 才提示一次 */
            if (file_count > 1 && g_vd.corruption_skip_count < file_count) {
                app_send_message2(APP_MSG_VIDEO_DEC_SKIP_CORRUPT, (int)g_vd.play_gen, 0);
            } else {
                video_dec_ui_show_alert(2);
            }
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
/* 只做静音恢复动作、不写 g_vd: 退出异步线程只调本函数 */
static void vd_mute_restore_apply(u8 mute_on)
{
    if (mute_on) {
        app_audio_mute(AUDIO_MUTE_DEFAULT);
        log_info("video_dec_mode_exit_mute_restore: restore mute ON\n");
    } else {
        app_audio_mute(AUDIO_UNMUTE_DEFAULT);
        log_info("video_dec_mode_exit_mute_restore: restore mute OFF\n");
    }
}
void video_dec_mode_exit_mute_restore(void)
{
    vd_mute_restore_apply(g_vd.mute_before_enter);
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
    g_vd.current_index = (g_vd.current_index + vd_nav_dir_norm() + file_count) % file_count;
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
        g_vd.current_index = (g_vd.current_index + vd_nav_dir_norm() + file_count) % file_count;
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
            g_vd.nav_dir = 1;   // OK 键语义 = 下一张
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
            /* 遗留: 原 9 格页 prefetch 已删(缩略图由六宫格页引擎负责) */
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
            g_vd.nav_dir = 1;
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
            /* 遗留: 原 9 格页 prefetch 已删(缩略图由六宫格页引擎负责) */
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
            g_vd.nav_dir = -1;
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
        if (!s_entered_from_grid) {
            video_dec_ui_back_to_home();
        }
        break;
    case VIDEO_DEC_STATE_VIDEO_PLAY:
        video_dec_stop_video();
        if (!s_entered_from_grid) {
            video_dec_ui_back_to_home();
        }
        break;
    case VIDEO_DEC_STATE_LIST:
    default:
        if (!s_entered_from_grid) {
            video_dec_ui_back_to_home();
        }
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
        video_dec_thumb_invalidate();   /* 删除后 flm 索引前移, path 键缓存作废 */
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
    video_dec_thumb_invalidate();   /* 全删: path 键缓存作废 */
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
/* 遗留簿记, 留待未来模式内 LIST UI 使用, 当前不 prefetch */
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
            kc_free(g_vd.thumb_data[victim]);   /* 装的是 f.thumbnail(FLM 的指针), 配对未证实 ⇒ 保持 kc_free */
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

/* 遗留簿记, 留待未来模式内 LIST UI 使用, 当前不 prefetch */
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

/*==============================================================================
 * 照片缩略图解码核心 + 异步引擎 (六宫格页, JPG 真缩略图)
 *
 * 线程归属: request/peek/set_ready_cb/cancel_wait/invalidate/release_all 供 UI
 * (LVGL)线程调用; 解码落在专用低优先级 worker(4KB 栈), 完成经 ready_cb 通知 UI
 * (UI 侧只拷 path/gidx 后 lvgl_rpc 归队, 不得带 tile 指针跨线程)。
 * 生命周期: 页面 LOAD 注册+请求, UNLOAD release_all 全释放(无全局常驻缓存);
 * 模式侧只留 cancel/invalidate 挂点作双保险。
 * 显示安全: LRU<=6=一页请求数; 请求批次内新页 tile 不会被同批次淘汰(UI 先
 * rebuild 清理旧格再 request, 命中/attach 时 re-peek), 淘汰只淘汰已离页 tile。
 * 内存纪律: JPG 整读<=128KB + VDEC LINE<=43KB + VPSS<=29KB + tile 12KB, 中间
 * 缓冲解完即放; 解码前按空闲内存与维数核算预算, 不足直接降级不试。
 *==============================================================================*/
#define VTHUMB_VDEC_LINE_CNT      16
#define VTHUMB_VDEC_CACHE_LINES   32
#define VTHUMB_VDEC_FRAME_EXTRA   1024
#define VTHUMB_LINE_BUF_MAX       (43u * 1024u)   /* LINE 缓冲维数门槛(640 宽 420/422 放行) */
#define VTHUMB_VPSS_BUF_EXTRA     1024
#define VTHUMB_EXTRA_BUDGET       (92u * 1024u)   /* LINE+VPSS+tile+余量(整读 128KB 时峰值约 210KB) */
#define VTHUMB_GET_FRAME_TIMEOUT  2000            /* 单张离线下帧超时(ms), 决定 cancel 等待上界 */
#define VTHUMB_AVI_MAX_CHUNK      (128u * 1024u)  /* AVI 单帧(JPEG)防护: MJPG 640x480 帧 30-60KB */
#define VTHUMB_SLOT_EMPTY         0
#define VTHUMB_SLOT_WAIT          1
#define VTHUMB_SLOT_SKIP          2               /* 未知类型: 不参与解码 */
#define VTHUMB_SLOT_WORK          3               /* worker 解码中 */
#define VTHUMB_TILE_BG_R          0x2E             /* 留边底色=现照片占位 0x2E3A55 同系 */
#define VTHUMB_TILE_BG_G          0x3A
#define VTHUMB_TILE_BG_B          0x55

typedef struct {
    char path[128];
    int  gidx;
    u16  gen;
    u8   state;
    u8   type;                       /* VIDEO_DEC_TYPE_PHOTO / VIDEO(AVI) */
} vd_thumb_slot_t;

typedef struct {
    char path[128];
    u8  *tile;                       /* kc_malloc 12288B */
} vd_thumb_lru_t;

static struct {
    OS_MUTEX lock;
    OS_SEM   wake;                   /* worker 唤醒 */
    OS_SEM   done;                   /* worker 完成/ack 通知(cancel_wait 轮询) */
    u8   init;
    vd_thumb_slot_t slot[VIDEO_DEC_THUMB_SLOT_MAX];
    vd_thumb_lru_t  lru[VIDEO_DEC_THUMB_LRU_MAX];
    int  lru_count;
    u16  gen;
    u8   cancel;
    u8   busy;
    u8   exit;
    int  worker_pid;
    video_dec_thumb_ready_cb_t ready_cb;
    void *ready_priv;
} g_thumb;

static void vd_thumb_engine_init(void)
{
    if (g_thumb.init) {
        return;
    }
    os_mutex_create(&g_thumb.lock);
    os_sem_create(&g_thumb.wake, 0);
    os_sem_create(&g_thumb.done, 0);
    g_thumb.init = 1;
}

static void vd_thumb_slot_clear_locked(void)
{
    int i;
    for (i = 0; i < VIDEO_DEC_THUMB_SLOT_MAX; i++) {
        g_thumb.slot[i].state = VTHUMB_SLOT_EMPTY;
        g_thumb.slot[i].path[0] = '\0';
        g_thumb.slot[i].gidx = -1;
    }
}

/* 以下 LRU 操作调用方需已持锁; lru[0] 最近使用, 队尾最久未用 */
static int vd_thumb_lru_find_locked(const char *path)
{
    int i;
    for (i = 0; i < g_thumb.lru_count; i++) {
        if (!strcmp(g_thumb.lru[i].path, path)) {
            return i;
        }
    }
    return -1;
}

static u8 *vd_thumb_lru_touch_locked(const char *path)
{
    int i;
    vd_thumb_lru_t tmp;

    i = vd_thumb_lru_find_locked(path);
    if (i < 0) {
        return NULL;
    }
    if (i > 0) {
        tmp = g_thumb.lru[i];
        for (; i > 0; i--) {
            g_thumb.lru[i] = g_thumb.lru[i - 1];
        }
        g_thumb.lru[0] = tmp;
    }
    return g_thumb.lru[0].tile;
}

static void vd_thumb_lru_insert_locked(const char *path, u8 *tile)
{
    int i;

    if (g_thumb.lru_count >= VIDEO_DEC_THUMB_LRU_MAX) {
        i = g_thumb.lru_count - 1;               /* 淘汰队尾并立即释放 tile */
        if (g_thumb.lru[i].tile) {
            phy_free(g_thumb.lru[i].tile);
            g_thumb.lru[i].tile = NULL;
        }
        g_thumb.lru_count--;
    }
    for (i = g_thumb.lru_count; i > 0; i--) {
        g_thumb.lru[i] = g_thumb.lru[i - 1];
    }
    snprintf(g_thumb.lru[0].path, sizeof(g_thumb.lru[0].path), "%s", path);
    g_thumb.lru[0].tile = tile;
    g_thumb.lru_count++;
}

static u32 vd_thumb_frame_size(u16 width, u16 height, mpi_pixel_format_t format)
{
    switch (format) {
    case MPI_PIXEL_FMT_YUV420SP:
        return (u32)width * height * 3u / 2u;
    case MPI_PIXEL_FMT_YUV444_PACKED:
        return (u32)width * height * 3u;
    case MPI_PIXEL_FMT_YUV422_PACKED:
    default:
        return (u32)width * height * 2u;
    }
}

/* VPSS 输出档: 按源宽高比取 <=160x120 的偶数适配尺寸(4:3 源即 160x120 本档) */
static int vd_thumb_calc_vpss_size(u16 src_w, u16 src_h, u16 *out_w, u16 *out_h)
{
    u32 tw;
    u32 th;

    if (src_w < 2 || src_h < 2) {
        return -1;
    }
    if ((u32)src_w * VIDEO_DEC_THUMB_VPSS_H_MAX <=
        (u32)src_h * VIDEO_DEC_THUMB_VPSS_W_MAX) {
        th = VIDEO_DEC_THUMB_VPSS_H_MAX;           /* 高度受限 */
        tw = (u32)src_w * VIDEO_DEC_THUMB_VPSS_H_MAX / src_h;
    } else {
        tw = VIDEO_DEC_THUMB_VPSS_W_MAX;           /* 宽度受限 */
        th = (u32)src_h * VIDEO_DEC_THUMB_VPSS_W_MAX / src_w;
    }
    if (tw < 2) {
        tw = 2;
    }
    if (th < 2) {
        th = 2;
    }
    *out_w = (u16)(tw & ~1u);
    *out_h = (u16)(th & ~1u);
    return (*out_w >= 2 && *out_h >= 2) ? 0 : -1;
}

#if !VIDEO_DEC_TILE_NV12_TEST
/* YUV420SP -> 96x64 RGB565 最近邻采样: 整图 min-fit, 留边填占位底色 */
static void vd_thumb_yuv420sp_to_rgb565(const mpi_video_frame_info_t *frame, u8 *tile)
{
    const u8 *y;
    const u8 *uv;
    u32 stride;
    u32 out_w;
    u32 out_h;
    u32 dw;
    u32 dh;
    u32 ox;
    u32 oy;
    u32 ty;
    u32 tx;
    u16 bg;
    u16 *dst = (u16 *)tile;
    int u_first;

    if (!frame || !tile) {
        return;
    }
    y = frame->y_addr;
    uv = frame->c_addr;
    out_w = frame->width;
    out_h = frame->height;
    stride = frame->stride ? frame->stride : out_w;
    u_first = (frame->yuv_order != MPI_YUV_ORDER_YVYU) ? 1 : 0;
    bg = (u16)(((VTHUMB_TILE_BG_R >> 3) << 11) | ((VTHUMB_TILE_BG_G >> 2) << 5) |
               (VTHUMB_TILE_BG_B >> 3));
    if (!y || !uv || out_w < 2 || out_h < 2) {
        for (ty = 0; ty < VIDEO_DEC_THUMB_TILE_H; ty++) {
            for (tx = 0; tx < VIDEO_DEC_THUMB_TILE_W; tx++) {
                dst[ty * VIDEO_DEC_THUMB_TILE_W + tx] = bg;
            }
        }
        return;
    }
    /* 4:3 整图适配: min-fit 到 96x64 */
    if ((u32)out_w * VIDEO_DEC_THUMB_TILE_H <= (u32)out_h * VIDEO_DEC_THUMB_TILE_W) {
        dh = VIDEO_DEC_THUMB_TILE_H;
        dw = (u32)out_w * VIDEO_DEC_THUMB_TILE_H / out_h;
    } else {
        dw = VIDEO_DEC_THUMB_TILE_W;
        dh = (u32)out_h * VIDEO_DEC_THUMB_TILE_W / out_w;
    }
    ox = ((u32)VIDEO_DEC_THUMB_TILE_W - dw) / 2;
    oy = ((u32)VIDEO_DEC_THUMB_TILE_H - dh) / 2;
    for (ty = 0; ty < VIDEO_DEC_THUMB_TILE_H; ty++) {
        for (tx = 0; tx < VIDEO_DEC_THUMB_TILE_W; tx++) {
            u16 px = bg;
            if (tx >= ox && tx < ox + dw && ty >= oy && ty < oy + dh) {
                u32 sx = (tx - ox) * out_w / dw;
                u32 sy = (ty - oy) * out_h / dh;
                u32 uv_off = (sy >> 1) * stride + ((sx >> 1) << 1);
                int C = (int)y[sy * stride + sx] - 16;
                int D = (int)uv[uv_off + (u_first ? 0 : 1)] - 128;
                int E = (int)uv[uv_off + (u_first ? 1 : 0)] - 128;
                int R = (298 * C + 409 * E + 128) >> 8;
                int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
                int B = (298 * C + 516 * D + 128) >> 8;
                if (R < 0) {
                    R = 0;
                } else if (R > 255) {
                    R = 255;
                }
                if (G < 0) {
                    G = 0;
                } else if (G > 255) {
                    G = 255;
                }
                if (B < 0) {
                    B = 0;
                } else if (B > 255) {
                    B = 255;
                }
                px = (u16)(((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3));
            }
            dst[ty * VIDEO_DEC_THUMB_TILE_W + tx] = px;
        }
    }
}
#endif /* !VIDEO_DEC_TILE_NV12_TEST: RGB565 现网路径 */

/* NV12 直显测试(VIDEO_DEC_TILE_NV12_TEST=1): jlgpu 渲染侧滞留物理池问题复现(2026-09-07) */
#if VIDEO_DEC_TILE_NV12_TEST
/* tile 组装: VPSS 帧(YUV420SP 160x120) 最近邻缩到 96x64 NV12(Y+UV 连续, U 先) */
static void vd_thumb_frame_to_nv12_tile(const mpi_video_frame_info_t *frame, u8 *tile)
{
    const u8 *y;
    const u8 *uv;
    u32 stride;
    u32 out_w;
    u32 out_h;
    u32 dw;
    u32 dh;
    u32 ox;
    u32 oy;
    u32 ty;
    u32 tx;
    int u_first;
    u8 *y_dst = tile;
    u8 *uv_dst = tile + VIDEO_DEC_THUMB_TILE_W * VIDEO_DEC_THUMB_TILE_H;

    if (!frame || !tile) {
        return;
    }
    y = frame->y_addr;
    uv = frame->c_addr;
    out_w = frame->width;
    out_h = frame->height;
    stride = frame->stride ? frame->stride : out_w;
    u_first = (frame->yuv_order != MPI_YUV_ORDER_YVYU) ? 1 : 0;
    if (!y || !uv || out_w < 2 || out_h < 2) {
        memset(tile, 65, VIDEO_DEC_THUMB_TILE_BYTES);   /* 占位底色(Y≈65) */
        return;
    }
    /* 4:3 整图适配: min-fit 到 96x64, 留边填底色(0x2E3A55 -> BT601 Y=65 U=141 V=120) */
    if ((u32)out_w * VIDEO_DEC_THUMB_TILE_H <= (u32)out_h * VIDEO_DEC_THUMB_TILE_W) {
        dh = VIDEO_DEC_THUMB_TILE_H;
        dw = (u32)out_w * VIDEO_DEC_THUMB_TILE_H / out_h;
    } else {
        dw = VIDEO_DEC_THUMB_TILE_W;
        dh = (u32)out_h * VIDEO_DEC_THUMB_TILE_W / out_w;
    }
    ox = ((u32)VIDEO_DEC_THUMB_TILE_W - dw) / 2;
    oy = ((u32)VIDEO_DEC_THUMB_TILE_H - dh) / 2;
    for (ty = 0; ty < VIDEO_DEC_THUMB_TILE_H; ty++) {
        for (tx = 0; tx < VIDEO_DEC_THUMB_TILE_W; tx++) {
            u32 yv = 65;
            u32 uv_u = 141;
            u32 uv_v = 120;
            if (tx >= ox && tx < ox + dw && ty >= oy && ty < oy + dh) {
                u32 sx = (tx - ox) * out_w / dw;
                u32 sy = (ty - oy) * out_h / dh;
                yv = y[sy * stride + sx];
                if ((tx & 1) == 0 && (ty & 1) == 0) {
                    u32 uv_off = (sy >> 1) * stride + (sx & ~1u);
                    u32 c0 = uv[uv_off];
                    u32 c1 = uv[uv_off + 1];
                    uv_u = u_first ? c0 : c1;   /* NV12 要求 U 在前 */
                    uv_v = u_first ? c1 : c0;
                }
            }
            y_dst[ty * VIDEO_DEC_THUMB_TILE_W + tx] = (u8)yv;
            if ((tx & 1) == 0 && (ty & 1) == 0) {
                u32 uv_idx = ((ty >> 1) * (VIDEO_DEC_THUMB_TILE_W >> 1) + (tx >> 1)) << 1;
                uv_dst[uv_idx] = (u8)uv_u;
                uv_dst[uv_idx + 1] = (u8)uv_v;
            }
        }
    }
}
#endif /* VIDEO_DEC_TILE_NV12_TEST: NV12 直显测试路径 */

static void vd_thumb_fill_chn(int mod_id, int dev_id, int chn_id, mpi_chn_t *chn)
{
    chn->mod_id = mod_id;
    chn->dev_id = dev_id;
    chn->chn_id = chn_id;
}

/* 解码 path 照片 -> kc_malloc 的 96x64 RGB565 tile; 失败返回 NULL 且不残留任何分配。
 * 任一步失败/超预算均完整清理后返回 NULL(UI 侧保持现占位, 不崩不黑)。 */
/* AVI(MJPG) 首有效帧 JPEG 提取(阶段二封面, 2026-09-05): avilib 顺序跳过 0 长帧,
 * 取首个有效视频帧字节。返回 kc 缓冲归调用方 kc_free。超防护/失败返回非 0。 */
static int vd_thumb_avi_first_jpg(const char *path, u8 **jpg_out, u32 *len_out)
{
    avi_t *avi;
    const char *comp;
    long frames;
    long need;
    u8 *buf = NULL;

    if (!path || !path[0]) {
        return -1;
    }
    avi = AVI_open_input_file(path, 0);   /* getIndex=0: 首帧顺序扫描, 不付整份文件索引 */
    if (!avi) {
        log_error("vd_thumb: AVI open fail: %s\n", path);
        return -1;
    }
    comp = AVI_video_compressor(avi);
    if (!comp || strncmp(comp, "MJPG", 4) != 0) {
        log_error("vd_thumb: AVI not MJPG (%s): %s\n", comp ? comp : "?", path);
        AVI_close(avi);
        return -1;
    }
    frames = AVI_video_frames(avi);
    if (frames <= 0) {
        log_error("vd_thumb: AVI no frames: %s\n", path);
        AVI_close(avi);
        return -1;
    }
    need = AVI_read_first_frame(avi, NULL, 0);   /* 第一遍: 只探首帧长度(不依赖索引) */
    if (need <= 0 || need > VTHUMB_AVI_MAX_CHUNK) {
        log_error("vd_thumb: AVI first frame over budget %ld: %s\n", need, path);
        AVI_close(avi);
        return -1;
    }
    buf = (u8 *)phy_malloc((u32)need);   /* 第二遍: 按实际帧长分配, 替代恒定 128KB */
    if (!buf) {
        AVI_close(avi);
        return -1;
    }
    if (AVI_read_first_frame(avi, buf, need) != need) {
        phy_free(buf);
        AVI_close(avi);
        return -1;
    }
    *jpg_out = buf;
    *len_out = (u32)need;
    AVI_close(avi);
    return 0;
}

/* 缩略图解码全局门控: 回放模式(mode_running)占用 VDEC 期间禁止;
 * IDLE/拍照/录像主界面等宿主(六宫格入口)允许——相册切录像时的 pmm 峰值叠加
 * 由请求串行与 cancel 兜底, 禁止模式过严会导致主页直进六宫格永不补图。 */
static int vd_thumb_decode_allowed(void)
{
    u8 cur = app_get_current_mode_name();
    return !g_vd.mode_running &&
           (cur == APP_MODE_IDLE || cur == APP_MODE_PHOTO ||
            cur == APP_MODE_VIDEO_REC);
}

static u8 *vd_thumb_decode_tile(const char *path, u8 type)
{
    FILE *fp = NULL;
    u8 *jpg = NULL;
    u8 *tile = NULL;
    u8 jpg_exif = 0;
    u32 jpg_len = 0;
    u32 free_base;
    u32 vdec_buf;
    u32 vpss_buf;
    long fsize;
    app_playback_ret_t pr;
    mpi_ret_t ret;
    mpi_stream_t stream;
    mpi_vdec_probe_info_t pic;
    mpi_vdec_chn_attr_t vdec_attr;
    mpi_vpss_grp_attr_t grp_attr;
    mpi_vpss_chn_attr_t chn_attr;
    mpi_vdec_chn_t vdec_chn = -1;
    mpi_vpss_grp_t vpss_grp = -1;
    mpi_vpss_chn_t vpss_chn = -1;
    mpi_chn_t vdec_src;
    mpi_chn_t vpss_dst;
    mpi_video_frame_info_t frame;
    u16 tw = 0;
    u16 th = 0;
    int i;
    int h_vdec = 0;
    int h_grp = 0;
    int h_chn = 0;
    int h_chn_en = 0;
    int h_vdec_start = 0;
    int h_bound = 0;

    if (!path || !path[0]) {
        return NULL;
    }
    if (!vd_thumb_decode_allowed()) {
        /* 非 IDLE 模式(录像/拍照/全屏回放)或 video_dec 模式占用期禁止缩略图解码:
         * 防双路 VDEC/VPSS/pmm 叠加(2026-09-05 真机: 回放叠加 ASSERT, 录像叠加内存不足) */
        log_info("vd_thumb: mode active(cur=%d), skip decode: %s\n",
                 app_get_current_mode_name(), path);
        return NULL;
    }
    free_base = (u32)memory_get_size(P_MEMORY_UNUSED) +
                (u32)memory_get_size(P_VLT_MEMORY_UNUSED);
    if (free_base < VIDEO_DEC_THUMB_FULL_READ_MAX + VTHUMB_EXTRA_BUDGET) {
        /* 修订5: 解码前 probe 空闲预算, 不足直接降级(整读最坏情况都放不下) */
        log_error("vd_thumb: low free mem, degrade: %s free=%u\n", path, free_base);
        return NULL;
    }

    memset(&frame, 0, sizeof(frame));
    if (type == VIDEO_DEC_TYPE_VIDEO) {
        /* AVI(MJPG) 首帧封面(阶段二): 取首个有效视频帧的 JPEG 字节 */
        if (vd_thumb_avi_first_jpg(path, &jpg, &jpg_len) != 0) {
            return NULL;                         /* 打不开/非MJPG/超限: 保持占位 */
        }
    } else {
        /* 照片: EXIF 内嵌缩略图优先(接口契约为 malloc 缓冲, 固定用 free 释放) */
        pr = app_playback_read_embedded_thumb(path, &jpg, &jpg_len);
        if (pr == APP_PLAYBACK_OK) {
            jpg_exif = 1;
        } else if (pr == APP_PLAYBACK_ERR_NOT_FOUND) {
            /* 无内嵌缩略图: fopen/flen 顺序整读(参照 app_playback_image.c 整读范式) */
            fp = fopen(path, "r");
            if (!fp) {
                log_error("vd_thumb: open fail: %s\n", path);
                return NULL;
            }
            fsize = flen(fp);
            if (fsize <= 0 || (u32)fsize < 8) {
                fclose(fp);
                return NULL;
            }
            /* 修订3: 整读硬顶 128KB, 不按 1/4 空闲上浮; 超限直接降级 */
            if ((u32)fsize > VIDEO_DEC_THUMB_FULL_READ_MAX) {
                log_error("vd_thumb: jpg too large reject: %s len=%ld\n", path, fsize);
                fclose(fp);
                return NULL;
            }
            jpg = (u8 *)phy_malloc((u32)fsize);   // 必须走物理池: 大图(1.3~1.6MB)走堆会把池擦掉
            if (!jpg) {
                fclose(fp);
                return NULL;
            }
            if (fread(jpg, 1, (u32)fsize, fp) != (u32)fsize) {
                phy_free(jpg);
                fclose(fp);
                return NULL;
            }
            fclose(fp);
            fp = NULL;
            jpg_len = (u32)fsize;
        } else {
            log_error("vd_thumb: read embedded thumb ret=%d: %s\n", (int)pr, path);
            return NULL;
        }
    }
    if (jpg_len < 8 || jpg_len > VIDEO_DEC_THUMB_FULL_READ_MAX) {
        goto out;
    }

    memset(&stream, 0, sizeof(stream));
    stream.addr = jpg;
    stream.len = jpg_len;
    memset(&pic, 0, sizeof(pic));
    ret = mpi_vdec_probe_pic_info(&stream, &pic);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: probe fail ret=%d: %s\n", ret, path);
        goto out;
    }
    if (!pic.align_width || !pic.align_height || pic.width < 2 || pic.height < 2 ||
        pic.align_width > 4096 || pic.align_height > 4096) {
        log_error("vd_thumb: bad dims %dx%d: %s\n", (int)pic.width, (int)pic.height, path);
        goto out;
    }
    /* VDEC LINE 缓冲按维数核算(align_w x 32 行 + 1024, 同 playback 公式) */
    vdec_buf = vd_thumb_frame_size(pic.align_width, VTHUMB_VDEC_CACHE_LINES,
                                   pic.format) + VTHUMB_VDEC_FRAME_EXTRA;
    if (vdec_buf > VTHUMB_LINE_BUF_MAX) {
        /* 超 640 宽源图(如 720P 照片)LINE 缓冲超预算: 拒绝解码降级, 不挤内存 */
        log_error("vd_thumb: line buf over budget reject: %s %dx%d buf=%u\n",
                  path, (int)pic.align_width, (int)pic.align_height, vdec_buf);
        goto out;
    }
    if (vd_thumb_calc_vpss_size(pic.width, pic.height, &tw, &th) != 0) {
        goto out;
    }
    vpss_buf = vd_thumb_frame_size(tw, th, MPI_PIXEL_FMT_YUV420SP) +
               VTHUMB_VPSS_BUF_EXTRA;

    ret = mpi_vdec_find_free_chn(&vdec_chn);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: no free VDEC chn: %s\n", path);
        goto out;
    }
    memset(&vdec_attr, 0, sizeof(vdec_attr));
    vdec_attr.hw_dev = VIDEO_DEC_VDEC_DEV;
    vdec_attr.type = MPI_VDEC_TYPE_JPEG;
    vdec_attr.out_yuv_order = MPI_YUV_ORDER_YUYV;
    vdec_attr.output_mode = MPI_VDEC_OUTPUT_LINE;
    vdec_attr.line_cnt = VTHUMB_VDEC_LINE_CNT;
    vdec_attr.frame_buf_size = vdec_buf;
    ret = mpi_vdec_create_chn(vdec_chn, &vdec_attr);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: create VDEC chn fail ret=%d\n", ret);
        goto out;
    }
    h_vdec = 1;

    ret = mpi_vpss_find_free_grp(&vpss_grp);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: no free VPSS grp: %s\n", path);
        goto teardown;
    }
    memset(&grp_attr, 0, sizeof(grp_attr));
    grp_attr.src_width = pic.align_width;
    grp_attr.src_height = pic.align_height;
    grp_attr.src_format = MPI_VPSS_SRC_FMT_YUV;
    grp_attr.src_pixel_format = pic.format;
    grp_attr.src_yuv_order = MPI_YUV_ORDER_YUYV;
    ret = mpi_vpss_create_grp(vpss_grp, &grp_attr);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: create VPSS grp fail ret=%d\n", ret);
        goto teardown;
    }
    h_grp = 1;

    ret = mpi_vpss_start_grp(vpss_grp);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: start VPSS grp fail ret=%d\n", ret);
        goto teardown;
    }
    ret = mpi_vpss_find_free_chn(vpss_grp, &vpss_chn);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: no free VPSS chn\n");
        goto teardown;
    }
    memset(&chn_attr, 0, sizeof(chn_attr));
    chn_attr.width = tw;
    chn_attr.height = th;
    chn_attr.format = MPI_PIXEL_FMT_YUV420SP;
    chn_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    chn_attr.buf_size = vpss_buf;                 /* 离线单帧队列(不建 VO) */
    ret = mpi_vpss_create_chn(vpss_grp, vpss_chn, &chn_attr);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: create VPSS chn fail ret=%d\n", ret);
        goto teardown;
    }
    h_chn = 1;
    ret = mpi_vpss_enable_chn(vpss_grp, vpss_chn);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: enable VPSS chn fail ret=%d\n", ret);
        goto teardown;
    }
    h_chn_en = 1;

    ret = mpi_vdec_start_chn(vdec_chn);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: start VDEC chn fail ret=%d\n", ret);
        goto teardown;
    }
    h_vdec_start = 1;

    vd_thumb_fill_chn(MPI_MOD_ID_VDEC, vdec_attr.hw_dev, vdec_chn, &vdec_src);
    vd_thumb_fill_chn(MPI_MOD_ID_VPSS, vpss_grp, 0, &vpss_dst);
    ret = mpi_sys_bind(&vdec_src, &vpss_dst);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: bind VDEC->VPSS fail ret=%d\n", ret);
        goto teardown;
    }
    h_bound = 1;

    ret = mpi_vdec_send_stream(vdec_chn, &stream);
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: send stream fail ret=%d\n", ret);
        goto teardown;
    }
    log_info("vd_thumb: sent chn=%d %dx%d: %s\n", (int)vdec_chn,
             (int)pic.align_width, (int)pic.align_height, path);

    /* 离线下帧: 短超时轮询(500ms x4), 每轮查取消——即使驱动忽略超时参数,
     * 取消/异常也能在 2s 内退出, 保证 worker 线程可停(线程内部不可中断阻塞的根治) */
    ret = -1;    /* 非 MPI_SUCCESS 即失败; 用 -1 兜底(MPI 无通用 FAILED 码) */
    for (i = 0; i < 4; i++) {
        u8 cancelled;
        os_mutex_pend(&g_thumb.lock, 0);
        cancelled = g_thumb.cancel;
        os_mutex_post(&g_thumb.lock);
        if (cancelled) {
            log_info("vd_thumb: cancel during get_frame: %s\n", path);
            goto teardown;
        }
        ret = mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &frame, 500);
        if (ret == MPI_SUCCESS) {
            break;
        }
        log_info("vd_thumb: get_frame retry(%d/4) ret=%d: %s\n", i + 1, ret, path);
    }
    if (ret != MPI_SUCCESS) {
        log_error("vd_thumb: get VPSS frame fail ret=%d: %s\n", ret, path);
        goto teardown;
    }
    tile = (u8 *)phy_malloc(VIDEO_DEC_THUMB_TILE_BYTES);   // 必须走物理池: kc(堆)版实测每轮擦掉池 ~185KB(2026-09-21 回退实验)
    if (!tile) {
        log_error("vd_thumb: tile malloc fail: %s\n", path);
        (void)mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frame);
        goto teardown;
    }
#if VIDEO_DEC_TILE_NV12_TEST
    vd_thumb_frame_to_nv12_tile(&frame, tile);
#else
    vd_thumb_yuv420sp_to_rgb565(&frame, tile);
#endif
    (void)mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frame);

teardown:
    /* 拆链按下游->上游全量执行(与 mpi_vdec_vpss.c 同序), 未创建项跳过 */
    if (h_bound) {
        (void)mpi_sys_unbind(&vdec_src, &vpss_dst);
    }
    if (h_vdec_start) {
        (void)mpi_vdec_stop_chn(vdec_chn);
    }
    if (h_chn_en) {
        (void)mpi_vpss_disable_chn(vpss_grp, vpss_chn);
    }
    if (h_chn) {
        (void)mpi_vpss_destroy_chn(vpss_grp, vpss_chn);
    }
    if (h_grp) {
        (void)mpi_vpss_stop_grp(vpss_grp);
        (void)mpi_vpss_destroy_grp(vpss_grp);
    }
    if (h_vdec) {
        (void)mpi_vdec_destroy_chn(vdec_chn);
    }
out:
    if (jpg) {
        if (jpg_exif) {
            free(jpg);          /* EXIF 接口契约: 调用方 free() */
        } else {
            phy_free(jpg);
        }
        jpg = NULL;
    }
    return tile;
}

/* ---- FAT 冷路径后台预热: 进多宫格/移动光标时请求, 后台延迟 300ms 预热"焦点文件" ----
 * 背景: 首次打开某文件的 FAT 冷路径要 ~2.4s(实测 FS 缓存填充 365 扇区 x 7.6ms),
 * 落在 video_dec_play 里 => 用户按 OK 后黑屏 2.4s。这里在用户浏览时就后台暖好。
 * 安全性: 只 fopen+fread(4KB)+fclose(与原有一次性预热同法); 不在 app_core 小栈线程打日志;
 * 延迟 300ms 让焦点稳定后才做, 避免快速翻页把 SD 读请求堆起来。 */
static volatile int g_warm_req_idx = -1;   /* 请求预热的 index; -1=无 */
static volatile u8  g_warm_stop    = 0;
static int          g_warm_pid     = 0;    /* 预热 worker 的 pid(0=未建) */

static void vd_fat_warm_worker(void *priv)
{
    (void)priv;
    while (!g_warm_stop) {
        int idx = g_warm_req_idx;
        if (idx < 0) {
            os_time_dly(5);                 /* 10ms 轮询 */
            continue;
        }
        g_warm_req_idx = -1;                /* 取走本次请求 */
        os_time_dly(30);                    /* 300ms: 焦点稳定才预热 */
        if (g_warm_req_idx >= 0) {
            continue;                       /* 期间又变了 => 丢弃本次 */
        }
        {
            flm_file_t fe;
            if (flm_get_entry(idx, &fe) == 0 && fe.path[0]) {
                FILE *f = fopen(fe.path, "r");
                if (f != NULL) {
                    static u8 wb[4096];
                    (void)fread(wb, 1, sizeof(wb), f);
                    fclose(f);
                }
            }
        }
    }
}

/* 供 UI 层调用: 请求预热某个 index 的文件(只置标志, 零栈开销) */
void video_dec_fat_warm_request(int idx)
{
    if (idx >= 0) {
        g_warm_req_idx = idx;
    }
}

/* worker 主循环: 取 WAIT 槽 -> 解码 -> 入 LRU -> ready_cb; 无活时挂起等唤醒 */
static void vd_thumb_worker(void *priv)
{
    (void)priv;

    for (;;) {
        char path[128];
        int gidx = -1;
        u8 ftype = VIDEO_DEC_TYPE_PHOTO;
        u8 *tile = NULL;
        int run = 0;
        int mode_wait = 0;
        video_dec_thumb_ready_cb_t cb = NULL;
        void *cb_priv = NULL;

        os_mutex_pend(&g_thumb.lock, 0);
        if (g_thumb.exit) {
            g_thumb.worker_pid = 0;      /* 自清理: 允许下次请求再起 worker */
            g_thumb.busy = 0;
            os_mutex_post(&g_thumb.lock);
            break;
        }
        if (g_thumb.cancel) {
            g_thumb.cancel = 0;                    /* ack: 本批次不再取槽 */
            os_mutex_post(&g_thumb.lock);
            os_sem_post(&g_thumb.done);
            continue;
        }
        if (!g_thumb.busy) {
            int i;
            for (i = 0; i < VIDEO_DEC_THUMB_SLOT_MAX; i++) {
                vd_thumb_slot_t *s = &g_thumb.slot[i];
                if (s->state == VTHUMB_SLOT_WAIT && s->gen == g_thumb.gen) {
                    if (!vd_thumb_decode_allowed()) {
                        /* 非 IDLE/模式占用期: 槽保留 WAIT 稍后重试——返回六宫格时
                         * 屏 LOAD 先于模式退出, 直接拒会永久空白(真机实况)。 */
                        mode_wait = 1;
                        break;
                    }
                    memcpy(path, s->path, sizeof(path));
                    gidx = s->gidx;
                    ftype = s->type;
                    s->state = VTHUMB_SLOT_WORK;
                    g_thumb.busy = 1;
                    run = 1;
                    break;
                }
            }
        }
        os_mutex_post(&g_thumb.lock);

        if (mode_wait) {
            static u16 mw_cnt = 0;
            if ((mw_cnt++ % 20) == 0) {
                log_info("vd_thumb: waiting mode off (slot held, %u)\n", mw_cnt / 20);
            }
            (void)os_sem_pend(&g_thumb.wake, 200); /* 定时重查, 等模式退出后解码 */
            continue;
        }

        if (run) {
            tile = vd_thumb_decode_tile(path, ftype);
            os_mutex_pend(&g_thumb.lock, 0);
            g_thumb.busy = 0;
            if (tile) {
                int i;
                int matched = -1;
                for (i = 0; i < VIDEO_DEC_THUMB_SLOT_MAX; i++) {
                    vd_thumb_slot_t *s = &g_thumb.slot[i];
                    if (s->state == VTHUMB_SLOT_WORK && s->gen == g_thumb.gen &&
                        s->gidx == gidx && !strcmp(s->path, path)) {
                        matched = i;
                        break;
                    }
                }
                if (matched >= 0 && !g_thumb.cancel) {
                    vd_thumb_lru_insert_locked(path, tile);
                    g_thumb.slot[matched].state = VTHUMB_SLOT_EMPTY;
                    cb = g_thumb.ready_cb;
                    cb_priv = g_thumb.ready_priv;
                    tile = NULL;                 /* 已入 LRU, 归引擎所有 */
                } else if (matched >= 0) {
                    /* 取消/失效窗口内完成的解码: 释放结果 */
                    g_thumb.slot[matched].state = VTHUMB_SLOT_EMPTY;
                    phy_free(tile);
                } else {
                    /* 槽已被新批次替换(翻页/重请求): 释放过期结果 */
                    phy_free(tile);
                }
            } else {
                int i;
                for (i = 0; i < VIDEO_DEC_THUMB_SLOT_MAX; i++) {
                    vd_thumb_slot_t *s = &g_thumb.slot[i];
                    if (s->state == VTHUMB_SLOT_WORK && s->gidx == gidx &&
                        !strcmp(s->path, path)) {
                        s->state = VTHUMB_SLOT_EMPTY;  /* 失败: UI 保持现占位 */
                        break;
                    }
                }
            }
            os_mutex_post(&g_thumb.lock);
            os_sem_post(&g_thumb.done);
            if (cb) {
                cb(cb_priv, path, gidx);
            }
            continue;
        }
        (void)os_sem_pend(&g_thumb.wake, 0);      /* 空快照: 等 request/唤醒 */
    }
}

/* 页请求: UI 线程调用; 快照 [base, base+n) 照片槽(flm 取条目只拷 path/type) */
int video_dec_thumb_request(int base, int n)
{
    int i;
    int pid = 0;

    vd_thumb_engine_init();
    if (base < 0 || n <= 0) {
        return -1;
    }
    if (n > VIDEO_DEC_THUMB_SLOT_MAX) {
        n = VIDEO_DEC_THUMB_SLOT_MAX;
    }

    os_mutex_pend(&g_thumb.lock, 0);
    if (!g_thumb.worker_pid) {
        os_mutex_post(&g_thumb.lock);
        if (thread_fork("vd_thumb_worker", VIDEO_DEC_THUMB_WORKER_PRIO,
                        VIDEO_DEC_THUMB_WORKER_STACK, 0, &pid,
                        vd_thumb_worker, NULL) != OS_NO_ERR) {
            log_error("vd_thumb: worker fork fail\n");
            return -1;
        }
        os_mutex_pend(&g_thumb.lock, 0);
        g_thumb.worker_pid = pid;
    }
    vd_thumb_slot_clear_locked();                /* 新批次作废旧快照(含在飞 WORK 槽) */
    for (i = 0; i < n; i++) {
        flm_file_t fe;
        int gidx = base + i;
        vd_thumb_slot_t *s = &g_thumb.slot[i];

        if (flm_get_entry(gidx, &fe) != 0) {
            continue;                            /* 越界/刚删除: 留 EMPTY */
        }
        s->gidx = gidx;
        s->gen = g_thumb.gen;
        s->type = fe.type;
        if (fe.type == VIDEO_DEC_TYPE_PHOTO || fe.type == VIDEO_DEC_TYPE_VIDEO) {
            /* 照片整读解码 / AVI 首帧封面(阶段二) 都走 WAIT; LRU 命中跳过 */
            if (vd_thumb_lru_find_locked(fe.path) < 0) {
                snprintf(s->path, sizeof(s->path), "%s", fe.path);
                s->state = VTHUMB_SLOT_WAIT;
            }
            /* 已缓存: 不重复解码(rebuild 的 peek 已挂图) */
        } else {
            s->state = VTHUMB_SLOT_SKIP;         /* 未知类型: 占位 */
        }
    }
    os_mutex_post(&g_thumb.lock);
    os_sem_post(&g_thumb.wake);
    return 0;
}

/* LRU 命中返回 tile(96x64 RGB565)并 touch 队首; 未命中返回 NULL */
const void *video_dec_thumb_peek(const char *path)
{
    u8 *tile;

    vd_thumb_engine_init();
    if (!path || !path[0]) {
        return NULL;
    }
    os_mutex_pend(&g_thumb.lock, 0);
    tile = vd_thumb_lru_touch_locked(path);
    os_mutex_post(&g_thumb.lock);
    return tile;
}

void video_dec_thumb_set_ready_cb(video_dec_thumb_ready_cb_t cb, void *priv)
{
    vd_thumb_engine_init();
    os_mutex_pend(&g_thumb.lock, 0);
    g_thumb.ready_cb = cb;
    g_thumb.ready_priv = priv;
    os_mutex_post(&g_thumb.lock);
}

/* 异步取消: UI 线程路径用(点格/全屏 LOAD)——置取消+清 WAIT 槽+唤醒即返回, 不等待;
 * worker 解完当前张或空闲时自行 ack; VDEC 互斥由 mode 侧开回放前的同步等待保证。 */
void video_dec_thumb_cancel_async(void)
{
    int i;

    vd_thumb_engine_init();
    os_mutex_pend(&g_thumb.lock, 0);
    g_thumb.cancel = 1;
    for (i = 0; i < VIDEO_DEC_THUMB_SLOT_MAX; i++) {
        if (g_thumb.slot[i].state == VTHUMB_SLOT_WAIT) {
            g_thumb.slot[i].state = VTHUMB_SLOT_EMPTY;
        }
    }
    os_mutex_post(&g_thumb.lock);
    os_sem_post(&g_thumb.wake);
}

/* 置取消并等当前张解码结束(单张有界); 未处理快照作废, 之后需重新 request */
int video_dec_thumb_cancel_wait(void)
{
    int i;

    vd_thumb_engine_init();
    os_mutex_pend(&g_thumb.lock, 0);
    if (!g_thumb.worker_pid && !g_thumb.busy) {
        /* worker 未起/无在飞: 清残留快照即可 */
        vd_thumb_slot_clear_locked();
        g_thumb.cancel = 0;
        os_mutex_post(&g_thumb.lock);
        return 0;
    }
    g_thumb.cancel = 1;
    for (i = 0; i < VIDEO_DEC_THUMB_SLOT_MAX; i++) {
        if (g_thumb.slot[i].state == VTHUMB_SLOT_WAIT) {
            g_thumb.slot[i].state = VTHUMB_SLOT_EMPTY;
        }
    }
    os_mutex_post(&g_thumb.lock);
    os_sem_post(&g_thumb.wake);                  /* 唤醒 idle worker 处理 ack */
    for (i = 0; i < 40; i++) {                   /* 上界约 2s: 单张解码有界(取帧带超时) */
        u8 idle;
        os_mutex_pend(&g_thumb.lock, 0);
        idle = (g_thumb.cancel == 0) && (g_thumb.busy == 0);
        os_mutex_post(&g_thumb.lock);
        if (idle) {
            break;
        }
        (void)os_sem_pend(&g_thumb.done, 50);
    }
    os_mutex_pend(&g_thumb.lock, 0);
    if (g_thumb.cancel || g_thumb.busy) {
        /* 超时兜底(2026-09-05 真机): 不再 thread_kill(KILL_WAIT)——其语义闭源不可控,
         * worker 若卡在 mpi 调用内会永久等, 调用方(UI/mode)随之卡死。改为放弃旧 worker:
         * gen++ 作废其在飞结果 + 清 worker_pid 允许下次 request 重建新 worker
         * (旧线程即使卡死也不阻塞任何路径; 卡死点由 vd_thumb 分步日志定位)。 */
        log_error("vd_thumb: cancel_wait timeout(2s) busy=%d pid=%d, abandon worker\n",
                  g_thumb.busy, g_thumb.worker_pid);
        g_thumb.gen++;
        g_thumb.worker_pid = 0;
        g_thumb.busy = 0;
        g_thumb.cancel = 0;
    }
    g_thumb.cancel = 0;
    vd_thumb_slot_clear_locked();
    os_mutex_post(&g_thumb.lock);
    return 0;
}

/* 失效: 代际号++ + 清快照 + LRU 全部释放归零(删除确认/拔卡/DB_CHANGED/重进页) */
void video_dec_thumb_invalidate(void)
{
    int i;

    vd_thumb_engine_init();
    os_mutex_pend(&g_thumb.lock, 0);
    g_thumb.gen++;                               /* 在飞解码结果按代际作废 */
    vd_thumb_slot_clear_locked();
    for (i = 0; i < g_thumb.lru_count; i++) {
        if (g_thumb.lru[i].tile) {
            phy_free(g_thumb.lru[i].tile);
            g_thumb.lru[i].tile = NULL;
        }
        g_thumb.lru[i].path[0] = '\0';
    }
    g_thumb.lru_count = 0;
    os_mutex_post(&g_thumb.lock);
}

/* 全屏回放模式是否激活(激活期缩略图解码被禁); UI 侧补发请求前查询 */
int video_dec_thumb_mode_active(void)
{
    return g_vd.mode_running ? 1 : 0;
}

/* 离开六宫格页全释放: 停 worker + 清 LRU/快照/回调, 无全局常驻 */
void video_dec_thumb_release_all(void)
{
    int i;

    /* 无等待离开页(2026-09-05 真机修复): 不等 worker、不 thread_kill——KILL_WAIT 语义
     * 闭源不可控, worker 卡 mpi 调用时会永久阻塞调用方(UI 冻结在页面的根因候选)。
     * 屏已切走不可见: 置 cancel+清 WAIT 槽+gen 作废+释放 LRU tile(内存立还);
     * 在飞解码结果由 worker 按 cancel/代际自释放, worker 空闲睡 wake 待下次复用。 */
    vd_thumb_engine_init();
    os_mutex_pend(&g_thumb.lock, 0);
    g_thumb.cancel = 1;
    vd_thumb_slot_clear_locked();
    g_thumb.gen++;
    for (i = 0; i < g_thumb.lru_count; i++) {
        if (g_thumb.lru[i].tile) {
            phy_free(g_thumb.lru[i].tile);
            g_thumb.lru[i].tile = NULL;
        }
        g_thumb.lru[i].path[0] = '\0';
    }
    g_thumb.lru_count = 0;
    g_thumb.ready_cb = NULL;
    g_thumb.ready_priv = NULL;
    os_mutex_post(&g_thumb.lock);
    os_sem_post(&g_thumb.wake);    /* 唤醒睡中的 worker 让它 ack cancel(空转无害) */
}

int video_dec_generate_thumbnail(video_dec_file_t *file)
{
    /* 同步解码入口(保留供模式内 LIST UI/未来调用方): 照片整读 / AVI(MJPG)首帧;
     * 任何失败: 不分配直接返回非 0, 调用方保持占位。 */
    if (!file || (file->type != VIDEO_DEC_TYPE_PHOTO && file->type != VIDEO_DEC_TYPE_VIDEO)) {
        return -1;
    }
    file->thumbnail = vd_thumb_decode_tile(file->path, file->type);
    return file->thumbnail ? 0 : -1;
}
int video_dec_get_storage_info(video_dec_storage_t *info)
{
    if (flm_sd_online() != 0) {
        return -1;
    }
    return flm_get_storage_info(
               info ? &info->total_kb : NULL,
               info ? &info->free_kb : NULL,
               NULL,   /* photo/video_count 全树无读者, 跳过逐条遍历 */
               NULL);
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
/* 峰值前移: scan 前先把骨架的堆成本取走(fb5 两块 115200 + 帧队列 128KB), 全有或全无;
 * 只入池不建通道, 首播 pipeline_open/fq_open 命中池块 ⇒ 不重复建、不再抢连续块。
 * 任一步失败即回滚已入池的槽, 由调用方降级(不自动播, 停 LIST), 不许崩。 */
static int vd_prebuild_pipeline(void)
{
    int vo_ready = vd_fb_pool_prime();
    int fq_ok = 0;

    if (vo_ready == VDEC_FB_POOL_SLOTS) {
        fq_ok = (app_playback_demux_frame_queue_prealloc(VDEC_VIDEO_BUF_SIZE) == 0);
    }
    if (vo_ready != VDEC_FB_POOL_SLOTS || !fq_ok) {
        vd_fb_pool_unprime();
        log_info("[VDEC] prebuild pipeline: fail (fq=%d vo=%d)\n", fq_ok, vo_ready);
        return -1;
    }

    /* 预暖堆: AVI 帧索引(~189KB)是回放期堆的第一个大分配, 直接视频时它会当场从物理池
     * 扩页(实测 -189KB)导致起播内存不足; 照片先解码再切视频则正常(索引复用既有堆空间)。
     * 这里趁池子最干净先付掉这笔切痕, 之后索引分配不再动物理池。 */
    {
        void *warm = kc_malloc(200u * 1024u);
        if (warm != NULL) {
            kc_free(warm);
            log_info("[PMM] heap warm 200KB ok\n");
        }
    }
    log_info("[VDEC] prebuild pipeline: ok (fq=%d vo=%d)\n", fq_ok, vo_ready);
    return 0;
}
/*==============================================================================
 * Exit async release (app_core 栈外执行退出尾部两步)
 *==============================================================================*/
/* video_dec_exit 尾部的静音恢复与文件释放不改变退出链顺序语义, 但会叠加调用深度;
 * 移交短命线程执行, 退出路径不再吃 app_core 栈。所有权在 start 里同步摘除,
 * worker 只碰摘下来的旧指针, 不读不写 g_vd; 下次 video_dec_init 前 wait 保证完成 */
static struct {
    OS_MUTEX lock;
    OS_SEM   done;                   /* worker 完成通知 */
    u8   init;
    u8   busy;                       /* worker 在飞 */
    u8   waiting;                    /* app_core 正在等 worker 完成 */
    u8 **files;                      /* 已从 g_vd 摘除的 thumb_data */
    int  files_count;
    u8   mute_on;
    int  pid;
} g_vd_exit_async;

static void vd_exit_async_init(void)
{
    if (g_vd_exit_async.init) {
        return;
    }
    os_mutex_create(&g_vd_exit_async.lock);
    os_sem_create(&g_vd_exit_async.done, 0);
    g_vd_exit_async.init = 1;
}

static void vd_exit_async_worker(void *arg)
{
    u8 **files;
    int count;
    u8 mute_on;

    (void)arg;
    os_mutex_pend(&g_vd_exit_async.lock, 0);
    files = g_vd_exit_async.files;
    count = g_vd_exit_async.files_count;
    mute_on = g_vd_exit_async.mute_on;
    g_vd_exit_async.files = NULL;
    g_vd_exit_async.files_count = 0;
    os_mutex_post(&g_vd_exit_async.lock);

    vd_mute_restore_apply(mute_on);
    vd_free_files_ptr(files, count);

    os_mutex_pend(&g_vd_exit_async.lock, 0);
    g_vd_exit_async.busy = 0;
    g_vd_exit_async.pid = 0;
    if (g_vd_exit_async.waiting) {
        g_vd_exit_async.waiting = 0;
        os_sem_post(&g_vd_exit_async.done);
    }
    os_mutex_post(&g_vd_exit_async.lock);
}

/* 等上一次退出的异步释放完成(单轮上限 2s); 超时只留痕不阻塞: 旧数组所有权已摘除,
 * 与下一次进模式重新分配的数组无别名, 内存安全不依赖等待成功 */
static void vd_exit_async_wait(void)
{
    if (!g_vd_exit_async.init) {
        return;
    }
    for (;;) {
        os_mutex_pend(&g_vd_exit_async.lock, 0);
        if (!g_vd_exit_async.busy) {
            os_mutex_post(&g_vd_exit_async.lock);
            return;
        }
        g_vd_exit_async.waiting = 1;
        os_mutex_post(&g_vd_exit_async.lock);
        if (os_sem_pend(&g_vd_exit_async.done, 200) != OS_NO_ERR) {
            log_error("[VDEC] exit async: wait timeout\n");
            return;
        }
        /* 上一轮超时遗留的 post 会立即唤醒: 循环复查 busy, 未完成则继续等 */
    }
}

static void vd_exit_async_start(void)
{
    u8 **files;
    int count;
    u8 mute_on;
    int pid = 0;

    vd_exit_async_init();
    vd_exit_async_wait();            /* 上一轮未完成则先等, 防同名线程并发 */
    mute_on = g_vd.mute_before_enter;
    os_mutex_pend(&g_vd_exit_async.lock, 0);
    files = g_vd.thumb_data;         /* 所有权移交: 摘除后 g_vd 不再引用 */
    count = g_vd.thumb_count;
    g_vd.thumb_data = NULL;
    g_vd.thumb_count = 0;
    g_vd.thumb_lru_count = 0;
    g_vd.muted = mute_on;
    if (g_vd_exit_async.busy) {      /* 上一次仍未被调度: 本轮回落同步, 不并发 */
        os_mutex_post(&g_vd_exit_async.lock);
        vd_mute_restore_apply(mute_on);
        vd_free_files_ptr(files, count);
        log_info("[VDEC] exit async: fork=0\n");
        return;
    }
    g_vd_exit_async.files = files;
    g_vd_exit_async.files_count = count;
    g_vd_exit_async.mute_on = mute_on;
    g_vd_exit_async.busy = 1;
    os_mutex_post(&g_vd_exit_async.lock);

    if (thread_fork("vd_exit_async", VIDEO_DEC_THUMB_WORKER_PRIO, 2048, 0, &pid,
                    vd_exit_async_worker, NULL) != OS_NO_ERR) {
        os_mutex_pend(&g_vd_exit_async.lock, 0);
        files = g_vd_exit_async.files;
        count = g_vd_exit_async.files_count;
        g_vd_exit_async.files = NULL;
        g_vd_exit_async.files_count = 0;
        g_vd_exit_async.busy = 0;
        os_mutex_post(&g_vd_exit_async.lock);
        vd_mute_restore_apply(mute_on);   /* fork 失败回落同步, 不静默不释放 */
        vd_free_files_ptr(files, count);
        log_info("[VDEC] exit async: fork=0\n");
        return;
    }
    g_vd_exit_async.pid = pid;
    log_info("[VDEC] exit async: fork=1\n");
}
/* 起播判据在 FAT 预暖【之前】定死: 预暖是 fopen/fread/fclose 深调用链, 跑在 app_core 小栈上,
 * 实测(2026-09-21)偶发"跑过预暖后这次不起播", 且加打印即不复现(Heisenbug) —— 故不依赖预暖后的局部量 */
static volatile int g_autoplay_pending;
int video_dec_init(void)
{
    /* FAT 后台预热 worker: 模式内常驻(进多宫格/移光标时由 UI 层投请求) */
    if (!g_warm_pid) {
        g_warm_stop = 0;
        if (thread_fork("vd_fat_warm", VIDEO_DEC_THUMB_WORKER_PRIO, 1024, 0, &g_warm_pid,
                        vd_fat_warm_worker, NULL) != OS_NO_ERR) {
            g_warm_pid = 0;
        }
    }
    log_info("video_dec_init\n");
    vd_exit_async_wait();            /* 上次退出的异步释放须先完成 */
    video_dec_thumb_cancel_wait();   /* 进全屏回放前停缩略图 worker(双保险) */
    g_vd.mode_running = 1;           /* 提前置位: init 起即禁缩略图解码(VDEC 占用期) */
    memset(&g_vd, 0, sizeof(g_vd));
    g_vd.mode_running = 1;           /* memset 复位后重置 */
    g_vd.nav_dir = 1;                /* 默认方向: 下一个 */
    g_vd.mute_before_enter = app_audio_get_dac_digital_mute();
    g_vd_fb_pool.enable = 1;         /* fb5 在 video_dec_ui_init 创建, 置位须在它之前 */
    if (vd_open_playback() != 0) {
        log_error("video_dec_init: vd_open_playback failed\n");
        g_vd.mode_running = 0;       /* 失败回滚: 允许缩略图继续(六宫格场景) */
        vd_fb_pool_release();        /* 失败回滚: 清标志并释放池 */
        return -1;
    }
    video_dec_ui_init(VDEC_PB_WIDTH, VDEC_PB_HEIGHT);
    /* 碎片整理: 实测出现"池子 808KB 空闲却拿不到 128KB 连续块"(physics memory size 828160
     * 而 prebuild fail fq=0) => 只分配了 ~30KB 却把 808KB 戳碎, 唯一解释是堆(malloc 按单页
     * 增长)的页零散散布在池里。memory_defrag() 正是搬 vmem/堆页的整理器(全树此前零调用),
     * 故在预建前调一次, 把堆页收拢, 让连续块重新可用。 */
    {
        u32 td0 = sys_timer_get_ms();
        int dfr = memory_defrag();
        log_info("[VDEC] memory_defrag before prebuild: ret=%d took=%ums\n",
                 dfr, (unsigned)(sys_timer_get_ms() - td0));
    }
    /* 峰值前移: 此刻池子最干净(缩略图刚释放), 预建失败则不自动播 */
    int prebuild_ok = (vd_prebuild_pipeline() == 0);
    /* 建全库名字缓存(87.8KB): 必须在预建【之后】(放前面会挤掉预建 => prebuild_ok=0 => 不自动播)。
     * 无缓存时 lru_miss 会走全库遍历的慢路径(long 名目录项逐条 zalloc), 卡在 FAT 上会导致
     * 看门狗超时死机; 开着缓存则回放全程不遍历, P_MEM=0。
     * 代价: 建立时的一次遍历把最大连续块从 196608 压到 65536 —— 实测可接受, 优先保不死机。 */
    flm_names_cache_prepare();
    vd_scan_and_load();
    /* 进回放模式自动解码显示第一个文件(用户无需按OK)。LIST态只有文件名文本无
     * 缩略图渲染, 停在LIST=白屏; 有文件则直接全屏解码取首张(照片/视频)。
     * 相册列表点了具体某格进来时, 用 s_pending_start_index 指定起始文件。 */
    log_info("[VDEC][MEMDBG] auto_play_check count=%d pending=%d from_grid=%d\n",
             flm_get_count(), s_pending_start_index, (int)s_entered_from_grid);
    int start = 0;
    if (s_pending_start_index >= 0) {
        start = s_pending_start_index;
        s_pending_start_index = -1;
    }
    if (start < 0 || start >= flm_get_count()) {
        start = 0;
    }
    g_autoplay_pending = (prebuild_ok && flm_get_count() > 0) ? 1 : 0;
    log_info("[VDEC][MEMDBG] auto_play_gate: prebuild_ok=%d count=%d start=%d pending=%d\n",
             prebuild_ok, flm_get_count(), start, (int)g_autoplay_pending);
    /* FAT 预暖(必须保留): 打开 AVI 时 dir_find 扫 1400+ 长名目录项每项 zalloc(512) 累积 ~188KB, 不预暖
     * 这笔钱留到播放时(池子只剩 ~460KB)付会打穿池子([P_MEM] 风暴+断言); 判据是内存水位不是 play 耗时。 */
    {
        static u8 g_fat_warmed = 0;
        if (!g_fat_warmed && prebuild_ok && flm_get_count() > 0) {
            flm_file_t we;
            if (flm_get_entry(start, &we) == 0 && we.path[0]) {
                FILE *wf = fopen(we.path, "r");
                if (wf != NULL) {
                    static u8 wbuf[4096];
                    (void)fread(wbuf, 1, sizeof(wbuf), wf);
                    fclose(wf);
                    g_fat_warmed = 1;
                    log_info("[VDEC] fat warm: %s\n", we.path);
                }
            }
        }
    }
    if (g_autoplay_pending) {
        video_dec_play(start);
        log_info("[VDEC][MEMDBG] auto_play_done start=%d\n", start);
    }
    return 0;
}
/* 模式退出专用清理(2026-09-05 真机): 释放 LRU tile 内存, 但保留 WAIT 请求槽与代际——
 * 返回六宫格时屏 LOAD 先于模式退出, 请求不能被退出流程作废(cancel_wait/invalidate
 * 都会清槽, 是"返回后永不补图"的根因); worker 在 mode_running=0 后自动取槽解码。 */
static void video_dec_thumb_mode_exit_clean(void)
{
    int i;

    vd_thumb_engine_init();
    os_mutex_pend(&g_thumb.lock, 0);
    for (i = 0; i < g_thumb.lru_count; i++) {
        if (g_thumb.lru[i].tile) {
            phy_free(g_thumb.lru[i].tile);
            g_thumb.lru[i].tile = NULL;
        }
        g_thumb.lru[i].path[0] = '\0';
    }
    g_thumb.lru_count = 0;
    os_mutex_post(&g_thumb.lock);
}

void video_dec_exit(void)
{
    /* 停 FAT 预热 worker(不 kill, 置标志让它自己退): 与缩略图 worker 同策略 */
    g_warm_stop = 1;
    g_warm_req_idx = -1;
    log_info("video_dec_exit enter, mode_running=%d\n", g_vd.mode_running);
    /* 只清 LRU、保留等待槽(见 video_dec_thumb_mode_exit_clean 注释); worker 由
     * mode_running 门控不会在退出期间解码, 无需 cancel_wait */
    video_dec_thumb_mode_exit_clean();
    /* 先撤 LVGL 视频 img 并排空异步渲染(dev_close fb5), 再停 VO/回放通路。
     * 对齐 photo_exit 顺序(photo_ui_deinit 先于 app_camera_display_stop):
     * fb5 由回放 VO(写)与 LVGL 视频 img(读)共享, 先撤 LVGL 侧、排空在飞的
     * LIST==2 异步渲染, 再停 VO 拆通路, 避免 VO 停时 fb5 buffer 归还/释放与
     * 异步渲染竞态(快速退出再进入时 "no available buffer" / dma_video0 mmu 异常)。 */
    video_dec_ui_deinit();

    if (g_vd.pb_handle) {
        app_playback_video_stop(g_vd.pb_handle);
    }
    g_vd.video_playing = 0;
    g_vd.video_paused = 0;
    video_dec_ui_close_video();
    vd_close_photo_if_showing();
    video_dec_ui_time_hide();
    vd_close_playback();
    vd_fb_pool_release();            /* fb5 已 close, 固定缓冲还回物理池(录像需要) */
    app_playback_demux_frame_queue_pool_release();   /* 帧队列常驻块(128KB)也还回物理池: 录像不需要它 */
    /* 注意: 这里【不】释放全库名字缓存(87.8KB)。它跨"多宫格 ↔ 全屏"保持, 避免每次从多宫格
     * 点开一个文件都重建(实测重建 2.3s = FS 缓存冷填充 365 扇区)。释放点改在
     * app_enter_video_rec_mode()(进录像等吃内存模式时还), 保证不影响其他模式。 */
    /* 静音恢复+文件释放移交短命线程(顺序语义不变, 只是移出 app_core 栈);
     * 线程创建失败或上轮未完成则内部回落同步执行 */
    vd_exit_async_start();
    g_vd.mode_running = 0;
    g_vd.state = VIDEO_DEC_STATE_IDLE;
    log_info("video_dec_exit done, mode off\n");
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
    /* arg=index+1 (相册列表指定起始文件, 0=不指定) -> 供 video_dec_init 起始播放 */
    s_pending_start_index = (arg >= 1) ? (arg - 1) : -1;
    s_entered_from_grid = (arg >= 1);
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
    /* 框架可靠退出钩子(2026-09-05 真机): 模式任务循环常收不到切换消息而漏跑
     * video_dec_exit → mode_running 残留 1 → 缩略图 worker 永久门控(返回六宫格后
     * 永不补图, 需经录像/拍照模式才恢复的实况)。此处清 mode_running 解除门控:
     * 等待中的请求槽保留, worker 随即补解; video_dec_exit 若随后执行其清理幂等。 */
    g_vd.mode_running = 0;
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

