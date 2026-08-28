
#include "lvgl.h"
#include "src/draw/lv_image_decoder.h"
#include "src/draw/lv_image_decoder_private.h"

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void create_image_display(lv_obj_t *parent);
static void create_status_label(lv_obj_t *parent);
static void apply_filter(int index);
static void on_timer_tick(lv_timer_t *timer);
static void clear_all_filters(void);
static void prepare_bw_buffer(void);

/**********************
 *  STATIC VARIABLES
 **********************/

static const int TIMER_INTERVAL_MS = 3000;

static lv_obj_t *g_filter_image   = NULL;
static lv_obj_t *g_image_container = NULL;
static lv_obj_t *g_invert_overlay  = NULL;
static lv_obj_t *g_status_label    = NULL;
static lv_timer_t *g_timer         = NULL;
static int          g_current_filter = 0;  /* 0..5 = Normal,Red,Green,Yellow,B&W,Invert */

#include "../../lv_examples.h"
LV_IMAGE_DECLARE(img_cogwheel_argb);
#define ORIG_IMG_PATH (&img_cogwheel_argb)
// static const char * ORIG_IMG_PATH = "A:assert/home/png/home_backgroud.png";

static const lv_color_t FILTER_RED    = {.red = 0xFF, .green = 0x00, .blue = 0x00};
static const lv_color_t FILTER_GREEN  = {.red = 0x00, .green = 0xFF, .blue = 0x00};
static const lv_color_t FILTER_YELLOW = {.red = 0xFF, .green = 0xFF, .blue = 0x00};

static const char *FILTER_NAMES[] = {
    "Normal", "Red", "Green", "Yellow", "B&W", "Invert"
};
static const int FILTER_COUNT = 6;

/* ── B&W pixel-filter buffer (allocated once) ── */
static uint8_t       *g_bw_buf   = NULL;
static lv_image_dsc_t g_bw_dsc;
static bool           g_bw_ready = false;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_example_image_filter(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    /* Title */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "LV Image Filter Test");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    create_image_display(scr);
    create_status_label(scr);

    /* Pre-decode B&W buffer (one-time cost) */
    prepare_bw_buffer();

    /* Start auto-cycle timer: 1s per filter */
    g_timer = lv_timer_create(on_timer_tick, TIMER_INTERVAL_MS, NULL);
    lv_timer_ready(g_timer);  /* fire first tick immediately */
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* ─── Image Display ─── */

static void create_image_display(lv_obj_t *parent)
{
    g_image_container = lv_obj_create(parent);
    lv_obj_set_size(g_image_container, 480, 280);
    lv_obj_align(g_image_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(g_image_container, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(g_image_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_image_container, 2, 0);
    lv_obj_set_style_border_color(g_image_container, lv_color_hex(0x888888), 0);
    lv_obj_set_style_radius(g_image_container, 8, 0);
    lv_obj_set_style_pad_all(g_image_container, 0, 0);
    lv_obj_clear_flag(g_image_container, LV_OBJ_FLAG_SCROLLABLE);

    /* Invert mask overlay — white, shown only during invert */
    g_invert_overlay = lv_obj_create(g_image_container);
    lv_obj_remove_style_all(g_invert_overlay);
    lv_obj_set_size(g_invert_overlay, 480, 280);
    lv_obj_center(g_invert_overlay);
    lv_obj_set_style_bg_color(g_invert_overlay, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(g_invert_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(g_invert_overlay, 0, 0);
    lv_obj_clear_flag(g_invert_overlay, LV_OBJ_FLAG_SCROLLABLE);

    /* Main image */
    g_filter_image = lv_image_create(g_image_container);
    lv_image_set_src(g_filter_image, ORIG_IMG_PATH);
    lv_obj_set_size(g_filter_image, 480, 280);
    lv_obj_center(g_filter_image);
    lv_image_set_inner_align(g_filter_image, LV_IMAGE_ALIGN_STRETCH);
}

/* ─── Status Label (shows current filter name) ─── */

static void create_status_label(lv_obj_t *parent)
{
    g_status_label = lv_label_create(parent);
    lv_obj_set_style_text_font(g_status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_status_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(g_status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_label_set_text(g_status_label, "Normal");
}

/* ─── Timer Callback — cycle to next filter ─── */

static void on_timer_tick(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    apply_filter(g_current_filter);

    /* Advance for next tick */
    g_current_filter++;
    if (g_current_filter >= FILTER_COUNT) {
        g_current_filter = 0;
    }
}

/* ─── Apply a single filter by index ─── */

static void apply_filter(int index)
{
    if (g_filter_image == NULL) {
        return;
    }

    clear_all_filters();

    /* Update status label */
    lv_label_set_text(g_status_label, FILTER_NAMES[index]);

    switch (index) {
    case 0: /* Normal */
        lv_image_set_src(g_filter_image, ORIG_IMG_PATH);
        break;

    case 1: /* Red */
        lv_image_set_src(g_filter_image, ORIG_IMG_PATH);
        lv_obj_set_style_image_recolor(g_filter_image, FILTER_RED, 0);
        lv_obj_set_style_image_recolor_opa(g_filter_image, LV_OPA_50, 0);
        break;

    case 2: /* Green */
        lv_image_set_src(g_filter_image, ORIG_IMG_PATH);
        lv_obj_set_style_image_recolor(g_filter_image, FILTER_GREEN, 0);
        lv_obj_set_style_image_recolor_opa(g_filter_image, LV_OPA_50, 0);
        break;

    case 3: /* Yellow */
        lv_image_set_src(g_filter_image, ORIG_IMG_PATH);
        lv_obj_set_style_image_recolor(g_filter_image, FILTER_YELLOW, 0);
        lv_obj_set_style_image_recolor_opa(g_filter_image, LV_OPA_50, 0);
        break;

    case 4: /* B&W */
        if (g_bw_ready) {
            lv_image_set_src(g_filter_image, &g_bw_dsc);
        }
        break;

    case 5: /* Invert — white mask + DIFFERENCE blend */
        lv_image_set_src(g_filter_image, ORIG_IMG_PATH);
        lv_obj_clear_flag(g_invert_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_image_set_blend_mode(g_filter_image, LV_BLEND_MODE_DIFFERENCE);
        break;
    }
}

/* ─── Clear-All-Filters Helper ─── */

static void clear_all_filters(void)
{
    lv_image_set_blend_mode(g_filter_image, LV_BLEND_MODE_NORMAL);
    lv_obj_add_flag(g_invert_overlay, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_image_recolor(g_filter_image, lv_color_black(), 0);
    lv_obj_set_style_image_recolor_opa(g_filter_image, LV_OPA_TRANSP, 0);
}

/* ═══════════════════════════════════════════════════════════════
 *  B&W:  Decode → Grayscale → New Image Source  (one-time)
 * ═══════════════════════════════════════════════════════════════ */

static void prepare_bw_buffer(void)
{
    if (g_bw_ready) {
        return;
    }

    lv_image_decoder_dsc_t dsc;
    lv_result_t res = lv_image_decoder_open(&dsc, ORIG_IMG_PATH, NULL);
    if (res != LV_RESULT_OK) {
        LV_LOG_WARN("B&W: decoder open failed");
        return;
    }
    if (dsc.decoded == NULL || dsc.decoded->data == NULL) {
        LV_LOG_WARN("B&W: decoded data is NULL");
        lv_image_decoder_close(&dsc);
        return;
    }

    int32_t w = dsc.header.w;
    int32_t h = dsc.header.h;
    uint32_t stride = dsc.decoded->header.stride;

    size_t buf_bytes = (size_t)w * h * sizeof(lv_color32_t);
    g_bw_buf = lv_malloc(buf_bytes);
    if (!g_bw_buf) {
        lv_image_decoder_close(&dsc);
        return;
    }

    const uint8_t *src_data = dsc.decoded->data;
    lv_color32_t *dst = (lv_color32_t *)g_bw_buf;

    for (int32_t y = 0; y < h; y++) {
        const lv_color32_t *src_row =
            (const lv_color32_t *)(src_data + (size_t)y * stride);
        lv_color32_t *dst_row = dst + (size_t)y * w;

        for (int32_t x = 0; x < w; x++) {
            uint8_t gray = (uint8_t)(((uint16_t)src_row[x].red   * 77 +
                                      (uint16_t)src_row[x].green * 150 +
                                      (uint16_t)src_row[x].blue  * 29) >> 8);
            dst_row[x].red   = gray;
            dst_row[x].green = gray;
            dst_row[x].blue  = gray;
            dst_row[x].alpha = src_row[x].alpha;
        }
    }

    lv_image_decoder_close(&dsc);

    g_bw_dsc.header.w      = w;
    g_bw_dsc.header.h      = h;
    g_bw_dsc.header.cf     = dsc.decoded->header.cf;
    g_bw_dsc.header.stride = w * sizeof(lv_color32_t);
    g_bw_dsc.data          = g_bw_buf;
    g_bw_dsc.data_size     = buf_bytes;

    g_bw_ready = true;
}
