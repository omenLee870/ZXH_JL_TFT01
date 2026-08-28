/**
 * @file lv_imagemask.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_imagemask_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/widgets/canvas/lv_canvas.h"
#include "../../../../../src/draw/lv_draw.h"
#include "../../../../../src/draw/lv_draw_buf.h"

#if LV_USE_IMAGEMASK != 0

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_imagemask: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_imagemask_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_imagemask_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_imagemask_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_imagemask_event(const lv_obj_class_t *class_p, lv_event_t *e);
static lv_image_dsc_t *create_processed_mask(lv_imagemask_t *m, const void *src, int32_t target_w, int32_t target_h);
static void update_mask(lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_imagemask_class = {
    .base_class = &lv_image_class,
    .constructor_cb = lv_imagemask_constructor,
    .destructor_cb = lv_imagemask_destructor,
    .event_cb = lv_imagemask_event,
    .instance_size = sizeof(lv_imagemask_t),
    .width_def = 100,
    .height_def = 100,
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_imagemask_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

void lv_imagemask_set_src(lv_obj_t *obj, const void *src)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_image_set_src(obj, src);
}

void lv_imagemask_set_mask(lv_obj_t *obj, const void *mask_src)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;

    // 释放之前的拥有的遮罩
    if (m->mask_dsc) {
        if (m->mask_dsc->data) {
            lv_free((void *)m->mask_dsc->data);
        }
        lv_free(m->mask_dsc);
        m->mask_dsc = NULL;
    }
    if (m->mask_type == LV_IMAGE_SRC_FILE || m->mask_type == LV_IMAGE_SRC_BIN) {
        lv_free((void *)m->mask);
        m->mask = NULL;
        m->mask_type = LV_IMAGE_SRC_UNKNOWN;
    }

    lv_image_src_t mask_type = lv_image_src_get_type(mask_src);
    if (mask_type == LV_IMAGE_SRC_VARIABLE) {
        m->mask = mask_src;
        m->mask_type = LV_IMAGE_SRC_VARIABLE;
    } else if (mask_type == LV_IMAGE_SRC_FILE || mask_type == LV_IMAGE_SRC_BIN) {
        m->mask = lv_malloc(lv_strlen(mask_src) + 1);
        lv_strcpy((char *)m->mask, mask_src);
        m->mask_type = LV_IMAGE_SRC_FILE;
    } else {
        m->mask = NULL;
        m->mask_type = LV_IMAGE_SRC_UNKNOWN;
    }

    update_mask(obj);
}

void lv_imagemask_set_mask_scale(lv_obj_t *obj, int32_t scale)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    m->mask_scale = scale;
    update_mask(obj);
}

void lv_imagemask_set_mask_rotation(lv_obj_t *obj, int32_t rotation)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    m->mask_rotation = rotation;
    update_mask(obj);
}

void lv_imagemask_set_mask_offset_x(lv_obj_t *obj, int32_t offset_x)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    m->mask_offset_x = offset_x;
    update_mask(obj);
}

void lv_imagemask_set_mask_offset_y(lv_obj_t *obj, int32_t offset_y)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    m->mask_offset_y = offset_y;
    update_mask(obj);
}

void lv_imagemask_set_mask_pivot(lv_obj_t *obj, int32_t x, int32_t y)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    lv_point_set(&m->mask_pivot, x, y);
    update_mask(obj);
}

/**********************
 *   GETTER FUNCTIONS
 **********************/

const void *lv_imagemask_get_mask(const lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    return m->mask;
}

int32_t lv_imagemask_get_mask_scale(const lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    return m->mask_scale;
}

int32_t lv_imagemask_get_mask_rotation(const lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    return m->mask_rotation;
}

int32_t lv_imagemask_get_mask_offset_x(const lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    return m->mask_offset_x;
}

int32_t lv_imagemask_get_mask_offset_y(const lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    return m->mask_offset_y;
}

void lv_imagemask_get_mask_pivot(const lv_obj_t *obj, lv_point_t *pivot)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    *pivot = m->mask_pivot;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_imagemask_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    m->mask_dsc = NULL;
    m->mask = NULL;
    m->mask_type = LV_IMAGE_SRC_UNKNOWN;
    m->mask_scale = LV_SCALE_NONE;
    m->mask_rotation = 0;
    m->mask_offset_x = 0;
    m->mask_offset_y = 0;
    lv_point_set(&m->mask_pivot, 0, 0);
}

static void lv_imagemask_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    // 释放拥有的资源
    if (m->mask_dsc) {
        if (m->mask_dsc->data) {
            lv_free((void *)m->mask_dsc->data);
        }
        lv_free(m->mask_dsc);
        m->mask_dsc = NULL;
    }
    if (m->mask_type == LV_IMAGE_SRC_FILE) {
        // if(imgmask->mask_type == LV_IMG_SRC_FILE || imgmask->mask_type == LV_IMG_SRC_BIN) {
        lv_free((void *)m->mask);
        m->mask = NULL;
        m->mask_type = LV_IMAGE_SRC_UNKNOWN;
    }

}

static void lv_imagemask_event(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);

    if (lv_obj_event_base(MY_CLASS, e) != LV_RESULT_OK) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_SIZE_CHANGED) {
        update_mask(obj);
    }
}

// 创建一个A8遮罩，通过将src渲染到ARGB8888的canvas上，并提取alpha通道
static lv_image_dsc_t *create_processed_mask(lv_imagemask_t *m, const void *src, int32_t target_w, int32_t target_h)
{
    if (src == NULL || target_w <= 0 || target_h <= 0) {
        return NULL;
    }

    lv_obj_t *temp_canvas = lv_canvas_create(lv_screen_active());
    if (temp_canvas == NULL) {
        return NULL;
    }

    uint32_t temp_buf_size = (uint32_t)target_w * (uint32_t)target_h * 4u;
    uint8_t *temp_buf = lv_malloc(temp_buf_size);
    if (!temp_buf) {
        lv_obj_delete(temp_canvas);
        return NULL;
    }

    lv_canvas_set_buffer(temp_canvas, temp_buf, target_w, target_h, LV_COLOR_FORMAT_ARGB8888);
    lv_canvas_fill_bg(temp_canvas, lv_color_black(), LV_OPA_TRANSP);

    // 将源图像绘制到canvas上
    lv_layer_t layer;
    lv_canvas_init_layer(temp_canvas, &layer);

    lv_draw_image_dsc_t draw_dsc;
    lv_draw_image_dsc_init(&draw_dsc);
    draw_dsc.src = src;
    draw_dsc.scale_x = m->mask_scale;
    draw_dsc.scale_y = m->mask_scale;
    draw_dsc.rotation = m->mask_rotation;
    draw_dsc.pivot = m->mask_pivot;

    lv_area_t coords;
    lv_area_set(&coords, m->mask_offset_x, m->mask_offset_y, target_w - 1, target_h - 1);
    lv_draw_image(&layer, &draw_dsc, &coords);
    lv_canvas_finish_layer(temp_canvas, &layer);

    uint32_t stride = lv_draw_buf_width_to_stride((uint32_t)target_w, LV_COLOR_FORMAT_A8);
    uint32_t mask_buf_size = stride * (uint32_t)target_h;
    uint8_t *mask_buf = lv_malloc(mask_buf_size);
    if (!mask_buf) {
        lv_free(temp_buf);
        lv_obj_delete(temp_canvas);
        return NULL;
    }

    // 提取alpha通道到A8
    const uint32_t *argb_data = (const uint32_t *)temp_buf;
    uint32_t px_cnt = (uint32_t)target_w * (uint32_t)target_h;
    lv_memzero(mask_buf, mask_buf_size);
    if (stride == (uint32_t)target_w) {
        for (uint32_t i = 0; i < px_cnt; i++) {
            uint8_t alpha = (uint8_t)((argb_data[i] >> 24) & 0xFF);
            mask_buf[i] = alpha;
        }
    } else {
        for (int32_t y = 0; y < target_h; y++) {
            uint8_t *dst_row = mask_buf + (uint32_t)y * stride;
            const uint32_t *src_row = argb_data + (uint32_t)y * (uint32_t)target_w;
            for (int32_t x = 0; x < target_w; x++) {
                dst_row[x] = (uint8_t)((src_row[x] >> 24) & 0xFF);
            }
        }
    }

    lv_image_dsc_t *mask_dsc = lv_malloc(sizeof(lv_image_dsc_t));
    if (!mask_dsc) {
        lv_free(temp_buf);
        lv_free(mask_buf);
        lv_obj_delete(temp_canvas);
        return NULL;
    }


    lv_memzero(mask_dsc, sizeof(lv_image_dsc_t));
    mask_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    mask_dsc->header.cf = LV_COLOR_FORMAT_A8;
    mask_dsc->header.flags = 0;
    mask_dsc->header.w = (uint16_t)target_w;
    mask_dsc->header.h = (uint16_t)target_h;
    mask_dsc->header.stride = (uint16_t)stride;
    mask_dsc->header.reserved_2 = 0;
    mask_dsc->data = mask_buf;
    mask_dsc->data_size = mask_buf_size;

    lv_free(temp_buf);
    lv_obj_delete(temp_canvas);

    return mask_dsc;
}

static void update_mask(lv_obj_t *obj)
{
    int32_t w = lv_obj_get_width(obj);
    int32_t h = lv_obj_get_height(obj);
    if (w <= 0 || h <= 0) {
        return;
    }

    lv_imagemask_t *m = (lv_imagemask_t *)obj;
    lv_image_dsc_t *old = m->mask_dsc;
    lv_image_dsc_t *mask_dsc = create_processed_mask(m, m->mask, w, h);
    if (mask_dsc) {
        m->mask_dsc = mask_dsc;
        lv_obj_set_style_bitmap_mask_src(obj, mask_dsc, 0);
        if (old) {
            if (old->data) {
                lv_free((void *)old->data);
            }
            lv_free(old);
        }
    } else {
        lv_obj_set_style_bitmap_mask_src(obj, NULL, 0);
        if (old) {
            if (old->data) {
                lv_free((void *)old->data);
            }
            lv_free(old);
            m->mask_dsc = NULL;
        }
    }
}

#endif /*LV_USE_IMAGEMASK*/
