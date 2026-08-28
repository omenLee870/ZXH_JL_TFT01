/**
 * @file lv_album.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_album_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/indev/lv_indev_private.h"
#include <string.h>
#include <math.h>

#if LV_USE_ALBUM != 0

#if LV_USE_IMAGE == 0
#error "lv_album: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_album_class

#define CLAMP(x, min, max)              (x < min ? min : (x > max ? max : x))

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_album_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_album_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_album_event_cb(const lv_obj_class_t *class_p, lv_event_t *e);

static bool srcs_reserve(lv_album_t *alb, uint16_t need_cap);
static void scroll_anim_exec_cb(void *var, int32_t val);
static void scroll_anim_ready_cb(lv_anim_t *a);
static void start_offset_anim(lv_album_t *alb, int32_t target, bool is_rebound);
static void set_tran_img_by_dir(lv_album_t *alb, int8_t dir_sign);
static void update_slide_effect(lv_album_t *alb, int32_t offset);
static void reset_images_state(lv_album_t *alb);
static void reset_scroll_state(lv_album_t *alb);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_album_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_album_constructor,
    .destructor_cb = lv_album_destructor,
    .event_cb = lv_album_event_cb,
    .width_def = 200,
    .height_def = 200,
    .instance_size = sizeof(lv_album_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_album_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    lv_album_t *alb = (lv_album_t *)obj;
    alb->main_cont = lv_obj_create(obj);
    alb->tran_cont = lv_obj_create(obj);
    lv_obj_set_size(alb->main_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_size(alb->tran_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(alb->main_cont, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(alb->tran_cont, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(alb->main_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(alb->tran_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_align(alb->main_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_align(alb->tran_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(alb->main_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(alb->tran_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(alb->main_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(alb->tran_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(alb->main_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(alb->tran_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(alb->main_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(alb->tran_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(alb->main_cont, true, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(alb->tran_cont, true, LV_PART_MAIN);

    alb->main_img = lv_image_create(alb->main_cont);
    alb->tran_img = lv_image_create(alb->tran_cont);

    lv_obj_add_flag(alb->tran_cont, LV_OBJ_FLAG_HIDDEN);

    return obj;
}

void lv_album_add_pic(lv_obj_t *album, const void *src)
{
    if (!album || !src) {
        return;
    }
    lv_album_t *alb = (lv_album_t *)album;

    if (!srcs_reserve(alb, alb->count + 1)) {
        return;
    }
    alb->srcs[alb->count++] = src;

    /* 初始化首图 */
    if (alb->act_idx < 0 && alb->main_cont) {
        alb->act_idx = 0;
        lv_image_set_src(alb->main_img, alb->srcs[0]);
        lv_obj_set_pos(alb->main_cont, 0, 0);
        lv_obj_set_style_transform_scale(alb->main_cont, 256, 0);
        lv_obj_set_style_opa(alb->main_cont, LV_OPA_COVER, 0);
        /* 若此前因清空而隐藏，首次添加时恢复显示 */
        lv_obj_remove_flag(alb->main_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_send_event(album, LV_EVENT_VALUE_CHANGED, NULL);
    }
}

void lv_album_remove_pic(lv_obj_t *album, uint16_t index)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return;
    }
    if (index >= alb->count) {
        return;
    }

    /* 停止动画，复位滚动状态 */
    lv_anim_delete(album, scroll_anim_exec_cb);
    reset_scroll_state(alb);

    /* 移动数组，删除 index */
    if (alb->count > 1 && index < alb->count - 1) {
        memmove((void *)(alb->srcs + index), (void *)(alb->srcs + index + 1),
                (alb->count - index - 1) * sizeof(alb->srcs[0]));
    }
    alb->count--;

    /* 调整当前索引 */
    if (alb->count == 0) {
        alb->act_idx = -1;
        if (alb->main_cont) {
            lv_image_set_src(alb->main_img, NULL);
            lv_obj_add_flag(alb->main_cont, LV_OBJ_FLAG_HIDDEN);
        }
        if (alb->tran_cont) {
            lv_image_set_src(alb->tran_img, NULL);
            lv_obj_add_flag(alb->tran_cont, LV_OBJ_FLAG_HIDDEN);
        }
        reset_images_state(alb);
        lv_obj_invalidate((lv_obj_t *)alb);
        return;
    }

    if (alb->act_idx > (int16_t)index) {
        alb->act_idx--;
    } else if (alb->act_idx == (int16_t)index) {
        /* 删除当前图，选择就近一张 */
        if (alb->act_idx >= (int16_t)alb->count) {
            alb->act_idx = (int16_t)alb->count - 1;
        }
    }

    /* 更新显示为新的 act_idx */
    if (alb->main_cont && alb->act_idx >= 0) {
        lv_image_set_src(alb->main_img, alb->srcs[alb->act_idx]);
        alb->scroll.offset = 0;
        alb->scroll.tran_index = -1;
        update_slide_effect(alb, 0);
        lv_obj_invalidate(alb->main_cont);
        lv_obj_invalidate((lv_obj_t *)alb);
        lv_obj_send_event((lv_obj_t *)alb, LV_EVENT_VALUE_CHANGED, NULL);
    }
}

void lv_album_remove_all_pic(lv_obj_t *album)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return;
    }

    /* 终止动画并复位 */
    lv_anim_delete(album, scroll_anim_exec_cb);
    reset_scroll_state(alb);

    /* 清空列表 */
    if (alb->srcs) {
        lv_free((void *)alb->srcs);
        alb->srcs = NULL;
    }
    alb->count = 0;
    alb->act_idx = -1;
    if (alb->main_cont) {
        lv_image_set_src(alb->main_img, NULL);
        lv_obj_add_flag(alb->main_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (alb->tran_cont) {
        lv_image_set_src(alb->tran_img, NULL);
        lv_obj_add_flag(alb->tran_cont, LV_OBJ_FLAG_HIDDEN);
    }
    reset_images_state(alb);
}

/*=====================
 * Setter functions
 *====================*/

void lv_album_set_act(lv_obj_t *album, int16_t act_idx)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return;
    }
    if (act_idx < 0 || act_idx >= (int16_t)alb->count || act_idx == alb->act_idx) {
        return;
    }
    alb->act_idx = act_idx;
    lv_image_set_src(alb->main_img, alb->srcs[alb->act_idx]);
    lv_obj_send_event((lv_obj_t *)alb, LV_EVENT_VALUE_CHANGED, NULL);
}

void lv_album_set_effect(lv_obj_t *album, lv_album_effect_t type, const lv_album_effect_cfg_t *config)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return;
    }

    alb->effect.type = type;
    if (config) {
        alb->effect = *config;
        alb->effect.type = type;
    } else {
        switch (type) {
        case LV_ALBUM_EFFECT_SLIDE:
            alb->effect.anim_time = 300;
            alb->effect.path_cb = lv_anim_path_ease_out;
            break;
        case LV_ALBUM_EFFECT_SLIDE_SCALE:
            alb->effect.anim_time = 300;
            alb->effect.path_cb = lv_anim_path_ease_out;
            alb->effect.params.slide_scale.scale_end = 256;   /* 1.0x */
            alb->effect.params.slide_scale.scale_start = 180; /* ~0.7x */
            alb->effect.params.slide_scale.fade_start = 50;
            alb->effect.params.slide_scale.fade_end = 255;
            break;
        case LV_ALBUM_EFFECT_SCALE_FADE:
            alb->effect.anim_time = 320;
            alb->effect.path_cb = lv_anim_path_ease_out;
            alb->effect.params.scale_fade.scale_end = 512;   /* 2.0x */
            alb->effect.params.scale_fade.scale_start = 51;  /* ~0.2x */
            break;
        case LV_ALBUM_EFFECT_EDGE_ROTATE:
            alb->effect.anim_time = 320;
            alb->effect.path_cb = lv_anim_path_ease_out;
            alb->effect.params.edge_rotate.angle = 60;
            break;
        case LV_ALBUM_EFFECT_FLIP_AXIS:
            alb->effect.anim_time = 320;
            alb->effect.path_cb = lv_anim_path_ease_out;
            break;
        }
    }
}

void lv_album_set_load_more_cb(lv_obj_t *album, lv_album_load_more_cb_t cb)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return;
    }
    alb->load_more_cb = cb;
}

void lv_album_set_transform_cb(lv_obj_t *album, lv_album_transform_cb_t cb)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return;
    }
    alb->transform_cb = cb;
}

void lv_album_set_auto_load(lv_obj_t *album, bool enable)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return;
    }
    alb->auto_load_more = enable;
}

/*=====================
 * Getter functions
 *=====================*/

uint16_t lv_album_get_count(lv_obj_t *album)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return 0;
    }
    return alb->count;
}

int16_t lv_album_get_act(lv_obj_t *album)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return -1;
    }
    return alb->act_idx;
}

const void *lv_album_get_act_pic(lv_obj_t *album)
{
    lv_album_t *alb = (lv_album_t *)album;
    if (!alb) {
        return NULL;
    }
    if (alb->act_idx < 0 || alb->act_idx >= (int16_t)alb->count) {
        return NULL;
    }
    return alb->srcs[alb->act_idx];
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_album_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_album_t *alb = (lv_album_t *)obj;

    alb->effect.type = LV_ALBUM_EFFECT_SLIDE;
    alb->effect.anim_time = 400;
    alb->effect.path_cb = lv_anim_path_ease_out;
    alb->effect.params.slide_scale.scale_end = 256;
    alb->effect.params.slide_scale.scale_start = 256;
    alb->effect.params.slide_scale.fade_start = 255;
    alb->effect.params.slide_scale.fade_end = 255;

    alb->main_cont = NULL;
    alb->tran_cont = NULL;
    alb->act_idx = -1;
    alb->auto_load_more = false;
    alb->syncing = false;
    alb->scroll.dir = LV_DIR_HOR; /* 默认水平滚动 */
    alb->scroll.offset = 0;
    alb->scroll.momentum = 0;     /* 作为 px/s 速度估计 */
    alb->scroll.is_scroll = false;
    alb->scroll.animating = false;
    alb->scroll.target = 0;
    alb->scroll.tran_index = -1;
    alb->scroll.drag_dir = 0;
    alb->scroll.offset_y = 0;
    alb->scroll.last_tick = 0;
    alb->scroll.axis_lock = 0;

    alb->transform_cb = NULL;

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_album_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_album_t *alb = (lv_album_t *)obj;
    if (alb->srcs) {
        lv_free((void *)alb->srcs);
        alb->srcs = NULL;
    }

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_album_event_cb(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);
    lv_result_t res;

    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RESULT_OK) {
        return;
    }

    lv_obj_t *obj = lv_event_get_current_target(e);
    lv_album_t *alb = (lv_album_t *)obj;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        /* 被打断时：停止动画，但保留当前位置与过渡图，避免坐标突变 */
        lv_anim_delete(obj, scroll_anim_exec_cb);
        alb->scroll.animating = false;
        alb->scroll.is_scroll = false;
        alb->scroll.momentum = 0;
        alb->scroll.drag_dir = 0; /* 方向重新判定 */
        alb->scroll.axis_lock = 0;/* 轴向重新判定 */
        alb->scroll.offset_y = 0;
        alb->scroll.last_tick = lv_tick_get();
        alb->syncing = false;     /* 重置一次性加载标记 */

        /* 若上次切换动画被打断：
         * - 若偏移已超过半宽且存在过渡图，直接判定切换完成；
         * - 否则回到中心，清理过渡状态，避免残留的大 offset 影响本次手势。 */
        int32_t w = lv_obj_get_width(obj);
        int32_t off = alb->scroll.offset;
        if (LV_ABS(off) > (w / 2) && alb->scroll.tran_index >= 0 && alb->count > 0) {
            alb->act_idx = alb->scroll.tran_index;
            lv_image_set_src(alb->main_img, alb->srcs[alb->act_idx]);
            alb->scroll.offset = 0;
            alb->scroll.target = 0;
            alb->scroll.tran_index = -1;
            reset_images_state(alb);
            lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
        } else if (off != 0 || alb->scroll.tran_index != -1) {
            alb->scroll.offset = 0;
            alb->scroll.target = 0;
            alb->scroll.tran_index = -1;
            reset_images_state(alb);
        }
    } else if (code == LV_EVENT_PRESSING) {

        if (alb->count == 0) {
            return;
        }
        lv_indev_t *indev = lv_event_get_indev(e);
        if (!indev) {
            return;
        }

        int32_t dx = indev->pointer.vect.x;
        int32_t dy = indev->pointer.vect.y;

        /* 计算像素/秒的速度估计 */
        uint32_t now = lv_tick_get();
        uint32_t dt = (alb->scroll.last_tick == 0) ? 0 : (now - alb->scroll.last_tick);
        alb->scroll.last_tick = now;
        if (dt > 0) {
            /* 即时速度 px/s */
            int32_t v_inst = (int32_t)dx * 1000 / (int32_t)dt;
            /* 指数平滑：alpha=0.3 */
            alb->scroll.momentum = (int32_t)((alb->scroll.momentum * 7 + v_inst * 3) / 10);
        }

        int32_t w = lv_obj_get_width(obj);
        int32_t h = lv_obj_get_height(obj);
        alb->scroll.offset = CLAMP(alb->scroll.offset + dx, -w, w);
        alb->scroll.offset_y = CLAMP(alb->scroll.offset_y + dy, -h, h);

        /* 轴向锁定：优先判断主导方向，垂直主导则不拦截 */
        if (alb->scroll.axis_lock == 0) {
            if (LV_ABS(alb->scroll.offset) > 10 || LV_ABS(alb->scroll.offset_y) > 10) {
                alb->scroll.axis_lock = (LV_ABS(alb->scroll.offset) >= LV_ABS(alb->scroll.offset_y)) ? 1 : 2;
            }
        }
        if (alb->scroll.axis_lock == 2) {
            /* 垂直主导：不作为水平滚动处理 */
            return;
        }

        if (!alb->scroll.is_scroll && LV_ABS(alb->scroll.offset) > 10) {
            alb->scroll.is_scroll = true;
        }
        int8_t dir_sign = (alb->scroll.offset > 0) ? 1 : ((alb->scroll.offset < 0) ? -1 : 0);

        if (dir_sign != 0 && (dir_sign != alb->scroll.drag_dir || alb->scroll.tran_index < 0)) {
            alb->scroll.drag_dir = dir_sign;
            set_tran_img_by_dir(alb, dir_sign);
        }

        update_slide_effect(alb, alb->scroll.offset);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (alb->count == 0) {
            return;
        }
        /* 若非滚动但存在偏移，说明是中断动画后未继续拖拽，直接回弹 */
        if (!alb->scroll.is_scroll) {
            if (alb->scroll.offset != 0) {
                alb->scroll.target = 0;
                start_offset_anim(alb, 0, true);
            }
            return;
        }
        int32_t w = lv_obj_get_width(obj);
        int32_t off = alb->scroll.offset;
        bool can_switch = (alb->scroll.tran_index >= 0);

        bool do_switch = false;
        if (can_switch) {
            /* 针对 FLIP_AXIS 特效使用 50% 阈值，其他特效使用 33% 阈值 */
            int32_t threshold = (alb->effect.type == LV_ALBUM_EFFECT_FLIP_AXIS) ? (w / 2) : (w / 3);
            if (LV_ABS(off) > threshold) {
                do_switch = true;
            } else {
                /* 速度阈值 */
                int32_t v_th = (int32_t)w * 3;
                if (v_th < 500) {
                    v_th = 500;    /* 最小阈值，避免过小屏幕过于敏感 */
                }
                if (LV_ABS(alb->scroll.momentum) > v_th) {
                    do_switch = true;
                }
            }
        }

        alb->scroll.target = do_switch ? ((off > 0) ? w : -w) : 0;
        start_offset_anim(alb, alb->scroll.target, alb->scroll.target == 0);
    }
}

static bool srcs_reserve(lv_album_t *alb, uint16_t need_cap)
{
    if (!alb) {
        return false;
    }
    if (need_cap == 0) {
        if (alb->srcs) {
            lv_free((void *)alb->srcs);
            alb->srcs = NULL;
        }
        return true;
    }

    const void **nsrcs = lv_realloc(alb->srcs, sizeof(const void *) * need_cap);
    if (!nsrcs) {
        return false;
    }
    alb->srcs = nsrcs;
    return true;
}

/*=====================
 * Internal helpers
 *====================*/
static void set_tran_img_by_dir(lv_album_t *alb, int8_t dir_sign)
{
    if (!alb || !alb->main_cont || !alb->tran_cont) {
        return;
    }
    if (alb->count == 0) {
        return;    /* 无图片不设置过渡图，也不触发加载 */
    }
    int16_t idx = alb->act_idx;
    int16_t want = -1;
    if (dir_sign > 0) {
        want = (idx >= 1) ? (idx - 1) : -1;    /* 右滑：上一张 */
    } else if (dir_sign < 0) {
        /* 左滑：下一张；若已是最后一张且允许自动加载，先尝试加载更多 */
        if (idx + 1 >= (int16_t)alb->count) {
            if (alb->auto_load_more && alb->load_more_cb && !alb->syncing) {
                alb->syncing = true; /* 本次手势仅尝试一次 */
                uint16_t old_cnt = alb->count;
                alb->load_more_cb((lv_obj_t *)alb);
                if (alb->count > old_cnt) {
                    want = (idx + 1 < (int16_t)alb->count) ? (idx + 1) : -1;
                } else {
                    want = -1;
                }
            } else {
                want = -1;
            }
        } else {
            want = idx + 1;
        }
    }

    if (want == alb->scroll.tran_index) {
        /* 目标未变化，避免重复设置 */
        return;
    }
    if (want < 0) {
        alb->scroll.tran_index = -1;
        lv_obj_add_flag(alb->tran_cont, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    alb->scroll.tran_index = want;
    lv_image_set_src(alb->tran_img, alb->srcs[want]);
    lv_obj_remove_flag(alb->tran_cont, LV_OBJ_FLAG_HIDDEN);

    int32_t w = lv_obj_get_width((lv_obj_t *)alb);
    if (dir_sign > 0) {
        lv_obj_set_pos(alb->tran_cont, -w, 0);
    } else if (dir_sign < 0) {
        lv_obj_set_pos(alb->tran_cont, w, 0);
    }
    lv_obj_set_style_transform_scale(alb->tran_cont, 256, 0);
    lv_obj_set_style_opa(alb->tran_cont, LV_OPA_COVER, 0);
}

static void update_slide_effect(lv_album_t *alb, int32_t offset)
{
    if (!alb || !alb->main_cont) {
        return;
    }
    if (alb->count == 0) {
        return;    /* 无图片时不执行任何位移/特效 */
    }
    int32_t w = lv_obj_get_width((lv_obj_t *)alb);
    int8_t dir_sign = (offset > 0) ? 1 : ((offset < 0) ? -1 : 0);

    /* 通用比例 */
    float ratio = w ? (float)LV_ABS(offset) / (float)w : 0.f;
    if (ratio > 1.f) {
        ratio = 1.f;
    }

    /* 若绑定了自定义变换回调，则交由外部处理，内置效果不再执行 */
    if (alb->transform_cb) {
        alb->transform_cb((lv_obj_t *)alb,
                          alb->main_cont ? alb->main_cont : NULL,
                          (alb->tran_cont && alb->scroll.tran_index >= 0) ? alb->tran_cont : NULL,
                          offset, w, ratio, dir_sign);
        return;
    }

    /* 先处理位置（部分模式会覆盖为固定 0） */
    lv_obj_set_x(alb->main_cont, offset);
    if (alb->scroll.tran_index >= 0 &&
        alb->tran_cont) {
        lv_obj_set_x(alb->tran_cont, (dir_sign >= 0) ? (offset - w) : (offset + w));
    }

    switch (alb->effect.type) {
    case LV_ALBUM_EFFECT_SLIDE: {
        break;
    }
    case LV_ALBUM_EFFECT_SLIDE_SCALE: {
        /* 当前缩小，目标放大，带透明过渡 */
        uint16_t z_big = alb->effect.params.slide_scale.scale_end ? alb->effect.params.slide_scale.scale_end : 256;
        uint16_t z_small = alb->effect.params.slide_scale.scale_start ? alb->effect.params.slide_scale.scale_start : 256;
        uint8_t fade_max = alb->effect.params.slide_scale.fade_end;
        uint16_t cur_zoom = (uint16_t)(z_big - (z_big - z_small) * ratio);
        uint16_t tran_zoom = (uint16_t)(z_small + (z_big - z_small) * ratio);
        lv_obj_set_style_transform_scale(alb->main_cont, cur_zoom, 0);
        if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
            lv_obj_set_style_transform_scale(alb->tran_cont, tran_zoom, 0);
        }
        uint8_t cur_opa = (uint8_t)(fade_max * (1.f - ratio));
        uint8_t tran_opa = (uint8_t)(fade_max * ratio);
        lv_obj_set_style_opa(alb->main_cont, cur_opa, 0);
        if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
            lv_obj_set_style_opa(alb->tran_cont, tran_opa, 0);
        }
        break;
    }
    case LV_ALBUM_EFFECT_SCALE_FADE: {
        lv_obj_set_x(alb->main_cont, 0);
        if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
            lv_obj_set_x(alb->tran_cont, 0);
        }
        float curr_scale, other_scale;
        if (offset < 0) { /* 左滑：显示右图 */
            curr_scale = 1.0f + ratio * (alb->effect.params.scale_fade.scale_end / 256.0f - 1.0f);
            other_scale = (alb->effect.params.scale_fade.scale_start / 256.0f) + ratio * (1.0f -
                          alb->effect.params.scale_fade.scale_start / 256.0f);
        } else if (offset > 0) { /* 右滑：显示左图 */
            curr_scale = 1.0f - ratio * (1.0f - alb->effect.params.scale_fade.scale_start / 256.0f);
            other_scale = (alb->effect.params.scale_fade.scale_end / 256.0f) - ratio * (alb->effect.params.scale_fade.scale_end /
                          256.0f - 1.0f);
        } else {
            curr_scale = 1.0f;
            other_scale = 1.0f;
        }

        lv_obj_set_style_transform_pivot_x(alb->main_cont, lv_obj_get_width((lv_obj_t *)alb->main_cont) / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(alb->main_cont, lv_obj_get_height((lv_obj_t *)alb->main_cont) / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_scale(alb->main_cont, (uint16_t)(curr_scale * 256), LV_PART_MAIN);
        uint8_t cur_opa = (uint8_t)(LV_OPA_COVER * (1.0f - ratio));
        lv_obj_set_style_opa(alb->main_cont, cur_opa, LV_PART_MAIN);

        if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
            lv_obj_set_style_transform_pivot_x(alb->tran_cont, lv_obj_get_width((lv_obj_t *)alb->tran_cont) / 2, LV_PART_MAIN);
            lv_obj_set_style_transform_pivot_y(alb->tran_cont, lv_obj_get_height((lv_obj_t *)alb->tran_cont) / 2, LV_PART_MAIN);
            lv_obj_set_style_transform_scale(alb->tran_cont, (uint16_t)(other_scale * 256), LV_PART_MAIN);
            uint8_t other_opa = (uint8_t)(LV_OPA_COVER * ratio);
            lv_obj_set_style_opa(alb->tran_cont, other_opa, LV_PART_MAIN);
        }
        break;
    }
    case LV_ALBUM_EFFECT_EDGE_ROTATE: {
        lv_obj_set_x(alb->main_cont, 0);
        if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
            lv_obj_set_x(alb->tran_cont, 0);
        }

        int16_t max_angle = alb->effect.params.edge_rotate.angle ? alb->effect.params.edge_rotate.angle : 60;

        /* 计算统一 pivot（r_center_x/y） */
        int32_t obj_w = lv_obj_get_width((lv_obj_t *)alb);
        int32_t obj_h = lv_obj_get_height((lv_obj_t *)alb);
        int32_t r_center_x = obj_w / 2;
        int32_t r_center_y;
        if (max_angle == 60) {
            r_center_y = obj_h + (obj_w * lv_trigo_sin(max_angle)) / LV_TRIGO_SIN_MAX;
        } else if (max_angle == 45) {
            r_center_y = obj_h + (obj_w * lv_trigo_sin(max_angle) * 2) / LV_TRIGO_SIN_MAX;
        } else {
            /* 其它角度按 60 度的通用公式处理 */
            r_center_y = obj_h + (obj_w * lv_trigo_sin(max_angle)) / LV_TRIGO_SIN_MAX;
        }

        /* 角度按偏移线性映射 */
        int32_t angle = (int32_t)((int32_t)offset * max_angle / (obj_w == 0 ? 1 : obj_w));

        /* 当前图 */
        lv_obj_set_style_transform_pivot_x(alb->main_cont, r_center_x, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(alb->main_cont, r_center_y, LV_PART_MAIN);
        lv_obj_set_style_transform_rotation(alb->main_cont, angle * 10, LV_PART_MAIN);

        /* 过渡图：根据方向确定相对角度 */
        if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
            lv_obj_set_style_transform_pivot_x(alb->tran_cont, r_center_x, LV_PART_MAIN);
            lv_obj_set_style_transform_pivot_y(alb->tran_cont, r_center_y, LV_PART_MAIN);
            int32_t other_angle = (offset > 0) ? (angle - max_angle) : (angle + max_angle);
            lv_obj_set_style_transform_rotation(alb->tran_cont, other_angle * 10, LV_PART_MAIN);
        }

        break;
    }
    case LV_ALBUM_EFFECT_FLIP_AXIS: {
        /* 中心轴翻转效果（翻书效果） */
        lv_obj_set_x(alb->main_cont, 0);
        if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
            lv_obj_set_x(alb->tran_cont, 0);
        }

        /* 设置中心点为容器中心 */
        int32_t pivot_x = lv_obj_get_width((lv_obj_t *)alb) / 2;
        int32_t pivot_y = lv_obj_get_height((lv_obj_t *)alb) / 2;

        bool show_main = true;
        if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
            show_main = (ratio < 0.5f);
        }

        /* 前半段（ratio < 0.5）：显示和变换 main_cont */
        if (show_main) {
            lv_obj_remove_flag(alb->main_cont, LV_OBJ_FLAG_HIDDEN);
            if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
                lv_obj_add_flag(alb->tran_cont, LV_OBJ_FLAG_HIDDEN);
            }

            float scale_x = 1.0f - 2.0f * ratio;
            if (scale_x < 0.0f) {
                scale_x = 0.0f;    /* 防止负值 */
            }

            lv_matrix_t matrix;
            lv_matrix_identity(&matrix);
            lv_matrix_translate(&matrix, (float)pivot_x, (float)pivot_y);
            lv_matrix_scale(&matrix, scale_x, 1.0f);
            lv_matrix_translate(&matrix, -(float)pivot_x, -(float)pivot_y);
            lv_obj_set_transform(alb->main_cont, &matrix);
        }
        /* 后半段（ratio >= 0.5）：显示和变换 tran_cont */
        else {
            lv_obj_add_flag(alb->main_cont, LV_OBJ_FLAG_HIDDEN);

            if (alb->scroll.tran_index >= 0 && alb->tran_cont) {
                lv_obj_remove_flag(alb->tran_cont, LV_OBJ_FLAG_HIDDEN);

                float scale_x = 2.0f * (ratio - 0.5f);
                if (scale_x < 0.0f) {
                    scale_x = 0.0f;    /* 防止负值 */
                }
                if (scale_x > 1.0f) {
                    scale_x = 1.0f;    /* 防止超过1.0 */
                }

                lv_matrix_t matrix;
                lv_matrix_identity(&matrix);
                lv_matrix_translate(&matrix, (float)pivot_x, (float)pivot_y);
                lv_matrix_scale(&matrix, scale_x, 1.0f);
                lv_matrix_translate(&matrix, -(float)pivot_x, -(float)pivot_y);
                lv_obj_set_transform(alb->tran_cont, &matrix);
            }
        }

        break;
    }
    }
}

static void reset_images_state(lv_album_t *alb)
{
    if (!alb) {
        return;
    }
    if (alb->main_cont) {
        lv_obj_set_pos(alb->main_cont, 0, 0);
        lv_obj_set_style_transform_scale(alb->main_cont, LV_SCALE_NONE, 0);
        lv_obj_set_style_opa(alb->main_cont, LV_OPA_COVER, 0);
        lv_obj_set_style_transform_rotation(alb->main_cont, 0, LV_PART_MAIN);
        lv_obj_reset_transform(alb->main_cont);
        lv_obj_remove_flag(alb->main_cont, LV_OBJ_FLAG_HIDDEN); /* 确保可见 */
    }
    if (alb->tran_cont) {
        lv_obj_set_pos(alb->tran_cont, 0, 0);
        lv_obj_set_style_transform_scale(alb->tran_cont, LV_SCALE_NONE, 0);
        lv_obj_set_style_opa(alb->tran_cont, LV_OPA_COVER, 0);
        lv_obj_set_style_transform_rotation(alb->tran_cont, 0, LV_PART_MAIN);
        lv_obj_reset_transform(alb->tran_cont);
        lv_obj_add_flag(alb->tran_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

static void reset_scroll_state(lv_album_t *alb)
{
    if (!alb) {
        return;
    }
    alb->scroll.animating = false;
    alb->scroll.is_scroll = false;
    alb->scroll.offset = 0;
    alb->scroll.offset_y = 0;
    alb->scroll.target = 0;
    alb->scroll.tran_index = -1;
    alb->scroll.drag_dir = 0;
    alb->scroll.axis_lock = 0;
    alb->scroll.momentum = 0;
    alb->scroll.last_tick = 0;
}

static void start_offset_anim(lv_album_t *alb, int32_t target, bool is_rebound)
{
    if (!alb) {
        return;
    }
    lv_obj_t *obj = (lv_obj_t *)alb;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, alb->scroll.offset, target);
    uint16_t base_time = alb->effect.anim_time ? alb->effect.anim_time : 300;
    uint16_t anim_time = is_rebound ? LV_MAX(60, (uint16_t)(base_time / 3)) : base_time;
    lv_anim_set_duration(&a, anim_time);
    if (alb->effect.path_cb) {
        lv_anim_set_path_cb(&a, alb->effect.path_cb);
    }
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)scroll_anim_exec_cb);
    lv_anim_set_completed_cb(&a, scroll_anim_ready_cb);
    alb->scroll.animating = true;
    lv_anim_start(&a);
}

static void scroll_anim_ready_cb(lv_anim_t *a)
{
    lv_obj_t *obj = a->var;
    lv_album_t *alb = (lv_album_t *)obj;
    int32_t dst = alb->scroll.target;
    bool switched = (dst != 0) && (alb->scroll.tran_index >= 0);
    if (switched) {
        alb->act_idx = alb->scroll.tran_index;
        lv_image_set_src(alb->main_img, alb->srcs[alb->act_idx]);
    }
    reset_scroll_state(alb);
    reset_images_state(alb);
    if (switched) {
        lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    }
    alb->syncing = false;
}

static void scroll_anim_exec_cb(void *var, int32_t val)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_album_t *alb = (lv_album_t *)obj;
    alb->scroll.offset = (int32_t)val;
    update_slide_effect(alb, alb->scroll.offset);
}

#endif /* LV_USE_ALBUM */
