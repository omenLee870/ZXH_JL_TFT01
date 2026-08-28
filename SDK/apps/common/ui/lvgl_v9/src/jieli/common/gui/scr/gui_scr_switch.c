/**
 * @file gui_scr_switch.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "gui_scr_switch.h"

#if LV_USE_GUI_SCR_SWITCH != 0
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/misc/lv_anim_timeline.h"

/*********************
 *      DEFINES
 *********************/

/* 重置页面状态，当页面模式发生变化时，如果某些页面的属性、样式（zoom、angle、x、y）没有及时更新，可能还是上一次的页面状态，所以需要重置 */
#define RESET_PREV_SCR_STATE(old_scr, new_prev_scr) \
    if (old_scr == NULL || (new_prev_scr != NULL && old_scr != new_prev_scr)) { \
        reset_scr_state(new_prev_scr, GUI_SCR_MOVE_MODE_CUSTOM); \
    }

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void anim_start_cb(struct _lv_anim_t *a);
static void anim_end_cb(struct _lv_anim_t *a);
static void reset_scr_state(lv_obj_t *scr, int32_t mode);

static void scr_anim_stop();
static bool scr_anim_is_running();
int32_t get_anim_end_scr_id(int32_t mode);

/* 处理不同模式下的动画 */
static int32_t handle_flip_mode_anim(gui_scr_switch_anim_t *anim, lv_obj_t *act_scr, lv_obj_t *prev_scr);
static int32_t handle_edge_mode_anim(gui_scr_switch_anim_t *anim, lv_obj_t *act_scr, lv_obj_t *prev_scr);
static int32_t handle_flip_zoom_mode_anim(gui_scr_switch_anim_t *anim, lv_obj_t *act_scr, lv_obj_t *prev_scr);
static int32_t handle_scale_opacity_mode_anim(gui_scr_switch_anim_t *anim, lv_obj_t *act_scr, lv_obj_t *prev_scr);

static void setup_anim(lv_anim_t *a, void *var, lv_anim_exec_xcb_t exec_cb,  int32_t start_value, int32_t end_value,
                       gui_scr_switch_anim_t *user_data, bool is_first_anim);
static void set_x_anim(void *obj, int32_t v);
static void set_y_anim(void *obj, int32_t v);
static void set_angle_anim(void *obj, int32_t v);
static void set_scale_anim(void *obj, int32_t v);
static void set_opa_anim(void *obj, int32_t v);

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_anim_timeline_t *anim_timeline = NULL;
static gui_scr_switch_get_scr_cb_t get_setuped_scr_cb = NULL;
static gui_scr_manager_t *switch_manager = NULL;
static lv_obj_t *bg_scr = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int32_t gui_scr_switch_move(gui_scr_switch_t *scr_switch, int32_t mode)
{
    if (get_setuped_scr_cb == NULL) {
        LV_LOG_WARN("get_scr_cb is NULL");
        return -1;
    }

    int32_t xoffset = scr_switch->cur_pos.x - scr_switch->start_pos.x;
    int32_t yoffset = scr_switch->cur_pos.y - scr_switch->start_pos.y;
    int32_t cur_src_id = gui_scr_get_id(gui_scr_get_act());

    gui_scr_t *cur_screen = gui_scr_get(cur_src_id);
    int32_t next_id = gui_scr_manager_next(switch_manager, cur_src_id);
    int32_t prev_id = gui_scr_manager_prev(switch_manager, cur_src_id);
    gui_scr_t *next_screen = get_setuped_scr_cb(next_id);
    gui_scr_t *prev_screen = get_setuped_scr_cb(prev_id);
    int32_t scr_width = lv_obj_get_width(cur_screen->scr);

    if (bg_scr == NULL) {
        bg_scr = lv_obj_create(NULL);
        lv_obj_set_pos(bg_scr, 0, 0);
        lv_obj_set_size(bg_scr, lv_display_get_horizontal_resolution(lv_display_get_default()), lv_display_get_vertical_resolution(lv_display_get_default()));
        /* 使用scr的背景色  */
        lv_obj_set_style_bg_color(bg_scr, lv_obj_get_style_bg_color(cur_screen->scr, LV_PART_MAIN), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bg_scr, lv_obj_get_style_bg_opa(cur_screen->scr, LV_PART_MAIN), LV_PART_MAIN);
    }

    if (prev_screen == NULL && scr_switch->loop_head) {
        prev_id = gui_scr_manager_tail(switch_manager);
        if (prev_id == -1 || prev_id == cur_src_id) {
            return -1;
        }
        prev_screen = get_setuped_scr_cb(prev_id);
    }
    if (next_screen == NULL && scr_switch->loop_tail) {
        next_id = gui_scr_manager_head(switch_manager);
        if (next_id == -1 || next_id == cur_src_id) {
            return -1;
        }
        next_screen = get_setuped_scr_cb(next_id);
    }

    scr_anim_stop(); /* 如果当前还有在运行松开动画，需要先停止动画 */

    switch (mode) {
    case GUI_SCR_MOVE_MODE_SLIDE: {
        xoffset = LV_CLAMP(-scr_width, xoffset,
                           scr_width); /* 限制偏移量在屏幕宽度范围内，避免从第一个页面直接切换显示前后第二个页面 */
        lv_display_t *disp = lv_display_get_default();
        int32_t act_scr_pos = lv_obj_get_x(disp->act_scr);

        if (xoffset > 0) {
            if (prev_screen != NULL) {
                RESET_PREV_SCR_STATE(disp->prev_scr, prev_screen->scr);
                disp->prev_scr = prev_screen->scr;
                lv_obj_set_pos(cur_screen->scr, xoffset, 0);
                lv_obj_set_pos(prev_screen->scr, -lv_obj_get_width(prev_screen->scr) + xoffset, 0);
            }
        } else if (xoffset < 0) {
            if (next_screen != NULL) {
                RESET_PREV_SCR_STATE(disp->prev_scr, next_screen->scr);
                disp->prev_scr = next_screen->scr;
                lv_obj_set_pos(cur_screen->scr, xoffset, 0);
                lv_obj_set_pos(next_screen->scr, lv_obj_get_width(next_screen->scr) + xoffset, 0);
            }
        } else {
            disp->prev_scr = NULL;
        }
        return 0;
    }
    case GUI_SCR_MOVE_MODE_ROTATE_EDGE: {
        xoffset = LV_CLAMP(-scr_width, xoffset, scr_width);
        lv_display_t *disp = lv_display_get_default();

        int32_t r_center_x = lv_obj_get_width(cur_screen->scr) / 2;
#if EDGE_SWITCH_MAX_ANGLE == 60
        /* 对于60度旋转：屏幕高度 + 宽度 * sin(60) */
        int32_t r_center_y = lv_obj_get_height(cur_screen->scr) + lv_obj_get_width(cur_screen->scr) * lv_trigo_sin(
                                 EDGE_SWITCH_MAX_ANGLE) / LV_TRIGO_SIN_MAX;
#elif EDGE_SWITCH_MAX_ANGLE == 45
        /* 对于45度旋转：屏幕高度 + 宽度 * sin(45) * 2 */
        int32_t r_center_y = lv_obj_get_height(cur_screen->scr) + (lv_obj_get_width(cur_screen->scr) * lv_trigo_sin(
                                 EDGE_SWITCH_MAX_ANGLE) * 2 / LV_TRIGO_SIN_MAX);
#endif

        if (xoffset > 0) {
            if (prev_screen != NULL) {
                RESET_PREV_SCR_STATE(disp->prev_scr, prev_screen->scr);
                int32_t angle = xoffset * EDGE_SWITCH_MAX_ANGLE / scr_width;
                disp->prev_scr = prev_screen->scr;
                // disp->bg_scr = bg_scr;
                lv_obj_set_style_transform_pivot_x(cur_screen->scr, r_center_x, LV_PART_MAIN);
                lv_obj_set_style_transform_pivot_y(cur_screen->scr, r_center_y, LV_PART_MAIN);
                lv_obj_set_style_transform_rotation(cur_screen->scr, angle * 10, LV_PART_MAIN);

                lv_obj_set_style_transform_pivot_x(prev_screen->scr, r_center_x, LV_PART_MAIN);
                lv_obj_set_style_transform_pivot_y(prev_screen->scr, r_center_y, LV_PART_MAIN);
                lv_obj_set_style_transform_rotation(prev_screen->scr, (angle - EDGE_SWITCH_MAX_ANGLE) * 10, LV_PART_MAIN);
            }
        } else if (xoffset < 0) {
            if (next_screen != NULL) {
                RESET_PREV_SCR_STATE(disp->prev_scr, next_screen->scr);
                int32_t angle = xoffset * EDGE_SWITCH_MAX_ANGLE / scr_width;
                disp->prev_scr = next_screen->scr;
                // disp->bg_scr = bg_scr;
                lv_obj_set_style_transform_pivot_x(cur_screen->scr, r_center_x, LV_PART_MAIN);
                lv_obj_set_style_transform_pivot_y(cur_screen->scr, r_center_y, LV_PART_MAIN);
                lv_obj_set_style_transform_rotation(cur_screen->scr, angle * 10, LV_PART_MAIN);

                lv_obj_set_style_transform_pivot_x(next_screen->scr, r_center_x, LV_PART_MAIN);
                lv_obj_set_style_transform_pivot_y(next_screen->scr, r_center_y, LV_PART_MAIN);
                lv_obj_set_style_transform_rotation(next_screen->scr, (angle + EDGE_SWITCH_MAX_ANGLE) * 10, LV_PART_MAIN);
            }
        } else {
            disp->prev_scr = NULL;
            // disp->bg_scr = NULL;
        }
        return 0;
    }
    case GUI_SCR_MOVE_MODE_SLIDE_ZOOM: {
        xoffset = LV_CLAMP(-scr_width, xoffset, scr_width);
        lv_display_t *disp = lv_display_get_default();

        /* 计算缩放比例和透明度 (0.0~1.0) */
        float ratio = (float)LV_ABS(xoffset) / scr_width;
        float left_scale = 1.0f - ratio * 0.3f;    // 左侧缩小30%
        float right_scale = 0.7f + ratio * 0.3f;   // 右侧从70%放大到100%
        lv_opa_t left_opa = LV_OPA_COVER * (1.0f - ratio); // 左侧逐渐透明
        lv_opa_t right_opa = LV_OPA_COVER * ratio;         // 右侧逐渐显现

        if (xoffset > 0) { // 右滑显示左侧页面
            if (prev_screen != NULL) {
                RESET_PREV_SCR_STATE(disp->prev_scr, prev_screen->scr);
                disp->prev_scr = prev_screen->scr;
                // disp->bg_scr = bg_scr;
                /* 当前页面右移+缩小+淡出 */
                lv_obj_set_x(cur_screen->scr, xoffset);
                lv_obj_set_style_transform_scale(cur_screen->scr, left_scale * 256, 0);
                lv_obj_set_style_opa(cur_screen->scr, left_opa, 0);

                /* 左侧页面从左边进入+放大+淡入 */
                lv_obj_set_x(prev_screen->scr, -scr_width + xoffset);
                lv_obj_set_style_transform_scale(prev_screen->scr, right_scale * 256, 0);
                lv_obj_set_style_opa(prev_screen->scr, right_opa, 0);
            }
        } else if (xoffset < 0) { // 左滑显示右侧页面
            if (next_screen != NULL) {
                RESET_PREV_SCR_STATE(disp->prev_scr, next_screen->scr);
                disp->prev_scr = next_screen->scr;
                // disp->bg_scr = bg_scr;
                /* 当前页面左移+缩小+淡出 */
                lv_obj_set_x(cur_screen->scr, xoffset);
                lv_obj_set_style_transform_scale(cur_screen->scr, left_scale * 256, 0);
                lv_obj_set_style_opa(cur_screen->scr, left_opa, 0);

                /* 右侧页面从右边进入+放大+淡入 */
                lv_obj_set_x(next_screen->scr, scr_width + xoffset);
                lv_obj_set_style_transform_scale(next_screen->scr, right_scale * 256, 0);
                lv_obj_set_style_opa(next_screen->scr, right_opa, 0);
            }
        } else {
            disp->prev_scr = NULL;
            // disp->bg_scr = NULL;
        }
        return 0;
    }
    case GUI_SCR_MOVE_MODE_SCALE_FADE: {
        xoffset = LV_CLAMP(-scr_width, xoffset, scr_width);
        lv_display_t *disp = lv_display_get_default();

        /* 计算缩放比例和透明度 (0.0~1.0) */
        float ratio = (float)LV_ABS(xoffset) / scr_width;

        // 计算屏幕中心点
        int32_t screen_width = lv_obj_get_width(cur_screen->scr);
        int32_t screen_height = lv_obj_get_height(cur_screen->scr);
        int32_t pivot_x = screen_width / 2;
        int32_t pivot_y = screen_height / 2;

        float curr_scale, other_scale;
        if (xoffset < 0) { // 左滑显示右侧页面
            // 当前页面从100%放大到200%
            curr_scale = 1.0f + ratio * 1.0f;
            // 下一页面从20%放大到100%
            other_scale = 0.2f + ratio * 0.8f;
        } else if (xoffset > 0) { // 右滑显示左侧页面
            // 当前页面从100%缩小到20%
            curr_scale = 1.0f - ratio * 0.8f;
            // 前一页面从200%缩小到100%
            other_scale = 2.0f - ratio;
        } else {
            curr_scale = 1.0f;
            other_scale = 1.0f;
        }

        // 透明度效果
        lv_opa_t curr_opa = LV_OPA_COVER * (1.0f - ratio); // 当前页面稍微降低透明度
        lv_opa_t other_opa = LV_OPA_COVER * ratio; // 其他页面增加透明度

        if (xoffset > 0) { // 右滑显示左侧页面
            if (prev_screen != NULL) {
                RESET_PREV_SCR_STATE(disp->prev_scr, prev_screen->scr);
                disp->prev_scr = prev_screen->scr;
                // disp->bg_scr = bg_scr;

                // 设置当前页面缩放中心点和缩放比例
                lv_obj_set_style_transform_pivot_x(cur_screen->scr, pivot_x, LV_PART_MAIN);
                lv_obj_set_style_transform_pivot_y(cur_screen->scr, pivot_y, LV_PART_MAIN);
                lv_obj_set_style_transform_scale(cur_screen->scr, curr_scale * 256, LV_PART_MAIN);
                lv_obj_set_style_opa(cur_screen->scr, curr_opa, LV_PART_MAIN);

                // 设置前一页面缩放中心点和缩放比例
                lv_obj_set_style_transform_pivot_x(prev_screen->scr, pivot_x, LV_PART_MAIN);
                lv_obj_set_style_transform_pivot_y(prev_screen->scr, pivot_y, LV_PART_MAIN);
                lv_obj_set_style_transform_scale(prev_screen->scr, other_scale * 256, LV_PART_MAIN);
                lv_obj_set_style_opa(prev_screen->scr, other_opa, LV_PART_MAIN);
            }
        } else if (xoffset < 0) { // 左滑显示右侧页面
            if (next_screen != NULL) {
                RESET_PREV_SCR_STATE(disp->prev_scr, next_screen->scr);
                disp->prev_scr = next_screen->scr;
                // disp->bg_scr = bg_scr;

                // 设置当前页面缩放中心点和缩放比例
                lv_obj_set_style_transform_pivot_x(cur_screen->scr, pivot_x, LV_PART_MAIN);
                lv_obj_set_style_transform_pivot_y(cur_screen->scr, pivot_y, LV_PART_MAIN);
                lv_obj_set_style_transform_scale(cur_screen->scr, curr_scale * 256, LV_PART_MAIN);
                lv_obj_set_style_opa(cur_screen->scr, curr_opa, LV_PART_MAIN);

                // 设置下一页面缩放中心点和缩放比例
                lv_obj_set_style_transform_pivot_x(next_screen->scr, pivot_x, LV_PART_MAIN);
                lv_obj_set_style_transform_pivot_y(next_screen->scr, pivot_y, LV_PART_MAIN);
                lv_obj_set_style_transform_scale(next_screen->scr, other_scale * 256, LV_PART_MAIN);
                lv_obj_set_style_opa(next_screen->scr, other_opa, LV_PART_MAIN);
            }
        } else {
            disp->prev_scr = NULL;
            // disp->bg_scr = NULL;
        }
        return 0;
    }
    default:
        break;
    }
    return -1;
}

int32_t gui_scr_switch_move_anim(gui_scr_switch_anim_t *anim)
{
    lv_display_t *disp = lv_display_get_default();
    if (disp == NULL) {
        return -1;
    }

    lv_obj_t *act_scr = disp->act_scr;
    lv_obj_t *prev_scr = disp->prev_scr;
    if (act_scr == NULL || prev_scr == NULL || act_scr == prev_scr) {
        return -1;
    }

    switch (anim->mode) {
    case GUI_SCR_MOVE_MODE_SLIDE:
        return handle_flip_mode_anim(anim, act_scr, prev_scr);
    case GUI_SCR_MOVE_MODE_ROTATE_EDGE:
        return handle_edge_mode_anim(anim, act_scr, prev_scr);
    case GUI_SCR_MOVE_MODE_SLIDE_ZOOM:
        return handle_flip_zoom_mode_anim(anim, act_scr, prev_scr);
    case GUI_SCR_MOVE_MODE_SCALE_FADE:
        return handle_scale_opacity_mode_anim(anim, act_scr, prev_scr);
    default:
        return -1;
    }
}

bool gui_scr_switch_is_enable()
{
    /* 判断当前页面是否有在页面管理器中，如果没有，说明当前页面不能进行页面切换 */
    gui_scr_t *act_scr = gui_scr_get_act();
    if (act_scr == NULL) {
        return false;
    }
    int32_t idx = gui_scr_manager_get_idx(switch_manager, act_scr->id);
    if (idx == -1) {
        return false;
    }

    return true;
}

void gui_scr_switch_set_get_scr_cb(gui_scr_switch_get_scr_cb_t get_scr_cb)
{
    get_setuped_scr_cb = get_scr_cb;
}

void gui_scr_switch_set_manager(gui_scr_manager_t *manager)
{
    switch_manager = manager;
}

lv_dir_t gui_scr_switch_get_dir(lv_point_t *start_pos, lv_point_t *end_pos)
{
    lv_dir_t dir = LV_DIR_NONE;
    int32_t x_diff = end_pos->x - start_pos->x;
    int32_t y_diff = end_pos->y - start_pos->y;
    if (LV_ABS(x_diff) > LV_ABS(y_diff)) {
        if (x_diff > DIR_THRESHOLD) {
            dir = LV_DIR_RIGHT;
        } else if (x_diff < -DIR_THRESHOLD) {
            dir = LV_DIR_LEFT;
        }
    } else {
        if (y_diff > DIR_THRESHOLD) {
            dir = LV_DIR_BOTTOM;
        } else if (y_diff < -DIR_THRESHOLD) {
            dir = LV_DIR_TOP;
        }
    }
    return dir;
}

lv_obj_t *gui_scr_switch_get_bg_scr(void)
{
    if (bg_scr == NULL) {
        bg_scr = lv_obj_create(NULL);
        lv_obj_set_size(bg_scr, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
        lv_obj_set_pos(bg_scr, 0, 0);
        lv_obj_set_style_bg_color(bg_scr, lv_obj_get_style_bg_color(lv_screen_active(), LV_PART_MAIN), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bg_scr, lv_obj_get_style_bg_opa(lv_screen_active(), LV_PART_MAIN), LV_PART_MAIN);
    }
    return bg_scr;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * 页面切换动画开始回调
 * @param a
 */
static void anim_start_cb(struct _lv_anim_t *a)
{
    gui_scr_switch_anim_t *anim = (gui_scr_switch_anim_t *)a->user_data;
    if (anim->anim_start_cb != NULL) {
        anim->anim_start_cb(anim->src_id);
    }
}

/**
 * 页面切换动画结束回调
 * @param a
 */
static void anim_end_cb(struct _lv_anim_t *a)
{
    // 如果动画被打断了，没有正常结束，不需要执行后续操作更新页面
    if (scr_anim_is_running() == false) {
        return;
    }

    gui_scr_switch_anim_t *anim = (gui_scr_switch_anim_t *)a->user_data;
    int32_t dest_scr_id = get_anim_end_scr_id(anim->mode);
    lv_display_t *disp = lv_display_get_default();
    lv_obj_t *act_scr = disp->act_scr;
    lv_obj_t *prev_scr = disp->prev_scr;

    if (anim->load_scr != NULL) {
        anim->load_scr(dest_scr_id);
    }
    if (anim->anim_end_cb != NULL) {
        anim->anim_end_cb(dest_scr_id);
    }
}

/**
 * @brief 复位页面状态
 * @param scr The screen object to reset
 * @param mode The transition mode to apply
 */
static void reset_scr_state(lv_obj_t *scr, int32_t mode)
{
    if (scr == NULL || lv_obj_is_valid(scr) == false) {
        return;
    }

    switch (mode) {
    case GUI_SCR_MOVE_MODE_SLIDE:
        lv_obj_set_x(scr, 0);
        break;
    case GUI_SCR_MOVE_MODE_ROTATE_EDGE:
        lv_obj_set_style_transform_pivot_x(scr, 0, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(scr, 0, LV_PART_MAIN);
        lv_obj_set_style_transform_rotation(scr, 0, LV_PART_MAIN);
        break;
    case GUI_SCR_MOVE_MODE_SLIDE_ZOOM:
        lv_obj_set_x(scr, 0);
        lv_obj_set_style_transform_scale(scr, 256, LV_PART_MAIN);
        lv_obj_set_style_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
        break;
    case GUI_SCR_MOVE_MODE_SCALE_FADE:
        lv_obj_set_x(scr, 0);
        lv_obj_set_style_transform_pivot_x(scr, 0, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(scr, 0, LV_PART_MAIN);
        lv_obj_set_style_transform_scale(scr, 256, LV_PART_MAIN);
        lv_obj_set_style_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
        break;
    default:
        lv_obj_set_x(scr, 0);
        lv_obj_set_style_transform_pivot_x(scr, 0, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(scr, 0, LV_PART_MAIN);
        lv_obj_set_style_transform_rotation(scr, 0, LV_PART_MAIN);
        break;
    }
}

/**
 * 中断当前页面切换动画
 */
static void scr_anim_stop()
{
    if (anim_timeline != NULL) {
        lv_anim_timeline_t *temp = anim_timeline;
        anim_timeline = NULL;
        lv_anim_timeline_delete(temp);
    }
}

/**
 * 判断页面切换动画是否正在运行
 * @return true 动画正在运行; false 动画未运行
 */
static bool scr_anim_is_running()
{
    return anim_timeline != NULL;
}

/**
 * 获取页面切换动画结束后要显示的页面ID
 * @param mode
 * @return int32_t
 */
int32_t get_anim_end_scr_id(int32_t mode)
{
    switch (mode) {
    case GUI_SCR_MOVE_MODE_SLIDE: {
        lv_display_t *disp = lv_display_get_default();
        lv_obj_t *act_scr = disp->act_scr;
        lv_obj_t *prev_scr = disp->prev_scr;
        if (act_scr == NULL || prev_scr == NULL) {
            return -1;
        }
        gui_scr_t *dest_scr = LV_ABS(lv_obj_get_x(act_scr)) < lv_obj_get_width(act_scr) * FLIP_SWITCH_THRESHOLD ?
                              gui_scr_get_act() : gui_scr_get_by_obj(prev_scr);
        return dest_scr == NULL ? -1 : dest_scr->id;
    }
    case GUI_SCR_MOVE_MODE_ROTATE_EDGE: {
        lv_display_t *disp = lv_display_get_default();
        lv_obj_t *act_scr = disp->act_scr;
        lv_obj_t *prev_scr = disp->prev_scr;
        if (act_scr == NULL || prev_scr == NULL) {
            return -1;
        }
        gui_scr_t *dest_scr = LV_ABS(lv_obj_get_style_transform_rotation(act_scr,
                                     LV_PART_MAIN)) < EDGE_SWITCH_MAX_ANGLE * EDGE_SWITCH_THRESHOLD ? gui_scr_get_act() : gui_scr_get_by_obj(prev_scr);
        return dest_scr == NULL ? -1 : dest_scr->id;
    }
    case GUI_SCR_MOVE_MODE_SLIDE_ZOOM: {
        lv_display_t *disp = lv_display_get_default();
        lv_obj_t *act_scr = disp->act_scr;
        lv_obj_t *prev_scr = disp->prev_scr;
        if (act_scr == NULL || prev_scr == NULL) {
            return -1;
        }

        // 基于实际宽度计算阈值
        int32_t screen_width = lv_obj_get_width(act_scr);
        int32_t threshold_pixels = screen_width * FLIP_SWITCH_THRESHOLD;
        bool should_switch = LV_ABS(lv_obj_get_x(act_scr)) >= threshold_pixels;

        gui_scr_t *dest_scr = should_switch ? gui_scr_get_by_obj(prev_scr) : gui_scr_get_act();
        // 强制重置目标页面状态（无论是否切换）
        if (dest_scr != NULL) {
            reset_scr_state(dest_scr->scr, GUI_SCR_MOVE_MODE_SLIDE_ZOOM);
            reset_scr_state(act_scr, GUI_SCR_MOVE_MODE_SLIDE_ZOOM);  // 同时重置当前页面
        }
        return dest_scr == NULL ? -1 : dest_scr->id;

    }
    case GUI_SCR_MOVE_MODE_SCALE_FADE: {
        lv_display_t *disp = lv_display_get_default();
        lv_obj_t *act_scr = disp->act_scr;
        lv_obj_t *prev_scr = disp->prev_scr;
        if (act_scr == NULL || prev_scr == NULL) {
            return -1;
        }

        lv_opa_t curr_opa = lv_obj_get_style_opa(act_scr, 0);
        int32_t curr_zoom = lv_obj_get_style_transform_scale_x(act_scr, 0);
        bool is_left_slide = curr_zoom > 256;  // 当前页面放大说明是左滑
        bool should_switch;
        if (is_left_slide) {
            // 左滑：当前页面放大超过160%或透明度小于70%，则切换页面
            should_switch = curr_zoom >= 256 * 1.6f || curr_opa <= LV_OPA_COVER * 0.7;
        } else {
            // 右滑：当前页面缩小小于40%或透明度小于70%，则切换页面
            should_switch = curr_zoom <= 256 * 0.4f || curr_opa <= LV_OPA_COVER * 0.7;
        }

        gui_scr_t *dest_scr = should_switch ? gui_scr_get_by_obj(prev_scr) : gui_scr_get_act();
        return dest_scr == NULL ? -1 : dest_scr->id;
    }
    default:
        return -1;
    }
}

/**
 * 处理Flip模式下的页面切换动画
 * @param anim
 * @param act_scr
 * @param prev_scr
 * @return int32_t
 */
static int32_t handle_flip_mode_anim(gui_scr_switch_anim_t *anim, lv_obj_t *act_scr, lv_obj_t *prev_scr)
{
    if (act_scr == NULL || prev_scr == NULL || act_scr == prev_scr) {
        return -1;
    }

    if (anim_timeline != NULL) {
        lv_anim_timeline_delete(anim_timeline);
    }
    anim_timeline = lv_anim_timeline_create();

    int32_t screen_width = lv_obj_get_width(act_scr);
    int32_t act_scr_pos = lv_obj_get_x(act_scr);
    bool is_slide_left = act_scr_pos < 0;
    bool should_switch = LV_ABS(act_scr_pos) >= screen_width * FLIP_SWITCH_THRESHOLD;

    /* 计算终点位置 */
    int32_t act_scr_end_pos = should_switch ?
                              (is_slide_left ? -screen_width : screen_width) : 0;
    int32_t prev_scr_end_pos = should_switch ?
                               0 : (is_slide_left ? screen_width : -screen_width);

    lv_anim_t anim_1, anim_2;
    setup_anim(&anim_1, act_scr, set_x_anim, act_scr_pos, act_scr_end_pos, anim, true);
    setup_anim(&anim_2, prev_scr, set_x_anim, lv_obj_get_x(prev_scr), prev_scr_end_pos, anim, false);

    lv_anim_timeline_add(anim_timeline, 0, &anim_1);
    lv_anim_timeline_add(anim_timeline, 0, &anim_2);

    lv_anim_timeline_start(anim_timeline);
    return 0;
}

/**
 * 处理Edge模式下的页面切换动画
 * @param anim
 * @param act_scr
 * @param prev_scr
 * @return int32_t
 */
static int32_t handle_edge_mode_anim(gui_scr_switch_anim_t *anim, lv_obj_t *act_scr, lv_obj_t *prev_scr)
{
    if (act_scr == NULL || prev_scr == NULL || act_scr == prev_scr) {
        return -1;
    }

    if (anim_timeline != NULL) {
        lv_anim_timeline_delete(anim_timeline);
    }
    anim_timeline = lv_anim_timeline_create();

    int16_t current_angle = lv_obj_get_style_transform_rotation(act_scr, LV_PART_MAIN) / 10;
    bool should_switch = LV_ABS(current_angle) >= (EDGE_SWITCH_MAX_ANGLE * EDGE_SWITCH_THRESHOLD);
    bool is_slide_left = current_angle < 0;

    /* 计算终点角度 */
    int32_t act_scr_end_angle = should_switch ?
                                (is_slide_left ? -EDGE_SWITCH_MAX_ANGLE : EDGE_SWITCH_MAX_ANGLE) : 0;
    int32_t prev_scr_end_angle = should_switch ?
                                 0 : (is_slide_left ? EDGE_SWITCH_MAX_ANGLE : -EDGE_SWITCH_MAX_ANGLE);

    lv_anim_t a1, a2;
    setup_anim(&a1, act_scr, set_angle_anim, current_angle * 10, act_scr_end_angle * 10, anim, true);
    lv_anim_set_path_cb(&a1, lv_anim_path_ease_out);

    int16_t prev_scr_angle = lv_obj_get_style_transform_rotation(prev_scr, LV_PART_MAIN) / 10;
    setup_anim(&a2, prev_scr, set_angle_anim, prev_scr_angle * 10, prev_scr_end_angle * 10, anim, false);
    lv_anim_set_path_cb(&a2, lv_anim_path_ease_out);

    lv_anim_timeline_add(anim_timeline, 0, &a1);
    lv_anim_timeline_add(anim_timeline, 0, &a2);

    lv_anim_timeline_start(anim_timeline);
    return 0;
}

/* 扩展动画处理函数 */
static int32_t handle_flip_zoom_mode_anim(gui_scr_switch_anim_t *anim, lv_obj_t *act_scr, lv_obj_t *prev_scr)
{
    if (act_scr == NULL || prev_scr == NULL || act_scr == prev_scr) {
        return -1;
    }

    scr_anim_stop();
    anim_timeline = lv_anim_timeline_create();

    /* 动态计算缩放基准值 */
    int32_t screen_width = lv_obj_get_width(act_scr);
    int32_t base_scale = 256;
    bool should_switch = LV_ABS(lv_obj_get_x(act_scr)) >= screen_width * FLIP_SWITCH_THRESHOLD;
    /* 计算动态缩放比例 */
    float ratio = (float)LV_ABS(lv_obj_get_x(act_scr)) / screen_width;
    float left_scale = 1.0f - ratio * 0.3f;    // 从 100% 缩小到 70%
    float right_scale = 0.7f + ratio * 0.3f;   // 从 70% 放大到 100%

    /* 当前页面动画参数 */
    int32_t act_scr_end_x = should_switch ? (lv_obj_get_x(act_scr) > 0 ? screen_width : -screen_width) : 0;
    int32_t act_scr_end_scale = (int32_t)(left_scale * base_scale);  // 动态计算缩放值
    lv_opa_t act_scr_end_opa = should_switch ? LV_OPA_TRANSP : LV_OPA_COVER;

    /* 目标页面动画参数 */
    int32_t prev_scr_end_x = should_switch ? 0 : (lv_obj_get_x(prev_scr) > 0 ? screen_width : -screen_width);
    int32_t prev_scr_end_scale = (int32_t)(right_scale * base_scale);
    lv_opa_t prev_scr_end_opa = should_switch ? LV_OPA_COVER : LV_OPA_TRANSP;

    /* 配置四个动画通道 */
    lv_anim_t a1, a2, a3, a4;

    // 当前页面X轴移动
    setup_anim(&a1, act_scr, set_x_anim, lv_obj_get_x(act_scr), act_scr_end_x, anim, true);

    // 当前页面缩放
    setup_anim(&a2, act_scr, set_scale_anim,
               lv_obj_get_style_transform_scale_x(act_scr, 0),
               act_scr_end_scale, anim, false);

    // 目标页面X轴移动
    setup_anim(&a3, prev_scr, set_x_anim, lv_obj_get_x(prev_scr), prev_scr_end_x, anim, false);

    // 目标页面缩放
    setup_anim(&a4, prev_scr, set_scale_anim, lv_obj_get_style_transform_scale_x(prev_scr, 0), prev_scr_end_scale, anim,
               false);

    /* 添加透明度动画 */
    lv_anim_set_exec_cb(&a2, set_opa_anim);
    lv_anim_set_values(&a2, lv_obj_get_style_opa(act_scr, 0), act_scr_end_opa);

    lv_anim_set_exec_cb(&a4, set_opa_anim);
    lv_anim_set_values(&a4, lv_obj_get_style_opa(prev_scr, 0), prev_scr_end_opa);

    /* 设置动画曲线 */
    lv_anim_set_path_cb(&a1, lv_anim_path_ease_out);
    lv_anim_set_path_cb(&a3, lv_anim_path_ease_out);
    lv_anim_set_path_cb(&a2, lv_anim_path_ease_in);
    lv_anim_set_path_cb(&a4, lv_anim_path_ease_in);
    /* 添加至时间轴 */
    lv_anim_timeline_add(anim_timeline, 0, &a1);
    lv_anim_timeline_add(anim_timeline, 50, &a2);
    lv_anim_timeline_add(anim_timeline, 50, &a3); // 延迟50ms产生交错效果
    lv_anim_timeline_add(anim_timeline, 50, &a4);

    lv_anim_timeline_start(anim_timeline);
    return 0;
}

/**
 * 处理Scale_Opacity模式下的页面切换动画
 * @param anim
 * @param act_scr
 * @param prev_scr
 * @return int32_t
 */
static int32_t handle_scale_opacity_mode_anim(gui_scr_switch_anim_t *anim, lv_obj_t *act_scr, lv_obj_t *prev_scr)
{
    if (act_scr == NULL || prev_scr == NULL || act_scr == prev_scr) {
        return -1;
    }

    scr_anim_stop();
    anim_timeline = lv_anim_timeline_create();

    int32_t screen_width = lv_obj_get_width(act_scr);
    int32_t screen_height = lv_obj_get_height(act_scr);
    int32_t pivot_x = screen_width / 2;
    int32_t pivot_y = screen_height / 2;

    lv_opa_t curr_opa = lv_obj_get_style_opa(act_scr, 0);
    int32_t curr_zoom = lv_obj_get_style_transform_scale_x(act_scr, 0);
    int32_t prev_zoom = lv_obj_get_style_transform_scale_x(prev_scr, 0);
    bool is_left_slide = curr_zoom > 256;  // 当前页面放大说明是左滑
    bool should_switch;
    if (is_left_slide) {
        // 左滑：当前页面放大超过160%或透明度小于70%，则切换页面
        should_switch = curr_zoom >= 256 * 1.6f || curr_opa <= LV_OPA_COVER * 0.7;
    } else {
        // 右滑：当前页面缩小小于40%或透明度小于70%，则切换页面
        should_switch = curr_zoom <= 256 * 0.4f || curr_opa <= LV_OPA_COVER * 0.7;
    }

    int32_t act_scr_end_scale, prev_scr_end_scale;
    if (is_left_slide) {
        act_scr_end_scale = should_switch ? (int32_t)(3.5f * 256) : 256;  // 350% 或 100%
        prev_scr_end_scale = should_switch ? 256 : 38;  // 100% 或 15%
    } else {
        act_scr_end_scale = should_switch ? 38 : 256;  // 15% 或 100%
        prev_scr_end_scale = should_switch ? 256 : 768;  // 100% 或 300%
    }

    lv_opa_t act_scr_end_opa = should_switch ? 0 : LV_OPA_COVER;
    lv_opa_t prev_scr_end_opa = should_switch ? LV_OPA_COVER : 0;
    lv_anim_t a1, a2, a3, a4;

    // 当前页面
    setup_anim(&a1, act_scr, set_scale_anim,
               curr_zoom,
               act_scr_end_scale, anim, true);
    lv_anim_set_path_cb(&a1, lv_anim_path_ease_out);
    setup_anim(&a2, act_scr, set_opa_anim,
               curr_opa,
               act_scr_end_opa, anim, false);
    lv_anim_set_path_cb(&a2, lv_anim_path_ease_in_out);

    // 目标页面
    setup_anim(&a3, prev_scr, set_scale_anim,
               prev_zoom,
               prev_scr_end_scale, anim, false);
    lv_anim_set_path_cb(&a3, lv_anim_path_ease_out);
    setup_anim(&a4, prev_scr, set_opa_anim,
               lv_obj_get_style_opa(prev_scr, 0),
               prev_scr_end_opa, anim, false);
    lv_anim_set_path_cb(&a4, lv_anim_path_ease_in_out);

    // 设置缩放中心点
    lv_obj_set_style_transform_pivot_x(act_scr, pivot_x, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(act_scr, pivot_y, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_x(prev_scr, pivot_x, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(prev_scr, pivot_y, LV_PART_MAIN);

    /* 添加至时间轴，稍微错开时间，让效果更流畅 */
    lv_anim_timeline_add(anim_timeline, 0, &a1);
    lv_anim_timeline_add(anim_timeline, 10, &a2);
    lv_anim_timeline_add(anim_timeline, 5, &a3);
    lv_anim_timeline_add(anim_timeline, 15, &a4);

    lv_anim_timeline_start(anim_timeline);
    return 0;
}

/**
 * 设置动画参数
 * @param a
 * @param var
 * @param exec_cb
 * @param start_value
 * @param end_value
 * @param user_data
 * @param is_first_anim
 */
static void setup_anim(lv_anim_t *a, void *var, lv_anim_exec_xcb_t exec_cb,  int32_t start_value, int32_t end_value,
                       gui_scr_switch_anim_t *user_data, bool is_first_anim)
{
    lv_anim_init(a);
    lv_anim_set_var(a, var);
    lv_anim_set_exec_cb(a, exec_cb);
    lv_anim_set_duration(a, 300);
    lv_anim_set_values(a, start_value, end_value);

    if (is_first_anim) {
        lv_anim_set_user_data(a, user_data);
        lv_anim_set_start_cb(a, anim_start_cb);
        lv_anim_set_deleted_cb(a, anim_end_cb);
    }
}

static void set_x_anim(void *obj, int32_t v)
{
    lv_obj_set_x(obj, v);
}

static void set_y_anim(void *obj, int32_t v)
{
    lv_obj_set_y(obj, v);
}

static void set_angle_anim(void *obj, int32_t v)
{
    lv_obj_set_style_transform_rotation(obj, v, LV_PART_MAIN);
}

/* 新增动画执行回调 */
static void set_scale_anim(void *obj, int32_t v)
{
    lv_obj_set_style_transform_scale(obj, v, LV_PART_MAIN);
}

static void set_opa_anim(void *obj, int32_t v)
{
    lv_obj_set_style_opa(obj, v, LV_PART_MAIN);
}

#endif /* LV_USE_GUI_SCR_SWITCH */
