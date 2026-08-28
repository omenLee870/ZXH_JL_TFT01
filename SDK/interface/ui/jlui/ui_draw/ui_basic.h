#ifndef __BASIC_H__
#define __BASIC_H__

#include "generic/typedef.h"
#include "generic/rect.h"

#define ALIGN_MASK  0x3

#define COLOR_16_SWAP   0
typedef union {
    struct {
#if COLOR_16_SWAP == 0
        u16 blue : 5;
        u16 green : 6;
        u16 red : 5;
#else
        u16 green_h : 3;
        u16 red : 5;
        u16 blue : 5;
        u16 green_l : 3;
#endif
    } ch;
    u16 full;
} color16_t;

typedef union {
    struct {
        u16 blue : 5;
        u16 green : 6;
        u16 red : 5;
    } ch;
    u16 full;
} color16_l;

typedef union {
    struct {
        u16 green_h : 3;
        u16 red : 5;
        u16 blue : 5;
        u16 green_l : 3;
    } ch;
    u16 full;
} color16_b;

typedef union {
    struct {
        u8 blue;
        u8 green;
        u8 red;
        u8 alpha;
    } ch;
    u32 full;
} color32_t;


typedef color16_t color_t;
typedef color16_l color_l;
typedef color16_b color_b;

struct gpu_blend_interface {
    void (*gpu_blend)(u8 *mask_buf, struct rect *mask_r, u8 *disp_buf, struct rect *disp_r, int color);
    void (*gpu_output)(u8 *mask_buf, struct rect *mask_r, u8 *disp_buf, struct rect *disp_r, int color);
    //expand
};




int ui_disp_line_buf_init();
int ui_disp_line_buf_uninit();
void *ui_disp_line_buf_malloc(int size);
void ui_disp_line_buf_release(void *buf);
int ui_disp_win_hor_res_get();

void ui_disp_buffer_rect_set(struct rect *rect);
void ui_disp_buffer_rect_get(struct rect *rect);
struct rect *ui_disp_dispbuf_rect_get();
u8 *ui_disp_buffer_get();
void ui_disp_buffer_set(u16 *buf, int buf_size);
u16 ui_disp_get_buffer_size();
u16 ui_disp_get_buffer_line(u16 width);
void ui_disp_buffer_clear(struct rect *rect, color_t color);
void ui_disp_buffer_flush(struct rect *rect, u8 *buffer);
int ui_disp_out_stride_set(u16 stride);
int ui_disp_out_stride_get();

int ui_gpu_buf_max_size_set(int max_size);
int ui_gpu_buf_max_size_get();
int ui_gpu_buf_init();
int ui_gpu_buf_release();
u8 *ui_gpu_buf_get();
int ui_gpu_buf_clear();
void ui_gpu_buf_set(u8 *buf, int size);
int ui_gpu_buf_get_default_size();

void memset_fast(void *dst, u8 v, u32 len);
void memset_ff(void *dst, u32 len);
void memset_00(void *dst, u32 len);

void blend(u8 *mask_buf, int abs_x, int abs_y, int len, u8 calpha, color_t color);
void blend_nopa(u8 *mask_buf, int abs_x, int abs_y, int len, color_t color);
void blend_opa(u8 *mask_buf, int abs_x, int abs_y, int len, color_t color, u8 opa);
void blend_map(u8 *mask_buf, int abs_x, int abs_y, int len, color_t *map);
void blend_map_opa(u8 *mask_buf, int abs_x, int abs_y, int len, color_t *map, u8 opa);

int gpu_blend_init(struct gpu_blend_interface *api);
void gpu_blend(u8 *mask_buf, struct rect *mask_r, u8 *disp_buf, struct rect *disp_r, int color);
#endif

