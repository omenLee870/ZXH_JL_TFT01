#ifndef LV_DRAW_JLGPU_H
#define LV_DRAW_JLGPU_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"
#include "asm/eva_sfr.h"



#define BOOL_DEFINE_CONFLICT

#include "../../display/lv_display_private.h"
#include "../lv_image_decoder_private.h"
#include "../sw/lv_draw_sw.h"
#include "../lv_draw.h"
#include "../../misc/lv_types.h"
#include "src/draw/lv_draw_buf_private.h"
#include "src/draw/lv_draw_private.h"
#include "src/draw/sw/lv_draw_sw_private.h"
/*#include "../../../libs/jl_bin/lv_jl_bin.h"*/

#include "lv_draw_jlgpu_common.h"
#include "lv_draw_jlgpu_vec.h"

/*#include "asm/gpu/jlgpu.h"*/
/*#include "asm/jlgpu_driver.h"*/

/*********************
 *      DEFINES
 *********************/
/*#define JL_GPU2D ((JLGPU_SFR_TypeDef *)EVA_GPU)*/

/*#define MATH_SINF(x) sinf(x)*/
/*#define MATH_COSF(x) cosf(x)*/
/*#define MATH_SQRTF(x) sqrtf(x)*/

#define MATH_PI  3.14159265358979323846f
#define MATH_HALF_PI 1.57079632679489661923f
#define MATH_TWO_PI 6.28318530717958647692f
#define DEG_TO_RAD 0.017453292519943295769236907684886f
#define RAD_TO_DEG 57.295779513082320876798154814105f

#define MATH_TANF(x) tanf(x)
#define MATH_SINF(x) sinf(x)
#define MATH_COSF(x) cosf(x)
#define MATH_ASINF(x) asinf(x)
#define MATH_ACOSF(x) acosf(x)
#define MATH_FABSF(x) fabsf(x)
#define MATH_SQRTF(x) sqrtf(x)

#define MATH_RADIANS(deg) ((deg) * DEG_TO_RAD)
#define MATH_DEGREES(rad) ((rad) * RAD_TO_DEG)

#define CLAMP_TO_ZERO(x) (((x) < 0) ? 0 : (x))

#if LV_COLOR_DEPTH == 8
#error "Can't use 2.5D GPU with LV_COLOR_DEPTH == 8"
#endif

#if LV_COLOR_DEPTH == 16
#define LV_GPU_COLOR_FORMAT JLGPU_FORMAT_RGB565
#elif LV_COLOR_DEPTH == 24
#define LV_GPU_COLOR_FORMAT JLGPU_FORMAT_RGB888
#elif LV_COLOR_DEPTH == 32
#define LV_GPU_COLOR_FORMAT JLGPU_FORMAT_ARGB8888
#else
/*Can't use GPU with other formats*/
#endif

#define LV_JL_GPU_MIN_AREA_SIZE 0    // 交付给 GPU 渲染的最小区域大小，小于这个大小则不采用 GPU，脏块太小的渲染动作反而影响整体的渲染速度

/*#define jlgpu_DRAW_FULL_FB_ENABLE 0      // 使用小窗绘制速度会更快，默认关闭这个宏*/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct _lv_jlgpu_global_sfr {
    jlgpu_surface_t surface;
    jlgpu_global_matrix_t global_matrix;
    jlgpu_colorkey_t colorkey;
    jlgpu_global_path_t global_path;
    jlgpu_uv_adr_t uv_adr;
} lv_jlgpu_global_sfr_t;

typedef struct _lv_jlgpu_ctx_t {
    void *ctx;
    uint8_t *ctx_buf;
    void *task_base;//gpu任务基地址
    void *cur_task;//gpu任务基地址
    jlgpu_ops_t jlgpu_ops;
    uint8_t jlgpu_task_num;      //GPU任务链节点数
    /*lv_jlgpu_global_sfr_t global_sfr;*/
} lv_jlgpu_ctx_t;

#if LV_USE_DRAW_JLGPU_LIST
//用于管理串链表后的附加buf
typedef struct jlgpu_attach_buf jlgpu_attach_buf_t;
struct jlgpu_attach_buf {
    jlgpu_attach_buf_t *next;  // 指向下一个节点
    uint8_t *data;      // 分配的内存
};
#endif


typedef struct _lv_draw_jlgpu_unit_t {
    lv_draw_unit_t base_unit;
    lv_draw_task_t *task_act;
    //lv_draw_sw_thread_dsc_t thread_dscs[LV_DRAW_SW_DRAW_UNIT_CNT];
#if LV_USE_OS
    lv_thread_sync_t sync;
    lv_thread_t thread;
    volatile bool inited;
    volatile bool exit_status;
#endif

    //全局path内存
    jlgpu_vec_t *global_path;

//gpu链表参数
#if LV_USE_DRAW_JLGPU_LIST
    //gpu链表参数融入到unit中
    lv_jlgpu_ctx_t *jlgpu_ctx;
    jlgpu_attach_buf_t *jlgpu_attach_buf_head;
    uint32_t attach_buf_bytes;      // 分配的内存大小

    //gpu链表内存池
    uint8_t *pool_buf;
    uint16_t pool_size;
    uint16_t pool_pos;
#endif

} lv_draw_jlgpu_unit_t;

/**********************
 *  页面切换特效全局状态（多unit共享，避免swap导致的标志混乱）
 **********************/
typedef struct _lv_page_effect_state_t {
    //特效时序
    uint8_t effect_enter;
    uint8_t effect_exit;
    uint8_t effect_busy;

    //特效模式
    lv_screen_load_anim_t page_mode;

    int start_x;//触摸按下后的第一个位置
    int touch_x;//当前触摸位置
    int x_offset;

    int start_y;//触摸按下后的第一个位置(y)
    int touch_y;//当前触摸位置(y)
    int y_offset;

    //切换页面obj
    lv_obj_t *act_scr;
    lv_obj_t *new_scr;

    //页面切换动画控制
    lv_anim_t *a_old;
    lv_anim_t *a_new;

    //多页面链表管理
    lv_jlgpu_ctx_t *jlgpu_ctx_list[16];
    jlgpu_global_matrix_t *global_matrix[16];
    lv_area_t *draw_area[16];
    uint8_t draw_order[16];
    uint8_t end_page_index_id;
    uint8_t swipe_left;     //0=右滑(旧页在右), 1=左滑(旧页在左)
    uint8_t swipe_ready;    //0=未检测方向, 1=已检测, 防止首帧闪烁

} lv_page_effect_state_t;

extern lv_page_effect_state_t g_page_effect_state;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

#if LV_USE_DRAW_JLGPU

void lv_draw_jlgpu_init(void);

void lv_draw_jlgpu_deinit(void);

void _jlgpu_buf_invalidate_cache_cb(const lv_draw_buf_t *draw_buf, const lv_area_t *area);

#if (LV_USE_DRAW_JLGPU_FILL_ENABLE == 1)
void lv_draw_jlgpu_fill(lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords);
#endif

#if (LV_USE_DRAW_JLGPU_IMG_ENABLE == 1)
void lv_draw_jlgpu_img(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords);
#endif

#if (LV_USE_DRAW_JLGPU_LABEL_ENABLE == 1)
void lv_draw_jlgpu_label(lv_draw_task_t *t, const lv_draw_label_dsc_t *dsc, const lv_area_t *coords);
void lv_draw_jlgpu_label_init(lv_draw_jlgpu_unit_t *u);
#endif

#if (LV_USE_DRAW_JLGPU_ARC_ENABLE == 1)
void lv_draw_jlgpu_arc(lv_draw_task_t *t, const lv_draw_arc_dsc_t *dsc, const lv_area_t *coords);
#endif

#if (LV_USE_DRAW_JLGPU_BORDER_ENABLE == 1)
void lv_draw_jlgpu_border(lv_draw_task_t *t, const lv_draw_border_dsc_t *dsc, const lv_area_t *coords);
#endif

#if (LV_USE_DRAW_JLGPU_MASK_RECTANGLE_ENABLE == 1)
void lv_draw_jlgpu_mask_rect(lv_draw_task_t *t, const lv_draw_mask_rect_dsc_t *dsc, const lv_area_t *coords);
#endif

#if (LV_USE_DRAW_JLGPU_TRIANGLE_ENABLE == 1)
void lv_draw_jlgpu_triangle(lv_draw_task_t *t, const lv_draw_triangle_dsc_t *dsc);
#endif

#if (LV_USE_DRAW_JLGPU_LINE_ENABLE == 1)
void lv_draw_jlgpu_line(lv_draw_task_t *t, const lv_draw_line_dsc_t *dsc);
#endif

#if (LV_USE_DRAW_JLGPU_BOX_SHADOW_ENABLE == 1)
void lv_draw_jlgpu_box_shadow(lv_draw_task_t *t, const lv_draw_box_shadow_dsc_t *dsc, const lv_area_t *coords);
#endif


lv_jlgpu_ctx_t *lv_get_base_jlgpu_ctx(void);
void lv_jlgpu_ctx_init(lv_jlgpu_ctx_t *jlgpu_ctx);
void lv_jlgpu_ctx_uninit(lv_jlgpu_ctx_t *jlgpu_ctx);
void lv_jlgpu_reset_ctx_buf(lv_jlgpu_ctx_t *jlgpu_ctx);
void jlgpu_ctx_buffer_init(lv_jlgpu_ctx_t *jlgpu_ctx);

void jlgpu_hw_set_sfr_top(JLGPU_TOP_INFO_TypeDef *sfr_top);

bool lv_is_draw_buf(uint8_t *buf);
jlgpu_vec_t *lv_jlgpu_path_get(lv_draw_jlgpu_unit_t *unit);
lv_draw_jlgpu_unit_t *lv_get_draw_jlgpu_unit(void);
void lv_jlgpu_draw_buf_copy(lv_draw_buf_t *dest, const lv_area_t *dest_area, const lv_draw_buf_t *src, const lv_area_t *src_area, int use_gpu_list);
void lv_jlgpu_draw_buf_clear(lv_draw_buf_t *draw_buf, const lv_area_t *a);
void lv_jlgpu_clean_buf(lv_draw_jlgpu_unit_t *unit, uint8_t *draw_buf, uint8_t buf_format, const lv_area_t *a);
int lv_page_effect_manager(lv_display_t *disp_drv, lv_draw_jlgpu_unit_t *unit, const lv_area_t *screen_area, uint8_t *px_map);
void lv_page_effect_set_page_index(lv_screen_load_anim_t page_mode, uint32_t page_id, uint32_t page_index);

uint8_t lv_jlgpu_get_gxdma_bpp(uint8_t out_format);
#if LV_USE_DRAW_JLGPU_LIST
void lv_jlgpu_add_task_node(lv_draw_jlgpu_unit_t *unit, void *new_task);
uint8_t *lv_jlgpu_attach_buf_list_add(lv_draw_jlgpu_unit_t *unit, uint8_t *src_buf, uint32_t buf_size);
void lv_jlgpu_attach_buf_list_free(lv_draw_jlgpu_unit_t *unit);
void lv_jlgpu_task_list_free(lv_draw_jlgpu_unit_t *unit);
/*void lv_jlgpu_list_init(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint8_t *out_buf,uint8_t out_format);*/
void lv_jlgpu_sfr_set(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint32_t bg_width, uint8_t *out_buf, uint8_t out_format);
void lv_jlgpu_sfr_set_surface(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint8_t *out_buf, uint8_t out_format);
void lv_jlgpu_sfr_top_set(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint16_t pingpong_buf_w, uint16_t pingpong_buf_h, uint8_t *pingpong_buf0, uint8_t *pingpong_buf1);
void lv_jlgpu_task_list_run(lv_draw_jlgpu_unit_t *unit);
void lv_jlgpu_gxdma_sfr_set(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint16_t in_stride, uint8_t *out_buf, uint8_t in_format, uint8_t out_format, uint8_t out_jstc_en, uint8_t out_rotate);
void lv_jlgpu_list_uninit(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint8_t *out_buf, uint8_t out_format);
void lv_jlgpu_mem_pool_init(lv_draw_jlgpu_unit_t *unit, uint8_t *pool_buf, uint16_t buf_size);
void lv_jlgpu_mem_pool_uninit(lv_draw_jlgpu_unit_t *unit);
uint8_t *lv_jlgpu_mem_pool_get_buf(lv_draw_jlgpu_unit_t *unit, uint8_t *src_buf, uint16_t buf_size);
void lv_jlgpu_mem_pool_clean(lv_draw_jlgpu_unit_t *unit);
void refr_area_test(const lv_area_t *area_p, int32_t y_offset);
void lv_jlgpu_global_path_init(void);
void lv_jlgpu_global_path_uninit(void);
void lv_jlgpu_global_path_reset(void);

/*void lv_jlgpu_set_global_trans_param(lv_area_t *screen_window, uint32_t touch_x ,uint8_t right_left,lv_page_move_mode_t page_mode);*/
void lv_jlgpu_reset_global_trans_param(void);

void jlgpu_hw_sfr_register_dump(void);
#endif

int lv_jlgpu_pnd_sem_clr(void);
void lv_jlgpu_mutex_pend(void);
void lv_jlgpu_mutex_post(void);

#if LV_USE_DRAW_JLGPU_LIST == 2
void lv_draw_jlgpu_unit_swap(void);
#endif

/**********************
 *      MACROS
 **********************/
#endif /*LV_USE_DRAW_JLGPU*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_JLGPU_H*/
