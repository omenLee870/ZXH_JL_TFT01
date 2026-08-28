/**
 * @file prop.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "prop.h"
#include "dyn_reader.h"
#include "dyn_utils.h"
#include "res.h"
#include <string.h>
#include <time.h>

#if LV_USE_GUI_PARSE_DYN != 0

/*********************
 *      DEFINES
 *********************/

static const char *scaleclock_hour_ticks[] = {"12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", NULL};

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool read_i32(const uint8_t **p, const uint8_t *end, int32_t *out);
static const char *dropdown_symbol_for_dir(int32_t dir);
static void control_deleted_event_cb(lv_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool prop_set_item(uint32_t prop_id, dyn_view_t payload, lv_obj_t *obj, int32_t page_id)
{
    const uint8_t *p = payload.ptr;
    const uint8_t *end = payload.ptr + payload.len;
    int32_t v1;
    int32_t v2;
    int32_t v3;

    switch (prop_id) {
    case LV_OBJ_SET_POS_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_obj_set_pos(obj, v1, v2);
        break;
    case LV_OBJ_SET_SIZE_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_obj_set_size(obj, v1, v2);
        break;
    case LV_OBJ_SET_X_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_set_x(obj, v1);
        break;
    case LV_OBJ_SET_Y_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_set_y(obj, v1);
        break;
    case LV_OBJ_SET_WIDTH_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_set_width(obj, v1);
        break;
    case LV_OBJ_SET_HEIGHT_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_set_height(obj, v1);
        break;
    case LV_OBJ_SET_USER_DATA_ID: {
        char *data = NULL;
        if (!dyn_read_string_alloc(&p, end, &data)) {
            return false;
        }
        lv_obj_set_user_data(obj, data);
        lv_obj_add_event_cb(obj, control_deleted_event_cb, LV_EVENT_DELETE, data);
    }
    break;
    case LV_OBJ_ADD_STATE_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_add_state(obj, v1);
        break;
    case LV_OBJ_ADD_FLAG_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_add_flag(obj, v1);
        break;
    case LV_OBJ_CLEAR_FLAG_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_remove_flag(obj, v1);
        break;
    case LV_OBJ_ALIGN_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2) || !read_i32(&p, end, &v3)) {
            return false;
        }
        lv_obj_align(obj, v1, v2, v3);
        break;

    case LV_LABEL_SET_TEXT_ID:
    case LV_LABEL_SET_TEXT_FMT_ID: {
        char *text = NULL;
        if (!dyn_read_string_alloc(&p, end, &text)) {
            return false;
        }
        lv_label_set_text(obj, text);
        lv_free(text);
    }
    break;
    case LV_LABEL_SET_LONG_MODE_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_label_set_long_mode(obj, v1);
        break;
    case LV_LABEL_SET_RECOLOR_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_label_set_recolor(obj, v1);
        break;

#if LV_USE_CHECKBOX
    case LV_CHECKBOX_SET_TEXT_ID: {
        char *text = NULL;
        if (!dyn_read_string_alloc(&p, end, &text)) {
            return false;
        }
        lv_checkbox_set_text(obj, text);
        lv_free(text);
    }
    break;
#endif

    case LV_IMG_SET_SRC_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_image_set_src(obj, res_get(page_id, v1));
        break;
    case LV_IMG_SET_PIVOT_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_image_set_pivot(obj, v1, v2);
        break;
    case LV_IMG_SET_ANGLE_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_image_set_rotation(obj, v1);
        break;
    case LV_IMG_SET_ZOOM_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_image_set_scale(obj, v1);
        break;
    case LV_IMG_SET_OFFSET_X_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_image_set_offset_x(obj, v1);
        break;
    case LV_IMG_SET_OFFSET_Y_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_image_set_offset_y(obj, v1);
        break;

#if LV_USE_SCALE
    case LV_SCALECLOCK_SET_LABEL_ENABLE_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_scale_set_label_show(obj, v1 == 1);
        if (v1 == 1) {
            lv_scale_set_text_src(obj, scaleclock_hour_ticks);
        }
        break;
    case LV_SCALECLOCK_SET_TOTAL_TICK_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_scale_set_total_tick_count(obj, v1);
        break;
    case LV_SCALECLOCK_SET_MAJOR_TICK_EVERY_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_scale_set_major_tick_every(obj, v1);
        break;
    case LV_SCALECLOCK_SET_HOUR_NEEDLE_LINE_ID:
    case LV_SCALECLOCK_SET_MIN_NEEDLE_LINE_ID:
    case LV_SCALECLOCK_SET_SEC_NEEDLE_LINE_ID: {
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2) || !read_i32(&p, end, &v3)) {
            return false;
        }
        lv_obj_t *line = lv_line_create(obj);
        lv_obj_set_style_line_width(line, v1, LV_PART_MAIN);
        lv_obj_set_style_line_color(line, lv_color_hex(v2), LV_PART_MAIN);
        lv_obj_set_style_line_rounded(line, true, LV_PART_MAIN);
        lv_obj_update_layout(obj);
        lv_scale_set_line_needle_value(obj, line, v3, 0);
        lv_obj_set_user_data(line, (void *)(intptr_t)v3);
    }
    break;
    case LV_SCALECLOCK_SET_TIME_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2) || !read_i32(&p, end, &v3)) {
            return false;
        }
        if (lv_obj_get_child_count(obj) >= 1) {
            lv_obj_t *hour_nd = lv_obj_get_child(obj, 0);
            int32_t hour_val = (v1 % 12) * 60 + v2;
            if (lv_obj_check_type(hour_nd, &lv_image_class)) {
                lv_scale_set_image_needle_value(obj, hour_nd, hour_val);
            } else {
                lv_scale_set_line_needle_value(obj, hour_nd, (int32_t)(intptr_t)lv_obj_get_user_data(hour_nd),
                                               hour_val);
            }
        }
        if (lv_obj_get_child_count(obj) >= 2) {
            lv_obj_t *min_nd = lv_obj_get_child(obj, 1);
            int32_t min_val = v2 * 12 + v3 / 5;
            if (lv_obj_check_type(min_nd, &lv_image_class)) {
                lv_scale_set_image_needle_value(obj, min_nd, min_val);
            } else {
                lv_scale_set_line_needle_value(obj, min_nd, (int32_t)(intptr_t)lv_obj_get_user_data(min_nd),
                                               min_val);
            }
        }
        if (lv_obj_get_child_count(obj) >= 3) {
            lv_obj_t *sec_nd = lv_obj_get_child(obj, 2);
            int32_t sec_val = v3 * 12;
            if (lv_obj_check_type(sec_nd, &lv_image_class)) {
                lv_scale_set_image_needle_value(obj, sec_nd, sec_val);
            } else {
                lv_scale_set_line_needle_value(obj, sec_nd, (int32_t)(intptr_t)lv_obj_get_user_data(sec_nd),
                                               sec_val);
            }
        }
        break;
    case LV_SCALECLOCK_SET_DIAL_IMG_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_set_style_bg_image_src(obj, res_get(page_id, v1), LV_PART_MAIN);
        break;
    case LV_SCALECLOCK_SET_HOUR_NEEDLE_IMG_ID:
    case LV_SCALECLOCK_SET_MIN_NEEDLE_IMG_ID:
    case LV_SCALECLOCK_SET_SEC_NEEDLE_IMG_ID: {
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2) || !read_i32(&p, end, &v3)) {
            return false;
        }
        lv_obj_update_layout(obj);
        lv_obj_t *img = lv_image_create(obj);
        lv_image_set_src(img, res_get(page_id, v1));
        lv_obj_align(img, LV_ALIGN_CENTER, lv_image_get_src_width(img) / 2 - v2,
                     lv_image_get_src_height(img) / 2 - v3);
        lv_image_set_pivot(img, v2, v3);
        lv_scale_set_image_needle_value(obj, img, 0);
    }
    break;
#endif

    case LV_IMGLIST_SET_SRC_ID: {
        int32_t size;
        if (!read_i32(&p, end, &size) || size < 0) {
            return false;
        }
        void **src = lv_malloc(size * sizeof(void *));
        if (src == NULL && size > 0) {
            return false;
        }
        lv_memset(src, 0, size * sizeof(void *));
        for (int i = 0; i < size; i++) {
            if (!read_i32(&p, end, &v1)) {
                lv_free(src);
                return false;
            }
            src[i] = res_get(page_id, v1);
        }
        lv_imglist_set_src(obj, (const void **)src, size);
        lv_obj_add_event_cb(obj, control_deleted_event_cb, LV_EVENT_DELETE, src);
    }
    break;
    case LV_IMGLIST_SET_ACT_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_imglist_set_act(obj, v1);
        break;

    case LV_LINE_SET_POINTS_ID: {
        int32_t size;
        if (!read_i32(&p, end, &size) || size < 0) {
            return false;
        }
        lv_point_precise_t *points = lv_malloc(size * sizeof(lv_point_precise_t));
        if (points == NULL && size > 0) {
            return false;
        }
        lv_memset(points, 0, size * sizeof(lv_point_precise_t));
        for (int i = 0; i < size; i++) {
            if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
                lv_free(points);
                return false;
            }
            points[i].x = v1;
            points[i].y = v2;
        }
        lv_line_set_points(obj, points, size);
        lv_obj_add_event_cb(obj, control_deleted_event_cb, LV_EVENT_DELETE, points);
    }
    break;

#if LV_USE_BAR
    case LV_BAR_SET_MODE_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_bar_set_mode(obj, v1);
        break;
    case LV_BAR_SET_RANGE_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_bar_set_range(obj, v1, v2);
        break;
    case LV_BAR_SET_VALUE_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_bar_set_value(obj, v1, v2);
        break;
    case LV_BAR_SET_START_VALUE_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_bar_set_start_value(obj, v1, v2);
        break;
    case LV_BAR_SET_ANIM_TIME_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_obj_set_style_anim_duration(obj, v1, 0);
        break;
#endif

#if LV_USE_SLIDER
    case LV_SLIDER_SET_RANGE_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_slider_set_range(obj, v1, v2);
        break;
    case LV_SLIDER_SET_MODE_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_slider_set_mode(obj, v1);
        break;
    case LV_SLIDER_SET_VALUE_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_slider_set_value(obj, v1, v2);
        break;
    case LV_SLIDER_SET_LEFT_VALUE_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_slider_set_start_value(obj, v1, v2);
        break;
#endif

#if LV_USE_LED
    case LV_LED_SET_BRIGHT_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_led_set_brightness(obj, (uint8_t)v1);
        break;
    case LV_LED_SET_COLOR_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_led_set_color(obj, lv_color_hex(v1));
        break;
#endif

#if LV_USE_DROPDOWN
    case LV_DROPDOWN_SET_OPTIONS_ID: {
        char *text = NULL;
        if (!dyn_read_string_alloc(&p, end, &text)) {
            return false;
        }
        lv_dropdown_set_options(obj, text);
        lv_free(text);
    }
    break;
    case LV_DROPDOWN_SET_DIR_SYMBOL_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_dropdown_set_dir(obj, v1);
        lv_dropdown_set_symbol(obj, v2 ? dropdown_symbol_for_dir(v1) : NULL);
        break;
    case LV_DROPDOWN_SET_SELECTED_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_dropdown_set_selected(obj, v1);
        break;
#endif

#if LV_USE_SPINNER
    case LV_SPINNER_SET_ANIM_PARAMS_ID:
        if (!read_i32(&p, end, &v1) || !read_i32(&p, end, &v2)) {
            return false;
        }
        lv_spinner_set_anim_params(obj, v1, v2);
        break;
#endif

    case LV_DIGITCLOCK_SET_TIME_ID: {
        char *fmt = NULL;
        int32_t year;
        int32_t month;
        int32_t day;
        int32_t hour;
        int32_t min;
        int32_t sec;
        if (!dyn_read_string_alloc(&p, end, &fmt)) {
            return false;
        }
        if (!read_i32(&p, end, &year) || !read_i32(&p, end, &month) || !read_i32(&p, end, &day) ||
            !read_i32(&p, end, &hour) || !read_i32(&p, end, &min) || !read_i32(&p, end, &sec)) {
            lv_free(fmt);
            return false;
        }

        /* digitclock 协议使用自然年月日，strftime 需要 struct tm 的偏移字段。 */
        struct tm time;
        lv_memset(&time, 0, sizeof(time));
        time.tm_year = year - 1900;
        time.tm_mon = month - 1;
        time.tm_mday = day;
        time.tm_hour = hour;
        time.tm_min = min;
        time.tm_sec = sec;
        dyn_digitclock_set_text(obj, fmt, &time);
        lv_free(fmt);
    }
    break;

#if LV_USE_ANIMIMG
    case LV_ANIMIMG_SET_DURATION_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_animimg_set_duration(obj, v1);
        break;
    case LV_ANIMIMG_SET_REPEAT_COUNT_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        lv_animimg_set_repeat_count(obj, v1);
        break;
    case LV_ANIMIMG_SET_SRC_ID: {
        int32_t size;
        if (!read_i32(&p, end, &size) || size < 0) {
            return false;
        }
        void **src = lv_malloc(size * sizeof(void *));
        if (src == NULL && size > 0) {
            return false;
        }
        lv_memset(src, 0, size * sizeof(void *));
        for (int i = 0; i < size; i++) {
            if (!read_i32(&p, end, &v1)) {
                lv_free(src);
                return false;
            }
            src[i] = res_get(page_id, v1);
        }
        lv_animimg_set_src(obj, (const void **)src, size);
        lv_obj_add_event_cb(obj, control_deleted_event_cb, LV_EVENT_DELETE, src);
    }
    break;
    case LV_ANIMIMG_SET_DEFAULT_ENABLE_ID:
        if (!read_i32(&p, end, &v1)) {
            return false;
        }
        if (v1) {
            lv_animimg_start(obj);
        }
        break;
#endif

    default:
        LV_LOG_WARN("unknown prop item %d", (int)prop_id);
        return true;
    }

    if (p != end) {
        LV_LOG_WARN("prop payload size mismatch %d", (int)prop_id);
        return false;
    }
    return true;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool read_i32(const uint8_t **p, const uint8_t *end, int32_t *out)
{
    uint32_t value;
    if (!dyn_read_le32(p, end, &value)) {
        return false;
    }
    *out = (int32_t)value;
    return true;
}

static const char *dropdown_symbol_for_dir(int32_t dir)
{
    switch (dir) {
    case LV_DIR_TOP:
        return LV_SYMBOL_UP;
    case LV_DIR_LEFT:
        return LV_SYMBOL_LEFT;
    case LV_DIR_RIGHT:
        return LV_SYMBOL_RIGHT;
    case LV_DIR_BOTTOM:
    default:
        return LV_SYMBOL_DOWN;
    }
}

/**
 * @brief 控件删除事件回调，用于释放属性解析时为控件分配的附加数据。
 *
 * @param e LVGL 删除事件
 */
static void control_deleted_event_cb(lv_event_t *e)
{
    void *user_data = lv_event_get_user_data(e);
    if (user_data != NULL) {
        lv_free(user_data);
    }
}

#endif
