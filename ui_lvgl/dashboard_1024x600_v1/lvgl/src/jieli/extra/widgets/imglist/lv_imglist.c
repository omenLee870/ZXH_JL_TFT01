/**
 * @file lv_imglist.c
 * 
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_imglist_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#if LV_USE_IMGLIST != 0

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
    #error "lv_imglist: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE  1) "
#endif

/*********************
 *      DEFINES
 *********************/

#define MY_CLASS &lv_imglist_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_imglist_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_imglist_class = {
    .constructor_cb = lv_imglist_constructor,
    .instance_size = sizeof(lv_imglist_t),
    .base_class = &lv_image_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_imglist_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_imglist_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_imglist_set_src(lv_obj_t * obj, const void * dsc[], uint8_t num)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imglist_t * imglist = (lv_imglist_t *)obj;
    imglist->dsc = dsc;
    imglist->pic_count = num;
    imglist->act_idx = 0;
    if (num > 0) {
        lv_image_set_src(obj, dsc[0]);
    }
}

void lv_imglist_set_act(lv_obj_t * obj, uint32_t act_idx)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_imglist_t * imglist = (lv_imglist_t *)obj;
    if (imglist->act_idx == act_idx) return;

    imglist->act_idx = act_idx < imglist->pic_count ? act_idx : imglist->pic_count - 1;
    if (imglist->dsc) {
        lv_image_set_src(obj, imglist->dsc[imglist->act_idx]);
    }

    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

uint32_t lv_imglist_get_act(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_imglist_t * imglist = (lv_imglist_t *)obj;
    return imglist->act_idx;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_imglist_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");

    LV_UNUSED(class_p);
    lv_imglist_t * imglist = (lv_imglist_t *)obj;

    imglist->dsc = NULL;
    imglist->pic_count = -1;
    imglist->act_idx = 0;
}

#endif