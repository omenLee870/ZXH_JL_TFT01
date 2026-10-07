/**
 * @file lv_jlgif.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_jlgif_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/misc/cache/instance/lv_image_cache.h"

#if LV_USE_JLGIF

/*********************
 *      DEFINES
 *********************/

#define MY_CLASS    &lv_jlgif_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_jlgif_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_jlgif_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void next_frame_task_cb(lv_timer_t * t);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_jlgif_class = {
    .constructor_cb = lv_jlgif_constructor,
    .destructor_cb = lv_jlgif_destructor,
    .instance_size = sizeof(lv_jlgif_t),
    .base_class = &lv_image_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_jlgif_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_jlgif_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_jlgif_set_src(lv_obj_t * obj, const void * src)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_jlgif_t * jlgif = (lv_jlgif_t *)obj;

    /*Close previous gif if any*/
    if(jlgif->gif) {
        lv_image_cache_drop(&jlgif->imgdsc);
        jl_close_gif(jlgif->gif);
        jlgif->gif = NULL;
        jlgif->imgdsc.data = NULL;
    }

    if(lv_image_src_get_type(src) == LV_IMAGE_SRC_BIN) {
        jlgif->gif = jl_open_gif_file(src);
    }
    else if(lv_image_src_get_type(src) == LV_IMAGE_SRC_VARIABLE) {
        const lv_image_dsc_t * img_dsc = src;
        jlgif->gif = jl_open_gif_data(img_dsc->data, img_dsc->data_size);
    }
    else {
        LV_LOG_WARN("Could't load the source");
        return;
    }

    if(jlgif->gif == NULL) {
        LV_LOG_WARN("Failed to open JLGIF");
        return;
    }

    jlgif->imgdsc.header.cf = LV_COLOR_FORMAT_I8;
    jlgif->imgdsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    jlgif->imgdsc.header.reserved_2 = 0;
    jlgif->imgdsc.header.stride = jlgif->gif->header.width;
    jlgif->imgdsc.header.w = jlgif->gif->header.width;
    jlgif->imgdsc.header.h = jlgif->gif->header.height;
    jlgif->imgdsc.data_size = jlgif->gif->header.width * jlgif->gif->header.height + 256 * 4;
    jlgif->imgdsc.data = jlgif->gif->canvas;
    jlgif->last_frame_time = lv_tick_get();

    lv_image_set_src(obj, &jlgif->imgdsc);

    lv_timer_resume(jlgif->timer);
    lv_timer_reset(jlgif->timer);

    next_frame_task_cb(jlgif->timer);
}

void lv_jlgif_restart(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_jlgif_t * jlgif = (lv_jlgif_t *)obj;

    if(jlgif->gif) {
        jl_rewind(jlgif->gif);
        jl_get_frame(jlgif->gif);
        lv_obj_invalidate(obj);

        lv_timer_resume(jlgif->timer);
        lv_timer_reset(jlgif->timer);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_jlgif_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_jlgif_t * jlgif = (lv_jlgif_t *)obj;

    jlgif->gif = NULL;
    jlgif->timer = lv_timer_create(next_frame_task_cb, 10, obj);
    lv_timer_pause(jlgif->timer);
    lv_memzero(&jlgif->imgdsc, sizeof(lv_image_dsc_t));
    jlgif->last_frame_time = 0;
}

static void lv_jlgif_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_jlgif_t * jlgif = (lv_jlgif_t *)obj;
    lv_image_cache_drop(&jlgif->imgdsc);
    if(jlgif->gif) {
        jl_close_gif(jlgif->gif);
        jlgif->gif = NULL;
    }
    lv_timer_delete(jlgif->timer);
}

static void next_frame_task_cb(lv_timer_t * t)
{
    lv_obj_t * obj = (lv_obj_t *)lv_timer_get_user_data(t);
    lv_jlgif_t * jlgif = (lv_jlgif_t *)obj;

    if(!jlgif->gif) return;

    uint32_t elaps = lv_tick_elaps(jlgif->last_frame_time);
    uint32_t frame_delay = (jlgif->gif->frames[jlgif->gif->curr_frame].delay > 0) ?
                           jlgif->gif->frames[jlgif->gif->curr_frame].delay :
                           jlgif->gif->header.delay;

    if(elaps < frame_delay) return;

    /* 获取下一帧 */
    int ret = jl_get_frame(jlgif->gif);

    if(ret == 0) {
        /* GIF 播放结束，发送 READY 事件，然后重新开始播放 */
        lv_result_t res = lv_obj_send_event(obj, LV_EVENT_READY, NULL);

        jl_rewind(jlgif->gif);
        jl_get_frame(jlgif->gif);
    }

    lv_obj_invalidate(obj);
    jlgif->last_frame_time = lv_tick_get();
}

#endif /* LV_USE_JLGIF */
