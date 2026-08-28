/**
 * @file jl_etc2.h
 *
 */

#ifndef JL_ETC2_H
#define JL_ETC2_H


#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"

typedef struct c2_info {
    uint32_t width;//原始宽高
    uint32_t height;
    uint16_t format;
// 0 : ARGB8888, 图像格式
// 1 : ARGB8565
// 2 : ARGB1555
// 3 : ARGB4444
// 4 : RGB888
// 5 : RGB565
// 6 : BT601
// 7 : BT709
// 8 : AL88
// 16 : AL44
// 17 : AL22
// 18 : L8
// 19 : L4
// 20 : L2
// 21 : L1
// 22 : A8
// 23 : A4
// 24 : A2
// 25 : A1
    uint32_t img_width; //扩充后的宽高
    uint32_t img_height;
} c2_info_t;


#if LV_USE_JL_ETC2

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/


/**********************
 *      MACROS
 **********************/

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
