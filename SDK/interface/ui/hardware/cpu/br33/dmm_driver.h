
#ifndef __JLDMM_DRIVER_H__
#define __JLDMM_DRIVER_H__

#include "typedef.h"

typedef struct {
    volatile unsigned int dmm_en : 1;
    volatile unsigned int sfr_update : 1;
    volatile unsigned int sfr_busy : 1;
    volatile unsigned int dmm_busy : 1;
    volatile unsigned int dmm_kst : 1;
    volatile unsigned int dmm_continuous : 1;
    volatile unsigned int dmm_stop : 1;
    volatile unsigned int sw_ctl : 1;
    volatile unsigned int sw_done : 1;
    volatile unsigned int unuse0 : 23;
} DMM_CON0;

typedef struct {
    volatile unsigned int fst_ie : 1;
    volatile unsigned int sfr_ie : 1;
    volatile unsigned int fend_ie : 1;
    volatile unsigned int line_ie : 1;
    volatile unsigned int buf_ie : 1;
    volatile unsigned int soft_ie : 1;
    volatile unsigned int stop_ie : 1;
    volatile unsigned int unuse0 : 25;
} DMM_PND_IE;


typedef struct {
    volatile unsigned int fst_clr : 1;
    volatile unsigned int sfr_clr : 1;
    volatile unsigned int fend_clr : 1;
    volatile unsigned int line_clr : 1;
    volatile unsigned int buf_clr : 1;
    volatile unsigned int soft_clr : 1;
    volatile unsigned int stop_clr : 1;
    volatile unsigned int unuse0 : 25;
} DMM_PND_CLR;

typedef struct {
    volatile unsigned int fst_pnd : 1;
    volatile unsigned int sfr_pnd : 1;
    volatile unsigned int fend_pnd : 1;
    volatile unsigned int line_pnd : 1;
    volatile unsigned int buf_pnd : 1;
    volatile unsigned int soft_pnd : 1;
    volatile unsigned int stop_pnd : 1;
    volatile unsigned int unuse0 : 25;
} DMM_PND;

typedef struct {
    volatile unsigned int line : 13;
    volatile unsigned int unuse0 : 19;
} DMM_Line_TAR;

typedef struct {
    volatile unsigned int slice_buf : 1;
    volatile unsigned int slice_first : 1;
    volatile unsigned int slice_last : 1;
    volatile unsigned int slice_width : 13;
    volatile unsigned int slice_height : 13;
    volatile unsigned int unuse0 : 3;
} DMM_SLICE_CON;

typedef struct {
    volatile unsigned int slice_adr : 32;
} DMM_SLICE_ADR;

typedef struct {
    volatile unsigned int in_color_fmt : 5;
    volatile unsigned int in_big_endian : 1;
    volatile unsigned int in_rb_swap : 1;
    volatile unsigned int in_alpha_end : 1;
    volatile unsigned int in_adr_mode : 1;
    volatile unsigned int in_src_mode : 2;
    volatile unsigned int in_jstc_en : 1;
    volatile unsigned int unuse0 : 4;
    volatile unsigned int out_color_fmt : 5;
    volatile unsigned int out_big_endian : 1;
    volatile unsigned int out_rb_swap : 1;
    volatile unsigned int dither_en : 1;
    volatile unsigned int out_buf_line : 5;
    volatile unsigned int unuse1 : 3;
} DMM_CON1;

typedef struct {
    volatile unsigned int in0_x_st : 13;
    volatile unsigned int unuse0 : 3;
    volatile unsigned int in0_y_st : 13;
    volatile unsigned int unuse1 : 3;
} DMM_IN0_ST;

typedef struct {
    volatile unsigned int in0_x_end : 13;
    volatile unsigned int unuse0 : 3;
    volatile unsigned int in0_y_end : 13;
    volatile unsigned int unuse1 : 3;
} DMM_IN0_END;

typedef struct {
    volatile unsigned int in0_stride : 17;
    volatile unsigned int unuse0 : 15;
} DMM_IN0_STRIDE;

typedef struct {
    volatile unsigned int in0_base : 32;
} DMM_IN0_BASE;

typedef struct {
    volatile unsigned int in0_base_uv : 32;
} DMM_IN0_BASE_UV;

typedef struct {
    volatile unsigned int in1_x_st : 13;
    volatile unsigned int unuse0 : 3;
    volatile unsigned int in1_y_st : 13;
    volatile unsigned int unuse1 : 3;
} DMM_IN1_ST;

typedef struct {
    volatile unsigned int in1_x_end : 13;
    volatile unsigned int unuse0 : 3;
    volatile unsigned int in1_y_end : 13;
    volatile unsigned int unuse1 : 3;
} DMM_IN1_END;

typedef struct {
    volatile unsigned int in1_stride : 17;
    volatile unsigned int unuse0 : 15;
} DMM_IN1_STRIDE;

typedef struct {
    volatile unsigned int in1_base : 32;
} DMM_IN1_BASE;

typedef struct {
    volatile unsigned int in1_base_uv : 32;
} DMM_IN1_BASE_UV;

typedef struct {
    volatile unsigned int out_stride : 15;
    volatile unsigned int unuse0 : 17;
} DMM_OUT_STRIDE;

typedef struct {
    volatile unsigned int out_base : 32;
} DMM_OUT_BASE0;

typedef struct {
    volatile unsigned int out_base : 32;
} DMM_OUT_BASE1;

typedef struct {
    volatile int ccm00 : 16;
    volatile int ccm01 : 16;
} DMM_CCM0_CON0;

typedef struct {
    volatile int ccm02 : 16;
    volatile unsigned int unuse0 : 16;
} DMM_CCM0_CON1;

typedef struct {
    volatile int ccm03 : 22;
    volatile unsigned int unuse0 : 10;
} DMM_CCM0_CON2;

typedef struct {
    volatile int ccm10 : 16;
    volatile int ccm11 : 16;
} DMM_CCM1_CON0;

typedef struct {
    volatile int ccm12 : 16;
    volatile unsigned int unuse0 : 16;
} DMM_CCM1_CON1;

typedef struct {
    volatile int ccm13 : 22;
    volatile unsigned int unuse0 : 10;
} DMM_CCM1_CON2;

typedef struct {
    volatile int ccm20 : 16;
    volatile int ccm21 : 16;
} DMM_CCM2_CON0;

typedef struct {
    volatile int ccm22 : 16;
    volatile unsigned int unuse0 : 16;
} DMM_CCM2_CON1;

typedef struct {
    volatile int ccm23 : 22;
    volatile unsigned int unuse0 : 10;
} DMM_CCM2_CON2;

typedef struct {
    uint8_t gamma_lut[256];
} DMM_LUT_DATA;

typedef struct {
    DMM_CON0        dmm_con0;
    DMM_PND_IE      dmm_pnd_ie;
    DMM_PND_CLR     dmm_pnd_clr;
    DMM_PND         dmm_pnd;
    DMM_Line_TAR    dmm_line_tar;
    DMM_SLICE_CON   dmm_slice_con;
    DMM_SLICE_ADR   dmm_slice_adr;
    uint32_t        RESERVED[0x80 - 0x6 - 1];
    DMM_CON1        dmm_con1;
    DMM_IN0_ST      dmm_in0_st;
    DMM_IN0_END     dmm_in0_end;
    DMM_IN0_STRIDE  dmm_in0_stride;
    DMM_IN0_BASE    dmm_in0_base;
    DMM_IN0_BASE_UV dmm_in0_base_uv;
    DMM_IN1_ST      dmm_in1_st;
    DMM_IN1_END     dmm_in1_end;
    DMM_IN1_STRIDE  dmm_in1_stride;
    DMM_IN1_BASE    dmm_in1_base;
    DMM_IN1_BASE_UV dmm_in1_base_uv;
    DMM_OUT_STRIDE  dmm_out_stride;
    DMM_OUT_BASE0   dmm_out_base0;
    DMM_OUT_BASE1   dmm_out_base1;
    DMM_CCM0_CON0   dmm_ccm0_con0;
    DMM_CCM0_CON1   dmm_ccm0_con1;
    DMM_CCM0_CON2   dmm_ccm0_con2;
    DMM_CCM1_CON0   dmm_ccm1_con0;
    DMM_CCM1_CON1   dmm_ccm1_con1;
    DMM_CCM1_CON2   dmm_ccm1_con2;
    DMM_CCM2_CON0   dmm_ccm2_con0;
    DMM_CCM2_CON1   dmm_ccm2_con1;
    DMM_CCM2_CON2   dmm_ccm2_con2;
    DMM_LUT_DATA    dmm_lut_data;
} JLDMM_SFR_TypeDef;

typedef struct {
    int matrix[12];
} dmm_reg_ccm_param_t;

typedef struct {
    uint8_t gamma[256];
} dmm_reg_gamma_param_t;

typedef struct {
    uint8_t in_color_fmt;
    uint8_t in_big_endian;
    uint8_t in_rb_swap;
    uint8_t in_alpha_end;
    uint8_t in_adr_mode;
    uint8_t in_src_mode;
    uint8_t in_jstc_en;

    uint8_t out_color_fmt;
    uint8_t out_big_endian;
    uint8_t out_rb_swap;
    uint8_t dither_en;
    uint8_t out_buf_line;

    uint8_t continue_mode;

} dmm_reg_common_param_t;


typedef struct {
    int x_min;
    int x_max;
    int y_min;
    int y_max;
} dmm_reg_area_param_t;


typedef struct {
    int src0_stride;
    int src1_stride;
    int out_stride;
} dmm_reg_stride_param_t;

typedef struct {
    uint8_t *src0_adr;
    uint8_t *src0_uv_adr;
    uint8_t *src1_adr;
    uint8_t *src1_uv_adr;
    uint8_t *dst_adr0;
    uint8_t *dst_adr1;
} dmm_reg_adr_param_t;

//////////////////////
//format

typedef enum {
    JLDMM_FORMAT_ARGB8888 = 0x00,
    JLDMM_FORMAT_ARGB8565 = 0x01,
    JLDMM_FORMAT_ARGB1555 = 0x02,
    JLDMM_FORMAT_ARGB4444 = 0x03,
    JLDMM_FORMAT_RGB888 = 0x04,
    JLDMM_FORMAT_RGB565 = 0x05,

    JLDMM_FORMAT_YUV422_BT601 = 0x06,
    JLDMM_FORMAT_YUV422_BT709 = 0x07,

    JLDMM_FORMAT_AL88 = 0x08,

    JLDMM_FORMAT_YUV420_BT601 = 0x09,
    JLDMM_FORMAT_YUV420_BT709 = 0x0a,

    JLDMM_FORMAT_YUV444_BT601 = 0x0b,
    JLDMM_FORMAT_YUV444_BT709 = 0x0c,
    JLDMM_FORMAT_A8_T8 = 0x0d,
    JLDMM_FORMAT_RGB666 = 0x0e,//add

    JLDMM_FORMAT_AL44 = 0x10,
    JLDMM_FORMAT_AL22 = 0x11,
    JLDMM_FORMAT_L8 = 0x12,
    JLDMM_FORMAT_L4 = 0x13,
    JLDMM_FORMAT_L2 = 0x14,
    JLDMM_FORMAT_L1 = 0x15,
    JLDMM_FORMAT_A8 = 0x16,
    JLDMM_FORMAT_A4 = 0x17,
    JLDMM_FORMAT_A2 = 0x18,
    JLDMM_FORMAT_A1 = 0x19,
} JLDMM_Img_Format_TypeDef;


#include "system/includes.h"

/*==============================================================================
 *                           回调函数类型
 *============================================================================*/

/**
 * @brief DMM帧开始回调
 * @param user_data: 用户数据
 */
typedef void (*dmm_frame_start_callback_t)(void *user_data);

/**
 * @brief DMM帧结束回调
 * @param user_data: 用户数据
 */
typedef void (*dmm_frame_end_callback_t)(void *user_data);

/**
 * @brief DMM行处理回调（用于同步DBI输出）
 * @param line: 当前行号
 * @param user_data: 用户数据
 */
typedef void (*dmm_line_callback_t)(uint32_t line, void *user_data);

/**
 * @brief DMM Slice Buffer完成回调
 * @param user_data: 用户数据
 */
typedef void (*dmm_buffer_done_callback_t)(void *user_data);

/**
 * @brief DMM Stop 停止回调
 * @param user_data: 用户数据
 */
typedef void (*dmm_frame_stop_callback_t)(void *user_data);

/**
 * @brief 回调函数配置
 */
typedef struct {
    dmm_frame_start_callback_t frame_start_cb;
    dmm_frame_end_callback_t frame_end_cb;
    dmm_line_callback_t line_cb;
    dmm_buffer_done_callback_t buffer_done_cb;
    dmm_frame_stop_callback_t dmm_stop_cb;
    void *user_data;
} dmm_callback_t;

uint32_t dmm_get_format_bpp(uint32_t format);
uint32_t dmm_get_yuv420_bpp(uint32_t format, uint32_t adr_mode);


void dmm_set_ccm_param(dmm_reg_ccm_param_t *reg_ccm_param);
void dmm_set_gamma_param(dmm_reg_gamma_param_t *reg_gamma_param);
void dmm_set_common_param(dmm_reg_common_param_t *reg_common_param);
void dmm_set_src0_area_param(dmm_reg_area_param_t *reg_area_param);
void dmm_set_src1_area_param(dmm_reg_area_param_t *reg_area_param);
void dmm_set_stride_param(dmm_reg_stride_param_t *reg_stride_param);
void dmm_set_adr_param(dmm_reg_adr_param_t *reg_adr_param);
void dmm_set_dst_adr_param(dmm_reg_adr_param_t *reg_adr_param);
void dmm_set_src_adr_param(dmm_reg_adr_param_t *reg_adr_param);

int JLDMM_init();
int JLDMM_free();
int JLDMM_sw_run();
void JLDMM_run();
int JLDMM_wait_done();
int JLDMM_stop();
void JLDMM_irq_init(void);
void JLDMM_irq_uninit(void);

int JLDMM_run_slice();
int JLDMM_wait_done_slice();
int JLDMM_wait_sfr_finish();
void JLDMM_set_no_wait_vsync(uint8_t no_wait_vsync);
/* 预置 JLDMM_wait_done() 所用 frame 信号量的计数: SPI/MCU 屏(单帧模式)首帧置 1,
 * 让"先 wait_done 再 run"的首帧不必等一个还没发生的 frame end */
void JLDMM_set_frame_sem(uint8_t cnt);

int JLDMM_register_callback(const dmm_callback_t *callbacks);

void dmm_debug_sfr_param();

void set_rgb_gain_ccm_param(float r_gain, float g_gain, float b_gain, dmm_reg_ccm_param_t *ccm_param);
void set_brightenss_ccm_param(float brightenss_gain, dmm_reg_ccm_param_t *ccm_param);
void set_contrast_ccm_param(float contrast_gain, int mean, dmm_reg_ccm_param_t *ccm_param);
void set_saturation_ccm_param(float saturation_gain, dmm_reg_ccm_param_t *ccm_param);
void set_hue_ccm_param(float angle, dmm_reg_ccm_param_t *ccm_param);
void set_gamma_param(float gamma, dmm_reg_gamma_param_t *gamma_param);

#endif

