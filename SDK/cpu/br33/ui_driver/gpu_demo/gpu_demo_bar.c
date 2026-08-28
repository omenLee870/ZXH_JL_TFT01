#include "gpu_demo.h"
#include "ui_draw/ui_math.h"
#include "ui/lcd/lcd_drive.h"      /*! 引入lcd接口模块   */

#if (defined GPU_DEMO_BAR && GPU_DEMO_BAR)
static int task_id = 1;

static pJLGPUChainHead_t draw_bar(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, int x, int y, int w, int h, u32 fill_color)
{
    u8 a = ARGB8888_A(fill_color);
    u8 r = ARGB8888_R(fill_color);
    u8 g = ARGB8888_G(fill_color);
    u8 b = ARGB8888_B(fill_color);

    jlgpu_task_fill_param_init(task_id/* GPU_FILL_TASK_ID*/, elm_id/* GPU_FILL_ELEMENT */, a, r, g, b);

    jlgpu_task_gradient_set_quality(JLGPU_RENDERING_QUALITY_MSAA_8X);

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w + 1, h + 1);

    /*
       设置填充任务的限制区域，一般等于父控件区域。
       填充实际渲染区域为jlgpu_task_fill_set_draw、jlgpu_task_fill_set_area与path区域的交集。
     */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    jl_path_t *path = jl_path_create();
    jl_path_append_rect(path, x, y, w, h, h);
    jl_path_end(path);

    /* jl_path_dump(path); */
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, task_id/* GPU_FILL_TASK_ID */, elm_id/* GPU_FILL_ELEMENT */, &task_param);

    return head;
}

pJLGPUChainHead_t draw_bar_by_path(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, int x, int y, int w, int h, u32 backcolor, u32 fillcolor, u8 percent)
{
    int curr_w = w * percent / 100;
    draw_bar(head, screen, task_id + 0, elm_id, x, y, w, h, backcolor);
    draw_bar(head, screen, task_id + 1, elm_id, x, y, curr_w, h, fillcolor);

    return head;
}

pJLGPUChainHead_t gpu_demo_draw_bar(pJLGPUChainHead_t head, struct rect *screen)
{
    task_id = 1;

    struct lcd_info lcd_info_t;
    struct lcd_interface *lcd = lcd_get_hdl();
    lcd->get_screen_info(&lcd_info_t);

    draw_bar_by_path(head, screen, task_id++, 1, (lcd_info_t.width - 200) / 2, (lcd_info_t.height - 50) / 2, 200, 50, 0xffc9e1f3, 0xff2196f3, 50);

    return head;
}
#endif
