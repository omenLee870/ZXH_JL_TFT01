#ifndef LV_ANIM_EFFECT_H
#define LV_ANIM_EFFECT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * 试用效果动画需要开启 配置lv_conf.h：
 *      #define CONFIG_LV_COLOR_SCREEN_TRANSP = 1
 * 否则，旋转和缩放都会显示异常；
 */
#if LV_USE_ANIM_EFFECT
#include "lv_anim_effect_global.h"
#include "lv_anim_effect_attention.h"
#include "lv_anim_effect_bounce.h"
#include "lv_anim_effect_fade.h"
#include "lv_anim_effect_back.h"
#include "lv_anim_effect_sliding.h"
#include "lv_anim_effect_rotate.h"
#include "lv_anim_effect_zooming.h"
#include "lv_anim_effect_specials.h"
#endif /*LV_USE_ANIM_EFFECT*/
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_ANIM_EFFECT_H*/
