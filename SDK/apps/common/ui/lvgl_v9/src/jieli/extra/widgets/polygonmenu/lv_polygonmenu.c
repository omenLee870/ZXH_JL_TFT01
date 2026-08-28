/**
 * @file lv_kaleidoscopemenu.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_polygonmenu_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/indev/lv_indev_private.h"
#include "../../../../jieli/common/gui/scr/gui_scr.h"


#if LV_USE_POLYGONMENU != 0
#include "math.h"

#define abs(x)    (((x) > 0) ? (x) : (-(x)))

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_kaleidoscopemenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif


/*********************
 *      DEFINES
 *********************/

#define MENU_RENDER_MARGIN_HOR 50
#define MENU_RENDER_MARGIN_VER 50
#define MENU_ZOOM0_MAX_ROW_WIDTH_FACOTR 0.5 // 缩放最小时，最长的一行占据菜单的宽度的比例
#define MENU_REBOUND_DELAY_MS 250           // 动画回弹延时
#define MENU_INERTIA_MOVING_THRESHOLD 50    // 惯性滑动开启的距离阈值

#define MENU_HEXAGON_ICON_Y_OFFSET_FACTOR 0.9 // 用于调整六边形时图标在Y方向上的紧凑程度， 取0.8 ~ 1.0 之间


#define MY_CLASS &lv_polygonmenu_class



/**********************
 *      TYPEDEFS
 **********************/


typedef enum {
    STATUS_ZOOM_0 = 0, // 最小
    STATUS_ZOOM_1,
    STATUS_ZOOM_2, // 默认
    STATUS_ZOOM_3,
    STATUS_ZOOM_4, // 会进行 item transform
    STATUS_ZOOM_5,
    STATUS_ZOOM_6,
    STATUS_ZOOM_7,
    STATUS_ZOOM_8,
    STATUS_ZOOM_9, //
    STATUS_ZOOM_2_BACK,
    STATUS_ZOOM_4_BACK,

    STATUS_ZOOM_2_MOVING,
    STATUS_ZOOM_4_MOVING,
    STATUS_ZOOM_4_INERTIA_MOVING // 惯性
} lv_polygonmenu_status_t;

typedef enum {
    EFFECT_ZOOM = 0,
    EFFECT_SLIDE,
    EFFECT_TRANSFORM_ZOOM
} lv_polygonmenu_effect_t;

typedef struct {
    lv_polygonmenu_effect_t type;
    uint16_t target_icon_size;
    int32_t target_btn_idx;
    int32_t icon_size_step;
} lv_polygonmenu_effect_zoom_t;

typedef struct {
    lv_polygonmenu_effect_t type;
    int32_t target_offset_x;
    int32_t target_offset_y;
    int32_t x_step;
    int32_t y_step;
} lv_polygonmenu_effect_slide_t;

typedef struct {
    lv_polygonmenu_effect_t type;
    int32_t target_btn_idx;
    int32_t target_zoom;
    int32_t current_zoom;
    int32_t zoom_step;
    int32_t initial_zoom;
} lv_polygonmenu_effect_transform_zoom_t;


/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_polygonmenu_class_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_polygonmenu_class_destructor(const struct _lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_polygonmenu_button_class_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_polygonmenu_event(const lv_obj_class_t *class_p, lv_event_t *e);
static void move_timer_cb(lv_timer_t *timer_p);
static void temp_status_report_cb(lv_timer_t *timer_p);

static void icon_arrangment(lv_obj_t *menu_obj, int32_t icon_size);
static void icon_position(lv_obj_t *menu_obj);
static int32_t get_icon_size_by_zoom_status(lv_obj_t *menu_obj, lv_polygonmenu_status_t status);
static void move_back(lv_obj_t *menu_obj);
static int16_t rect_change_circle_cb(lv_point_t *icon_pos, const lv_obj_t *obj, const lv_obj_t *btn);
static int32_t get_menu_active_btn_idx(lv_obj_t *menu_obj);
static void reset_offset_by_active_btn_idx(lv_obj_t *menu_obj, int32_t idx);
static lv_polygonmenu_button_t *get_visible_icon_by_idx(lv_obj_t *menu_obj, int32_t idx);
static void reset_menu(lv_obj_t *menu_obj);
static int32_t get_menu_max_row_icon_count(lv_obj_t *menu_obj);
static void create_menu_transform_zoom_task(lv_obj_t *menu_obj, lv_polygonmenu_status_t prev_status,
        lv_polygonmenu_status_t target_status, int32_t target_btn_idx, bool long_time);


static void bubble_event_cb(lv_event_t *e);
static float hal_gfx_sqrt(uint32_t x);
static void clear_move_tmr(lv_obj_t *menu_obj);
static void start_move_tmr(lv_obj_t *menu_obj, void *tmr_data);
static void normalize_menu_move_offset(float radius, int32_t x, int32_t y, int32_t *x_p, int32_t *y_p);
static bool is_scr_transition(lv_obj_t *menu_obj);
static inline bool is_scr_cb_existing(lv_obj_t *menu_obj);
static void init_bg_scr(lv_obj_t *obj);


/**********************
 *  STATIC VARIABLES
 **********************/


const lv_obj_class_t lv_polygonmenu_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_polygonmenu_class_constructor,
    .destructor_cb = lv_polygonmenu_class_destructor,
    .event_cb = lv_polygonmenu_event,
    .width_def = 380,
    .height_def = 450,
    .instance_size = sizeof(lv_polygonmenu_t),
};
#define MY_CLASS &lv_polygonmenu_class

const lv_obj_class_t lv_polygonmenu_button_class = {
    .base_class = &lv_image_class,
    .constructor_cb = lv_polygonmenu_button_class_constructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_polygonmenu_button_t),
};

static lv_obj_t *bg_scr = NULL;


/**********************
 *      MACROS
 **********************/

#define SET_MENU_STATUS(menu_p, new_status) \
    {                                         \
        menu_p->prev_status = menu_p->status;   \
        menu_p->status = new_status;            \
    }

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_polygonmenu_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));

    return obj;
}

lv_obj_t *lv_polygonmenu_add_button(lv_obj_t *menu_obj, const void *icon)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu = (lv_polygonmenu_t *)menu_obj;

    lv_obj_t *menu_btn = lv_obj_class_create_obj(&lv_polygonmenu_button_class, menu_obj);
    lv_obj_class_init_obj(menu_btn);

    lv_image_set_scale(menu_btn, 256);

    if (icon) {
        lv_image_set_src(menu_btn, icon);
    }

    reset_menu(menu_obj);
    return menu_btn;
}


void lv_polygonmenu_zoom_in(lv_obj_t *menu_obj)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    lv_display_t *disp = lv_display_get_default();
    lv_obj_t *menu_scr = lv_obj_get_screen(menu_obj);
    bool illegal_scr_status = menu_scr != lv_screen_active();
    if (illegal_scr_status) {
        LV_LOG_WARN("illegal scr status");
        return;
    }
    int32_t idx = get_menu_active_btn_idx(menu_obj);
    if (idx < 0) {
        return;
    }

    lv_obj_t *child = lv_obj_get_child(menu_obj, idx);
    lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)child;

    if ((menu_p->status >= STATUS_ZOOM_0 && menu_p->status <= STATUS_ZOOM_3) || menu_p->status == STATUS_ZOOM_2_BACK ||
        menu_p->status == STATUS_ZOOM_4_BACK) {
        if (menu_p->status == STATUS_ZOOM_2_BACK) {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_2);
        } else if (menu_p->status == STATUS_ZOOM_4_BACK) {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_4);
        }

        if (menu_p->status == STATUS_ZOOM_1) {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_2_BACK);
        } else {
            SET_MENU_STATUS(menu_p, menu_p->status + 1);
        }


        if (menu_p->move_tmr_data) {
            clear_move_tmr(menu_obj);
            lv_timer_reset(menu_p->temp_status_report_tmr);
            if (menu_p->prev_status <= STATUS_ZOOM_3) {
                menu_p->icon_size = get_icon_size_by_zoom_status(menu_obj, menu_p->prev_status);
            }
        }

        lv_polygonmenu_effect_zoom_t *info_p = lv_malloc(sizeof(lv_polygonmenu_effect_zoom_t));
        LV_ASSERT_MALLOC(info_p);
        info_p->type = EFFECT_ZOOM;
        info_p->target_btn_idx = idx;
        info_p->target_icon_size = get_icon_size_by_zoom_status(menu_obj, menu_p->status);
        info_p->icon_size_step = (info_p->target_icon_size - menu_p->icon_size) / 4;
        if (menu_p->prev_status == STATUS_ZOOM_8 && menu_p->status == STATUS_ZOOM_9) {
            // 稍微延长过渡时间
            info_p->icon_size_step /= 3.0;
        }

        start_move_tmr(menu_obj, info_p);

    } else if (menu_p->status >= STATUS_ZOOM_4 && menu_p->status <= STATUS_ZOOM_8) { // 不处理 STATUS_ZOOM_9
        if (menu_p->move_tmr_data && menu_p->status == STATUS_ZOOM_4) {
            return;
        }
        if (menu_p->status == STATUS_ZOOM_4 && (menu_p->prev_status == STATUS_ZOOM_4_BACK ||
                                                menu_p->prev_status == STATUS_ZOOM_3)) {
            // LV_LOG_WARN("ffff %d", menu_p->prev_status);
        }

        bool scr_cb_exist = is_scr_cb_existing(menu_obj);
        bool scr_transition = is_scr_transition(menu_obj);
        if (scr_cb_exist && btn_p->scr_id >= 0 && !scr_transition && !menu_p->scr_cb.is_scr_act_cb(btn_p->scr_id)) {
            lv_obj_t *target_scr = menu_p->scr_cb.get_setup_scr_cb(btn_p->scr_id);
            if (!target_scr) {
                return;
            }

            // 初始化target_scr
            lv_obj_set_style_opa(target_scr, LV_OPA_0, 0);
            lv_obj_set_pos(target_scr, lv_display_get_horizontal_resolution(disp) / 2, lv_display_get_vertical_resolution(disp) / 2);
            lv_obj_set_style_transform_pivot_x(target_scr, 0, 0);
            lv_obj_set_style_transform_pivot_y(target_scr, 0, 0);
            lv_obj_set_style_transform_rotation(target_scr, 0, 0);

            disp->prev_scr = target_scr;

            lv_obj_set_style_bg_opa(disp->act_scr, LV_OPA_0, 0);
            lv_obj_set_style_bg_opa(menu_obj, LV_OPA_0, 0);
            init_bg_scr(disp->act_scr);
            // disp->bg_scr = bg_scr;

            scr_transition = true;
        }
        if (menu_p->status == STATUS_ZOOM_8 && !scr_transition) {
            return;
        }

        if (menu_p->move_tmr_data) {
            clear_move_tmr(menu_obj);
            lv_timer_reset(menu_p->temp_status_report_tmr);
        }

        SET_MENU_STATUS(menu_p, menu_p->status + 1);
        create_menu_transform_zoom_task(menu_obj, menu_p->prev_status, menu_p->status, idx, false);

    }

}




void lv_polygonmenu_zoom_out(lv_obj_t *menu_obj)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    lv_display_t *disp = lv_display_get_default();
    lv_obj_t *menu_scr = lv_obj_get_screen(menu_obj);

    bool illegal_scr_status = menu_scr != lv_screen_active();
    if (illegal_scr_status) {
        LV_LOG_WARN("illegal scr status");
        return;
    }

    int32_t idx = get_menu_active_btn_idx(menu_obj);
    if (idx < 0) {
        return;
    }

    if ((menu_p->status >= STATUS_ZOOM_0 && menu_p->status <= STATUS_ZOOM_4) || menu_p->status == STATUS_ZOOM_2_BACK ||
        menu_p->status == STATUS_ZOOM_4_BACK) {
        if (menu_p->move_tmr_data && menu_p->prev_status >= STATUS_ZOOM_5) {
            // 避免屏幕过渡状态没有正常移除
            return;
        }

        if (menu_p->status == STATUS_ZOOM_0) {
            lv_timer_reset(menu_p->temp_status_report_tmr);
            return;
        }

        if (menu_p->move_tmr_data) {
            if (menu_p->status <= STATUS_ZOOM_4) {
                menu_p->icon_size = get_icon_size_by_zoom_status(menu_obj, menu_p->status);
            }
            clear_move_tmr(menu_obj);
            lv_timer_reset(menu_p->temp_status_report_tmr);
        }

        if (menu_p->status == STATUS_ZOOM_2_BACK) {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_2);
        } else if (menu_p->status == STATUS_ZOOM_4_BACK) {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_4);
        }

        if (menu_p->status == STATUS_ZOOM_3) {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_2_BACK);
        } else {
            SET_MENU_STATUS(menu_p, menu_p->status - 1);
        }

        lv_polygonmenu_effect_zoom_t *info_p = lv_malloc(sizeof(lv_polygonmenu_effect_zoom_t));
        LV_ASSERT_MALLOC(info_p);
        info_p->type = EFFECT_ZOOM;
        info_p->target_btn_idx = idx;
        info_p->target_icon_size = get_icon_size_by_zoom_status(menu_obj, menu_p->status);
        info_p->icon_size_step = (info_p->target_icon_size - menu_p->icon_size) / 3;

        start_move_tmr(menu_obj, info_p);
    } else if (menu_p->status >= STATUS_ZOOM_5 && menu_p->status <= STATUS_ZOOM_8) { // 不处理 STATUS_ZOOM_9
        if (menu_p->move_tmr_data) {
            clear_move_tmr(menu_obj);
            lv_timer_reset(menu_p->temp_status_report_tmr);
        }
        SET_MENU_STATUS(menu_p, menu_p->status - 1);

        create_menu_transform_zoom_task(menu_obj, menu_p->prev_status, menu_p->status, idx, false);
    }
}


void lv_polygonmenu_button_load_scr(lv_obj_t *btn_obj)
{
    LV_ASSERT_OBJ(btn_obj, &lv_polygonmenu_button_class);
    lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)btn_obj;

    lv_obj_t *menu_obj = lv_obj_get_parent(btn_obj);
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;

    if (!btn_p->is_hide && !menu_p->move_tmr_data) {
        uint32_t idx = lv_obj_get_index(btn_obj);
        if (idx >= lv_obj_get_child_count(menu_obj)) {
            return;
        }
        if (menu_p->status == STATUS_ZOOM_2) {
            reset_offset_by_active_btn_idx(menu_obj, idx);
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_3);
            lv_polygonmenu_zoom_in(menu_obj);
        } else if (menu_p->status == STATUS_ZOOM_4) {
            lv_display_t *disp = lv_display_get_default();
            // 初始化过渡状态
            bool scr_cb_exist = is_scr_cb_existing(menu_obj);
            if (scr_cb_exist && btn_p->scr_id >= 0 && !menu_p->scr_cb.is_scr_act_cb(btn_p->scr_id)) {
                lv_obj_t *target_scr = menu_p->scr_cb.get_setup_scr_cb(btn_p->scr_id);
                if (!target_scr) {
                    return;
                }
                // 初始化target_scr
                lv_obj_set_style_opa(target_scr, LV_OPA_0, 0);
                lv_obj_set_pos(target_scr, lv_display_get_horizontal_resolution(disp) / 2, lv_display_get_vertical_resolution(disp) / 2);
                lv_obj_set_style_transform_pivot_x(target_scr, 0, 0);
                lv_obj_set_style_transform_pivot_y(target_scr, 0, 0);
                lv_obj_set_style_transform_rotation(target_scr, 0, 0);

                disp->prev_scr = target_scr;

                menu_p->prev_status = STATUS_ZOOM_4;
                menu_p->status = STATUS_ZOOM_9;

                lv_obj_set_style_bg_opa(disp->act_scr, LV_OPA_0, 0);
                lv_obj_set_style_bg_opa(menu_obj, LV_OPA_0, 0);
                init_bg_scr(disp->act_scr);
                // disp->bg_scr = bg_scr;

                create_menu_transform_zoom_task(menu_obj, menu_p->prev_status, menu_p->status, idx, false);
            } else {
                reset_offset_by_active_btn_idx(menu_obj, idx);
                icon_position(menu_obj);

                menu_p->prev_status = STATUS_ZOOM_4;
                menu_p->status = STATUS_ZOOM_7;
                create_menu_transform_zoom_task(menu_obj, menu_p->prev_status, menu_p->status, idx, false);
            }

        }
    }
}


void lv_polygonmenu_return(lv_obj_t *menu_obj)
{
    // 是否 menu 类型实例
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;

    lv_obj_t *menu_scr = lv_obj_get_screen(menu_obj);
    int32_t child_cnt = lv_obj_get_child_count(menu_obj);
    lv_display_t *disp = lv_display_get_default();

    // 是否能加载新屏幕
    if (!menu_p->scr_cb.load_scr_cb) {
        return;
    }
    // 判断当前是否 非菜单屏幕
    gui_scr_t *act_gui_scr = gui_scr_get_act();
    gui_scr_t *menu_gui_scr = gui_scr_get_by_obj(menu_scr);
    if (!act_gui_scr || !menu_gui_scr || act_gui_scr->scr == menu_gui_scr->scr) {
        return;
    }

    // 加载到菜单屏幕
    menu_p->scr_cb.load_scr_cb(menu_gui_scr->id);

    lv_obj_t *menu_btn = NULL;
    lv_polygonmenu_button_t *btn_p = NULL;
    int32_t idx = 0;
    for (; idx < child_cnt; idx++) {
        lv_obj_t *child = lv_obj_get_child(menu_obj, idx);
        btn_p = (lv_polygonmenu_button_t *)child;
        if (btn_p->scr_id == act_gui_scr->id && !btn_p->is_hide) {
            menu_btn = child;
            break;
        }
    }
    if (!menu_btn) {
        return;
    }

    // 初始化过渡状态
    bool scr_cb_exist = is_scr_cb_existing(menu_obj);
    if (scr_cb_exist) {
        clear_move_tmr(menu_obj);

        // 初始化 menu_scr
        lv_obj_set_style_transform_pivot_x(menu_scr, 0, 0);
        lv_obj_set_style_transform_pivot_y(menu_scr, 0, 0);
        lv_obj_set_style_transform_rotation(menu_scr, 0, 0);
        // 初始化时菜单放在屏幕外。从而避免在返回的一开始时menu_transform_zoom_task未开始，导致菜单闪一下
        lv_obj_set_style_translate_x(menu_obj, lv_display_get_horizontal_resolution(disp) + LV_ABS(lv_obj_get_x(menu_obj)), 0);
        lv_obj_set_style_translate_y(menu_obj, lv_display_get_vertical_resolution(disp) + LV_ABS(lv_obj_get_y(menu_obj)), 0);

        lv_obj_set_style_opa(act_gui_scr->scr, LV_OPA_100, 0);
        lv_obj_set_pos(act_gui_scr->scr, 0, 0);
        lv_obj_set_style_transform_pivot_x(act_gui_scr->scr, 0, 0);
        lv_obj_set_style_transform_pivot_y(act_gui_scr->scr, 0, 0);
        lv_obj_set_style_transform_rotation(act_gui_scr->scr, 0, 0);
        lv_obj_set_style_transform_scale(act_gui_scr->scr, 256, 0);

        menu_p->prev_status = STATUS_ZOOM_9;
        menu_p->status = STATUS_ZOOM_4;

        icon_arrangment(menu_obj, get_icon_size_by_zoom_status(menu_obj, STATUS_ZOOM_4));
        reset_offset_by_active_btn_idx(menu_obj, idx);
        icon_position(menu_obj);

        disp->prev_scr = act_gui_scr->scr;

        lv_obj_set_style_bg_opa(menu_gui_scr->scr, LV_OPA_0, 0);
        lv_obj_set_style_bg_opa(menu_obj, LV_OPA_0, 0);
        init_bg_scr(menu_gui_scr->scr);
        // disp->bg_scr = bg_scr;

        create_menu_transform_zoom_task(menu_obj, menu_p->prev_status, menu_p->status, idx, true);
    }
}



/*=====================
 * Setter functions
 *====================*/


void lv_polygonmenu_set_shape(lv_obj_t *menu_obj, lv_polygonmenu_shape_t shape)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    LV_ASSERT(shape == LV_POLYGONMENU_SHAPE_SQUARE || shape == LV_POLYGONMENU_SHAPE_HEXAGON);

    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    if (menu_p->shape != shape) {
        menu_p->shape = shape;
        reset_menu(menu_obj);
    }
}

void lv_polygonmenu_set_icon_min_size(lv_obj_t *menu_obj,
                                      int32_t icon_min_size)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    if (menu_p->icon_min_size != icon_min_size) {
        menu_p->icon_min_size = icon_min_size;
        reset_menu(menu_obj);
    }
}

void lv_polygonmenu_set_item_transform_cb(lv_obj_t *menu_obj, lv_polygonmenu_item_transform_cb_t cb)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    menu_p->item_transform_cb = cb;
    reset_menu(menu_obj);
}

void lv_polygonmenu_set_style_param(lv_obj_t *menu_obj, lv_polygonmenu_style_param_t *style_param)
{
    if (style_param) {
        LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
        lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
        menu_p->style_param = *style_param;
        reset_menu(menu_obj);
    }
}

void lv_polygonmenu_button_set_hide(lv_obj_t *btn_obj, bool hide)
{
    LV_ASSERT_OBJ(btn_obj, &lv_polygonmenu_button_class);
    lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)btn_obj;
    if (btn_p->is_hide != hide) {
        btn_p->is_hide = hide;
        lv_obj_t *menu_obj = lv_obj_get_parent(btn_obj);
        LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
        reset_menu(menu_obj);
    }
}

void lv_polygonmenu_set_scr_cb(lv_obj_t *menu_obj, lv_polygonmenu_scr_cb scr_cb)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    menu_p->scr_cb = scr_cb;
    reset_menu(menu_obj);
}

void lv_polygonmenu_button_set_scr_id(lv_obj_t *btn_obj, int32_t scr_id)
{
    LV_ASSERT_OBJ(btn_obj, &lv_polygonmenu_button_class);
    lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)btn_obj;

    lv_obj_t *menu_obj = lv_obj_get_parent(btn_obj);
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);

    btn_p->scr_id = scr_id;
    reset_menu(menu_obj);
}



/*=====================
 * Getter functions
 *====================*/

lv_polygonmenu_shape_t lv_polygonmenu_get_shape(lv_obj_t *menu_obj)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    return menu_p->shape;
}

int32_t lv_polygonmenu_get_icon_min_size(lv_obj_t *menu_obj)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    return menu_p->icon_min_size;
}

lv_polygonmenu_item_transform_cb_t lv_polygonmenu_get_item_transform_cb(lv_obj_t *menu_obj)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    return menu_p->item_transform_cb;
}

lv_polygonmenu_style_param_t *lv_polygonmenu_get_style_param(lv_obj_t *menu_obj)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    return &menu_p->style_param;
}

bool lv_polygonmenu_button_is_hide(lv_obj_t *btn_obj)
{
    LV_ASSERT_OBJ(btn_obj, &lv_polygonmenu_button_class);
    lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)btn_obj;
    return btn_p->is_hide;
}

lv_polygonmenu_scr_cb lv_polygonmenu_get_scr_cb(lv_obj_t *menu_obj)
{
    LV_ASSERT_OBJ(menu_obj, &lv_polygonmenu_class);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    return menu_p->scr_cb;
}

int32_t lv_polygonmenu_button_get_scr_id(lv_obj_t *btn_obj)
{
    LV_ASSERT_OBJ(btn_obj, &lv_polygonmenu_button_class);
    lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)btn_obj;

    return btn_p->scr_id;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void init_bg_scr(lv_obj_t *obj)
{
    if (bg_scr == NULL) {
        bg_scr = lv_obj_create(NULL);
    }
    lv_obj_set_pos(bg_scr, 0, 0);
    lv_obj_set_size(bg_scr, lv_display_get_horizontal_resolution(lv_display_get_default()), lv_display_get_vertical_resolution(lv_display_get_default()));
    /* 使用scr的背景色  */
    lv_obj_set_style_bg_color(bg_scr, lv_obj_get_style_bg_color(obj, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bg_scr, LV_OPA_100, LV_PART_MAIN);
}


static void lv_polygonmenu_class_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)obj;

    menu_p->shape = LV_POLYGONMENU_SHAPE_SQUARE;
    // menu_p->shape = LV_POLYGONMENU_SHAPE_HEXAGON;
    menu_p->offset_x = 0;
    menu_p->offset_y = 0;
    menu_p->icon_size = 0;
    menu_p->icon_width = 0;
    menu_p->icon_height = 0;
    menu_p->prev_status = STATUS_ZOOM_2;
    menu_p->status = STATUS_ZOOM_2;
    menu_p->move_tmr = lv_timer_create(move_timer_cb, LV_DEF_REFR_PERIOD, obj);
    menu_p->move_tmr_data = NULL;
    menu_p->temp_status_report_tmr = lv_timer_create(temp_status_report_cb, MENU_REBOUND_DELAY_MS, obj);

    menu_p->item_transform_cb = rect_change_circle_cb;
    menu_p->style_param.star.sphere_r_corner = 150;
    menu_p->style_param.star.sphere_distortion_coeff = 0.1;
    menu_p->style_param.star.sphere_radius = 200;
    menu_p->icon_min_size = 25;

    menu_p->scr_cb.get_setup_scr_cb = NULL;
    menu_p->scr_cb.is_scr_act_cb = NULL;
    menu_p->scr_cb.load_scr_cb = NULL;

    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_polygonmenu_class_destructor(const struct _lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)obj;
    clear_move_tmr(obj);

    if (menu_p->move_tmr) {
        lv_timer_delete(menu_p->move_tmr);
    }
    if (menu_p->temp_status_report_tmr) {
        lv_timer_delete(menu_p->temp_status_report_tmr);
    }

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_polygonmenu_button_class_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_polygonmenu_button_t *menu_btn = (lv_polygonmenu_button_t *)obj;
    menu_btn->is_hide = false;
    menu_btn->offset_x = 0;
    menu_btn->offset_y = 0;
    menu_btn->scr_id = -1;

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(obj, bubble_event_cb, LV_EVENT_ALL, NULL);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_polygonmenu_event(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);
    lv_result_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RESULT_OK) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)obj;

    if (code == LV_EVENT_PRESSED) {
        int32_t child_cnt = lv_obj_get_child_count(obj);
        if (!child_cnt || !menu_p->icon_size) {
            return;
        }
        if (menu_p->status == STATUS_ZOOM_2) {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_2_MOVING);
        } else if (menu_p->status == STATUS_ZOOM_4) {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_4_MOVING);
        }
    } else if (code == LV_EVENT_PRESSING) {
        if (menu_p->status == STATUS_ZOOM_2_MOVING || menu_p->status == STATUS_ZOOM_4_MOVING) {
            lv_indev_t *indev = lv_event_get_indev(e);
            // 叠加移动距离
            int32_t x_move_distance = indev->pointer.vect.x;
            int32_t y_move_distance = indev->pointer.vect.y;
            menu_p->offset_x += x_move_distance;
            menu_p->offset_y += y_move_distance;

            int32_t x, y;
            int32_t radius = 0;
            int32_t larger_icon_size = LV_MAX(menu_p->icon_width, menu_p->icon_height);
            if (menu_p->shape == LV_POLYGONMENU_SHAPE_SQUARE) {
                radius = larger_icon_size * menu_p->max_row_icon_count / 2.0 * powf(2, 0.5);
            } else if (menu_p->shape == LV_POLYGONMENU_SHAPE_HEXAGON) {
                radius = larger_icon_size * menu_p->max_row_icon_count / 2.0 + larger_icon_size / 3;
            }
            // 限制滑动范围
            normalize_menu_move_offset(radius, menu_p->offset_x, menu_p->offset_y, &x, &y);
            menu_p->offset_x = x;
            menu_p->offset_y = y;
            icon_position(obj);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (menu_p->status == STATUS_ZOOM_2_MOVING) {
            // 回到中心；
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_2_BACK);
            move_back(obj);
        } else if (menu_p->status == STATUS_ZOOM_4_MOVING) {
            lv_indev_t *indev = lv_event_get_indev(e);
            int32_t x_move_distance = indev->pointer.vect.x;
            int32_t y_move_distance = indev->pointer.vect.y;
            // 惯性
            float move_instance = hal_gfx_sqrt(x_move_distance * x_move_distance + y_move_distance * y_move_distance);
            // LV_LOG_WARN(" vect.x : %d  vect.y : %d ", x_move_distance, y_move_distance);
            if (move_instance > MENU_INERTIA_MOVING_THRESHOLD) {
                int target_x_offset = menu_p->offset_x + x_move_distance * 2;
                int target_y_offset = menu_p->offset_y + y_move_distance * 2;
                int32_t x, y;
                int32_t radius = LV_MAX(menu_p->icon_width, menu_p->icon_height) * menu_p->max_row_icon_count / 2.0;
                normalize_menu_move_offset(radius, target_x_offset, target_y_offset, &x, &y);
                move_instance = hal_gfx_sqrt((x - menu_p->offset_x) * (x - menu_p->offset_x) + (y - menu_p->offset_y) *
                                             (y - menu_p->offset_y));

                if (LV_ABS(menu_p->offset_x) < radius && LV_ABS(menu_p->offset_y) < radius &&
                    move_instance > MENU_INERTIA_MOVING_THRESHOLD) {
                    SET_MENU_STATUS(menu_p, STATUS_ZOOM_4_INERTIA_MOVING);
                    lv_polygonmenu_effect_slide_t *info_p = lv_malloc(sizeof(lv_polygonmenu_effect_slide_t));
                    LV_ASSERT_MALLOC(info_p);
                    info_p->type = EFFECT_SLIDE;
                    info_p->target_offset_x = x;
                    info_p->target_offset_y = y;
                    info_p->x_step = (info_p->target_offset_x - menu_p->offset_x) / 4;
                    info_p->y_step = (info_p->target_offset_y - menu_p->offset_y) / 4;

                    start_move_tmr(obj, info_p);

                    return;
                }
            }
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_4_BACK);
            move_back(obj);
        }
    } else if (code == LV_EVENT_SIZE_CHANGED) {
        // 完全重置;
        reset_menu(obj);
    } else if (code == LV_EVENT_CHILD_CREATED || code == LV_EVENT_CHILD_DELETED) {
        reset_menu(obj);
    }
}

static inline float get_transition_scr_opa_factor_by_status(lv_polygonmenu_status_t status)
{
    switch (status) {
    case STATUS_ZOOM_4:
        return 0.05 * LV_OPA_100;
    case STATUS_ZOOM_5:
        return 0.1 * LV_OPA_100;
    case STATUS_ZOOM_6:
        return 0.3 * LV_OPA_100;
    case STATUS_ZOOM_7:
        return 0.5 * LV_OPA_100;
    case STATUS_ZOOM_8:
        return 0.7 * LV_OPA_100;
    case STATUS_ZOOM_9:
        return LV_OPA_100;
    default:
        return LV_OPA_0;
    }
}
static inline float get_transition_btn_opa_factor_by_status(lv_polygonmenu_status_t status)
{
    switch (status) {
    case STATUS_ZOOM_4:
        return LV_OPA_100;
    case STATUS_ZOOM_5:
        return 0.7 * LV_OPA_100;
    case STATUS_ZOOM_6:
    case STATUS_ZOOM_7:
    case STATUS_ZOOM_8:
    case STATUS_ZOOM_9:
        return LV_OPA_0;
    default:
        return LV_OPA_0;
    }
}

static void move_timer_cb(lv_timer_t *timer_p)
{
    lv_obj_t *menu_obj = (lv_obj_t *)lv_timer_get_user_data(timer_p);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)lv_timer_get_user_data(timer_p);
    lv_polygonmenu_effect_t *effect_p = menu_p->move_tmr_data;
    if (!effect_p) {
        return;
    }

    switch (*effect_p) {
    case EFFECT_ZOOM: {
        lv_polygonmenu_effect_zoom_t *data_p = menu_p->move_tmr_data;
        int32_t icon_size = data_p->target_icon_size;
        int32_t idx = data_p->target_btn_idx;

        bool end = false;
        if (abs(icon_size - menu_p->icon_size) < LV_MAX(abs(data_p->icon_size_step), 5)) {
            end = true;
        } else {
            icon_size = menu_p->icon_size + data_p->icon_size_step;
        }

        icon_arrangment(menu_obj, icon_size);
        reset_offset_by_active_btn_idx(menu_obj, idx);
        icon_position(menu_obj);

        if (end) {
            clear_move_tmr(menu_obj);
            lv_timer_reset(menu_p->temp_status_report_tmr);

            if (menu_p->status == STATUS_ZOOM_2_BACK || menu_p->status == STATUS_ZOOM_4_BACK) {
                move_back(menu_obj);
            }
        }
        break;
    }

    case EFFECT_SLIDE: {
        lv_polygonmenu_effect_slide_t *data_p = menu_p->move_tmr_data;
        int32_t offset_x = data_p->target_offset_x;
        int32_t offset_y = data_p->target_offset_y;

        bool end = true;
        if (abs(offset_x - menu_p->offset_x) <= LV_MAX(abs(data_p->x_step), 5)) {
            offset_x = data_p->target_offset_x;
        } else {
            offset_x = menu_p->offset_x + data_p->x_step;
            end = false;
        }

        if (abs(offset_y - menu_p->offset_y) <= LV_MAX(abs(data_p->y_step), 5)) {
            offset_y = data_p->target_offset_y;
        } else {
            offset_y = menu_p->offset_y + data_p->y_step;
            end = false;
        }

        menu_p->offset_x = offset_x;
        menu_p->offset_y = offset_y;
        icon_position(menu_obj);
        if (end) {
            clear_move_tmr(menu_obj);
            lv_timer_reset(menu_p->temp_status_report_tmr);

            if (menu_p->status == STATUS_ZOOM_2_BACK) {
                SET_MENU_STATUS(menu_p, STATUS_ZOOM_2);
            } else if (menu_p->status == STATUS_ZOOM_4_INERTIA_MOVING) {
                SET_MENU_STATUS(menu_p, STATUS_ZOOM_4_BACK);
                move_back(menu_obj);
            } else if (menu_p->status == STATUS_ZOOM_4_BACK) {
                SET_MENU_STATUS(menu_p, STATUS_ZOOM_4);
            }
        }

        break;
    }
    case EFFECT_TRANSFORM_ZOOM: {
        lv_polygonmenu_effect_transform_zoom_t *data_p = menu_p->move_tmr_data;
        int32_t current_zoom = data_p->current_zoom;
        const int32_t target_zoom = data_p->target_zoom;
        const int32_t idx = data_p->target_btn_idx;
        lv_display_t *disp = lv_display_get_default();
        bool end = false;

        if (abs(current_zoom - target_zoom) <= LV_MAX(abs(data_p->zoom_step), 5)) {
            current_zoom = target_zoom;
            end = true;
        } else {
            current_zoom += data_p->zoom_step;
        }
        data_p->current_zoom = current_zoom;

        int32_t menu_width = lv_obj_get_width(menu_obj), menu_height = lv_obj_get_height(menu_obj);
        int32_t new_width = (float)current_zoom * menu_width / 256 ;
        int32_t new_height = (float)current_zoom * menu_height / 256 ;

        int32_t x = (new_width - menu_width) / 2;
        int32_t y = (new_height - menu_height) / 2;

        lv_obj_set_style_transform_scale(menu_obj, current_zoom, 0);
        lv_obj_set_style_translate_x(menu_obj, -x, 0);
        lv_obj_set_style_translate_y(menu_obj, -y, 0);
        lv_obj_invalidate(lv_obj_get_parent(menu_obj));

        bool scr_transition = is_scr_transition(menu_obj);

        // lv_obj_t *menu_scr = lv_obj_get_screen(menu_obj);
        // lv_obj_t *new_scr = disp->act_scr == menu_scr ? disp->prev_scr : disp->act_scr;
        lv_obj_t *menu_scr = disp->act_scr;
        lv_obj_t *new_scr = disp->prev_scr;

        if (scr_transition) {
            lv_obj_t *target_child = lv_obj_get_child(menu_obj, idx);
            lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)target_child;
            LV_ASSERT(btn_p->scr_id >= 0);

            int32_t scr_width = lv_display_get_horizontal_resolution(disp), scr_height = lv_display_get_vertical_resolution(disp);
            // scr_width / menu_width 和 scr_height / menu_height 用于 菜单和屏幕不等大时避免新屏幕大小突变
            int32_t new_scr_width = (float)lv_obj_get_content_width(target_child) * current_zoom / 256 * scr_width / menu_width ;
            int32_t new_scr_height = (float)lv_obj_get_content_height(target_child) * current_zoom / 256 * scr_height / menu_height;
            new_scr_width = LV_MIN(new_scr_width, scr_width);
            new_scr_height = LV_MIN(new_scr_height, scr_height);

            int32_t x1 = (scr_width - new_scr_width) / 2, y1 = (scr_height - new_scr_height) / 2;
            float new_scr_zoom_factor = (float)new_scr_width / scr_width;
            lv_obj_set_style_transform_scale(new_scr, 256 * new_scr_zoom_factor, 0);
            lv_obj_set_pos(new_scr, x1, y1);

            // 新屏幕透明度计算
            int32_t opa;
            float new_status_opa = get_transition_scr_opa_factor_by_status(menu_p->status);
            float prev_status_opa = get_transition_scr_opa_factor_by_status(menu_p->prev_status);
            opa = prev_status_opa + (new_status_opa - prev_status_opa) * ((float)current_zoom - data_p->initial_zoom) /
                  (target_zoom - data_p->initial_zoom);
            lv_obj_set_style_opa(new_scr, opa, 0);

            // 菜单按钮透明度计算
            new_status_opa = get_transition_btn_opa_factor_by_status(menu_p->status);
            prev_status_opa = get_transition_btn_opa_factor_by_status(menu_p->prev_status);
            opa = prev_status_opa + (new_status_opa - prev_status_opa) * ((float)current_zoom - data_p->initial_zoom) /
                  (target_zoom - data_p->initial_zoom);
            lv_obj_set_style_opa(target_child, opa, 0);
        }

        if (end) {
            clear_move_tmr(menu_obj);
            lv_timer_reset(menu_p->temp_status_report_tmr);

            if (menu_p->status == STATUS_ZOOM_4 || menu_p->status == STATUS_ZOOM_9) {
                lv_obj_t *target_child = lv_obj_get_child(menu_obj, idx);
                lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)target_child;
                lv_obj_set_style_opa(target_child, LV_OPA_100, 0);

                lv_obj_set_style_transform_scale(menu_obj, 256, 0);
                lv_obj_set_style_translate_x(menu_obj, 0, 0);
                lv_obj_set_style_translate_y(menu_obj, 0, 0);
                lv_obj_set_style_bg_opa(menu_scr, LV_OPA_100, 0);
                lv_obj_set_style_bg_opa(menu_obj, LV_OPA_100, 0);

                disp->prev_scr = NULL;
                // disp->bg_scr = NULL;

                if (scr_transition && btn_p->scr_id >= 0) {
                    lv_obj_set_style_opa(new_scr, LV_OPA_100, 0);
                    lv_obj_set_style_transform_scale(new_scr, 256, 0);
                    lv_obj_set_pos(new_scr, 0, 0);

                    if (menu_p->status == STATUS_ZOOM_4) {
                        // disp->act_scr = menu_scr;
                    } else {
                        // reset_menu(menu_obj);
                        menu_p->status = STATUS_ZOOM_4;
                        menu_p->prev_status = STATUS_ZOOM_3;
                        if (menu_p->scr_cb.load_scr_cb) {
                            menu_p->scr_cb.load_scr_cb(btn_p->scr_id);
                        }
                    }
                }
            }
        }
        break;
    }
    default: {
        LV_ASSERT(false);
    }
    }
}


static void temp_status_report_cb(lv_timer_t *timer_p)
{
    lv_obj_t *menu_obj = (lv_obj_t *)lv_timer_get_user_data(timer_p);
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)lv_timer_get_user_data(timer_p);
    if (menu_p->move_tmr_data) {
        return;
    }
    switch (menu_p->status) {
    case STATUS_ZOOM_0:
    case STATUS_ZOOM_1: {
        SET_MENU_STATUS(menu_p, STATUS_ZOOM_1);
        lv_polygonmenu_zoom_in(menu_obj);
        break;
    }
    case STATUS_ZOOM_3: {
        LV_ASSERT(menu_p->prev_status == STATUS_ZOOM_2 || menu_p->prev_status == STATUS_ZOOM_4);
        if (menu_p->prev_status == STATUS_ZOOM_2) {
            lv_polygonmenu_zoom_out(menu_obj);
        } else {
            lv_polygonmenu_zoom_in(menu_obj);
        }
        break;
    }
    case STATUS_ZOOM_5:
    case STATUS_ZOOM_6:
    case STATUS_ZOOM_7:
    case STATUS_ZOOM_8: {
        SET_MENU_STATUS(menu_p, STATUS_ZOOM_4);
        int32_t idx = get_menu_active_btn_idx(menu_obj);
        if (idx >= 0) {
            create_menu_transform_zoom_task(menu_obj, menu_p->prev_status, menu_p->status, idx, false);
        }
        break;
    }
    }
}



static void icon_arrangment(lv_obj_t *menu_obj, int32_t icon_size)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    int32_t menu_btn_cnt = lv_obj_get_child_count(menu_obj);
    if (!menu_btn_cnt) {
        return;
    }
    LV_ASSERT(menu_p->shape == LV_POLYGONMENU_SHAPE_SQUARE || menu_p->shape == LV_POLYGONMENU_SHAPE_HEXAGON);

    int32_t max_row_icon_count = get_menu_max_row_icon_count(menu_obj);
    menu_p->max_row_icon_count = max_row_icon_count;
    menu_p->icon_size = icon_size;
    if (!icon_size) {
        return;
    }

    int32_t icon_width = icon_size + lv_obj_get_style_pad_column(menu_obj, LV_PART_ITEMS);
    int32_t icon_height = icon_size + lv_obj_get_style_pad_row(menu_obj, LV_PART_ITEMS);

    menu_p->icon_width = icon_width;
    menu_p->icon_height = icon_height;

    // 中心点
    int32_t idx = 0;
    int32_t x = 0, y = 0;
    lv_polygonmenu_button_t *center_btn = NULL;
    // 找到第一个可见的icon
    while (idx < menu_btn_cnt) {
        lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
        idx++;
        if (btn_p) {
            center_btn = btn_p;
            center_btn->offset_x = x;
            center_btn->offset_y = y;
            break;
        }
    }
    if (!center_btn) {
        return;
    }

    // 初始排列，根据形状决定
    if (menu_p->shape == LV_POLYGONMENU_SHAPE_SQUARE) {
        // 偶数行时
        if (!(max_row_icon_count & 1)) {
            x = icon_width / 2;
            y = icon_height / 2;
            center_btn->offset_x = x;
            center_btn->offset_y = y;
        }
        x -= icon_width;
        // 从中心点的左一个开始计算
        int32_t step = 1;
        while (idx < menu_btn_cnt) {
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 上
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                y -= icon_height;
            }
            step++;
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 右
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                x += icon_width;
            }
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 下
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                y += icon_height;
            }
            step++;
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 左
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                x -= icon_width;
            }
        }
    } else if (menu_p->shape == LV_POLYGONMENU_SHAPE_HEXAGON) {
        const int32_t y_offset = icon_height * MENU_HEXAGON_ICON_Y_OFFSET_FACTOR;
        x -= icon_width;
        // 从中心点的左一个开始计算
        int32_t step = 1;
        while (idx < menu_btn_cnt) {
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 右上
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                x += icon_width / 2;
                y -= y_offset;
            }
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 右
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                x += icon_width;
            }
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 右下
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                x += icon_width / 2;
                y += y_offset;
            }
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 左下
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                x -= icon_width / 2;
                y += y_offset;
            }
            step++;
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 左
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                x -= icon_width;
            }
            step--;
            for (int32_t j = 0; idx < menu_btn_cnt && j < step; j++, idx++) {
                // 左上
                lv_polygonmenu_button_t *btn_p = get_visible_icon_by_idx(menu_obj, idx);
                if (!btn_p) {
                    j--;
                    continue;
                }

                btn_p->offset_x = x;
                btn_p->offset_y = y;
                x -= icon_width / 2;
                y -= y_offset;
            }
            step++;
        }
    }
}

static void icon_position(lv_obj_t *menu_obj)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    int32_t menu_btn_cnt = lv_obj_get_child_count(menu_obj);
    if (!menu_btn_cnt) {
        return;
    }


    int32_t icon_size = menu_p->icon_size;
    int32_t icon_width = menu_p->icon_width, icon_height = menu_p->icon_height;
    if (!menu_p->icon_size) {
        return;
    }

    lv_area_t menu_a;
    lv_obj_get_coords(menu_obj, &menu_a);

    const int32_t menu_content_width = lv_obj_get_content_width(menu_obj);
    const int32_t menu_content_height = lv_obj_get_content_height(menu_obj);

    const int32_t offset_x = menu_content_width / 2.0 + menu_p->offset_x - icon_width / 2;
    const int32_t offset_y = menu_content_height / 2.0 + menu_p->offset_y - icon_height / 2;
    const bool use_sphere_transform = menu_p->status == STATUS_ZOOM_4_MOVING ||
                                      menu_p->status == STATUS_ZOOM_4_INERTIA_MOVING || menu_p->status == STATUS_ZOOM_4_BACK ||
                                      menu_p->status == STATUS_ZOOM_4;

    const int32_t padding = use_sphere_transform ? icon_size - icon_size * MENU_HEXAGON_ICON_Y_OFFSET_FACTOR : 0;
    for (int32_t i = 0; i < menu_btn_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(menu_obj, i);
        lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)child;
        lv_image_t *img_p = (lv_image_t *)child;

        int32_t btn_pad_left = lv_obj_get_style_pad_left(child, LV_PART_MAIN);
        int32_t btn_pad_top = lv_obj_get_style_pad_top(child, LV_PART_MAIN);
        int32_t btn_border = lv_obj_get_style_border_width(child, LV_PART_MAIN);
        int32_t btn_x_edge_offset = btn_pad_left + btn_border;
        int32_t btn_y_edge_offset = btn_pad_top + btn_border;

        lv_point_t item_pos = {
            .x = btn_p->offset_x + offset_x - btn_x_edge_offset,
            .y = btn_p->offset_y + offset_y - btn_y_edge_offset
        };
        bool is_hide = btn_p->is_hide;
        bool hidden_flag = lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN);


        if (is_hide) {
            if (!hidden_flag) {
                lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
            }
            continue;
        }

        float zoom_factor = 1.0;
        if (img_p->src_type != LV_IMAGE_SRC_UNKNOWN) {
            int32_t img_size = LV_MIN(img_p->h, img_p->w);
            zoom_factor = (float)(LV_MAX(icon_size - padding, 0)) / img_size;
            int32_t new_zoom_val = zoom_factor * 256;
            if (img_p->scale_x != new_zoom_val) {
                lv_image_set_scale(child, new_zoom_val);
                lv_obj_refr_size(child);
            }
        }

        if (!use_sphere_transform) {
            lv_obj_set_pos(child, item_pos.x, item_pos.y);
            lv_obj_set_style_transform_scale(child, 256, 0);
            if (hidden_flag) {
                lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
            }
            // LV_LOG_WARN("\n i: %d icon-size:%d zoom-factor: %f item-size: %d zoom: 256  x: %d y: %d \n", i, icon_size, zoom_factor, lv_obj_get_width(child), item_pos.x, item_pos.y);
            continue;
        }

        int16_t zoom = 256;
        // item_pos改为icon中心位置
        item_pos.x += icon_width / 2;
        item_pos.y += icon_height / 2;
        if (menu_p->item_transform_cb) {
            // 暂时忽略pad和border的偏移
            item_pos.x += btn_x_edge_offset;
            item_pos.y += btn_y_edge_offset;
            zoom = menu_p->item_transform_cb(&item_pos, menu_obj, child);
            item_pos.x -= btn_x_edge_offset;
            item_pos.y -= btn_y_edge_offset;
        }

        if ((item_pos.x + icon_width < (menu_a.x1 - MENU_RENDER_MARGIN_HOR)) ||
            (item_pos.x - icon_width > (menu_a.x2 + MENU_RENDER_MARGIN_HOR)) ||
            (item_pos.y + icon_height < (menu_a.y1 - MENU_RENDER_MARGIN_VER)) ||
            (item_pos.y - icon_height > (menu_a.y2 + MENU_RENDER_MARGIN_VER))) {
            is_hide = true;
        }

        int32_t width = icon_width * zoom / 256;
        int32_t height = icon_height * zoom / 256;

        if (is_hide || LV_MAX(width, height) < menu_p->icon_min_size) {
            if (!hidden_flag) {
                lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
            }
            // if ((item_pos.y - icon_size > (menu_a.y2 + MENU_RENDER_MARGIN_VER))) {
            //     break;
            // }
        } else {
            // LV_LOG_WARN("\n i: %d icon-size:%d img-w: %d img-h:%d img-zoom: %d child-width: %d transform zoom: %d  x: %d y: %d \n",
            //   i, icon_size, img_p->w, img_p->h, img_p->zoom , lv_obj_get_width(child), zoom, item_pos.x, item_pos.y);

            // 不使用； img_zoom 搭配 transform_zoom ，在某种情况下会导致内存分配失败
            // lv_obj_set_style_transform_scale(child, zoom, 0);

            lv_image_set_scale(child, zoom_factor * zoom);
            lv_obj_refr_size(child);

            lv_obj_set_pos(child, item_pos.x - width / 2, item_pos.y - height / 2);
            if (hidden_flag) {
                lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}



static int32_t get_icon_size_by_zoom_status(lv_obj_t *menu_obj, lv_polygonmenu_status_t status)
{

    int32_t menu_width = lv_obj_get_content_width(menu_obj);
    int32_t menu_height = lv_obj_get_content_height(menu_obj);
    int32_t menu_size = LV_MIN(menu_width, menu_height);
    if (menu_size <= 0) {
        return 0;
    }

    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    int32_t max_row_icon_count = get_menu_max_row_icon_count(menu_obj);
    if (!max_row_icon_count) {
        return 0;
    }

    if (status == STATUS_ZOOM_9) {
        // 避免新屏幕最终大小有误差
        return menu_size;
    }


    LV_ASSERT(status >= STATUS_ZOOM_0 && status <= STATUS_ZOOM_4_BACK);
    if (status == STATUS_ZOOM_2_BACK) {
        status = STATUS_ZOOM_2;
    } else if (status == STATUS_ZOOM_4_BACK) {
        status = STATUS_ZOOM_4;
    }

    // 缩放最小时，btn_row_size 为 LV_MIN(menu_width - menu_horizotal_offset, menu_height - menu_vertical_offset) / MENU_ZOOM0_MAX_ROW_WIDTH_FACOTR
    // 缩放最大时，btn_row_size 为 menu_size * max_row_icon_count
    const float base = powf(1.0 / MENU_ZOOM0_MAX_ROW_WIDTH_FACOTR * max_row_icon_count,
                            1.0 / (STATUS_ZOOM_9 - STATUS_ZOOM_0));
    float btn_row_size = menu_size * MENU_ZOOM0_MAX_ROW_WIDTH_FACOTR * powf(base, status - STATUS_ZOOM_0);
    return btn_row_size / max_row_icon_count;

}

static void move_back(lv_obj_t *menu_obj)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    const int32_t menu_btn_cnt = lv_obj_get_child_count(menu_obj);
    if (menu_p->move_tmr_data) {
        return;
    }

    if (menu_p->status == STATUS_ZOOM_2_BACK) {
        if (menu_btn_cnt && (menu_p->offset_x || menu_p->offset_y)) {
            lv_polygonmenu_effect_slide_t *info_p = lv_malloc(sizeof(lv_polygonmenu_effect_slide_t));
            LV_ASSERT_MALLOC(info_p);
            info_p->type = EFFECT_SLIDE;
            info_p->target_offset_x = 0;
            info_p->target_offset_y = 0;
            info_p->x_step = -menu_p->offset_x / 3;
            info_p->y_step = -menu_p->offset_y / 3;

            start_move_tmr(menu_obj, info_p);
        } else {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_2);
        }
    } else if (menu_p->status == STATUS_ZOOM_4_BACK) {
        int32_t new_x = 0, new_y = 0;
        int32_t idx = get_menu_active_btn_idx(menu_obj);
        if (idx >= 0) {
            lv_obj_t *child = lv_obj_get_child(menu_obj, idx);
            lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)child;
            new_x = -btn_p->offset_x;
            new_y = -btn_p->offset_y;
        }
        if (new_x != menu_p->offset_x || new_y != menu_p->offset_y) {
            lv_polygonmenu_effect_slide_t *info_p = lv_malloc(sizeof(lv_polygonmenu_effect_slide_t));
            LV_ASSERT_MALLOC(info_p);

            info_p->type = EFFECT_SLIDE;
            info_p->target_offset_x = new_x;
            info_p->target_offset_y = new_y;
            info_p->x_step = (info_p->target_offset_x - menu_p->offset_x) / 3;
            info_p->y_step = (info_p->target_offset_y - menu_p->offset_y) / 3;

            start_move_tmr(menu_obj, info_p);
        } else {
            SET_MENU_STATUS(menu_p, STATUS_ZOOM_4);
        }
    }
}

static int16_t rect_change_circle_cb(lv_point_t *icon_pos, const lv_obj_t *obj, const lv_obj_t *btn)
{
#define SQUARE(x) ((x) * (x))
#define PYTHAG(x, y) hal_gfx_sqrt(SQUARE(x) + SQUARE(y))
#define PYTHAG3D(x, y, z) hal_gfx_sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z))

    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)obj;

    const int32_t width = lv_obj_get_content_width(obj);
    const int32_t height = lv_obj_get_content_height(obj);
    const int32_t x_center = width / 2;
    const int32_t y_center = height / 2;
    const int32_t icon_size = lv_obj_get_content_width(btn); /* 图标的大小 */
    const int32_t icon_radius = icon_size / 2;       /* 图标的半径 */
    // const int32_t r_corner = LV_MIN(x_center, y_center); /* 菜单的圆角半径 */
    const uint16_t r_corner = LV_MIN(LV_MIN(x_center, y_center), menu_p->style_param.star.sphere_r_corner);

    /* 边缘检测阈值 */
    const int32_t edge_det_threshold = LV_MAX(icon_radius, r_corner); /* 边缘检测阈值 */
    const float inner_width_half = (width / 2.0f) - icon_radius;      /* 内部宽度的一半 */
    const float inner_height_half = (height / 2.0f) - icon_radius;    /* 内部高度的一半 */

    // const float sphere_radius = 700.f; /* 球体半径 */
    // const float distortion_coeff = 1.9f; /* 畸变系数 */
    const float sphere_radius = menu_p->style_param.star.sphere_radius;
    const float distortion_coeff = menu_p->style_param.star.sphere_distortion_coeff;

    float scale = 0;

    float cx = icon_pos->x - x_center;
    float cy = icon_pos->y - y_center;
    float r2d = PYTHAG(cx, cy);

    /* 球面变换 */
    if (r2d > 1.f) {
        float r3d = PYTHAG3D(cx, cy, sphere_radius);
        float force = (r3d - sphere_radius) * distortion_coeff;
        icon_pos->x -= force * cx / r3d;
        icon_pos->y -= force * cy / r3d;
    }

    bool on_edge = false;
    float ex = 0;
    float ey = 0;

    /* 边缘检测 */
    if (edge_det_threshold == x_center && edge_det_threshold == y_center) {
        on_edge = true;
        ex = edge_det_threshold - icon_pos->x;
        ey = edge_det_threshold - icon_pos->y;
    } else {
        if (icon_pos->x < edge_det_threshold) {
            ex = edge_det_threshold - icon_pos->x;
            on_edge = true;
        } else if (icon_pos->x > (width - edge_det_threshold)) {
            ex = width - icon_pos->x - edge_det_threshold;
            on_edge = true;
        }

        if (icon_pos->y < edge_det_threshold) {
            ey = edge_det_threshold - icon_pos->y;
            on_edge = true;
        } else if (icon_pos->y > (height - edge_det_threshold)) {
            ey = height - icon_pos->y - edge_det_threshold;
            on_edge = true;
        }
    }

    if (on_edge) {
        float rr = PYTHAG(ex, ey);
        float dist2edge = edge_det_threshold - rr;

        if (dist2edge < icon_radius) {
            scale = dist2edge / icon_size + 0.5;
            // scale = dist2edge / icon_size + 1.0;
            icon_pos->x += (1.f - scale) * icon_radius * (ex / rr);
            icon_pos->y += (1.f - scale) * icon_radius * (ey / rr);
        } else {
            on_edge = false;
        }
    }

    if (!on_edge) {
        float scale_x = LV_MAX((inner_width_half - fabsf(cx)) / inner_width_half, 0);
        float scale_y = LV_MAX((inner_height_half - fabsf(cy)) / inner_height_half, 0);
        // scale = 1.f + 0.6f * scale_x * scale_y;
        scale = 1.f + 0.15f * scale_x * scale_y;
        if (scale < 1.f) {
            scale = 1.f;
        }
    }

    return (int16_t)(scale * 256);
}


static int32_t get_menu_active_btn_idx(lv_obj_t *menu_obj)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    const int32_t menu_btn_cnt = lv_obj_get_child_count(menu_obj);

    if (menu_btn_cnt) {
        int32_t new_x, new_y, new_i;
        for (int32_t i = 0; i < menu_btn_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(menu_obj, i);
            lv_polygonmenu_button_t *btn_p = (lv_polygonmenu_button_t *)child;
            if (btn_p->is_hide) {
                continue;
            }

            if (!i) {
                new_x = btn_p->offset_x;
                new_y = btn_p->offset_y;
                new_i = 0;
                continue;
            }

            if (
                pow(btn_p->offset_x - (-menu_p->offset_x), 2) + pow(btn_p->offset_y - (-menu_p->offset_y), 2) <
                pow(new_x - (-menu_p->offset_x), 2) + pow(new_y - (-menu_p->offset_y), 2)) {
                new_x = btn_p->offset_x;
                new_y = btn_p->offset_y;
                new_i = i;
            }
        }
        return new_i;
    }

    return -1;
}

static void reset_offset_by_active_btn_idx(lv_obj_t *menu_obj, int32_t idx)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;

    if (idx >= 0) {
        const lv_obj_t *child = lv_obj_get_child(menu_obj, idx);
        const lv_polygonmenu_button_t *btn = (lv_polygonmenu_button_t *)child;

        menu_p->offset_x = -btn->offset_x;
        menu_p->offset_y = -btn->offset_y;
        return;
    }

    menu_p->offset_x = 0;
    menu_p->offset_y = 0;
    return;
}

static lv_polygonmenu_button_t *get_visible_icon_by_idx(lv_obj_t *menu_obj, int32_t idx)
{
    lv_obj_t *child = lv_obj_get_child(menu_obj, idx);
    lv_polygonmenu_button_t *btn = (lv_polygonmenu_button_t *)child;
    if (btn->is_hide) {
        return NULL;
    }
    return btn;
}


static void reset_menu(lv_obj_t *menu_obj)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    lv_obj_t *scr = lv_obj_get_screen(menu_obj);

    lv_obj_set_style_opa(scr, LV_OPA_100, 0);
    lv_obj_set_style_transform_scale(scr, 256, 0);
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_100, 0);

    if (scr == lv_screen_active()) {
        lv_display_t *disp = lv_display_get_default();
        // disp->act_scr = scr;
        if (disp->prev_scr != NULL) {
            lv_obj_set_style_opa(disp->prev_scr, LV_OPA_100, 0);
            lv_obj_set_style_transform_scale(disp->prev_scr, 256, 0);
            lv_obj_set_pos(disp->prev_scr, 0, 0);
            lv_obj_set_style_bg_opa(disp->prev_scr, LV_OPA_100, 0);
            disp->prev_scr = NULL;
        }
        // disp->bg_scr = NULL;
    }

    lv_obj_set_style_transform_scale(menu_obj, 256, 0);
    lv_obj_set_style_translate_x(menu_obj, 0, 0);
    lv_obj_set_style_translate_y(menu_obj, 0, 0);
    lv_obj_set_style_bg_opa(menu_obj, LV_OPA_100, 0);


    if (is_scr_transition(menu_obj) && menu_p->move_tmr_data) {
        lv_polygonmenu_effect_zoom_t *data_p = menu_p->move_tmr_data;
        LV_ASSERT(data_p->type == EFFECT_ZOOM);

        lv_obj_t *child = lv_obj_get_child(menu_obj, data_p->target_btn_idx);
        lv_obj_set_style_opa(child, LV_OPA_100, 0);
    }

    menu_p->status = STATUS_ZOOM_2;
    menu_p->prev_status = STATUS_ZOOM_2;

    clear_move_tmr(menu_obj);
    menu_p->offset_x = 0;
    menu_p->offset_y = 0;

    icon_arrangment(menu_obj, get_icon_size_by_zoom_status(menu_obj, menu_p->status));
    icon_position(menu_obj);
}

// 计算最长的一行的图标个数
static int32_t get_menu_max_row_icon_count(lv_obj_t *menu_obj)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;

    int32_t menu_btn_cnt = lv_obj_get_child_count(menu_obj);
    if (!menu_btn_cnt) {
        return 0;
    }

    int32_t visible_count = 0;
    for (int32_t i = 0 ; i < menu_btn_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(menu_obj, i);
        lv_polygonmenu_button_t *btn = (lv_polygonmenu_button_t *)child;
        if (!btn->is_hide) {
            visible_count++;
        }
    }
    if (!visible_count) {
        return 0;
    }

    if (menu_p->shape == LV_POLYGONMENU_SHAPE_SQUARE) {
        int32_t length = (int)sqrt(visible_count);
        return length * length == visible_count ? length : (length + 1);
    } else {
        LV_ASSERT(menu_p->shape == LV_POLYGONMENU_SHAPE_HEXAGON);
        int32_t item_count_on_edge = (int)(ceil((sqrt(12 * visible_count - 3) + 3) / 6.0));
        return item_count_on_edge * 2 - 1;
    }
}


static void create_menu_transform_zoom_task(lv_obj_t *menu_obj, lv_polygonmenu_status_t prev_status,
        lv_polygonmenu_status_t target_status, int32_t target_btn_idx, bool long_time)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    LV_ASSERT(menu_p->move_tmr_data == NULL);

    // EFFECT_TRANSFORM_ZOOM
    int32_t status4_icon_size = get_icon_size_by_zoom_status(menu_obj, STATUS_ZOOM_4);
    int32_t current_icon_size = get_icon_size_by_zoom_status(menu_obj, prev_status);
    int32_t next_icon_size = get_icon_size_by_zoom_status(menu_obj, target_status);

    //计算zoom
    int32_t current_zoom = (float)current_icon_size / status4_icon_size * 256;
    int32_t target_zoom = (float)next_icon_size / status4_icon_size * 256;

    lv_polygonmenu_effect_transform_zoom_t *info_p = lv_malloc(sizeof(lv_polygonmenu_effect_transform_zoom_t));
    LV_ASSERT_MALLOC(info_p);
    info_p->type = EFFECT_TRANSFORM_ZOOM;
    info_p->target_btn_idx = target_btn_idx;
    info_p->target_zoom = target_zoom;
    info_p->current_zoom = current_zoom;
    info_p->zoom_step = (target_zoom - current_zoom) / 4;
    if (long_time) {
        info_p->zoom_step = (target_zoom - current_zoom) / 6;
    }
    info_p->initial_zoom = current_zoom;

    start_move_tmr(menu_obj, info_p);
}


static float hal_gfx_sqrt(uint32_t x)
{
    union {
        float f;
        unsigned int i;
    } u;

    if (x == 0) {
        return 0.0f;
    }

    u.f = x;
    u.i += 0x3F800000U;
    u.i >>= 1;

    return u.f;
}

static void clear_move_tmr(lv_obj_t *menu_obj)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    lv_timer_pause(menu_p->move_tmr);
    if (menu_p->move_tmr_data) {
        lv_free(menu_p->move_tmr_data);
        menu_p->move_tmr_data = NULL;
    }
}

static void start_move_tmr(lv_obj_t *menu_obj, void *tmr_data)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    lv_timer_resume(menu_p->move_tmr);
    LV_ASSERT(!menu_p->move_tmr_data);
    menu_p->move_tmr_data = tmr_data;
}

static void bubble_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_PRESSED:
    case LV_EVENT_PRESSING:
    case LV_EVENT_PRESS_LOST:
    case LV_EVENT_LONG_PRESSED:
    case LV_EVENT_LONG_PRESSED_REPEAT:
    case LV_EVENT_RELEASED:
    case LV_EVENT_HIT_TEST:
        lv_obj_send_event(lv_obj_get_parent(obj), code, lv_event_get_param(e));
        return;
    default:
        return;
    }
}



static void normalize_menu_move_offset(float radius, int32_t x, int32_t y, int32_t *x_p, int32_t *y_p)
{
    float length = hal_gfx_sqrt(x * x + y * y);

    if (length > radius) {
        float t = radius / length;
        *x_p = t * x;
        *y_p = t * y;
    } else {
        *x_p = x;
        *y_p = y;
    }
}

static bool is_scr_transition(lv_obj_t *menu_obj)
{
    lv_display_t *disp = lv_display_get_default();
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    lv_obj_t *menu_scr = lv_obj_get_screen(menu_obj);
    return (
               (menu_p->status >= STATUS_ZOOM_5 && menu_p->status <= STATUS_ZOOM_9) ||
               (menu_p->prev_status >= STATUS_ZOOM_5 && menu_p->status == STATUS_ZOOM_4)) &&
           disp->prev_scr != NULL;
}

static inline bool is_scr_cb_existing(lv_obj_t *menu_obj)
{
    lv_polygonmenu_t *menu_p = (lv_polygonmenu_t *)menu_obj;
    return menu_p->scr_cb.get_setup_scr_cb != NULL && menu_p->scr_cb.is_scr_act_cb != NULL && menu_p->scr_cb.load_scr_cb !=
           NULL;
}


#endif
