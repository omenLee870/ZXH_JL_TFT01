/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/**
 * @file    ui_pipeline_demo.c
 * @brief   UI→GPU→LCD 流水线 Demo — 4 个页面示例(466x466圆形屏)
 *
 * 页面1 "表盘": 径向渐变背景 + 渐变色圆环(270°开口, 用 angle 控制) + 中心纹理
 * 页面2 "运动环": 三色圆环叠加 + 深色背景 + 中心半透明数值区
 * 页面3 "媒体卡": 深紫渐变背景 + 居中封面纹理 + 渐变进度条
 * 页面4 "球面图标阵": 37图标同心圆布局, sphere projection 缩放模拟球面效果
 *
 * 规则: 每个 widget_* 函数 = 1 个 GPU 任务, xxx_param_init 定义局部 task_param
 *
 * API 校正 (参照 gpu_port.h):
 *   - ring(cx, cy, ri, ro)  仅4参数 — 圆环内径/外径
 *   - angle(cx, cy, sa, ea) 仅4参数 — 起始/结束角度 (ring+angle 复合渐变用)
 *   - radial(cx, cy, ri, ro) 仅4参数 — 径向渐变内外径
 *   - texture_param_init 仅2参数 (task_id, elm_id)
 */

#include "gpu_demo.h"
#include "img_data/img_data.h"
#include <math.h>
#include "res/resfile.h"

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)

#define PAGE_W    466
#define PAGE_H    466
#define IMG_TEST  IMG_PIXEL_ARGB8888

/* ═══════════════════════════════════════════════════════════════════════
 *  页面1 "表盘": 3 控件
 *    bg(径向渐变) → ring(蓝紫渐变色圆环, 270°开口, 外210内180) → center(纹理居中160)
 * ═══════════════════════════════════════════════════════════════════════ */

static void w1_bg(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page) / 2;
    int cy = rect_top(page) + rect_height(page) / 2;
    static u32 clut[] = {
        ARGB8888(0xff, 0x08, 0x0c, 0x1a),
        ARGB8888(0xff, 0x0e, 0x16, 0x2e),
        ARGB8888(0xff, 0x04, 0x06, 0x10),
    };

    jlgpu_task_gradient_param_init(1, 0);
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);
    jlgpu_task_gradient_set_affine_translate(rect_left(page), rect_top(page));
    jlgpu_task_gradient_set_radial(cx, cy, 0, rect_width(page) / 2);
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);
    jlgpu_task_gradient_set_draw(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_task_gradient_set_area(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 1, 0, &task_param);
}

static void w1_ring(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page) / 2;
    int cy = rect_top(page) + rect_height(page) / 2;
    static u32 clut[] = {
        ARGB8888(0xff, 0x00, 0xa0, 0xe0),
        ARGB8888(0x80, 0x00, 0x50, 0xb0),
        ARGB8888(0x40, 0x00, 0x30, 0x80),
    };

    jlgpu_task_gradient_param_init(2, 1);
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_REPEAT);
    jlgpu_task_gradient_set_affine_translate(cx - 210, cy - 210);
    jlgpu_task_gradient_set_linear(cx - 210, cy - 210, cx + 210, cy + 210);
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RING_GRADIENT);
    jlgpu_task_gradient_set_ring(cx, cy, 180, 210);
    jlgpu_task_gradient_set_angle(cx, cy, 0, 270);
    jlgpu_task_gradient_set_draw(cx - 210, cy - 210, 420, 420);
    jlgpu_task_gradient_set_area(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 2, 1, &task_param);
}

static void w1_center(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page) / 2;
    int cy = rect_top(page) + rect_height(page) / 2;
    int d  = 150;
    int x  = cx - d / 2, y = cy - d / 2;

    jlgpu_task_texture_param_init(3, 2);
    jlgpu_task_img_set_format(img_data_get_format(IMG_TEST));
    jlgpu_task_img_set_size(img_data_get_width(IMG_TEST),
                            img_data_get_height(IMG_TEST));
    jlgpu_task_img_set_data(img_data_get_addr(IMG_TEST));
    jlgpu_task_img_set_endian(img_data_get_endian(IMG_TEST));
    jlgpu_task_img_set_compress(img_data_get_compress_mode(IMG_TEST));
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(IMG_TEST));
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(IMG_TEST));
    jlgpu_task_img_set_draw(x, y, d, d);
    jlgpu_task_img_set_area(rect_left(page), rect_top(page),
                            rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 3, 2, &task_param);
}

pJLGPUChainHead_t draw_page_1(pJLGPUChainHead_t head, struct rect *screen)
{
    struct rect page = {rect_left(screen), rect_top(screen), PAGE_W, PAGE_H};
    w1_bg(head, &page);
    w1_ring(head, &page);
    w1_center(head, &page);
    return head;
}


/* ═══════════════════════════════════════════════════════════════════════
 *  页面2 "运动环": 5 控件
 *    bg(深色填充) → ring1(橙红, 外180内166, 330°=91%) → ring2(绿, 外158内144, 240°=67%)
 *    → ring3(蓝, 外136内122, 180°=50%) → value(中心填充)
 * ═══════════════════════════════════════════════════════════════════════ */

static void w2_bg(pJLGPUChainHead_t head, struct rect *page)
{
    jlgpu_task_fill_param_init(1, 0, 0xff, 0x0a, 0x0c, 0x14);
    jlgpu_task_fill_set_draw(rect_left(page), rect_top(page),
                             rect_width(page), rect_height(page));
    jlgpu_task_fill_set_area(rect_left(page), rect_top(page),
                             rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 1, 0, &task_param);
}

static void w2_ring1(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page)  / 2;
    int cy = rect_top(page)  + rect_height(page) / 2;
    static u32 clut[] = {
        ARGB8888(0xff, 0xff, 0x30, 0x50),
        ARGB8888(0x80, 0xc0, 0x20, 0x40),
    };

    jlgpu_task_gradient_param_init(2, 1);
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_gradient_set_clut(clut, 0, JLGPU_CLUT_FORMAT_ARGB8888);
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);
    jlgpu_task_gradient_set_affine_translate(cx - 180, cy - 180);
    jlgpu_task_gradient_set_linear(cx - 180, cy - 180, cx + 180, cy + 180);
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RING_GRADIENT);
    jlgpu_task_gradient_set_ring(cx, cy, 166, 180);
    jlgpu_task_gradient_set_angle(cx, cy, 0, 330);
    jlgpu_task_gradient_set_draw(cx - 180, cy - 180, 360, 360);
    jlgpu_task_gradient_set_area(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 2, 1, &task_param);
}

static void w2_ring2(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page)  / 2;
    int cy = rect_top(page)  + rect_height(page) / 2;
    static u32 clut[] = {
        ARGB8888(0xff, 0x20, 0xd0, 0x60),
        ARGB8888(0x80, 0x10, 0x90, 0x40),
    };

    jlgpu_task_gradient_param_init(3, 2);
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_gradient_set_clut(clut, 0, JLGPU_CLUT_FORMAT_ARGB8888);
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);
    jlgpu_task_gradient_set_affine_translate(cx - 158, cy - 158);
    jlgpu_task_gradient_set_linear(cx - 158, cy - 158, cx + 158, cy + 158);
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RING_GRADIENT);
    jlgpu_task_gradient_set_ring(cx, cy, 144, 158);
    jlgpu_task_gradient_set_angle(cx, cy, 0, 240);
    jlgpu_task_gradient_set_draw(cx - 158, cy - 158, 316, 316);
    jlgpu_task_gradient_set_area(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 3, 2, &task_param);
}

static void w2_ring3(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page)  / 2;
    int cy = rect_top(page)  + rect_height(page) / 2;
    static u32 clut[] = {
        ARGB8888(0xff, 0x00, 0xa0, 0xf0),
        ARGB8888(0x80, 0x00, 0x60, 0xc0),
    };

    jlgpu_task_gradient_param_init(4, 3);
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_gradient_set_clut(clut, 0, JLGPU_CLUT_FORMAT_ARGB8888);
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);
    jlgpu_task_gradient_set_affine_translate(cx - 136, cy - 136);
    jlgpu_task_gradient_set_linear(cx - 136, cy - 136, cx + 136, cy + 136);
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RING_GRADIENT);
    jlgpu_task_gradient_set_ring(cx, cy, 122, 136);
    jlgpu_task_gradient_set_angle(cx, cy, 0, 180);
    jlgpu_task_gradient_set_draw(cx - 136, cy - 136, 272, 272);
    jlgpu_task_gradient_set_area(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 4, 3, &task_param);
}

static void w2_value(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page)  / 2;
    int cy = rect_top(page)  + rect_height(page) / 2;
    int d  = 100, x = cx - d / 2, y = cy - d / 2;

    jlgpu_task_fill_param_init(5, 4, 0x30, 0xff, 0xff, 0xff);
    jlgpu_task_fill_set_draw(x, y, d, d);
    jlgpu_task_fill_set_area(rect_left(page), rect_top(page),
                             rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 5, 4, &task_param);
}

pJLGPUChainHead_t draw_page_2(pJLGPUChainHead_t head, struct rect *screen)
{
    struct rect page = {rect_left(screen), rect_top(screen), PAGE_W, PAGE_H};
    w2_bg(head, &page);
    w2_ring1(head, &page);
    w2_ring2(head, &page);
    w2_ring3(head, &page);
    w2_value(head, &page);
    return head;
}


/* ═══════════════════════════════════════════════════════════════════════
 *  页面3 "媒体卡": 5 控件
 *    bg(深紫径向渐变) → cover(纹理居中220) → bar_bg(进度底轨) → bar_hi(渐变进度72%)
 *    → ctrl(底部操作按钮)
 * ═══════════════════════════════════════════════════════════════════════ */

static void w3_bg(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page) / 2;
    int cy = rect_top(page) + rect_height(page) / 2;
    static u32 clut[] = {
        ARGB8888(0xff, 0x18, 0x0e, 0x22),
        ARGB8888(0xff, 0x22, 0x16, 0x32),
        ARGB8888(0xff, 0x0c, 0x06, 0x16),
    };

    jlgpu_task_gradient_param_init(1, 0);
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);
    jlgpu_task_gradient_set_affine_translate(rect_left(page), rect_top(page));
    jlgpu_task_gradient_set_radial(cx, cy - 40, 0, rect_width(page) / 2);
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);
    jlgpu_task_gradient_set_draw(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_task_gradient_set_area(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 1, 0, &task_param);
}

static void w3_cover(pJLGPUChainHead_t head, struct rect *page)
{
    int cx = rect_left(page) + rect_width(page) / 2;
    int cy = rect_top(page) + 155;
    int d  = 220, x = cx - d / 2, y = cy - d / 2;

    jlgpu_task_texture_param_init(2, 1);
    jlgpu_task_img_set_format(img_data_get_format(IMG_TEST));
    jlgpu_task_img_set_size(img_data_get_width(IMG_TEST),
                            img_data_get_height(IMG_TEST));
    jlgpu_task_img_set_data(img_data_get_addr(IMG_TEST));
    jlgpu_task_img_set_endian(img_data_get_endian(IMG_TEST));
    jlgpu_task_img_set_compress(img_data_get_compress_mode(IMG_TEST));
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(IMG_TEST));
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(IMG_TEST));
    jlgpu_task_img_set_draw(x, y, d, d);
    jlgpu_task_img_set_area(rect_left(page), rect_top(page),
                            rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 2, 1, &task_param);
}

static void w3_bar_bg(pJLGPUChainHead_t head, struct rect *page)
{
    int h = 6;
    int y = rect_top(page) + 330;
    int x = rect_left(page) + 80;

    jlgpu_task_fill_param_init(3, 2, 0x40, 0xff, 0xff, 0xff);
    jlgpu_task_fill_set_draw(x, y, rect_width(page) - 160, h);
    jlgpu_task_fill_set_area(rect_left(page), rect_top(page),
                             rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 3, 2, &task_param);
}

static void w3_bar_hi(pJLGPUChainHead_t head, struct rect *page)
{
    int h  = 6;
    int bw = (rect_width(page) - 160) * 72 / 100;
    int y  = rect_top(page) + 330;
    int x  = rect_left(page) + 80;
    u32 clut[] = {
        ARGB8888(0xff, 0xff, 0x40, 0x60),
        ARGB8888(0xff, 0xff, 0x80, 0xa0),
    };

    jlgpu_task_gradient_param_init(4, 3);
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_gradient_set_clut(clut, 0, JLGPU_CLUT_FORMAT_ARGB8888);
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);
    jlgpu_task_gradient_set_affine_translate(x, y);
    jlgpu_task_gradient_set_linear(x, y, x + bw, y);
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);
    jlgpu_task_gradient_set_draw(x, y, bw, h);
    jlgpu_task_gradient_set_area(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 4, 3, &task_param);
}

static void w3_ctrl(pJLGPUChainHead_t head, struct rect *page)
{
    int h  = 36;
    int cx = rect_left(page) + rect_width(page) / 2;
    int y  = rect_top(page) + rect_height(page) - h - 40;
    int w  = 200;
    u32 clut[] = {
        ARGB8888(0xff, 0x30, 0x20, 0x50),
        ARGB8888(0xff, 0x50, 0x30, 0x80),
    };

    jlgpu_task_gradient_param_init(5, 4);
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_gradient_set_clut(clut, 0, JLGPU_CLUT_FORMAT_ARGB8888);
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);
    jlgpu_task_gradient_set_affine_translate(cx - w / 2, y);
    jlgpu_task_gradient_set_linear(cx - w / 2, y, cx - w / 2, y + h);
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);
    jlgpu_task_gradient_set_draw(cx - w / 2, y, w, h);
    jlgpu_task_gradient_set_area(rect_left(page), rect_top(page),
                                 rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 5, 4, &task_param);
}

pJLGPUChainHead_t draw_page_3(pJLGPUChainHead_t head, struct rect *screen)
{
    struct rect page = {rect_left(screen), rect_top(screen), PAGE_W, PAGE_H};
    w3_bg(head, &page);
    w3_cover(head, &page);
    w3_bar_bg(head, &page);
    w3_bar_hi(head, &page);
    w3_ctrl(head, &page);
    return head;
}


/* ═══════════════════════════════════════════════════════════════════════
 *  页面4 "球面图标阵": 38 控件 (1 bg + 37 texture, 使用JLUI资源图片)
 *    37个纹理图标按同心圆布局，4圈：1 + 6 + 12 + 18
 *    从内到外图标通过 sphere projection + affine scale 模拟球面效果
 *    图片ID由静态数组 page4_img_ids[] 配置，不足37个时循环复用
 *
 *    纹理资源采用"边读边配置"模式: 每创建一个GPU任务时，
 *    从JLUI资源包中实时读取图片信息并配置纹理参数，
 *    压缩/寻址/Stride 等参数通过 res_get_image_stride 从资源中解析。
 * ═══════════════════════════════════════════════════════════════════════ */
#include "result_pic_index.h"
#define SPHERE_R          ((float)(PAGE_W) / 2.0f)
#define ICON_BASE         50
#define JLUI_RES_PATH     "storage/res_nor_mode/C/JL/JL.res"
#define JLUI_RES_BASE     0xa200000
#define PAGE4_ICON_COUNT  37

/* ── 图片ID静态数组: 修改此处即可更换图标 ── */
static const uint32_t page4_img_ids[] = {
    PAGE4_ec53_MENU_1CLOCK,
    PAGE4_16e3_MENU_4MUSIC,
    PAGE4_bd70_MENU_2TELEPHONE,
    PAGE4_d68f_MENU_6WEATHER,
    PAGE4_5ddc_MENU_22COMPASS,
    PAGE4_50f5_MENU_18CALENDAR,
    PAGE4_35f5_MENU_10HEART_RATE,
    PAGE4_68d3_MENU_11BLOOD_OXYGEN,
    PAGE4_ebf6_MENU_12BLOOD_PRESSURE,
    PAGE4_7bc1_MENU_20PRESSURE,
    PAGE4_5919_MENU_8OUTDOOR_SPORTS,
    PAGE4_fd1a_MENU_9INDOOR_SPORTS,
    PAGE4_7058_MENU_5ALARM_CLOCK,
    PAGE4_5dc3_MENU_27STOPWATCH,
    PAGE4_9b61_MENU_17CALCULATOR,
    PAGE4_f9e4_MENU_3INFORMATION,
    PAGE4_5813_MENU_28PHOTOGRAPH,
    PAGE4_b775_MENU_16VOICE_ASSISTANT,
    PAGE4_c9a3_MENU_29SET,
    PAGE4_e9f7_MENU_13BREATHING,
};
#define PAGE4_IMG_ID_NUM  (sizeof(page4_img_ids) / sizeof(page4_img_ids[0]))
extern int jlui_res_get_image_info(u32 prj_id, u32 img_id, UI_RESFILE *specfile, struct image_file *image, struct flash_file_info *info);
extern int __ui_res_flash_info_get(struct flash_file_info *fi, char *path, char *str, u8 dev, u32 addr, int mmu_tab);

/* ── helper: 创建带仿射缩放的JLUI纹理图标任务
 *   边读取资源信息边配置GPU任务，compress/stride/adr_mode 等参数从资源解析。
 *   每个调用独立 task_param，可在循环中反复调用。 ── */
static void w4_icon(pJLGPUChainHead_t head, int tid, int eid,
                    int ax, int ay, int aw, int ah,
                    int cx, int cy, float sphere_scale,
                    UI_RESFILE *fp,
                    struct flash_file_info *file_info,
                    uint32_t img_id)
{
    struct image_file image = {0};
    struct flash_file_info info = {0};
    uint8_t adr, cmp;
    int cmp_size, stride, gpu_fmt, base, x, y, dx, dy;

    /* ── ① 从 JLUI 资源中读取图片信息 ── */
    if (jlui_res_get_image_info(0, img_id, fp, &image, &info)) {
        return;
    }

    gpu_fmt = res_format_to_gpu(image.format);
    if (GPU_IMG_FMT_AX(gpu_fmt) || GPU_IMG_FMT_LX(gpu_fmt) ||
        GPU_IMG_FMT_ALXX(gpu_fmt)) {
        int tab_size = get_clut_format_tabsize(image.format, image.clut_format);
        image.offset += tab_size;
        image.len    -= tab_size;
    }
    ui_res_get_image_flash_info(&info, file_info, &image);

    /* ── ② 从资源中解析 stride / compress / adr_mode ── */
    stride = res_get_image_stride(gpu_fmt, &image, &adr, &cmp, &cmp_size);

    /* ── ③ 计算仿射变换参数 ── */
    base    = ICON_BASE;
    x       = cx - base / 2;
    y       = cy - base / 2;
    dx      = (int)((float)(base - (int)((float)base * sphere_scale)) / 2.0f);
    dy      = dx;

    /* ── ④ 配置 GPU 纹理任务 ── */
    jlgpu_task_texture_param_init(tid, eid);
    jlgpu_task_img_set_format(gpu_fmt);
    jlgpu_task_img_set_size(image.width, image.height);
    jlgpu_task_img_set_data(info.offset);
    jlgpu_task_img_set_endian(JLGPU_ENDIAN_LITTLE);
    jlgpu_task_img_set_alpha_end(JLGPU_ALPHA_END_ARGB);
    jlgpu_task_img_set_stride(stride);
    jlgpu_task_img_set_compress(cmp);
    jlgpu_task_img_set_compress_size(cmp_size);
    jlgpu_task_img_set_adr_mode(adr);

    /* 仿射变换: GPU 硬件完成缩放 + 居中平移 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    jlgpu_task_img_set_affine_scale(sphere_scale, sphere_scale);
    jlgpu_task_img_set_affine_translate(x + dx, y + dy);

    jlgpu_task_img_set_draw(x, y, base, base);
    jlgpu_task_img_set_area(ax, ay, aw, ah);
    jlgpu_update_task_by_id(head, tid, eid, &task_param);
}

/* ── widget: 深色背景 ── */
static void w4_bg(pJLGPUChainHead_t head, struct rect *page)
{
    jlgpu_task_fill_param_init(0, 0, 0xff, 0x0c, 0x10, 0x18);
    jlgpu_task_fill_set_draw(rect_left(page), rect_top(page),
                             rect_width(page), rect_height(page));
    jlgpu_task_fill_set_area(rect_left(page), rect_top(page),
                             rect_width(page), rect_height(page));
    jlgpu_update_task_by_id(head, 0, 0, &task_param);
}

/* ── 页面4入口 ── */
pJLGPUChainHead_t draw_page_4(pJLGPUChainHead_t head, struct rect *screen)
{
    struct rect page = {rect_left(screen), rect_top(screen), PAGE_W, PAGE_H};
    int cx = rect_left(&page) + rect_width(&page) / 2;
    int cy = rect_top(&page) + rect_height(&page) / 2;
    int px = rect_left(&page);
    int py = rect_top(&page);
    int pw = rect_width(&page);
    int ph = rect_height(&page);
    int i, ri, tid = 0;

    struct {
        int count;
        int r;
    } rings[] = {
        { 1,   0 },
        { 6,  70 },
        {12, 130 },
        {18, 190 },
    };

    /* ── 打开 JLUI 资源文件 ── */
    UI_RESFILE *fp = res_fopen(JLUI_RES_PATH, "r");
    struct flash_file_info file_info = {0};

    if (fp) {
        __ui_res_flash_info_get(&file_info, (char *)JLUI_RES_PATH,
                                "res", PHY_JL_EXTERN_FLASH,
                                JLUI_RES_BASE, false);
    }

    /* ── 创建控件 ── */
    w4_bg(head, &page);
    tid = 1;

    for (ri = 0; ri < 4; ri++) {
        int count = rings[ri].count;
        int r     = rings[ri].r;
        float scale, z;

        if (r == 0) {
            scale = 1.0f;
        } else {
            z     = sqrtf(SPHERE_R * SPHERE_R - (float)(r * r));
            scale = z / SPHERE_R;
        }
        if (scale < 0.24f) {
            scale = 0.24f;
        }

        for (i = 0; i < count; i++) {
            float angle = (float)i * 360.0f / (float)count;
            float rad   = angle * 3.14159265f / 180.0f;
            int ix = cx + (int)((float)r * cosf(rad));
            int iy = cy + (int)((float)r * sinf(rad));
            uint32_t img_id = page4_img_ids[(tid - 1) % PAGE4_IMG_ID_NUM];

            w4_icon(head, tid, tid,
                    px, py, pw, ph, ix, iy, scale,
                    fp, &file_info, img_id);
            tid++;
        }
    }

    if (fp) {
        res_fclose(fp);
    }
    return head;
}

#endif /* GPU_DEMO_ASYN_DRAW */
