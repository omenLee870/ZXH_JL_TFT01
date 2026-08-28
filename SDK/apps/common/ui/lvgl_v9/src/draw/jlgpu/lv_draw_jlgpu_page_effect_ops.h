#ifndef LV_DRAW_JLGPU_PAGE_EFFECT_OPS_H
#define LV_DRAW_JLGPU_PAGE_EFFECT_OPS_H

#include "lv_draw_jlgpu.h"

/** Ops table for sticky 3D page effects.
 *  Each effect type (cube, hexagon, ...) provides a static instance.
 *  Extend this struct when adding new sticky effects. */
typedef struct sticky_effect_ops {
    void (*init)(lv_draw_jlgpu_unit_t *unit, bool list_flag);
    void (*uninit)(void);
    int (*move)(lv_draw_jlgpu_unit_t *unit, int xoffset, int yoffset);
    int (*get_face_scr_id)(int x, int y);
    int (*get_face_index)(int x, int y);
    void (*get_page_id)(uint32_t *page_id_arr);
} sticky_effect_ops_t;

#endif
