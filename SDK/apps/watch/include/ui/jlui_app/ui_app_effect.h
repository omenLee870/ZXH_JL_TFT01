#ifndef __jlui_app_ui_effect__
#define __jlui_app_ui_effect__
#include "ui_core.h"
#include "jlui/ui_page_manager.h"
#include "jlui/ui_page_switch.h"

struct ui_page_draw {
    pJLGPUTaskHead_t *new_task_list;
    struct rect *rec_page_rect;
    struct rect *rec_gpu_rect;
    struct rect *rec_lcd_rect;
    u32 new_list_create;
    u32 normal_list_flag;
    u32 list_total;
};
struct ui_effect_module {
    int style;
    int (*ontouch)(struct element_touch_event *);
    int (*get_status)(void);
    void (*uninit)(void);
    int (*effect_draw)(pJLGPUMultTaskList_t mult_list, struct ui_page_draw *draw);
};
extern struct ui_effect_module ui_effect_module_begin[];
extern struct ui_effect_module ui_effect_module_end[];

#define REGISTER_UI_EFFECT_MODULE(mode) \
	const struct ui_effect_module ui_effect_mod_##mode \
		sec(.ui_effect_module) = {\
			.style = mode,


struct ui_effect_module *ui_effect_get_handle_by_style(int style);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_effect_set_alpha 修改透明度
 *
 * @param elm		控件句柄
 * @param alpha		透明度
 * @param child		是否更新子控件为同一透明度
 * @param redraw	是否刷新
 */
/* ------------------------------------------------------------------------------------*/
void ui_effect_set_alpha(struct element *elm, int alpha, int child, int redraw);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_ram_image_attrs_set 创建ram图像信息
 *
 * @param img
 * @param data
 * @param data_len
 * @param width
 * @param height
 * @param format
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_ram_image_attrs_set(struct ui_image_attrs *img, u8 *data, int data_len, int width, int height, int format);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_create_task_list_by_image 添加图片任务
 *
 * @param head
 * @param image_attr
 * @param xoffset
 * @param yoffset
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUTaskHead_t jlgpu_create_task_list_by_image(pJLGPUTaskHead_t head, struct ui_image_attrs *image_attr, int xoffset, int yoffset);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_create_task_list_by_image_set_blend_mode 创建图片任务链
 *
 * @param head
 * @param image_attr
 * @param xoffset
 * @param yoffset
 * @param blend_mode 指定blend方式
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUTaskHead_t jlgpu_create_task_list_by_image_set_blend_mode(pJLGPUTaskHead_t head, struct ui_image_attrs *image_attr, int xoffset, int yoffset, int blend_mode);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief menu_enter_app_anim 页面进入动画
 *
 * @param app_id	要进入的页面
 * @param touch_x	触点x
 * @param touch_y	触点y
 */
/* ------------------------------------------------------------------------------------*/
void menu_enter_app_anim(u32 app_id, int touch_x, int touch_y);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief menu_enter_app_state 状态判断
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int menu_enter_app_state();
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_set_pos 设置返回坐标
 *
 * @param pos_x
 * @param pos_y
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_set_pos(u16 pos_x, u16 pos_y);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_set_en 使能返回特效
 *
 * @param enable
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_set_en(u8 enable);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_set_win 记录需要从哪个页面返回
 *
 * @param window
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_set_win(u32 window);
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
void ui_return_page_effect_enable(u32 window, u16 pos_x, u16 pos_y, u8 enable);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_init 返回特效
 *
 * @param ret_page	返回页面
 *
 * @return 0 支持启动返回特效，-1 不支持返回特效
 */
/* ------------------------------------------------------------------------------------*/
int ui_return_page_effect_init(u32 ret_page);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_move 跟手滑动
 *
 * @param xoffset
 * @param yoffset
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_move(int xoffset, int yoffset);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_free 抬手动画
 */
/* ------------------------------------------------------------------------------------*/
void ui_return_page_effect_free();
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_touch 返回跟手
 *
 * @param e
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_return_page_effect_touch(struct element_touch_event *e);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_return_page_effect_in_move 返回动画状态
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_return_page_effect_in_move();

#endif
