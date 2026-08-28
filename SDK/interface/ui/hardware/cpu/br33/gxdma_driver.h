#ifndef __GXDMA_DRIVER_H__
#define __GXDMA_DRIVER_H__

#include "typedef.h"

//// gxdma register define
typedef struct {
    volatile unsigned int all_done_pnd : 1;
        volatile unsigned int block_done_pnd : 1;
        volatile unsigned int pause_pnd : 1;
        volatile unsigned int unuse0 : 29;
    } GXDMA_PND_CON;

    typedef struct {
    volatile unsigned int all_done_clr : 1;
        volatile unsigned int block_done_clr : 1;
        volatile unsigned int pause_clr : 1;
        volatile unsigned int unuse0 : 29;
    } GXDMA_PND_CLR;

    typedef struct {
    volatile unsigned int all_done_ie : 1;
        volatile unsigned int block_done_ie : 1;
        volatile unsigned int pause_ie : 1;
        volatile unsigned int unuse0 : 29;
    } GXDMA_PND_IE;

    typedef struct {
    volatile unsigned int clk_en : 1;
        volatile unsigned int nrst : 1;
        volatile unsigned int unuse : 30;
    } GXDMA_COM_CON;

    typedef struct {
    volatile unsigned int kstart : 1;
        volatile unsigned int pause : 1;
        volatile unsigned int vbus_clean : 1;
        volatile unsigned int unuse0 : 13;
        volatile unsigned int status : 2;
        volatile unsigned int unuse1 : 14;
    } GXDMA_STA_CON;

    typedef struct {
    volatile unsigned int src_fmt : 5;
        volatile unsigned int src_big_end : 1;
        volatile unsigned int src_alpha_end : 1;
        volatile unsigned int src_rb_swap : 1;
        volatile unsigned int unuse : 24;
    } GXDMA_SRC_FMT;

    typedef struct {
    volatile unsigned int pic_w : 12;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int pic_h : 12;
        volatile unsigned int unuse1 : 4;
    } GXDMA_SRC_PIC;

    typedef struct {
    volatile unsigned int x_min : 12;
        volatile unsigned int unuse0 : 20;
    } GXDMA_SRC_X_MIN;

    typedef struct {
    volatile unsigned int x_max : 12;
        volatile unsigned int unuse0 : 20;
    } GXDMA_SRC_X_MAX;

    typedef struct {
    volatile unsigned int y_min : 12;
        volatile unsigned int unuse0 : 20;
    } GXDMA_SRC_Y_MIN;

    typedef struct {
    volatile unsigned int y_max : 12;
        volatile unsigned int unuse0 : 20;
    } GXDMA_SRC_Y_MAX;

    typedef struct {
    volatile unsigned int src_stride : 15;
        volatile unsigned int unuse0 : 17;
    } GXDMA_SRC_STRIDE;

    typedef struct {
    volatile unsigned int src_base_adr : 32;
    } GXDMA_SRC_BADR;

    typedef struct {
    volatile unsigned int dst_fmt : 5;
        volatile unsigned int dst_big_end : 1;
        volatile unsigned int dst_alpha_end : 1;
        volatile unsigned int dst_rb_swap : 1;
        volatile unsigned int dst_adr_mode : 1;
        volatile unsigned int rot_mode : 4;
        volatile unsigned int dither_en : 1;
        volatile unsigned int limit_en : 1;
        volatile unsigned int unuse : 6;
        volatile unsigned int jstc_en : 1;
        volatile unsigned int jstc_mode : 2;
        volatile unsigned int jstc_gray_th : 8;
    } GXDMA_DST_FMT;

    typedef struct {
    volatile unsigned int dst_stride : 18;
        volatile unsigned int unuse0 : 14;
    } GXDMA_DST_STRIDE;

    typedef struct {
    volatile unsigned int dst_base_adr : 32;
    } GXDMA_DST_BADR;

    typedef struct {
    volatile unsigned int dst_base_uv : 32;
    } GXDMA_DST_BADR_UV;

    typedef struct {
    GXDMA_PND_CON pnd_con;
    GXDMA_PND_CLR pnd_clr;
    GXDMA_PND_IE pnd_ie;
    GXDMA_COM_CON com_con;
    GXDMA_STA_CON sta_con;
    GXDMA_SRC_FMT src_fmt;
    GXDMA_SRC_PIC src_pic;
    GXDMA_SRC_X_MIN src_x_min;
    GXDMA_SRC_X_MAX src_x_max;
    GXDMA_SRC_Y_MIN src_y_min;
    GXDMA_SRC_Y_MAX src_y_max;
    GXDMA_SRC_STRIDE src_stride;
    GXDMA_SRC_BADR src_adr;
    GXDMA_DST_FMT dst_fmt;
    GXDMA_DST_STRIDE dst_stride;
    GXDMA_DST_BADR dst_adr;
    GXDMA_DST_BADR_UV dst_adr_uv;
} GXDMA_SFR_TypeDef;
////////////////////////////////////////////////
//
/**
 * @brief GXDMA完成回调函数
 * @param user_data: 用户数据
 */
typedef void (*gxdma_done_callback_t)(void *user_data);

/**
 * @brief 回调函数配置
 */
typedef struct {
    gxdma_done_callback_t done_callback;    // 完成回调
    void *user_data;                        // 用户数据
} gxdma_callback_t;

enum {
    GXDMA_OUT_ROTATE_0 = 0,
    GXDMA_OUT_MIRROR,
    GXDMA_OUT_FLIP,
    GXDMA_OUT_ROTATE_90,
    GXDMA_OUT_ROTATE_180,
    GXDMA_OUT_ROTATE_270,
    GXDMA_OUT_ROTATE_90_MIRROR,
    GXDMA_OUT_ROTATE_90_FLIP,
};

enum {
    GXDMA_JSTC_MODE_A = 0,
    GXDMA_JSTC_MODE_RGB888 = 1,
    GXDMA_JSTC_MODE_ARGB888 = 2,
};

typedef struct {
    uint16_t width;
    uint16_t height;

    uint16_t src_fmt;
    uint16_t in_stride;
    uint8_t *src;
    uint8_t src_rb_swap;
    uint8_t src_big_end;
    uint8_t src_alpha_end;

    uint16_t dst_fmt;
    uint16_t out_stride;
    uint8_t *dst;
    uint8_t *dst_uv;
    uint8_t out_rb_swap;
    uint8_t out_big_end;
    uint8_t out_alpha_end;
    uint8_t out_adr_mode;
    uint8_t jstc_mode;
    uint8_t gray_th;
    uint8_t rot_mode;
    uint8_t jstc_en;
    uint8_t limit_en;
    uint8_t dither_en;
} gxdma_reg_param_t;

int JLGXDMA_init();
void JLGXDMA_set_param(gxdma_reg_param_t *reg_param);
void JLGXDMA_set_src_area(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max);
uint8_t is_gxdma_rotate(uint8_t rot_mode);
void JLGXDMA_link_gpu(u8 en);
int JLGXDMA_register_callback(const gxdma_callback_t *callbacks);
int JLGXDMA_run();
int JLGXDMA_wait_done();
void JLGXDMA_irq_init(void);
void JLGXDMA_irq_uninit(void);
#endif
