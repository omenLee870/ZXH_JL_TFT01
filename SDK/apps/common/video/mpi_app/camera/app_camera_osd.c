#include "app_camera_mpi_priv.h"
#include "system/includes.h"
#include "sdk_config.h"
#include "osd.h"

#define APP_CAMERA_OSD_FONT_W           16u
#define APP_CAMERA_OSD_FONT_H           32u
#define APP_CAMERA_OSD_BPP              2u
#define APP_CAMERA_OSD_WIDTH_ALIGN      64u
#define APP_CAMERA_OSD_BUF_ALIGN        64u
#define APP_CAMERA_OSD_TEXT_MAX         256u
#define APP_CAMERA_OSD_REFRESH_MS       1000u

extern uint32_t timer_get_ms(void);

static uint16_t app_camera_osd_align_up(uint16_t value, uint16_t align)
{
    return (uint16_t)(((uint32_t)value + align - 1u) / align * align);
}

static uint8_t app_camera_osd_char_bytes(const char *text)
{
    if (text == NULL || text[0] == '\0') {
        return 0;
    }
    if ((uint8_t)text[0] >= 0x80 && text[1] != '\0') {
        return 2;
    }
    return 1;
}


// vir rtc
static uint8_t test_rtc_is_leap_year(uint16_t year)
{
    return ((year % 4u) == 0 && (year % 100u) != 0) || ((year % 400u) == 0);
}

static uint8_t test_rtc_month_days(uint16_t year, uint8_t month)
{
    static const uint8_t month_days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31,
    };

    if (month == 2 && test_rtc_is_leap_year(year)) {
        return 29;
    }
    if (month < 1 || month > 12) {
        return 31;
    }
    return month_days[month - 1u];
}

static void test_rtc_add_seconds(struct sys_time *time, uint32_t seconds)
{
    if (time == NULL) {
        return;
    }

    time->sec += seconds % 60u;
    seconds /= 60u;
    if (time->sec >= 60u) {
        time->sec -= 60u;
        seconds++;
    }

    time->min += seconds % 60u;
    seconds /= 60u;
    if (time->min >= 60u) {
        time->min -= 60u;
        seconds++;
    }

    time->hour += seconds % 24u;
    seconds /= 24u;
    if (time->hour >= 24u) {
        time->hour -= 24u;
        seconds++;
    }

    while (seconds > 0) {
        uint8_t days = test_rtc_month_days(time->year, time->month);

        time->day++;
        if (time->day > days) {
            time->day = 1;
            time->month++;
            if (time->month > 12u) {
                time->month = 1;
                time->year++;
            }
        }
        seconds--;
    }
}

static void test_rtc_time(struct sys_time *time)
{
    static uint8_t init;
    static uint32_t start_ms;
    uint32_t now_ms;

    if (time == NULL) {
        return;
    }

    now_ms = timer_get_ms();
    if (!init) {
        start_ms = now_ms;
        init = 1;
    }

    time->year = 2026;
    time->month = 5;
    time->day = 25;
    time->hour = 0;
    time->min = 0;
    time->sec = 0;

    test_rtc_add_seconds(time, (now_ms - start_ms) / 1000u);
}
// vir rtc



static void app_camera_osd_get_time(app_camera_text_osd_t *text_osd, struct sys_time *time)
{
    (void)text_osd;

    if (time == NULL) {
        return;
    }
    memset(time, 0, sizeof(*time));
    test_rtc_time(time);

    //debug:
    //printf("time: %d-%d-%d %d:%d:%d \n",time->year,time->month,time->day,time->hour,time->min,time->sec);// vir rtc
}

uint8_t app_camera_osd_has_time(const char *text)
{
    if (text == NULL) {
        return 0;
    }

    return strstr(text, "yyyy-nn-dd") != NULL ||
           strstr(text, "yyyy/nn/dd") != NULL ||
           strstr(text, "hh:mm:ss") != NULL;
}


typedef struct {
    uint8_t utf8[3];
    uint8_t font[2];
} app_camera_osd_utf8_font_map_t;

static const app_camera_osd_utf8_font_map_t app_camera_osd_utf8_font_map[] = {
    { {0xE4, 0xBA, 0xAC}, {0xBE, 0xA9} },
    { {0xE5, 0xAE, 0x81}, {0xC4, 0xFE} },
    { {0xE5, 0xB7, 0x9D}, {0xB4, 0xA8} },
    { {0xE6, 0xB2, 0xAA}, {0xBB, 0xA6} },
    { {0xE6, 0xB4, 0xA5}, {0xBD, 0xF2} },
    { {0xE6, 0xB5, 0x99}, {0xD5, 0xE3} },
    { {0xE6, 0xB8, 0x9D}, {0xD3, 0xE5} },
    { {0xE6, 0xB9, 0x98}, {0xCF, 0xE6} },
    { {0xE7, 0xB2, 0xA4}, {0xD4, 0xC1} },
    { {0xE8, 0xB1, 0xAB}, {0xD4, 0xA5} },
    { {0xE8, 0xB4, 0xB5}, {0xB9, 0xF3} },
    { {0xE8, 0xB5, 0xA3}, {0xB8, 0xD3} },
    { {0xE8, 0xBE, 0xBD}, {0xC1, 0xC9} },
    { {0xE9, 0xB2, 0x81}, {0xC2, 0xB3} },
    { {0xE9, 0xBB, 0x91}, {0xBA, 0xDA} },
    { {0xE6, 0x96, 0xB0}, {0xD0, 0xC2} },
    { {0xE8, 0x8B, 0x8F}, {0xCB, 0xD5} },
    { {0xE8, 0x97, 0x8F}, {0xB2, 0xD8} },
    { {0xE9, 0x97, 0xBD}, {0xC3, 0xF6} },
    { {0xE6, 0x99, 0x8B}, {0xBD, 0xFA} },
    { {0xE7, 0x90, 0xBC}, {0xC7, 0xED} },
    { {0xE5, 0x86, 0x80}, {0xBC, 0xBD} },
    { {0xE5, 0x90, 0x89}, {0xBC, 0xAA} },
    { {0xE6, 0xA1, 0x82}, {0xB9, 0xF0} },
    { {0xE7, 0x94, 0x98}, {0xB8, 0xCA} },
    { {0xE7, 0x9A, 0x96}, {0xCD, 0xEE} },
    { {0xE8, 0x92, 0x99}, {0xC3, 0xC9} },
    { {0xE9, 0x84, 0x82}, {0xB6, 0xF5} },
    { {0xE9, 0x99, 0x95}, {0xC9, 0xC2} },
    { {0xE9, 0x9D, 0x92}, {0xC7, 0xE0} },
    { {0xE4, 0xBA, 0x91}, {0xD4, 0xC6} },
    { {0xE6, 0xB8, 0xAF}, {0xB8, 0xDB} },
    { {0xE6, 0xBE, 0xB3}, {0xB0, 0xC4} },
    { {0xE5, 0x8F, 0xB0}, {0xCC, 0xA8} },
};

static uint8_t app_camera_osd_utf8_len(uint8_t c)
{
    if ((c & 0x80u) == 0) {
        return 1;
    }
    if ((c & 0xE0u) == 0xC0u) {
        return 2;
    }
    if ((c & 0xF0u) == 0xE0u) {
        return 3;
    }
    if ((c & 0xF8u) == 0xF0u) {
        return 4;
    }
    return 0;
}

static uint8_t app_camera_osd_is_utf8_cont(uint8_t c)
{
    return (c & 0xC0u) == 0x80u;
}

static uint8_t app_camera_osd_is_font_gbk_char(const char *text)
{
    const uint8_t *table = (const uint8_t *)osd_str_total;
    uint32_t table_len = strlen(osd_str_total);
    uint32_t index;
    uint8_t c0;
    uint8_t c1;

    if (text == NULL || text[0] == '\0' || text[1] == '\0') {
        return 0;
    }

    c0 = (uint8_t)text[0];
    c1 = (uint8_t)text[1];
    for (index = 0; index + 1u < table_len; index++) {
        if (table[index] >= 0x80u) {
            if (table[index] == c0 && table[index + 1u] == c1) {
                return 1;
            }
            index++;
        }
    }
    return 0;
}

static const app_camera_osd_utf8_font_map_t *app_camera_osd_find_utf8_font(const char *text)
{
    uint32_t i;
    const uint8_t *src = (const uint8_t *)text;
    uint32_t map_count = sizeof(app_camera_osd_utf8_font_map) / sizeof(app_camera_osd_utf8_font_map[0]);

    if (text == NULL) {
        return NULL;
    }

    for (i = 0; i < map_count; i++) {
        const app_camera_osd_utf8_font_map_t *map = &app_camera_osd_utf8_font_map[i];

        if (src[0] == map->utf8[0] && src[1] == map->utf8[1] && src[2] == map->utf8[2]) {
            return map;
        }
    }
    return NULL;
}

static app_camera_ret_t app_camera_osd_text_to_font_text(char *out,
        uint32_t out_size,
        const char *in)
{
    uint32_t out_len = 0;
    const char *cursor = in;

    if (out == NULL || out_size == 0 || in == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    while (*cursor != '\0') {
        uint8_t c = (uint8_t) * cursor;

        if (c < 0x80u) {
            if (out_len + 1u >= out_size) {
                return APP_CAMERA_ERR_INVALID_ARG;
            }
            out[out_len++] = *cursor++;
            continue;
        }

        if (app_camera_osd_is_font_gbk_char(cursor)) {
            if (out_len + 2u >= out_size) {
                return APP_CAMERA_ERR_INVALID_ARG;
            }
            out[out_len++] = cursor[0];
            out[out_len++] = cursor[1];
            cursor += 2;
            continue;
        }

        if (app_camera_osd_utf8_len(c) == 3u &&
            cursor[1] != '\0' && cursor[2] != '\0' &&
            app_camera_osd_is_utf8_cont((uint8_t)cursor[1]) &&
            app_camera_osd_is_utf8_cont((uint8_t)cursor[2])) {
            const app_camera_osd_utf8_font_map_t *map = app_camera_osd_find_utf8_font(cursor);

            if (map == NULL) {
                return APP_CAMERA_ERR_NOT_SUPPORT;
            }
            if (out_len + 2u >= out_size) {
                return APP_CAMERA_ERR_INVALID_ARG;
            }
            out[out_len++] = (char)map->font[0];
            out[out_len++] = (char)map->font[1];
            cursor += 3;
            continue;
        }

        return APP_CAMERA_ERR_NOT_SUPPORT;
    }

    out[out_len] = '\0';
    return APP_CAMERA_OK;
}

static void app_camera_osd_append_time_value(char *out,
        uint32_t out_size,
        uint32_t *len,
        const char *fmt,
        unsigned int value)
{
    int ret;

    if (out == NULL || len == NULL || *len >= out_size) {
        return;
    }

    ret = snprintf(out + *len, out_size - *len, fmt, value);
    if (ret < 0) {
        return;
    }
    if ((uint32_t)ret >= out_size - *len) {
        *len = out_size - 1u;
    } else {
        *len += (uint32_t)ret;
    }
}

static void app_camera_osd_format_time_text(char *out,
        uint32_t out_size,
        const char *tmpl,
        const struct sys_time *time)
{
    uint32_t len = 0;
    const char *cursor = tmpl;

    if (out == NULL || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (tmpl == NULL || time == NULL) {
        return;
    }

    while (*cursor != '\0' && len + 1u < out_size) {
        if (strncmp(cursor, "yyyy", 4) == 0) {
            app_camera_osd_append_time_value(out, out_size, &len, "%04u", (unsigned int)time->year);
            cursor += 4;
        } else if (strncmp(cursor, "nn", 2) == 0) {
            app_camera_osd_append_time_value(out, out_size, &len, "%02u", (unsigned int)time->month);
            cursor += 2;
        } else if (strncmp(cursor, "dd", 2) == 0) {
            app_camera_osd_append_time_value(out, out_size, &len, "%02u", (unsigned int)time->day);
            cursor += 2;
        } else if (strncmp(cursor, "hh", 2) == 0) {
            app_camera_osd_append_time_value(out, out_size, &len, "%02u", (unsigned int)time->hour);
            cursor += 2;
        } else if (strncmp(cursor, "mm", 2) == 0) {
            app_camera_osd_append_time_value(out, out_size, &len, "%02u", (unsigned int)time->min);
            cursor += 2;
        } else if (strncmp(cursor, "ss", 2) == 0) {
            app_camera_osd_append_time_value(out, out_size, &len, "%02u", (unsigned int)time->sec);
            cursor += 2;
        } else {
            out[len++] = *cursor++;
            out[len] = '\0';
        }
    }
    out[out_size - 1u] = '\0';
}

app_camera_ret_t app_camera_osd_text_format(app_camera_text_osd_t *text_osd,
        const char *text)
{
    if (text_osd == NULL || text == NULL || text[0] == '\0') {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    text_osd->has_time = app_camera_osd_has_time(text);
    return app_camera_osd_text_to_font_text(text_osd->text, sizeof(text_osd->text), text);
}

static const char *app_camera_osd_render_text(app_camera_text_osd_t *text_osd,
        char *render_text,
        uint32_t render_text_size)
{
    struct sys_time time;

    if (text_osd == NULL) {
        return NULL;
    }
    if (!text_osd->has_time) {
        return text_osd->text;
    }

    app_camera_osd_get_time(text_osd, &time);
    app_camera_osd_format_time_text(render_text, render_text_size, text_osd->text, &time);
    return render_text;
}

static app_camera_ret_t app_camera_osd_measure_text(const char *text,
        uint16_t *width,
        uint16_t *height)
{
    uint32_t max_chars = 0;
    uint32_t line_chars = 0;
    uint32_t lines = 1;
    const char *cursor = text;

    if (text == NULL || text[0] == '\0' || width == NULL || height == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (strlen(text) > APP_CAMERA_OSD_TEXT_MAX) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    while (*cursor != '\0') {
        uint8_t used;

        if (*cursor == '\n') {
            if (line_chars > max_chars) {
                max_chars = line_chars;
            }
            line_chars = 0;
            lines++;
            cursor++;
            continue;
        }

        used = app_camera_osd_char_bytes(cursor);
        if (used == 0) {
            break;
        }
        line_chars += used;
        cursor += used;
    }

    if (line_chars > max_chars) {
        max_chars = line_chars;
    }
    if (max_chars == 0 || max_chars > UINT16_MAX / APP_CAMERA_OSD_FONT_W ||
        lines > UINT16_MAX / APP_CAMERA_OSD_FONT_H) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    *width = app_camera_osd_align_up((uint16_t)(max_chars * APP_CAMERA_OSD_FONT_W),
                                     APP_CAMERA_OSD_WIDTH_ALIGN);
    *height = (uint16_t)(lines * APP_CAMERA_OSD_FONT_H);
    return APP_CAMERA_OK;
}

static const uint8_t *app_camera_osd_find_glyph(const char *text, uint8_t *used)
{
    const uint8_t *table = (const uint8_t *)osd_str_total;
    const uint8_t *matrix = (const uint8_t *)osd2_str_matrix;
    uint32_t table_len = strlen(osd_str_total);
    uint32_t glyph_size;
    uint8_t c0;
    uint8_t c1;
    uint32_t index;

    if (used == NULL) {
        return NULL;
    }
    *used = app_camera_osd_char_bytes(text);
    if (*used == 0 || table_len == 0) {
        return NULL;
    }

    glyph_size = sizeof(osd2_str_matrix) / table_len;
    c0 = (uint8_t)text[0];
    c1 = (uint8_t)text[1];

    for (index = 0; index < table_len; index++) {
        if (table[index] == c0) {
            if (c0 < 0x80 || (index + 1 < table_len && table[index + 1] == c1)) {
                uint32_t offset = index * glyph_size;

                return (offset + glyph_size * *used <= sizeof(osd2_str_matrix)) ?
                       (matrix + offset) : NULL;
            }
        }
        if (table[index] >= 0x80 && index + 1 < table_len) {
            index++;
        }
    }

    return NULL;
}

static void app_camera_osd_draw_glyph(app_camera_osd_bitmap_t *bitmap,
                                      uint16_t x,
                                      uint16_t y,
                                      const uint8_t *glyph,
                                      uint8_t used)
{
    uint16_t row;
    uint16_t glyph_width = APP_CAMERA_OSD_FONT_W * used;
    uint16_t glyph_stride = glyph_width * APP_CAMERA_OSD_BPP / 8u;

    if (bitmap == NULL || bitmap->buf == NULL || glyph == NULL || used == 0) {
        return;
    }

    for (row = 0; row < APP_CAMERA_OSD_FONT_H; row++) {
        memcpy(bitmap->buf + (uint32_t)(y + row) * bitmap->stride +
               (uint32_t)x * APP_CAMERA_OSD_BPP / 8u,
               glyph + (uint32_t)row * glyph_stride,
               glyph_stride);
    }
}

static void app_camera_osd_bitmap_render(app_camera_osd_bitmap_t *bitmap, const char *text)
{
    const char *cursor;
    uint16_t x = 0;
    uint16_t y = 0;

    if (bitmap == NULL || bitmap->buf == NULL || text == NULL) {
        return;
    }

    memset(bitmap->buf, 0, bitmap->buf_size);
    cursor = text;
    while (*cursor != '\0') {
        const uint8_t *glyph;
        uint8_t used;

        if (*cursor == '\n') {
            x = 0;
            y += APP_CAMERA_OSD_FONT_H;
            cursor++;
            continue;
        }

        glyph = app_camera_osd_find_glyph(cursor, &used);
        if (used == 0) {
            break;
        }
        app_camera_osd_draw_glyph(bitmap, x, y, glyph, used);
        x += APP_CAMERA_OSD_FONT_W * used;
        cursor += used;
    }
}

static app_camera_ret_t app_camera_osd_bitmap_alloc(app_camera_osd_bitmap_t *bitmap,
        uint16_t width,
        uint16_t height)
{
    uint32_t alloc_size;
    uintptr_t aligned;

    if (bitmap == NULL || width == 0 || height == 0) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    bitmap->width = width;
    bitmap->height = height;
    bitmap->stride = width * APP_CAMERA_OSD_BPP / 8u;
    bitmap->buf_size = (uint32_t)bitmap->stride * height;
    alloc_size = bitmap->buf_size + APP_CAMERA_OSD_BUF_ALIGN - 1u;
    bitmap->buf_base = (uint8_t *)malloc(alloc_size);
    if (bitmap->buf_base == NULL) {
        memset(bitmap, 0, sizeof(*bitmap));
        return APP_CAMERA_ERR_NOMEM;
    }

    aligned = ((uintptr_t)bitmap->buf_base + APP_CAMERA_OSD_BUF_ALIGN - 1u) &
              ~((uintptr_t)APP_CAMERA_OSD_BUF_ALIGN - 1u);
    bitmap->buf = (uint8_t *)aligned;
    memset(bitmap->buf, 0, bitmap->buf_size);
    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_osd_text_parse(const char *text,
        app_camera_osd_bitmap_t *bitmap)
{
    uint16_t width;
    uint16_t height;
    app_camera_ret_t ret;

    if (bitmap == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    ret = app_camera_osd_measure_text(text, &width, &height);
    if (ret != APP_CAMERA_OK) {
        return ret;
    }
    ret = app_camera_osd_bitmap_alloc(bitmap, width, height);
    if (ret != APP_CAMERA_OK) {
        return ret;
    }

    app_camera_osd_bitmap_render(bitmap, text);
    return APP_CAMERA_OK;
}

void app_camera_osd_bitmap_free(app_camera_osd_bitmap_t *bitmap)
{
    if (bitmap == NULL) {
        return;
    }
    if (bitmap->buf_base != NULL) {
        free(bitmap->buf_base);
    }
    memset(bitmap, 0, sizeof(*bitmap));
}

static uint8_t app_camera_osd_bitmap_size_match(app_camera_text_osd_t *text_osd,
        uint16_t width,
        uint16_t height)
{
    uint8_t i;

    if (text_osd == NULL) {
        return 0;
    }

    for (i = 0; i < APP_CAMERA_OSD_BUF_COUNT; i++) {
        app_camera_osd_bitmap_t *bitmap = &text_osd->bitmap[i];

        if (bitmap->buf == NULL || bitmap->width != width || bitmap->height != height) {
            return 0;
        }
    }
    return 1;
}

static app_camera_ret_t app_camera_osd_alloc_bitmaps(app_camera_text_osd_t *text_osd,
        uint16_t width,
        uint16_t height)
{
    uint8_t i;
    app_camera_ret_t ret;

    if (app_camera_osd_bitmap_size_match(text_osd, width, height)) {
        return APP_CAMERA_OK;
    }

    app_camera_osd_free_bitmap(text_osd);
    for (i = 0; i < APP_CAMERA_OSD_BUF_COUNT; i++) {
        ret = app_camera_osd_bitmap_alloc(&text_osd->bitmap[i], width, height);
        if (ret != APP_CAMERA_OK) {
            app_camera_osd_free_bitmap(text_osd);
            return ret;
        }
        text_osd->bitmap[i].x = app_camera_osd_align_up(text_osd->x, 16u);
        text_osd->bitmap[i].y = app_camera_osd_align_up(text_osd->y, 4u);
    }
    text_osd->buf_index = 0;
    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_osd_render_to_index(app_camera_text_osd_t *text_osd,
        uint8_t index)
{
    char render_text[APP_CAMERA_OSD_TEXT_MAX + 1u];
    const char *text;

    if (text_osd == NULL || index >= APP_CAMERA_OSD_BUF_COUNT || text_osd->text[0] == '\0') {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    text = app_camera_osd_render_text(text_osd, render_text, sizeof(render_text));
    if (text == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    app_camera_osd_bitmap_render(&text_osd->bitmap[index], text);
    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_osd_create_bitmap(app_camera_text_osd_t *text_osd)
{
    char render_text[APP_CAMERA_OSD_TEXT_MAX + 1u];
    const char *text;
    uint16_t width;
    uint16_t height;
    app_camera_ret_t ret;

    if (text_osd == NULL || text_osd->text[0] == '\0') {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    text = app_camera_osd_render_text(text_osd, render_text, sizeof(render_text));
    if (text == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    ret = app_camera_osd_measure_text(text, &width, &height);
    if (ret != APP_CAMERA_OK) {
        return ret;
    }

    ret = app_camera_osd_alloc_bitmaps(text_osd, width, height);
    if (ret != APP_CAMERA_OK) {
        return ret;
    }

    return app_camera_osd_render_to_index(text_osd, text_osd->buf_index);
}

void app_camera_osd_free_bitmap(app_camera_text_osd_t *text_osd)
{
    uint8_t i;

    if (text_osd == NULL) {
        return;
    }
    for (i = 0; i < APP_CAMERA_OSD_BUF_COUNT; i++) {
        app_camera_osd_bitmap_free(&text_osd->bitmap[i]);
    }
    text_osd->buf_index = 0;
}

app_camera_ret_t app_camera_osd_update_bitmap(app_camera_text_osd_t *text_osd,
        app_camera_osd_bitmap_t **bitmap)
{
    uint8_t next_index;
    app_camera_ret_t ret;

    if (text_osd == NULL || bitmap == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (text_osd->text[0] == '\0') {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    *bitmap = NULL;
    next_index = text_osd->buf_index ^ 1u;
    if (text_osd->bitmap[next_index].buf == NULL) {
        return APP_CAMERA_ERR_NOT_READY;
    }

    ret = app_camera_osd_render_to_index(text_osd, next_index);
    if (ret != APP_CAMERA_OK) {
        return ret;
    }

    text_osd->buf_index = next_index;
    *bitmap = &text_osd->bitmap[text_osd->buf_index];
    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_osd_make_vpss_attr(app_camera_osd_bitmap_t *bitmap,
        mpi_vpss_osd_attr_t *attr)
{
    if (bitmap == NULL || attr == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (bitmap->buf == NULL) {
        return APP_CAMERA_ERR_NOT_READY;
    }

    memset(attr, 0, sizeof(*attr));
    attr->mode = MPI_VPSS_OSD_MODE_2BIT;
    attr->x = bitmap->x;
    attr->y = bitmap->y;
    attr->width = bitmap->width;
    attr->height = bitmap->height;
    attr->color[0] = APP_CAMERA_OSD_COLOR0;
    attr->color[1] = APP_CAMERA_OSD_COLOR_TEXT;
    attr->color[2] = APP_CAMERA_OSD_COLOR_TEXT;
    attr->color[3] = APP_CAMERA_OSD_COLOR_TEXT;
    attr->buf_addr = bitmap->buf;
    attr->buf_size = bitmap->buf_size;
    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_osd_set_vpss_attr(app_camera_handle_t ctx,
        mpi_vpss_chn_t vpss_chn,
        app_camera_osd_bitmap_t *bitmap,
        uint8_t update)
{
    mpi_vpss_osd_attr_t attr;
    mpi_ret_t ret;
    app_camera_ret_t app_ret;

    if (ctx == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    app_ret = app_camera_osd_make_vpss_attr(bitmap, &attr);
    if (app_ret != APP_CAMERA_OK) {
        return app_ret;
    }

    if (update) {
        ret = mpi_vpss_update_chn_osd_attr(ctx->vpss_grp, vpss_chn, &attr);
    } else {
        ret = mpi_vpss_set_chn_osd_attr(ctx->vpss_grp, vpss_chn, &attr);
    }
    if (ret != MPI_SUCCESS) {
        printf("failed to %s OSD attr, ret=%d\n", update ? "update" : "set", ret);
        return APP_CAMERA_ERR_IO;
    }

    return APP_CAMERA_OK;
}

static void app_camera_osd_timer(void *priv)
{
    app_camera_text_osd_t *text_osd = (app_camera_text_osd_t *)priv;
    app_camera_osd_bitmap_t *bitmap = NULL;
    app_camera_ret_t ret;

    if (text_osd == NULL || text_osd->owner == NULL ||
        !text_osd->enable || !text_osd->has_time) {
        return;
    }

    ret = app_camera_osd_update_bitmap(text_osd, &bitmap);
    if (ret != APP_CAMERA_OK) {
        return;
    }

    (void)app_camera_osd_set_vpss_attr(text_osd->owner, text_osd->vpss_chn, bitmap, 1);
}

static app_camera_ret_t app_camera_osd_start_timer(app_camera_handle_t ctx,
        app_camera_text_osd_t *text_osd,
        mpi_vpss_chn_t vpss_chn)
{
    if (ctx == NULL || text_osd == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (!text_osd->enable || !text_osd->has_time) {
        return APP_CAMERA_OK;
    }
    if (text_osd->timer != 0) {
        return APP_CAMERA_OK;
    }

    text_osd->owner = ctx;
    text_osd->vpss_chn = vpss_chn;
    text_osd->timer = sys_timer_add(text_osd, app_camera_osd_timer, APP_CAMERA_OSD_REFRESH_MS);
    if (text_osd->timer == 0) {
        return APP_CAMERA_ERR_IO;
    }

    return APP_CAMERA_OK;
}

void app_camera_osd_stop_timer(app_camera_text_osd_t *text_osd)
{
    if (text_osd == NULL) {
        return;
    }
    if (text_osd->timer != 0) {
        sys_timer_del(text_osd->timer);
        text_osd->timer = 0;
    }
}

app_camera_ret_t app_camera_osd_set_text(app_camera_text_osd_t *text_osd,
        const app_camera_osd_attr_t *attr)
{
    app_camera_ret_t ret;

    if (text_osd == NULL || attr == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (text_osd->enable == 1) {
        return APP_CAMERA_ERR_BUSY;
    }

    ret = app_camera_osd_text_format(text_osd, attr->text);
    if (ret != APP_CAMERA_OK) {
        return ret;
    }

    text_osd->x = attr->x;
    text_osd->y = attr->y;
    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_osd_apply(app_camera_handle_t ctx,
                                      app_camera_text_osd_t *text_osd,
                                      mpi_vpss_chn_t vpss_chn)
{
    mpi_ret_t mpi_ret;
    app_camera_ret_t ret;

    if (ctx == NULL || text_osd == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (!text_osd->enable) {
        return APP_CAMERA_OK;
    }

    text_osd->owner = ctx;
    text_osd->vpss_chn = vpss_chn;

    ret = app_camera_osd_create_bitmap(text_osd);
    if (ret != APP_CAMERA_OK) {
        goto err_out;
    }

    ret = app_camera_osd_set_vpss_attr(ctx, vpss_chn,
                                       &text_osd->bitmap[text_osd->buf_index], 0);
    if (ret != APP_CAMERA_OK) {
        goto err_out;
    }

    mpi_ret = mpi_vpss_enable_chn_osd(ctx->vpss_grp, vpss_chn);
    if (mpi_ret != MPI_SUCCESS) {
        printf("failed to enable OSD, ret=%d\n", mpi_ret);
        ret = APP_CAMERA_ERR_IO;
        goto err_out;
    }

    ret = app_camera_osd_start_timer(ctx, text_osd, vpss_chn);
    if (ret != APP_CAMERA_OK) {
        (void)mpi_vpss_disable_chn_osd(ctx->vpss_grp, vpss_chn);
        goto err_out;
    }

    return APP_CAMERA_OK;

err_out:
    app_camera_osd_stop_timer(text_osd);
    app_camera_osd_free_bitmap(text_osd);
    return ret;
}

app_camera_ret_t app_camera_osd_enable(app_camera_handle_t ctx,
                                       app_camera_text_osd_t *text_osd,
                                       mpi_vpss_chn_t vpss_chn,
                                       uint8_t vpss_started)
{
    app_camera_ret_t ret;

    if (ctx == NULL || text_osd == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (text_osd->enable == 1) {
        return APP_CAMERA_ERR_EXIST;
    }

    text_osd->enable = 1;
    text_osd->owner = ctx;
    text_osd->vpss_chn = vpss_chn;

    if (!vpss_started) {
        return APP_CAMERA_OK;
    }

    ret = app_camera_osd_apply(ctx, text_osd, vpss_chn);
    if (ret != APP_CAMERA_OK) {
        text_osd->enable = 0;
    }
    return ret;
}

app_camera_ret_t app_camera_osd_disable(app_camera_handle_t ctx,
                                        app_camera_text_osd_t *text_osd,
                                        mpi_vpss_chn_t vpss_chn)
{
    mpi_ret_t ret;

    if (ctx == NULL || text_osd == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    u8 was_enabled = text_osd->enable;
    text_osd->enable = 0;
    app_camera_osd_stop_timer(text_osd);

    /* OSD 未启用过时跳过 mpi_vpss_disable_chn_osd:
     * 其内部 wait dst idle 会等当前帧扫描完成(最多 1s 超时),
     * 没有启用过的 OSD 白等, 拖慢 photo->rec 切换逾 1s。 */
    if (was_enabled) {
        ret = mpi_vpss_disable_chn_osd(ctx->vpss_grp, vpss_chn);
        if (ret != MPI_SUCCESS) {
            printf("failed to disable OSD, ret=%d\n", ret);
        }
    }

    app_camera_osd_free_bitmap(text_osd);
    return APP_CAMERA_OK;
}
