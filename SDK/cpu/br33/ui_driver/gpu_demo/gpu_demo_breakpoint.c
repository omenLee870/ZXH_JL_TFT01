
#include "gpu_demo.h"

#if (defined GPU_DEMO_BREAKPOINT && GPU_DEMO_BREAKPOINT)

/* 707N 自定义绘图结构 */
typedef struct gpu_task_draw {
    void (*cb_func)(int id, u8 *dst_buf, struct rect *dst_r, struct rect *src_r, u8 bytes_per_pixel, void *priv, void *matrix);
    void (*cb_exit)(void *draw_info, void *priv);
    void *priv;
    int priv_len;
    int draw_id;
    struct rect task_r;
} JLGPUTaskDraw_t, *pJLGPUTaskDraw_t;

#define GPU_FILL_COLOR			ARGB8888(0xff, 0x00, 0xff, 0x00) /*! 颜色值定义   */

/* ***************************************************************************/
/**
 * \Brief :        创建填充任务，这里仅用于演示，非断点中断必须
 *
 * \Param :        head
 * \Param :        screen
 */
/* ***************************************************************************/
void draw_fill_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 填充颜色ARGB8888 */
    u8 a = ARGB8888_A(GPU_FILL_COLOR);
    u8 r = ARGB8888_R(GPU_FILL_COLOR);
    u8 g = ARGB8888_G(GPU_FILL_COLOR);
    u8 b = ARGB8888_B(GPU_FILL_COLOR);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    jlgpu_task_fill_param_init(2, 2, a, r, g, b);
    jlgpu_task_fill_set_draw(100, 100, 100, 100);
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));
    jlgpu_update_task_by_id(head, 2, 2, &task_param);
}


/* ***************************************************************************/
/**
 * \Brief :        断点中断回调函数形式
 *
 * \Param :        head 当前运行的任务链头指针
 * \Param :        taskp 当前正在运行的GPU指令指针
 * \Param :        priv 自定义参数，创建断点中断时传入的参数，在这个回调中会传回来
 */
/* ***************************************************************************/
static void gpu_breakpoint_callback(pJLGPUChainHead_t head, pJLGPUChainTaskUnit_t taskp)
{
    /* jlgpu_chain_dump_task_unit(taskp, 0); */

    /* 创建断点中断时，设置了custom_data参数，这里可以通过 jlgpu_chain_task_custom_data(taskp) 获取回来 */
    pJLGPUTaskDraw_t c_draw = (pJLGPUTaskDraw_t)jlgpu_chain_task_custom_data(taskp);

    printf("%s() %d, head: 0x%x, taskp: 0x%x, priv_data: 0x%x\n", __func__, __LINE__, (u32)head, (u32)taskp, (u32)c_draw);
}

/* ***************************************************************************/
/**
 * \Brief :        自定义数据释放回调函数，GPU释放指令时会调用
 *
 * \Param :        taskp 当前GPU任务
 * \Param :        priv 自定义数据custom_data
 */
/* ***************************************************************************/
static void gpu_custom_data_free(pJLGPUChainTaskUnit_t taskp, void *priv)
{
    /* printf("%s() %d, priv data: 0x%x\n", __func__, __LINE__, (u32)priv); */
    jlgpu_free(priv);
}

/* ***************************************************************************/
/**
 * \Brief :        断点中断DEMO开始
 */
/* ***************************************************************************/
void gpu_demo_breakpoint_init(void)
{
    jlgpu_chain_set_breakpoint_cb(gpu_breakpoint_callback);
    jlgpu_chain_set_custom_data_free_cb(gpu_custom_data_free);
}

/* ***************************************************************************/
/**
 * \Brief :        断点中断DEMO结束
 */
/* ***************************************************************************/
void gpu_demo_breakpoint_free(void)
{
    jlgpu_chain_set_breakpoint_cb(NULL);
    jlgpu_chain_set_custom_data_free_cb(NULL);
}

/* ***************************************************************************/
/**
 * \Brief :        断点中断DEMO回调
 *
 * \Param :        head 当前任务链头
 * \Param :        screen 当前屏幕位置
 *
 * \Return :        NULL
 */
/* ***************************************************************************/
pJLGPUChainHead_t gpu_demo_create_breakpoint_task(pJLGPUChainHead_t head, struct rect *screen)
{
    // 创建一个GPU指令，根据需求创建，一般情况下考虑在断点回调中处理具体工作，创建最小的填充指令最佳
    jlgpu_task_fill_param_init(1, 1, 0, 0, 0, 0);
    // 设置填充指令的渲染区域和限制区域，注意这里不能随便设置。需要保证不开断点时GPU指令能正常运行
    jlgpu_task_fill_set_draw(rect_left(screen), rect_top(screen), 1, 1);
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    // 构建自定义数据，可以用malloc或static变量，最终会在gpu_custom_data_free回调中传回应用层释放
    pJLGPUTaskDraw_t draw = (pJLGPUTaskDraw_t)jlgpu_malloc(sizeof(JLGPUTaskDraw_t));
    /* printf("%s() %d, priv data: 0x%x\n", __func__, __LINE__, (u32)draw); */

    jlgpu_task_breakpoint_enable();  /* 使能GPU指令的任务断点 */
    jlgpu_task_set_custom_data(draw);  /* 设置GPU指令的自定义数据 */

    jlgpu_update_task_by_id(head, 1, 1, &task_param);  /* 创建GPU指令并追加到head中 */

    /* 创建一个填充任务，非必须 */
    draw_fill_task(head, screen);

    return NULL;
}
#endif
