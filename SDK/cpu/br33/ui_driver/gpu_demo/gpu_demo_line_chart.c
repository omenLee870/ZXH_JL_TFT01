#include "gpu_demo.h"
#include "ui_draw/ui_math.h"
#include "ui/lcd/lcd_drive.h"      /*! 引入lcd接口模块   */

#if (defined GPU_DEMO_LINE_CHART && GPU_DEMO_LINE_CHART)
static int task_id = 1;

float sqrt_to_float(const sqrt_res_t *q)
{
    return (float)q->i + (float)q->f / 256.0f;
}

float vectorLength(int dx, int dy)
{
    u32 x = dx * dx + dy * dy;
    sqrt_res_t res;
    u32 mask = 1;
    while (mask * mask <= x) {
        mask <<= 1;
    }
    mask >>= 1;
    _sqrt(x, &res, mask);
    return sqrt_to_float(&res);
    /* return Math.sqrt(dx * dx + dy * dy); */
}

void drawLineWidth(jl_path_t *path, u16 x1, u16 y1, u16 x2, u16 y2, u8 lineWidth)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    float len = vectorLength(dx, dy);

    if (len == 0) {
        printf("两点重合，无法绘制");
        return;
    }

    // const path = jl_path_create();
    float radius = lineWidth / 2;

    // 计算方向
    float nx = dx / len;
    float ny = dy / len;

    // 计算矩形两侧的平行线
    float perpX = -dy / len * radius;
    float perpY = dx / len * radius;

    float x3 = x1 + perpX;
    float y3 = y1 + perpY;
    float x4 = x2 + perpX;
    float y4 = y2 + perpY;
    float x5 = x1 - perpX;
    float y5 = y1 - perpY;
    float x6 = x2 - perpX;
    float y6 = y2 - perpY;

    // 绘制矩形部分
    jl_path_move_to(path, x3, y3);
    jl_path_line_to(path, x4, y4);
    jl_path_line_to(path, x6, y6);
    jl_path_line_to(path, x5, y5);
    jl_path_line_to(path, x3, y3);
}

pJLGPUChainHead_t draw_line_chart(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, int x, int y, int w, int h, u16 coordinates[][2], u16 coordinates_num, u8 line_width, u32 fill_color)
{
    int i;
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
    for (i = 0; i < coordinates_num - 1; i++) {
        printf("%d : (%d, %d)\n", i, coordinates[i][0], coordinates[i][1]);
        drawLineWidth(path, coordinates[i][0], coordinates[i][1], coordinates[i + 1][0], coordinates[i + 1][1], line_width);
    }
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

pJLGPUChainHead_t draw_line_chart_endpoint(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, int x, int y, int w, int h, u16 coordinates[][2], u16 coordinates_num, u8 line_width, u32 fill_color)
{
    int i;
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
    for (i = 0; i < coordinates_num; i++) {
        printf("%d : (%d, %d)\n", i, coordinates[i][0], coordinates[i][1]);
        jl_path_append_circle(path, coordinates[i][0], coordinates[i][1], line_width + 2, line_width + 2);
    }
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

pJLGPUChainHead_t draw_line_chart_by_path(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, int x, int y, int w, int h, u16 coordinates[][2], u16 coordinates_num, u8 line_width, u32 fill_color)
{
    draw_line_chart(head, screen, task_id, elm_id, x, y, w, h, coordinates, coordinates_num, line_width, fill_color);
    draw_line_chart_endpoint(head, screen, task_id + 1, elm_id, x, y, w, h, coordinates, coordinates_num, line_width, fill_color);
    return head;
}


pJLGPUChainHead_t gpu_demo_draw_line_chart(pJLGPUChainHead_t head, struct rect *screen)
{
    task_id = 1;
    u16 coordinates[][2] = {
        {50, 50},
        {80, 100},
        {150, 50},
        {200, 130},
        {200, 200},
        {100, 200},
        {100, 300},
        {200, 330},
        {240, 280},
        {240, 200},
        {240, 150},
        {260, 130},
        {290, 130},
        {310, 150},
        {310, 350},
    };

    struct lcd_info lcd_info_t;
    struct lcd_interface *lcd = lcd_get_hdl();
    lcd->get_screen_info(&lcd_info_t);

    int half_width = lcd_info_t.width / 2;
    int half_height = lcd_info_t.height / 2;
    int xoffset = half_width / 2;
    int yoffset = half_height / 2;
    float ratio_w = (float)half_width / 320;
    float ratio_h = (float)half_height / 386;

    for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
        printf("%d : (%d, %d)\n", i, coordinates[i][0], coordinates[i][1]);
        coordinates[i][0] = xoffset + coordinates[i][0] * ratio_w;
        coordinates[i][1] = yoffset + coordinates[i][1] * ratio_h;
        printf("%d : (%d, %d)\n", i, coordinates[i][0], coordinates[i][1]);
    }

    draw_line_chart_by_path(head, screen, task_id++, 1, 0, 0, lcd_info_t.width, lcd_info_t.height, coordinates, sizeof(coordinates) / sizeof(coordinates[0]), 6, 0xffff0000);

    return head;
}
#endif
