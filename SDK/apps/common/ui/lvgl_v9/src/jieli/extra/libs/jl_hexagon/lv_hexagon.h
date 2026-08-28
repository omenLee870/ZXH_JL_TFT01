#ifndef LV_HEXAGON_H
#define LV_HEXAGON_H

#include "lv_draw_jlgpu.h"

#define POLYTOPE_SIX    6   //六面体
#define POLYTOPE_EIGHT  8   //八面体
#define POLYTOPE_ALL    255 //共存
#define POLYTOPE_CONFIG    POLYTOPE_SIX
extern const u8 polytope;

int lv_hexagon_init(int width[8], int height[8], float w_scale, float h_scale, float view_distance, int win_x, int win_y, int win_w, int win_h, float h_ofs_range);
void lv_hexagon_uninit(void);
int lv_hexagon_draw(lv_draw_jlgpu_unit_t *unit, float x_update_val, float y_update_val, float x_angle, float h_ofs_val, u8 *reset_sep, u8 is_draw);
void lv_hexagon_polytope_type_set(u8 type);
int lv_hexagon_get_face_index(int x, int y);

//设置多面体组成页面
void lv_hexagon_effect_set_page_index(uint32_t page_id, uint32_t page_index);

#endif
