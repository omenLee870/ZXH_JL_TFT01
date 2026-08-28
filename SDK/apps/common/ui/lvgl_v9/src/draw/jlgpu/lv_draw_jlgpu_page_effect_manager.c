/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "lv_draw_jlgpu.h"
#include "display/lv_display.h"
#include "jieli/extra/libs/jl_hexagon/lv_hexagon.h"
#include "lv_draw_jlgpu_page_effect_hexagon.h"
#include "lv_draw_jlgpu_page_effect_hexagon_reflection.h"
#include "jieli/extra/libs/cube/lv_cube.h"
#include "lv_draw_jlgpu_page_effect_cube.h"
#include "lv_draw_jlgpu_page_effect_ops.h"
#include "lv_page_flip.h"
#include "lv_page_scale.h"
#include "lv_page_edge_flip.h"
#include "lv_page_cube_flip.h"
#include "lv_port_disp.h"
#include "jieli/common/gui/scr/gui_scr.h"
#include "app_config.h"
#include "gxdma_driver.h"

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if LV_USE_DRAW_JLGPU_LIST

/*********************
 *      DEFINES
 *********************/
#define LV_PAGE_CLICK_SWIPE_THRESHOLD  10  /* 滑动/点击区分阈值(像素)，超过此距离视为滑动 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern lv_obj_t *ui_get_setup_scr_obj(int32_t scr_id);
extern void get_touch_x_y_status(int32_t *x, int32_t *y, uint8_t *status);
extern void jlgpu_set_page_effect_x_anim(void *obj, int32_t v);
extern int32_t lv_port_get_first_touch_x(void);
extern void dbi_link_gpu(u8 en);
/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_layer_t static_layer = {0};
static lv_timer_t *static_timer = NULL;
lv_jlgpu_ctx_t *jlgpu_ctx_list_ori = NULL;

_WEAK_ lv_obj_t *ui_get_setup_scr_obj(int32_t scr_id)
{
    return NULL;
}

int lv_page_effect_get_page_num(lv_screen_load_anim_t page_mode)
{
    switch (page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
    case LV_SCR_LOAD_ANIM_HEXAGON:
        return POLYTOPE_CONFIG;
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
        return POLYTOPE_CONFIG * 2;
    case LV_SCR_LOAD_ANIM_LRB_MIRROR:
        return 4;
    default:
        return 2;
    }
}

bool lv_page_effect_need_sticky(lv_screen_load_anim_t page_mode)
{
    if (page_mode == LV_SCR_LOAD_ANIM_CUBE ||
        page_mode == LV_SCR_LOAD_ANIM_HEXAGON ||
        page_mode == LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION ||
        page_mode == LV_SCR_LOAD_ANIM_LRB_MIRROR) {
        return true;
    } else {
        return false;
    }
}

/** Select the ops table for the current sticky 3D effect.
 *  Extend this switch when adding new effects. */
static const sticky_effect_ops_t *sticky_effect_get_ops(void)
{
    switch (g_page_effect_state.page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
        return lv_cube_effect_get_ops();
    case LV_SCR_LOAD_ANIM_HEXAGON:
        return lv_hexagon_effect_get_ops();
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
        return lv_hexagon_reflection_effect_get_ops();
    default:
        return NULL;
    }
}

void lv_page_effect_set_page_index(lv_screen_load_anim_t page_mode,
                                   uint32_t page_id, uint32_t page_index)
{
    switch (page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
        lv_cube_effect_set_page_index(page_id, page_index);
        break;
    case LV_SCR_LOAD_ANIM_HEXAGON:
        lv_hexagon_effect_set_page_index(page_id, page_index);
        break;
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
        lv_hexagon_reflection_effect_set_page_index(page_id, page_index);
        break;
    default:
        break;
    }
}

/* 返回初始正对相机的面索引，不同模型的正面不同 */
static int lv_page_effect_get_front_index(void)
{
    switch (g_page_effect_state.page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
        return 3;  /* cube: 实测 face 3 初始朝向相机 */
    case LV_SCR_LOAD_ANIM_HEXAGON:
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
    default:
        return 0;  /* hexagon: 面0初始角度0°正对+Z */
    }
}

/* 将当前页面移到正面（面向用户），下次触发多页面特效时当前页始终面向用户。
 * 在 effect_enter 阶段、init() 之前调用。 */
static void lv_page_effect_make_front(void)
{
    const sticky_effect_ops_t *ops = sticky_effect_get_ops();
    if (!ops || !g_page_effect_state.act_scr) {
        return;
    }

    // 反查当前页 page_id
    gui_scr_t *scr = gui_scr_get_by_obj(g_page_effect_state.act_scr);
    int32_t target_id = gui_scr_get_id(scr);

    if (target_id < 0) {
        return;
    }

    uint32_t buf[16];
    ops->get_page_id(buf);

    // page_id 数组的实际长度：reflection 模式是 POLYTOPE_CONFIG（不含倒影面），
    // 其他模式与 page_num 一致
    int page_num = lv_page_effect_get_page_num(g_page_effect_state.page_mode);
    int id_count = (g_page_effect_state.page_mode == LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION)
                   ? POLYTOPE_CONFIG : page_num;
    int front_idx = lv_page_effect_get_front_index();

    if (front_idx < 0 || front_idx >= id_count) {
        front_idx = 0;
    }

    // 查找 target_id 是否已在数组中
    int found = -1;
    for (int i = 0; i < id_count; i++) {
        if ((int32_t)buf[i] == target_id) {
            found = i;
            break;
        }
    }

    if (found == front_idx) {
        // 已经在正面，不处理
    } else if (found >= 0) {
        // 已在数组中其他位置：swap(front_idx, found)
        uint32_t tmp = buf[front_idx];
        buf[front_idx] = buf[found];
        buf[found] = tmp;
    } else {
        // 不在数组中：右移一位，插入 front_idx（最后一位被挤出）
        for (int i = id_count - 1; i > front_idx; i--) {
            buf[i] = buf[i - 1];
        }
        buf[front_idx] = (uint32_t)target_id;
    }

    // 写回
    for (int i = 0; i < id_count; i++) {
        lv_page_effect_set_page_index(g_page_effect_state.page_mode, buf[i], i);
    }
}

uint8_t *lv_jlgpu_get_offset_buf(uint8_t *px_map, const lv_area_t *screen_window, lv_area_t *draw_area)
{
    uint32_t w = lv_area_get_width(screen_window);
    uint32_t y = draw_area->y1;//h-1
    uint32_t x = draw_area->x1;
    uint32_t bpp = 2;

    uint8_t *px_map_2 = px_map + (w * y + x) * bpp;
    return px_map_2;
}

//页面特效绘制
void lv_jlgpu_set_global_trans_param(lv_draw_jlgpu_unit_t *unit, const lv_area_t *screen_window, int32_t touch_x, lv_screen_load_anim_t page_mode, uint8_t *px_map)
{
    uint8_t gpu_link_dbi = lv_jl_lcd_get_need_gpu_link_dbi();
    if (gpu_link_dbi) {
        //关闭gpu联动dbi
        jlgpu_hw_set_sfr_top(NULL);
        lv_jlgpu_pnd_sem_clr();
        dbi_link_gpu(0);
    }

    uint8_t need_gxdma =  lv_jl_lcd_get_need_gxdma();
    if (need_gxdma) {
        /* 每帧重置 GPU 硬件状态, 保持 ctx 内部干净 */
        JLGXDMA_link_gpu(0);
        jlgpu_hw_set_sfr_top(NULL);
        lv_jlgpu_pnd_sem_clr();
        JLGXDMA_init();
    }

    //页面切换逻辑
    //2.特效绘制
    uint32_t fb_width = lv_area_get_width(screen_window);

    // 左右滑动方向适配：RTL 时镜像 touch_x 并交换新旧 ctx
    int32_t eff_touch_x = touch_x;
    lv_jlgpu_ctx_t *eff_ctx_old = g_page_effect_state.jlgpu_ctx_list[0];
    lv_jlgpu_ctx_t *eff_ctx_new = g_page_effect_state.jlgpu_ctx_list[1];
    if (g_page_effect_state.swipe_left) {
        eff_touch_x = (int32_t)fb_width - touch_x;
        eff_ctx_old = g_page_effect_state.jlgpu_ctx_list[1];
        eff_ctx_new = g_page_effect_state.jlgpu_ctx_list[0];
    }

    switch (page_mode) {
    case LV_SCR_LOAD_ANIM_DEFAULT:
        // eff_touch_x 语义：0=全旧页, fb_width=全新页
        // 左区域 [0, eff_touch_x]=eff_ctx_new（新页面），右区域 [eff_touch_x, fb_width]=eff_ctx_old（旧页面）
        // RTL 时方向适配已在入口完成（eff_touch_x 镜像 + ctx 交换）
        if (!g_page_effect_state.swipe_ready) {
            //方向未确定：只画旧页面全屏，避免首帧闪烁
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        } else {
            int32_t div = eff_touch_x;
            if (div < 0) {
                div = 0;
            }
            if (div > (int32_t)fb_width) {
                div = fb_width;
            }

            //绘制左边区域（新页面）
            unit->jlgpu_ctx = eff_ctx_new;
            {
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = div;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                if (draw_area.x1 <= draw_area.x2) {
                    lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                }
            }

            //绘制右边区域（旧页面）
            unit->jlgpu_ctx = eff_ctx_old;
            {
                lv_area_t draw_area;
                draw_area.x1 = div;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                if (draw_area.x1 <= draw_area.x2) {
                    lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                }
            }
        }
        break;
    case LV_SCR_LOAD_ANIM_FLIP:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                //方向未确定：只画旧页面全屏
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                float degree = (float)eff_touch_x * 180.0f / (float)fb_width;

                /* Avoid exactly 90 degrees (degenerate case) */
                if (degree > 89.999f && degree < 90.001f) {
                    degree = 90.001f;
                }

                int fb_height = lv_area_get_height(screen_window);
                int affine_width = lv_page_flip_get_affine_width(fb_width, fb_height, degree);
                int half_w = fb_width / 2;
                int mid_left, mid_right;
                lv_area_t draw_left, draw_mid, draw_right;

                /* Compute regions based on degree */
                if (degree < 90.0f) {
                    /* Fold is on the right side of center */
                    mid_left  = half_w - (affine_width / 2);
                    mid_right = half_w;

                    draw_left.x1 = screen_window->x1;
                    draw_left.x2 = mid_left - 1;
                    draw_left.y1 = screen_window->y1;
                    draw_left.y2 = screen_window->y2;

                    draw_mid.x1 = mid_left;
                    draw_mid.x2 = mid_right;
                    draw_mid.y1 = screen_window->y1;
                    draw_mid.y2 = screen_window->y2;

                    draw_right.x1 = half_w;
                    draw_right.x2 = screen_window->x2;
                    draw_right.y1 = screen_window->y1;
                    draw_right.y2 = screen_window->y2;

                    /* Left: new page (static) */
                    if (draw_left.x1 <= draw_left.x2) {
                        unit->jlgpu_ctx = eff_ctx_new;
                        lv_jlgpu_sfr_set(unit, &draw_left, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_left),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }

                    /* Middle: old page flipped (perspective warp) */
                    if (draw_mid.x1 <= draw_mid.x2) {
                        jlgpu_matrix_t flip_fw, flip_bw;
                        jlgpu_global_matrix_t gm = {0};
                        lv_page_flip_get_matrix(&flip_fw, &flip_bw, 180.0f - degree, fb_width, fb_height);
                        gm.trans_en = 1;
                        gm.fw_mat = flip_fw;
                        gm.bw_mat = flip_bw;

                        unit->jlgpu_ctx = eff_ctx_old;
                        jlgpu_set_global_trans_param(&gm);
                        lv_jlgpu_sfr_set(unit, &draw_mid, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_mid),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                        lv_jlgpu_reset_global_trans_param();
                    }

                    /* Right: old page (static) */
                    if (draw_right.x1 <= draw_right.x2) {
                        unit->jlgpu_ctx = eff_ctx_old;
                        lv_jlgpu_sfr_set(unit, &draw_right, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_right),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }
                } else {
                    /* degree > 90: fold passed center, back side of flip = new page */
                    mid_left  = half_w;
                    mid_right = half_w + (affine_width / 2);

                    draw_left.x1 = screen_window->x1;
                    draw_left.x2 = half_w - 1;
                    draw_left.y1 = screen_window->y1;
                    draw_left.y2 = screen_window->y2;

                    draw_mid.x1 = mid_left;
                    draw_mid.x2 = mid_right;
                    draw_mid.y1 = screen_window->y1;
                    draw_mid.y2 = screen_window->y2;

                    draw_right.x1 = mid_right + 1;
                    draw_right.x2 = screen_window->x2;
                    draw_right.y1 = screen_window->y1;
                    draw_right.y2 = screen_window->y2;

                    /* Left: new page (static) */
                    if (draw_left.x1 <= draw_left.x2) {
                        unit->jlgpu_ctx = eff_ctx_new;
                        lv_jlgpu_sfr_set(unit, &draw_left, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_left),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }

                    /* Middle: new page's other half (back side of flip) */
                    if (draw_mid.x1 <= draw_mid.x2) {
                        jlgpu_matrix_t flip_fw, flip_bw;
                        jlgpu_global_matrix_t gm = {0};
                        lv_page_flip_get_matrix(&flip_fw, &flip_bw, 180.0f - degree, fb_width, fb_height);
                        gm.trans_en = 1;
                        gm.fw_mat = flip_fw;
                        gm.bw_mat = flip_bw;

                        unit->jlgpu_ctx = eff_ctx_new;
                        jlgpu_set_global_trans_param(&gm);
                        lv_jlgpu_sfr_set(unit, &draw_mid, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_mid),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                        lv_jlgpu_reset_global_trans_param();
                    }

                    /* Right: old page (static) */
                    if (draw_right.x1 <= draw_right.x2) {
                        unit->jlgpu_ctx = eff_ctx_old;
                        lv_jlgpu_sfr_set(unit, &draw_right, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_right),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }
                }
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            /* Draw target page full screen on exit */
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_SCALE_DOUBLE:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                int fb_height = lv_area_get_height(screen_window);
                float tmp_scale = LV_PAGE_SCALE_DELTA;
                float old_scale = tmp_scale * (float)(fb_width - eff_touch_x) / (float)fb_width + LV_PAGE_SCALE_MIN;
                float new_scale = tmp_scale * (float)eff_touch_x / (float)fb_width + LV_PAGE_SCALE_MIN;
                jlgpu_global_matrix_t gm_old = {0};
                jlgpu_global_matrix_t gm_new = {0};
                jlgpu_matrix_t fw, bw;

                /* Old page: region [eff_touch_x, right], shrinks as it moves away */
                lv_area_t draw_old;
                draw_old.x1 = eff_touch_x;
                draw_old.x2 = screen_window->x2;
                draw_old.y1 = screen_window->y1;
                draw_old.y2 = screen_window->y2;
                if (draw_old.x1 <= draw_old.x2) {
                    lv_page_scale_get_matrix(&fw, &bw, old_scale, fb_width, fb_height);
                    gm_old.trans_en = 1;
                    gm_old.fw_mat = fw;
                    gm_old.bw_mat = bw;
                    unit->jlgpu_ctx = eff_ctx_old;
                    jlgpu_set_global_trans_param(&gm_old);
                    lv_jlgpu_sfr_set(unit, &draw_old, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_old),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }

                /* New page: region [0, eff_touch_x], grows as it enters */
                lv_area_t draw_new;
                draw_new.x1 = screen_window->x1;
                draw_new.x2 = eff_touch_x < screen_window->x2 ? eff_touch_x : screen_window->x2;
                draw_new.y1 = screen_window->y1;
                draw_new.y2 = screen_window->y2;
                if (draw_new.x1 <= draw_new.x2) {
                    lv_page_scale_get_matrix(&fw, &bw, new_scale, fb_width, fb_height);
                    gm_new.trans_en = 1;
                    gm_new.fw_mat = fw;
                    gm_new.bw_mat = bw;
                    unit->jlgpu_ctx = eff_ctx_new;
                    jlgpu_set_global_trans_param(&gm_new);
                    lv_jlgpu_sfr_set(unit, &draw_new, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_new),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_CENTER_FLIP:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {

                int fb_height = lv_area_get_height(screen_window);
                float degree = 180.0f - (float)eff_touch_x * 180.0f / (float)fb_width;
                jlgpu_global_matrix_t gm = {0};

                /* Avoid exactly 90 degrees */
                if (degree > 89.999f && degree < 90.001f) {
                    degree = 90.001f;
                }

                lv_page_flip_get_full_screen_matrix(&gm, degree, fb_width, fb_height);

                /* degree > 90: showing old page front, flip reveals new page behind */
                /* degree < 90: showing new page back, old page visible behind */
                lv_jlgpu_ctx_t *flip_ctx;
                lv_jlgpu_ctx_t *under_ctx;
                if (degree > 90.0f) {
                    flip_ctx = eff_ctx_old;   /* old page on top, flipping */
                    under_ctx = eff_ctx_new;  /* new page behind */
                } else {
                    flip_ctx = eff_ctx_new;   /* new page on top (back side) */
                    under_ctx = eff_ctx_old;  /* old page behind */
                }

                /* Draw underlay (static, full screen) */
                {
                    //背景清0
                    lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                }

                /* Draw flipped layer on top with perspective matrix */
                {
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    unit->jlgpu_ctx = flip_ctx;
                    jlgpu_set_global_trans_param(&gm);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                }
                lv_jlgpu_reset_global_trans_param();
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_EDGE_FLIP:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                int fb_height = lv_area_get_height(screen_window);
                float degree = (float)eff_touch_x * 45.0f / (float)fb_width;
                float angle = LV_EDGE_FLIP_ANGLE;
                jlgpu_global_matrix_t gm_old = {0};
                jlgpu_global_matrix_t gm_new = {0};

                /* Clean buffer (background) */
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);

                /* Both pages are drawn full screen with their own bottom-hinge flip matrix.
                 * face_flag=0: old page rotates from 0 to -angle (hinge at bottom)
                 * face_flag=1: new page rotates from +angle to 0 */

                /* Draw old page (current face, rotates away) */
                {
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    lv_page_edge_flip_get_full_screen_matrix(&gm_old, degree, fb_width, fb_height,
                            angle, 0);
                    unit->jlgpu_ctx = eff_ctx_old;
                    jlgpu_set_global_trans_param(&gm_old);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }

                /* Draw new page (next face, rotates into view) */
                {
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    lv_page_edge_flip_get_full_screen_matrix(&gm_new, degree, fb_width, fb_height,
                            angle, 1);
                    unit->jlgpu_ctx = eff_ctx_new;
                    jlgpu_set_global_trans_param(&gm_new);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_CUBE_FLIP:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                int fb_height = lv_area_get_height(screen_window);
                float degree = (float)(fb_width - eff_touch_x) * 90.0f / (float)fb_width;

                /* Avoid exactly 90 degrees (degenerate) */
                if (degree > 89.999f) {
                    degree = 89.999f;
                }

                /* Clean buffer background */
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);

                /* Draw old page (rotates away from viewer) */
                {
                    jlgpu_global_matrix_t gm = {0};
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    lv_page_cube_flip_get_full_screen_matrix(&gm, degree, fb_width, fb_height, 1);
                    unit->jlgpu_ctx = eff_ctx_old;
                    jlgpu_set_global_trans_param(&gm);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }

                /* Draw new page on top (rotates into view) */
                {
                    jlgpu_global_matrix_t gm = {0};
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    lv_page_cube_flip_get_full_screen_matrix(&gm, degree, fb_width, fb_height, 0);
                    unit->jlgpu_ctx = eff_ctx_new;
                    jlgpu_set_global_trans_param(&gm);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_HEXAGON:
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
    case LV_SCR_LOAD_ANIM_CUBE:
        if (g_page_effect_state.effect_busy) {
            lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
            /* sticky 特效：x_offset/y_offset 取反后传入 move，
             * 因为 hexagon/cube 的旋转方向与动画值方向相反。 */
            {
                int32_t eff_x_offset = -g_page_effect_state.x_offset;
                int32_t eff_y_offset = -g_page_effect_state.y_offset;
                sticky_effect_get_ops()->move(unit, eff_x_offset, eff_y_offset);
            }
            int page_num = lv_page_effect_get_page_num(g_page_effect_state.page_mode);
            for (int i = 0; i < page_num; i++) {
                uint8_t draw_order = g_page_effect_state.draw_order[i];
                if (draw_order >= page_num) {
                    continue; /* 背面：跳过不可见面 */
                }
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[draw_order];
                // clamp draw_area to screen bounds to avoid out-of-bounds framebuffer access
                lv_area_t da = *g_page_effect_state.draw_area[draw_order];
                if (da.x1 < screen_window->x1) {
                    da.x1 = screen_window->x1;
                }
                if (da.y1 < screen_window->y1) {
                    da.y1 = screen_window->y1;
                }
                if (da.x2 > screen_window->x2) {
                    da.x2 = screen_window->x2;
                }
                if (da.y2 > screen_window->y2) {
                    da.y2 = screen_window->y2;
                }
                if (da.x1 > da.x2 || da.y1 > da.y2) {
                    continue;
                }
                lv_jlgpu_sfr_set(unit, &da, fb_width, lv_jlgpu_get_offset_buf(px_map, screen_window, &da), JLGPU_FORMAT_RGB565);
                jlgpu_set_global_trans_param(g_page_effect_state.global_matrix[draw_order]);
                lv_jlgpu_task_list_run(unit);
            }
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width, lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area), JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    default:
        break;
    }

    if (gpu_link_dbi) {
        //打开gpu联动dbi
        dbi_link_gpu(1);
    }
}

//关闭特效
void lv_jlgpu_reset_global_trans_param(void)
{
    jlgpu_matrix_t matrix;
    jlgpu_matrix_set_identity(&matrix);

    jlgpu_global_matrix_t global_matrix = {0};
    global_matrix.trans_en = 0;
    global_matrix.fw_mat = matrix;//正向矩阵
    global_matrix.bw_mat = matrix;//逆向矩阵
    jlgpu_set_global_trans_param(&global_matrix);
}


static void layer_initialize(lv_display_t *disp_drv, lv_layer_t *temp_layer, const lv_area_t *area)
{
    lv_layer_init(temp_layer);
    temp_layer->parent = NULL;
    temp_layer->_clip_area = *area;
    temp_layer->buf_area = *area;
    temp_layer->phy_clip_area = *area;
    temp_layer->color_format = LV_COLOR_FORMAT_RGB565;

    //必须链到链表里
    if (disp_drv->layer_head) {
        lv_layer_t *tail = disp_drv->layer_head;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = temp_layer;
    } else {
        disp_drv->layer_head = temp_layer;
    }

    //借取显存的layerbuf,不额外申请buf;
    lv_layer_t *layer = disp_drv->layer_head;
    temp_layer->draw_buf = layer->draw_buf;
}

static void layer_remove_from_disp(lv_display_t *disp_drv, lv_layer_t *new_layer)
{
    if (!disp_drv || !new_layer || !disp_drv->layer_head) {
        return;
    }

    lv_layer_t *current = disp_drv->layer_head;
    lv_layer_t *previous = NULL;
    bool found = false;

    while (current != NULL) {
        if (current == new_layer) {
            found = true;
            break;
        }
        previous = current;
        current = current->next;
    }

    if (!found) {
        LV_LOG_INFO("Layer not found in list, skipping removal.");
        return;
    }

    if (previous == NULL) {
        disp_drv->layer_head = current->next;
    } else {
        previous->next = current->next;
    }

    LV_LOG_INFO("Layer removed and freed successfully.");
}

static void layer_set_empty_area(lv_layer_t *layer)
{
    //把new_layer的area清空，避免影响下一轮刷新
    lv_area_t empty_area = {0};
    layer->_clip_area = empty_area;
    layer->buf_area = empty_area;
    layer->phy_clip_area = empty_area;
}


static void lv_anim_set_current_value(lv_anim_t *a, int32_t current_value)
{
    uint32_t t = lv_map(current_value, a->start_value, a->end_value, 0, a->duration);
    a->act_time = t;
}
static void lv_anim_set_act_time(lv_anim_t *a, int32_t act_time)
{
    a->act_time = act_time;
}

//跟手定时器
static void timer_cb(lv_timer_t *timer)
{
    int32_t x;
    int32_t y;
    uint8_t status;
    static uint8_t first_down = 0;
    static uint8_t g_prev_touch_status = 0; //上一轮触摸状态
    static int32_t g_first_touch_x = 0; //记录定时器首次按下坐标(x)
    static int32_t g_first_touch_y = 0; //记录定时器首次按下坐标(y)

    get_touch_x_y_status(&x, &y, &status);
    //printf("-----x,y,status= %d,%d,%d\n",x,y,status);

    //针对sticky特效
    //检测定时器期间首次触摸按下，记录坐标
    if (status && !g_prev_touch_status) {
        g_first_touch_x = x;
        g_first_touch_y = y;
    }
    g_prev_touch_status = status;

    //特效期间触摸按下
    if (g_page_effect_state.effect_busy && status && g_page_effect_state.a_old) {
        //暂停动画
        if (!lv_anim_is_paused(g_page_effect_state.a_old)) {
            lv_anim_pause(g_page_effect_state.a_old);
            lv_anim_pause(g_page_effect_state.a_new);
        }

        //保留第一下按下的坐标，检测滑动方向
        if (!first_down) {
            g_page_effect_state.start_x = x;
            g_page_effect_state.start_y = y;
            first_down = 1;

            //根据首次触摸位置判断滑动方向
            lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
            int32_t fb_w = lv_display_get_horizontal_resolution(disp);
            int32_t first_touch = lv_port_get_first_touch_x();
            if (first_touch > fb_w / 2) {
                //触摸点在右半部分：从右往左滑（旧页在左，新页从右进入）
                g_page_effect_state.swipe_left = 1;
            } else {
                //触摸点在左半部分：从左往右滑（旧页在右，新页从左进入）
                g_page_effect_state.swipe_left = 0;
            }
            g_page_effect_state.swipe_ready = 1;

            /* sticky 特效：重置 touch_x/touch_y 避免动画→触控接管时 offset 跳变 */
            if (lv_page_effect_need_sticky(g_page_effect_state.page_mode)) {
                g_page_effect_state.touch_x = 0;
                g_page_effect_state.touch_y = 0;
            }
        }

        //触摸接管：使用手指移动距离（delta）作为分界位置
        {
            int32_t first_touch;
            int32_t delta_x;
            bool is_sticky = lv_page_effect_need_sticky(g_page_effect_state.page_mode);

            if (!is_sticky) {
                first_touch = lv_port_get_first_touch_x();
            } else {
                first_touch = g_first_touch_x;
            }

            if (is_sticky) {
                /* sticky 特效：使用原始位移（允许负值支持反向滑动）。
                 * g_first_touch_x - x：手指左移为正、右移为负，方向自带。 */
                delta_x = g_first_touch_x - x;
            } else if (g_page_effect_state.swipe_left) {
                //从右往左滑：手指左移时 delta 增大
                delta_x = first_touch - x;
            } else {
                //从左往右滑：手指右移时 delta 增大
                delta_x = x - first_touch;
            }

            if (!is_sticky && delta_x < 0) {
                delta_x = 0;
            }
            if (!is_sticky) {
                //获取屏幕宽度用于 clamp（仅非 sticky 需要）
                lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
                int32_t fb_w = lv_display_get_horizontal_resolution(disp);
                if (delta_x > fb_w) {
                    delta_x = fb_w;
                }
            }

            if ((g_page_effect_state.touch_x != delta_x)) {
                jlgpu_set_page_effect_x_anim(g_page_effect_state.act_scr, delta_x);
            }

            /* sticky 特效：同步计算 y_offset（与 x_offset 逻辑一致） */
            if (is_sticky) {
                int32_t delta_y = g_first_touch_y - y;
                g_page_effect_state.y_offset = delta_y - g_page_effect_state.touch_y;
                g_page_effect_state.touch_y = delta_y;
            }
        }

        //特效期间触摸抬起
    } else if (g_page_effect_state.effect_busy && !status && g_page_effect_state.a_old) {

        //两个页面的情况,交回给动画接管
        if (!lv_page_effect_need_sticky(g_page_effect_state.page_mode)) {
            //退出后需要把动画的current值更新上去
            if (lv_anim_is_paused(g_page_effect_state.a_old)) {

                int32_t first_touch = lv_port_get_first_touch_x();
                int32_t delta_x;
                if (g_page_effect_state.swipe_left) {
                    delta_x = first_touch - x;
                } else {
                    delta_x = x - first_touch;
                }
                if (delta_x < 0) {
                    delta_x = 0;
                }

                //获取屏幕宽度，判断分界线位置决定动画方向
                lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
                int32_t fb_width = lv_display_get_horizontal_resolution(disp);
                if (delta_x > fb_width) {
                    delta_x = fb_width;
                }

                if (delta_x <= fb_width / 2) {
                    //分界线在左半部分：取消切换，回弹到0（旧页面全屏）
                    lv_anim_set_values(g_page_effect_state.a_old, delta_x, 0);
                    lv_anim_set_current_value(g_page_effect_state.a_old, delta_x);

                    //目标页面改回旧页面
                    g_page_effect_state.end_page_index_id = 0;
                    g_page_effect_state.new_scr = g_page_effect_state.act_scr;
                } else {
                    //分界线在右半部分：继续切换，滑动到fb_width（新页面全屏）
                    lv_anim_set_values(g_page_effect_state.a_old, delta_x, fb_width);
                    lv_anim_set_current_value(g_page_effect_state.a_old, delta_x);

                    //目标页面保持为新页面
                    g_page_effect_state.end_page_index_id = 1;
                }
                lv_anim_set_act_time(g_page_effect_state.a_new, g_page_effect_state.a_old->act_time);
                lv_anim_resume(g_page_effect_state.a_old);
                lv_anim_resume(g_page_effect_state.a_new);

                //松手后不再进入
                lv_timer_pause(timer);
            } else {
                /* 快速滑动松手 / 纯动画（按键触发等）：
                 * a_old 未暂停，动画已在运行。用首次触摸位置判断方向。
                 * 纯动画场景下 lv_port_get_first_touch_x() 可能是旧值，
                 * 但不影响功能——动画回调已设默认方向作为兜底。 */
                lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
                int32_t fb_w = lv_display_get_horizontal_resolution(disp);
                int32_t first_touch = lv_port_get_first_touch_x();

                if (first_touch > fb_w / 2) {
                    g_page_effect_state.swipe_left = 1; // 右→左
                } else {
                    g_page_effect_state.swipe_left = 0; // 左→右
                }
                g_page_effect_state.swipe_ready = 1;
                lv_obj_invalidate(g_page_effect_state.act_scr);
            }
            //多页面情况
        } else {
            //抬起
            if (first_down) {
                // 计算累计滑动距离，用于区分滑动和点击
                int32_t first_touch = g_first_touch_x;//lv_port_get_first_touch_x();
                int32_t delta_x;
                if (g_page_effect_state.swipe_left) {
                    delta_x = first_touch - x;
                } else {
                    delta_x = x - first_touch;
                }
                if (delta_x < 0) {
                    delta_x = -delta_x;
                }

                if (delta_x <= LV_PAGE_CLICK_SWIPE_THRESHOLD) {
                    // 点击：根据触摸坐标确定目标页面
                    int scr_id = -1;
                    int face_index = -1;
                    const sticky_effect_ops_t *ops = sticky_effect_get_ops();
                    if (ops) {
                        scr_id = ops->get_face_scr_id(x, y);
                        face_index = ops->get_face_index(x, y);
                    }

                    if (lv_anim_is_paused(g_page_effect_state.a_old) && scr_id != -1) {
                        lv_anim_resume(g_page_effect_state.a_old);
                        lv_anim_resume(g_page_effect_state.a_new);

                        //松手后不再进入
                        lv_timer_pause(timer);

                        //修改目标页面
                        lv_obj_t *new_page_obj = ui_get_setup_scr_obj(scr_id);
                        g_page_effect_state.new_scr = new_page_obj;
                        g_page_effect_state.end_page_index_id = (uint8_t)face_index;
                    }
                }
                // 滑动（delta_x > 阈值）：不做任何操作，保持跟手状态
                // timer 继续运行，下次触摸可以继续滑动
            }
        }

        //清空标志
        first_down = 0;
        g_page_effect_state.start_x = x;
    }

}

void lv_page_effect_clean(void)
{
    int page_num = lv_page_effect_get_page_num(g_page_effect_state.page_mode);
    int ctx_count = page_num;
    bool shared_ctx = (g_page_effect_state.page_mode == LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION);

    // reflection 模式 ctx 后半段和前半段共享指针，只释放前半段
    if (shared_ctx) {
        ctx_count = POLYTOPE_CONFIG;
    }

    for (int i = 0; i < ctx_count; i++) {
        if (g_page_effect_state.jlgpu_ctx_list[i]) {
            lv_jlgpu_ctx_uninit(g_page_effect_state.jlgpu_ctx_list[i]);
            lv_free(g_page_effect_state.jlgpu_ctx_list[i]);
            g_page_effect_state.jlgpu_ctx_list[i] = NULL;
        }
    }
    // shared ctx 的后半段指针置空，避免悬空
    if (shared_ctx) {
        for (int i = ctx_count; i < page_num; i++) {
            g_page_effect_state.jlgpu_ctx_list[i] = NULL;
        }
    }

    for (int i = 0; i < page_num; i++) {
        if (g_page_effect_state.global_matrix[i]) {
            lv_free(g_page_effect_state.global_matrix[i]);
            g_page_effect_state.global_matrix[i] = NULL;
        }
        if (g_page_effect_state.draw_area[i]) {
            lv_free(g_page_effect_state.draw_area[i]);
            g_page_effect_state.draw_area[i] = NULL;
        }
    }
}

int lv_page_effect_manager(lv_display_t *disp_drv, lv_draw_jlgpu_unit_t *unit, const lv_area_t *screen_area, uint8_t *px_map)
{
    if (g_page_effect_state.page_mode >= LV_SCR_LOAD_ANIM_DEFAULT && g_page_effect_state.page_mode <= LV_SCR_LOAD_ANIM_USER) {
        //入场cb
        if (g_page_effect_state.effect_enter) {
            //固定环节
            g_page_effect_state.effect_enter = 0;
            lv_jlgpu_task_list_free(unit);
            jlgpu_ctx_list_ori = unit->jlgpu_ctx;

            //初始化layer,区域为全屏,配置为显存buf,用来收集链表
            layer_initialize(disp_drv, &static_layer, screen_area);

            int page_num = lv_page_effect_get_page_num(g_page_effect_state.page_mode);
            if (page_num == 2) { //双页面特效
                g_page_effect_state.end_page_index_id = 1; //默认切换到新页面
                g_page_effect_state.jlgpu_ctx_list[0] = lv_malloc_zeroed(sizeof(lv_jlgpu_ctx_t));
                g_page_effect_state.global_matrix[0] = lv_malloc_zeroed(sizeof(jlgpu_global_matrix_t));
                g_page_effect_state.draw_area[0] = lv_malloc_zeroed(sizeof(lv_area_t));
                lv_jlgpu_ctx_init(g_page_effect_state.jlgpu_ctx_list[0]);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_obj_redraw(&static_layer, g_page_effect_state.act_scr);//收集

                g_page_effect_state.jlgpu_ctx_list[1] = lv_malloc_zeroed(sizeof(lv_jlgpu_ctx_t));
                g_page_effect_state.global_matrix[1] = lv_malloc_zeroed(sizeof(jlgpu_global_matrix_t));
                g_page_effect_state.draw_area[1] = lv_malloc_zeroed(sizeof(lv_area_t));
                lv_jlgpu_ctx_init(g_page_effect_state.jlgpu_ctx_list[1]);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[1];
                lv_obj_redraw(&static_layer, g_page_effect_state.new_scr);//收集
            } else { //多页面特效
                lv_obj_t *page_obj;

                // 当前页面移到正面
                lv_page_effect_make_front();

                uint32_t page_id_arr[POLYTOPE_CONFIG];
                sticky_effect_get_ops()->get_page_id(page_id_arr);

                for (int i = 0; i < POLYTOPE_CONFIG; i++) {
                    page_obj = ui_get_setup_scr_obj(page_id_arr[i]);
                    g_page_effect_state.jlgpu_ctx_list[i] = lv_malloc_zeroed(sizeof(lv_jlgpu_ctx_t));
                    g_page_effect_state.global_matrix[i] = lv_malloc_zeroed(sizeof(jlgpu_global_matrix_t));
                    g_page_effect_state.draw_area[i] = lv_malloc_zeroed(sizeof(lv_area_t));
                    lv_jlgpu_ctx_init(g_page_effect_state.jlgpu_ctx_list[i]);
                    unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[i];
                    if (page_obj) {
                        lv_obj_redraw(&static_layer, page_obj);//收集任务
                    }
                    // reflection 模式：倒影面可以复用本体 ctx（仅矩阵和 draw_area 独立），
                    // 但需要单独分配 global_matrix 和 draw_area（每个面的投影矩阵不同）
                    if (g_page_effect_state.page_mode == LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION) {
                        int refl_idx = i + POLYTOPE_CONFIG;
                        g_page_effect_state.jlgpu_ctx_list[refl_idx] = g_page_effect_state.jlgpu_ctx_list[i]; // 指针复用
                        g_page_effect_state.global_matrix[refl_idx] = lv_malloc_zeroed(sizeof(jlgpu_global_matrix_t));
                        g_page_effect_state.draw_area[refl_idx] = lv_malloc_zeroed(sizeof(lv_area_t));
                    }
                }

                sticky_effect_get_ops()->init(unit, true);
            }

            unit->jlgpu_ctx = jlgpu_ctx_list_ori;
            //把new_layer的area清空，避免影响下一轮刷新
            layer_set_empty_area(&static_layer);
            g_page_effect_state.effect_busy = 1;
            //创建定时器
            if (static_timer == NULL) {
                static_timer = lv_timer_create(timer_cb, LV_DEF_REFR_PERIOD, NULL);
            }
        }//effect_enter

        if (g_page_effect_state.effect_busy) {
            lv_jlgpu_set_global_trans_param(unit, screen_area, g_page_effect_state.touch_x, g_page_effect_state.page_mode, px_map);
            lv_jlgpu_reset_global_trans_param();
            unit->jlgpu_ctx = jlgpu_ctx_list_ori;
            return 0;
        } //effect_busy

        //出场cb
        if (g_page_effect_state.effect_exit) {
            lv_jlgpu_set_global_trans_param(unit, screen_area, g_page_effect_state.touch_x, g_page_effect_state.page_mode, px_map);
            g_page_effect_state.effect_exit = 0;

            //清理内存
            lv_page_effect_clean();

            //temp_layer在链表中删除,无则忽略
            layer_remove_from_disp(disp_drv, &static_layer);

            //3d特效清理
            if (lv_page_effect_need_sticky(g_page_effect_state.page_mode)) {
                sticky_effect_get_ops()->uninit();
            }

            unit->jlgpu_ctx = jlgpu_ctx_list_ori;
            lv_jlgpu_task_list_free(unit);//所有链表的遗留内存都清理掉

            //标准矩阵
            lv_jlgpu_reset_global_trans_param();

            if (static_timer) {
                lv_timer_delete(static_timer);
                static_timer = NULL;
            }

            //清空所有标志
            jlgpu_ctx_list_ori = NULL;
            g_page_effect_state.touch_x = 0;
            g_page_effect_state.touch_y = 0;
            g_page_effect_state.x_offset = 0;
            g_page_effect_state.y_offset = 0;
            g_page_effect_state.start_x = 0;
            g_page_effect_state.start_y = 0;
            g_page_effect_state.swipe_left = 0;
            g_page_effect_state.swipe_ready = 0;
            g_page_effect_state.act_scr = NULL;
            g_page_effect_state.new_scr = NULL;
            g_page_effect_state.page_mode = LV_SCR_LOAD_ANIM_NONE;
            g_page_effect_state.end_page_index_id = 0;

            return 0;
        }//effect_exit

    }

    return 1;
}

#endif
#endif
#endif /* #if CONFIG_LVGL_UI_ENABLE */
