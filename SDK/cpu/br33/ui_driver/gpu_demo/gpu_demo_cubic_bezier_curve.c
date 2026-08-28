#include "gpu_demo.h"
#include "ui_draw/ui_math.h"
#include "ui/lcd/lcd_drive.h"      /*! 引入lcd接口模块   */

#if (defined GPU_DEMO_CUBIC_BEZIER && GPU_DEMO_CUBIC_BEZIER)
static int task_id = 1;

void draw_cubic_curve(jl_path_t *path, int x_offset, int y_offset, int width, int height, u16 *data, u16 len)
{
    int chart_width = width;
    int chart_height = height;
    int i;

    printf("x : %d, y : %d, width : %d, height : %d, chart_width : %d, chart_height : %d\n", x_offset, y_offset, width, height, chart_width, chart_height);

    // 找到心率数据的最大值和最小值
    float min_hr = 1000.0f, max_hr = 0.0f;
    int data_count = len;

    for (i = 0; i < data_count; i++) {
        u16 hr = data[i];
        if (hr < min_hr) {
            min_hr = hr;
        }
        if (hr > max_hr) {
            max_hr = hr;
        }
    }

    min_hr = 0;
    max_hr = height;

    // 添加垂直缩放余量
    int range = max_hr - min_hr;
    min_hr -= range * 0.1;
    max_hr += range * 0.1;
    range = max_hr - min_hr;
    printf("min_hr = %f, max_hr = %f, range = %d\n", min_hr, max_hr, range);

    float *x_points = (float *)malloc(len * sizeof(float));
    float *y_points = (float *)malloc(len * sizeof(float));
    for (i = 0; i < data_count; i++) {
        x_points[i] = (i * 1.0f / (data_count - 1)) * chart_width;
        y_points[i] = chart_height * (1.0 - (data[i] - min_hr) / range);
        printf("%d : (%f, %f)\n", i, x_points[i], y_points[i]);
    }

    jl_path_move_to(path, x_offset, y_offset);
    jl_path_line_to(path, x_offset + x_points[0], y_offset + y_points[0]);

    for (i = 0; i < data_count - 1; i++) {
        float cx1;
        float cy1;
        float cx2;
        float cy2;
        if (i == 0) {
            // 第一段：使用简单的控制点
            cx1 = x_points[0] + (x_points[1] - x_points[0]) / 3;
            cy1 = y_points[0] + (y_points[1] - y_points[0]) / 3;
            cx2 = x_points[1] - (x_points[2] - x_points[0]) / 6;
            cy2 = y_points[1] - (y_points[2] - y_points[0]) / 6;
        } else if (i == (data_count - 2)) {
            // 最后一段：使用简单的控制点
            cx1 = x_points[i] + (x_points[i + 1] - x_points[i - 1]) / 6;
            cy1 = y_points[i] + (y_points[i + 1] - y_points[i - 1]) / 6;
            cx2 = x_points[i + 1] - (x_points[i + 1] - x_points[i]) / 3;
            cy2 = y_points[i + 1] - (y_points[i + 1] - y_points[i]) / 3;
        } else {
            // 中间段：使用Catmull-Rom样条控制点计算，使曲线更平滑
            cx1 = x_points[i] + (x_points[i + 1] - x_points[i - 1]) / 6;
            cy1 = y_points[i] + (y_points[i + 1] - y_points[i - 1]) / 6;
            cx2 = x_points[i + 1] - (x_points[i + 2] - x_points[i]) / 6;
            cy2 = y_points[i + 1] - (y_points[i + 2] - y_points[i]) / 6;
        }

        jl_path_cubic_to(path, x_offset + cx1, y_offset + cy1, x_offset + cx2, y_offset + cy2, x_offset + x_points[i + 1], y_offset + y_points[i + 1]);
    }

    jl_path_line_to(path, x_offset + width - 0, y_offset + y_points[data_count - 1]);
    jl_path_line_to(path, x_offset + width - 0, y_offset + height - 0);
    jl_path_line_to(path, x_offset, y_offset + height - 0);
    jl_path_line_to(path, x_offset, y_offset);

    free(x_points);
    free(y_points);
}

pJLGPUChainHead_t draw_cubic_bezier_curve_by_path(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, int x, int y, int w, int h, int curve_x, int curve_y, int curve_w, int curve_h, u16 *data, u16 len,  u32 fill_color)
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
    draw_cubic_curve(path, curve_x, curve_y, curve_w, curve_h, data, len);
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


pJLGPUChainHead_t draw_cubic_bezier_curve_by_path_with_linear_gradient(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, int x, int y, int w, int h, int curve_x, int curve_y, int curve_w, int curve_h, u16 *data, u16 len,  u32 fill_color)
{
    jlgpu_task_gradient_param_init(task_id/* GPU_FILL_TASK_ID*/, elm_id/* GPU_FILL_ELEMENT */);

    jlgpu_task_gradient_set_quality(JLGPU_RENDERING_QUALITY_MSAA_8X);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定使能缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    static uint32_t clut[] = {
        ARGB8888(0xff, 242, 66, 51),
        ARGB8888(0xff, 246, 101, 89),
        ARGB8888(0xff, 252, 231, 229),
    };
    /* 设置渐变任务颜色表，颜色表格式参考jlgpu_def.h的jlgpu_clut_format_t定义 */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式，参考jlgpu_def.h的jlgpu_spread_mode_t定义，但不允许JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 由于启动了变换功能，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x, y + h);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    jl_path_t *path = jl_path_create();
    draw_cubic_curve(path, curve_x, curve_y, curve_w, curve_h, data, len);
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

pJLGPUChainHead_t gpu_demo_draw_cubic_bezier(pJLGPUChainHead_t head, struct rect *screen)
{
    task_id = 1;

    struct lcd_info lcd_info_t;
    struct lcd_interface *lcd = lcd_get_hdl();
    lcd->get_screen_info(&lcd_info_t);

    int half_width = lcd_info_t.width / 2;
    int half_height = lcd_info_t.height / 2;
    float ratio = (float)half_width / 160;

    u16 heart_rate_data[] = {270, 212, 255, 232, 242, 219, 190, 222, 185, 250};

    /* draw_cubic_bezier_curve_by_path(head, screen, task_id++, 1,  */
    /* 0, 0, lcd_info_t.width, lcd_info_t.height,  */
    /* 0, half_width, lcd_info_t.width, half_width,  */
    /* heart_rate_data, sizeof(heart_rate_data) / sizeof(heart_rate_data[0]), 0xffff0000); */

    draw_cubic_bezier_curve_by_path_with_linear_gradient(head, screen, task_id++, 1,
            0, 0, lcd_info_t.width, lcd_info_t.height,
            0, half_width, lcd_info_t.width, half_width,
            heart_rate_data, sizeof(heart_rate_data) / sizeof(heart_rate_data[0]), 0xffff0000);

    return head;
}
#endif
