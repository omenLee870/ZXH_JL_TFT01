#include "app_config.h"
/* #include "app_task.h" */
#include "system/timer.h"
#include "device/device.h"
#include "key_event_deal.h"

#include "res/resfile.h"
#include "jlui/ui.h"
#include "jlui_app/ui_style.h"
#include "jlui_app/ui_api.h"
#include "jlui_app/res_config.h"
#include "jlui_app/ui_resource.h"
#include "jlui_app/ui_sys_param.h"
#include "jlui_app/ui_app_effect.h"
#include "jlui_effect.h"		//特效相关
#include "gpu_port.h"
#include "gpu_task.h"

#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI_EFFECT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_action_mense_manage.data.bss")
#pragma data_seg(".ui_action_mense_manage.data")
#pragma const_seg(".ui_action_mense_manage.text.const")
#pragma code_seg(".ui_action_mense_manage.text")
#endif

#if (defined CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
#if TCFG_UI_APP_EFFECT_ENABLE

#define DUMP_RECT(func, line, name, rect) \
	printf("[RECT] %s() %d, %s [%d, %d, %d, %d]\n", func, line, name, (rect)->left, (rect)->top, (rect)->width, (rect)->height)

#if CONFIG_LVGL_UI_ENABLE
#define GPU_GET_IMAGE_STRIDE(fmt, img, adr, cmp, cmp_size) \
    lvgl_res_get_image_stride(fmt, img, adr, cmp, cmp_size)
#else
#define GPU_GET_IMAGE_STRIDE(fmt, img, adr, cmp, cmp_size) \
    res_get_image_stride(fmt, img, adr, cmp, cmp_size)
#endif

/*************************************************************************************/
//					应用层实现ui特效的接口
/*************************************************************************************/

extern struct ui_platform_api *ui_get_platform_api();



struct page_move_preview {
    int page_id;
    int image_id;
    /* int mode;//可拓展 */
};
// 以下页面id优先使用图片作为页面预览
static const struct page_move_preview preview[] = {
    {ID_WINDOW_CALENDAR, PAGE66_b35e_CALENDAR},
    /* {ID_WINDOW_SLEEP, PAGE66_ebb4_SLEEP}, */
};
static int get_face_preview(int page_id)
{
    int i;
    for (i = 0; i < sizeof(preview) / sizeof(preview[0]); i++) {
        if (preview[i].page_id == page_id) {
            return preview[i].image_id;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_effect_set_alpha 修改控件透明度
 *
 * @param elm	控件句柄
 * @param alpha	透明度
 * @param child	是否同步到子控件
 * @param redraw	是否刷新
 */
/* ------------------------------------------------------------------------------------*/
void ui_effect_set_alpha(struct element *elm, int alpha, int child, int redraw)
{
    ASSERT(alpha >= 0);
    ASSERT(alpha <= 100);
    if (elm) {
        elm->css.alpha = alpha;
    }
    if (child) {
        struct element *child_elm;
        list_for_each_child_element(child_elm, elm) {
            ui_effect_set_alpha(child_elm, alpha, 1, 0);
        }
    }
    if (redraw) {
        ui_core_redraw(elm);
    }
}
struct ui_effect_module *ui_effect_get_handle_by_style(int style)
{
    struct ui_effect_module *p = NULL;
    for (p = (struct ui_effect_module *)ui_effect_module_begin; p < (struct ui_effect_module *)ui_effect_module_end; p++) {
        /* printf("%s search:%d find:%d",__func__,style,p->style); */
        if (p->style == style) {
            return p;
        }
    }
    return NULL;
}
int ui_ram_image_attrs_set(struct ui_image_attrs *img, u8 *data, int data_len, int width, int height, int format)
{
    img->width = width;
    img->height = height;
    img->has_clut = 0;
    img->compress = 0;
    img->data = data;
    img->len = data_len;
    img->gpu_format = GPU_FORMAT_ARGB8565;
    img->tab = NULL;
    img->clut_tab = NULL;
    return 0;
}


static void (*jlgpu_create_task_list_by_image_cb_func)(JLGPUTaskParam_t *param);
pJLGPUTaskHead_t jlgpu_create_task_list_by_image(pJLGPUTaskHead_t head, struct ui_image_attrs *image_attr, int xoffset, int yoffset)
{
    gpu_matrix_t matrix = {0};
    JLGPUTaskParam_t task_param = {0};
    task_param.task_type = GPU_TASK_IMAGE;

    gpu_matrix_set_identity(&matrix);
    gpu_matrix_translate(&matrix, -xoffset, -yoffset);

    //基础指令参数
    gpu_boundbox_t bbox;
    bbox.minx = 0;
    bbox.miny = 0;
    bbox.maxx = image_attr->width;
    bbox.maxy = image_attr->height;
    gpu_matrix_get_boundbox(&matrix, &bbox, &bbox);
    /* task_param.bbox.minx = bbox.minx; */
    /* task_param.bbox.maxx = bbox.maxx; */
    /* task_param.bbox.miny = bbox.miny; */
    /* task_param.bbox.maxy = bbox.maxy; */
    task_param.draw.left   = bbox.minx;
    task_param.draw.width  = bbox.maxx - bbox.minx;
    task_param.draw.top    = bbox.miny;
    task_param.draw.height = bbox.maxy - bbox.miny;

    task_param.global_alpha = 128;
    task_param.blend_mode = 1;
    task_param.premult = 1;

    //纹理指令
    task_param.format = image_attr->gpu_format;
    task_param.clut_format = image_attr->clut_format;
    task_param.image.format = image_attr->format;
    task_param.image.width = image_attr->width;
    task_param.image.height = image_attr->height;
    task_param.image.compress = image_attr->compress;
    task_param.image.has_clut = image_attr->has_clut;
    task_param.has_clut = image_attr->has_clut;
    task_param.image.len = image_attr->len;
    task_param.texture.data = image_attr->data;
    task_param.texture.mmu_tab_base = image_attr->tab;
    if (!task_param.image.compress) {
        task_param.texture.not_compress = 1;
        task_param.texture.adr_mode = 1;
    }
    //透视指令
    task_param.perspective_en = 1;
    /* task_param.fg_win.left = 0; */
    /* task_param.fg_win.width = image_attr->width; */
    /* task_param.fg_win.top = 0; */
    /* task_param.fg_win.height = image_attr->height; */
    task_param.texture.crop.left   = 0;
    task_param.texture.crop.width  = image_attr->width;
    task_param.texture.crop.top    = 0;
    task_param.texture.crop.height = image_attr->height;

    task_param.matrix = &matrix;
    jlgpu_get_win_rect(&task_param.area);

    pJLGPUTaskHead_t new_head = NULL;
    if (head) {
        new_head = jlgpu_chain_copy_create_head(head, false);
    } else {
        new_head = jlgpu_chain_create_head();
    }

    pJLGPUTaskUnit_t taskp;
    jlgpu_task_texture_param_map_707n();
    if (jlgpu_create_task_list_by_image_cb_func) {
        jlgpu_create_task_list_by_image_cb_func(&task_param);
    }
    taskp = jlgpu_create_task(new_head, &task_param);
    taskp->info.image_width = image_attr->width;
    taskp->info.image_height = image_attr->height;

    return new_head;
}
static void img_reset_blend_mode_dst_in_cb(JLGPUTaskParam_t *param)
{
    param->blend_mode = GPU_BLEND_DST_IN;
    param->priority = 0x7ff;
}
static void img_reset_blend_mode_dst_out_cb(JLGPUTaskParam_t *param)
{
    param->blend_mode = GPU_BLEND_DST_OUT;
    param->priority = 0x7ff;
}
pJLGPUTaskHead_t jlgpu_create_task_list_by_image_set_blend_mode(pJLGPUTaskHead_t head, struct ui_image_attrs *image_attr, int xoffset, int yoffset, int blend_mode)
{
    if (blend_mode ==  GPU_BLEND_DST_IN) {
        jlgpu_create_task_list_by_image_cb_func = img_reset_blend_mode_dst_in_cb;
    } else if (blend_mode == GPU_BLEND_DST_OUT) {
        jlgpu_create_task_list_by_image_cb_func = img_reset_blend_mode_dst_out_cb;
    } else {
        ASSERT(0);
    }
    pJLGPUTaskHead_t  new_head = jlgpu_create_task_list_by_image(head, image_attr, xoffset, yoffset);
    jlgpu_create_task_list_by_image_cb_func = NULL;
    return new_head;
}
pJLGPUTaskHead_t jlgpu_task_list_map_window_id(u32 win_id)
{
    int image_id = get_face_preview(win_id);
    if (image_id == -1) {
        return NULL;
    }
    struct ui_platform_api *platform_api = ui_get_platform_api();
    ASSERT(platform_api);

    struct draw_context dc_tmp = {0};
    struct rect rect = {0};
    struct element *win_elm = ui_core_get_element_by_id(win_id);
    if (!win_elm) {
        return NULL;
    }
    ui_core_get_draw_context(&dc_tmp, win_elm, &rect);
    struct rect win_rect;
    jlgpu_get_win_rect(&win_rect);
    struct ui_image_attrs image_attr;
    dc_tmp.prj = 0;
    dc_tmp.page = (image_id >> 16) & 0xff;
    platform_api->read_image_info(&dc_tmp, image_id & 0xffff, &image_attr);
    pJLGPUTaskHead_t new_head = jlgpu_create_task_list_by_image(NULL, &image_attr, win_rect.left, win_rect.top);
    return new_head;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief page_mode_mode_flip_effect_draw 翻页特效处理
 *
 * @param mult_list
 * @param draw
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
extern int ui_page_get_mode();
int page_mode_mode_flip_effect_draw(pJLGPUMultTaskList_t mult_list, struct ui_page_draw *draw)
{
    //参数
    struct ui_page_priv *page_ctrl = ui_page_get_param();
    struct rect rect;
    ASSERT(page_ctrl->degree != 90.0f);
    int idx = (page_ctrl->degree <= 90.0f) ? page_ctrl->curr_page_index : !page_ctrl->curr_page_index;
    idx = (page_ctrl->left_flip) ? !idx : idx;

    //创建新链表
    pJLGPUTaskHead_t new_head = jlgpu_task_list_copy_create(NULL, jlgpu_mult_task_head_by_index(mult_list, idx), NULL, 0);
    ASSERT(new_head);
    //处理
    gpu_matrix_t matrix;
    gpu_matrix_set_identity(&matrix);
    jlgpu_get_win_rect(&rect);
    get_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree);
    if (ui_page_get_mode() == PAGE_MOVE_MODE_CENTER_FLIP) {
        jlgpu_chain_head_set_status(new_head, HEAD_STATUS_READY);
    }
    jlgpu_task_list_copy_mul_matrix_by_group(new_head, &matrix, 0);
    //排序
    int new_list_num = 1;

    int list_total = jlgpu_mult_task_list_get_head_num(mult_list);

    draw->new_task_list = (pJLGPUTaskHead_t *)zalloc(sizeof(pJLGPUTaskHead_t) * (list_total + new_list_num));
    draw->rec_page_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_gpu_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_lcd_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));

    for (int idx = 0; idx < list_total; idx++) {
        draw->new_task_list[idx] = (pJLGPUTaskHead_t)jlgpu_mult_task_head_by_index(mult_list, idx);
        if (ui_page_get_mode() == PAGE_MOVE_MODE_CENTER_FLIP) {
            jlgpu_chain_head_set_status(draw->new_task_list[idx], HEAD_STATUS_IDLE);
        }
        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx, &draw->rec_page_rect[idx], &draw->rec_gpu_rect[idx], &draw->rec_lcd_rect[idx]);
    }
    draw->new_task_list[list_total] = new_head;
    draw->new_list_create |= BIT(2);

    for (int idx = 0; idx < new_list_num; idx++) {
        draw->rec_gpu_rect[list_total + idx].left = page_ctrl->flip_left;
        draw->rec_gpu_rect[list_total + idx].width = page_ctrl->flip_width;
    }
    draw->list_total = list_total + new_list_num;
    return draw->list_total ;
}
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_FLIP)
.effect_draw =  page_mode_mode_flip_effect_draw,
};
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_CENTER_FLIP)
.effect_draw =  page_mode_mode_flip_effect_draw,
};

pJLGPUTaskHead_t jlui_create_bg_task_list(u32 out_format)
{
    pJLGPUTaskHead_t background_head = jlgpu_chain_create_head();
    ASSERT(background_head);
    struct rect win_rect;
    jlgpu_get_win_rect(&win_rect);
    jlgpu_task_fill_param_init(0, 0, 0x64, 0x20, 0x20, 0x20);
    struct rect lcd_rect;
    lcd_rect.left = 0;
    lcd_rect.top = 0;
    lcd_rect.width = win_rect.width;
    lcd_rect.height = win_rect.height;
    memcpy(&task_param.draw, &lcd_rect, sizeof(struct rect));
    jlgpu_get_win_rect(&task_param.area);
    task_param.task_type = GPU_TASK_FILL;
    jlgpu_task_fill_param_map_707n();
    jlgpu_create_task(background_head, &task_param);
    jlgpu_set_task_list_out_format(background_head, out_format);
    return background_head;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief page_mode_mode_cube_filp_effect_draw 立方体翻页
 *
 * @param mult_list
 * @param draw
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int page_mode_mode_cube_filp_effect_draw(pJLGPUMultTaskList_t mult_list, struct ui_page_draw *draw)
{
    //排序
    int set_idx = 0;
    int new_list_num = 2;//背景+倒影
    int list_total = jlgpu_mult_task_list_get_head_num(mult_list);

    draw->new_task_list = (pJLGPUTaskHead_t *)zalloc(sizeof(pJLGPUTaskHead_t) * (list_total + new_list_num));
    draw->rec_page_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_gpu_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_lcd_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));

    //参数
    struct ui_page_priv *page_ctrl = ui_page_get_param();
    struct rect rect;
    ASSERT(page_ctrl->degree != 90.0f);

    int idx = page_ctrl->curr_page_index;
    int idx_other = !idx;
    //创建新链表
    u32 out_format = jlgpu_get_task_list_out_format(jlgpu_mult_task_head_by_index(mult_list, 0));
    pJLGPUTaskHead_t background_head = jlui_create_bg_task_list(out_format);
    draw->new_list_create |= BIT(set_idx);
    draw->new_task_list[set_idx] = background_head;
    draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
    draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
    set_idx++;
    //处理
    gpu_matrix_t matrix;
    gpu_matrix_set_identity(&matrix);
    jlgpu_get_win_rect(&rect);
    struct element *root = ui_core_get_root();
    struct element *win;
    u32 curr_win = 0;
    u32 other_win = 0;
    list_for_each_child_element(win, root) {
        if (win->dc->index == idx) {
            curr_win  = win->id;
        } else if (win->dc->index == idx_other) {
            other_win = win->id;
        }
    }
#if 1		//倒影
    // 当前页面的倒影
    pJLGPUTaskHead_t new_head = NULL;

    get_cube_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 1, page_ctrl->left_flip, 0);
    if (curr_win) {
        pJLGPUTaskHead_t map_head = jlgpu_task_list_map_window_id(curr_win);
        if (map_head) {
            jlgpu_set_task_list_out_format(map_head, out_format);
            new_head = jlgpu_task_list_copy_create(NULL, map_head, &matrix, 0);
            jlgpu_delete_task_list_head(map_head);
        } else {
            new_head = jlgpu_task_list_copy_create(NULL, jlgpu_mult_task_head_by_index(mult_list, idx), &matrix, 0);
        }
    }

    // 待切换的页面的倒影
    get_cube_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 1, page_ctrl->left_flip, 1);
    if (other_win) {
        pJLGPUTaskHead_t map_head = jlgpu_task_list_map_window_id(other_win);
        if (map_head) {
            jlgpu_task_list_copy_create(new_head, map_head, &matrix, 1);
            jlgpu_delete_task_list_head(map_head);
        } else {
            jlgpu_task_list_copy_create(new_head, jlgpu_mult_task_head_by_index(mult_list, idx_other), &matrix, 1);
        }
    }
    //不需要背景
    jlgpu_task_enable(jlgpu_task_find_first_task(new_head), 0);
    // 阴影部分透明处理
    u8 order_tab[] = {0, 1};
    u8 global_alpha_tab[2] = { 32, 32};
    jlgpu_task_list_copy_group_global_alpha_reset(new_head, order_tab, global_alpha_tab, 2);
    draw->new_list_create |= BIT(set_idx);
    draw->new_task_list[set_idx] = new_head;
    draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
    draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
    set_idx++;
#endif
    pJLGPUTaskHead_t tmp_head  = NULL;
    //页面0
    get_cube_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 0, page_ctrl->left_flip, 0);
    if (curr_win) {
        tmp_head = jlgpu_task_list_map_window_id(curr_win);
        if (tmp_head) {
            jlgpu_set_task_list_out_format(tmp_head, out_format);
            draw->new_list_create |= BIT(set_idx);
            draw->normal_list_flag |=  BIT(set_idx);
        } else {
            tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx);
            if (/*curr_win != DIAL_PAGE_0*/0) {
                jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
            } else {
                pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, &matrix, 0);
                tmp_head = tmp_head1;
                draw->new_list_create |= BIT(set_idx);
            }
        }
    }

    jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
    draw->new_task_list[set_idx] = tmp_head;
    jlgpu_mult_task_head_get_rect_by_index(mult_list, idx, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
    set_idx++;
    //页面1
    get_cube_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 0, page_ctrl->left_flip, 1);
    if (other_win) {
        tmp_head = jlgpu_task_list_map_window_id(other_win);
        if (tmp_head) {
            jlgpu_set_task_list_out_format(tmp_head, out_format);
            draw->new_list_create |= BIT(set_idx);
            draw->normal_list_flag |=  BIT(set_idx);
        } else {
            tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx_other);
            if (/*other_win != DIAL_PAGE_0*/0) {
                jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
            } else {
                pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, &matrix, 0);
                tmp_head = tmp_head1;
                draw->new_list_create |= BIT(set_idx);
            }
        }
    }
    jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
    draw->new_task_list[set_idx] = tmp_head;
    jlgpu_mult_task_head_get_rect_by_index(mult_list,  idx_other, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
    set_idx++;

    draw->list_total = set_idx;
    return draw->list_total;
}
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_CUBE_FLIP)
.effect_draw =  page_mode_mode_cube_filp_effect_draw,
};

/* ------------------------------------------------------------------------------------*/
/**
 * @brief page_mode_mode_drift_filp_effect_draw 漂移翻页
 *
 * @param mult_list
 * @param draw
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int page_mode_mode_drift_filp_effect_draw(pJLGPUMultTaskList_t mult_list, struct ui_page_draw *draw)
{
    //排序
    int set_idx = 0;
    int new_list_num = 3;//背景+2倒影
    int list_total = jlgpu_mult_task_list_get_head_num(mult_list);

    draw->new_task_list = (pJLGPUTaskHead_t *)zalloc(sizeof(pJLGPUTaskHead_t) * (list_total + new_list_num));
    draw->rec_page_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_gpu_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_lcd_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));

    //参数
    struct ui_page_priv *page_ctrl = ui_page_get_param();
    struct rect rect;
    ASSERT(page_ctrl->degree != 90.0f);

    int idx = page_ctrl->curr_page_index;
    int idx_other = !idx;
    struct element *root = ui_core_get_root();
    struct element *win;
    u32 curr_win = 0;
    u32 other_win = 0;
    list_for_each_child_element(win, root) {
        if (win->dc->index == idx) {
            curr_win  = win->id;
        } else if (win->dc->index == idx_other) {
            other_win = win->id;
        }
    }
    //创建新链表
    u32 out_format = jlgpu_get_task_list_out_format(jlgpu_mult_task_head_by_index(mult_list, 0));
    pJLGPUTaskHead_t background_head = jlui_create_bg_task_list(out_format);
    draw->new_list_create |= BIT(set_idx);
    draw->new_task_list[set_idx] = background_head;
    draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
    draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
    set_idx++;
    //处理
    gpu_matrix_t matrix;
    gpu_matrix_set_identity(&matrix);
    jlgpu_get_win_rect(&rect);
    pJLGPUTaskHead_t new_head = NULL;
    pJLGPUTaskHead_t tmp_head = NULL;
    if (page_ctrl->degree > 45.0f) {
#if 1			//加倒影，ram紧张
        // 阴影部分透明处理
        u8 order_tab[] = {0};
        u8 global_alpha_tab[] = {32};
        // 当前页面的倒影
        get_drift_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 1, page_ctrl->left_flip, 0);
        if (curr_win) {
            pJLGPUTaskHead_t map_head = jlgpu_task_list_map_window_id(curr_win);
            if (map_head) {
                jlgpu_set_task_list_out_format(map_head, out_format);
                new_head = jlgpu_task_list_copy_create(NULL, map_head, &matrix, 0);
                jlgpu_delete_task_list_head(map_head);
            } else {
                new_head = jlgpu_task_list_copy_create(NULL, jlgpu_mult_task_head_by_index(mult_list, idx), &matrix, 0);
            }
        }
        ASSERT(new_head);
        jlgpu_task_enable(jlgpu_task_find_first_task(new_head), 0);
        jlgpu_task_list_copy_group_global_alpha_reset(new_head, order_tab, global_alpha_tab, 1);
        draw->new_list_create |= BIT(set_idx);
        draw->new_task_list[set_idx] = new_head;
        draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
        draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
        set_idx++;
        // 待切换的页面的倒影
        new_head = NULL;
        get_drift_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 1, page_ctrl->left_flip, 1);
        if (other_win) {
            pJLGPUTaskHead_t map_head = jlgpu_task_list_map_window_id(other_win);
            if (map_head) {
                jlgpu_set_task_list_out_format(map_head, out_format);
                new_head = jlgpu_task_list_copy_create(NULL, map_head, &matrix, 0);
                jlgpu_delete_task_list_head(map_head);
            } else {
                new_head = jlgpu_task_list_copy_create(NULL, jlgpu_mult_task_head_by_index(mult_list, idx_other), &matrix, 0);
            }
        }
        ASSERT(new_head);
        jlgpu_task_enable(jlgpu_task_find_first_task(new_head), 0);
        jlgpu_task_list_copy_group_global_alpha_reset(new_head, order_tab, global_alpha_tab, 1);
        draw->new_list_create |= BIT(set_idx);
        draw->new_task_list[set_idx] = new_head;
        draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
        draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
        set_idx++;
#endif

        //页面0
        get_drift_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 0, page_ctrl->left_flip, 0);
        if (curr_win) {
            tmp_head = jlgpu_task_list_map_window_id(curr_win);
            if (tmp_head) {
                jlgpu_set_task_list_out_format(tmp_head, out_format);
                draw->new_list_create |= BIT(set_idx);
                draw->normal_list_flag |=  BIT(set_idx);
            } else {
                tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx);
                if (/*curr_win != DIAL_PAGE_0*/0) {
                    jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
                } else {
                    pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, NULL, 0);
                    tmp_head = tmp_head1;
                    draw->new_list_create |= BIT(set_idx);
                }
            }
        }
        jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
        draw->new_task_list[set_idx] = tmp_head;
        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
        set_idx++;
        //页面1
        get_drift_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 0, page_ctrl->left_flip, 1);
        if (other_win) {
            tmp_head = jlgpu_task_list_map_window_id(other_win);
            if (tmp_head) {
                jlgpu_set_task_list_out_format(tmp_head, out_format);
                draw->new_list_create |= BIT(set_idx);
                draw->normal_list_flag |=  BIT(set_idx);
            } else {
                tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx_other);
                if (/*other_win != DIAL_PAGE_0*/0) {
                    jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
                } else {
                    pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, NULL, 0);
                    tmp_head = tmp_head1;
                    draw->new_list_create |= BIT(set_idx);
                }
            }
        }
        jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
        draw->new_task_list[set_idx] = tmp_head;
        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx_other, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
        set_idx++;

    } else {
#if 1			//加倒影，ram紧张
        u8 order_tab[] = {0};
        u8 global_alpha_tab[] = {32};
        // 待切换的页面的倒影
        get_drift_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 1, page_ctrl->left_flip, 1);
        if (other_win) {
            pJLGPUTaskHead_t map_head = jlgpu_task_list_map_window_id(other_win);
            if (map_head) {
                jlgpu_set_task_list_out_format(map_head, out_format);
                new_head = jlgpu_task_list_copy_create(NULL, map_head, &matrix, 0);
                jlgpu_delete_task_list_head(map_head);
            } else {
                new_head = jlgpu_task_list_copy_create(NULL, jlgpu_mult_task_head_by_index(mult_list, idx_other), &matrix, 0);
            }
        }

        ASSERT(new_head);
        jlgpu_task_enable(jlgpu_task_find_first_task(new_head), 0);
        jlgpu_task_list_copy_group_global_alpha_reset(new_head, order_tab, global_alpha_tab, 1);
        draw->new_list_create |= BIT(set_idx);
        draw->new_task_list[set_idx] = new_head;
        draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
        draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
        set_idx++;

        // 当前页面的倒影
        get_drift_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 1, page_ctrl->left_flip, 0);
        if (curr_win) {
            pJLGPUTaskHead_t map_head = jlgpu_task_list_map_window_id(curr_win);
            if (map_head) {
                jlgpu_set_task_list_out_format(map_head, out_format);
                new_head = jlgpu_task_list_copy_create(NULL, map_head, &matrix, 0);
                jlgpu_delete_task_list_head(map_head);
            } else {
                new_head = jlgpu_task_list_copy_create(NULL, jlgpu_mult_task_head_by_index(mult_list, idx), &matrix, 0);
            }
        }
        ASSERT(new_head);
        jlgpu_task_enable(jlgpu_task_find_first_task(new_head), 0);
        jlgpu_task_list_copy_group_global_alpha_reset(new_head, order_tab, global_alpha_tab, 1);
        draw->new_list_create |= BIT(set_idx);
        draw->new_task_list[set_idx] = new_head;
        draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
        draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
        set_idx++;
#endif
        //页面1
        get_drift_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 0, page_ctrl->left_flip, 1);
        if (other_win) {
            tmp_head = jlgpu_task_list_map_window_id(other_win);
            if (tmp_head) {
                jlgpu_set_task_list_out_format(tmp_head, out_format);
                draw->new_list_create |= BIT(set_idx);
                draw->normal_list_flag |=  BIT(set_idx);
            } else {
                tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx_other);
                if (/*other_win != DIAL_PAGE_0*/0) {
                    jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
                } else {
                    pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, NULL, 0);
                    tmp_head = tmp_head1;
                    draw->new_list_create |= BIT(set_idx);
                }
            }
        }
        jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);

        draw->new_task_list[set_idx] = tmp_head;
        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx_other, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
        set_idx++;

        //页面0
        get_drift_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, 0, page_ctrl->left_flip, 0);
        if (curr_win) {
            tmp_head = jlgpu_task_list_map_window_id(curr_win);
            if (tmp_head) {
                jlgpu_set_task_list_out_format(tmp_head, out_format);
                draw->new_list_create |= BIT(set_idx);
                draw->normal_list_flag |=  BIT(set_idx);
            } else {
                tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx);
                if (/*curr_win != DIAL_PAGE_0*/0) {
                    jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
                } else {
                    pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, NULL, 0);
                    tmp_head = tmp_head1;
                    draw->new_list_create |= BIT(set_idx);
                }
            }
        }
        jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
        draw->new_task_list[set_idx] = tmp_head;
        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
        set_idx++;
    }

    draw->list_total = set_idx;
    return draw->list_total;
}
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_DRIFT_FLIP)
.effect_draw =  page_mode_mode_drift_filp_effect_draw,
};
/* ------------------------------------------------------------------------------------*/
/**
 * @brief page_mode_mode_edge_filp_effect_draw 边沿翻转
 *
 * @param mult_list
 * @param draw
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int page_mode_mode_edge_filp_effect_draw(pJLGPUMultTaskList_t mult_list, struct ui_page_draw *draw)
{
    //排序
    int set_idx = 0;
    int new_list_num = 1;
    int list_total = jlgpu_mult_task_list_get_head_num(mult_list);

    draw->new_task_list = (pJLGPUTaskHead_t *)zalloc(sizeof(pJLGPUTaskHead_t) * (list_total + new_list_num));
    draw->rec_page_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_gpu_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_lcd_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));

    //参数
    struct ui_page_priv *page_ctrl = ui_page_get_param();
    struct rect rect;

    int idx = page_ctrl->curr_page_index;
    int idx_other = !idx;

    struct element *root = ui_core_get_root();
    struct element *win;
    u32 curr_win = 0;
    u32 other_win = 0;
    list_for_each_child_element(win, root) {
        if (win->dc->index == idx) {
            curr_win  = win->id;
        } else if (win->dc->index == idx_other) {
            other_win = win->id;
        }
    }

    //创建新链表
    u32 out_format = jlgpu_get_task_list_out_format(jlgpu_mult_task_head_by_index(mult_list, 0));
    pJLGPUTaskHead_t background_head = jlui_create_bg_task_list(out_format);
    draw->new_list_create |= BIT(set_idx);
    draw->new_task_list[set_idx] = background_head;
    draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
    draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
    set_idx++;

    //处理
    gpu_matrix_t matrix;
    gpu_matrix_set_identity(&matrix);
    jlgpu_get_win_rect(&rect);
    pJLGPUTaskHead_t tmp_head  = NULL;
    //页面0
    get_edge_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, page_ctrl->left_flip, 0, page_ctrl->edge_flip_angle);
    tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx);
    if (/*curr_win != DIAL_PAGE_0*/0) {
        jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
    } else {
        pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, NULL, 0);
        tmp_head = tmp_head1;
        draw->new_list_create |= BIT(set_idx);
    }
    jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
    draw->new_task_list[set_idx] = tmp_head;
    jlgpu_mult_task_head_get_rect_by_index(mult_list, idx, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
    set_idx++;

    //页面1
    get_edge_flip_matrix(&matrix, rect.left, rect.top, rect.width, rect.height, page_ctrl->degree, page_ctrl->left_flip, 1, page_ctrl->edge_flip_angle);
    tmp_head = jlgpu_mult_task_head_by_index(mult_list,  idx_other);
    if (/*other_win != DIAL_PAGE_0*/0) {
        jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
    } else {
        pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, &matrix, 0);
        tmp_head = tmp_head1;
        draw->new_list_create |= BIT(set_idx);
    }
    jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
    draw->new_task_list[set_idx] = tmp_head;
    jlgpu_mult_task_head_get_rect_by_index(mult_list, idx_other, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
    set_idx++;

    draw->list_total = set_idx;
    return draw->list_total;
}
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_EDGE_FLIP)
.effect_draw =  page_mode_mode_edge_filp_effect_draw,
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief page_mode_mode_board_filp_effect_draw 翻板
 *
 * @param mult_list
 * @param draw
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int page_mode_mode_board_filp_effect_draw(pJLGPUMultTaskList_t mult_list, struct ui_page_draw *draw)
{
    //排序
    int set_idx = 0;
    int new_list_num = 2;//背景+翻板填充
    int list_total = jlgpu_mult_task_list_get_head_num(mult_list);

    draw->new_task_list = (pJLGPUTaskHead_t *)zalloc(sizeof(pJLGPUTaskHead_t) * (list_total + new_list_num));
    draw->rec_page_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_gpu_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_lcd_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));

    //参数
    struct ui_page_priv *page_ctrl = ui_page_get_param();
    struct rect rect;
    ASSERT(page_ctrl->degree != 90.0f);
    int idx = page_ctrl->curr_page_index;
    int idx_other = !idx;

    struct element *root = ui_core_get_root();
    struct element *win;
    u32 curr_win = 0;
    u32 other_win = 0;
    list_for_each_child_element(win, root) {
        if (win->dc->index == idx) {
            curr_win  = win->id;
        } else if (win->dc->index == idx_other) {
            other_win = win->id;
        }
    }


    //创建新链表
    u32 out_format = jlgpu_get_task_list_out_format(jlgpu_mult_task_head_by_index(mult_list, 0));
    pJLGPUTaskHead_t background_head = jlui_create_bg_task_list(out_format);
    draw->new_list_create |= BIT(set_idx);
    draw->new_task_list[set_idx] = background_head;
    draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
    draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
    set_idx++;

    //处理
    gpu_matrix_t matrix;
    gpu_matrix_set_identity(&matrix);
    jlgpu_get_win_rect(&rect);

    board_cube_init(rect.width, rect.height, 1.0f, 1.0f, rect.left, rect.top, rect.width, rect.height);
    pJLGPUTaskHead_t tmp_head  = NULL;
    if (page_ctrl->degree > 90.0f) {
        // 当前页面
        get_board_flip_matrix(&matrix, page_ctrl->degree, page_ctrl->left_flip, 0);
        tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx);
        if (/*curr_win != DIAL_PAGE_0*/0) {
            jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
        } else {
            pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, NULL, 0);
            tmp_head = tmp_head1;
            draw->new_list_create |= BIT(set_idx);
        }
        jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
        draw->new_task_list[set_idx] = tmp_head;
        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
        set_idx++;

        // 待切换的页面
        get_board_flip_matrix(&matrix, page_ctrl->degree, page_ctrl->left_flip, 1);
        tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx_other);
        if (/*other_win != DIAL_PAGE_0*/0) {
            jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
        } else {
            pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, NULL, 0);
            tmp_head = tmp_head1;
            draw->new_list_create |= BIT(set_idx);
        }
        jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
        draw->new_task_list[set_idx] = tmp_head;

        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx_other, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
        set_idx++;

    } else {
        // 待切换的页面
        get_board_flip_matrix(&matrix, page_ctrl->degree, page_ctrl->left_flip, 1);
        tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx_other);
        if (/*other_win != DIAL_PAGE_0*/0) {
            jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
        } else {
            pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, NULL, 0);
            tmp_head = tmp_head1;
            draw->new_list_create |= BIT(set_idx);
        }

        jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
        draw->new_task_list[set_idx] = tmp_head;

        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx_other, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
        set_idx++;
        // 当前页面
        get_board_flip_matrix(&matrix, page_ctrl->degree, page_ctrl->left_flip, 0);
        tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx);
        if (/*curr_win != DIAL_PAGE_0*/0) {
            jlgpu_task_enable(jlgpu_task_find_first_task(tmp_head), 0);
        } else {
            pJLGPUTaskHead_t tmp_head1 = jlgpu_task_list_copy_create_with_backcolor(0, tmp_head, &matrix, 0);
            tmp_head = tmp_head1;
            draw->new_list_create |= BIT(set_idx);
        }
        jlgpu_task_list_copy_mul_matrix_by_group(tmp_head, &matrix, 0);
        draw->new_task_list[set_idx] = tmp_head;
        jlgpu_mult_task_head_get_rect_by_index(mult_list, idx, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
        set_idx++;
    }
    // 填充板宽
    GPU_out_format_t fill_out_format = jlgpu_get_task_list_out_format(tmp_head);
    //创建任务链
    pJLGPUTaskHead_t fill_head = jlgpu_chain_create_head();
    ASSERT(fill_head);
    //设置任务链输出的格式
    jlgpu_set_task_list_out_format(fill_head, fill_out_format);
    struct rect draw_task_rect = {0};
    draw_task_rect.left = 0;
    draw_task_rect.top = 0;
    draw_task_rect.width = rect.width;
    draw_task_rect.height = rect.height;
    // 0xFFD3D3D3: 浅灰色; 0xFF404040: 深灰色
    create_board_flip_fill_task(fill_head, draw_task_rect, 0xFFD3D3D3, page_ctrl->degree, page_ctrl->left_flip);
    board_cube_uninit();
    draw->new_list_create |= BIT(set_idx);
    draw->new_task_list[set_idx] = fill_head;
    draw->rec_gpu_rect[set_idx].left = page_ctrl->flip_left;
    draw->rec_gpu_rect[set_idx].width = page_ctrl->flip_width;
    set_idx++;
    draw->list_total = set_idx;
    return draw->list_total;
}
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_BOARD_FLIP)
.effect_draw =  page_mode_mode_board_filp_effect_draw,
};
/* ------------------------------------------------------------------------------------*/
/**
 * @brief page_mode_mode_default_effect_draw 默认，水平滑动
 *
 * @param mult_list
 * @param draw
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int page_mode_mode_default_effect_draw(pJLGPUMultTaskList_t mult_list, struct ui_page_draw *draw)
{
    //排序
    int set_idx = 0;
    int new_list_num = 0;
    int list_total = jlgpu_mult_task_list_get_head_num(mult_list);

    draw->new_task_list = (pJLGPUTaskHead_t *)zalloc(sizeof(pJLGPUTaskHead_t) * (list_total + new_list_num));
    draw->rec_page_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_gpu_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));
    draw->rec_lcd_rect  = zalloc(sizeof(struct rect) * (list_total + new_list_num));

    //参数
    struct ui_page_priv *page_ctrl = ui_page_get_param();
    struct rect rect;
    ASSERT(page_ctrl->degree != 90.0f);
    int idx = page_ctrl->curr_page_index;
    int idx_other = !idx;
    pJLGPUTaskHead_t tmp_head  = NULL;
    // 当前页面
    tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx);
    draw->new_task_list[set_idx] = tmp_head;
    jlgpu_mult_task_head_get_rect_by_index(mult_list, idx, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
    set_idx++;

    // 待切换的页面
    tmp_head = jlgpu_mult_task_head_by_index(mult_list, idx_other);
    draw->new_task_list[set_idx] = tmp_head;
    jlgpu_mult_task_head_get_rect_by_index(mult_list, idx_other, &draw->rec_page_rect[set_idx], &draw->rec_gpu_rect[set_idx], &draw->rec_lcd_rect[set_idx]);
    set_idx++;
    draw->list_total = set_idx;
    return draw->list_total;
}
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_DOUBLE_PAGE)
.effect_draw =  page_mode_mode_default_effect_draw,
};
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_SCALE_DOUBLE)
.effect_draw =  page_mode_mode_default_effect_draw,
};


//===============================================================================================//
//		自定义特效处理demo
/*页面缩放+透明度变化*/
//===============================================================================================//
#if TCFG_UI_EFFECT_USED_DEMO
#include "jlui/ui_page_switch.h"
void ui_page_effect_double_scale_alpha_cb(struct element *curr_elm, struct element *prev_elm, struct element *next_elm, int cur_left)
{
    struct element *curr_elm_layer, *other_elm_layer;
    struct ui_page_priv *priv = ui_page_get_param();
    int page_width = curr_elm->dc->width;
    int page_height = curr_elm->dc->height;

    struct rect rect;
    rect.left = 0;
    rect.top = 0;
    rect.width = page_width;
    rect.height = page_height;

    struct rect lcdrect;
    lcdrect.left = 0;
    lcdrect.top = 0;
    lcdrect.width = page_width;
    lcdrect.height = page_height;

    if (curr_elm && prev_elm) {
        //配置当前页面区域
        rect.left = cur_left;
        memcpy(&curr_elm->dc->page_r, &rect, sizeof(struct rect));
        get_rect_cover(&rect, &lcdrect, &curr_elm->dc->lcd_r);
        memcpy(&curr_elm->dc->gpu_r, &curr_elm->dc->lcd_r, sizeof(struct rect));
        curr_elm->dc->gpu_r.left = curr_elm->dc->lcd_r.left - curr_elm->dc->page_r.left;
        //配置另一页面区域
        rect.left = cur_left - page_width;
        memcpy(&prev_elm->dc->page_r, &rect, sizeof(struct rect));
        get_rect_cover(&rect, &lcdrect, &prev_elm->dc->lcd_r);
        memcpy(&prev_elm->dc->gpu_r, &prev_elm->dc->lcd_r, sizeof(struct rect));
        prev_elm->dc->gpu_r.left = prev_elm->dc->lcd_r.left - prev_elm->dc->page_r.left;
        //计算缩放系数
        float tmp_scale = priv->scale_max - priv->scale_min;
        float curr_scale  = tmp_scale * (page_width - abs(cur_left)) / page_width + priv->scale_min;
        float other_scale  = tmp_scale * (page_width - abs(cur_left - page_width)) / page_width + priv->scale_min;
        //配置到页面
        list_for_each_child_element(curr_elm_layer, curr_elm) {
            ui_core_set_element_ratio(curr_elm_layer, curr_scale, curr_scale, 1);
            int alpha = 100 * curr_scale;
            ui_effect_set_alpha(curr_elm_layer, alpha, 1, 0);
        }
        list_for_each_child_element(other_elm_layer, prev_elm) {
            ui_core_set_element_ratio(other_elm_layer, other_scale, other_scale, 1);
            int alpha = 100 * other_scale;
            ui_effect_set_alpha(other_elm_layer, alpha, 1, 0);
        }
    } else if (curr_elm && next_elm) {
        rect.left = cur_left;
        memcpy(&curr_elm->dc->page_r, &rect, sizeof(struct rect));
        get_rect_cover(&rect, &lcdrect, &curr_elm->dc->lcd_r);
        memcpy(&curr_elm->dc->gpu_r, &curr_elm->dc->lcd_r, sizeof(struct rect));
        curr_elm->dc->gpu_r.left = curr_elm->dc->lcd_r.left - curr_elm->dc->page_r.left;

        rect.left = cur_left + page_width;
        memcpy(&next_elm->dc->page_r, &rect, sizeof(struct rect));
        get_rect_cover(&rect, &lcdrect, &next_elm->dc->lcd_r);
        memcpy(&next_elm->dc->gpu_r, &next_elm->dc->lcd_r, sizeof(struct rect));
        next_elm->dc->gpu_r.left = next_elm->dc->lcd_r.left - next_elm->dc->page_r.left;

        float tmp_scale = priv->scale_max - priv->scale_min;
        float curr_scale  = tmp_scale * (page_width - abs(cur_left)) / page_width + priv->scale_min;
        float other_scale  = tmp_scale * (page_width - abs(cur_left + page_width)) / page_width + priv->scale_min;
        list_for_each_child_element(curr_elm_layer, curr_elm) {
            ui_core_set_element_ratio(curr_elm_layer, curr_scale, curr_scale, 1);
            int alpha = 100 * curr_scale;
            ui_effect_set_alpha(curr_elm_layer, alpha, 1, 0);
        }
        list_for_each_child_element(other_elm_layer, next_elm) {
            ui_core_set_element_ratio(other_elm_layer, other_scale, other_scale, 1);
            int alpha = 100 * other_scale;
            ui_effect_set_alpha(other_elm_layer, alpha, 1, 0);
        }
    }
}
REGISTER_UI_EFFECT_MODULE(PAGE_MOVE_MODE_USER)
.effect_draw =  page_mode_mode_default_effect_draw,
};
#endif
//=====================================================================================================================//
//			页面进入退出动画
//=====================================================================================================================//

struct ui_page_switch_priv {
    pJLGPUTaskHead_t head; 	// 前一个页面的GPU任务链

    struct rect win_rect;

    ui_anim_t anim; 		// 抬手后动画

    u32 curr_page;
    u32 target_page;

    u16 touch_x; 			// 进入子界面时点击的点位置，用于返回时缩小到对应位置
    u16 touch_y;

    u16 last_pos_x; 		// 要提前拦截tp消息，需要手动处理tp移动距离
    u16 last_pos_y;

    s16 x_offset; 			// x方向移动距离
    u8 return_effect;		// 返回特效标志
    u8 effect_status; 		// 是否准备好特效

    u32 window;				//从该页面退出时将触发返回动画
    void *buf_addr;

    u16  buf_size;
};
static struct  ui_page_switch_priv __ui_page_switch = {0};
#define __this	(&__ui_page_switch)
enum {
    PAGE_SWITCH_STATUS_NULL,
    PAGE_SWITCH_STATUS_ENTER,
    PAGE_SWITCH_STATUS_EXIT,
};
#define abs(x)      (((x) > 0) ? (x) : (-(x)))

/* ------------------------------------------------------------------------------------*/
/**
 * @brief menu_enter_app_anim_cb 页面进入动画回调
 *
 * @param var	动画标记
 * @param v		动画进度
 */
/* ------------------------------------------------------------------------------------*/
static void menu_enter_app_anim_cb(int var, int v)
{
    struct rect gpu_win;
    jlgpu_get_win_rect(&gpu_win);
    gpu_matrix_t matrix;

    /* APP页面放大 */
    float ratio = (float)v / 100;
    float zoom = 1.0f / ratio;
    int m_x = __this->touch_x * (1.0f - ratio);
    int m_y = __this->touch_y * (1.0f - ratio);
    //初始化矩阵
    gpu_matrix_set_identity(&matrix);
    //偏移到gpu中心（显示区域）
    gpu_matrix_translate(&matrix, -gpu_win.left, -gpu_win.top);
    //中心缩放（向缩放中心偏移）
    /* gpu_matrix_translate(&matrix, -m_x, -m_y); */
    //缩放
    /* gpu_matrix_scale(&matrix, zoom, zoom); */
    //将矩阵赋值给链表
    jlgpu_task_list_copy_mul_matrix_by_group(__this->head, &matrix, 2);

    /* 菜单页面放大 */
    ratio = 1.0f + ratio;
    zoom = 1.0f / ratio;
    m_x = (gpu_win.width * ratio - gpu_win.width) / 2;
    m_y = (gpu_win.height * ratio - gpu_win.height) / 2;
    /* printf("move x: %d, y: %d\n", m_x, m_y); */
    //初始化矩阵
    gpu_matrix_set_identity(&matrix);
    //偏移到gpu中心（显示区域）
    gpu_matrix_translate(&matrix, -gpu_win.left, -gpu_win.top);
    //中心缩放（向缩放中心偏移）
    gpu_matrix_translate(&matrix, m_x, m_y);
    //缩放
    gpu_matrix_scale(&matrix, zoom, zoom);
    //将矩阵赋值给链表
    jlgpu_task_list_copy_mul_matrix_by_group(__this->head, &matrix, 1);

    /* 调整透明度 */
    u8 order_tab[] = {0, 1, 2}; //页面排序
    u8 alpha_tab[] = {128, 128, 32}; //透明度
    alpha_tab[1] = 128 * (1.0f - (ratio - 1.0f));
    alpha_tab[2] = 128 * (ratio - 1.0f);
    alpha_tab[0] = alpha_tab[1];
    /* printf("ratio:%f alpha:%d %d %d", ratio, alpha_tab[0], alpha_tab[1], alpha_tab[2]); */
    jlgpu_task_list_copy_group_global_alpha_reset(__this->head, order_tab, alpha_tab, sizeof(order_tab));

    /* 发送到GPU任务进行绘制 */
    int err, msg[32];
    msg[0] = GPU_MSG_DRAW_LIST;
    msg[1] = (int)__this->head;
    msg[2] = (int)__this->buf_addr;
    msg[3] = (int)__this->buf_size;
    msg[4] = 0;

    err = os_taskq_post_type(GPU_TASK_NAME, msg[0], 6, &msg[1]);
    if (err != OS_NO_ERR) {
        printf("Error! post taskq to gpu task faild. err: %d\n", err);
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief menu_list_anim_ready_cb 动画结束回调
 *
 * @param p
 */
/* ------------------------------------------------------------------------------------*/
static void menu_list_anim_ready_cb(struct _ui_anim_t *p)
{
    /* 清空绘制消息队列，避免因为GPU任务有消息被阻塞导致资源释放了还被调用 */
    os_taskq_del_type(GPU_TASK_NAME, GPU_MSG_DRAW_LIST);
    jlgpu_scheduler_wait_sync();
    /* 动画结束，跳转到APP页面 */
    if (p) {
        struct element *elm = ui_core_get_element_by_id(__this->target_page);
        if (elm) {
            ui_hide(__this->target_page);
        }
        if (__this->target_page == ID_WINDOW_NOTICE) {
            extern u8 create_control_by_menu_set(u8 en);
            create_control_by_menu_set(1);
        }
        UI_SHOW_WINDOW(__this->target_page);
    }
    /* 清空过渡动画任务链 */
    jlgpu_task_list_copy_destroy(__this->head);
    ui_anim_del(__this->target_page, NULL);
    __this->head = NULL;
    __this->target_page = 0;
    __this->effect_status = PAGE_SWITCH_STATUS_NULL;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief menu_enter_app_anim 页面进入动画
 *
 * @param app_id	要进入的页面
 * @param touch_x	触点x
 * @param touch_y	触点y
 */
/* ------------------------------------------------------------------------------------*/
void menu_enter_app_anim(u32 app_id, int touch_x, int touch_y)
{
    if (__this->effect_status) {
        return;
    }
    /* 获取当前页面的GPU任务链 */
    u32 cur_id = ui_get_current_window_id();
    struct element *elm = ui_core_get_element_by_id(cur_id);
    __this->buf_addr = elm->dc->buf;
    __this->buf_size = elm->dc->len;
    __this->touch_x = touch_x;
    __this->touch_y = touch_y;
    __this->effect_status = PAGE_SWITCH_STATUS_ENTER;

    /* 加载GPU任务链 */
    extern pJLGPUTaskHead_t jlui_load_win_task_list(pJLGPUTaskHead_t head, u32 win_id, int index, int hide);
    __this->head = jlui_load_win_task_list(__this->head, cur_id, 1, false);
    __this->head = jlui_load_win_task_list(__this->head, app_id, 2, false);

    /* 使能返回特效 */
    /* printf("%s(), app_id: 0x%x, dial_id: 0x%x\n", __func__, app_id, ID_WINDOW_DIAL); */
    if (app_id != ID_WINDOW_DIAL) {
        extern void ui_return_page_effect_enable(u32 window, u16 pos_x, u16 pos_y, u8 enable);
        ui_return_page_effect_enable(app_id, __this->touch_x, __this->touch_y, true);
    }
    __this->curr_page = cur_id;
    __this->target_page = app_id;
    /* 创建过渡动画 */
    ui_anim_init(&__this->anim);
    ui_anim_set_var(&__this->anim, app_id);
    ui_anim_set_path_cb(&__this->anim, ui_anim_path_ease_out);
    ui_anim_set_exec_cb(&__this->anim, menu_enter_app_anim_cb);
    ui_anim_set_values(&__this->anim, 1, 100);
    ui_anim_set_time(&__this->anim, 400);
    /* ui_anim_set_ready_cb(&__this->anim, menu_list_anim_ready_cb); */
    ui_anim_set_deleted_cb(&__this->anim, menu_list_anim_ready_cb);
    ui_anim_start(&__this->anim);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief menu_enter_app_state 状态判断
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int menu_enter_app_state()
{
    return __this->effect_status;
}


/*
 通过右划 ELM_EVENT_TOUCH_R_MOVE 触发返回上一级的页面，
 可通过设置坐标 ui_return_page_effect_set_pos 和设置使
 能 ui_return_page_effect_set_en 来控制是否开启过渡效
 果，其中设置坐标为子界面缩小到最小时消失的坐标。
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_set_pos 设置返回坐标
 *
 * @param pos_x
 * @param pos_y
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_set_pos(u16 pos_x, u16 pos_y)
{
    __this->touch_x = pos_x;
    __this->touch_y = pos_y;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_set_en 使能返回特效
 *
 * @param enable
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_set_en(u8 enable)
{
    __this->return_effect = enable;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_set_win 记录需要从哪个页面返回
 *
 * @param window
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_set_win(u32 window)
{
    __this->window = window;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_enable 返回特效配置
 *
 * @param window
 * @param pos_x
 * @param pos_y
 * @param enable
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_enable(u32 window, u16 pos_x, u16 pos_y, u8 enable)
{
    ui_return_page_effect_set_pos(pos_x, pos_y);
    ui_return_page_effect_set_en(enable);
    ui_return_page_effect_set_win(window);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_init 返回特效
 *
 * @param ret_page	返回页面
 *
 * @return 0 支持启动返回特效，-1 不支持返回特效
 */
/* ------------------------------------------------------------------------------------*/
int ui_return_page_effect_init(u32 ret_page)
{
    if (__this->effect_status
        || (!__this->return_effect)
        || (__this->window != ui_get_current_window_id())
        || (strcmp(os_current_task(), "ui"))) {
        return -1;
    }
    if (__this->target_page) {
        ui_anim_del(__this->target_page, NULL);
    }
    extern pJLGPUTaskHead_t jlui_load_win_task_list(pJLGPUTaskHead_t head, u32 win_id, int index, int hide);
    u32 cur_page = ui_get_current_window_id();
    struct element *elm = ui_core_get_element_by_id(cur_page);
    ui_core_get_element_abs_rect(elm, &__this->win_rect);
    jlgpu_scheduler_set_redraw_mode(elm->dc, GPU_SYNC_REDRAW); // 设置为同步刷新
    __this->target_page = ret_page;
    __this->head = jlui_load_win_task_list(__this->head, ret_page, 2, true);  // 加载上一个页面
    __this->head = jlui_load_win_task_list(__this->head, cur_page, 1, false); // 加载当前的页面
    __this->x_offset = 0;

    int ret = jlgpu_mult_task_head_modify_by_index(elm->dc->gpu_mult_list, elm->dc->index, __this->head);
    ASSERT(!ret);
    __this->effect_status = PAGE_SWITCH_STATUS_EXIT;
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_draw 返回特效绘制
 */
/* ------------------------------------------------------------------------------------*/
static void ui_return_page_effect_draw()
{
    if (!__this->win_rect.width) {
        return;
    }
    float ratio = 1.0f - (float)(abs(__this->x_offset)) / __this->win_rect.width;
    ratio = (ratio < 0.01f) ? 0.01f : ratio;
    float zoom = 1.0f / ratio;
    /* printf("%s(), x_offset: %d, ratio: %f, zoom: %f\n", __func__, __this->x_offset, ratio, zoom); */

    struct rect gpu_win;
    jlgpu_get_win_rect(&gpu_win);

    /* 当前页面从1.0倍缩小 */
    gpu_matrix_t matrix;
    int mx = __this->touch_x * (1.0f - ratio);
    int my = __this->touch_y * (1.0f - ratio);
    gpu_matrix_set_identity(&matrix);
    gpu_matrix_translate(&matrix, -(gpu_win.left + mx), -(gpu_win.top + my));
    gpu_matrix_scale(&matrix, zoom, zoom);
    jlgpu_task_list_copy_mul_matrix_by_group(__this->head, &matrix, 1);

    /* 菜单界面从2.0倍缩小 */
    mx = __this->win_rect.width * ratio;
    my = __this->win_rect.height * ratio;
    ratio = 1.0f + ratio;
    zoom = 1.0f / ratio;
    gpu_matrix_set_identity(&matrix);
    gpu_matrix_translate(&matrix, -(gpu_win.left - mx), -(gpu_win.top - my));
    gpu_matrix_scale(&matrix, zoom, zoom);
    jlgpu_task_list_copy_mul_matrix_by_group(__this->head, &matrix, 2);

    /* 调整透明度 */
    u8 order_tab[] = {0, 1, 2};
    u8 alpha_tab[] = {128, 128, 128};
    alpha_tab[1] *= (ratio - 1.0f);
    alpha_tab[2] *= (1.0f - (ratio - 1.0f));
    alpha_tab[0] = alpha_tab[1];
    jlgpu_task_list_copy_group_global_alpha_reset(__this->head, order_tab, alpha_tab, sizeof(order_tab));

    struct element *curr_elm = ui_core_get_element_by_id(ui_get_current_window_id());
    ASSERT(curr_elm);
    struct rect lcdrect;
    lcdrect.left = 0;
    lcdrect.top = 0;
    ASSERT(curr_elm->dc);
    lcdrect.width = curr_elm->dc->width;
    lcdrect.height = curr_elm->dc->height;
    extern struct ui_platform_api *ui_get_platform_api();
    struct ui_platform_api *platform_api = ui_get_platform_api();
    if (platform_api->put_draw_context) {
        struct rect rect_orig;
        memcpy(&rect_orig, &curr_elm->dc->rect_orig, sizeof(struct rect));
        memcpy(&curr_elm->dc->rect_orig, &lcdrect, sizeof(struct rect));
        curr_elm->dc->refresh = true;
        platform_api->put_draw_context(curr_elm->dc);
        memcpy(&curr_elm->dc->rect_orig, &rect_orig, sizeof(struct rect));
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_move 跟手滑动
 *
 * @param xoffset
 * @param yoffset
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_move(int xoffset, int yoffset)
{
    if (!xoffset || !__this->effect_status) {
        return;
    }

    /* printf("%s(), %d, xoffset: %d\n", __func__, __LINE__, xoffset); */
    __this->x_offset += xoffset;
    if (__this->x_offset <= 0) {
        __this->x_offset = 0;
    } else if (__this->x_offset >= __this->win_rect.width) {
        __this->x_offset = __this->win_rect.width;
    }
    ui_return_page_effect_draw();
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_anim_cb 返回动画
 *
 * @param var
 * @param v
 */
/* ------------------------------------------------------------------------------------*/
static void ui_return_page_anim_cb(int var, int v)
{
    __this->x_offset = v;
    ui_return_page_effect_draw();
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_anim_ready 返回动画结束
 *
 * @param p
 */
/* ------------------------------------------------------------------------------------*/
static void ui_return_page_anim_ready(struct _ui_anim_t *p)
{
    jlgpu_scheduler_wait_sync();
    u32 cur_page = ui_get_current_window_id();
    struct element *elm = ui_core_get_element_by_id(cur_page);
    if (elm) {
        int ret = jlgpu_mult_task_head_modify_by_index(elm->dc->gpu_mult_list, elm->dc->index, elm->dc->gpu_task_head);
        ASSERT(!ret);
        jlgpu_scheduler_set_redraw_mode(elm->dc, GPU_ASYN_REDRAW); // 恢复为异步刷新
    }
    if (cur_page != __this->target_page) {
        ui_hide(cur_page);
    }
    jlgpu_task_list_copy_destroy(__this->head);
    printf("%s hide:%x show:%x", __func__, cur_page, __this->target_page);
    /* UI_SHOW_WINDOW(p->var); */
    if (p && (cur_page != __this->target_page)) {
        ui_show(__this->target_page);
    }
    ui_anim_del(__this->target_page, NULL);
    __this->head = NULL;

    if (__this->x_offset == __this->win_rect.width) {
        ui_return_page_effect_enable(0, 0, 0, false);
    } else {
        //返回原来页面
        ui_return_page_push(__this->target_page);
    }
    __this->x_offset = 0;
    __this->target_page = 0;
    __this->effect_status = PAGE_SWITCH_STATUS_NULL;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_free 抬手动画
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_free()
{
    if (!__this->effect_status) {
        return;
    }
    int start = __this->x_offset;
    int end, var;

    if (start < 20) {
        end = 0;
        var = ui_get_current_window_id();
    } else {
        end = __this->win_rect.width;
        var = __this->target_page;
    }
    __this->target_page = var;

    ui_anim_init(&__this->anim);
    ui_anim_set_var(&__this->anim, __this->target_page);
    ui_anim_set_path_cb(&__this->anim, ui_anim_path_ease_out);
    ui_anim_set_exec_cb(&__this->anim, ui_return_page_anim_cb);
    ui_anim_set_values(&__this->anim, start, end);
    ui_anim_set_time(&__this->anim, 200);
    ui_anim_set_ready_cb(&__this->anim, ui_return_page_anim_ready);
    ui_anim_start(&__this->anim);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_touch 返回跟手
 *
 * @param e
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_return_page_effect_touch(struct element_touch_event *e)
{
    /* printf("%s status:%d event:%d", __func__, __this->effect_status, e->event); */
    if (__this->effect_status == PAGE_SWITCH_STATUS_EXIT) {
        switch (e->event) {
        case ELM_EVENT_TOUCH_MOVE:
            int xoffset = e->pos.x - __this->last_pos_x;
            int yoffset = e->pos.y - __this->last_pos_y;
            ui_return_page_effect_move(xoffset, yoffset);
            break;
        case ELM_EVENT_TOUCH_UP:
            ui_return_page_effect_free();
            break;
        default:
            break;
        }
        __this->last_pos_x = e->pos.x;
        __this->last_pos_y = e->pos.y;
        return true;
    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_in_move 返回动画状态
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_return_page_effect_in_move()
{
    return __this->effect_status ? true : false;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_page_switch_effect_stop 页面特效stop
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_page_switch_effect_stop()
{
    if (__this->effect_status == PAGE_SWITCH_STATUS_ENTER) {
        menu_list_anim_ready_cb(NULL);
    } else if (__this->effect_status == PAGE_SWITCH_STATUS_EXIT) {
        int start = __this->x_offset;
        int end, var;

        if (start < 20) {
            end = 0;
            var = ui_get_current_window_id();
        } else {
            end = __this->win_rect.width;
            var = __this->target_page;
        }
        __this->target_page = var;
        ui_return_page_anim_ready(NULL);
    }
    return 0;
}


#endif
#endif


