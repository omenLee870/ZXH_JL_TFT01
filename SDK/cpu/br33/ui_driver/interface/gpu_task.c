/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_task.c
 *
 * @brief GPU 任务调度模块
 *
 * @author zhuhaifang@zh-jieli.com
 *
 * @version V1.0.0
 *
 * @date 2024-08-01
 */
/* ------------------------------------------------------------------------------------*/
#include "app_config.h"
#include "product_test.h"
#include "jlui/ui_core.h"

#include "gpu_port.h"
#include "gpu_task.h"


#include "includes.h"

#include "dbi.h"
#include "ui_measure.h"
#include "buffer_manager.h"
#include "ui_addr.h"

#include "ui_page_switch.h"
#include "jlui_effect.h"		//特效相关
#include "ui_core.h"
#include "jljpeg_decode.h"
#include "jlui_app/ui_style.h"
#include "jlui_app/ui_app_effect.h"
#include "ui_multi_page_manager.h"
#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_GPU_PUBLIC_MODLS_ENABLE))

#include "time_count.h"

#define     APP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

/* debug 打印配置 */
#define LOG_TAG_CONST		JL_GPU
#define LOG_TAG             "[JL GPU]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
#define DUMP_RECT(func, line, name, rect) \
	printf("[RECT] %s() %d, %s [%d, %d, %d, %d]\n", func, line, name, (rect)->left, (rect)->top, (rect)->width, (rect)->height)


/* DMA模块和PSRAM相关 */
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
#include "asm/dma_copy.h"
#include "lcd_task.h"
#endif

#define CONFIG_LCD_BUF_DYNAMIC_MIN_LINE		(4)				//若剩余空间换算的行数低于此值，则丢帧或断言
#define CONFIG_LCD_BUF_LEFT_SPACE			(40*1024)		//动态申请buf后，留下的剩余空间

extern const int JLUI_MULTI_PAGE_OVERLAY_SUPPORT;

struct mult_chain_def {
    JLGPUMultTaskList_t common;
    JLGPUMultTaskList_t shadow;
};
static struct mult_chain_def mult_chain = {0};
#define __MultChain		(&mult_chain)

void jlgpu_mult_task_list_common_init()
{
    INIT_LIST_HEAD(&(__MultChain->common.list));
    __MultChain->common.head = NULL;
    __MultChain->common.identity = -1;
}

void jlgpu_mult_task_list_shadow_init()
{
    INIT_LIST_HEAD(&(__MultChain->shadow.list));
    __MultChain->shadow.head = NULL;
    __MultChain->shadow.identity = -1;
}

pJLGPUMultTaskList_t jlgpu_mult_task_list_get_common_root()
{
    return &(__MultChain->common);
}
pJLGPUMultTaskList_t jlgpu_mult_task_list_switch_root(pJLGPUMultTaskList_t mult)
{
    pJLGPUMultTaskList_t common = &(__MultChain->common);
    pJLGPUMultTaskList_t shadow = &(__MultChain->shadow);
    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
        ASSERT(mult == common);
        return common;
    } else {
        return (mult == common) ? shadow : common;
    }
}
int jlgpu_mult_task_list_check(pJLGPUMultTaskList_t mult)
{
    pJLGPUMultTaskList_t common = &(__MultChain->common);
    pJLGPUMultTaskList_t shadow = &(__MultChain->shadow);
    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
        ASSERT(mult == common);
    }
    if ((mult == common) || (mult == shadow)) {
        return 1;
    } else {
        return 0;
    }
}
pJLGPUMultTaskList_t jlgpu_mult_task_list_sync_shadow(pJLGPUMultTaskList_t mult)
{
    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
        return NULL;
    }
    pJLGPUMultTaskList_t shadow = jlgpu_mult_task_list_switch_root(mult);

    jlgpu_mult_chain_clear_head(shadow);

    // 拷贝所有任务链
    jlgpu_mult_chain_shadow_head(mult, shadow);

    return shadow;
}



/* 私有参数 */
struct gpu_task_private_def {
    struct draw_context dc;	// DC数据备份，在GPU合成时使用，防止GPU合成时，DC内容被UI框架修改

    void *buffer_hdl;	// buf管理句柄，用于设置buf状态、获取空闲buf等
    void *buffer_adr;	// buf指针，用于记录当前使用的buf地址
#if TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE
    void *disp_adr;	//gpu task申请的显存buf
    void *disp_hdl;	//gpu task申请的显存管理
#endif
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
    void *psram_addr;
    void *psram_buf_hdl;
    void *psram_buf_adr;
    void *psram_lcd_buf;
    void *psram_last_buf;
#endif
    OS_SEM buf_idle_sem; // buf 设置为空闲时信号量，防止GPU获取空闲buf时进入循环死等

    /* GPU空闲信号量，等待GPU同步时会pend住，DBI模块推完最后一个分块时会post，允许GPU合成下一帧 */
    OS_SEM gpu_sem;		// GPU空闲信号量

    /* TE同步信号量，等待TE时会pend住，TE中断过来时会post，释放TE等待 */
    OS_SEM lcd_te_sem;	// TE同步信号量

    /* 推屏buf使用信号量，DBI启动推屏前需pend住等上一个buf推完，DBI推完buf后会post允许下一次推屏 */
    OS_SEM buffer_sem;	// BUF使用信号量

    pJLGPUMultTaskList_t mult_list;
    pJLGPUMultTaskList_t last_mult_list;

    u8 has_init;		// 模块初始化标志，防止信号量未初始化就使用，导致系统异常
    u8 gpu_line_end_flag : 1;	// 最后一块推屏标志，仅当合成最后一个分块时为1，其它时候为0,仅仅表示gpu,不代表lcd已经推完
    u8 lcd_draw_line_end_flag: 1;	// 最后一块推屏标志，仅当合成最后一个分块时为1，其它时候为0,表示lcd
    u8 redraw_mode;		// 同步或异步刷新标志
    u8 wait_gpu_sem;	// 是否需等GPU信号量标志
    u8 gpu_is_busy;		// GPU繁忙标志，GPU正在合成任务链时为真
    u8 draw_task_list;	// 绘制GPU任务链标志

    u8 release_frame_buf; // 释放PSRAM帧buf

    int lcd_width;
    int lcd_height;

    OS_SEM te_dly_sem;	// te延迟
    u32 soft_max_us; 	// 刷一帧的最大时间
};
static struct gpu_task_private_def gpu_task_priv ALIGNED(4) = {0};
#define __this	(&gpu_task_priv)


extern volatile int64_t dc_te_absolute_us;
extern volatile int64_t dc_start_absolute_us;
extern volatile int lcd_spi_last_us;

extern u32 lcd_get_te_frame_period_us(void);
extern u32 lcd_get_spi_frame_period_us(void);
extern u32 lcd_get_te_phase_us();
extern void gpu_free_frame_buf(void *p);
extern void jpeg_module_free_res_cb(void *p);
extern struct element *ui_core_get_root();
extern u8	buf_is_heap_addr(void *p);
extern const int JLUI_GPU_DMA_TO_PSRAM;
extern void *jlgpu_unit_to_instruction(pJLGPUTaskUnit_t taskp);
extern const int config_gpu_cache_psram_jpeg_en ;
extern const int config_gpu_cache_psram_file_res_en;
extern int jpeg_module_opj_invaild_clr();
extern int jpeg_module_opj_set_invaild_by_index(int index);
extern size_t xPortGetPhysiceMemorySize(void);
void *jlui_malloc(int size, u32 ram_type, u32 module_type);
void jlui_free(void *buf, u32 ram_type, u32 module_type);
void jlui_argb8565_to_rgb565(u8 *out_buf, struct rect *out_r, u8 *in_buf, struct rect *in_r);
extern uint32_t gpu_task_get_format_bpp(uint32_t format);
extern void psram_flush_cache(void *begin, u32 len);
extern int async_buffer_clr();

extern const int JLUI_LCD_RAMLESS_ENABLE;
extern const int JLUI_MULTI_PAGE_OVERLAY_SUPPORT;
extern const int GPU_INPUT_TO_NOCACHE;

u32 gpu_task_get_max_fps()
{
    return 1000000 / __this->soft_max_us;
}

void gpu_task_set_max_fps(u32 fps)
{
    __this->soft_max_us = 1000000 / fps;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_lcd_buf_line_get 计算可用行数
 *
 * @param left_space 剩余ram空间
 * @param stride	 一行大小
 * @param buf_num	buf数量（一般为双buf）
 * @param max_line	最大行数,-1时不设上限
 *
 * @return 行数
 */
/* ------------------------------------------------------------------------------------*/
static int jlgpu_lcd_buf_line_get(int left_space, int stride, int buf_num, int max_line, int total_line)
{
    if (left_space < CONFIG_LCD_BUF_LEFT_SPACE) { //预留一部分空间
        return 0;
    }
    left_space -= CONFIG_LCD_BUF_LEFT_SPACE;
    int check_line = left_space / stride / buf_num;
    /* printf("%s left_space:%d check_line:%d total:%d max:%d",__func__,left_space,check_line,total_line,max_line); */
    if (max_line != -1) { //上限
        check_line = (check_line > max_line) ? max_line : check_line;
    }
    if (check_line < CONFIG_LCD_BUF_DYNAMIC_MIN_LINE) {	//下限
        return 0;
    }
    check_line = (check_line > total_line) ? total_line : check_line;
    /* printf("%s left_space:%d check_line:%d total:%d max:%d",__func__,left_space,check_line,total_line,max_line); */
    return check_line;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief  jlgpu_scheduler_aysnc_free_buf_cb 异步释放buf回调
 *
 * @param p
 */
/* ------------------------------------------------------------------------------------*/
static void jlgpu_scheduler_aysnc_free_buf_cb(void *p)
{
    if (buf_is_heap_addr(p)) {
        gpu_free_frame_buf(p);
    } else {
        ASSERT(0, "p:%x", (int)p);
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_async_free_buf post buf到gpu task释放
 *
 * @param p
 *
 * @return
 *
 * @notes: buf可能被gpu异步使用时，post到gpu中释放，避免出现硬件访问异常
 */
/* ------------------------------------------------------------------------------------*/
int jlgpu_scheduler_async_free_buf(void *p)
{
    int ret = 0;
    if (strcmp(os_current_task(), GPU_TASK_NAME)) {
        int argv[3] = {0};
        int retry = 3;
        argv[0] = (int) jlgpu_scheduler_aysnc_free_buf_cb;
        argv[1] = 1;
        argv[2] = (int)p;

__try_again:
        ret = os_taskq_post_type(GPU_TASK_NAME, Q_CALLBACK, 3, argv);
        if (ret) {
            if (retry) {
                os_time_dly(1);
                retry--;
                goto __try_again;
            }
            printf("%s post ret:%d retry:%d\n", __func__, ret, retry);
        }
    } else {
        jlgpu_scheduler_aysnc_free_buf_cb(p);
    }
    return ret;
}

int jlgpu_scheduler_async_gpu_cache_free()
{
    int ret = 0;
    int argv[3] = {0};
    int retry = 3;
    argv[0] = (int)gpu_cache_clr_all_invalid;
    argv[1] = 0;

__try_again:
    ret = os_taskq_post_type(GPU_TASK_NAME, Q_CALLBACK, 2, argv);
    if (ret) {
        if (retry) {
            os_time_dly(1);
            retry--;
            goto __try_again;
        }
        printf("%s post ret:%d retry:%d\n", __func__, ret, retry);
    }
    return ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_async_free_buf post buf到gpu task释放
 *
 * @param p
 *
 * @return
 *
 * @notes: buf可能被gpu异步使用时，post到gpu中释放，避免出现硬件访问异常
 */
/* ------------------------------------------------------------------------------------*/
int jlgpu_scheduler_async_free_jpeg_res(void *p)
{

    int argv[3] = {0};
    int retry = 3;
    argv[0] = (int)  jpeg_module_free_res_cb;
    argv[1] = 1;
    argv[2] = (int)p;

__try_again:
    int ret = os_taskq_post_type(GPU_TASK_NAME, Q_CALLBACK, 3, argv);
    if (ret) {
        if (retry) {
            os_time_dly(1);
            retry--;
            goto __try_again;
        }
        printf("%s post ret:%d retry:%d\n", __func__, ret, retry);
    }
    return ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_flush_finish_cb DBI模块中断回调，每推完一帧数据起一次
 *
 * @Params err 错误标志
 */
/* ------------------------------------------------------------------------------------*/
static int lcd_flush_finish_cb(void *priv)
{
    if (JLUI_GPU_DMA_TO_PSRAM) {
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
        if (JLUI_LCD_RAMLESS_ENABLE) {
            if (priv) {
                struct dbi_ramless_callback_param *param = (struct dbi_ramless_callback_param *)priv;

                if (param->lcd_buff_update) {
                    if (param->lcd_buff_new == (u32)__this->psram_lcd_buf) {
                        set_buffer_used(__this->psram_buf_hdl, __this->psram_lcd_buf);
                    }
                    if (param->lcd_buff_old == (u32)__this->psram_last_buf) {
                        set_buffer_idle(__this->psram_buf_hdl, __this->psram_last_buf);
                        os_sem_post(&__this->buf_idle_sem);
                    }
                }
            }
        } else {
            set_buffer_idle(__this->psram_buf_hdl, __this->psram_lcd_buf);
            os_sem_post(&__this->buf_idle_sem);
        }
        /* LCD推屏结束，buf才空闲，可以进行buf释放 */
        if (__this->release_frame_buf && !__this->gpu_is_busy) {
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
            os_taskq_del_type(LCD_TASK_NAME, LCD_MSG_FLUSH); // 清空lcd_task消息
#endif
            if (__this->psram_buf_hdl) {
                buffer_manager_free(__this->psram_buf_hdl);
                __this->psram_buf_hdl = NULL;
                __this->psram_buf_adr = NULL;
                __this->psram_lcd_buf = NULL;
            }
            if (__this->psram_addr) {
                free_psram(__this->psram_addr);
                __this->psram_addr = NULL;
            }
        }
#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
        lcd_spi_last_us = jiffies_usec2offset(dc_start_absolute_us, jiffies_usec());
#endif
#endif
    } else {
        if (__this->buffer_hdl) {
            /* 推屏完成，设置推完屏的buf为空闲状态，允许GPU获取这块buf进行合成 */
            set_buffer_idle(__this->buffer_hdl, __this->buffer_adr);
            if (!__this->lcd_draw_line_end_flag) {
                __this->buffer_hdl = NULL;
                __this->buffer_adr = NULL;
            }
            os_sem_post(&__this->buf_idle_sem);

            /* 当line_end_flag为真时，说明当前推完的是最后一个分块，此时已经完成一帧的合成和推屏 */
            if (__this->lcd_draw_line_end_flag) {
                __this->lcd_draw_line_end_flag = 0;
#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
                lcd_spi_last_us = jiffies_usec2offset(dc_start_absolute_us, jiffies_usec());
#endif
                if (__this->draw_task_list) {
                    buffer_manager_free(__this->buffer_hdl);
                    __this->draw_task_list = 0;
                }
#if TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE
                //free_buf
                if (__this->disp_adr) {
                    jlui_free(__this->disp_adr, UI_RAM_SRAM, UI_MODULE_BUFFER);
                    __this->disp_adr = NULL;
                }
                //buffer_manager_free
                if (__this->disp_hdl) {
                    buffer_manager_free(__this->disp_hdl);
                    __this->disp_hdl = NULL;
                }
#endif// TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE
                __this->buffer_hdl = NULL;
                __this->buffer_adr = NULL;

                __this->wait_gpu_sem = false;
                os_sem_post(&__this->gpu_sem);	// 最后一个分块推完，发送GPU信号量
                /* __this->wait_gpu_sem = true; *///br33
            }
            os_sem_post(&__this->buffer_sem);	// 每推完一个分块，发送一个buf信号量
        }
    }
    IO_DBI_LOW();

    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_te_sync_int_handler TE中断同步回调，在TE中断中调用，用于TE等待同步
 */
/* ------------------------------------------------------------------------------------*/
void lcd_te_sync_int_handler(void)
{
    /* APP_IO_DEBUG_1(B,5); */
    if (__this->has_init) {
        dc_te_absolute_us = jiffies_usec();
        os_sem_post(&__this->lcd_te_sem);
    }
    /* APP_IO_DEBUG_0(B,5); */
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_wait_te TE等待同步函数
 */
/* ------------------------------------------------------------------------------------*/
static void lcd_wait_te()
{
#if TCFG_LCD_PIN_TE == NO_CONFIG_PORT
    return;
#else
    os_sem_set(&__this->lcd_te_sem, 0);
    while (1) {
        /* pend住等待TE信号量，释放CPU */
        int err = os_sem_pend(&__this->lcd_te_sem, 10);
        if (err != OS_NO_ERR) {
            if (err == OS_TIMEOUT) {
                printf("lcd wait te timeout.\n");
                break;
            } else {
                printf("lcd wait te error.\n");
            }
        } else {
            break;
        }
    }
#endif
}

static void lcd_te_dly_tm_deal(void *priv)
{
    os_sem_post(&__this->te_dly_sem);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_wait_te_sync TE自适应同步
 */
/* ------------------------------------------------------------------------------------*/
void lcd_wait_te_sync()
{
#if (PRODUCT_TEST_ENABLE && (!PT_LCD_TP_ENABLE))
    if (product_test_check_run()) {
        return ;
    }
#endif /* #if (PRODUCT_TEST_ENABLE && (!PT_LCD_TP_ENABLE)) */
#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)

#if (defined TCFG_LCD_TE_QUICK_SYNC && TCFG_LCD_TE_QUICK_SYNC)
    /* 自适应TE同步逻辑，仅在推第一个分块时需要判断是否进行TE同步 */

    u32 period_us = lcd_get_te_frame_period_us();//tft屏幕刷新一帧时间
    u32 spi_period_us = lcd_get_spi_frame_period_us();//spi推屏一帧时间

    u32 rela_us = jiffies_usec2offset(dc_te_absolute_us, jiffies_usec());//当前时间距离te中断的时间

    u32 refresh_us = jiffies_usec2offset(dc_start_absolute_us, jiffies_usec());

    //转换为距离te点相对时间，解决te中断压力情况中断延后导致的计数器溢出问题
    if (rela_us >= period_us) {
        rela_us -= period_us;
    }

    log_debug("period_us:%d, spi_period_us:%d, rela_us:%d, refresh_us:%d \n",
              period_us, spi_period_us, rela_us, refresh_us);

    u32 max_period_us = __this->soft_max_us;
    if (max_period_us < spi_period_us) {
        max_period_us = spi_period_us;
    }
    int te_dly, te_wth, te_remain;

    te_dly = period_us - spi_period_us;
    if (te_dly < 0) {
        te_dly = 0;
    }
    te_remain = max_period_us - period_us;
    if (te_remain < 0) {
        te_remain = 0;
    }
    te_wth = period_us - (te_dly + te_remain);
    if (te_wth < 0) {
        te_wth = 0;
    }
    log_debug("te_dly:%dus, te_wth:%dus, te_remain:%dus \n", te_dly, te_wth, te_remain);

    int   lcd_need_wait_te = 0;

#if 0
    //判断时间差是否大于2倍te,要么是中途停止刷屏要么是推屏超时2te，无论哪个情况都要等te
    if (refresh_us >= 2 * period_us) {
        lcd_need_wait_te = 1;
        goto __end;//结束计算
    }

    //判断时间差是否<1 te,1e 能推完的下次繼續等
    if (lcd_spi_last_us <= period_us) {
        lcd_need_wait_te = 1;
        goto __end;//结束计算
    }

    //rela_us 在这里是标识超过当前te线的时间，不代表距离上次一次te的时间，考虑溢出问题

    //把te 触发点 转换,te 距离刷屏0点的距离
    int te_offset = (period_us - lcd_get_te_phase_us() % period_us);//或者理解为触发中断到屏幕刷新0行的时间
    int remain_render_flush_time_min;//
    int remain_render_flush_time_max;//

    if (rela_us > te_offset) {//证明当前刷新点已经越过了0行了
        //当前rela_us - te_offset 代表越过0行的刷新时间
        remain_render_flush_time_min = period_us - (rela_us - te_offset); //1 te 周期内完成的时间剩余
        remain_render_flush_time_max = 2 * period_us - (rela_us - te_offset); //2 te 周期内完成的时间剩余
    } else {
        //lcd_get_te_phase_us() + rela_us代表当前点在越过0行的刷新行时间
        remain_render_flush_time_min = period_us - (lcd_get_te_phase_us() + rela_us);//1 te 周期内完成的时间剩余
        remain_render_flush_time_max = 2 * period_us - (lcd_get_te_phase_us() + rela_us);//2 te 周期内完成的时间剩余
    }
    if (lcd_spi_last_us < spi_period_us) {
        lcd_spi_last_us = spi_period_us; //正常是要小于spi 周期的，除非局部刷新
    }

    //1.15 只是一个系数
    if (lcd_spi_last_us * 1.15f >  remain_render_flush_time_max) { //上次刷新时间加上系数比剩余最大时间还大，需要等te
        lcd_need_wait_te = 1;
    } //need wait te

    if (spi_period_us   < remain_render_flush_time_min) {
        //spi 速度快过了,本来要求最快17ms推完，但是spi是16ms推完，瞬时速度过度，需要分块的策略或者等te了
        /* printf("%d %d,%d %d,%d %d %d %d\n", remain_render_flush_time_min, remain_render_flush_time_max, spi_period_us , rela_us, lcd_spi_last_us, period_us,lcd_get_spi_frame_period_us(),lcd_get_te_phase_us()); */
        lcd_need_wait_te = 1;
    } //need wait te

#else
    if (rela_us >= period_us) {
        lcd_need_wait_te = 1;
        goto __end;//结束计算
    }

    int offset_origin_us = rela_us + (lcd_get_te_phase_us() % period_us);//当前距离原点的偏移
    if (offset_origin_us >= period_us) {
        offset_origin_us -= period_us;
    }
    if (offset_origin_us > (te_dly + te_wth)) {
        lcd_need_wait_te = 1;
    } else if (offset_origin_us < te_dly) {
        goto __dly;
    }

#endif

__end:
    if (lcd_need_wait_te) {
        lcd_wait_te();
__dly:
        int dly_us = te_dly - (lcd_get_te_phase_us() % period_us);
        if (dly_us > 1000) {
            os_sem_set(&__this->te_dly_sem, 0);
            sys_hi_timeout_add(NULL, lcd_te_dly_tm_deal, dly_us / 1000);
            os_sem_pend(&__this->te_dly_sem, period_us / 10000 + 1);
            dly_us = dly_us % 1000 - 50;
        }
        if (dly_us > 10) {
            udelay(dly_us);
        }
    }
    dc_start_absolute_us = jiffies_usec();
#else
    lcd_wait_te();
#endif

#endif
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_flush_buffer_start 启动DBI模块推屏
 *
 * @Params buf_hdl buffer管理句柄
 * @Params buf_adr buffer地址/待推屏的buf地址
 * @Params rect 推屏的区域，与buf_adr指向的地址大小相同
 * @Params is_continue 是否续传，每次推屏第一个分块为0，其它分块为1，直到本次推屏结束
 */
/* ------------------------------------------------------------------------------------*/
static void lcd_flush_buffer_start(void *buf_hdl, void *buf_adr, struct rect *rect, int is_continue)
{
    int x_start = rect->left;
    int x_end = x_start + rect->width - 1;
    int y_start = rect->top;
    int y_end = y_start + rect->height - 1;


    IO_CPU_HIGH();
    /* 等待上一个buf推完 */
    int os_err = os_sem_pend(&__this->buffer_sem, 100);
    if (os_err) {
        ASSERT(0, "%s os_err:%d\n", __func__, os_err);
    }
    /* put_buf(buf_adr, (x_end - x_start) * 2); */
    IO_CPU_LOW();

    //设置是否lcd最后一块,注意这里的位置要在同步之后，防止中断还没有起来
    if (__this->gpu_line_end_flag) {
        __this->lcd_draw_line_end_flag = 1;
    }

    /* 设置buf为使用状态，避免被其它地方使用 */
    set_buffer_used(buf_hdl, buf_adr);
    __this->buffer_hdl = buf_hdl;
    __this->buffer_adr = buf_adr;
    IO_DBI_HIGH();

    if (!is_continue) {
#if !TCFG_LCD_TE_SYNC_HW_ENABLE
        lcd_wait_te_sync();
#endif
        /* 第一个分块启动DBI模块推屏 */
        lcd_draw_kistart(buf_adr, x_start, x_end, y_start, y_end);	// DBI kistart
    } else {
        /* 其它分块启动DBI模块续传 */
        lcd_draw_continue(buf_adr, x_start, x_end, y_start, y_end);	// DBI 续传
    }
}


#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief dma_copy_callback dma拷贝到PSRAM回调
 *
 * @Params priv dma异步拷贝回调参数
 */
/* ------------------------------------------------------------------------------------*/
static void dma_copy_callback(void *priv)
{
    if (__this->buffer_hdl) {
        set_buffer_idle(__this->buffer_hdl, __this->buffer_adr);
        /* __this->buffer_hdl = NULL; */
        /* __this->buffer_adr = NULL; */
        os_sem_post(&__this->buf_idle_sem);

        if (__this->lcd_draw_line_end_flag) {
            __this->lcd_draw_line_end_flag = 0;
            /* lcd_spi_last_us = jiffies_usec2offset(dc_start_absolute_us, jiffies_usec()); */
            __this->wait_gpu_sem = false;
            os_sem_post(&__this->gpu_sem);	// 最后一个分块推完，发送GPU信号量
            __this->wait_gpu_sem = true;

            if (__this->draw_task_list) {
                buffer_manager_free(__this->buffer_hdl);
                __this->draw_task_list = 0;
            }


            /* 最后一个分块DMA拷贝完成，启动DBI推屏 */
            __this->psram_last_buf = __this->psram_lcd_buf;
            __this->psram_lcd_buf = __this->psram_buf_adr;
            set_buffer_pend(__this->psram_buf_hdl, __this->psram_lcd_buf);

            int err = 0;
            int msg[32];
            msg[0] = LCD_MSG_FLUSH;
            msg[1] = LCD_FLUSH_KISTART;
            msg[2] = 0;
            msg[3] = __this->lcd_width - 1;
            msg[4] = 0;
            msg[5] = __this->lcd_height - 1;
            msg[6] = (u32)__this->psram_buf_adr;
            msg[7] = (u32)__this->psram_buf_hdl;
            if (JLUI_LCD_RAMLESS_ENABLE == 0) {
                set_buffer_used(__this->psram_buf_hdl, __this->psram_lcd_buf);
            }


            /* 启动推屏 */
            err = os_taskq_post_type(LCD_TASK_NAME, msg[0], 8, &msg[1]);
            if (err != OS_NO_ERR) {
                printf("Error, post lcd flush msg failed!\n");
            }
        }
        os_sem_post(&__this->buffer_sem);	// 每推完一个分块，发送一个buf信号量
        __this->buffer_hdl = NULL;
        __this->buffer_adr = NULL;
    }
}

static void psram_flush_buffer_start(void *buf_hdl, void *buf_adr, struct rect *rect, int is_continue)
{
    /* 等待上一个buf推完 */
    int os_err = os_sem_pend(&__this->buffer_sem, 100);
    if (os_err) {
        ASSERT(0, "%s os_err:%d\n", __func__, os_err);
    }
    /* put_buf(buf_adr, (x_end - x_start) * 2); */

    //设置是否lcd最后一块,注意这里的位置要在同步之后，防止中断还没有起来
    if (__this->gpu_line_end_flag) {
        __this->lcd_draw_line_end_flag = 1;
    }

    /* 设置buf为使用状态，避免被其它地方使用 */
    set_buffer_used(buf_hdl, buf_adr);
    __this->buffer_hdl = buf_hdl;
    __this->buffer_adr = buf_adr;

    if (rect->width < __this->lcd_width) {
        // 局部刷新
        int length = rect->width * 2;
        for (int line = 0; line < rect->height; line++) {
            int offset = (line + rect->top) * __this->lcd_width * 2 + rect->left * 2;
            u8 *psram_addr = (u8 *)((u32)__this->psram_buf_adr + offset);
            u8 *input_addr = (u8 *)((u32)buf_adr + line * length);
            /* printf("dst: 0x%x, src: 0x%x\n", (u32)psram_addr, (u32)input_addr); */
            memcpy(psram_addr, input_addr, length);
            /* dma_memcpy_sync(psram_addr, input_addr, length); */
        }
        dma_copy_callback(NULL);
    } else {
        // 全局刷新
        int offset = rect->width * rect->top * 2;
        int length = rect->width * rect->height * 2;
        u8 *psram_addr = (u8 *)UI_PSRAM_CACHE_TO_NOCACHE((u32)__this->psram_buf_adr + offset);
        dma_memcpy_async_with_callback(psram_addr, buf_adr, length, dma_copy_callback, NULL);
    }
}

/* 释放PSRAM缓存，在LCD进入休眠时调用 */
void psram_frame_buf_release()
{
    __this->release_frame_buf = true;
#if 0
    if (__this->psram_buf_hdl) {
        buffer_manager_free(__this->psram_buf_hdl);
        __this->psram_buf_hdl = NULL;
        __this->psram_buf_adr = NULL;
        __this->psram_lcd_buf = NULL;
    }
    if (__this->psram_addr) {
        free_psram(__this->psram_addr);
        __this->psram_addr = NULL;
    }
#endif
}
#endif



/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_wait_sync 等待GPU合成完毕
 *
 * @return 0 正常，-OS_TIMEOUT 等待超时
 */
/* ------------------------------------------------------------------------------------*/
int jlgpu_scheduler_wait_sync()
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    while (__this->gpu_is_busy && __this->wait_gpu_sem) {
        /* printf("%s %d value:%d rets:%x", __func__, __LINE__, __this->gpu_sem.value, rets); */

        if (os_sem_pend(&__this->gpu_sem, 50) == OS_TIMEOUT) {
            __this->wait_gpu_sem = false;
            log_error("%s(), wait GPU task run timeout!\n", __func__);
            ASSERT(0);
            return -OS_TIMEOUT;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_reset_dc_gpu_task_head 重置所有dc的任务链
 */
/* ------------------------------------------------------------------------------------*/
void jlgpu_scheduler_reset_dc_gpu_task_head()
{
    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
        return;
    }

    pJLGPUMultTaskList_t gpu_mult_list_common = jlgpu_mult_task_list_get_common_root();
    pJLGPUMultTaskList_t gpu_mult_list_shadow =  jlgpu_mult_task_list_switch_root(gpu_mult_list_common);
    struct element *root = ui_core_get_root();
    struct element *elm;
    list_for_each_child_element(elm, root) {
        if (elm && elm->dc) {
            u8 index = elm->dc->index;
            pJLGPUTaskHead_t head_common =  jlgpu_mult_task_head_by_index(gpu_mult_list_common, index);
            pJLGPUTaskHead_t head_shadow =  jlgpu_mult_task_head_by_index(gpu_mult_list_shadow, index);
            if (elm->dc->gpu_task_head != head_common && head_shadow) {
                elm->dc->gpu_task_head = head_shadow;
            }
            /* ASSERT(elm->dc->gpu_task_head); */
        }
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_set_redraw_mode 设置GPU任务的刷新模式
 *
 * @Params dc UI框架DC结构体
 * @Params mode 刷新模式
 */
/* ------------------------------------------------------------------------------------*/
void jlgpu_scheduler_set_redraw_mode(struct draw_context *dc, gpu_redraw_mode_t mode)
{
    if (mode != __this->redraw_mode) {
        jlgpu_scheduler_wait_sync();

        if (__this->redraw_mode == GPU_ASYN_REDRAW) {
            /* 原先是异步刷新，现在要切换到同步刷新 */
            dc->gpu_mult_list = __this->last_mult_list;
            dc->gpu_task_head = jlgpu_mult_task_head_by_index(dc->gpu_mult_list, dc->index);

            /* 由异步切换到同步模式时，清空影子组多任务链管理，减少RAM占用 */
            jlgpu_mult_task_list_clr(jlgpu_mult_task_list_switch_root(dc->gpu_mult_list));
            /* 更新dc指向的任务链 */
            jlgpu_scheduler_reset_dc_gpu_task_head();
        } else {
            /* 原先是同步刷新，现在要切换到异步刷新 */
            /* dc->gpu_mult_list = __this->mult_list; */
            dc->gpu_mult_list = __this->last_mult_list;
            dc->gpu_task_head = jlgpu_mult_task_head_by_index(dc->gpu_mult_list, dc->index);

            /* 由同步切换到异步模式时，同步影子组多任务链管理，UI和GPU并行 */
            jlgpu_mult_task_list_sync_shadow(dc->gpu_mult_list);
            /* 更新dc指向的任务链 */
            jlgpu_scheduler_reset_dc_gpu_task_head();
        }

        /* 设置当前刷新方式 */
        __this->redraw_mode = mode;
    }
}

int jlgpu_scheduler_get_redraw_mode()
{
    return __this->redraw_mode;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_create_mult_task_list 创建多任务链管理句柄
 *
 * @Params dc UI框架DC指针
 *
 * @return 多任务链管理句柄
 */
/* ------------------------------------------------------------------------------------*/
void *jlgpu_scheduler_create_mult_task_list(struct draw_context *dc)
{
    /* 创建多任务链管理句柄 */
    if (!__this->mult_list) {
        __this->mult_list = jlgpu_mult_task_list_get_common_root();
    }

    /* 如果是同步刷新，则不需要创建多任务链管理句柄 */
    if (__this->redraw_mode == GPU_SYNC_REDRAW) {
        /* ASSERT(0); */
        return __this->mult_list;
    }

    /* 等待GPU空闲，防止改到GPU正在使用的内容 */
    jlgpu_scheduler_wait_sync();

    /* 将GPU任务链添加到多任务链管理句柄 */
    jlgpu_mult_task_list_add(__this->mult_list, dc->gpu_task_head, dc->index);

    /* 同步影子多任务链管理句柄 */
    jlgpu_mult_task_list_sync_shadow(__this->mult_list);

    /* 更新dc指向的任务链 */
    jlgpu_scheduler_reset_dc_gpu_task_head();
    return (void *)__this->mult_list;
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_delete_mult_task_list 删除多任务链管理句柄
 *
 * @Params dc UI框架DC指针
 */
/* ------------------------------------------------------------------------------------*/
void jlgpu_scheduler_delete_mult_task_list(struct draw_context *dc)
{
    /* 如果是同步刷新模式，只需删除当前的GPU任务链即可 */
    if (__this->redraw_mode == GPU_SYNC_REDRAW) {
        jlgpu_delete_task_list_head(dc->gpu_task_head);
        return;
    }

    /* 清空GPU任务的队列消息 */
    os_taskq_del_type(GPU_TASK_NAME, GPU_MSG_DRAW);

    /* 等待GPU合成完成 */
    jlgpu_scheduler_wait_sync();

    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
        jlgpu_mult_task_list_del_by_index(__this->mult_list, dc->index);
    } else {
        /* 找到当前DC使用的GPU任务链 */
        void *gpu_task_head;
        gpu_task_head = jlgpu_mult_task_head_by_index(__this->mult_list, dc->index);

        /* 从多任务链管理句柄删除当前的任务链 */
        jlgpu_mult_task_list_del(__this->mult_list, gpu_task_head);

        /* 删除当前的GPU任务链 */
        jlgpu_delete_task_list_head(gpu_task_head);

    }

    /* 同步影子多任务链管理句柄 */
    jlgpu_mult_task_list_sync_shadow(__this->mult_list);
    /* 更新dc指向的任务链 */
    jlgpu_scheduler_reset_dc_gpu_task_head();
}


#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
void psram_double_buffer_init()
{
    if (!__this->psram_addr) {
        int psram_buf_size = __this->lcd_width * __this->lcd_height * 2 * 2; // 申请PSRAM两个帧缓存buf
        __this->psram_addr = malloc_psram(psram_buf_size);
        if (!__this->psram_buf_hdl) {
            buffer_manager_init(__this->psram_buf_hdl, __this->psram_addr, psram_buf_size, 2); // 创建buf管理句柄
        }
    }
    __this->release_frame_buf = false;
}
#endif

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_kick_start 启动GPU合成并调用DBI刷新
 *
 * @Params mult_list 需合成的多任务链管理句柄
 * @Params dc UI框架的DC指针
 *
 * @return 0 正常合成，其它 合成失败
 */
/* ------------------------------------------------------------------------------------*/
static int jlgpu_scheduler_kick_start(pJLGPUMultTaskList_t mult_list, struct draw_context *dc)
{
    //释放无效的gpu_cache_ram
    jlgpu_scheduler_async_gpu_cache_free();
    /* gpu_cache_clr_all_invalid(); */
    //合成前需要重置jpeg
    /* jpeg_module_opj_invaild_clr(); */
    /* jljpeg_decode_reset_curr(); */
    /* 防止出现野指针进入 */
    if (!jlgpu_mult_task_list_check(mult_list)) {
        log_error("%s(), unknow mult GPU task list: 0x%p, not run!\n", __func__, mult_list);
        return -1;
    }

    /* 记录当前GPU合成的多任务链管理句柄 */
    __this->last_mult_list = mult_list;

    /* 获取当前需要刷新的GPU任务链 */
    void *gpu_task_head = jlgpu_mult_task_head_by_index(mult_list, dc->index);

    /* 如果没找到任务链，则退出 */
    if (!gpu_task_head) {
        return -1;
    }
    __this->gpu_is_busy = true;
    __this->wait_gpu_sem  = true;
    /* printf("@@@@@ draw mult_list: 0x%p, task_head: 0x%p, index: %d\n", mult_list, gpu_task_head, dc->index); */
    /* printf("@@@@@ draw_state: %d\n", dc->draw_state); */
    /* jlgpu_dump_all_task(gpu_task_head, __func__, __LINE__); */

    /* Get draw area. */
    struct rect refresh_rect;
    if (JLUI_LCD_RAMLESS_ENABLE) {
        refresh_rect.left = 0;
        refresh_rect.top = 0;
        refresh_rect.width = __this->lcd_width;
        refresh_rect.height = __this->lcd_height;
    } else {
        struct rect lcd_rect;
        lcd_rect.left = 0;
        lcd_rect.top = 0;
        lcd_rect.width = __this->lcd_width;
        lcd_rect.height = __this->lcd_height;
        if (!get_rect_cover(&dc->rect_orig, &lcd_rect, &refresh_rect)) {
            ASSERT(0);
        }
    }


    /* 如果是左右划屏时，刷新区域强制从0坐标开始 */
    if (dc->draw_state == GPU_SYNTHESIS) {
        refresh_rect.left = 0;
        refresh_rect.top = 0;
    }

    /* 是否对GPU任务链进行重新排序 */
    if (!dc->gpu_task_dont_sort) {
        jlgpu_task_list_reorder(gpu_task_head);
    }
    /* DUMP_RECT(__func__, __LINE__, "refresh_rect", &refresh_rect); */
    int width = refresh_rect.width;		// 刷新宽度
    int height = refresh_rect.height;	// 刷新高度
    int block_height = dc->lines;		// 分块高度

    /* If refresh dirty block, adjust block height. */
#if 0	//局部刷新暂不动态调整高度
    if (width < dc->width) {
        log_debug("%s(), Dirty block refresh, adjust block height!\n", __func__);
        block_height = (dc->width * dc->lines) / width;
    }
    log_info("%s(), block_height: %d\n", __func__, block_height);
#endif
    /* Calculation block number. */
    int block_number = (height % block_height) ? ((height / block_height) + 1) : (height / block_height);
    log_info("%s(), block_number: %d\n", __func__, block_number);

#if TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE
    int buf_num = 2;
    int refresh_stride = (width * gpu_get_format_bpp(GPU_OUT_FORMAT_RGB565) + 7) / 8;
    int check_line = jlgpu_lcd_buf_line_get(xPortGetPhysiceMemorySize(), refresh_stride, buf_num, block_height, height);
    if (!check_line) {
        log_error("not enough disp_buf ,pass this frame!!!\n");
        __this->gpu_is_busy = false;
        __this->wait_gpu_sem  = false;
        return -1;
    }
#endif

    if (JLUI_GPU_DMA_TO_PSRAM) {
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
        psram_double_buffer_init();

        if (width < __this->lcd_width || height < __this->lcd_height) {
            if (__this->psram_buf_adr) {
                wait_buf_to_idle(__this->psram_buf_hdl, __this->psram_buf_adr, &__this->buf_idle_sem, 500); // 局部刷新，已经有buf，等待buf空闲
            } else {
                __this->psram_buf_adr = get_idle_buffer_by_handler(__this->psram_buf_hdl, &__this->buf_idle_sem, 500, 0); // 全屏刷新，直接获取空闲buf实现乒乓
            }
        } else {
            __this->psram_buf_adr = get_idle_buffer_by_handler(__this->psram_buf_hdl, &__this->buf_idle_sem, 500, 0); // 全屏刷新，直接获取空闲buf实现乒乓
        }
#endif
    } else {
        /* Set lcd draw area. */
        lcd_set_draw_area(refresh_rect.left, (refresh_rect.left + width - 1), refresh_rect.top, (refresh_rect.top + height - 1));
    }

#if TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE
    ASSERT(!__this->disp_hdl);
    int buffer_alloc_adr_len = buf_num * check_line * refresh_stride;
    __this->disp_adr = jlui_malloc(buffer_alloc_adr_len, UI_RAM_SRAM, UI_MODULE_BUFFER);
    ASSERT(__this->disp_adr);
    buffer_manager_init(__this->disp_hdl, __this->disp_adr, buffer_alloc_adr_len, buf_num);
    block_height = check_line;
    block_number = (height % block_height) ? ((height / block_height) + 1) : (height / block_height);
    void *buffer_hdl = __this->disp_hdl;
#else
    void *buffer_hdl = dc->buffer_hdl;
#endif
    int draw_height;
    struct rect draw_rect = {0};
    __this->gpu_line_end_flag = 0; // 最后一块标志初始为0

    //特效准备
    struct ui_page_priv *page_ctrl = ui_page_get_param();
    struct ui_page_draw page_draw = {0};
    if ((dc->draw_state == GPU_SYNTHESIS) && (dc->index == 0)) {
        struct ui_effect_module *effmod_hd =  ui_effect_get_handle_by_style(page_ctrl->mode);
        if (effmod_hd && effmod_hd->effect_draw) {
            effmod_hd->effect_draw(mult_list, &page_draw);
        }
    }
    u8 *argb8565_buf = NULL;			//子链缓存buf
    pJLGPUTaskHead_t sub_head = NULL;	//子任务链头
    u32 *addr_buf = NULL;				//group任务地址信息
    int group_num = 0; 					//group数量
    if (dc->draw_state == GPU_DRAW_EACH_GROUP) {
        //创建子任务链头
        sub_head = jlgpu_create_task_list_head();
        jlgpu_set_task_list_out_format(sub_head, GPU_OUT_FORMAT_ARGB8565);
        //申请子任务链输出空间
        int out_stride = (width * gpu_get_format_bpp(GPU_OUT_FORMAT_ARGB8565) + 7) / 8;
        int argb8565_buf_len = out_stride * block_height;
        argb8565_buf  = zalloc(argb8565_buf_len);
        //遍历group数量
        pJLGPUTaskUnit_t sub_taskp_curr = jlgpu_task_find_first_task(gpu_task_head);
        pJLGPUTaskUnit_t sub_taskp_last = sub_taskp_curr;
        while (sub_taskp_curr) {
            if ((sub_taskp_curr->info.group != sub_taskp_last->info.group) || (jlgpu_find_next_task(sub_taskp_curr) == NULL)) {
                group_num ++;
            }
            sub_taskp_last = sub_taskp_curr;
            sub_taskp_curr =  jlgpu_find_next_task(sub_taskp_curr);
        }
        //记录每个group的第一个和最后一个任务的地址
        addr_buf  = zalloc(2 * group_num * sizeof(int));
        int addr_offset = 0;
        sub_taskp_curr = jlgpu_task_find_first_task(gpu_task_head);
        sub_taskp_last = sub_taskp_curr;
        while (sub_taskp_curr) {
            if ((sub_taskp_curr->info.group != sub_taskp_last->info.group) || (jlgpu_find_next_task(sub_taskp_curr) == NULL)) {
                //前个group的尾
                void *task_addr_last = jlgpu_unit_to_instruction(sub_taskp_last);
                //后个group的头
                void *task_addr_curr = jlgpu_unit_to_instruction(sub_taskp_curr);
                addr_buf[addr_offset] = (u32)task_addr_last;
                addr_buf[addr_offset + 1] = (u32)task_addr_curr;
                addr_offset += 2;
            }
            sub_taskp_last = sub_taskp_curr;
            sub_taskp_curr =  jlgpu_find_next_task(sub_taskp_curr);
        }

    }
    IO_FRAME_HIGH();
    /* printf("@@@@@ list_total: %d\n", list_total); */
    /* 开始分块合成并推屏 */
    for (int i = 0; i < block_number; i++) {

        /* 刷新最后一个分块时，最后一块标志为1 */
        if (i == (block_number - 1)) {
            __this->gpu_line_end_flag = 1;
        }

        /* Calculation current draw block height. */
        if ((i + 1) * block_height < height) {
            draw_height = block_height;
        } else {
            draw_height = height - (i * block_height);
        }

        /* Move to dirty block area. */
        draw_rect.left	 = refresh_rect.left + 0;
        draw_rect.top	 = refresh_rect.top + i * block_height;
        draw_rect.width  = width;
        draw_rect.height = draw_height;
        /* DUMP_RECT(__func__, __LINE__, "draw_rect", &draw_rect); */

        if (dc->draw_state == GPU_SYNTHESIS_AND_DRAW) {

            /* 普通页面合成并刷新 */
            u8 *idle_buf = get_idle_buffer_by_handler(buffer_hdl, &__this->buf_idle_sem, 500, 0);

            /* 清空缓存buf */
            int out_bpp = gpu_get_format_bpp(jlgpu_chain_head_out_format(gpu_task_head)) / 8;
            memset(idle_buf, 0, draw_rect.width * draw_rect.height * out_bpp);


            if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
                int n;
                int bottom_page_exist = 0;
                int num = jlgpu_mult_task_list_get(mult_list, NULL);
                int *index_tab = (int *)malloc(num * sizeof(int));
                jlgpu_mult_task_list_get(mult_list, index_tab);

                int bottom_page_num = ui_multi_page_get_index_by_prio(PAGE_PRIO_BOTTOM, NULL);
                if (bottom_page_num > 0) {
                    int *bottom_page_tab = (int *)malloc(bottom_page_num * sizeof(int));
                    ASSERT(bottom_page_tab);
                    ui_multi_page_get_index_by_prio(PAGE_PRIO_BOTTOM, bottom_page_tab);
                    for (n = 0; n < bottom_page_num; n++) {
                        pJLGPUTaskHead_t task_head = jlgpu_mult_task_head_by_index(mult_list, bottom_page_tab[n]);
                        if (task_head) {
                            bottom_page_exist = 1;
                            jlgpu_set_task_list_out_buf(task_head, idle_buf, &draw_rect, 0, 0);
                            jlgpu_task_list_run(task_head);
                        }
                    }
                    free(bottom_page_tab);
                }

                for (n = 0; n < num; n++) {
                    if (!ui_multi_page_check_index(index_tab[n])) {
                        pJLGPUTaskHead_t task_head = jlgpu_mult_task_head_by_index(mult_list, index_tab[n]);
                        if (task_head) {
                            if (bottom_page_exist) {
                                jlgpu_task_enable_by_id(task_head, 0, 0, 0);
                                jlgpu_task_layer_enable(task_head, 0);
                            }
                            /* 设置GPU任务链合成区域和输出buf */
                            jlgpu_set_task_list_out_buf(task_head, idle_buf, &draw_rect, 0, 0);
                            /* 启动GPU合成 */
                            jlgpu_task_list_run(task_head);
                        }
                    }
                }

                int top_page_num = ui_multi_page_get_index_by_prio(PAGE_PRIO_TOP, NULL);
                if (top_page_num > 0) {
                    int *top_page_tab = (int *)malloc(top_page_num * sizeof(int));
                    ASSERT(top_page_tab);
                    ui_multi_page_get_index_by_prio(PAGE_PRIO_TOP, top_page_tab);
                    for (n = 0; n < top_page_num; n++) {
                        pJLGPUTaskHead_t task_head = jlgpu_mult_task_head_by_index(mult_list, top_page_tab[n]);
                        if (task_head) {
                            jlgpu_task_enable_by_id(task_head, 0, 0, 0);
                            jlgpu_task_layer_enable(task_head, 0);
                            jlgpu_set_task_list_out_buf(task_head, idle_buf, &draw_rect, 0, 0);
                            jlgpu_task_list_run(task_head);
                        }
                    }
                    free(top_page_tab);
                }
                free(index_tab);
            } else {
                /* 设置GPU任务链合成区域和输出buf */
                jlgpu_set_task_list_out_buf(gpu_task_head, idle_buf, &draw_rect, 0, 0);
                /* 启动GPU合成 */
                jlgpu_task_list_run(gpu_task_head);
            }

            /* 设置缓存buf为等待状态，等待DBI模块推屏 */
            set_buffer_pend(buffer_hdl, idle_buf);

            /* 启动DBI模块推屏 */
            if (JLUI_GPU_DMA_TO_PSRAM) {
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
                psram_flush_buffer_start(buffer_hdl, idle_buf, &draw_rect, i);
#endif
            } else {
                lcd_flush_buffer_start(buffer_hdl, idle_buf, &draw_rect, i);
            }
        } else if (dc->draw_state == GPU_SYNTHESIS) {
            /* 划屏时合成并刷新 */
            u8 *idle_buf = get_idle_buffer_by_handler(buffer_hdl, &__this->buf_idle_sem, 500, 0);
            pJLGPUTaskHead_t dc_head = gpu_task_head;
            /* int out_bpp = gpu_get_format_bpp(dc_head->out_param.format) / 8; */
            int out_bpp = gpu_get_format_bpp(jlgpu_chain_head_out_format(dc_head)) / 8;
            memset(idle_buf, 0, draw_rect.width * draw_rect.height * out_bpp);
            int out_stride = width * out_bpp;
            if (page_draw.new_task_list) { //重排链表
                int bottom_page_exist = 0;
                int n;

                if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
                    int bottom_page_num = ui_multi_page_get_index_by_prio(PAGE_PRIO_BOTTOM, NULL);
                    if (bottom_page_num > 0) {
                        int *bottom_page_tab = (int *)malloc(bottom_page_num * sizeof(int));
                        ASSERT(bottom_page_tab);
                        ui_multi_page_get_index_by_prio(PAGE_PRIO_BOTTOM, bottom_page_tab);
                        for (n = 0; n < bottom_page_num; n++) {
                            pJLGPUTaskHead_t task_head = jlgpu_mult_task_head_by_index(mult_list, bottom_page_tab[n]);
                            if (task_head) {
                                bottom_page_exist = 1;
                                jlgpu_set_task_list_out_buf(task_head, idle_buf, &draw_rect, 0, 0);
                                jlgpu_task_list_run(task_head);
                            }
                        }
                        free(bottom_page_tab);
                    }
                }

                for (int idx = 0; idx < page_draw.list_total; idx++) {
                    pJLGPUTaskHead_t head = (pJLGPUTaskHead_t)page_draw.new_task_list[idx];
                    page_draw.rec_gpu_rect[idx].top = draw_rect.top;
                    page_draw.rec_gpu_rect[idx].height = draw_rect.height;
                    if (page_ctrl->mode > 2) {
                        if (page_draw.new_list_create & BIT(idx)) {
                            jlgpu_set_task_list_out_buf(head, idle_buf, &page_draw.rec_gpu_rect[idx], page_draw.rec_gpu_rect[idx].left, out_stride);
                        } else {
                            jlgpu_set_task_list_out_buf(head, idle_buf, &page_draw.rec_gpu_rect[idx], page_draw.rec_lcd_rect[idx].left, out_stride);
                        }
                    } else {
                        struct rect block_r;
                        block_r.top = draw_rect.top;
                        block_r.width = page_draw.rec_gpu_rect[idx].width;
                        block_r.height = draw_rect.height;
                        block_r.left = page_draw.rec_gpu_rect[idx].left + draw_rect.left - page_draw.rec_lcd_rect[idx].left;
                        if (block_r.left < 0) {
                            block_r.width = draw_rect.width;
                        }
                        jlgpu_set_task_list_out_buf(head, idle_buf, &block_r, 0, out_stride);
                    }

                    if (bottom_page_exist) {
                        jlgpu_task_enable_by_id(head, 0, 0, 0);
                        jlgpu_task_layer_enable(head, 0);
                    }
                    extern int ui_page_get_mode();
                    if (ui_page_get_mode() == PAGE_MOVE_MODE_CENTER_FLIP) {
                        if (jlgpu_chain_head_status(head) == HEAD_STATUS_READY) {
                            jlgpu_task_list_run(head); /*! 中心轴翻转时渲染位置 */
                        }
                    } else {
                        jlgpu_task_list_run(head);
                    }
                }

                if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
                    int top_page_num = ui_multi_page_get_index_by_prio(PAGE_PRIO_TOP, NULL);
                    if (top_page_num) {
                        int *top_page_tab = (int *)malloc(top_page_num * sizeof(int));
                        ASSERT(top_page_tab);
                        ui_multi_page_get_index_by_prio(PAGE_PRIO_TOP, top_page_tab);
                        for (n = 0; n < top_page_num; n++) {
                            pJLGPUTaskHead_t task_head = jlgpu_mult_task_head_by_index(mult_list, top_page_tab[n]);
                            if (task_head) {
                                jlgpu_task_enable_by_id(task_head, 0, 0, 0);
                                jlgpu_task_layer_enable(task_head, 0);
                                jlgpu_set_task_list_out_buf(task_head, idle_buf, &draw_rect, 0, 0);
                                jlgpu_task_list_run(task_head);
                            }
                        }
                        free(top_page_tab);
                    }
                }
            }
            set_buffer_pend(buffer_hdl, idle_buf);
            if (JLUI_GPU_DMA_TO_PSRAM) {
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
                psram_flush_buffer_start(buffer_hdl, idle_buf, &draw_rect, i);
#endif
            } else {
                lcd_flush_buffer_start(buffer_hdl, idle_buf, &draw_rect, i);
            }
        } else if (dc->draw_state == GPU_DRAW_EACH_GROUP) {
            //获取分块buf
            u8 *idle_buf = get_idle_buffer_by_handler(buffer_hdl, &__this->buf_idle_sem, 500, 0);
            /* sub_head->gpu_task_base_adr = ((pJLGPUTaskHead_t)gpu_task_head)->gpu_task_base_adr; */
            jlgpu_chain_set_base_task(sub_head, jlgpu_chain_base_task_addr((pJLGPUTaskHead_t)gpu_task_head));
            jlgpu_chain_set_tail_task(sub_head, jlgpu_chain_tail_task_addr((pJLGPUTaskHead_t)gpu_task_head));
            jlgpu_set_task_list_out_buf(sub_head, argb8565_buf, &draw_rect, 0, 0);
            for (int group_i = 0; group_i < group_num; group_i++) {
                void *task_addr_last = (void *)addr_buf[2 * group_i];
                void *task_addr_curr = (void *)addr_buf[2 * group_i + 1];
                //断开任务链
                gpu_task_add_to_next(task_addr_last, NULL);
                //合成当前group
                jlgpu_task_list_run(sub_head);
                //blend
                jlui_argb8565_to_rgb565(idle_buf, &draw_rect, argb8565_buf, &draw_rect);
                //断链重连
                gpu_task_add_to_next(task_addr_last, task_addr_curr);
                //指向下一个group
                /* sub_head->gpu_task_base_adr = task_addr_curr; */
                jlgpu_chain_set_base_task(sub_head, task_addr_curr);
            }
            //输出
            /* 设置缓存buf为等待状态，等待DBI模块推屏 */
            set_buffer_pend(buffer_hdl, idle_buf);
            /* 启动DBI模块推屏 */
            lcd_flush_buffer_start(buffer_hdl, idle_buf, &draw_rect, i);
        }
    }
    IO_FRAME_LOW();
    if (argb8565_buf) {
        free(argb8565_buf);
    }
    if (sub_head) {
        /* sub_head->gpu_task_base_adr  = NULL;//只释放表头 */
        jlgpu_chain_task_init(sub_head);
        jlgpu_delete_task_list_head(sub_head);
    }
    if (addr_buf) {
        free(addr_buf);
    }

    for (int i = 0; i < page_draw.list_total; i++) {
        if (page_draw.new_list_create & BIT(i)) {
            jlgpu_task_list_copy_destroy(page_draw.new_task_list[i]);
        }
    }

    if (page_draw.new_task_list) {
        free(page_draw.new_task_list);
    }
    if (page_draw.rec_gpu_rect) {
        free(page_draw.rec_gpu_rect);
    }
    if (page_draw.rec_lcd_rect) {
        free(page_draw.rec_lcd_rect);
    }
    if (page_draw.rec_page_rect) {
        free(page_draw.rec_page_rect);
    }
    if ((__this->redraw_mode == GPU_ASYN_REDRAW) && (dc->draw_state ==  GPU_SYNTHESIS_AND_DRAW)) { //同步刷新不释放
        gpu_cache_valid_sub();
    }
    jpeg_module_opj_set_invaild_by_index(dc->index);

    __this->gpu_is_busy = false;
    /* os_sem_post(&__this->gpu_sem); */

    /* GPU 合成完成, 释放圆环绘制等任务在 gpu_buf_add 中临时申请的 clut 缓冲 */
    extern void gpu_buf_list_free();
    gpu_buf_list_free();

    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
        async_buffer_clr();
    }
    void jljpeg_stream_src_data_unlock();
    jljpeg_stream_src_data_unlock();
    return 0;
}


void *jlgpu_scheduler_draw_mult_task_list(struct draw_context *dc)
{
    pJLGPUMultTaskList_t mult_list = dc->gpu_mult_list;
    pJLGPUMultTaskList_t next_mult = NULL;

    if (__this->redraw_mode == GPU_ASYN_REDRAW) {
        /* 获取多任务链管理影子组句柄，并将DC指针的多任务链管理、GPU任务链替换为影子组 */
        if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
            jlgpu_scheduler_wait_sync();

            next_mult = jlgpu_mult_task_list_switch_root(mult_list);
            dc->gpu_task_head = jlgpu_mult_task_head_idle_by_index(next_mult, dc->index, dc->gpu_task_head);
            dc->gpu_mult_list = next_mult;
            __this->mult_list = next_mult;

            if ((dc->draw_state == GPU_SYNTHESIS) && (dc->index)) {
                return next_mult;
            }
        } else {
            next_mult = jlgpu_mult_task_list_switch_root(mult_list);
            dc->gpu_task_head = jlgpu_mult_task_head_by_index(next_mult, dc->index);
            dc->gpu_mult_list = next_mult;
            __this->mult_list = next_mult;

            if ((dc->draw_state == GPU_SYNTHESIS) && (dc->index)) {
                return next_mult;
            }
            jlgpu_scheduler_wait_sync();
        }

        int err;
        int msg[32];
        msg[0] = GPU_MSG_DRAW;
        msg[1] = (int)dc->index;
        msg[2] = (int)mult_list;
        msg[3] = 0;

        /* 同步DC指针内容 */
        memcpy(&__this->dc, dc, sizeof(struct draw_context));

        /* 发送队列消息到GPU系统任务，由GPU系统任务调度GPU启动合成 */
        err = os_taskq_post_type(GPU_TASK_NAME, msg[0], 2, &msg[1]);

        if (err != OS_NO_ERR) {
            ASSERT(0);
            return NULL;
        }

        return next_mult;
    } else {
        if ((dc->draw_state == GPU_SYNTHESIS) && (dc->index)) {
            return dc->gpu_mult_list;
        }

        /* 串行刷新时，直接调用GPU合成接口 */
        jlgpu_scheduler_kick_start(dc->gpu_mult_list, dc);

        return dc->gpu_mult_list;
    }
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_scheduler_draw_task_list 绘制指定GPU任务链并推屏
 *
 * @Params head 待绘制的GPU任务链
 * @Params disp_buf 显存buf
 * @Params disp_buf_size 显存buf大小
 */
/* ------------------------------------------------------------------------------------*/
static void jlgpu_scheduler_draw_task_list(pJLGPUTaskHead_t head, void *disp_buf, int disp_buf_size)
{
    /* printf("%s(), head: 0x%x, disp_buf: 0x%x, buf_size: %d\n", __func__, (u32)head, (u32)disp_buf, disp_buf_size); */
    /* jlgpu_dump_all_task(head, __func__, __LINE__); */

    /* GPU 任务链绘制区域 */
    struct rect *win_rect = &head->win_rect;
    /* printf("draw rect[%d, %d, %d, %d]\n", win_rect->left, win_rect->top, win_rect->width, win_rect->height); */

    /* int out_format = head->out_param.format; */
    int out_format = jlgpu_chain_head_out_format(head);
    int bpp_byte = gpu_task_get_format_bpp(out_format) >> 3;

    /* 计算分块高度 */
    int block_buf_size = disp_buf_size >> 1;
    int block_height = block_buf_size / (win_rect->width * bpp_byte);
    /* printf("block_height: %d\n", block_height); */

    /* 计算分块数量 */
    int block_number = (win_rect->height % block_height) ? ((win_rect->height / block_height) + 1) : (win_rect->height / block_height);
    /* printf("block_number: %d\n", block_number); */

    /* 等待GPU空闲 */
    jlgpu_scheduler_wait_sync();

    /* 准备buf管理句柄，推屏结束后释放 */
    void *buf_hdl = NULL;
#if TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE
    if (!disp_buf) {
        __this->disp_adr = jlui_malloc(disp_buf_size, UI_RAM_SRAM, UI_MODULE_BUFFER);
        disp_buf = __this->disp_adr;
    }
#endif
    buffer_manager_init(buf_hdl, disp_buf, disp_buf_size, 2);

    /* 配置GPU渲染参数 */
    __this->gpu_is_busy = true;
    __this->wait_gpu_sem = true;
    __this->gpu_line_end_flag = false;
    __this->draw_task_list = true; // 绘制GPU任务链标志，用于释放buf管理句柄判断

    int draw_height;
    struct rect draw_rect = {0};

#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
    if (JLUI_GPU_DMA_TO_PSRAM) {
        psram_double_buffer_init();
        if (win_rect->width < __this->lcd_width || win_rect->height < __this->lcd_height) {
            if (__this->psram_buf_adr) {
                wait_buf_to_idle(__this->psram_buf_hdl, __this->psram_buf_adr, &__this->buf_idle_sem, 500); // 局部刷新，已经有buf，等待buf空闲
            } else {
                __this->psram_buf_adr = get_idle_buffer_by_handler(__this->psram_buf_hdl, &__this->buf_idle_sem, 500, 0); // 全屏刷新，直接获取空闲buf实现乒乓
            }
        } else {
            __this->psram_buf_adr = get_idle_buffer_by_handler(__this->psram_buf_hdl, &__this->buf_idle_sem, 500, 0); // 全屏刷新，直接获取空闲buf实现乒乓
        }
#else
    if (0) {
#endif
    } else {
        lcd_set_draw_area(win_rect->left, (win_rect->left + win_rect->width - 1), win_rect->top, (win_rect->top + win_rect->height - 1));
    }

    for (int i = 0; i < block_number; i++) {

        if ((i + 1) * block_height < win_rect->height) {
            draw_height = block_height;
        } else {
            draw_height = win_rect->height - (i * block_height);
            __this->gpu_line_end_flag = true;
        }

        draw_rect.left	 = win_rect->left + 0;
        draw_rect.top	 = win_rect->top + i * block_height;
        draw_rect.width  = win_rect->width;
        draw_rect.height = draw_height;
        /* printf("draw rect [%d, %d, %d, %d]\n", draw_rect.left, draw_rect.top, draw_rect.width, draw_rect.height); */

        u8 *idle_buf = get_idle_buffer_by_handler(buf_hdl, &__this->buf_idle_sem, 500, 0);
        memset(idle_buf, 0x00, block_buf_size);

        jlgpu_set_task_list_out_buf(head, idle_buf, &draw_rect, 0, 0);
        jlgpu_task_list_run(head);

        set_buffer_pend(buf_hdl, idle_buf);

#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
        if (JLUI_GPU_DMA_TO_PSRAM) {
            psram_flush_buffer_start(buf_hdl, idle_buf, &draw_rect, i);
#else
        if (0) {
#endif
        } else {
            lcd_flush_buffer_start(buf_hdl, idle_buf, &draw_rect, i);
        }
    }
    __this->gpu_is_busy = false;
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_task GPU 异步合成任务
 *
 * @Params p
 */
/* ------------------------------------------------------------------------------------*/
static void jlgpu_task(void *p)
{
    int ret;
    int msg[32] = {0};

    while (1) {
        /* 等待消息队列 */
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }

        switch (msg[0]) {
        case GPU_MSG_DRAW:
            /* 如果接收到绘制消息，启动GPU合成 */
            pJLGPUMultTaskList_t mult_list = (pJLGPUMultTaskList_t)msg[2];

            jlgpu_scheduler_kick_start(mult_list, &__this->dc);

            break;
        case GPU_MSG_DUMP:
            /* 将SRAM的数据转储到PSRAM，并将行排列转换为块排列 */
            /* 参数顺序：dst, src, w, h, format, buf_hdl */
            /* printf("dump data dst: 0x%x, src: 0x%x, w: %d, h: %d, fmt: %d\n", msg[1], msg[2], msg[3], msg[4], msg[5]); */
            texture_line_to_tile_base((void *)msg[1], (void *)msg[2], msg[3], msg[4], msg[5]);
            set_buffer_idle((void *)msg[6], (void *)msg[2]);
            break;
        case GPU_MSG_DRAW_LIST:
            pJLGPUTaskHead_t head = (pJLGPUTaskHead_t)msg[1];
            void *disp_buf = (void *)msg[2];
            int disp_buf_size = msg[3];
            jlgpu_scheduler_draw_task_list(head, disp_buf, disp_buf_size);
            break;
        case GPU_MSG_OTHER:
            break;
        default:
            break;
        }
    }
}
static void gpu_load_data_cb(void *priv, void *dst, void *src, int len)
{
    /* printf("%s(), priv: 0x%x, dst: 0x%x, src: 0x%x, len: %d\n", __func__, (u32)priv, (u32)dst, (u32)src, len); */
    UI_RESFILE *fp = (UI_RESFILE *)priv;
    res_fseek(fp, (u32)src, SEEK_SET);
    res_fread(fp, dst, len);
    if (GPU_INPUT_TO_NOCACHE) {
        if (UI_ADDR_IN_PSRAM_CACHE(dst)) {
            psram_flush_cache(dst, len);
        }
    }
}

void jlgpu_scheduler_init(
    void *(*malloc)(int, u32, u32),
    void (*free)(void *, u32, u32),
    u16 lcd_width, u16 lcd_height)
{
    __this->lcd_width = lcd_width;
    __this->lcd_height = lcd_height;
    if (__this->soft_max_us == 0) {
        __this->soft_max_us = 1000000 / 50;
    }

    lcd_draw_set_callback(lcd_flush_finish_cb);			// 设置lcd推屏结束回调

    /* GPU模块初始化 */
    /* jlgpu_module_init(malloc, free, __this->lcd_width, __this->lcd_height); */
    jlgpu_module_init(__this->lcd_width, __this->lcd_height);

    /* 兼容707N芯片，将GPU窗口移动到2K中间 */
    struct rect win_rect;
    rect_width(&win_rect)  = __this->lcd_width;
    rect_height(&win_rect) = __this->lcd_height;
    rect_left(&win_rect)   = (2047 - rect_width(&win_rect)) / 2;
    rect_top(&win_rect)    = (2047 - rect_height(&win_rect)) / 2;
    jlgpu_set_win_rect(&win_rect);

    /* 多链表管理初始化 */
    jlgpu_mult_task_list_common_init();
    jlgpu_mult_task_list_shadow_init();

    /* JPEG模块初始化 */
    extern int jpeg_module_init();
    jpeg_module_init();

    if (config_gpu_cache_psram_jpeg_en || config_gpu_cache_psram_file_res_en) {
        gpu_cache_set_alloc_cb(malloc, free);
        gpu_cache_init(gpu_load_data_cb, jlgpu_scheduler_async_free_buf);
    } else {
        gpu_cache_set_alloc_cb(malloc, free);
        gpu_cache_init(NULL, jlgpu_scheduler_async_free_buf);
    }

    /* 默认使用的多任务链管理句柄 */
    __this->mult_list = jlgpu_mult_task_list_get_common_root();

    /* 创建GPU任务 */
    task_create(jlgpu_task, NULL, GPU_TASK_NAME);

#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
    task_create(jllcd_task, NULL, LCD_TASK_NAME);
#endif

    /* 创建同步信号量 */
    os_sem_create(&__this->gpu_sem, 1);
    os_sem_create(&__this->lcd_te_sem, 1);
    os_sem_create(&__this->buffer_sem, 1);
    os_sem_create(&__this->buf_idle_sem, 1);
    os_sem_create(&__this->te_dly_sem, 1);

    /* 初始化标志 */
    __this->has_init = true;

    /* 默认开异步刷新 */
    __this->redraw_mode = GPU_ASYN_REDRAW;
    __this->gpu_is_busy = false;
}

void jlgpu_scheduler_free()
{
    jlgpu_module_free();

    if (config_gpu_cache_psram_jpeg_en || config_gpu_cache_psram_file_res_en) {
        gpu_cache_free();
    }

    __this->has_init = false;
}



/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_create_task_list_by_psram_img 将PSRAM中缓存的帧buf作为图片创建GPU指令链表
 *
 * @Params psram_img PSRAM中缓存的图片地址
 * @Params format PSRAM中缓存的图片数据格式，一般为RGB565格式
 * @Params img_w PSRAM中缓存的帧数据宽度（页面原来的宽度）
 * @Params img_h PSRAM中欢度的帧数据高度（页面原来的高度）
 *
 * @return GPU任务链
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUTaskHead_t jlgpu_create_task_list_by_psram_img(void *psram_img, int format, int img_w, int img_h)
{
    pJLGPUTaskHead_t head = jlgpu_create_task_list_head();
    jlgpu_set_task_list_out_format(head, format);

    jlgpu_task_texture_param_init(1, 1);
    task_param.format = GPU_FORMAT_RGB565;
    task_param.image.width = (img_w + 7) / 8 * 8;
    task_param.image.height = (img_h + 7) / 8 * 8;

    task_param.draw.left = 0;
    task_param.draw.top = 0;
    task_param.draw.width = task_param.image.width;
    task_param.draw.height = task_param.image.height;
    jlgpu_get_win_rect(&task_param.area);
    task_param.texture.not_compress = true;
    if (psram_cache_check(psram_img)) {
        task_param.texture.data = (u8 *)psram_cache_2_no_cache(psram_img);	// 转为nocache地址
    } else {
        task_param.texture.data = (u8 *)(psram_img);
    }
    task_param.texture.adr_mode = 0;	// 使用块排列

    /* 计算裁剪参数 */
    if ((task_param.image.width != img_w) || (task_param.image.height != img_h)) {
        task_param.crop_en = true;
        task_param.texture.crop.left = 0;
        task_param.texture.crop.top = 0;
        task_param.texture.crop.width = img_w;
        task_param.texture.crop.height = img_h;
    }

    jlgpu_update_task_by_id(head, 1, 1, &task_param);

    return head;
}


void *jlgpu_task_list_dump_to_psram(pJLGPUTaskHead_t head, int w, int h, void *disp_buf, int disp_buf_size)
{
    /* 获取GPU任务链输出格式 */
    /* int out_format = head->out_param.format; */
    int out_format = jlgpu_chain_head_out_format(head);

    /* 宽高做8对齐 */
    int out_w = ((w + 7) >> 3) << 3;
    int out_h = ((h + 7) >> 3) << 3;

    /* 计算需要使用的PSRAM缓存大小 */
    int out_size = (gpu_task_get_format_bpp(out_format) * out_w * out_h) >> 3;

    /* 申请PSRAM输出缓存 */
    void *out_addr = malloc_psram(out_size);
    ASSERT(out_addr, "Error, malloc psram block stroge faild! size: %d\n", out_size);
    /* return NULL; // 申请内存失败，说明内存不够缓存一帧了 */

    /* 准备buf管理句柄 */
    void *buf_hdl;
#if TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE
    if (!disp_buf) {
        __this->disp_adr = jlui_malloc(disp_buf_size, UI_RAM_SRAM, UI_MODULE_BUFFER);
        disp_buf = __this->disp_adr;
    }
#endif
    buffer_manager_init(buf_hdl, disp_buf, disp_buf_size, 2);

    /* 根据显存大小，重新计算分块高度，并将高度调整到向下8对齐 */
    // 注意，这里直接将输出设定为RGB565
    int block_h = disp_buf_size / 2 / 2 / w / 8 * 8;
    int draw_h = block_h;

    /* 计算分块数量，向上取整 */
    int block_num = (h + block_h - 1) / block_h;

    int msg[32];
    msg[0] = GPU_MSG_DUMP;
    struct rect draw_rect = {0};

    /* time_count_start(3); */
    /* 分块合成，并分块将输出转储到PSRAM块排列方式 */
    for (int i = 0; i < block_num; i++) {
        if (i == (block_num - 1)) {
            draw_h = h - (i * block_h); // 最后一个分块调整高度
        }
        draw_rect.left = 0;
        draw_rect.top = i * block_h;
        draw_rect.width = w;
        draw_rect.height = draw_h;

        /* 获取空闲buf */
        u8 *idle_buf = get_idle_buffer_by_handler(buf_hdl, &__this->buf_idle_sem, 500, 0);
        int out_bpp = gpu_get_format_bpp(jlgpu_chain_head_out_format(head)) / 8;
        memset(idle_buf, 0, draw_rect.width * draw_rect.height * out_bpp);
        jlgpu_set_task_list_out_buf(head, idle_buf, &draw_rect, 0, 0);
        jlgpu_task_list_run(head);

        /* 将合成好的分块post到GPU任务转储到PSRAM */
        msg[1] = (u32)out_addr + i * out_w * block_h * 2;
        msg[2] = (u32)idle_buf;
        msg[3] = draw_rect.width;
        msg[4] = draw_rect.height;
        msg[5] = out_format;
        msg[6] = (int)buf_hdl;
        msg[7] = 0;

        set_buffer_used(buf_hdl, idle_buf);
        int err = os_taskq_post_type(GPU_TASK_NAME, msg[0], 7, &msg[1]);
        if (err != OS_NO_ERR) {
            ASSERT(0);
            return NULL;
        }

    }
    buffer_manager_free(buf_hdl);
    /* time_count_end(3); */

    return out_addr;
}

#endif


