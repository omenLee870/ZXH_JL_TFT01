#ifndef LV_HEXAGON_REFLECTION_H
#define LV_HEXAGON_REFLECTION_H

#include "lv_draw_jlgpu.h"

#define REF_POLYTOPE_SIX    6   //六面体
#define REF_POLYTOPE_EIGHT  8   //八面体
#define REF_POLYTOPE_ALL    255 //共存
#define REF_POLYTOPE_CONFIG    REF_POLYTOPE_SIX
extern const u8 ref_polytope;

int lv_hexagon_reflection_init(int width[16], int height[16], float w_scale, float h_scale, float view_distance, int win_x, int win_y, int win_w, int win_h, float h_ofs_range);
void lv_hexagon_reflection_uninit(void);
int lv_hexagon_reflection_draw(lv_draw_jlgpu_unit_t *unit, float x_update_val, float y_update_val, float x_angle, float h_ofs_val, u8 *reset_sep, u8 is_draw);
void lv_hexagon_reflection_polytope_type_set(u8 type);
void lv_hexagon_reflection_set_interval(float interval);
int lv_hexagon_reflection_get_face_index(int x, int y);

#endif
