/**
 * @file lv_kaleidoscopemenu.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_kaleidoscopemenu_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/indev/lv_indev_private.h"
#include "math.h"

#if LV_USE_KALEIDOSCOPEMENU != 0

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_kaleidoscopemenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif


/*********************
 *      DEFINES
 *********************/

// #define ICON_WIDTH_HEIGHT_MAX   90
// #define ICON_INTERVAL_MAX       10
// #define ICON_SIGLE_MOVE         20

#define ICON_WIDTH_HEIGHT_MAX(menu)  (menu->icon_size)      //控件宽高
#define ICON_INTERVAL_MAX(menu)   (menu->ring_interval)     //控件间隔
#define ICON_SIGLE_MOVE(menu)    (menu->icon_single_move)        //单次移动距离，要能被(ICON_WIDTH_HEIGHT_MAX+ICON_INTERVAL_MAX)整除

#define UP_CENTER_INTERVAL      0               //抬手居中间隔，参考值0-5
#define ICON_RATIO_MIN          (0.3f)          //最小缩放系数，不能低于0.125
#define ICON_RATIO_MAX          (1.0f)          //最大缩放系数，不建议超过2

#define ICON_NUM_IN_CIRCLE      8               //每圈图标数量 需要能被360整除

#define ENERGY_CNT_MAX          (3)             //惯性最大计数值
#define ENERGY_TIME_INTERVAL    (30)            //参考值50-100
#define ENERGY_DISTANCE (20)                    // 惯性判定距离

#define C2LT(X,WH)      ((X)-(WH/2))                //图标中心转左上角坐标
#define LT2C(X,WH)      ((X)+(WH/2))                //图标左上角转中心坐标
#define ABS(x)          ((x)>0?(x):(-(x)))          //绝对值
#define DIR(x)          ((x)>0?(1):(-1))            //方向
#define MIN(a,b)        ((a<b)?(a):(b))             //最小值
#define SQUARE(x)         ((x) * (x))               // 平方
#define FABS_SQRT(x,y)  fabs(hal_gfx_sqrt(SQUARE(x) + SQUARE(y)))

#ifndef PI
#define PI 3.14159265
#endif

#define MY_CLASS &lv_kaleidoscopemenu_class


/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    KALE_DIR_NULL = 0,
    KALE_DIR_OUT = 1,
    KALE_DIR_IN = -1,
} lv_kaleidoscopemenu_move_dir_t;


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_kaleidoscopemenu_class_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_kaleidoscopemenu_class_destructor(const struct _lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_kaleidoscopemenu_button_class_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_kaleidoscopemenu_event(const lv_obj_class_t *class_p, lv_event_t *e);
static void touch_release_timer(lv_timer_t *timer);

static void icon_position(lv_obj_t *menu_obj, int r_offset, int dir);
static void icon_info_set(lv_obj_t *menu_obj, int btn_index, int icon_index);
static void bubble_event_cb(lv_event_t *e);

static float hal_gfx_sqrt(uint32_t x);
static inline bool is_visible_menu_btn(lv_obj_t *obj);
static inline bool is_menu_btn(lv_obj_t *obj);


/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_kaleidoscopemenu_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_kaleidoscopemenu_class_constructor,
    .destructor_cb = lv_kaleidoscopemenu_class_destructor,
    .event_cb = lv_kaleidoscopemenu_event,
    .width_def = 200,
    .height_def = 200,
    .instance_size = sizeof(lv_kaleidoscopemenu_t),
};

const lv_obj_class_t lv_kaleidoscopemenu_button_class = {
    .base_class = &lv_image_class,
    .constructor_cb = lv_kaleidoscopemenu_button_class_constructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_kaleidoscopemenu_button_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_kaleidoscopemenu_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    return obj;
}

lv_obj_t *lv_kaleidoscopemenu_add_button(lv_obj_t *parent, const void *icon)
{
    LV_ASSERT_OBJ(parent, &lv_kaleidoscopemenu_class);

    lv_obj_t *menu_btn = lv_obj_class_create_obj(&lv_kaleidoscopemenu_button_class, parent);
    lv_obj_class_init_obj(menu_btn);
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)parent;

    lv_image_t *img_p = (lv_image_t *)menu_btn;
    lv_obj_add_event_cb(menu_btn, bubble_event_cb, LV_EVENT_ALL, NULL);
    if (icon) {
        lv_image_set_src(menu_btn, icon);
    }
    if (img_p->src_type != LV_IMAGE_SRC_UNKNOWN) {
        lv_kaleidoscopemenu_button_t *kaleidoscopemenu_btn_p = (lv_kaleidoscopemenu_button_t *)menu_btn;
        int32_t img_size = LV_MIN(img_p->h, img_p->w);
    }
    icon_position(parent, 0, KALE_DIR_NULL);

    return menu_btn;
}


/*=====================
 * Setter functions
 *====================*/

static void reset_icon_single_move(lv_kaleidoscopemenu_t *menu)
{
    uint32_t interval_and_size = menu->ring_interval + menu->icon_size;

    if (interval_and_size % 5 == 0) {
        menu->icon_single_move = interval_and_size / 5;
        return;
    }

    // 最终要保证 single_move 能够被 (interval + icon_size) 整除
    uint16_t single_move = LV_MAX(interval_and_size / 3, 1);
    while (interval_and_size % single_move != 0 && single_move > 1) {
        single_move--;
    }
    menu->icon_single_move = single_move;
}

void lv_kaleidoscopemenu_set_icon_size(lv_obj_t *obj, uint16_t icon_size)
{
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)obj;
    menu->icon_size = icon_size;

    reset_icon_single_move(menu);
    icon_position(obj, 0, KALE_DIR_NULL);
}

void lv_kaleidoscopemenu_set_ring_interval(lv_obj_t *obj, uint16_t ring_interval)
{
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)obj;
    menu->ring_interval = ring_interval;

    reset_icon_single_move(menu);
    icon_position(obj, 0, KALE_DIR_NULL);
}

void lv_kaleidoscopemenu_button_set_hide(lv_obj_t *obj, bool hide)
{
    lv_kaleidoscopemenu_button_t *btn = (lv_kaleidoscopemenu_button_t *)obj;
    btn->_hide = hide;

    lv_obj_t *parent = lv_obj_get_parent(obj);
    LV_ASSERT_OBJ(parent, MY_CLASS);
    icon_position(parent, 0, KALE_DIR_NULL);
}


/*=====================
 * Getter functions
 *====================*/

uint16_t lv_kaleidoscopemenu_get_icon_size(lv_obj_t *obj)
{
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)obj;
    return menu->icon_size;
}
uint16_t lv_kaleidoscopemenu_get_ring_interval(lv_obj_t *obj)
{
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)obj;
    return menu->ring_interval;
}
bool lv_kaleidoscopemenu_is_hide(lv_obj_t *obj)
{
    lv_kaleidoscopemenu_button_t *btn = (lv_kaleidoscopemenu_button_t *)obj;
    return btn->_hide;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_kaleidoscopemenu_class_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)obj;
    menu->icon_size = 90;
    menu->ring_interval = 10;
    menu->icon_single_move = 20;

    menu->roffset = 0;
    menu->cnt = 0;
    menu->last_dir = KALE_DIR_NULL;
    menu->last_move_distance = 0;
    menu->key_elm_index = 0;

    menu->touch_release_timer = lv_timer_create(touch_release_timer, LV_DEF_REFR_PERIOD, obj);
    lv_timer_pause(menu->touch_release_timer);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_kaleidoscopemenu_class_destructor(const struct _lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)obj;
    if (menu->touch_release_timer) {
        lv_timer_delete(menu->touch_release_timer);
        menu->touch_release_timer = NULL;
    }

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_kaleidoscopemenu_button_class_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_kaleidoscopemenu_button_t *menu_btn = (lv_kaleidoscopemenu_button_t *)obj;
    menu_btn->_hide = false;

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_kaleidoscopemenu_event(const lv_obj_class_t *class_p, lv_event_t *e)
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
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)obj;

    if (code == LV_EVENT_PRESSED) {
        menu->last_dir = KALE_DIR_NULL;
        menu->last_move_distance = 0;
        lv_timer_pause(menu->touch_release_timer);
    } else if (code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_event_get_indev(e);
        //菜单中心
        float menu_center_x = lv_obj_get_width(obj) / 2.0;
        float menu_center_y = lv_obj_get_height(obj) / 2.0;
        //计算移动距离
        int32_t x_move_distance = indev->pointer.vect.x;
        int32_t y_move_distance = indev->pointer.vect.y;
        float move_distance = FABS_SQRT(x_move_distance, y_move_distance);
        //计算移动方向
        int32_t x_distance = indev->pointer.last_point.x - menu_center_x;
        int32_t y_distance = indev->pointer.last_point.y - menu_center_y;
        float start_point_to_parent_center_distance = FABS_SQRT(x_distance, y_distance);

        x_distance = indev->pointer.act_point.x - menu_center_x;
        y_distance = indev->pointer.act_point.y - menu_center_y;
        float end_point_to_parent_center_distance = FABS_SQRT(x_distance, y_distance);

        menu->last_dir = end_point_to_parent_center_distance > start_point_to_parent_center_distance ? KALE_DIR_OUT :
                         KALE_DIR_IN;
        menu->last_move_distance = move_distance;
        icon_position(obj, move_distance, menu->last_dir);

    } else if (code == LV_EVENT_RELEASED) {
        menu->energy_cnt = 0;
        if (menu->last_move_distance > ENERGY_DISTANCE) {
            lv_indev_t *indev = lv_event_get_indev(e);
            int energy_t = (indev->pointer.last_point.x + 1) & 0xffff ;
            int energy_x = indev->pointer.last_point.x;
            int energy_y = indev->pointer.last_point.y;
            float energy_r = FABS_SQRT(energy_x, energy_y);
            float energy_v = energy_r / energy_t;
            float energy_a = 0.05f;
            int energy_dist = energy_v * energy_v / 2 / energy_a;
            int energy_cnt = energy_dist / (ICON_WIDTH_HEIGHT_MAX(menu) + ICON_INTERVAL_MAX(menu));
            int center_cnt = (ICON_WIDTH_HEIGHT_MAX(menu) + ICON_INTERVAL_MAX(menu)) / ICON_SIGLE_MOVE(menu);
            LV_LOG_INFO("r:%.1f t:%d v:%.2f dist:%d cnt:%d", energy_r, energy_t, energy_v, energy_dist, energy_cnt);

            menu->energy_cnt = (energy_cnt > ENERGY_CNT_MAX) ? ENERGY_CNT_MAX : energy_cnt;
            menu->energy_cnt *= center_cnt;
        }
        lv_timer_resume(menu->touch_release_timer);
    } else if (code == LV_EVENT_SIZE_CHANGED) {
        menu->roffset = 0;
        icon_position(obj, 0, KALE_DIR_NULL);
        lv_timer_pause(menu->touch_release_timer);
    } else if (code == LV_EVENT_CHILD_CREATED || code == LV_EVENT_CHILD_DELETED) {
        menu->roffset = 0;
        icon_position(obj, 0, KALE_DIR_NULL);
        lv_timer_pause(menu->touch_release_timer);
    }
}

static void icon_position(lv_obj_t *menu_obj, int r_offset, int dir)
{
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)menu_obj;
    int32_t menu_child_cnt = lv_obj_get_child_count(menu_obj);
    if (!menu_child_cnt) {
        return;
    }

    menu->key_elm_index = menu->key_elm_index % menu_child_cnt;

    int distance = (ICON_WIDTH_HEIGHT_MAX(menu) + ICON_INTERVAL_MAX(menu));
    r_offset = (r_offset > distance) ? distance : r_offset;     //限制单次距离不要过大
    menu->roffset += (dir * r_offset);                        //累积偏移

    LV_LOG_INFO("%d %d %d", ABS(menu->roffset), distance, dir);

    for (int icon_index = 0, i = menu->key_elm_index;;) {
        lv_obj_t *child = lv_obj_get_child(menu_obj, i);
        if (is_visible_menu_btn(child)) {
            icon_info_set(menu_obj, i, icon_index++);
        } else if (is_menu_btn(child) && !lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
        }

        i = (i + 1) % menu_child_cnt;
        if (i == menu->key_elm_index) {
            break;
        }
    }

    if ((ABS(menu->roffset) >= distance) && (dir == KALE_DIR_OUT)) {     //向外移动
        //查找内圈的起始图标
        int final_key_elm_index = menu->key_elm_index;
        for (int i = (menu->key_elm_index + 1) % menu_child_cnt, icon_index = 0; i != menu->key_elm_index;
             i = (i + 1) % menu_child_cnt) {
            lv_obj_t *child = lv_obj_get_child(menu_obj, i);
            if (is_visible_menu_btn(child)) {
                icon_index++;
                final_key_elm_index = i;
            }
            if (icon_index == ICON_NUM_IN_CIRCLE) {
                break;
            }
        }

        menu->key_elm_index = final_key_elm_index;
        menu->roffset -= distance;        //调整半径
        menu->cnt = !menu->cnt;         //调整圈层参数
    } else if ((menu->roffset < 0) && (dir == KALE_DIR_IN)) {      //向内移动
        //查找外圈的起始图标
        int final_key_elm_index = menu->key_elm_index;
        for (int i = (menu->key_elm_index - 1 + menu_child_cnt) % menu_child_cnt, icon_index = 0; i != menu->key_elm_index;
             i = (i - 1 + menu_child_cnt) % menu_child_cnt) {
            lv_obj_t *child = lv_obj_get_child(menu_obj, i);
            if (is_visible_menu_btn(child)) {
                icon_index++;
                final_key_elm_index = i;
            }
            if (icon_index == ICON_NUM_IN_CIRCLE) {
                break;
            }
        }

        menu->key_elm_index = final_key_elm_index;
        menu->roffset += distance;        //调整半径
        menu->cnt = !menu->cnt;         //调整圈层参数
    }
}

static void icon_info_set(lv_obj_t *menu_obj, int btn_index, int icon_index)
{
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)menu_obj;

    float angle_full = 360.0f / ICON_NUM_IN_CIRCLE;     //全角：同一圈两个图标的间角
    float angle_half = 180.0f / ICON_NUM_IN_CIRCLE;     //半角：两圈之间图标的角度差

    int icon_center_x = 0;                              //图标中心x坐标
    int icon_center_y = 0;                              //图标中心y坐标
    int center_distance = 0;                            //图标与菜单中心点的距离

    int menu_content_width = lv_obj_get_content_width(menu_obj);
    int menu_content_height = lv_obj_get_content_height(menu_obj);

    int menu_radius = MIN(menu_content_width, menu_content_height) / 2;    //菜单最小内切圆半径
    int icon_radius = ICON_WIDTH_HEIGHT_MAX(menu) / 2;        //图标半径
    int ring_interval = ICON_INTERVAL_MAX(menu);              //两圈图标间的间隔
    int circle_index = icon_index % ICON_NUM_IN_CIRCLE; //所在圈层的序号
    int level_index = icon_index / ICON_NUM_IN_CIRCLE;  //第几圈

    lv_obj_t *btn = lv_obj_get_child(menu_obj, btn_index);

    //图标与极轴的角度
    float circle_icon_angle = circle_index * angle_full + ((menu->cnt + level_index) % 2) * angle_half;
    //极半径
    center_distance = menu->roffset + menu_radius - icon_radius - (ICON_WIDTH_HEIGHT_MAX(
                          menu) + ring_interval) * level_index;

    //极半径不能为负值
    if (center_distance - icon_radius < 0) {
        if (!lv_obj_has_flag(btn, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    } else {
        if (lv_obj_has_flag(btn, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_remove_flag(btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    //初始缩放
    lv_kaleidoscopemenu_button_t *menu_btn = (lv_kaleidoscopemenu_button_t *)btn;
    lv_image_t *img_p = (lv_image_t *)btn;
    if (img_p->src_type != LV_IMAGE_SRC_UNKNOWN) {
        int32_t img_size = LV_MIN(img_p->h, img_p->w);
        int32_t initial_zoom_val = (float)menu->icon_size / img_size * 256;
        if (lv_image_get_scale(btn) != initial_zoom_val) {
            lv_image_set_scale(btn, initial_zoom_val);
        }
    }

    const int32_t btn_border = lv_obj_get_style_border_width(btn, LV_PART_MAIN);
    const int32_t pad_left = lv_obj_get_style_pad_left(btn, LV_PART_MAIN);
    const int32_t pad_top = lv_obj_get_style_pad_top(btn, LV_PART_MAIN);

    //转换为以屏幕左上角为原点的直角坐标
    icon_center_x = center_distance * cosf(circle_icon_angle * PI / 180.0f);
    icon_center_y = center_distance * sinf(circle_icon_angle * PI / 180.0f);

    icon_center_x += menu_content_width / 2;
    icon_center_y += menu_content_height / 2;
    //计算缩放系数
    float min_float = ICON_RATIO_MIN;
    float max_float = ICON_RATIO_MAX;
    float real_float = min_float + ((float)center_distance / menu_radius) * (max_float - min_float);
    //将属性赋值给图标
    int32_t x = C2LT(icon_center_x, ICON_WIDTH_HEIGHT_MAX(menu) * real_float) - (pad_left + btn_border) * real_float;
    int32_t y = C2LT(icon_center_y, ICON_WIDTH_HEIGHT_MAX(menu) * real_float) - (pad_top + btn_border) * real_float;

    lv_obj_set_x(btn, x);
    lv_obj_set_y(btn, y);


    lv_obj_set_style_transform_scale(btn, real_float * 256, 0);

}


static void touch_release_timer(lv_timer_t *timer)
{
    lv_obj_t *menu_obj = (lv_obj_t *)lv_timer_get_user_data(timer);
    lv_kaleidoscopemenu_t *menu = (lv_kaleidoscopemenu_t *)menu_obj;

    // 惯性滑动
    if (menu->energy_cnt) {
        menu->energy_cnt--;
        if (menu->last_dir) {
            icon_position(menu_obj, ICON_SIGLE_MOVE(menu), menu->last_dir);
            return;
        }
    }

    // 惯性居中
    if (menu->roffset) {
        if (menu->last_dir == KALE_DIR_OUT) { //向外
            int distance = (ICON_WIDTH_HEIGHT_MAX(menu) + ICON_INTERVAL_MAX(menu));
            if (menu->roffset + ICON_SIGLE_MOVE(menu) < distance) {
                icon_position(menu_obj, ICON_SIGLE_MOVE(menu), menu->last_dir);
            } else {
                icon_position(menu_obj, distance - menu->roffset, menu->last_dir);
            }
        } else if (menu->last_dir == KALE_DIR_IN) { //向内
            if (menu->roffset - ICON_SIGLE_MOVE(menu) > 0) {
                icon_position(menu_obj, ICON_SIGLE_MOVE(menu), menu->last_dir);
            } else {
                icon_position(menu_obj, menu->roffset, menu->last_dir);
            }
        }
    } else {
        lv_timer_pause(menu->touch_release_timer);
        menu->roffset = 0;
        menu->energy_cnt = 0;
        icon_position(menu_obj, 0, KALE_DIR_NULL);
    }
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
        lv_obj_send_event(lv_obj_get_parent(obj), code, lv_event_get_param(e));
        return;
    default:
        return;
    }
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

static inline bool is_visible_menu_btn(lv_obj_t *obj)
{
    lv_kaleidoscopemenu_button_t *btn = (lv_kaleidoscopemenu_button_t *)obj;
    return lv_obj_get_class(obj) == &lv_kaleidoscopemenu_button_class && !btn->_hide;
}
static inline bool is_menu_btn(lv_obj_t *obj)
{
    lv_kaleidoscopemenu_button_t *btn = (lv_kaleidoscopemenu_button_t *)obj;
    return lv_obj_get_class(obj) == &lv_kaleidoscopemenu_button_class;
}


#endif
