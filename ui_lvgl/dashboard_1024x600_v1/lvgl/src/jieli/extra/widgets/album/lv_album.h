/**
 * @file lv_album.h
 *
 */
#ifndef LV_ALBUM_H
#define LV_ALBUM_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_ALBUM != 0
#include "../../../../../src/core/lv_obj.h"

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_album: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/* 效果类型 */
typedef enum {
    LV_ALBUM_EFFECT_SLIDE = 0,   /* 滑动 */
    LV_ALBUM_EFFECT_SLIDE_SCALE, /* 滑动 + 缩放 */
    LV_ALBUM_EFFECT_SCALE_FADE,  /* 缩放 + 透明 */
    LV_ALBUM_EFFECT_EDGE_ROTATE, /* 边缘旋转 */
    LV_ALBUM_EFFECT_FLIP_AXIS,   /* 中心轴翻转 */
} lv_album_effect_t;

typedef void (*lv_album_load_more_cb_t)(lv_obj_t * album);

/* 自定义变换回调 */
typedef void (*lv_album_transform_cb_t)(
    lv_obj_t * album,
    lv_obj_t * main_cont,
    lv_obj_t * tran_cont,     /* 可能为 NULL（无过渡图） */
    int32_t offset,        /* 当前拖拽偏移（>0 右滑，<0 左滑） */
    int32_t width,         /* 相册宽度 */
    float ratio,              /* 归一化比例 [0,1] */
    int8_t dir_sign           /* 方向标志：-1 左滑，1 右滑，0 无 */
);

/* 效果配置 */
typedef struct {
    lv_album_effect_t type;      /* 效果类型 默认LV_ALBUM_EFFECT_SLIDE */
    uint16_t anim_time;          /* ms 默认400ms */
    lv_anim_path_cb_t path_cb;   /* 动画路径 默认lv_anim_path_ease_out */
    union {
        /* 滑动 + 缩放 + 交叉透明 */
        struct {
            uint16_t scale_start;         /* 默认180=0.7x */
            uint16_t scale_end;           /* 默认256=1.0x */
            uint8_t  fade_start;          /* 默认50 */
            uint8_t  fade_end;            /* 默认255 */
        } slide_scale;

        /* 中心缩放 + 交叉透明 */
        struct {
            uint16_t scale_start;         /* 默认51=0.2x*/
            uint16_t scale_end;           /* 默认512=2.0x*/
        } scale_fade;

        /* 边缘旋转 */
        struct {
            int16_t  angle;     /* 默认60度 */
        } edge_rotate;
    } params;
} lv_album_effect_cfg_t;

extern const lv_obj_class_t lv_album_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*
 * Create a album object
 * @param parent pointer to the parent object
 * @return lv_obj_t* pointer to the created album object
 */
lv_obj_t * lv_album_create(lv_obj_t * parent);

/*
 * Add a picture to the album
 * @param album pointer to the album object
 * @param src pointer to the image source
 * @return lv_obj_t* pointer to the created picture object
 */
void lv_album_add_pic(lv_obj_t * album, const void * src);

/*
 * Remove a picture from the album by index
 * @param album pointer to the album object
 * @param index index of the picture to remove
 */
void lv_album_remove_pic(lv_obj_t * album, uint16_t index);

/*
 * Clear all pictures from the album
 * @param album pointer to the album object
 */
void lv_album_remove_all_pic(lv_obj_t * album);

/*=====================
 * Setter functions
 *====================*/

/*
 * Set the active picture index
 * @param album pointer to the album object
 * @param act_idx active picture index
 */
void lv_album_set_act(lv_obj_t * album, int16_t act_idx);

/*
 * Set/switch effect configuration
 * @param album pointer to the album object
 * @param type effect type
 * @param config effect configuration, can be NULL to use default configuration
 */
void lv_album_set_effect(lv_obj_t * album, lv_album_effect_t type, const lv_album_effect_cfg_t * config);

/*
 * Bind a custom transform callback (non-NULL takes precedence over callback for position/size transformation)
 * @param album pointer to the album object
 * @param cb custom transform callback
 */
void lv_album_set_transform_cb(lv_obj_t * album, lv_album_transform_cb_t cb);

/*
 * Set the load more callback and enable auto load more
 * @param album pointer to the album object
 * @param cb load more callback
 */
void lv_album_set_load_more_cb(lv_obj_t * album, lv_album_load_more_cb_t cb);

/*
 * Set the auto load more
 * @param album pointer to the album object
 * @param enable true to enable auto load more
 */
void lv_album_set_auto_load(lv_obj_t * album, bool enable);

/*=====================
 * Getter functions
 *====================*/

/*
 * Get the current picture count and index
 * @param album pointer to the album object
 * @return uint16_t current picture count
 * @return int16_t current picture index
 */
uint16_t lv_album_get_count(lv_obj_t * album);

/*
 * Get the active picture index
 * @param album pointer to the album object
 * @return int16_t active picture index
 */
int16_t lv_album_get_act(lv_obj_t * album);

/*
 * Get the active picture source
 * @param album pointer to the album object
 * @return char * active picture source
 */
const void * lv_album_get_act_pic(lv_obj_t * album);


#endif /* LV_USE_ALBUM */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_ALBUM_H */
