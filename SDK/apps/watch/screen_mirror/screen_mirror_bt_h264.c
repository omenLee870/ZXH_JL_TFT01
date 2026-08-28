#include "app_config.h"

#if TCFG_BT_NET_ENABLE || TCFG_EXT_WIFI_ENABLE || TCFG_UART_NET_ENABLE || TCFG_SPI_NET_ENABLE || TCFG_LTE_PHY_ENABLE || TCFG_RTL8733BS_ENABLE
#include "system/includes.h"
#include "stream_media_core.h"
#include "video_1_decode_api.h"
#include "lvgl.h"
#include "screen_mirror.h"
#include "screen_mirror_bt_h264.h"
#include "map_tiles.h"

#define LOG_TAG_CONST       SCREEN_MIRROR_BT_H264
#define LOG_TAG             "[SCREEN_MIRROR_BT_H264]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#if TCFG_BT_H264_SCR_ENABLE

extern void wdt_clear(void);
u32 timer_get_ms(void);
int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);

#define BT_H264_TASK_NAME       "map_bt_h264"
#define BT_H264_DEC_RAM_PSRAM   1u
#define BT_H264_NV12_BUF_NUM    2     /* NV12缓存，PSRAM不足时可改为1 */

#ifndef BT_H264_DECODE_BENCHMARK      /* 蓝牙数据接收 + 解码效率测试*/
#define BT_H264_DECODE_BENCHMARK 0
#endif

#define __ALIGN_16(a)   (((a) + 15) / 16 * 16)
#define BT_H264_DISPLAY_WIDTH   __ALIGN_16(1024)
#define BT_H264_DISPLAY_HEIGHT  __ALIGN_16(600)

#if TCFG_PSRAM_DEV_ENABLE
#define BT_H264_BIG_MALLOC(size)    malloc_psram(size)
#define BT_H264_BIG_FREE(ptr)       free_psram(ptr)
#else
#define BT_H264_BIG_MALLOC(size)    malloc(size)
#define BT_H264_BIG_FREE(ptr)       free(ptr)
#endif

enum {
    Q_BT_H264_DATA = Q_USER + 131,
    Q_BT_H264_KILL,
};

#if BT_H264_DECODE_BENCHMARK
enum bt_h264_frame_type {
    BT_H264_FRAME_UNKNOWN,
    BT_H264_FRAME_I,
    BT_H264_FRAME_P,
};
#endif

struct bt_h264_msg {
    u32 frame_size;
    u32 timestamp;
    u16 crc;
    u16 seq;
    u32 offset;
    u16 payload_len;
    u8 is_last;
    u8 payload[0];
};

struct bt_h264_nv12_slot {
    u8 *buf;
    u32 capacity;
    u32 width;
    u32 height;
    lv_image_dsc_t image_dsc;
    u8 busy;
};

struct screen_mirror_bt_h264_display {
    OS_MUTEX mutex;
    u8 mutex_inited;
    lv_obj_t *obj;
    lv_display_t *lv_disp;
    struct bt_h264_nv12_slot slots[BT_H264_NV12_BUF_NUM];
    struct bt_h264_nv12_slot *displayed;
    struct bt_h264_nv12_slot *rendering;
    struct bt_h264_nv12_slot *pending;
    u8 show_posted;
};

struct screen_mirror_bt_h264 {
    volatile u8 running;
    volatile u8 task_created;
    u8 decoder_inited;
    u8 format;
    u16 width;
    u16 height;
    u8 fps;

    void *dec_storage;
    u8 *stream_buf;
    u32 stream_capacity;
    u32 stream_len;
    u32 frame_size;
    u16 frame_crc;
    u16 frame_seq;
    u32 decoded_frames;
    volatile u32 received_frames;
    volatile u32 displayed_frames;
    volatile u32 display_drop_frames;
    u32 received_bytes;
    u32 last_frame_size;
    u32 stat_last_ms;
    u32 stat_last_received_frames;
    u32 stat_last_decoded_frames;
    u32 stat_last_displayed_frames;
    u32 stat_last_display_drop_frames;
    u32 stat_last_received_bytes;
#if BT_H264_DECODE_BENCHMARK
    u32 bench_i_frames;
    u32 bench_p_frames;
    u32 bench_other_frames;
    u64 bench_i_total_ms;
    u64 bench_p_total_ms;
    u32 bench_i_max_ms;
    u32 bench_p_max_ms;
#endif
    u32 decoder_psram_alloc_bytes;
    u32 decoder_sram_alloc_bytes;
};

static struct screen_mirror_bt_h264 g_bt_h264;
static struct screen_mirror_bt_h264_display g_bt_h264_disp;

#if BT_H264_DECODE_BENCHMARK
static u32 screen_mirror_bt_h264_find_start_code(const u8 *data, u32 size,
        u32 offset, u32 *code_len)
{
    u32 i;

    for (i = offset; i + 2 < size; i++) {
        if (data[i] || data[i + 1]) {
            continue;
        }
        if (data[i + 2] == 1) {
            *code_len = 3;
            return i;
        }
        if (i + 3 < size && data[i + 2] == 0 && data[i + 3] == 1) {
            *code_len = 4;
            return i;
        }
    }
    *code_len = 0;
    return size;
}

static enum bt_h264_frame_type screen_mirror_bt_h264_get_frame_type(
    const u8 *data, u32 size)
{
    u32 offset = 0;

    while (offset < size) {
        u32 code_len;
        u32 start = screen_mirror_bt_h264_find_start_code(
                        data, size, offset, &code_len);
        u32 next_code_len;
        u32 nal_start;
        u32 nal_end;
        u8 nal_type;

        if (!code_len) {
            break;
        }
        nal_start = start + code_len;
        if (nal_start >= size) {
            break;
        }
        nal_end = screen_mirror_bt_h264_find_start_code(
                      data, size, nal_start, &next_code_len);
        nal_type = data[nal_start] & 0x1f;
        if (nal_type == 5) {
            return BT_H264_FRAME_I;
        }
        /*
         * The low-latency screen stream contains only IDR and non-IDR
         * slices (no B frames): NAL type 5 is I, type 1 is P.
         */
        if (nal_type == 1) {
            return BT_H264_FRAME_P;
        }
        if (!next_code_len) {
            break;
        }
        offset = nal_end;
    }
    return BT_H264_FRAME_UNKNOWN;
}

static const char *screen_mirror_bt_h264_frame_type_name(
    enum bt_h264_frame_type type)
{
    switch (type) {
    case BT_H264_FRAME_I:
        return "I";
    case BT_H264_FRAME_P:
        return "P";
    default:
        return "?";
    }
}

static void screen_mirror_bt_h264_bench_record(
    struct screen_mirror_bt_h264 *ctx,
    enum bt_h264_frame_type type, u16 seq, u32 size,
    u32 decode_ms, int output_frames)
{
    if (type == BT_H264_FRAME_I) {
        ctx->bench_i_frames++;
        ctx->bench_i_total_ms += decode_ms;
        if (decode_ms > ctx->bench_i_max_ms) {
            ctx->bench_i_max_ms = decode_ms;
        }
    } else if (type == BT_H264_FRAME_P) {
        ctx->bench_p_frames++;
        ctx->bench_p_total_ms += decode_ms;
        if (decode_ms > ctx->bench_p_max_ms) {
            ctx->bench_p_max_ms = decode_ms;
        }
    } else {
        ctx->bench_other_frames++;
    }

    log_info("decode seq=%u type=%s size=%lu time=%lu ms output=%d",
             seq, screen_mirror_bt_h264_frame_type_name(type),
             size, decode_ms, output_frames);
}
#endif

static void screen_mirror_bt_h264_stats_reset(
    struct screen_mirror_bt_h264 *ctx)
{
    ctx->received_frames = 0;
    ctx->decoded_frames = 0;
    ctx->displayed_frames = 0;
    ctx->display_drop_frames = 0;
    ctx->received_bytes = 0;
    ctx->last_frame_size = 0;
    ctx->stat_last_received_frames = 0;
    ctx->stat_last_decoded_frames = 0;
    ctx->stat_last_displayed_frames = 0;
    ctx->stat_last_display_drop_frames = 0;
    ctx->stat_last_received_bytes = 0;
#if BT_H264_DECODE_BENCHMARK
    ctx->bench_i_frames = 0;
    ctx->bench_p_frames = 0;
    ctx->bench_other_frames = 0;
    ctx->bench_i_total_ms = 0;
    ctx->bench_p_total_ms = 0;
    ctx->bench_i_max_ms = 0;
    ctx->bench_p_max_ms = 0;
#endif
    ctx->stat_last_ms = timer_get_ms();
}

static void screen_mirror_bt_h264_stats_on_received(
    struct screen_mirror_bt_h264 *ctx, u32 frame_size)
{
    ctx->received_frames++;
    ctx->received_bytes += frame_size;
    ctx->last_frame_size = frame_size;
}

static void screen_mirror_bt_h264_stats_report(
    struct screen_mirror_bt_h264 *ctx)
{
    u32 now = timer_get_ms();
    u32 elapsed = now - ctx->stat_last_ms;
    u32 recv_delta;
    u32 decode_delta;
#if !BT_H264_DECODE_BENCHMARK
    u32 display_delta;
    u32 display_drop_delta;
#endif
    u32 bytes_delta;
    u32 avg_size;

    if (elapsed < 1000) {
        return;
    }

    recv_delta = ctx->received_frames - ctx->stat_last_received_frames;
    decode_delta = ctx->decoded_frames - ctx->stat_last_decoded_frames;
#if !BT_H264_DECODE_BENCHMARK
    display_delta = ctx->displayed_frames - ctx->stat_last_displayed_frames;
    display_drop_delta =
        ctx->display_drop_frames - ctx->stat_last_display_drop_frames;
#endif
    bytes_delta = ctx->received_bytes - ctx->stat_last_received_bytes;
    avg_size = recv_delta ? bytes_delta / recv_delta : 0;

#if BT_H264_DECODE_BENCHMARK
    log_info("stat recv_fps=%lu decode_fps=%lu "
             "recv_size=%lu avg_size=%lu",
             recv_delta, decode_delta, ctx->last_frame_size, avg_size);
#else
    log_info("stat recv_fps=%lu decode_fps=%lu display_fps=%lu "
             "recv_size=%lu avg_size=%lu display_drop=%lu",
             recv_delta, decode_delta, display_delta,
             ctx->last_frame_size, avg_size, display_drop_delta);
#endif

    ctx->stat_last_ms = now;
    ctx->stat_last_received_frames = ctx->received_frames;
    ctx->stat_last_decoded_frames = ctx->decoded_frames;
#if !BT_H264_DECODE_BENCHMARK
    ctx->stat_last_displayed_frames = ctx->displayed_frames;
    ctx->stat_last_display_drop_frames = ctx->display_drop_frames;
#endif
    ctx->stat_last_received_bytes = ctx->received_bytes;
}

static void screen_mirror_bt_h264_display_init_once(void)
{
    if (g_bt_h264_disp.mutex_inited) {
        return;
    }

    os_mutex_create(&g_bt_h264_disp.mutex);
    g_bt_h264_disp.mutex_inited = 1;
}

static int screen_mirror_bt_h264_display_can_run(void)
{
    return g_bt_h264.running &&
           map_video_page_active() &&
           map_video_source_is(MAP_VIDEO_SOURCE_BT_H264);
}

static void screen_mirror_bt_h264_i420_to_nv12(u8 *nv12, const u8 *i420,
        u32 width, u32 height)
{
    u32 y_size = width * height;
    u32 chroma_size = y_size / 4;
    const u8 *u = i420 + y_size;
    const u8 *v = u + chroma_size;
    u8 *uv = nv12 + y_size;

    memcpy(nv12, i420, y_size);
    for (u32 i = 0; i < chroma_size; i++) {
        uv[2 * i] = u[i];
        uv[2 * i + 1] = v[i];
    }
}

static void screen_mirror_bt_h264_lvgl_present_pending(void);

static void screen_mirror_bt_h264_lvgl_present_pending_async(void *priv)
{
    (void)priv;
    screen_mirror_bt_h264_lvgl_present_pending();
}

static void screen_mirror_bt_h264_render_ready_event_cb(lv_event_t *e)
{
    struct bt_h264_nv12_slot *slot;
    struct bt_h264_nv12_slot *old_slot;
    u8 defer_present = 0;

    (void)e;
    os_mutex_pend(&g_bt_h264_disp.mutex, 0);
    slot = g_bt_h264_disp.rendering;
    if (!slot) {
        os_mutex_post(&g_bt_h264_disp.mutex);
        return;
    }

    g_bt_h264_disp.rendering = NULL;
    old_slot = g_bt_h264_disp.displayed;
    g_bt_h264_disp.displayed = slot;
    g_bt_h264.displayed_frames++;
    if (old_slot && old_slot != slot) {
        old_slot->busy = 0;
    }
    if (g_bt_h264_disp.pending && !g_bt_h264_disp.show_posted) {
        g_bt_h264_disp.show_posted = 1;
        defer_present = 1;
    }
    os_mutex_post(&g_bt_h264_disp.mutex);

    /*
     * LV_EVENT_RENDER_READY is emitted before LVGL clears
     * rendering_in_progress. Defer the next invalidate to a later LVGL
     * timer pass; calling lv_image_set_src() here trips lv_inv_area().
     */
    if (defer_present) {
        if (lv_async_call(screen_mirror_bt_h264_lvgl_present_pending_async,
                          NULL) != LV_RESULT_OK) {
            os_mutex_pend(&g_bt_h264_disp.mutex, 0);
            g_bt_h264_disp.show_posted = 0;
            os_mutex_post(&g_bt_h264_disp.mutex);
            log_error("defer pending frame fail");
        }
    }
}

static void screen_mirror_bt_h264_lvgl_present_pending(void)
{
#if (defined LV_USE_VIDEO_DISP && LV_USE_VIDEO_DISP)
    struct bt_h264_nv12_slot *slot;
    struct bt_h264_nv12_slot *old_displayed;
    lv_obj_t *obj;

    screen_mirror_bt_h264_display_init_once();
    if (!screen_mirror_bt_h264_display_can_run()) {
        os_mutex_pend(&g_bt_h264_disp.mutex, 0);
        g_bt_h264_disp.show_posted = 0;
        slot = g_bt_h264_disp.pending;
        g_bt_h264_disp.pending = NULL;
        if (slot) {
            slot->busy = 0;
            g_bt_h264.display_drop_frames++;
        }
        os_mutex_post(&g_bt_h264_disp.mutex);
        return;
    }

    os_mutex_pend(&g_bt_h264_disp.mutex, 0);
    g_bt_h264_disp.show_posted = 0;
    obj = g_bt_h264_disp.obj;
    if (!obj || g_bt_h264_disp.rendering || !g_bt_h264_disp.pending) {
        if (!obj && g_bt_h264_disp.pending) {
            slot = g_bt_h264_disp.pending;
            g_bt_h264_disp.pending = NULL;
            slot->busy = 0;
            g_bt_h264.display_drop_frames++;
        }
        os_mutex_post(&g_bt_h264_disp.mutex);
        return;
    }

    slot = g_bt_h264_disp.pending;
    g_bt_h264_disp.pending = NULL;
    g_bt_h264_disp.rendering = slot;
    old_displayed = g_bt_h264_disp.displayed;
    g_bt_h264_disp.displayed = NULL;
    os_mutex_post(&g_bt_h264_disp.mutex);

    lv_image_set_src(obj, &slot->image_dsc);

    if (old_displayed && old_displayed != slot) {
        os_mutex_pend(&g_bt_h264_disp.mutex, 0);
        if (g_bt_h264_disp.displayed != old_displayed &&
            g_bt_h264_disp.rendering != old_displayed &&
            g_bt_h264_disp.pending != old_displayed) {
            old_displayed->busy = 0;
        }
        os_mutex_post(&g_bt_h264_disp.mutex);
    }
#else
    struct bt_h264_nv12_slot *slot;

    screen_mirror_bt_h264_display_init_once();
    os_mutex_pend(&g_bt_h264_disp.mutex, 0);
    g_bt_h264_disp.show_posted = 0;
    slot = g_bt_h264_disp.pending;
    g_bt_h264_disp.pending = NULL;
    if (slot) {
        slot->busy = 0;
        g_bt_h264.display_drop_frames++;
    }
    os_mutex_post(&g_bt_h264_disp.mutex);
#endif
}

static int screen_mirror_bt_h264_display_input_i420(const u8 *i420,
        u32 width, u32 height)
{
    struct bt_h264_nv12_slot *slot = NULL;
    u32 size;
    u8 replaced_pending = 0;
    u8 need_post = 0;
    int err;

    if (!i420 || !width || !height || (width & 1) || (height & 1) ||
        !screen_mirror_bt_h264_display_can_run()) {
        return 0;
    }

    size = width * height * 3 / 2;
    screen_mirror_bt_h264_display_init_once();
    os_mutex_pend(&g_bt_h264_disp.mutex, 0);
    if (!g_bt_h264_disp.obj) {
        os_mutex_post(&g_bt_h264_disp.mutex);
        return 0;
    }

    /* rendering 期间保留一帧 pending；再来新帧时只覆盖旧 pending。 */
    slot = g_bt_h264_disp.pending;
    if (slot) {
        replaced_pending = 1;
    } else {
        for (int i = 0; i < BT_H264_NV12_BUF_NUM; i++) {
            if (!g_bt_h264_disp.slots[i].busy) {
                slot = &g_bt_h264_disp.slots[i];
                break;
            }
        }
#if BT_H264_NV12_BUF_NUM == 1
        if (!slot &&
            !g_bt_h264_disp.rendering &&
            g_bt_h264_disp.displayed) {
            slot = g_bt_h264_disp.displayed;
            g_bt_h264_disp.displayed = NULL;
        }
#endif
    }
    if (!slot) {
        os_mutex_post(&g_bt_h264_disp.mutex);
        return 0;
    }

    if (slot->capacity < size) {
        u8 *new_buf = BT_H264_BIG_MALLOC(size);
        if (!new_buf) {
            void psram_heap_stats(void);
            psram_heap_stats();
            log_error("NV12 display buffer alloc fail, size=%lu", size);
            os_mutex_post(&g_bt_h264_disp.mutex);
            return 0;
        }
        log_info("NV12 display buffer alloc: addr=%p size=%lu (%ux%u)",
                 new_buf, size, width, height);
        if (slot->buf) {
            log_info("NV12 display buffer free (old): addr=%p size=%lu",
                     slot->buf, slot->capacity);
            BT_H264_BIG_FREE(slot->buf);
        }
        slot->buf = new_buf;
        slot->capacity = size;
    }

    /* TinyH264 输出 I420(Y+U+V)，JLGPU 显示使用 NV12(Y+UV)。 */
    screen_mirror_bt_h264_i420_to_nv12(slot->buf, i420, width, height);
#if TCFG_PSRAM_DEV_ENABLE
    int psram_cache_check(void *pv);
    if (psram_cache_check(slot->buf)) {
        DcuFlushRegion((u32 *)slot->buf, size);
    }
#endif
    memset(&slot->image_dsc, 0, sizeof(slot->image_dsc));
    slot->image_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    slot->image_dsc.header.cf = LV_COLOR_FORMAT_NV12;
    slot->image_dsc.header.w = width;
    slot->image_dsc.header.h = height;
    slot->image_dsc.header.stride = width;
    slot->image_dsc.data = slot->buf;
    slot->image_dsc.data_size = size;
    slot->width = width;
    slot->height = height;
    slot->busy = 1;
    g_bt_h264_disp.pending = slot;
    if (replaced_pending) {
        g_bt_h264.display_drop_frames++;
    }
    if (!g_bt_h264_disp.rendering && !g_bt_h264_disp.show_posted) {
        g_bt_h264_disp.show_posted = 1;
        need_post = 1;
    }
    os_mutex_post(&g_bt_h264_disp.mutex);

    if (need_post) {
        err = lvgl_rpc_post_func(
                  (void *)screen_mirror_bt_h264_lvgl_present_pending, 0);
        if (err) {
            os_mutex_pend(&g_bt_h264_disp.mutex, 0);
            g_bt_h264_disp.show_posted = 0;
            if (g_bt_h264_disp.pending == slot) {
                g_bt_h264_disp.pending = NULL;
                slot->busy = 0;
            }
            os_mutex_post(&g_bt_h264_disp.mutex);
            return 0;
        }
    }
    return 1;
}

static void screen_mirror_bt_h264_lvgl_create(int32_t width, int32_t height,
        int32_t x, int32_t y)
{
#if (defined LV_USE_VIDEO_DISP && LV_USE_VIDEO_DISP)
    lv_display_t *disp;
    lv_obj_t *obj;

    (void)width;
    (void)height;
    if (!screen_mirror_bt_h264_display_can_run()) {
        return;
    }

    disp = lv_display_get_default();
    if (!disp) {
        return;
    }

    screen_mirror_bt_h264_display_init_once();
    os_mutex_pend(&g_bt_h264_disp.mutex, 0);
    if (g_bt_h264_disp.obj) {
        os_mutex_post(&g_bt_h264_disp.mutex);
        return;
    }

    obj = lv_image_create(lv_screen_active());
    if (!obj) {
        os_mutex_post(&g_bt_h264_disp.mutex);
        return;
    }

    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, BT_H264_DISPLAY_WIDTH, BT_H264_DISPLAY_HEIGHT);
    // lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_STRETCH);
    lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_COVER);
    lv_obj_move_background(obj);

    g_bt_h264_disp.obj = obj;
    g_bt_h264_disp.lv_disp = disp;
    lv_display_add_event_cb(disp,
                            screen_mirror_bt_h264_render_ready_event_cb,
                            LV_EVENT_RENDER_READY,
                            &g_bt_h264_disp);
    os_mutex_post(&g_bt_h264_disp.mutex);
#else
    (void)width;
    (void)height;
    (void)x;
    (void)y;
#endif
}

static void screen_mirror_bt_h264_lvgl_destroy(void *disp_obj)
{
#if (defined LV_USE_VIDEO_DISP && LV_USE_VIDEO_DISP)
    if (g_bt_h264_disp.lv_disp) {
        lv_display_remove_event_cb_with_user_data(
            g_bt_h264_disp.lv_disp,
            screen_mirror_bt_h264_render_ready_event_cb,
            &g_bt_h264_disp);
    }
    if (disp_obj) {
        lv_obj_delete((lv_obj_t *)disp_obj);
    }

    os_mutex_pend(&g_bt_h264_disp.mutex, 0);
    for (int i = 0; i < BT_H264_NV12_BUF_NUM; i++) {
        if (g_bt_h264_disp.slots[i].buf) {
            log_info("NV12 display buffer free (destroy): "
                     "slot=%d addr=%p size=%lu",
                     i, g_bt_h264_disp.slots[i].buf,
                     g_bt_h264_disp.slots[i].capacity);
            BT_H264_BIG_FREE(g_bt_h264_disp.slots[i].buf);
            g_bt_h264_disp.slots[i].buf = NULL;
            g_bt_h264_disp.slots[i].capacity = 0;
        }
        g_bt_h264_disp.slots[i].width = 0;
        g_bt_h264_disp.slots[i].height = 0;
        memset(&g_bt_h264_disp.slots[i].image_dsc, 0,
               sizeof(g_bt_h264_disp.slots[i].image_dsc));
        g_bt_h264_disp.slots[i].busy = 0;
    }
    g_bt_h264_disp.lv_disp = NULL;
    g_bt_h264_disp.displayed = NULL;
    g_bt_h264_disp.rendering = NULL;
    g_bt_h264_disp.pending = NULL;
    g_bt_h264_disp.show_posted = 0;
    os_mutex_post(&g_bt_h264_disp.mutex);
#else
    (void)disp_obj;
#endif
}

void screen_mirror_bt_h264_map_enter(void)
{
#if BT_H264_DECODE_BENCHMARK
    return;
#else
    int err;

    if (!screen_mirror_bt_h264_display_can_run()) {
        return;
    }

    err = lvgl_rpc_post_func((void *)screen_mirror_bt_h264_lvgl_create,
                             4, g_bt_h264.width, g_bt_h264.height, 0, 0);
    if (err) {
        log_error("post display create fail: %d", err);
    }
#endif
}

void screen_mirror_bt_h264_map_leave(void)
{
#if BT_H264_DECODE_BENCHMARK
    return;
#else
    lv_obj_t *obj;
    int err;

    screen_mirror_bt_h264_display_init_once();
    os_mutex_pend(&g_bt_h264_disp.mutex, 0);
    obj = g_bt_h264_disp.obj;
    g_bt_h264_disp.obj = NULL;
    os_mutex_post(&g_bt_h264_disp.mutex);
    if (!obj) {
        return;
    }

    err = lvgl_rpc_post_func((void *)screen_mirror_bt_h264_lvgl_destroy,
                             1, obj);
    if (err) {
        os_mutex_pend(&g_bt_h264_disp.mutex, 0);
        if (!g_bt_h264_disp.obj) {
            g_bt_h264_disp.obj = obj;
        }
        os_mutex_post(&g_bt_h264_disp.mutex);
        log_error("post display destroy fail: %d", err);
    }
#endif
}

static void *screen_mirror_bt_h264_decoder_malloc(int size, u32 ram_type,
        u32 module_type)
{
    void *buf;

    if (size <= 0) {
        return NULL;
    }

#if TCFG_PSRAM_DEV_ENABLE
    if (ram_type == BT_H264_DEC_RAM_PSRAM) {
        buf = malloc_psram((size_t)size);
        if (buf) {
            g_bt_h264.decoder_psram_alloc_bytes += (u32)size;
        }
    } else
#endif
    {
        buf = malloc((size_t)size);
        if (buf) {
            g_bt_h264.decoder_sram_alloc_bytes += (u32)size;
        }
    }

#if !BT_H264_DECODE_BENCHMARK
    if (buf && size >= (64 * 1024)) {
        log_info("decoder big alloc: addr=%p size=%d ram=%lu (psram_total=%lu sram_total=%lu)",
                 buf, size, ram_type,
                 g_bt_h264.decoder_psram_alloc_bytes,
                 g_bt_h264.decoder_sram_alloc_bytes);
    }
#endif
    if (!buf) {
        log_error("decoder alloc fail: size=%d ram=%lu module=%lu",
                  size, ram_type, module_type);
    }
    return buf;
}

static void screen_mirror_bt_h264_decoder_free(void *buf, u32 ram_type,
        u32 module_type)
{
    (void)module_type;

    if (!buf) {
        return;
    }

#if TCFG_PSRAM_DEV_ENABLE
    if (ram_type == BT_H264_DEC_RAM_PSRAM) {
        free_psram(buf);
        return;
    }
#endif
    free(buf);
}

static void screen_mirror_bt_h264_reset_frame(struct screen_mirror_bt_h264 *ctx)
{
    if (ctx->stream_buf) {
        BT_H264_BIG_FREE(ctx->stream_buf);
        ctx->stream_buf = NULL;
    }
    ctx->stream_capacity = 0;
    ctx->stream_len = 0;
    ctx->frame_size = 0;
    ctx->frame_crc = 0;
}

static int screen_mirror_bt_h264_decoder_init(struct screen_mirror_bt_h264 *ctx)
{
    if (ctx->decoder_inited) {
        return 0;
    }

    ctx->decoder_psram_alloc_bytes = 0;
    ctx->decoder_sram_alloc_bytes = 0;
    video_1_register_alloc(screen_mirror_bt_h264_decoder_malloc,
                           screen_mirror_bt_h264_decoder_free);

    ctx->dec_storage = video_1_alloc();
    if (!ctx->dec_storage) {
        log_error("decoder storage alloc fail");
        return -1;
    }

    if (video_1_init(ctx->dec_storage) != 0) {
        log_error("decoder init fail");
        video_1_free(ctx->dec_storage);
        ctx->dec_storage = NULL;
        return -1;
    }

    ctx->decoder_inited = 1;
    ctx->decoded_frames = 0;
    log_info("decoder inited");
    return 0;
}

static void screen_mirror_bt_h264_decoder_deinit(struct screen_mirror_bt_h264 *ctx)
{
    screen_mirror_bt_h264_reset_frame(ctx);

    if (ctx->dec_storage) {
        video_1_deinit(ctx->dec_storage);
        video_1_free(ctx->dec_storage);
        ctx->dec_storage = NULL;
    }

    ctx->decoder_inited = 0;
#if BT_H264_DECODE_BENCHMARK
    log_info("decode summary I: count=%lu avg=%lu ms max=%lu ms; "
             "P: count=%lu avg=%lu ms max=%lu ms; other=%lu",
             ctx->bench_i_frames,
             ctx->bench_i_frames ?
             (u32)(ctx->bench_i_total_ms / ctx->bench_i_frames) : 0,
             ctx->bench_i_max_ms,
             ctx->bench_p_frames,
             ctx->bench_p_frames ?
             (u32)(ctx->bench_p_total_ms / ctx->bench_p_frames) : 0,
             ctx->bench_p_max_ms,
             ctx->bench_other_frames);
#endif
    log_info("decoder deinited, recv=%lu decoded=%lu display=%lu "
             "display_drop=%lu last_size=%lu "
             "dec_psram=%lu dec_sram=%lu",
             ctx->received_frames, ctx->decoded_frames,
             ctx->displayed_frames, ctx->display_drop_frames,
             ctx->last_frame_size,
             ctx->decoder_psram_alloc_bytes,
             ctx->decoder_sram_alloc_bytes);
}

static int screen_mirror_bt_h264_output(struct screen_mirror_bt_h264 *ctx)
{
    struct video_1_output out = {0};

    if (!ctx->dec_storage) {
        return 0;
    }

    video_1_decode_output(ctx->dec_storage, &out);
    if (!out.frame_buf || out.width <= 0 || out.height <= 0) {
        return 0;
    }

    ctx->decoded_frames++;
#if !BT_H264_DECODE_BENCHMARK
    if (ctx->decoded_frames == 1) {
        log_info("first frame %dx%d, dec_psram=%lu dec_sram=%lu",
                 out.width, out.height, ctx->decoder_psram_alloc_bytes,
                 ctx->decoder_sram_alloc_bytes);
    }
    if (!screen_mirror_bt_h264_display_input_i420(
            out.frame_buf, (u32)out.width, (u32)out.height)) {
        ctx->display_drop_frames++;
    }
#endif
    return 1;
}

static int screen_mirror_bt_h264_feed(struct screen_mirror_bt_h264 *ctx,
                                      const u8 *data, u32 len,
                                      u32 offset, u32 frame_size,
                                      u16 seq, u16 crc, u8 is_last)
{
    int frames = 0;
#if BT_H264_DECODE_BENCHMARK
    enum bt_h264_frame_type frame_type;
    u32 decode_start_ms;
    u32 decode_ms;
#endif

    if (!ctx->decoder_inited || !data || !len) {
        return 0;
    }

    if (offset == 0) {
        u32 alloc_size;
        u8 *new_buf;

        if (!frame_size) {
            return 0;
        }

        screen_mirror_bt_h264_reset_frame(ctx);
        alloc_size = frame_size + 4096;
        new_buf = BT_H264_BIG_MALLOC(alloc_size);
        if (!new_buf) {
            void psram_heap_stats(void);
            psram_heap_stats();
            log_error("PSRAM frame alloc fail, size=%lu", frame_size);
            return 0;
        }

        ctx->stream_buf = new_buf;
        ctx->stream_capacity = alloc_size;
        ctx->frame_size = frame_size;
        ctx->frame_crc = crc;
        ctx->frame_seq = seq;
    }

    if (!ctx->stream_buf || offset + len > ctx->stream_capacity) {
        log_error("fragment overflow, seq=%u off=%lu len=%lu", seq, offset, len);
        screen_mirror_bt_h264_reset_frame(ctx);
        return 0;
    }

    memcpy(ctx->stream_buf + offset, data, len);
    if (offset + len > ctx->stream_len) {
        ctx->stream_len = offset + len;
    }

    if (!is_last) {
        return 0;
    }

    if (ctx->frame_size && ctx->stream_len != ctx->frame_size) {
        log_error("frame incomplete, seq=%u recv=%lu expect=%lu",
                  ctx->frame_seq, ctx->stream_len, ctx->frame_size);
        screen_mirror_bt_h264_reset_frame(ctx);
        return 0;
    }

    if (ctx->frame_crc) {
        u16 calc_crc = CRC16_with_initval(ctx->stream_buf, ctx->stream_len, 0);
        if (calc_crc != ctx->frame_crc) {
            log_error("crc fail, seq=%u calc=0x%04x expect=0x%04x",
                      ctx->frame_seq, calc_crc, ctx->frame_crc);
            screen_mirror_bt_h264_reset_frame(ctx);
            return 0;
        }
    }

    screen_mirror_bt_h264_stats_on_received(ctx, ctx->stream_len);

#if BT_H264_DECODE_BENCHMARK
    frame_type = screen_mirror_bt_h264_get_frame_type(
                     ctx->stream_buf, ctx->stream_len);
    decode_start_ms = timer_get_ms();
#endif
    {
        u32 dec_off = 0;
        u32 zero_read_count = 0;

        while (dec_off < ctx->stream_len) {
            u32 read_bytes = 0;
            u32 ret = video_1_decode(ctx->dec_storage,
                                     ctx->stream_buf + dec_off,
                                     ctx->stream_len - dec_off,
                                     &read_bytes, NULL);

            if (ret == video_1_DEC_PIC_RDY) {
                frames += screen_mirror_bt_h264_output(ctx);
                zero_read_count = 0;
            } else if (ret == video_1_DEC_HDRS_RDY) {
                if (++zero_read_count > 4) {
                    log_error("decoder headers stuck, seq=%u", ctx->frame_seq);
                    break;
                }
                continue;
            } else if (ret == video_1_DEC_ERROR ||
                       ret == video_1_DEC_PARAM_SET_ERROR ||
                       ret == video_1_DEC_MEMALLOC_ERROR) {
                log_error("decode fail, seq=%u ret=%lu", ctx->frame_seq, ret);
                break;
            }

            if (!read_bytes) {
                if (++zero_read_count > 4) {
                    log_error("decoder no progress, seq=%u ret=%lu", ctx->frame_seq, ret);
                    break;
                }
                continue;
            }

            dec_off += read_bytes;
            zero_read_count = 0;
        }
    }
#if BT_H264_DECODE_BENCHMARK
    decode_ms = timer_get_ms() - decode_start_ms;
    screen_mirror_bt_h264_bench_record(
        ctx, frame_type, ctx->frame_seq, ctx->stream_len, decode_ms, frames);
#endif

    screen_mirror_bt_h264_reset_frame(ctx);
    return frames;
}

static void screen_mirror_bt_h264_recv_task(void *priv)
{
    int msg[8];
    struct screen_mirror_bt_h264 *ctx = priv;

    while (1) {
        if (os_taskq_pend("taskq", msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }

        switch (msg[0]) {
        case Q_BT_H264_DATA: {
            struct bt_h264_msg *pkt = (struct bt_h264_msg *)msg[1];
            if (!pkt) {
                break;
            }

            if (ctx->running) {
                if (!ctx->decoder_inited &&
                    screen_mirror_bt_h264_decoder_init(ctx) != 0) {
                    free(pkt);
                    break;
                }

                screen_mirror_bt_h264_feed(ctx,
                                           pkt->payload, pkt->payload_len,
                                           pkt->offset, pkt->frame_size,
                                           pkt->seq, pkt->crc, pkt->is_last);
                screen_mirror_bt_h264_stats_report(ctx);
            }

            free(pkt);
            wdt_clear();
            break;
        }

        case Q_BT_H264_KILL: {
            OS_SEM *kill_sem = (OS_SEM *)msg[1];
            while (os_taskq_accept(ARRAY_SIZE(msg), msg) == OS_TASKQ) {
                if (msg[0] == Q_BT_H264_DATA && msg[1]) {
                    free((void *)msg[1]);
                }
            }
            screen_mirror_bt_h264_decoder_deinit(ctx);
            os_sem_post(kill_sem);
            os_time_dly(-1);
            return;
        }

        default:
            break;
        }
    }
}

static int screen_mirror_bt_h264_receiver_start(void)
{
    if (g_bt_h264.task_created) {
        g_bt_h264.running = 1;
        return 0;
    }

    g_bt_h264.running = 1;
    if (os_task_create_affinity_core(screen_mirror_bt_h264_recv_task,
                                     &g_bt_h264, 15, 1024, 1024,
                                     BT_H264_TASK_NAME, 1)) {
        g_bt_h264.running = 0;
        log_error("receiver task create fail");
        return -1;
    }

    g_bt_h264.task_created = 1;
    log_info("receiver started");
    return 0;
}

static void screen_mirror_bt_h264_receiver_stop(void)
{
    OS_SEM kill_sem;
    int sem_ptr;

    g_bt_h264.running = 0;
    if (!g_bt_h264.task_created) {
        screen_mirror_bt_h264_decoder_deinit(&g_bt_h264);
        return;
    }

    os_sem_create(&kill_sem, 0);
    sem_ptr = (int)&kill_sem;
    if (os_taskq_post_type(BT_H264_TASK_NAME, Q_BT_H264_KILL, 1, &sem_ptr) == 0) {
        os_sem_pend(&kill_sem, 0);
    }
    os_sem_del(&kill_sem, OS_DEL_ALWAYS);

    task_kill(BT_H264_TASK_NAME);
    g_bt_h264.task_created = 0;
    log_info("receiver stopped");
}

static void screen_mirror_bt_h264_start_cb(const stream_media_stream_cfg_t *cfg)
{
    if (!cfg || cfg->format != STREAM_FORMAT_H264) {
        log_error("unsupported stream format");
        return;
    }

    if (map_video_source_acquire(MAP_VIDEO_SOURCE_BT_H264)) {
        return;
    }

    g_bt_h264.format = cfg->format;
    g_bt_h264.width = cfg->width;
    g_bt_h264.height = cfg->height;
    g_bt_h264.fps = cfg->fps;
    screen_mirror_bt_h264_stats_reset(&g_bt_h264);
    log_info("start stream %ux%u@%u format=%u quality=%u",
             cfg->width, cfg->height, cfg->fps, cfg->format);

    if (screen_mirror_bt_h264_receiver_start()) {
        map_video_source_release(MAP_VIDEO_SOURCE_BT_H264);
        return;
    }
#if !BT_H264_DECODE_BENCHMARK
    screen_mirror_bt_h264_map_enter();
#else
    log_info("decode benchmark enabled, LVGL output disabled");
#endif
}

static void screen_mirror_bt_h264_stop_cb(void)
{
    screen_mirror_bt_h264_receiver_stop();
#if !BT_H264_DECODE_BENCHMARK
    screen_mirror_bt_h264_map_leave();
#endif
    map_video_source_release(MAP_VIDEO_SOURCE_BT_H264);
}

static void screen_mirror_bt_h264_data_cb(const u8 *data, u16 len,
        u16 seq, u32 offset,
        u32 frame_size, u32 timestamp,
        u16 crc, u8 stream_type, u8 is_last)
{
    struct bt_h264_msg *msg;
    int arg;

    (void)stream_type;
    if (!g_bt_h264.running || !data || !len ||
        g_bt_h264.format != STREAM_FORMAT_H264) {
        return;
    }

    msg = malloc(sizeof(*msg) + len);
    if (!msg) {
        log_error("fragment message alloc fail");
        return;
    }

    msg->frame_size = frame_size;
    msg->timestamp = timestamp;
    msg->crc = crc;
    msg->seq = seq;
    msg->offset = offset;
    msg->payload_len = len;
    msg->is_last = is_last;
    memcpy(msg->payload, data, len);

    arg = (int)msg;
    if (os_taskq_post_type(BT_H264_TASK_NAME, Q_BT_H264_DATA, 1, &arg)) {
        free(msg);
    }
}
#if LV_USE_DEMO_MAP
// 离线地图导航数据获取回调示例
static u8 screen_mirror_bt_offline_nav_cb(
    const stream_media_offline_nav_info_t *info)
{
    if (!info) {
        return STREAM_MEDIA_ERR_FAIL;
    }

    printf("[OFFLINE_NAV] start lon=%ld lat=%ld\n",
           (long)info->start_longitude_e6,
           (long)info->start_latitude_e6);

    printf("[OFFLINE_NAV] end lon=%ld lat=%ld\n",
           (long)info->end_longitude_e6,
           (long)info->end_latitude_e6);

    /* 名称非零结尾, 拷贝到局部缓冲补 '\0' (map_offline_nav_start 内部会再复制) */
    char start_name[64];
    char end_name[64];
    start_name[0] = '\0';
    end_name[0] = '\0';
    if (info->start_name && info->start_name_len > 0) {
        int n = info->start_name_len < (int)sizeof(start_name) - 1
                ? info->start_name_len : (int)sizeof(start_name) - 1;
        memcpy(start_name, info->start_name, n);
        start_name[n] = '\0';
    }
    if (info->end_name && info->end_name_len > 0) {
        int n = info->end_name_len < (int)sizeof(end_name) - 1
                ? info->end_name_len : (int)sizeof(end_name) - 1;
        memcpy(end_name, info->end_name, n);
        end_name[n] = '\0';
    }

    printf("[OFFLINE_NAV] start=%s\n", start_name);
    printf("[OFFLINE_NAV] end=%s\n", end_name);

    /* 开启导航 (内部投递到 LVGL 线程) */
    map_offline_nav_start(info->start_longitude_e6, info->start_latitude_e6,
                          info->end_longitude_e6, info->end_latitude_e6,
                          start_name, end_name);

    return STREAM_MEDIA_ERR_OK;
}
#endif
int screen_mirror_bt_h264_init(void)
{
#if !BT_H264_DECODE_BENCHMARK
    screen_mirror_bt_h264_display_init_once();
#endif
    stream_media_set_stream_start_cb(screen_mirror_bt_h264_start_cb);
    stream_media_set_stream_stop_cb(screen_mirror_bt_h264_stop_cb);
    stream_media_set_data_push_cb(screen_mirror_bt_h264_data_cb);
#if LV_USE_DEMO_MAP
    stream_media_set_offline_nav_cb(screen_mirror_bt_offline_nav_cb);
#endif
    stream_media_core_init();
    log_info("callbacks registered");
    return 0;
}

void screen_mirror_bt_h264_deinit(void)
{
    screen_mirror_bt_h264_stop_cb();
}

early_initcall(screen_mirror_bt_h264_init);

#else

int screen_mirror_bt_h264_init(void)
{
    return 0;
}

void screen_mirror_bt_h264_deinit(void)
{
}

void screen_mirror_bt_h264_map_enter(void)
{
}

void screen_mirror_bt_h264_map_leave(void)
{
}

#endif

#endif /* 网络功能宏: 投屏依赖网络, kid camera 不用 */
