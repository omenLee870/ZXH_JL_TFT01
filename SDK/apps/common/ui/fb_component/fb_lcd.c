
/******************************************************************************
* File:             fb_lcd.c
*
* Author:
* Created:          02/28/24
* Description:      1.fb lcd 输出端口功能,实现驱屏相关功能
*****************************************************************************/
#include "asm/includes.h"
#include "lcd/lcd_conf.h"
#include "fs/fs.h"
#include "fb.h"

#include "sdk_config.h"

#if TCFG_UI_ENABLE

#define LOG_TAG_CONST       FB_LCD
#include "debug.h"

#define FB_LCD_FRAME_RATE_DEBUG_EN         0   //打印lcd数据更新帧率

struct fb_lcd_t {
    void *lcd;
    void *next_disp;/* 下一帧待显示的next_fb地址 */
    volatile u8 lcd_kick_start;
    u8 lcd_type;
    u8 interpolation_en;
    u16 out_buf_x; //显存x偏移
    u16 out_buf_y; //显存y偏移
    u16 out_buf_w; //显存宽
    u16 out_buf_h; //显存高
    u16 out_w;     //实际的推屏宽
    u16 out_h;     //实际的推屏高
    u32 out_buf[2]; //显存buffer
    u8  out_buf_num;
    u16 out_format;
    u16 rotate;
    volatile u8 out_buf_index;
    volatile u8 lcd_push_task_run;
    OS_SEM lcd_async_sem;
    OS_SEM lcd_async_busy_sem;
};

static u8 lcd_buf_num = 2;

static volatile u8  g_dmm_line_period; //dmm读取一行所需时间
static volatile u32 g_last_vsync_trig_time;
static volatile u64 g_vsync_start_time;
static volatile u16 g_dmm_line;

static struct fb_lcd_t _fb_lcd[FB_MAX_OUT_NUM];
#define  __this   (&_fb_lcd[id])

enum {
    FB_LCD_MSG_ASYNC_WAIT_VSYNC,
    FB_LCD_MSG_ASYNC_UPDATE,
};
int fb_frame_buf_rotate(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int src_stride,
                        int dst_width, int dst_height, int dst_stride, int degree, int xoffset, int yoffset,
                        int in_format, int out_format, uint8_t mirror);
void fb_lcd_frame_buf_update(u8 id, u8 *frame_buffer);


static void __fb_buf_clear(u8 *frame_buffer, u32 frame_size, u32 format)
{
    if (format == FB_COLOR_FORMAT_YUV422) {
        for (int i = 0; i < frame_size; i += 4) {
            frame_buffer[i] = 0x00;
            frame_buffer[i + 1] = 0x80;
            frame_buffer[i + 2] = 0x00;
            frame_buffer[i + 3] = 0x80;
        }
        DcuFlushRegion((void *)frame_buffer, frame_size);
    }
}
static void __fb_lcd_set_buf_num(u8 num)
{
    lcd_buf_num = num;
}

static int __fb_lcd_line_done_hook_func(int line)
{
    return 0;
}

static u16 __fb_abs(int x, int y, u8 *flag)
{
    if ((x) > (y)) {
        if (flag) {
            *flag = 0;
        }
        return ((x) - (y)) ;
    }
    if (flag) {
        *flag = 1;
    }
    return ((y) - (x));
}


//数据与lcd显存交换
static int __fb_lcd_frame_swap_flush(u8 id, u8 *frame_buffer)
{
    return 0;
}

#if FB_LCD_FRAME_RATE_DEBUG_EN
static int __fb_lcd_framerate_calc(u8 id)
{
    static u32 time[2];
    static u16 frames[2];
    u32 cur_time = 0;

    frames[id]++;
    cur_time = get_system_ms();
    if (time_after(cur_time, time[id])) {
        log_info("lcd[%d] update: %dfps", id, frames[id] * 1000 / (1000 + (cur_time - time[id])));
        time[id] = get_system_ms() + 1000;
        frames[id] = 0;
    }
    return 0;
}
#endif

static void dump_frame_buffer(uint8_t *buffer, int buffer_size)
{
    static int fcnt = 0;
    char name[64];
    sprintf(name, "storage/sd0/C/frame%d.dat", fcnt);
    fcnt++;
    /* if(fcnt == 100) { */
    if (fcnt >= 100 && fcnt <= 120) {
        FILE *f = fopen(name, "w+");
        if (f) {
            printf("write framw buffer %s\n", name);
            fwrite(buffer, buffer_size, 1, f);
            fclose(f);
        }
    }
}

static void *__fb_lcd_open_device(u8 id)
{
    return NULL;
}


/**
 * @brief   检查当前buf是否lcd正在显示的buf冲突
 * @param:  需要检查的buf地址
 * @return: 0:不冲突 非0: 冲突
 **/
int fb_lcd_buf_is_busy(u8 id, u8 *buf)
{
    return 0;
}

/**
 * @brief   打开lcd显存并初始化lcd
 * @param:  out_info: 输出参数配置
 * @return: lcd 设备句柄
 **/
void *fb_lcd_device_open(struct fb_out_info *out_info)
{
    return (void *)1;
}

/**
 * @brief   关闭lcd设备
 * @param:  none
 * @return:
 **/
int fb_lcd_device_close(u8 id)
{
    return 0;
}
/**
 * @brief   异步推屏
 * @param:  id: 输出id
 * @param:  frame_buffer: 待更新帧数据
 * @return: none
 **/
void fb_lcd_frame_buf_async_flush(u8 id, u8 *frame_buffer)
{
    char async_lcd_task_name[20];
    int msg[3];
    msg[0] = FB_LCD_MSG_ASYNC_WAIT_VSYNC;
    msg[1] = (int)frame_buffer;
    sprintf(async_lcd_task_name, "async_lcd_task%d", id);
    int err =  os_taskq_post_type(async_lcd_task_name, Q_USER, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("fb_lcd_async_flush post err=%d\n", err);
    }
}

/**
 * @brief   更新lcd显示数据
 * @param:  id: lcd id号
 * @param:  frame_buffer: 待更新帧数据
 * @return: none
 **/
void fb_lcd_frame_buf_update(u8 id, u8 *frame_buffer)
{
#if 0 ////显存写卡调试
    int out_buf_size = __this->out_w * __this->out_h * dma2d_get_format_bpp(__this->out_format) / 8;
    dump_frame_buffer(frame_buffer, out_buf_size);
#endif

#if FB_LCD_FRAME_RATE_DEBUG_EN
    frame_period_us = get_system_us() - frame_period_start_us;;
    if (max_frame_period_us < frame_period_us) { //统计最大帧间隔
        max_frame_period_us = frame_period_us;
        log_info("fb lcd max frame period : %dus", max_frame_period_us);
    }
#endif

#if FB_LCD_FRAME_RATE_DEBUG_EN
    __fb_lcd_framerate_calc(id);
#endif
}

int fb_lcd_frame_async_wait(u8 id)
{
    if (!os_sem_valid(&__this->lcd_async_sem)) {
        return 0;
    }
    if (!os_sem_valid(&__this->lcd_async_busy_sem)) {
        os_sem_create(&__this->lcd_async_busy_sem, 1);
    }

    return os_sem_pend(&__this->lcd_async_busy_sem, 50);
}

/**
 * @brief   lcd显存交换
 * @param:  id: 输出id
 * @param:  frame_buffer:待交互的显存
 * @return: none
 **/
int fb_lcd_frame_buf_swap(u8 id, u8 *frame_buffer)
{
    int ret = 0;
    ret = __fb_lcd_frame_swap_flush(id, frame_buffer);
#if FB_LCD_FRAME_RATE_DEBUG_EN
    __fb_lcd_framerate_calc(id);
#endif
    return ret;
}
/**
 * @brief   获取lcd是否插值显示
 * @param:  none
 * @return: lcd插值与否
 **/
u8 fb_lcd_get_interpolation(u8 id)
{
    return __this->interpolation_en;
}
/**
 * @brief   获取lcd显存水平方向起始
 * @param:  none
 * @return: lcd显存水平起始坐标
 **/
u16 fb_lcd_get_buf_xoffset(u8 id)
{
    return __this->out_buf_x;
}

/**
 * @brief   获取lcd显存垂直方向起始
 * @param:  none
 * @return: lcd显存垂直起始坐标
 **/
u16 fb_lcd_get_buf_yoffset(u8 id)
{
    return __this->out_buf_y;
}

/**
 * @brief   获取lcd屏幕宽度
 * @param:  none
 * @return: lcd屏幕宽度
 **/
u16 fb_lcd_get_width(u8 id)
{
    return __this->out_w;
}
/**
 * @brief   获取lcd屏幕高度
 * @param:  none
 * @return: lcd屏幕高度
 **/
u16 fb_lcd_get_height(u8 id)
{
    return __this->out_h;
}

/**
 * @brief   获取lcd显存宽度
 * @param:  none
 * @return: lcd显存宽度
 **/
u16 fb_lcd_get_buf_width(u8 id)
{
    return __this->out_buf_w;
}
/**
 * @brief   获取lcd显存高度
 * @param:  none
 * @return: lcd显存高度
 **/
u16 fb_lcd_get_buf_height(u8 id)
{
    return __this->out_buf_h;
}

/**
 * @brief   获取lcd屏显输入格式
 * @param:  none
 * @return: lcd屏显输入格式
 **/
u16 fb_lcd_get_format(u8 id)
{
    return __this->out_format;
}
/**
 * @brief   获取lcd屏显旋转角度
 * @param:  none
 * @return: lcd屏显旋转角度
 **/
u16 fb_lcd_get_rotate(u8 id)
{
    return __this->rotate;
}

/**
 * @brief   获取lcd屏显显存buffer0
 * @param:  none
 * @return: lcd屏显显存buffer0
 **/
u32 fb_lcd_get_buf0(u8 id)
{
    return __this->out_buf[0];
}
/**
 * @brief   设置lcd屏显显存buffer0
 * @param:  id
 * @param:  buffer
 * @return: none
 **/
void fb_lcd_set_buf0(u8 id, u8 *buffer)
{
    __this->out_buf[0] = (u32)buffer;
}
/**
 * @brief   设置lcd屏显显存buffer1
 * @param:  id
 * @param:  buffer
 * @return: none
 **/
void fb_lcd_set_buf1(u8 id, u8 *buffer)
{
    __this->out_buf[1] = (u32)buffer;
}
/**
 * @brief   获取lcd屏显显存buffer1
 * @param:  none
 * @return: lcd屏显显存buffer1
 **/
u32 fb_lcd_get_buf1(u8 id)
{
    return __this->out_buf[1];
}
/**
 * @brief   获取lcd 空闲的显存buffer
 * @param:  none
 * @return: lcd 空闲的显存buffer
 **/
u32 fb_lcd_get_idle_buf(u8 id)
{
    return __this->out_buf[__this->out_buf_index];
}

/**
 * @brief   获取lcd 显存个数
 * @param:  none
 * @return: lcd 显存个数
 **/
u8 fb_lcd_get_buf_num(u8 id)
{
    return __this->out_buf_num;
}


/**
 * @brief   lcd双buf情况下，显存buf切换
 * @param:  none
 * @return: 切换后可用的lcd显存buf
 **/
u8 *fb_lcd_buf_index_swap(u8 id)
{
    /* 双输出缓存切换 */
    __this->out_buf_index = !__this->out_buf_index;
    return (u8 *)__this->out_buf[__this->out_buf_index];
}

#endif

