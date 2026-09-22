/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_demo_draw.c
 *
 * @brief GPU DEMO GPU渲染示例
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-01-06
 */
/* ------------------------------------------------------------------------------------*/

#include "gpu_demo.h"

#if (defined GPU_DEMO_ENABLE && GPU_DEMO_ENABLE)


#define DBG_PRINT_POS  /*! 使能所有 TREAP_UP，FUNC_POS，FILE_POS，REAL_POS 打印 */
#define DBG_PRINT_AREA /*! 使能所有 RECT，AREA 打印 */
#define DBG_PRINT_FUNC /*! 使能所有 FUNC_ENTER，FUNC_LEAVE，FUNC_FAILD打印 */
#include "ui_debug.h"  /*! 引入调试打印模块  */


#include "buffer_manager.h"        /*! 引入buf管理模块   */

#include "ui/lcd/lcd_drive.h"      /*! 引入lcd接口模块   */

/* #include "effect_manager.h" */

#include "clock_manager/clock_manager.h" /*! 引入时钟管理 */

#include "ui_measure.h"  /* 引入IO翻转模块 */



#define GPU_DEMO_TIME	50         /*! GPU DEMO 刷新间隔 */
#define GPU_DEMO_TASK	"gpu_demo" /*! GPU DEMO 线程名称 */
#define GPU_DEMO_BG		ARGB8888(0xff, 0xa0, 0xa0, 0xa0) /*! GPU DEMO 背景颜色 */

struct gpu_demo_prev {
    u32 refr_timer_id;             /*! 刷新定时器ID      */
    u16 demo_index;                /*! 当前DEMO索引      */
    u16 demo_number;               /*! DEMO列表数量      */
    u16 block_height;              /*! 显存buf分块高度   */

    OS_SEM buffer_sem;             /*! 显存buf信号量     */
    void *buffer_hdl;              /*! 显存buf句柄       */
    void *buffer_adr;              /*! 显存buf地址       */
    struct lcd_info info;          /*! LCD 屏幕信息      */
    struct lcd_interface *lcd;     /*! LCD 屏幕句柄      */
};
static struct gpu_demo_prev gpu_demo = {0};
#define __this	(&gpu_demo)

typedef enum {
    GPU_DEMO_MSG_OTHER,
    GPU_DEMO_MSG_REFR,
} gpu_demo_msg_t; /*! DEMO线程消息类型 */


/* @partA =================================<< 链表任务示例 >>================================= */
/* 创建GPU任务链表头 */
static pJLGPUChainHead_t gpu_demo_head_init(void)
{
    /* 创建GPU任务链，开始渲染时创建 */
    pJLGPUChainHead_t head = jlgpu_create_task_list_head();
    ASSERT(head);

    /* 设置链表输出格式 */
    int out_format = (__this->info.color_format == LCD_COLOR_RGB888) ? GPU_FORMAT_RGB888 : GPU_FORMAT_RGB565;
    jlgpu_set_task_list_out_format(head, out_format);
    return head;
}

/* 释放GPU任务链表头 */
static void gpu_demo_head_free(pJLGPUChainHead_t head)
{
    /* 销毁GPU任务链，结束渲染时销毁 */
    jlgpu_delete_task_list_head(head);
}

/* 创建GPU任务链的背景任务，本质为第一个填充任务 */
static void gpu_demo_create_bg_task(pJLGPUChainHead_t head, struct rect draw_rect, u32 color)
{
    /* 填充颜色拆分为ARGB8888分量 */
    u8 a = ARGB8888_A(color);
    u8 r = ARGB8888_R(color);
    u8 g = ARGB8888_G(color);
    u8 b = ARGB8888_B(color);
    /* printf("background a: 0x%x, r: 0x%x, g: 0x%x, b: 0x%x\n", a, r, g, b); */

    /* GPU 填充任务配置参数初始化 */
    jlgpu_task_fill_param_init(0, 0, a, r, g, b);

    /* 设置GPU填充区域，默认为整个屏幕区域 */
    jlgpu_task_fill_set_draw(rect_left(&draw_rect), rect_top(&draw_rect), rect_width(&draw_rect), rect_height(&draw_rect));

    /*
       设置GPU填充任务绘制限制区域，
       GPU实际绘制为task_param.draw 与 task_param.area 重叠区域，这里也设置为全屏区域
     */
    /* jlgpu_get_win_rect(&task_param.area); */
    jlgpu_task_fill_set_area(rect_left(&draw_rect), rect_top(&draw_rect), rect_width(&draw_rect), rect_height(&draw_rect));

    /* 创建GPU任务，并添加到gpu_task_head任务链中 */
    jlgpu_update_task_by_id(head, 0, 0, &task_param);
}

/* 追加GPU demo任务 */
static pJLGPUChainHead_t gpu_demo_run_unit(pJLGPUChainHead_t head, struct rect draw_rect)
{
    pJLGPUChainHead_t draw_head = head;

    /* 运行当前demo */
    struct gpu_demo_list *demo_ops = &demo_list[__this->demo_index];
    if (demo_ops->draw) {
        draw_head = demo_ops->draw(head, &draw_rect);
    }
    printf("Demo name: %s, Test time: %d, Time count: %d\n", \
           demo_ops->name, demo_ops->time, demo_ops->count);

    /* 如果DEMO返回NULL，把draw_head指向刷新启动时的head */
    if (!draw_head) {
        draw_head = head;
    }

    /* 判断当前demo是否超过预设时间，如果超过则切换到下一个模式 */
    if (demo_ops->count >= demo_ops->time) {
        demo_ops->count = 0;
        /* 如果有demo释放回调，先执行当前demo的释放回调 */
        if (demo_ops->free) {
            demo_ops->free();
        }

        /* 切换demo索引 */
        __this->demo_index += 1;
        if (__this->demo_index >= __this->demo_number) {
            __this->demo_index = 0;
        }
        printf("Switch demo index: %d\n", __this->demo_index);

        /* 如果新的demo有初始化回调，执行初始化 */
        demo_ops = &demo_list[__this->demo_index];
        if (demo_ops->init) {
            demo_ops->init();
        }
    } else {
        /* demo执行时间累加 */
        demo_ops->count += GPU_DEMO_TIME;
    }

    return draw_head;
}

/* GPU demo列表初始化 */
static void gpu_demo_list_init()
{
    struct gpu_demo_list *demo_ops = &demo_list[0];

    /* 统计demo数量，并打印所有demo信息 */
    __this->demo_number = 0;
    while (demo_ops->draw) {
        demo_ops->count = 0;
        __this->demo_number ++;
        printf("DEMO[%d] name: %s, time: %d, cb: 0x%x\n", __this->demo_number - 1, \
               demo_ops->name, demo_ops->time, (u32)demo_ops->draw);
        demo_ops = &demo_list[__this->demo_number];
    }
    printf("Total demo number: %d\n", __this->demo_number);

    /* 默认运行第0个demo，如果有初始化要求，先初始化 */
    __this->demo_index = 0;
    demo_ops = &demo_list[__this->demo_index];
    if (demo_ops->init) {
        demo_ops->init();
    }
}


/* @partB =================================<< 屏幕操作示例 >>================================= */
static void lcd_wait_te()
{
    /*
       TE同步，等待TE需使用TE中断回调，使用信号量进行同步。为避免函数定义冲突，这里不设置TE等待具体函数
       可参考 gpu_task.c 文件中：lcd_wait_te(); lcd_te_sync_int_handler()和lcd_te_sem信号量相关逻辑。
     */
}

static int lcd_flush_finish_cb(void *p)
{
    set_buffer_idle(__this->buffer_hdl, __this->buffer_adr);
    os_sem_post(&__this->buffer_sem);
    IO_DBI_LOW();
    return 0;
}

static void gpu_demo_lcd_init(void)
{
    __this->lcd = lcd_get_hdl();

    /* 判断操作LCD必须的API是否存在 */
    ASSERT(__this->lcd);
    ASSERT(__this->lcd->init);
    ASSERT(__this->lcd->get_screen_info);
    ASSERT(__this->lcd->buffer_malloc);
    ASSERT(__this->lcd->buffer_free);
    ASSERT(__this->lcd->draw);
    ASSERT(__this->lcd->set_draw_area);
    ASSERT(__this->lcd->backlight_ctrl);

    /* LCD屏幕初始化 */
    if (__this->lcd->init) {
        extern const struct ui_devices_cfg ui_cfg_data;
        __this->lcd->init((void *)&ui_cfg_data);
    }

    /* 获取LCD屏幕信息，如屏幕宽、高等 */
    if (__this->lcd->get_screen_info) {
        __this->lcd->get_screen_info(&__this->info);
    }

    /* 清空屏幕显示，这里是用DBI模块的硬件纯色推屏功能，没有GPU参与，与LCD debug功能一致。 */
    if (__this->lcd->clear_screen) {
        __this->lcd->clear_screen(0x000000, 0, __this->info.width - 1, 0, __this->info.height - 1);
    }

    /* 设置LCD背光亮度等级为 100% */
    if (__this->lcd->backlight_ctrl) {
        __this->lcd->backlight_ctrl(100);
    }

    /* 创建buffer信号量 */
    os_sem_create(&__this->buffer_sem, 1);

    /* 设置DBI模块中断回调 */
    lcd_draw_set_callback(lcd_flush_finish_cb);
}

static void lcd_flush_buffer_start(void *buf_hdl, void *buf_adr, struct rect *rect, int is_continue)
{
    /* 将推屏的rect区域转为start和end区域格式 */
    int x_start = rect_left(rect);
    int x_end   = x_start + rect_width(rect) - 1;
    int y_start = rect_top(rect);
    int y_end   = y_start + rect_height(rect) - 1;

    IO_CPU_HIGH();
    /* 等待上一次DBI模块推完，并起中断。在DBI中断会post这个信号量 */
    if (os_sem_pend(&__this->buffer_sem, 100)) {
        /* 如果等待DBI模块推完数据超时，这里抛出Error打印 */
        printf("Error: wait DBI interrupt time out!\n");
    }
    IO_CPU_LOW();

    /* 把准备要推的buffer设置为使用状态 */
    set_buffer_used(__this->buffer_hdl, buf_adr);

    /* 把准备要推的buffer放到全局变量记录，等DBI模块推完后再中断中要用 */
    __this->buffer_adr = buf_adr;

    IO_DBI_HIGH();
    if (!is_continue) {
        /* 仅第一个分块需考虑TE同步 */
        lcd_wait_te();

        /* 第一个分块用kistart接口进行推屏 */
        lcd_draw_kistart(buf_adr, x_start, x_end, y_start, y_end);	// DBI kistart
    } else {
        /* 其它分块用continue接口进行续传即可 */
        lcd_draw_continue(buf_adr, x_start, x_end, y_start, y_end);	// DBI 续传
    }
}

/* @partC =================================<< 链表渲染示例 >>================================= */
void gpu_demo_start_draw(struct rect draw, pJLGPUTaskHead_t head)
{
    /* 将要绘制的宽、高、分块高度用其它变量缓存，非必须，只是为方便使用。 */
    int width  = rect_width(&draw);
    int height = rect_height(&draw);
    int block_height = __this->block_height;

    /* 计算分块数量，如果分块高度不能被全屏高度整除，最后一块不足一个分块高度，也要算做一个分块。 */
    int block_number = (height % block_height) ? ((height / block_height) + 1) : (height / block_height);
    /* printf("@@@ block_number: %d\n", block_number); */

    /* 设置LCD推屏区域。注意，这里是start和end的参数，而且刷新屏幕前设置一次要刷新的整个区域即可。 */
    lcd_set_draw_area(0, (width - 1), 0, (height - 1));

    rect_left(&draw) = 0;
    rect_top(&draw)  = 0;

    int draw_height;
    struct rect draw_rect = {0};
    IO_FRAME_HIGH();  /* 帧渲染开始 */
    for (int i = 0; i < block_number; i++) {

        /* 计算当前需要合成的块高度。
           考虑最后一个块可能不足一个分块的高度，因此每次合成前需计算本次要合成的块高度。 */
        if ((i + 1) * block_height < height) {
            draw_height = block_height;
        } else {
            /* 最后一个分块时，一般不满足预设高度，需计算实际高度才能推屏 */
            draw_height = height - (i * block_height);
        }

        /* 本次循环需要GPU合成的区域 */
        rect_left(&draw_rect)	= rect_left(&draw) + 0;
        rect_top(&draw_rect)	= rect_top(&draw) + i * block_height;
        rect_width(&draw_rect)  = width;
        rect_height(&draw_rect) = draw_height;
        /* JL_DUMP_RECT(&draw_rect); */

        /* 获取空闲的显存buf */
        u8 *idle_buf = NULL;
        u32 time_msec1 = jiffies_msec();
        do {
            /* 尝试获取空闲的显存buf */
            idle_buf = get_idle_buffer_to_lock(__this->buffer_hdl);
            u32 time_msec2 = jiffies_msec();
            if (time_msec2 - time_msec1 > 500) {
                /* 如果超过500ms没获取到空闲buf，则打印buffer管理句柄管理的所有buf状态 */
                buffer_manager_status_dump(__this->buffer_hdl);
            }
        } while (!idle_buf);
        int out_bpp = gpu_get_format_bpp(jlgpu_chain_head_out_format(head)) / 8;
        memset(idle_buf, 0x00, rect_width(&draw_rect) * rect_height(&draw_rect) * out_bpp);

        /* 设置GPU任务链输出buf和绘制区域 */
        jlgpu_set_task_list_out_buf(head, idle_buf, &draw_rect, 0, 0);

        /* 启动GPU合成 */
        jlgpu_task_list_run(head);

        /* 设置GPU输出的buf为等待状态，等待DBI模块推屏，到这里buf已经是GPU合成完毕的输出了。 */
        set_buffer_pend(__this->buffer_hdl, idle_buf);

        /* 启动LCD推屏 */
        rect_left(&draw_rect) -= rect_left(&draw);
        rect_top(&draw_rect)  -= rect_top(&draw);
        lcd_flush_buffer_start(__this->buffer_hdl, idle_buf, &draw_rect, i);
    }
    IO_FRAME_LOW();  /* 帧渲染结束 */
}

/* @partD =================================<< 渲染线程示例 >>================================= */
static void gpu_demo_env_init()
{
    u8 *buf;
    u32 len;

    /* 通过LCD的显存申请接口，申请显存buf，用于GPU硬件的输出缓存buf。
       这个接口会通过LCD屏驱配置的显存buf分块和数量进行申请。 */
    __this->lcd->buffer_malloc(&buf, &len);
    /* printf("@@@ disp buf init, buf: 0x%p, len: %d\n", buf, len); */

    /* 初始化显存buf管理模块。
       把刚刚申请的显存buf，和LCD屏驱配置的buf数量配置给buffer管理模块，创建buffer管理句柄。 */
    buffer_manager_init(__this->buffer_hdl, buf, len, __this->info.buf_num);

    /* 获取显存buf的分块行数。
       这里通过LCD驱动配置的显存buf长度反算，因为显存分块高度并没有被记录，但会反应在显存buf长度上。 */
    __this->block_height = len / __this->info.buf_num / __this->info.stride;

    /* GPU硬件模块初始化，需要设置默认窗口大小，这里使用屏幕信息的宽高作为窗口宽高 */
    jlgpu_module_init(__this->info.width, __this->info.height);

    /* 设置GPU窗口位置偏移 */
    struct rect win;
    rect_top(&win)    = GPU_WIN_OFFSET;
    rect_left(&win)   = GPU_WIN_OFFSET;
    rect_width(&win)  = __this->info.width;
    rect_height(&win) = __this->info.height;
    jlgpu_set_win_rect(&win);
}

static void gpu_demo_refr_timer(void *p)
{
    int err, msg[32] = {0};

    /* 固定间隔触发一次刷新 */
    msg[0] = GPU_DEMO_MSG_REFR;
    err = os_taskq_post_type(GPU_DEMO_TASK, msg[0], 2, msg);
    if (err != OS_NO_ERR) {
        printf("@@@ %s(), taskq post failed! err: %d\n", __func__, err);
    }
}

static void gpu_demo_task(void *p)
{
    FUNC_ENTER();

    extern void img_data_src_init();
    img_data_src_init();  /* 图片数据初始化 */

    gpu_demo_lcd_init();  /* LCD 屏幕初始化 */
    gpu_demo_env_init();  /* DEMO环境初始化 */
    gpu_demo_list_init(); /* DEMO列表初始化 */

    /* ui_effect_manager_init(); [>! 特效管理模块初始化 <] */

    /* 如果有定时器，可能第二次调用了本函数，删除原先定时器 */
    if (__this->refr_timer_id) {
        sys_timer_del(__this->refr_timer_id);
        __this->refr_timer_id = 0;
    }

    /*
       创建刷新触发定时器
       这里创建定时器，用于定时触发刷新DEMO，模拟UI刷新效果
     */
    __this->refr_timer_id = sys_timer_add(NULL, gpu_demo_refr_timer, GPU_DEMO_TIME);
    extern void gpu_buf_init();
    gpu_buf_init();

    /* GPU DEMO 线程主循环 */
    int ret, msg[32] = {0};
    while (1) {
        /* 挂起线程等待 */
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        /* printf("ret: %d, msg[0] = %d\n", ret, msg[0]); */
        if (ret != OS_TASKQ) {
            continue;
        }

        switch (msg[0]) {
        case GPU_DEMO_MSG_REFR:
            /* 触发刷新，刷新区域为全屏 */
            struct rect screen;
            jlgpu_get_win_rect(&screen);
            /* JL_DUMP_RECT(&screen); */

            /*
               渲染开始，初始化GPU任务链
               这里创建任务链，然后传给DEMO回调，添加完DEMO任务后再渲染
             */
            pJLGPUChainHead_t head = gpu_demo_head_init();

            /*
               创建默认背景任务
               非GPU渲染区域的显存为随机数据，因此一般需创建全屏的纯色任务，用于清空控件外的显存区域。
             */
            gpu_demo_create_bg_task(head, screen, GPU_DEMO_BG);

            /*
               追加DEMO任务到GPU任务链
               动画或特效DEMO时，会在DEMO里面创建新的任务链，返回到draw_head；
               1. 如果DEMO中未返回新的任务链，draw_head即head，直接渲染；
               2. 如果DEMO中返回了新的任务链，draw_head为新任务链，需要在demo中销毁head，避免内存泄露；
             */
            pJLGPUChainHead_t draw_head = gpu_demo_run_unit(head, screen); /*! 运行 DEMO    */

            /* jlgpu_dump_all_task(draw_head, __func__, __LINE__); [>! 打印任务链，用于debug <] */

            /* 启动渲染 */
            gpu_demo_start_draw(screen, draw_head);

            /* 销毁任务链 */
            gpu_demo_head_free(draw_head);
            extern void gpu_buf_list_free();
            gpu_buf_list_free();
            mem_stats();
            break;
        case GPU_DEMO_MSG_OTHER:
            break;
        default:
            break;
        }
    }
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_run GPU DEMO线程启动
 */
/* ------------------------------------------------------------------------------------*/
void gpu_demo_run(void)
{
    printf(">>>>>>>>>> GPU DEMO Run!\n");
    FUNC_ENTER(); /*! 函数进入打印，用于debug */

    clock_lock("ui_show", clk_get_max_frequency()); /*! 固定时钟频率 */

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)
    extern void ui_task_init(void);
    extern void gpu_task_init(void);
    extern void lcd_task_init(void);
    lcd_task_init();
    gpu_task_init();
    ui_task_init();
#else
    /* 创建GPU DEMO线程 */
    int err = os_task_create(gpu_demo_task, NULL, 10, 1024 * 5, 512, GPU_DEMO_TASK);
    if (err != OS_NO_ERR) {
        printf("GPU task create failed! err: %d\n", err);
    }
#endif

    FUNC_LEAVE(); /*! 函数离开打印，用于debug */
}

#endif

