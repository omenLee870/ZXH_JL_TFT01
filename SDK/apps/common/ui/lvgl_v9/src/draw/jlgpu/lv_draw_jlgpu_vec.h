#ifndef LV_DRAW_JLGPU_VEC_H
#define LV_DRAW_JLGPU_VEC_H

#include <stdint.h>
#include <math.h>
#include "jlgpu_framework.h"

#define JLGPU_FLOAT32_MIN (-3.402823466e+38f)
#define JLGPU_FLOAT32_MAX (3.402823466e+38f)

#define JLVEC_MAX(a,b) ((a>b)?a:b)
#define JLVEC_MIN(a,b) ((a<b)?a:b)

typedef struct {
    float x_min;
    float x_max;
    float y_min;
    float y_max;
} jlvec_bbox_t;


typedef struct {
    uint8_t *base;
    uint8_t *cur_seg;
    int16_t *cur_coord;
    int16_t total_len;
    int16_t nsegments;
    uint8_t bez_div;
    int32_t edge_num;
    jlvec_bbox_t path_bbox;

    uint32_t capacity;//总的buf的容量
} jlgpu_vec_t;

typedef struct jlgpu_point {
    int x;
    int y;
} jlgpu_point_t;



#include "lv_draw_jlgpu.h"

/*#define FLT_EPSILON      1.192092896e-07F*/

static inline uint8_t math_zero(float a)
{
    return (fabs(a) < FLT_EPSILON);
}

static inline uint8_t math_equal(float a, float b)
{
    return math_zero(a - b);
}

int jlgpu_compute_div(jlgpu_area_t bbox);

void jlgpu_path_init(jlgpu_vec_t *path, uint8_t *base, uint8_t bez_div);
void jlgpu_path_deinit(jlgpu_vec_t *path);

void jlgpu_path_end(jlgpu_vec_t *path);

void jlgpu_path_append_rect(
    jlgpu_vec_t *path,
    float x, float y,
    float w, float h,
    float r);

void jlgpu_path_reset(jlgpu_vec_t *path);


jlgpu_vec_t *jlgpu_path_create(void);
void jlgpu_path_destroy(jlgpu_vec_t *path);
void lv_jlgpu_path_reset(jlgpu_vec_t *path);
void lv_jlgpu_path_set_bez_div(jlgpu_vec_t *path, uint8_t bez_div);
/*jlgpu_vec_t *lv_jlgpu_path_get(lv_draw_jlgpu_unit_t *unit);*/

void jlgpu_path_move_to(jlgpu_vec_t *path, float x, float y);
void jlgpu_path_line_to(jlgpu_vec_t *path, float x, float y);
void jlgpu_path_quad_to(jlgpu_vec_t *path, float cx, float cy, float x, float y);
void jlgpu_path_cubic_to(jlgpu_vec_t *path, float cx1, float cy1, float cx2, float cy2, float x, float y);


void jlgpu_path_append_arc_right_angle(jlgpu_vec_t *path,
                                       float start_x, float start_y,
                                       float center_x, float center_y,
                                       float end_x, float end_y);
void jlgpu_path_append_arc(jlgpu_vec_t *path,
                           float cx, float cy,
                           float radius,
                           float start_angle,
                           float sweep,
                           uint8_t pie);
void jlgpu_path_append_circle(
    jlgpu_vec_t *path,
    float cx, float cy,
    float rx, float ry);

jlgpu_area_t jlgpu_compute_bbox(jlgpu_matrix_t *matrix, jlvec_bbox_t *bounding_box, int width, int height);


//test
void jlgpu_get_flipx_matrix(jlgpu_matrix_t *matrix, float rot);
void jlgpu_get_flipy_matrix(jlgpu_matrix_t *matrix, float rot);
void jlgpu_matrix_transform_point(const jlgpu_matrix_t *matrix, float x, float y, float *out_x, float *out_y);
void jlgpu_matrix_set_projection(jlgpu_matrix_t *matrix, float g, float h, float i);
#endif
