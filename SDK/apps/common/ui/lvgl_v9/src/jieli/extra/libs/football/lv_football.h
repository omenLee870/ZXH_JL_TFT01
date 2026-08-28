#ifndef LV_FOOTBALL_H
#define LV_FOOTBALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lv_draw_jlgpu.h"

/**
 * @brief 足球球面图标描述（lvgl 原生，不依赖 jlui 的 struct ui_image_attrs）
 *
 * 字段已按渲染需求归一：compress_mode / adr_mode 直接存 JLGPU_* 枚举，
 * 不再是 jlui 侧的 ENCODE_* 编码值，渲染核心可直接使用。
 */
typedef struct {
    uint16_t width;            /* 图像宽 */
    uint16_t height;           /* 图像高 */
    uint8_t  gpu_format;       /* JLGPU_FORMAT_* */
    uint8_t  clut_format;      /* JLGPU_FORMAT_*（有 clut 时有效） */
    uint8_t  compress_mode;    /* JLGPU_COMPRESS_MODE_* */
    uint8_t  adr_mode;         /* JLGPU_ADRMODE_* */
    uint8_t  has_clut;
    uint8_t  premult;
    uint8_t *data;             /* 实际图像数据（已偏移压缩头/palette） */
    uint8_t *tab;              /* mmu table（可选） */
    uint8_t *clut_tab;         /* clut 表地址（可选） */
    uint32_t len;              /* 压缩大小（RLE/ETC2）或原始长度 */
} lv_football_img_t;

/* 图标背景颜色（六边形背景叠加用，当前足球未启用） */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} lv_football_bg_rgb_t;

/* 足球绘制参数，替代 jlui 的 struct football_param */
typedef struct {
    lv_football_img_t *image;       /* 球面图标数组 */
    int image_num;
    lv_football_img_t *image_bg;    /* 六边形背景图（可选，mask_en 时有效） */
    int image_bg_num;
    lv_football_bg_rgb_t *bg_rgb;   /* 背景颜色（可选） */
} lv_football_param_t;

/*----------------------------------------------------------------------------*/
/**@brief    足球模型初始化
   @param    width : 足球球面宽度
   @param    height : 足球球面高度
   @param    scale : 足球球面缩放
   @param    view_distance : 视距(值越小, 足球越大)
   @param    win_x : win区域x坐标偏移
   @param    win_y : win区域y坐标偏移
   @param    win_w : win区域宽度
   @param    win_h : win区域高度
*/
/*----------------------------------------------------------------------------*/
int lv_football_init(int width, int height, float scale, float view_distance,
                     int win_x, int win_y, int win_w, int win_h);

/*----------------------------------------------------------------------------*/
/**@brief    足球一帧渲染：组球面透视任务 + 输出到 out_buf + 执行 GPU 合成。
             由 disp_flush 调用（全屏特效统一调度），内部使用足球独立 ctx，
             不污染 lvgl 正常 draw 流程的 unit ctx。
   @param    unit : lvgl draw unit
   @param    param : 足球显示相关参数
   @param    x_diff : x方向偏移
   @param    y_diff : y方向偏移
   @param    out_buf : 全帧 RGB565 输出缓冲（disp_flush 的 page_effect_buf）
   @param    screen_area : 输出区域
*/
/*----------------------------------------------------------------------------*/
void lv_football_flush(lv_draw_jlgpu_unit_t *unit, lv_football_param_t *param,
                       float x_diff, float y_diff, uint8_t *out_buf, lv_area_t *screen_area);

/*----------------------------------------------------------------------------*/
/**@brief   足球触摸点检测
   @param   x : 触摸点x坐标
   @param   y : 触摸点y坐标
   @return  -1: 触摸点未落在球面上, >= 0: 球面索引(从0开始)
*/
/*----------------------------------------------------------------------------*/
int lv_football_get_face_index(int x, int y);

/*----------------------------------------------------------------------------*/
/**@brief    足球资源释放
*/
/*----------------------------------------------------------------------------*/
void lv_football_uninit(void);

/*----------------------------------------------------------------------------*/
/**@brief   获取足球位置
   @param   cur_mat : 位置参数, float[9]数组
*/
/*----------------------------------------------------------------------------*/
void lv_football_get_pos(float *cur_mat);

/*----------------------------------------------------------------------------*/
/**@brief   设置足球位置
   @param   cur_mat : 位置参数, float[9]数组
*/
/*----------------------------------------------------------------------------*/
void lv_football_set_pos(float *cur_mat);

/*----------------------------------------------------------------------------*/
/**@brief   设置mask图片缩放倍数（mask_en使能后有效）
   @param   scale : 缩放倍数
*/
/*----------------------------------------------------------------------------*/
void lv_football_set_mask_scale(float scale);

/*----------------------------------------------------------------------------*/
/**@brief    足球六边形背景+图标mask
   @param    flag : 0/1
*/
/*----------------------------------------------------------------------------*/
void lv_football_set_bg_mask_en(uint8_t flag);

#ifdef __cplusplus
}
#endif

#endif /* LV_FOOTBALL_H */
