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
    /* 可选后置钩子: 在 manager 重放完所有槽(全局矩阵/窗口 SFR 已落到最后一槽)之后、
     * 该 busy 帧结束前被调用。用于在已绘画面上再叠一层合成(如 LRB 倒影纵向渐隐)。
     * cube/hexagon 不实现则保持 NULL, 行为与原来完全一致。 */
    void (*overlay)(lv_draw_jlgpu_unit_t *unit, uint8_t *px_map, const lv_area_t *screen_window);
} sticky_effect_ops_t;

#endif
