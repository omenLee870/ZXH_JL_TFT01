#ifndef __JLGPU_MATH_H__
#define __JLGPU_MATH_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "rect.h"
#include "asm/cpu.h"
#include "jlgpu_def.h"


#define GPU_MATH_CODE		AT(.math_api.text.cache.L2)

typedef struct {
    float m[3][3];
} jlgpu_matrix_t;

typedef struct {
    float x;
    float y;
} jlgpu_point2_t;

typedef struct {
    float x;
    float y;
    float z;
} jlgpu_point3_t;

typedef struct {
    int minx;
    int maxx;
    int miny;
    int maxy;
} jlgpu_boundbox_t;
#define bbox_min_x(bbox)	(bbox)->minx
#define bbox_max_x(bbox)	(bbox)->maxx
#define bbox_min_y(bbox)	(bbox)->miny
#define bbox_max_y(bbox)	(bbox)->maxy
#define bbox_width(bbox)	(bbox_max_x(bbox) - bbox_min_x(bbox))
#define bbox_height(bbox)	(bbox_max_y(bbox) - bbox_min_y(bbox))


float jlgpu_sinf(float x);
float jlgpu_cosf(float x);
jlgpu_matrix_t *gpu_matrix_create();
void jlgpu_matrix_free_(jlgpu_matrix_t *m);
void jlgpu_matrix_set_identity(jlgpu_matrix_t *m);
void jlgpu_matrix_set_translate(jlgpu_matrix_t *m, float tx, float ty);
void jlgpu_matrix_set_scale(jlgpu_matrix_t *m, float sx, float sy);
void jlgpu_matrix_set_shear(jlgpu_matrix_t *m, float shx, float shy);
void jlgpu_matrix_set_rotate(jlgpu_matrix_t *m, float a);
void jlgpu_matrix_set_zero(jlgpu_matrix_t *m);
void jlgpu_matrix_multiply(jlgpu_matrix_t *d, jlgpu_matrix_t *s0, jlgpu_matrix_t *s1);
void jlgpu_matrix_mul_const(jlgpu_matrix_t *m, float f);
int jlgpu_matrix_is_affine(jlgpu_matrix_t *m);
int jlgpu_matrix_is_identity(jlgpu_matrix_t *m);
int jlgpu_matrix_is_zero(jlgpu_matrix_t *m);
int jlgpu_matrix_invert(jlgpu_matrix_t *dm, jlgpu_matrix_t *m);


void jlgpu_matrix_translate(jlgpu_matrix_t *m, float tx, float ty);
void jlgpu_matrix_scale(jlgpu_matrix_t *m, float sx, float sy);
void jlgpu_matrix_shear(jlgpu_matrix_t *m, float shx, float shy);
void jlgpu_matrix_rotate(jlgpu_matrix_t *m, float a);

void jlgpu_matrix_rotate_scale(jlgpu_matrix_t *m, float cx, float cy, float angle, float sx, float sy);
void jlgpu_matrix_scale_rotate(jlgpu_matrix_t *m, float cx, float cy, float angle, float sx, float sy);
void jlgpu_matrix_rotate_center(jlgpu_matrix_t *m, float cx, float cy, float angle);

void jlgpu_transform_point2(jlgpu_point2_t *dp, jlgpu_matrix_t *m, jlgpu_point2_t *sp);

void jlgpu_point2_add(jlgpu_point2_t *d, jlgpu_point2_t *s0, jlgpu_point2_t *s1);
void jlgpu_point2_sub(jlgpu_point2_t *d, jlgpu_point2_t *s0, jlgpu_point2_t *s1);
void jlgpu_point2_mul(jlgpu_point2_t *d, jlgpu_point2_t *s0, jlgpu_point2_t *s1);
float jlgpu_point2_len(jlgpu_point2_t *s0, jlgpu_point2_t *s1);

void jlgpu_point_transform(jlgpu_matrix_t *matrix, float x_in, float y_in, float *x_out, float *y_out);
void jlgpu_matrix_set_flip(jlgpu_matrix_t *m, int x_en, int y_en);
void jlgpu_matrix_flip(jlgpu_matrix_t *m, int x_en, int y_en);
void jlgpu_matrix_get_boundbox(jlgpu_matrix_t *matrix, jlgpu_boundbox_t *src, jlgpu_boundbox_t *dst);

int jlgpu_matrix_calc_by_point(jlgpu_matrix_t *matrix, int x, int y, int pt_w, int pt_h, float points[8]);

// =====>>> 3D变换矩阵
void jlgpu_matrix_3d_init(float *M);
void jlgpu_matrix_3d_scale(float *out, float x_scale, float y_scale, float z_scale);
void jlgpu_matrix_3d_rotate(float *out, float x_rot, float y_rot, float z_rot);
int  jlgpu_matrix_3d_invert(float *in, float *out);
void jlgpu_matrix_3d_to_2d(float *m, float *in, float *out, int n);
void jlgpu_matrix_3d_34_mul_33(float *out, float *mat_34, float *mat_33);
void jlgpu_matrix_3d_33_mul_33(float *out, float *mat0, float *mat1);
int  jlgpu_matrix_3d_get_facing(float *p0, float *p1, float *p2);

void jlgpu_matrix_3d_rotate_dir(float *out, float *in, float x, float y, float z, float angle);
void jlgpu_matrix_3d_camera(float *out, float x, float y, float z, float width, float height, float fov_x, float fov_y);


// =====>>> 3D变换模型
typedef struct {
    char name[8];
    uint32_t version;
    uint32_t npoint;
    uint32_t nface;
    uint32_t npoint_per_face;
    float *points;
    uint16_t *faces;
    float *face_matrix;
} jlgpu_3Dmodel_t;

int  jlgpu_matrix_3d_get_face(jlgpu_3Dmodel_t *model, float *pts2d, int x, int y);
void jlgpu_matrix_3d_get_bbox(jlgpu_boundbox_t *bbox, float *pts2d, uint16_t *idxs, int n);
void jlgpu_compute_npoint_center(float *pts2d, uint16_t *idxs, int n, struct position *center);
void jlgpu_compute_touch_distance(float *distance, float *pts2d, uint16_t *idxs, int n,  int x, int y);




void jlgpu_matrix_dump(jlgpu_matrix_t *matrix, const char *name);

#define __DUMP_MATRIX(m) \
{ \
	printf("@@@@@ %s() %d, %s: 0x%x\n", __func__, __LINE__, #m, (u32)m); \
	jlgpu_matrix_dump(m, #m); \
	printf("\n"); \
}


#endif



