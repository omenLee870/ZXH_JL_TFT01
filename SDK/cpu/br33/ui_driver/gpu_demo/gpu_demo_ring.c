#include "gpu_demo.h"
#include "asm/math_fast_function.h"
#include "ui_draw/ui_math.h"
#include "ui/lcd/lcd_drive.h"      /*! 引入lcd接口模块   */

static int task_id = 1;

int get_task_id()
{
    return task_id++;
}

#if (defined GPU_DEMO_RING_RADIAL_ANGLE_PATH && GPU_DEMO_RING_RADIAL_ANGLE_PATH)
void gpu_demo_create_radial_angle_grad_task_custom(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, int x, int y, int w, int h, int cx, int cy, int ir, int or, int start_angle, int end_angle, u32 color)
{
    /* 渐变任务参数初始化 */
    jlgpu_task_gradient_param_init(task_id/* GPU_GRAD_TASK_ID */, elm_id/*GPU_GRAD_ELEMENT*/);

    /* 设置渐变任务的任务类型为径向+角度渐变 */
    jlgpu_task_gradient_set_grad_type(JLGPU_DRAW_RADIALGRAD_ANGLEGRAD);

    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    jlgpu_task_gradient_set_quality(JLGPU_RENDERING_QUALITY_MSAA_8X);

    /* 渐变任务颜色表 */
    /* static uint32_t clut[] = { */
    /* ARGB8888(0xff, 0xff, 0x00, 0x00), */
    /* ARGB8888(0xff, 0xff, 0x00, 0x00), */
    /* ARGB8888(0xff, 0xff, 0x00, 0x00), */
    /* }; */

    uint32_t *clut = (uint32_t *)malloc(sizeof(uint32_t) * 3);
    extern int gpu_buf_add(u8 * buf, u32 size);
    gpu_buf_add((u8 *)clut, sizeof(uint32_t) * 3);
    clut[0] = 0xffffffff;
    clut[1] = 0xffffffff;
    clut[2] = 0xffffffff;

    /* printf("%s() - %d, %08x, %08x, %08x\n", __func__, __LINE__, clut[0], clut[1], clut[2]); */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    uint32_t *radial_clut = (uint32_t *)malloc(sizeof(uint32_t) * 3);
    extern int gpu_buf_add(u8 * buf, u32 size);
    gpu_buf_add((u8 *)radial_clut, sizeof(uint32_t) * 3);
    radial_clut[0] = color;
    radial_clut[1] = color;
    radial_clut[2] = color;
    jlgpu_task_gradient_set_radial_clut(radial_clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变的填充默认 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    /* int w = GET_W(GPU_GRAD_DRAW_W); */
    /* int h = GET_H(GPU_GRAD_DRAW_H); */

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    /* int x = GET_X((rect_width(screen) - w) / 2); */
    /* int y = GET_Y((rect_height(screen) - h) / 2); */

    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 设置径向渐变的圆环参数 */
    /* int cx = x + w / 2; */
    /* int cy = y + h / 2; */
    /* int ir = w / 4; */
    /* int or = w / 2; */
    jlgpu_task_gradient_set_ring(cx, cy, ir, or);

    /* 设置角度渐变参数 */
    jlgpu_task_gradient_set_angle(cx, cy, start_angle/* GPU_GRAD_S_ANGLE */, end_angle/* GPU_GRAD_E_ANGLE */);

    /* 复合指令时，只需设置径向渐变的模式，根据径向渐变的参数设置 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RING_GRADIENT);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, task_id/* GPU_GRAD_TASK_ID */, elm_id/* GPU_GRAD_ELEMENT */, &task_param);
}


pJLGPUChainHead_t gpu_demo_create_fill_path_task_custom(pJLGPUTaskHead_t head, struct rect *screen, u32 task_id, u32 elm_id, float x, float y, float w, float h, u32 fill_color)
{
    /* 填充颜色ARGB8888 */
    /* u32 fill_color = 0xffff0000;//GPU_FILL_COLOR; */
    u8 a = ARGB8888_A(fill_color);
    u8 r = ARGB8888_R(fill_color);
    u8 g = ARGB8888_G(fill_color);
    u8 b = ARGB8888_B(fill_color);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    printf("%s() - %d, %02x%02x%02x%02x\n", __func__, __LINE__, a, r, g, b);
    jlgpu_task_fill_param_init(task_id/* GPU_FILL_TASK_ID*/, elm_id/* GPU_FILL_ELEMENT */, a, r, g, b);

    jlgpu_task_gradient_set_quality(JLGPU_RENDERING_QUALITY_MSAA_8X);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考。
     */
    /* int fill_draw_w = 50;//GPU_FILL_DRAW_W; */
    /* int fill_draw_h = 250;//GPU_FILL_DRAW_H; */
    /* int w = GET_W(fill_draw_w); */
    /* int h = GET_H(fill_draw_h); */
    /* int fill_draw_w = w; */
    /* int fill_draw_h = h; */

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将填充设置到屏幕中心：坐标 = （屏幕尺寸-填充尺寸）/2
     */
    /* int x = GET_X((rect_width(screen) - fill_draw_w) / 2); */
    /* int y = GET_Y((rect_height(screen) - fill_draw_h) / 2); */

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w + 1, h + 1);

    /*
       设置填充任务的限制区域，一般等于父控件区域。
       填充实际渲染区域为jlgpu_task_fill_set_draw、jlgpu_task_fill_set_area与path区域的交集。
     */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径，path不带变换时，要主动偏移到渲染位置 */
    jl_path_append_rect(path, 0 + x, 0 + y, w, h, h / 2);
    /* jl_path_dump(path); */
    /* 添加路径结束符，矢量路径必须添加结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, task_id/* GPU_FILL_TASK_ID */, elm_id/* GPU_FILL_ELEMENT */, &task_param);

    return head;
}

pJLGPUChainHead_t gpu_demo_draw_ring_by_radial_angle_and_path(pJLGPUChainHead_t head, struct rect *screen_rect, int w, int h, int small_radius, int big_radius, int start_angle, int end_angle, u32 backcolor, u32 fillcolor, u8 percent)
{
    /* static int percent = 50; */
    /* static int dir = 1; */
    /* int w = 320;//128; */
    /* int h = 320;//128; */
    int x = GET_X((rect_width(screen_rect) - w) / 2);
    int y = GET_Y((rect_height(screen_rect) - h) / 2);
    int cx = x + w / 2;
    int cy = y + h / 2;
    /* int small_radius = 120;//32; */
    /* int big_radius = 160;//64; */
    /* int start_angle = 135; */
    /* int end_angle = 405; */
    int curr_angle = start_angle + (end_angle - start_angle) * percent / 100;
    int middle_radius = (small_radius + big_radius) / 2;
    int endpoint_radius = (big_radius - small_radius) / 2;

    float sc_cx;
    float sc_cy;
    float ec_cx;
    float ec_cy;


    printf("[%d, %d, %d x %d], x : %d, y : %d, w : %d, h : %d, cx : %d, cy : %d, middle_radius : %d, start_angle : %d, end_angle : %d\n",
           screen_rect->left, screen_rect->top, screen_rect->width, screen_rect->height, x, y, w, h, cx, cy, middle_radius, start_angle, end_angle);

    gpu_demo_create_radial_angle_grad_task_custom(head, screen_rect, task_id++, 1, x, y, w, h, cx, cy, small_radius, big_radius, start_angle, end_angle, backcolor);
    sc_cx = cx + middle_radius * _trigo_cos(start_angle) * 1.0f / 32768;
    sc_cy = cy + middle_radius * _trigo_sin(start_angle) * 1.0f / 32768;
    /* printf("start circle (%f, %f)\n", sc_cx, sc_cy); */
    gpu_demo_create_fill_path_task_custom(head, screen_rect, task_id++, 1, sc_cx - endpoint_radius, sc_cy - endpoint_radius, endpoint_radius * 2, endpoint_radius * 2, backcolor);
    ec_cx = cx + middle_radius * _trigo_cos(end_angle) * 1.0f / 32768;
    ec_cy = cy + middle_radius * _trigo_sin(end_angle) * 1.0f / 32768;
    /* printf("end circle (%f, %f)\n", ec_cx, ec_cy); */
    gpu_demo_create_fill_path_task_custom(head, screen_rect, task_id++, 1, ec_cx - endpoint_radius, ec_cy - endpoint_radius, endpoint_radius * 2, endpoint_radius * 2, backcolor);

#if 1
    gpu_demo_create_radial_angle_grad_task_custom(head, screen_rect, task_id++, 1, x, y, w, h, cx, cy, small_radius, big_radius, start_angle, curr_angle, fillcolor);
    sc_cx = cx + middle_radius * _trigo_cos(start_angle) * 1.0f / 32768;
    sc_cy = cy + middle_radius * _trigo_sin(start_angle) * 1.0f / 32768;
    gpu_demo_create_fill_path_task_custom(head, screen_rect, task_id++, 1, sc_cx - endpoint_radius, sc_cy - endpoint_radius, endpoint_radius * 2, endpoint_radius * 2, fillcolor);
    ec_cx = cx + middle_radius * _trigo_cos(curr_angle) * 1.0f / 32768;
    ec_cy = cy + middle_radius * _trigo_sin(curr_angle) * 1.0f / 32768;
    gpu_demo_create_fill_path_task_custom(head, screen_rect, task_id++, 1, ec_cx - endpoint_radius, ec_cy - endpoint_radius, endpoint_radius * 2, endpoint_radius * 2, fillcolor);
#endif
    return head;
}

pJLGPUChainHead_t draw_3ring_by_radial_angle_and_path(pJLGPUChainHead_t head, struct rect *screen, float start_angle_deg, float end_angle_deg, u32 backcolor,
        float c0_inner_radius, float c0_outer_radius, u32 c0_fill_color, u8 c0_percent,
        float c1_inner_radius, float c1_outer_radius, u32 c1_fill_color, u8 c1_percent,
        float c2_inner_radius, float c2_outer_radius, u32 c2_fill_color, u8 c2_percent)
{
    gpu_demo_draw_ring_by_radial_angle_and_path(head, screen, c0_outer_radius * 2, c0_outer_radius * 2, c0_inner_radius, c0_outer_radius, start_angle_deg, end_angle_deg, backcolor, c0_fill_color, c0_percent);
    gpu_demo_draw_ring_by_radial_angle_and_path(head, screen, c1_outer_radius * 2, c1_outer_radius * 2, c1_inner_radius, c1_outer_radius, start_angle_deg, end_angle_deg, backcolor, c1_fill_color, c1_percent);
    gpu_demo_draw_ring_by_radial_angle_and_path(head, screen, c2_outer_radius * 2, c2_outer_radius * 2, c2_inner_radius, c2_outer_radius, start_angle_deg, end_angle_deg, backcolor, c2_fill_color, c2_percent);
    return head;
}

pJLGPUChainHead_t gpu_demo_draw_ring_radial_angle_path(pJLGPUChainHead_t head, struct rect *screen)
{
    task_id = 1;

    struct lcd_info lcd_info_t;
    struct lcd_interface *lcd = lcd_get_hdl();
    lcd->get_screen_info(&lcd_info_t);

    int half_width = lcd_info_t.width / 2;
    int half_height = lcd_info_t.height / 2;
    float ratio = (float)half_width / 160;

    draw_3ring_by_radial_angle_and_path(head, screen, 135, 405, 0xff7f7f7f,
                                        20 * ratio,  60 * ratio,  0xff0000ff, 30,
                                        70 * ratio,  110 * ratio, 0xff00ff00, 60,
                                        120 * ratio, 160 * ratio, 0xffff0000, 90);

    return head;
}
#endif


#if (defined GPU_DEMO_RING_PATH && GPU_DEMO_RING_PATH)
pJLGPUChainHead_t gpu_demo_draw_rounded_arc_by_path(pJLGPUChainHead_t head, struct rect *screen, u32 task_id, u32 elm_id, float center_x, float center_y, float radius, float line_width, float start_angle_deg, float end_angle_deg, u32 fill_color)
{
    /* 填充颜色ARGB8888 */
    /* u32 fill_color = 0xffff0000;//GPU_FILL_COLOR; */
    u8 a = ARGB8888_A(fill_color);
    u8 r = ARGB8888_R(fill_color);
    u8 g = ARGB8888_G(fill_color);
    u8 b = ARGB8888_B(fill_color);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    printf("%s() - %d, %02x%02x%02x%02x\n", __func__, __LINE__, a, r, g, b);
    jlgpu_task_fill_param_init(task_id/* GPU_FILL_TASK_ID*/, elm_id/* GPU_FILL_ELEMENT */, a, r, g, b);

    jlgpu_task_gradient_set_quality(JLGPU_RENDERING_QUALITY_MSAA_8X);

    /* =================================<< PATH配置开始 >>================================= */
    /* float center_x = 160; */
    /* float center_y = 193; */
    /* float radius = 120; */
    /* float line_width = 50; */
    /* float start_angle_deg = 135;  */
    /* float end_angle_deg = 405; */
    printf("center_x : %f, center_y : %f, radius : %f, line_width : %f, start_angle_deg : %f, end_angle_deg : %f\n", center_x, center_y, radius, line_width, start_angle_deg, end_angle_deg);


#define PI 3.1415926535897932384626433832795f
    float start_angle_rad = start_angle_deg * PI / 180;
    float end_angle_rad = end_angle_deg * PI / 180;
    float inner_radius = radius - line_width / 2;
    float outer_radius = radius + line_width / 2;
    printf("start_angle_rad : %f, end_angle_rad : %f, inner_radius : %f, outer_radius : %f\n", start_angle_rad, end_angle_rad, inner_radius, outer_radius);

    // 配置填充区域以及限制区域(往外扩充一个像素，避免浮点数导致的截边)
    int fill_x = (center_x - outer_radius) - 1;
    int fill_y = (center_y - outer_radius) - 1;
    int fill_w = (outer_radius * 2) + 2;
    int fill_h = (outer_radius * 2) + 2;
    jlgpu_task_fill_set_draw(fill_x, fill_y, fill_w, fill_h);
    jlgpu_task_fill_set_area(fill_x, fill_y, fill_w, fill_h);

    float start_cx = center_x + radius * _trigo_cos(start_angle_deg) * 1.0f / 32768;
    float start_cy = center_y + radius * _trigo_sin(start_angle_deg) * 1.0f / 32768;
    float end_cx = center_x + radius * _trigo_cos(end_angle_deg) * 1.0f / 32768;
    float end_cy = center_y + radius * _trigo_sin(end_angle_deg) * 1.0f / 32768;
    printf("start_cx : %f, start_cy : %f, end_cx : %f, end_cy : %f\n", start_cx, start_cy, end_cx, end_cy);

    float inner_start_cx = center_x + inner_radius * _trigo_cos(start_angle_deg) * 1.0f / 32768;
    float inner_start_cy = center_y + inner_radius * _trigo_sin(start_angle_deg) * 1.0f / 32768;
    float inner_end_cx = center_x + inner_radius * _trigo_cos(end_angle_deg) * 1.0f / 32768;
    float inner_end_cy = center_y + inner_radius * _trigo_sin(end_angle_deg) * 1.0f / 32768;
    printf("inner_start_cx : %f, inner_start_cy : %f, inner_end_cx : %f, inner_end_cy : %f\n", inner_start_cx, inner_start_cy, inner_end_cx, inner_end_cy);

    float outer_start_cx = center_x + outer_radius * _trigo_cos(start_angle_deg) * 1.0f / 32768;
    float outer_start_cy = center_y + outer_radius * _trigo_sin(start_angle_deg) * 1.0f / 32768;
    float outer_end_cx = center_x + outer_radius * _trigo_cos(end_angle_deg) * 1.0f / 32768;
    float outer_end_cy = center_y + outer_radius * _trigo_sin(end_angle_deg) * 1.0f / 32768;
    printf("outer_start_cx : %f, outer_start_cy : %f, outer_end_cx : %f, outer_end_cy : %f\n", outer_start_cx, outer_start_cy, outer_end_cx, outer_end_cy);

    jl_path_t *path = jl_path_create();
    jl_path_move_to(path, inner_start_cx, inner_start_cy);
    jl_path_line_to(path, outer_start_cx, outer_start_cy);
    jl_path_append_arc(path, center_x, center_y, outer_radius, start_angle_deg, end_angle_deg - start_angle_deg, 0);
    jl_path_line_to(path, inner_end_cx, inner_end_cy);
    jl_path_append_arc(path, center_x, center_y, inner_radius, end_angle_deg, start_angle_deg - end_angle_deg, 0);
    jl_path_append_circle(path, start_cx, start_cy, line_width / 2, line_width / 2);
    jl_path_append_circle(path, end_cx, end_cy, line_width / 2, line_width / 2);
    /* 添加路径结束符，矢量路径必须添加结束符 */
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


pJLGPUChainHead_t gpu_demo_draw_ring_by_path(pJLGPUChainHead_t head, struct rect *screen, float center_x, float center_y, float start_angle_deg, float end_angle_deg, u32 backcolor,
        float inner_radius, float outer_radius, u32 fill_color, u8 percent)
{
    int linewidth = outer_radius - inner_radius;
    int curr_end_angle_deg = start_angle_deg + (end_angle_deg - start_angle_deg) * percent / 100;
    gpu_demo_draw_rounded_arc_by_path(head, screen, task_id++, 1, center_x, center_y, inner_radius + linewidth / 2, linewidth, start_angle_deg, end_angle_deg, backcolor);
    gpu_demo_draw_rounded_arc_by_path(head, screen, task_id++, 1, center_x, center_y, inner_radius + linewidth / 2, linewidth, start_angle_deg, curr_end_angle_deg, fill_color);

    return head;
}

pJLGPUChainHead_t gpu_demo_draw_3ring_by_path(pJLGPUChainHead_t head, struct rect *screen, float center_x, float center_y, float start_angle_deg, float end_angle_deg, u32 backcolor,
        float c0_inner_radius, float c0_outer_radius, u32 c0_fill_color, u8 c0_percent,
        float c1_inner_radius, float c1_outer_radius, u32 c1_fill_color, u8 c1_percent,
        float c2_inner_radius, float c2_outer_radius, u32 c2_fill_color, u8 c2_percent)
{
    int c0_linewidth = c0_outer_radius - c0_inner_radius;
    int c0_end_angle_deg = start_angle_deg + (end_angle_deg - start_angle_deg) * c0_percent / 100;
    gpu_demo_draw_rounded_arc_by_path(head, screen, task_id++, 1, center_x, center_y, c0_inner_radius + c0_linewidth / 2, c0_linewidth, start_angle_deg, end_angle_deg, backcolor);
    gpu_demo_draw_rounded_arc_by_path(head, screen, task_id++, 1, center_x, center_y, c0_inner_radius + c0_linewidth / 2, c0_linewidth, start_angle_deg, c0_end_angle_deg, c0_fill_color);

    int c1_linewidth = c1_outer_radius - c1_inner_radius;
    int c1_end_angle_deg = start_angle_deg + (end_angle_deg - start_angle_deg) * c1_percent / 100;
    gpu_demo_draw_rounded_arc_by_path(head, screen, task_id++, 1, center_x, center_y, c1_inner_radius + c1_linewidth / 2, c1_linewidth, start_angle_deg, end_angle_deg, backcolor);
    gpu_demo_draw_rounded_arc_by_path(head, screen, task_id++, 1, center_x, center_y, c1_inner_radius + c1_linewidth / 2, c1_linewidth, start_angle_deg, c1_end_angle_deg, c1_fill_color);

    int c2_linewidth = c2_outer_radius - c2_inner_radius;
    int c2_end_angle_deg = start_angle_deg + (end_angle_deg - start_angle_deg) * c2_percent / 100;
    gpu_demo_draw_rounded_arc_by_path(head, screen, task_id++, 1, center_x, center_y, c2_inner_radius + c2_linewidth / 2, c2_linewidth, start_angle_deg, end_angle_deg, backcolor);
    gpu_demo_draw_rounded_arc_by_path(head, screen, task_id++, 1, center_x, center_y, c2_inner_radius + c2_linewidth / 2, c2_linewidth, start_angle_deg, c2_end_angle_deg, c2_fill_color);

    return head;
}



pJLGPUChainHead_t gpu_demo_draw_ring_path(pJLGPUChainHead_t head, struct rect *screen)
{
    task_id = 1;

    // 单圆环
    /* gpu_demo_draw_ring_by_path(head, screen, 163, 193, 135, 405, 0xff7f7f7f, 120, 160, 0xffff0000, 90); */

    struct lcd_info lcd_info_t;
    struct lcd_interface *lcd = lcd_get_hdl();
    lcd->get_screen_info(&lcd_info_t);

    int half_width = lcd_info_t.width / 2;
    int half_height = lcd_info_t.height / 2;
    float ratio = (float)half_width / 160;

    // 三圆环
    gpu_demo_draw_3ring_by_path(head, screen, half_width, half_height, 135, 405, 0xff7f7f7f,
                                20 * ratio,  60 * ratio,  0xff0000ff, 30,
                                70 * ratio,  110 * ratio, 0xff00ff00, 60,
                                120 * ratio, 160 * ratio, 0xffff0000, 90);

    return head;
}
#endif
