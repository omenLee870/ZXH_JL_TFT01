/**
 * @file bind.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "bind.h"
#include "dyn_reader.h"
#include "dyn_utils.h"
#include "../../../../core/lv_observer_private.h"
#include <string.h>

#if LV_USE_GUI_PARSE_DYN != 0

#include "time.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef union {
    int32_t value_int;
    bool value_bool;
    char *value_string;
    lv_color_t value_color;
    lv_calendar_date_t value_date;
    lv_point_t value_point;
    struct tm value_time;
    struct {
        void *ptr;
        int32_t len;
    } value_array;
} gui_msg_data_t;

typedef enum {
    VALUE_INT = 1,
    VALUE_CHAR,
    VALUE_SHORT,
    VALUE_COORD,
    VALUE_BOOL,
    VALUE_STRING,
    VALUE_COLOR,
    VALUE_DATE,
    VALUE_POINT,
    VALUE_TIME,
    VALUE_ARRAY,
    VALUE_UNKNOWN = 0,
} gui_msg_data_type_t;

typedef struct {
    gui_msg_data_t *data;
    char fmt[];
} digitclock_bind_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void obj_x_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_y_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_width_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_height_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_user_data_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_toggle_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_state_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_flag_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_clear_flag_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void obj_visible_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static digitclock_bind_ctx_t *digitclock_bind_ctx_create(gui_msg_data_t *data, const char *fmt);
static void bind_add_model_observer(int32_t mode, lv_subject_t *subject, lv_observer_cb_t observer_cb, lv_obj_t *obj);
static bool bind_observer_get_context(lv_observer_t *observer, lv_obj_t **obj, gui_msg_data_t **data);
static bool bind_event_get_send_context(lv_event_t *e, lv_obj_t **obj, int32_t *model_id,
                                        gui_bind_send_cb_t *send_cb);

#if LV_USE_LABEL
static void label_text_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void digitclock_time_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
#endif

#if LV_USE_CHECKBOX || LV_USE_SWITCH
static void checked_state_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void checked_state_event_cb(lv_event_t *e);
#endif

#if LV_USE_CHECKBOX
static void checkbox_text_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
#endif

#if LV_USE_BUTTON
#endif

#if LV_USE_IMAGE
static void img_pivot_x_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void img_pivot_y_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void img_rotate_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void img_zoom_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
#endif

#if LV_USE_SCALE
static void scale_time_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
#endif

#if LV_USE_IMGLIST
static void imglist_act_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
#endif

#if LV_USE_LINE
static void line_points_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
#endif

#if LV_USE_BAR
static void bar_value_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
#endif

#if LV_USE_SLIDER
static void slider_value_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void slider_left_value_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void slider_value_event_cb(lv_event_t *e);
static void slider_left_value_event_cb(lv_event_t *e);
#endif

#if LV_USE_LED
static void led_bright_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void led_color_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
#endif

#if LV_USE_DROPDOWN
static void dropdown_options_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void dropdown_selected_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void dropdown_selected_event_cb(lv_event_t *e);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

static gui_bind_send_cb_t gui_msg_send = NULL;
static gui_bind_get_cb_t gui_msg_get = NULL;
static gui_bind_get_subject_cb_t gui_msg_get_subject = NULL;
static gui_bind_get_data_cb_t gui_msg_get_data = NULL;

/**********************
 *      MACROS
 **********************/

#define ONE_WAY_MODEL_TO_VIEW_ID 0x01
#define ONE_WAY_VIEW_TO_MODEL_ID 0x02
#define TWO_WAY_VIEW_MODEL_ID    0x04

#define BIND_OBJ_X_ID          LV_OBJ_SET_X_ID
#define BIND_OBJ_Y_ID          LV_OBJ_SET_Y_ID
#define BIND_OBJ_WIDTH_ID      LV_OBJ_SET_WIDTH_ID
#define BIND_OBJ_HEIGHT_ID     LV_OBJ_SET_HEIGHT_ID
#define BIND_OBJ_USER_DATA_ID  LV_OBJ_SET_USER_DATA_ID
#define BIND_OBJ_TOGGLE_ID     LV_OBJ_TOGGLE_ID
#define BIND_OBJ_STATE_ID      LV_OBJ_ADD_STATE_ID
#define BIND_OBJ_FLAG_ID       LV_OBJ_ADD_FLAG_ID
#define BIND_OBJ_CLEAR_FLAG_ID LV_OBJ_CLEAR_FLAG_ID
#define BIND_OBJ_VISIBLE_ID    LV_OBJ_SET_VISIBLE_ID

// label
#define BIND_LABEL_TEXT_ID LV_LABEL_SET_TEXT_ID

// checkbox
#define BIND_CHECKBOX_TEXT_ID LV_CHECKBOX_SET_TEXT_ID
#define BIND_CHECKBOX_CHECKED_ID LV_CHECKBOX_SET_CHECKED_ID

// switch
#define BIND_SWITCH_CHECKED_ID LV_SWITCH_SET_CHECKED_ID

// led
#define BIND_LED_BRIGHT_ID LV_LED_SET_BRIGHT_ID
#define BIND_LED_COLOR_ID  LV_LED_SET_COLOR_ID

// dropdown
#define BIND_DROPDOWN_OPTIONS_ID  LV_DROPDOWN_SET_OPTIONS_ID
#define BIND_DROPDOWN_SELECTED_ID LV_DROPDOWN_SET_SELECTED_ID

// btn

// img
#define BIND_IMG_PIVOT_X_ID LV_IMG_SET_PIVOT_X_ID
#define BIND_IMG_PIVOT_Y_ID LV_IMG_SET_PIVOT_Y_ID
#define BIND_IMG_ROTATE_ID  LV_IMG_SET_ANGLE_ID
#define BIND_IMG_ZOOM_ID    LV_IMG_SET_ZOOM_ID

// analogue
#define BIND_ANACLOCK_TIME_ID LV_ANALOGCLOCK_SET_TIME_ID

// imglist
#define BIND_IMGLIST_ACT_ID LV_IMGLIST_SET_ACT_ID

// line
#define BIND_LINE_POINTS_ID LV_LINE_SET_POINTS_ID

// scale
#define BIND_SCALECLOCK_TIME_ID LV_SCALECLOCK_SET_TIME_ID

// bar
#define BIND_BAR_VALUE_ID LV_BAR_SET_VALUE_ID

// slider
#define BIND_SLIDER_VALUE_ID LV_SLIDER_SET_VALUE_ID
#define BIND_SLIDER_LEFT_VALUE_ID LV_SLIDER_SET_LEFT_VALUE_ID

// digitclock
#define BIND_DIGITCLOCK_TIME_ID LV_DIGITCLOCK_SET_TIME_ID

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool bind_set_item(uint32_t bind_id, dyn_view_t payload, lv_obj_t *obj, int32_t page_id)
{
    (void)page_id;
    /* 绑定项依赖外部模型系统：缺少回调时跳过绑定，但不阻断页面构建。 */
    if (gui_msg_get_subject == NULL) {
        LV_LOG_WARN("gui_msg_get_subject is NULL");
        return true;
    }
    if (gui_msg_get_data == NULL) {
        LV_LOG_WARN("gui_msg_get_data is NULL");
        return true;
    }
    if (gui_msg_send == NULL) {
        LV_LOG_WARN("gui_msg_send is NULL");
        return true;
    }
    if (gui_msg_get == NULL) {
        LV_LOG_WARN("gui_msg_get is NULL");
        return true;
    }

    const uint8_t *p = payload.ptr;
    const uint8_t *end = payload.ptr + payload.len;
    uint32_t mode_u32;
    uint32_t model_id_u32;
    if (!dyn_read_le32(&p, end, &mode_u32) || !dyn_read_le32(&p, end, &model_id_u32)) {
        return false;
    }

    int32_t type_id = (int32_t)bind_id;
    char *digitclock_fmt = NULL;
    if (type_id == BIND_DIGITCLOCK_TIME_ID) {
        if (!dyn_read_string_alloc(&p, end, &digitclock_fmt) || p != end) {
            if (digitclock_fmt != NULL) {
                lv_free(digitclock_fmt);
            }
            return false;
        }
    } else if (p != end) {
        return false;
    }

    int32_t mode = (int32_t)mode_u32;
    int32_t model_id = (int32_t)model_id_u32;
    lv_subject_t *subject = gui_msg_get_subject(model_id);
    if (subject == NULL) {
        if (digitclock_fmt != NULL) {
            lv_free(digitclock_fmt);
        }
        return true;
    }

    /* gui_msg_get 负责准备当前模型数据，随后 gui_msg_get_data 作为 observer 的 user_data 使用。 */
    gui_msg_get(model_id);
    switch (type_id) {
    case BIND_OBJ_X_ID:
        bind_add_model_observer(mode, subject, obj_x_observer_cb, obj);
        break;
    case BIND_OBJ_Y_ID:
        bind_add_model_observer(mode, subject, obj_y_observer_cb, obj);
        break;
    case BIND_OBJ_WIDTH_ID:
        bind_add_model_observer(mode, subject, obj_width_observer_cb, obj);
        break;
    case BIND_OBJ_HEIGHT_ID:
        bind_add_model_observer(mode, subject, obj_height_observer_cb, obj);
        break;
    case BIND_OBJ_USER_DATA_ID:
        bind_add_model_observer(mode, subject, obj_user_data_observer_cb, obj);
        break;
    case BIND_OBJ_TOGGLE_ID:
        bind_add_model_observer(mode, subject, obj_toggle_observer_cb, obj);
        break;
    case BIND_OBJ_STATE_ID:
        bind_add_model_observer(mode, subject, obj_state_observer_cb, obj);
        break;
    case BIND_OBJ_FLAG_ID:
        bind_add_model_observer(mode, subject, obj_flag_observer_cb, obj);
        break;
    case BIND_OBJ_CLEAR_FLAG_ID:
        bind_add_model_observer(mode, subject, obj_clear_flag_observer_cb, obj);
        break;
    case BIND_OBJ_VISIBLE_ID:
        bind_add_model_observer(mode, subject, obj_visible_observer_cb, obj);
        break;

#if LV_USE_LABEL
    case BIND_LABEL_TEXT_ID:
        bind_add_model_observer(mode, subject, label_text_observer_cb, obj);
        break;
    case BIND_DIGITCLOCK_TIME_ID:
        if (mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
            digitclock_bind_ctx_t *ctx = digitclock_bind_ctx_create(gui_msg_get_data(), digitclock_fmt);
            if (ctx == NULL) {
                if (digitclock_fmt != NULL) {
                    lv_free(digitclock_fmt);
                }
                return false;
            }
            lv_observer_t *observer = lv_subject_add_observer_obj(subject, digitclock_time_observer_cb, obj, ctx);
            if (observer == NULL) {
                lv_free(ctx);
                if (digitclock_fmt != NULL) {
                    lv_free(digitclock_fmt);
                }
                return false;
            }
            observer->auto_free_user_data = 1;
        }
        break;
#endif

#if LV_USE_BUTTON
#endif

#if LV_USE_IMAGE
    case BIND_IMG_PIVOT_X_ID:
        bind_add_model_observer(mode, subject, img_pivot_x_observer_cb, obj);
        break;
    case BIND_IMG_PIVOT_Y_ID:
        bind_add_model_observer(mode, subject, img_pivot_y_observer_cb, obj);
        break;
    case BIND_IMG_ROTATE_ID:
        bind_add_model_observer(mode, subject, img_rotate_observer_cb, obj);
        break;
    case BIND_IMG_ZOOM_ID:
        bind_add_model_observer(mode, subject, img_zoom_observer_cb, obj);
        break;
#endif

#if LV_USE_SCALE
    case BIND_ANACLOCK_TIME_ID:
    case BIND_SCALECLOCK_TIME_ID:
        bind_add_model_observer(mode, subject, scale_time_observer_cb, obj);
        break;
#endif

#if LV_USE_IMGLIST
    case BIND_IMGLIST_ACT_ID:
        bind_add_model_observer(mode, subject, imglist_act_observer_cb, obj);
        break;
#endif

#if LV_USE_LINE
    case BIND_LINE_POINTS_ID:
        bind_add_model_observer(mode, subject, line_points_observer_cb, obj);
        break;
#endif

#if LV_USE_BAR
    case BIND_BAR_VALUE_ID:
        bind_add_model_observer(mode, subject, bar_value_observer_cb, obj);
        break;
#endif

#if LV_USE_SLIDER
    case BIND_SLIDER_VALUE_ID:
        bind_add_model_observer(mode, subject, slider_value_observer_cb, obj);
        if (mode & ONE_WAY_VIEW_TO_MODEL_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
            lv_obj_add_event_cb(obj, slider_value_event_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)model_id);
        }
        break;
    case BIND_SLIDER_LEFT_VALUE_ID:
        bind_add_model_observer(mode, subject, slider_left_value_observer_cb, obj);
        if (mode & ONE_WAY_VIEW_TO_MODEL_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
            lv_obj_add_event_cb(obj, slider_left_value_event_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)model_id);
        }
        break;
#endif

#if LV_USE_LED
    case BIND_LED_BRIGHT_ID:
        bind_add_model_observer(mode, subject, led_bright_observer_cb, obj);
        break;
    case BIND_LED_COLOR_ID:
        bind_add_model_observer(mode, subject, led_color_observer_cb, obj);
        break;
#endif

#if LV_USE_CHECKBOX
    case BIND_CHECKBOX_TEXT_ID:
        bind_add_model_observer(mode, subject, checkbox_text_observer_cb, obj);
        break;
    case BIND_CHECKBOX_CHECKED_ID:
        bind_add_model_observer(mode, subject, checked_state_observer_cb, obj);
        if (mode & ONE_WAY_VIEW_TO_MODEL_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
            lv_obj_add_event_cb(obj, checked_state_event_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)model_id);
        }
        break;
#endif

#if LV_USE_SWITCH
    case BIND_SWITCH_CHECKED_ID:
        bind_add_model_observer(mode, subject, checked_state_observer_cb, obj);
        if (mode & ONE_WAY_VIEW_TO_MODEL_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
            lv_obj_add_event_cb(obj, checked_state_event_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)model_id);
        }
        break;
#endif

#if LV_USE_DROPDOWN
    case BIND_DROPDOWN_OPTIONS_ID:
        bind_add_model_observer(mode, subject, dropdown_options_observer_cb, obj);
        break;
    case BIND_DROPDOWN_SELECTED_ID:
        bind_add_model_observer(mode, subject, dropdown_selected_observer_cb, obj);
        if (mode & ONE_WAY_VIEW_TO_MODEL_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
            lv_obj_add_event_cb(obj, dropdown_selected_event_cb, LV_EVENT_VALUE_CHANGED,
                                (void *)(intptr_t)model_id);
        }
        break;
#endif

    default:
        LV_LOG_WARN("unknown bind item %d", (int)bind_id);
        break;
    }
    if (digitclock_fmt != NULL) {
        lv_free(digitclock_fmt);
    }
    return true;
}

void bind_set_send_cb(gui_bind_send_cb_t cb)
{
    gui_msg_send = cb;
}

void bind_set_get_cb(gui_bind_get_cb_t cb)
{
    gui_msg_get = cb;
}

void bind_set_get_subject_cb(gui_bind_get_subject_cb_t cb)
{
    gui_msg_get_subject = cb;
}

void bind_set_get_data_cb(gui_bind_get_data_cb_t cb)
{
    gui_msg_get_data = cb;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void obj_x_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_obj_set_x(obj, data->value_int);
}

static void obj_y_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_obj_set_y(obj, data->value_int);
}

static void obj_width_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_obj_set_width(obj, data->value_int);
}

static void obj_height_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_obj_set_height(obj, data->value_int);
}

static void obj_user_data_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_obj_set_user_data(obj, data->value_string);
}

static void obj_toggle_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    if (data->value_int) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    } else {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    }
}

static void obj_state_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_obj_remove_state(obj, LV_STATE_ANY);
    lv_obj_add_state(obj, data->value_int);
}

static void obj_flag_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_obj_add_flag(obj, data->value_int);
}

static void obj_clear_flag_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_obj_remove_flag(obj, data->value_int);
}

static void obj_visible_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    if (data->value_int) {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static digitclock_bind_ctx_t *digitclock_bind_ctx_create(gui_msg_data_t *data, const char *fmt)
{
    size_t fmt_len = strlen(fmt);
    digitclock_bind_ctx_t *ctx = lv_malloc(sizeof(digitclock_bind_ctx_t) + fmt_len + 1U);
    if (ctx == NULL) {
        return NULL;
    }

    ctx->data = data;
    lv_memcpy(ctx->fmt, fmt, fmt_len + 1U);
    return ctx;
}

static void bind_add_model_observer(int32_t mode, lv_subject_t *subject, lv_observer_cb_t observer_cb, lv_obj_t *obj)
{
    if (mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
        lv_subject_add_observer_obj(subject, observer_cb, obj, gui_msg_get_data());
    }
}

static bool bind_observer_get_context(lv_observer_t *observer, lv_obj_t **obj, gui_msg_data_t **data)
{
    lv_obj_t *target = lv_observer_get_target_obj(observer);
    if (target == NULL || lv_obj_is_valid(target) == false) {
        return false;
    }

    *obj = target;
    *data = (gui_msg_data_t *)lv_observer_get_user_data(observer);
    return true;
}

static bool bind_event_get_send_context(lv_event_t *e, lv_obj_t **obj, int32_t *model_id,
                                        gui_bind_send_cb_t *send_cb)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == NULL || lv_obj_is_valid(target) == false) {
        return false;
    }

    gui_bind_send_cb_t cb = gui_msg_send;
    if (cb == NULL) {
        LV_LOG_WARN("gui_msg_send is NULL");
        return false;
    }

    *obj = target;
    *model_id = (int32_t)(intptr_t)lv_event_get_user_data(e);
    *send_cb = cb;
    return true;
}

#if LV_USE_LABEL

static void label_text_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_label_set_text(obj, data->value_string);
}

static void digitclock_time_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj = lv_observer_get_target_obj(observer);
    if (obj == NULL || lv_obj_is_valid(obj) == false) {
        return;
    }

    digitclock_bind_ctx_t *ctx = (digitclock_bind_ctx_t *)lv_observer_get_user_data(observer);
    if (ctx == NULL || ctx->data == NULL) {
        return;
    }

    dyn_digitclock_set_text(obj, ctx->fmt, &ctx->data->value_time);
}

#endif

#if LV_USE_CHECKBOX || LV_USE_SWITCH

static void checked_state_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    if (data->value_bool) {
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(obj, LV_STATE_CHECKED);
    }
}

static void checked_state_event_cb(lv_event_t *e)
{
    lv_obj_t *obj;
    int32_t model_id;
    gui_bind_send_cb_t send_cb;
    if (!bind_event_get_send_context(e, &obj, &model_id, &send_cb)) {
        return;
    }
    int32_t value = lv_obj_has_state(obj, LV_STATE_CHECKED) ? 1 : 0;
    send_cb(model_id, (void *)(intptr_t)value, 1);
}

#endif

#if LV_USE_CHECKBOX

static void checkbox_text_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_checkbox_set_text(obj, data->value_string);
}

#endif

#if LV_USE_BUTTON

#endif

#if LV_USE_IMAGE

static void img_pivot_x_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_point_t pivot;
    lv_image_get_pivot(obj, &pivot);
    lv_image_set_pivot(obj, data->value_int, pivot.y);
}

static void img_pivot_y_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_point_t pivot;
    lv_image_get_pivot(obj, &pivot);
    lv_image_set_pivot(obj, pivot.x, data->value_int);
}

static void img_rotate_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_image_set_rotation(obj, data->value_int);
}

static void img_zoom_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_image_set_scale(obj, data->value_int);
}
#endif

#if LV_USE_SCALE

static void scale_time_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    struct tm time = data->value_time;
    lv_obj_t *hour_needle = lv_obj_get_child(obj, 0);
    lv_obj_t *min_needle = lv_obj_get_child(obj, 1);
    lv_obj_t *sec_needle = lv_obj_get_child(obj, 2);
    if (hour_needle == NULL || min_needle == NULL || sec_needle == NULL) {
        return;
    }

    if (lv_obj_check_type(hour_needle, &lv_image_class)) {
        lv_scale_set_image_needle_value(obj, hour_needle, (time.tm_hour % 12) * 60 + time.tm_min);
    } else {
        lv_scale_set_line_needle_value(obj, hour_needle, (int32_t)(intptr_t)lv_obj_get_user_data(hour_needle),
                                       (time.tm_hour % 12) * 60 + time.tm_min);
    }
    if (lv_obj_check_type(min_needle, &lv_image_class)) {
        lv_scale_set_image_needle_value(obj, min_needle, time.tm_min * 12 + time.tm_sec / 5);
    } else {
        lv_scale_set_line_needle_value(obj, min_needle, (int32_t)(intptr_t)lv_obj_get_user_data(min_needle),
                                       time.tm_min * 12 + time.tm_sec / 5);
    }
    if (lv_obj_check_type(sec_needle, &lv_image_class)) {
        lv_scale_set_image_needle_value(obj, sec_needle, time.tm_sec * 12);
    } else {
        lv_scale_set_line_needle_value(obj, sec_needle, (int32_t)(intptr_t)lv_obj_get_user_data(sec_needle),
                                       time.tm_sec * 12);
    }
}

#endif

#if LV_USE_IMGLIST

static void imglist_act_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_imglist_set_act(obj, data->value_int);
}

#endif

#if LV_USE_LINE

static void line_points_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_line_set_points(obj, (lv_point_precise_t *)data->value_array.ptr, data->value_array.len);
}

#endif

#if LV_USE_BAR

static void bar_value_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_bar_set_value(obj, data->value_int, LV_ANIM_OFF);
}

#endif

#if LV_USE_SLIDER

static void slider_value_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_slider_set_value(obj, data->value_int, LV_ANIM_OFF);
}

static void slider_left_value_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_slider_set_start_value(obj, data->value_int, LV_ANIM_OFF);
}

static void slider_value_event_cb(lv_event_t *e)
{
    lv_obj_t *obj;
    int32_t model_id;
    gui_bind_send_cb_t send_cb;
    if (!bind_event_get_send_context(e, &obj, &model_id, &send_cb)) {
        return;
    }
    int32_t value = lv_slider_get_value(obj);
    send_cb(model_id, (void *)(intptr_t)value, 1);
}

static void slider_left_value_event_cb(lv_event_t *e)
{
    lv_obj_t *obj;
    int32_t model_id;
    gui_bind_send_cb_t send_cb;
    if (!bind_event_get_send_context(e, &obj, &model_id, &send_cb)) {
        return;
    }
    int32_t value = lv_slider_get_left_value(obj);
    send_cb(model_id, (void *)(intptr_t)value, 1);
}

#endif

#if LV_USE_LED

static void led_bright_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_led_set_brightness(obj, (uint8_t)data->value_int);
}

static void led_color_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_led_set_color(obj, data->value_color);
}

#endif

#if LV_USE_DROPDOWN

static void dropdown_options_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_dropdown_set_options(obj, data->value_string);
}

static void dropdown_selected_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj;
    gui_msg_data_t *data;
    if (!bind_observer_get_context(observer, &obj, &data)) {
        return;
    }
    lv_dropdown_set_selected(obj, data->value_int);
}

static void dropdown_selected_event_cb(lv_event_t *e)
{
    lv_obj_t *obj;
    int32_t model_id;
    gui_bind_send_cb_t send_cb;
    if (!bind_event_get_send_context(e, &obj, &model_id, &send_cb)) {
        return;
    }
    int32_t value = lv_dropdown_get_selected(obj);
    send_cb(model_id, (void *)(intptr_t)value, 1);
}

#endif

#endif /*LV_USE_GUI_PARSE_DYN*/
