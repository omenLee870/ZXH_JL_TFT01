
#ifndef __PUBLIC_GPU_DRAW_H__
#define __PUBLIC_GPU_DRAW_H__

/* 自定义绘图信息和回调 */
typedef struct gpu_task_draw {
    void (*cb_func)(int id, u8 *dst_buf, \
                    struct rect *dst_r, struct rect *src_r, \
                    u8 bytes_per_pixel, void *priv, void *matrix);
    void (*cb_exit)(void *draw_info, void *priv);
    void *priv;
    int priv_len;
    int draw_id;
    struct rect task_r;
} JLGPUTaskDraw_t, *pJLGPUTaskDraw_t;


void jlui_gpu_blend_init();
int jdec_to_jlgpu_texture_blend(u8 *dst_buf, struct rect *dst_rect, u8 *src_buf, struct rect *src_rect, struct rect *draw_rect, int src_stride, int format, void *p, int line_surpule, struct rect *cover);

#endif
