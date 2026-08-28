/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#include "../gpu_demo.h"

#if (defined GPU_DEMO_ANIM_FLIP && GPU_DEMO_ANIM_FLIP)

#include "../img_data/img_data.h"

#include "rect.h"


#define FLIP_MIN_DEGREE		0.0f
#define FLIP_DEGREE_STEP	5.0f
#define FLIP_MAX_DEGREE		360.0f

struct gpu_demo_anim_flip {
    float degree; /*! 翻转角度 */
    float angle;  /*! 旋转角度 */
    float ratio;  /*! 缩放倍率 */
};
static struct gpu_demo_anim_flip anim_flip = {0};
#define __this (&anim_flip)


static void create_page_chain(pJLGPUChainHead_t head, struct rect *screen)
{
    int w = 128;
    int h = 128;

    jlgpu_task_fill_param_init(1, 1, 0xff, 0xff, 0x00, 0x00);

    jlgpu_task_fill_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    __this->ratio += 0.1;
    if (__this->ratio > 2.0) {
        __this->ratio = 0.1;
    }
    /* __this->ratio = 1.0; */
    jlgpu_task_fill_set_affine_scale(__this->ratio, __this->ratio);

    __this->angle += 1.0;
    if (__this->angle > 360) {
        __this->angle = 0;
    }
    /* __this->angle = 45 + 90; */
    jlgpu_task_fill_set_affine_rotate(w * __this->ratio / 2, h * __this->ratio / 2, __this->angle);

    int x = (rect_width(screen) - (w * __this->ratio)) / 2 + rect_left(screen);
    int y = (rect_height(screen) - (h * __this->ratio)) / 2 + rect_top(screen);
    jlgpu_task_fill_set_affine_translate(x, y);
    printf("%s() x: %d, y: %d, ratio: %f, angle: %f\n", __func__, x, y, __this->ratio, __this->angle);

    jlgpu_task_fill_set_draw(x, y, w, h);
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    /* 注意：使用变换时，路径path需从0坐标开始生成，GPU会根据矩阵平移参数控制渲染位置 */
    jl_path_append_rect(path, 0, 0, w, h, 20);
    /* 添加路径结束符，矢量路径必须添加结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量仿射变换，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    /* 设置仿射变换缩放 */
    jlgpu_task_path_set_affine_scale(__this->ratio, __this->ratio);
    /* 设置仿射变换旋转 */
    jlgpu_task_path_set_affine_rotate(w * __this->ratio / 2, h * __this->ratio / 2, __this->angle);
    /* 设置仿射变换平移 */
    jlgpu_task_path_set_affine_translate(x, y);
    /* =================================<< PATH配置结束 >>================================= */

    jlgpu_update_task_by_id(head, 1, 1, &task_param);
}

static void create_grad_chain(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(1, 1);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    /*
       设置渐变任务颜色表；
       颜色表格式参考jlgpu_def.h的jlgpu_clut_format_t定义
       clut_lvl对应颜色表中颜色数量：颜色数量 = 2 ^ clut_lvl + 1
     */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /*
       设置渐变扩展模式；
       参考jlgpu_def.h的jlgpu_spread_mode_t定义，但不允许JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO
     */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = 128;
    int h = 128;

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 由于启动了仿射变换功能，这里设置仿射变换的平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /*
       设置线性渐变参数；
       这里设置为渲染区域X方向从起始到结束坐标渐变
     */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    /* jl_path_t *path = jl_path_create(); */
    /* jl_path_append_rect(path, 0 + x, 0 + y, w, h, 20); */
    /* jl_path_end(path); */
    /* jlgpu_task_path_set_enable(); */
    /* jlgpu_task_path_set_path(path); */
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, 1, 1, &task_param);
}


void gpu_anim_flip_init()
{
    /* __this->degree = FLIP_MIN_DEGREE; */
}

void gpu_anim_flip_free()
{
    /* __this->degree = FLIP_MIN_DEGREE; */
}

pJLGPUChainHead_t gpu_anim_flip_draw(pJLGPUChainHead_t head, struct rect *screen)
{
    __this->degree += FLIP_DEGREE_STEP;
    if (__this->degree > FLIP_MAX_DEGREE) {
        __this->degree = FLIP_MIN_DEGREE;
    }
    /* __this->degree = 45; */
    printf("%s() degree: %f\n", __func__, __this->degree);

    /* create_page_chain(head, screen); #<{(|! 创建页面任务链 |)}># */
    create_grad_chain(head, screen);

    struct rect win;
    gpu_matrix_t matrix;
    gpu_matrix_set_identity(&matrix);
    jlgpu_get_win_rect(&win);

    /* 获取flip矩阵 */
    extern void get_flip_matrix(gpu_matrix_t *matrix, int screen_x, int screen_y, int screen_w, int screen_h, float degree);
    get_flip_matrix(&matrix, rect_left(&win), rect_top(&win), rect_width(&win), rect_height(&win), __this->degree);

    /* __DUMP_MATRIX(&matrix); */

    /* 创建为变换任务链 */
    pJLGPUTaskHead_t new_head = jlgpu_task_list_copy_create(NULL, head, NULL, 0);
    jlgpu_task_list_copy_mul_matrix_by_group(new_head, &matrix, 0);

    /* 删除原来任务链，防止内存泄露 */
    jlgpu_chain_delete_head(head);

    return new_head;
    /* return NULL; */
}



#endif // !GPU_DEMO_ANIM_CUBE


