/**
 * @file bind.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "bind.h"

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
    char * value_string;
    lv_color_t value_color;
    lv_calendar_date_t value_date;
    lv_point_t value_point;
    struct tm value_time;
    struct {
        void * ptr;
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

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void obj_x_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_y_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_width_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_height_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_user_data_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_toggle_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_state_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_flag_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_clear_flag_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
static void obj_visible_observer_cb(lv_observer_t * observer, lv_subject_t * subject);

#if LV_USE_LABEL
    static void label_text_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
#endif

#if LV_USE_BUTTON
#endif

#if LV_USE_IMAGE
    static void img_pivot_x_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
    static void img_pivot_y_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
    static void img_rotate_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
    static void img_zoom_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
#endif

#if LV_USE_SCALE
    static void analogclock_time_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
#endif

#if LV_USE_IMGLIST
    static void imglist_act_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
#endif

#if LV_USE_LINE
    static void line_points_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
#endif

#if LV_USE_SCALE
    static void scale_time_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
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

#define BIND_OBJ_X_ID          0x00000000
#define BIND_OBJ_Y_ID          0x00000001
#define BIND_OBJ_WIDTH_ID      0x00000002
#define BIND_OBJ_HEIGHT_ID     0x00000003
#define BIND_OBJ_USER_DATA_ID  0x00000004
#define BIND_OBJ_TOGGLE_ID     0x00000005
#define BIND_OBJ_STATE_ID      0x00000006
#define BIND_OBJ_FLAG_ID       0x00000007
#define BIND_OBJ_CLEAR_FLAG_ID 0x00000008
#define BIND_OBJ_VISIBLE_ID    0x00000009

// label
#define BIND_LABEL_TEXT_ID 0x00001000

// btn

// img
#define BIND_IMG_PIVOT_X_ID 0x00003000
#define BIND_IMG_PIVOT_Y_ID 0x00003001
#define BIND_IMG_ROTATE_ID  0x00003002
#define BIND_IMG_ZOOM_ID    0x00003003

// analogue
#define BIND_ANACLOCK_TIME_ID 0x00004000

// imglist
#define BIND_IMGLIST_ACT_ID 0x00005000

// line
#define BIND_LINE_POINTS_ID 0x00006000

// scale
#define BIND_SCALECLOCK_TIME_ID 0x00007000

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void bind_set(char * buf, int32_t size, lv_obj_t * obj, int32_t page_id)
{
    if(buf == NULL || size <= 0) {
        return;
    }
    if(gui_msg_get_subject == NULL) {
        LV_LOG_WARN("gui_msg_get_subject is NULL");
        return;
    }
    if(gui_msg_get_data == NULL) {
        LV_LOG_WARN("gui_msg_get_data is NULL");
        return;
    }
    if(gui_msg_send == NULL) {
        LV_LOG_WARN("gui_msg_send is NULL");
        return;
    }
    if(gui_msg_get == NULL) {
        LV_LOG_WARN("gui_msg_get is NULL");
        return;
    }

    JLDynControl * control = (JLDynControl *)buf;
    char * bind_buf = buf + control->bind_offset;
    int32_t offset = 0;
    int32_t bind_size = control->bind_cnt * 3 * sizeof(int32_t);
    while(offset < bind_size) {
        int32_t type_id = *(int32_t *)(bind_buf + offset);
        int32_t mode = *(int32_t *)(bind_buf + offset + sizeof(int32_t));
        int32_t model_id = *(int32_t *)(bind_buf + offset + 2 * sizeof(int32_t));
        offset += 3 * sizeof(int32_t);

        lv_subject_t * subject = gui_msg_get_subject(model_id);
        if(subject == NULL) {
            continue;
        }

        gui_msg_get(model_id);
        switch(type_id) {
            case BIND_OBJ_X_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_x_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_Y_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_y_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_WIDTH_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_width_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_HEIGHT_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_height_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_USER_DATA_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_user_data_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_TOGGLE_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_toggle_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_STATE_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_state_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_FLAG_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_flag_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_CLEAR_FLAG_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_clear_flag_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_OBJ_VISIBLE_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, obj_visible_observer_cb, obj, gui_msg_get_data());
                }
                break;

#if LV_USE_LABEL
            case BIND_LABEL_TEXT_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, label_text_observer_cb, obj, gui_msg_get_data());
                }
                break;
#endif

#if LV_USE_BUTTON
#endif

#if LV_USE_IMAGE
            case BIND_IMG_PIVOT_X_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, img_pivot_x_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_IMG_PIVOT_Y_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, img_pivot_y_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_IMG_ROTATE_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, img_rotate_observer_cb, obj, gui_msg_get_data());
                }
                break;
            case BIND_IMG_ZOOM_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, img_zoom_observer_cb, obj, gui_msg_get_data());
                }
                break;
#endif

#if LV_USE_SCALE
            case BIND_ANACLOCK_TIME_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, scale_time_observer_cb, obj, gui_msg_get_data());
                }
                break;
#endif

#if LV_USE_IMGLIST
            case BIND_IMGLIST_ACT_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, imglist_act_observer_cb, obj, gui_msg_get_data());
                }
                break;
#endif

#if LV_USE_LINE
            case BIND_LINE_POINTS_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, line_points_observer_cb, obj, gui_msg_get_data());
                }
                break;
#endif

#if LV_USE_SCALE
            case BIND_SCALECLOCK_TIME_ID:
                if(mode & ONE_WAY_MODEL_TO_VIEW_ID || mode & TWO_WAY_VIEW_MODEL_ID) {
                    lv_subject_add_observer_obj(subject, scale_time_observer_cb, obj, gui_msg_get_data());
                }
                break;
#endif

            default:
                break;
        }
    }
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

static void obj_x_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_obj_set_x(obj, data->value_int);
}

static void obj_y_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_obj_set_y(obj, data->value_int);
}

static void obj_width_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_obj_set_width(obj, data->value_int);
}

static void obj_height_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_obj_set_height(obj, data->value_int);
}

static void obj_user_data_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_obj_set_user_data(obj, data->value_string);
}

static void obj_toggle_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    if(data->value_int)
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    else
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_CHECKABLE);
}

static void obj_state_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_obj_remove_state(obj, LV_STATE_ANY);
    lv_obj_add_state(obj, data->value_int);
}

static void obj_flag_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_obj_add_flag(obj, data->value_int);
}

static void obj_clear_flag_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_obj_remove_flag(obj, data->value_int);
}

static void obj_visible_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    if(data->value_int)
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

#if LV_USE_LABEL

static void label_text_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_label_set_text(obj, data->value_string);
}

#endif

#if LV_USE_BUTTON

#endif

#if LV_USE_IMAGE

static void img_pivot_x_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_point_t pivot;
    lv_image_get_pivot(obj, &pivot);
    lv_image_set_pivot(obj, data->value_int, pivot.y);
}

static void img_pivot_y_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_point_t pivot;
    lv_image_get_pivot(obj, &pivot);
    lv_image_set_pivot(obj, pivot.x, data->value_int);
}

static void img_rotate_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_image_set_rotation(obj, data->value_int);
}

static void img_zoom_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_image_set_scale(obj, data->value_int);
}
#endif

#if LV_USE_SCALE

static void analogclock_time_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    scale_time_observer_cb(observer, subject);
}

#endif

#if LV_USE_IMGLIST

static void imglist_act_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_imglist_set_act(obj, data->value_int);
}

#endif

#if LV_USE_LINE

static void line_points_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    lv_line_set_points(obj, (lv_point_precise_t *)data->value_array.ptr, data->value_array.len);
}

#endif

#if LV_USE_SCALE

static void scale_time_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * obj = lv_observer_get_target_obj(observer);
    if(obj == NULL || lv_obj_is_valid(obj) == false) return;

    gui_msg_data_t * data = (gui_msg_data_t *)observer->user_data;
    struct tm time = data->value_time;
	lv_obj_t * hour_needle = lv_obj_get_child(obj, 0);
	lv_obj_t * min_needle = lv_obj_get_child(obj, 1);
	lv_obj_t * sec_needle = lv_obj_get_child(obj, 2);
    if (hour_needle == NULL || min_needle == NULL || sec_needle == NULL) return;

    if (lv_obj_check_type(hour_needle, &lv_image_class)) {
		lv_scale_set_image_needle_value(obj, hour_needle, (time.tm_hour % 12) * 60 + time.tm_min);
	} else {
		lv_scale_set_line_needle_value(obj, hour_needle, (int32_t)(intptr_t)lv_obj_get_user_data(hour_needle), (time.tm_hour % 12) * 60 + time.tm_min);
	}
	if (lv_obj_check_type(min_needle, &lv_image_class)) {
		lv_scale_set_image_needle_value(obj, min_needle, time.tm_min * 12 + time.tm_sec / 5);
	} else {
		lv_scale_set_line_needle_value(obj, min_needle, (int32_t)(intptr_t)lv_obj_get_user_data(min_needle), time.tm_min * 12 + time.tm_sec / 5);
	}
	if (lv_obj_check_type(sec_needle, &lv_image_class)) {
		lv_scale_set_image_needle_value(obj, sec_needle, time.tm_sec * 12);
	} else {
		lv_scale_set_line_needle_value(obj, sec_needle, (int32_t)(intptr_t)lv_obj_get_user_data(sec_needle), time.tm_sec * 12);
	}
}

#endif

#endif /*LV_USE_GUI_PARSE_DYN*/
