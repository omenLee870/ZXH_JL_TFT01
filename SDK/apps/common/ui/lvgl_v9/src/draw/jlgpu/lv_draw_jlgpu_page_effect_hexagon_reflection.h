#ifndef LV_DRAW_JLGPU_PAGE_EFFECT_HEXAGON_REFLECTION_H
#define LV_DRAW_JLGPU_PAGE_EFFECT_HEXAGON_REFLECTION_H

#include "lv_draw_jlgpu_page_effect_ops.h"

void lv_hexagon_reflection_effect_init(lv_draw_jlgpu_unit_t *unit, bool list_flag);

int lv_ui_hexagon_reflection_move(lv_draw_jlgpu_unit_t *unit, int xoffset, int yoffset);

void lv_hexagon_reflection_effect_set_page_index(uint32_t page_id, uint32_t page_index);

void lv_hexagon_reflection_effect_get_page_id(uint32_t *page_id_arr);

void lv_hexagon_reflection_effect_uninit(void);

int lv_hexagon_reflection_effect_get_face_scr_id(int x, int y);

const sticky_effect_ops_t *lv_hexagon_reflection_effect_get_ops(void);

#endif
