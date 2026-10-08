
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lv_ui_core.data.bss")
#pragma data_seg(".lv_ui_core.data")
#pragma const_seg(".lv_ui_core.text.const")
#pragma code_seg(".lv_ui_core.text")
#endif
/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "system/includes.h"
#include "ui/lcd/lcd_drive.h"
#include "ui/lcd/lcd_conf.h"
/*#include "ui/common/common.h"*/

#include "lv_port_disp.h"
#include <stdbool.h>

#include "jlgpu_framework.h"
#include "jlgpu_constomize.h"
#include "jlgpu_path.h"
#include "jlgpu_utils.h"

#include "fs/fs.h"
#include "draw/jlgpu/lv_draw_jlgpu.h"
#include "draw/jlgpu/lv_draw_jlgpu_ball.h"
#include "../../display/lv_display_private.h"
#include "../lv_draw.h"
#include "../../misc/lv_types.h"
#include "src/draw/lv_draw_buf_private.h"
#include "src/draw/lv_draw_private.h"
#include "lv_area_private.h"

#include "gxdma_driver.h"
#include "dmm_driver.h"

#include "asm/psram_api.h"
#include "gpu_port.h"

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
#warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
#define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
#warning Please define or replace the macro MY_DISP_VER_RES with the actual screen height, default value 240 is used for now.
#define MY_DISP_VER_RES    240
#endif

#define LV_UI_TRIPLE_BUFFER_EN   0//ui 3buffer

/* debug 耗时打印:gpu_run(GPU 渲染) 与 push(推屏) 耗时,
 * 每秒汇总打印一次平均/最大耗时。置 1 打开, 调试完置回 0。 */
#define LV_JLGPU_DISP_PROFILE    0

/* LIST==2 推屏时序开关(退路)
 *   0: 走"推屏独占门"(jlgpu_push_gate_*,见 lv_draw_jlgpu.c),保留双链路流水:
 *      渲染线程可以继续构建下一帧链表,只在"立即绘制"等真正碰 GPU 的操作上与推屏互斥
 *   1: 粗粒度安全网 —— 生产者 post 后等消费者推完屏(lcd->draw 返回)才 flush_ready,
 *      链表内存也延后到 lcd->draw 之后释放。能彻底串行化,但失去帧间流水
 * 背景:开了 gpu_link_dbi 推屏联动后,推屏期间被别的线程改 TOP/BUF SFR 或抢 GPU
 * 会让联动断流、DBI 永久等待 → 整机冻死。0 是治本做法,1 是把并发彻底关掉。 */
#define JLGPU_PUSH_SYNC_WITH_LVGL   0

#if (LV_COLOR_DEPTH == 16)
#define LV_JLGPU_OUT_FORMAT JLGPU_FORMAT_RGB565
#elif (LV_COLOR_DEPTH == 24)
#define LV_JLGPU_OUT_FORMAT JLGPU_FORMAT_RGB888
#elif (LV_COLOR_DEPTH == 32)
#define LV_JLGPU_OUT_FORMAT JLGPU_FORMAT_ARGB8888
#endif
#define LV_JLGPU_GXDMA_OUT_FORMAT LV_JLGPU_OUT_FORMAT

#define BYTE_PER_PIXEL (jlgpu_get_format_bpp(LV_JLGPU_OUT_FORMAT) / 8)

/**********************
 *      TYPEDEFS
 **********************/
typedef struct _lv_disp_lcd_t {
    struct lcd_interface *lcd;
    uint8_t rotate;
    uint8_t jstc_en;
    uint8_t need_gxdma;
    uint8_t gpu_link_dbi;
    /* 走 TOP->GXDMA 且没有 JSTC/GPU-DBI 联动: 可以按脏区收窄 GPU 输出窗口, 只重画/只搬脏区。
     * 非 PSRAM 工程 need_gxdma 为 0(GPU 直写 SRAM 更快), 这里也是 0, 走普通 DIRECT 直写。 */
    uint8_t partial_render;
    /* 上一帧的真实脏区(已对齐)。双 buffer 交替, 本帧写的那块上次写是两帧前, 缺的是
     * "上帧脏区 ∪ 本帧脏区", 所以窗口取这个并集, 写一次就够, 不用再往另一块写一遍
     * (那要多跑一次完整 GPU run)。记的必须是"真实脏区"而不是"窗口": 窗口本身已经含
     * 上帧脏区了, 记窗口会让并集一路滚大。 */
    lv_area_t prev_dirty;
    uint8_t prev_dirty_valid;
    uint16_t jstc_pingping_buf_w;
    uint16_t jstc_pingping_buf_h;
    uint16_t buf_stride;
    uint32_t buffer_size;
    uint8_t *jstc_pingping_buf0;
    uint8_t *jstc_pingping_buf1;

    lv_area_t win_area;
    /* 本帧 GPU 输出窗口(已 8 对齐)与它在物理 tile 缓存里的字节偏移。
     * 推屏在消费线程里做, 那里拿不到 LVGL 传下来的脏区, 所以这里存一份;
     * 一帧只允许在飞一个, 由 jlgpu_run_disp_sem 保证不会被覆盖。 */
    lv_area_t render_area;
    uint32_t render_ofs;
    uint8_t *lcd_buf0;
    uint8_t *lcd_buf1;
    uint16_t lcd_width; //lcd 原始宽
    uint16_t lcd_height; //lcd 原始高
    uint16_t lcd_align_width; //lcd 对齐宽
    uint16_t lcd_align_height; //lcd 对齐高
} lv_disp_lcd;


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

static int __lv_gpulist_flush_framerate_calc_ex(u32 gpu_us, u32 push_us, bool is_effect);
static int __lv_gpulist_flush_framerate_calc(u32 gpu_us, u32 push_us);

#if LV_USE_DRAW_JLGPU_LIST
static void lv_jlgpu_link_dbi_draw_full_buf(lv_display_t *disp_drv, lv_draw_jlgpu_unit_t *unit, uint8_t *px_map, const lv_draw_buf_t *src, const lv_area_t *src_area);
#endif

/* 方案A：首帧完成 flush 后再开背光，避免花屏和闪烁，等画面已写入 GRAM 再开 */
static void lvgl_try_open_backlight_after_first_frame(u8 is_last_flush);
/**********************
 *  STATIC VARIABLES
 **********************/
static lv_disp_lcd lv_jl_lcd = {0};
/* 0: 尚未打开背光，custom 初始化亮度未应用 1: 已经 VM 或默认亮度应用 */
static u8 s_lvgl_first_frame_bl_opened = 0;

/**
 * @brief 查询首帧背光是否已打开（custom 侧 apply/ajust 使用）
 * @return 0 未打开背光，非 0 已打开
 */
u8 lvgl_is_first_frame_backlight_opened(void)
{
    return s_lvgl_first_frame_bl_opened;
}

/*
 * 默认实现供 dashboard / kid camera / glass 以及 watch 共用
 * LVGL watch 由 custom/ui_action_flashlight.c 重写，方案用 VM 控制亮度
 */
_WEAK_ void lvgl_ui_apply_brightness_setting(void)
{
    extern int lcd_drv_backlight_ctrl(u8 percent);

    if (lv_jl_lcd.lcd && lv_jl_lcd.lcd->backlight_ctrl) {
        lv_jl_lcd.lcd->backlight_ctrl(100);
    } else {
        lcd_drv_backlight_ctrl(100);
    }
}

#if LV_UI_TRIPLE_BUFFER_EN
static OS_SEM triple_buf_sem;
#endif

#if LV_USE_DRAW_JLGPU_LIST == 2
#define JLGPU_RUN_DISP_CB_NAME  "jlgpu_run_disp_task"
static void _jlgpu_run_disp_cb(void *ptr);
static OS_SEM jlgpu_run_disp_sem;      /* 消费者 -> 生产者:上一帧已处理完,token 归还 */
#if JLGPU_PUSH_SYNC_WITH_LVGL
static OS_SEM jlgpu_push_done_sem;     /* 消费者 -> 生产者:推屏(lcd->draw)已完成 */
#endif
#endif
/**********************
 *      MACROS
 **********************/
/* 足球特效钩子：watch 等 app 侧 ui_action_football.c 提供强实现覆盖。
 * 默认返回 false，表示无足球特效（其它 app 共用本文件时不影响）。 */
_WEAK_ bool lv_football_menu_active(void)
{
    return false;
}

_WEAK_ bool lv_football_menu_flush(lv_draw_jlgpu_unit_t *unit, uint8_t *out_buf, lv_area_t *screen_area)
{
    (void)unit;
    (void)out_buf;
    (void)screen_area;
    return false;
}
/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_disp_draw_lcd(const lv_area_t *area, uint8_t *px_map)
{
    lv_jl_lcd.lcd->set_draw_area(area->x1, area->x2, area->y1, area->y2);
    lv_jl_lcd.lcd->draw((uint8_t *)px_map, area->x1, area->x2, area->y1, area->y2);
}

#if LV_USE_DRAW_JLGPU
u8 lv_jl_lcd_get_need_gxdma(void)
{
    return lv_jl_lcd.need_gxdma;
}

u8 lv_jl_lcd_get_need_gpu_link_dbi(void)
{
    return lv_jl_lcd.gpu_link_dbi;
}

u8 lv_jl_lcd_get_partial_render(void)
{
    return lv_jl_lcd.partial_render;
}

/* 把上一帧的真实脏区并进本帧的 bbox(理由见 lv_disp_lcd.prev_dirty 的说明),
 * 同时记下本帧的真实脏区供下一帧用。lv_refr.c 合并完本帧脏区后调它, 再把结果对齐。
 * 两边(这里记的、和 lv_refr.c 交给 LVGL 渲染的)必须是同一个矩形, 否则 TOP 窗口里
 * 会出现没渲染到的区域。 */
void lv_jl_lcd_join_prev_dirty(lv_area_t *dirty)
{
    lv_area_t cur = *dirty;

    lv_jl_lcd_align_render_area(&cur);
    if (lv_jl_lcd.prev_dirty_valid) {
        lv_area_join(dirty, dirty, &lv_jl_lcd.prev_dirty);
    }
    lv_jl_lcd.prev_dirty = cur;
    lv_jl_lcd.prev_dirty_valid = 1;
}
#endif

static u8 lv_jl_lcd_need_gxdma(struct dbi_hw_param *dbi_hw, u8 rotate, u16 format)
{
    // return 1;
    if (dbi_hw && dbi_hw->jstc_en) {
        return 1;
    } else if (rotate) {
        return 1;
    }
    return 0;
}

static void lcd_debug_frame_rate(void *p)
{
    u32 average_cnt = (u32)((u32)p);
    printf("\n~~~~~~~~~~~~~~lcd frame rate~~~~~~~~~~~~~~~~~\n");
    printf("average_one_frame_time:%dus, fps=%d.%dHz\n", average_cnt, 1000 * 1000 / average_cnt, (1000 * 1000) * 100 / average_cnt % 100);

    if ((1000 * 1000 / average_cnt) > 63) { //留一点余量
        // ASSERT(0, "lcd frame rate is too high!! It should be less than or equal to 60Hz");
        printf("lcd frame rate is too high!! It should be less than or equal to 60Hz\n");
    }
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
}
static void lcd_frame_end_callback(void *user_data)
{
#define CALC_FRAMES  (60)
    static int last_cnt = 0;
    static u32 calc_cnt = CALC_FRAMES;
    static u32 sum_cnt = 0;
    u32 curr_cnt = 0;
    u32 average_cnt = 0;

    if (calc_cnt > 0) {
        int64_t get_system_us(void);
        curr_cnt = get_system_us();
        if (last_cnt) {
            calc_cnt--;
            sum_cnt += (curr_cnt - last_cnt);
            if (calc_cnt == 0) {
                average_cnt = sum_cnt / CALC_FRAMES;
                if (average_cnt) {
                    sys_timeout_add((void *)average_cnt, lcd_debug_frame_rate, 1);
                }
            }
        }
        last_cnt = curr_cnt;
    }

#if LV_UI_TRIPLE_BUFFER_EN
    os_sem_post(&triple_buf_sem);
#endif

}

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    extern struct lcd_platform_data *lcd_get_platform_data();
    struct lcd_platform_data *lcd_pd = lcd_get_platform_data();
    struct lcd_info lcdinfo = {0};
    u16 lcd_w = 0;
    u16 lcd_h = 0;
    lv_jl_lcd.jstc_en = lcd_pd->dbi_hw.jstc_en;
    u8 rotation = 0;
    ASSERT(lv_jl_lcd.lcd->get_screen_info)
    if (lv_jl_lcd.lcd->get_screen_info) {
        lv_jl_lcd.lcd->get_screen_info(&lcdinfo);
        lcd_w = lcdinfo.width;
        lcd_h = lcdinfo.height;
        lv_jl_lcd.jstc_en = lcd_pd->dbi_hw.jstc_en;
        lv_jl_lcd.rotate = lcdinfo.rotate;
        printf("lcdinfo rotate=%d\n", lv_jl_lcd.rotate);
        rotation = is_gxdma_rotate(lv_jl_lcd.rotate);
    }
#if !LV_USE_DRAW_JLGPU
    // run cpu
    if (lv_jl_lcd.jstc_en) {
        ASSERT(0, "lvgl jstc_en is not supported in cpu mode");
    }
    if (lv_jl_lcd.rotate) {
        ASSERT(0, "lvgl rotate is not supported in cpu mode");
    }
#endif

    /* PSRAM 工程一律走 GXDMA: GPU 直写 PSRAM 只能走非缓存别名, 带宽明显低于
     * "GPU 输出经 TOP ping-pong 缓冲、由 GXDMA 突发写回"这条通路, 整帧差一倍以上。
     * 帧缓存在内部 SRAM 时两者没有这个差距, 直写还少一次搬运, 所以只在需要时开。
     * 需要的情形: 旋转/镜像(GXDMA 输出必须是 tile)或 JSTC 压缩。 */
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
    lv_jl_lcd.need_gxdma = 1;
#else
    lv_jl_lcd.need_gxdma = lv_jl_lcd_need_gxdma(&lcd_pd->dbi_hw, lv_jl_lcd.rotate, LV_JLGPU_GXDMA_OUT_FORMAT);
#endif
    lv_jl_lcd.gpu_link_dbi = (lcd_pd->dbi_hw.eva_link_mode == LINKAGE_MODE_GPU) ? 1 : 0;

    if (lv_jl_lcd.gpu_link_dbi && lv_jl_lcd.need_gxdma) {
        printf("warn : lv_jl_lcd.need_gxdma == gpu_link_dbi == 1, the GPU can connect with one type of hardware module\n");
        if (lv_jl_lcd_need_gxdma(&lcd_pd->dbi_hw, lv_jl_lcd.rotate, LV_JLGPU_GXDMA_OUT_FORMAT)) {
            printf("cancel gpu_link_dbi\n");
            lv_jl_lcd.gpu_link_dbi = 0;
        } else {
            printf("cancel need_gxdma\n");
            lv_jl_lcd.need_gxdma = 0;
        }
    }

    /* 走 TOP->GXDMA 且没有 JSTC / GPU-DBI 联动: 可以按脏区收窄 GPU 输出窗口, 只重画/只搬脏区。
     * 90_MIRROR / 90_FLIP 的合成顺序没有明确定义, 这两个模式退回全屏渲染, 免得落点算错。
     *
     * 想整体退回全屏渲染(FULL)时, 把下面这行直接改成:
     *     lv_jl_lcd.partial_render = 0;
     * 置 0 之后渲染模式会自动回到 FULL(见下面 force_full 的判据), 同时脏区 bbox 合并、
     * 跳过 LVGL 自带脏区同步、窗口收窄与 tile 落点偏移全部失效, 即完全回到收窄窗口之前
     * 那条通路, 没有任何多余开销。
     * 注意不要只改下面 lv_display_set_buffers() 传的 mode: 它内部就是调
     * lv_display_set_render_mode(), 随后会被 force_full 那段覆盖掉, 改了不生效。 */
    lv_jl_lcd.partial_render = (lv_jl_lcd.need_gxdma && !lv_jl_lcd.jstc_en && !lv_jl_lcd.gpu_link_dbi &&
                                lv_jl_lcd.rotate <= LCD_ROTATE_270) ? 1 : 0;

    printf("lv_jl_lcd.need_gxdma=%d gpu_link_dbi=%d partial_render=%d\n",
           lv_jl_lcd.need_gxdma, lv_jl_lcd.gpu_link_dbi, lv_jl_lcd.partial_render);
    if (rotation) {
        lcd_w = lcdinfo.height;
        lcd_h = lcdinfo.width;
    }

    lv_jl_lcd.lcd_width = lcd_w;
    lv_jl_lcd.lcd_height = lcd_h;

    lv_jl_lcd.lcd_align_width = lcd_w;
    lv_jl_lcd.lcd_align_height = lcd_h;

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t *disp = lv_display_create(lcd_w, lcd_h);

    lv_display_set_flush_cb(disp, disp_flush);

#if 0
    /* Example 1
     * One buffer for partial rendering*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_1_1[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];            /*A buffer for 10 rows*/
    lv_display_set_buffers(disp, buf_1_1, NULL, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

#if !LV_USE_FRAME_BUF
#define BLOCK_BUF_HEIGHT LV_USE_BLOCK_ROW
    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_1[MY_DISP_HOR_RES * BLOCK_BUF_HEIGHT * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_2[MY_DISP_HOR_RES * BLOCK_BUF_HEIGHT * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

#if LV_USE_FRAME_BUF
    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    LV_ATTRIBUTE_MEM_ALIGN

    /*static uint8_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];*/

    uint32_t lv_disp_buffer_size = lcdinfo.height * lcdinfo.width * BYTE_PER_PIXEL;

#if LV_USE_DRAW_JLGPU_LIST
    if (lv_jl_lcd.need_gxdma || lv_jl_lcd.gpu_link_dbi) {
        if (lv_jl_lcd.rotate || lv_jl_lcd.jstc_en) {
            // 旋转或jstc_en时, tile mode 格式需要8对齐
            lv_jl_lcd.lcd_align_width = LV_ALIGN_UP(lcd_w, 8);
            lv_jl_lcd.lcd_align_height = LV_ALIGN_UP(lcd_h, 8);
        }
        // lv_jl_lcd.jstc_pingping_buf_w = lv_jl_lcd.lcd_align_width;
        lv_jl_lcd.jstc_pingping_buf_w = (lv_jl_lcd.lcd_align_width) > 512 ? 512 : lv_jl_lcd.lcd_align_width;
        lv_jl_lcd.jstc_pingping_buf_h = 32;
        lv_jl_lcd.buf_stride = lv_jl_lcd.jstc_pingping_buf_w * BYTE_PER_PIXEL;
        u32 buf_size = lv_jl_lcd.jstc_pingping_buf_h * lv_jl_lcd.buf_stride;
        lv_jl_lcd.jstc_pingping_buf0 = lv_malloc(buf_size);
        ASSERT(lv_jl_lcd.jstc_pingping_buf0);
        printf("lv_jl_lcd.jstc_pingping_buf0=%x\n", (uint32_t)lv_jl_lcd.jstc_pingping_buf0);
        lv_jl_lcd.jstc_pingping_buf1 = lv_malloc(buf_size);
        ASSERT(lv_jl_lcd.jstc_pingping_buf1);
        printf("lv_jl_lcd.jstc_pingping_buf1=%x\n", (uint32_t)lv_jl_lcd.jstc_pingping_buf1);

        if (lv_jl_lcd.gpu_link_dbi) {
            lv_disp_buffer_size = 1; //gpu 联动 dbi, 无需lvgl显存,这里申请1byte为不让lvgl内部报错
        } else if (lv_jl_lcd.jstc_en) {
            lv_disp_buffer_size = lv_jl_lcd.lcd_align_height / 8 * (lv_jl_lcd.lcd_align_width / 8 * 64); //RGB888
        } else {

            lv_disp_buffer_size = lv_jl_lcd.lcd_align_height * lv_jl_lcd.lcd_align_width * lv_jlgpu_get_gxdma_bpp(LV_JLGPU_GXDMA_OUT_FORMAT) / 8;
        }
    }
#endif
    lv_jl_lcd.buffer_size = lv_disp_buffer_size;
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
    uint8_t *buf_3_1 = malloc_psram(lv_disp_buffer_size);
    uint8_t *buf_3_2 = malloc_psram(lv_disp_buffer_size);

    if (!buf_3_1 || !buf_3_2) {
        // 分配失败处理
        LV_LOG_ERROR("malloc Failed\n");
        return;
    }

    if (buf_3_1) {
        memset(buf_3_1, 0, lv_disp_buffer_size);
    }
    if (buf_3_2) {
        memset(buf_3_2, 0, lv_disp_buffer_size);
    }
#else
    uint8_t *buf_3_1 = lv_malloc(lv_disp_buffer_size);
    uint8_t *buf_3_2 = lv_malloc(lv_disp_buffer_size);
#endif



    if (buf_3_1 == NULL) {
        LV_LOG_ERROR("malloc Failed\n");
        return;
    }

    if (lv_jl_lcd.lcd) {
        lv_area_t full_area = {0};
        full_area.x1 = 0;
        full_area.y1 = 0;
        full_area.x2 = lcdinfo.width - 1;
        full_area.y2 = lcdinfo.height - 1;
        uint8_t *px_map = buf_3_2 ? buf_3_2 : buf_3_1;
        if (lv_jl_lcd.jstc_en) {
            //开启jstc时,设为全0刷屏不是黑色，下面固定色值才会刷黑色
            uint8_t *byte_ptr = (uint8_t *)px_map;
            uint32_t total_bytes = lv_disp_buffer_size;
            static const uint8_t pattern[16] = {
                0x00, 0x00, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
                0x00, 0x81, 0x02, 0x81, 0xAA, 0xAA, 0xAA, 0xAA
            };

            uint32_t full_blocks = total_bytes / 16;
            uint32_t remainder = total_bytes % 16;

            for (uint32_t i = 0; i < full_blocks; i++) {
                memcpy(byte_ptr + i * 16, pattern, 16);
            }
            if (remainder) {
                memcpy(byte_ptr + full_blocks * 16, pattern, remainder);
            }
        } else {
            lv_memset(px_map, 0, lv_disp_buffer_size);
        }
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
        DcuFlushRegion((uint32_t *)px_map, lv_disp_buffer_size);
#endif
        lv_port_disp_draw_lcd(&full_area, px_map);
    }

    /* 开启背光 */
#if  ((TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH) || (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_KID_CAMERA))
    /*
     * 方案A：LVGL watch延迟开背光
     * 首帧不走 GPU link 等硬件模块以确保数据已正确写入 GRAM
     * 通过检测 disp_flush 的 flushing_last 标志来触发
     */
#else
    /*
     * 非 LVGL watch和kid camera（dashboard / glass 等）
     * 在初始化完成后立即打开背光
     */
    if (lv_jl_lcd.lcd->backlight_ctrl) {
        lv_jl_lcd.lcd->backlight_ctrl(100);
    } else {
        extern int lcd_drv_backlight_ctrl(u8 percent);
        lcd_drv_backlight_ctrl(100);
    }
    s_lvgl_first_frame_bl_opened = 1;
#endif


    LV_LOG("\n%s() %d, %d, stride:%d format: %d,rotate:%d jstc:%d\n", __func__, lcd_w, lcd_h, lcdinfo.stride, lcdinfo.color_format, lcdinfo.rotate, lv_jl_lcd.jstc_en);
    if (lv_jl_lcd.lcd_align_width != lcd_w) {
        LV_LOG("lcd_align_width = %d\n", lv_jl_lcd.lcd_align_width);
    }
    if (lv_jl_lcd.lcd_align_height != lcd_h) {
        LV_LOG("lcd_align_height = %d\n", lv_jl_lcd.lcd_align_height);
    }

    /* LV_ATTRIBUTE_MEM_ALIGN */
    /* static uint8_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL]; */
    //lv_display_set_buffers(disp, buf_3_1, /* buf_3_2 */NULL, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);
    //lv_display_set_buffers(disp, buf_3_1, /* buf_3_2 */NULL, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_FULL);
    /* 两个分支都传 DIRECT: lv_display_set_buffers() 内部就是调 lv_display_set_render_mode(),
     * 最终模式由下面的判定决定, 这里传的 mode 会被后面那次覆盖。 */
    lv_display_set_buffers(disp, buf_3_1, buf_3_2, (lcd_w * lcd_h * BYTE_PER_PIXEL), LV_DISPLAY_RENDER_MODE_DIRECT);

    /* 只有 GPU-DBI 联动、JSTC、以及收窄不了窗口的 GXDMA 联动(90_MIRROR/90_FLIP)才走全屏;
     * 其余(GPU 直写、能收窄窗口的 TOP->GXDMA)走 DIRECT 脏区渲染。 */
    bool force_full = (lv_jl_lcd.gpu_link_dbi || lv_jl_lcd.jstc_en ||
                       (lv_jl_lcd.need_gxdma && !lv_jl_lcd.partial_render)) ? true : false;
    if (force_full) {
        /* 1. gpu dbi 联动时, 只有 2 块联动 buffer, 无法作脏区同步, 只能全屏渲染
         * 2. jstc_en 时, 脏区同步会导致显存画质多次压缩损失, 只能全屏渲染 */
        lv_display_set_render_mode(disp, LV_DISPLAY_RENDER_MODE_FULL);
    }
    printf("lvgl render_mode=%s (need_gxdma=%d gpu_link_dbi=%d rotate=%d jstc=%d)\n",
           (disp->render_mode == LV_DISPLAY_RENDER_MODE_FULL) ? "FULL" : "DIRECT",
           lv_jl_lcd.need_gxdma, lv_jl_lcd.gpu_link_dbi, lv_jl_lcd.rotate, lv_jl_lcd.jstc_en);

    dmm_callback_t dmm_cb = {
        .frame_end_cb = lcd_frame_end_callback,
    };
    JLDMM_register_callback(&dmm_cb);

#if LV_UI_TRIPLE_BUFFER_EN
    static lv_draw_buf_t draw_buf3 = {0};
    uint8_t *buf_3_3 = malloc_psram(lv_disp_buffer_size);
    ASSERT(buf_3_3);
    // if (buf_3_3 == NULL) {
    //     LV_LOG_ERROR("buf_3_3 malloc Failed\n");
    //     return;
    // }
    lv_memset(buf_3_3, 0, lv_disp_buffer_size);
    lv_color_format_t cf = lv_display_get_color_format(disp);
    lv_draw_buf_init(&draw_buf3, lcd_w, lcd_h, cf, 0, buf_3_3, lcd_w * lcd_h * BYTE_PER_PIXEL);

    lv_display_set_3rd_draw_buffer(disp, &draw_buf3);
    os_sem_create(&triple_buf_sem, 0);
#endif

#endif

#if LV_USE_DRAW_JLGPU_LIST == 2
    os_sem_create(&jlgpu_run_disp_sem, 1);
#if JLGPU_PUSH_SYNC_WITH_LVGL
    os_sem_create(&jlgpu_push_done_sem, 0);
#endif
    /* 该线程会跑到 GPU 框架里(jlgpu_run 的路径/矩阵浮点运算、lv_jl_lcd.lcd->draw)，
     * 运算量随 UI 绘制任务数量增长。原 1024 字节栈过小，栈溢出是"无打印直接黑屏"的典型原因
     * (LIST==1 没有这个线程)，这里放大到 4K，与同为渲染通路的 fb_combine 线程一致 */
    thread_fork(JLGPU_RUN_DISP_CB_NAME, 25, 4096, 128, NULL, _jlgpu_run_disp_cb, NULL);

#endif

}

/**********************
 *   STATIC FUNCTIONS
 **********************/
extern uint32_t get_system_ms(void);
#if LV_JLGPU_DISP_PROFILE
extern int64_t get_system_us(void);
#endif

/* 帧率统计: 每秒打印一次 fps。
 * LV_JLGPU_DISP_PROFILE 打开时, 同时统计 gpu_run(GPU 渲染合成) 与 push(推屏/LCD 传输)
 * 耗时并每秒追加打印平均/最大耗时。gpu_us = jlgpu_task_list_disp_run + task_list_free;
 * push_us = 推屏链路(set_draw_area + draw)及其前准备(wait_or_recover 等)的耗时。
 *
 * 页面特效期间走的是 disp_flush 里另一条分支(lv_page_effect_manager 接管整帧, 那边
 * 自己合成自己推屏后直接 return), 不经过下面 t0/t1 这条路, 所以那条分支会把它的
 * gpu/push 也调到这里来, 并把 is_effect 置 true 单独累计一行(effect: ...):
 * 左右滑那套特效是全屏 3D 合成 + 多面体贴图, 是本功能最重的路径, 混在总量里看不清。
 * 注意: 特效帧现在也算进总数了(以前完全不计), 所以 effect 行与总行的差值就是特效自己的代价。 */
static int __lv_gpulist_flush_framerate_calc_ex(u32 gpu_us, u32 push_us, bool is_effect)
{
    static u32 time = 0;
    static u16 frames = 0;
    u32 cur_time = 0;

#if LV_JLGPU_DISP_PROFILE
    static u32 gpu_sum = 0, gpu_max = 0;
    static u32 push_sum = 0, push_max = 0;
    static u32 fx_frames = 0, fx_gpu_sum = 0, fx_gpu_max = 0, fx_push_sum = 0, fx_push_max = 0;

    gpu_sum += gpu_us;
    push_sum += push_us;
    if (gpu_us > gpu_max) {
        gpu_max = gpu_us;
    }
    if (push_us > push_max) {
        push_max = push_us;
    }

    if (is_effect) {
        fx_frames++;
        fx_gpu_sum += gpu_us;
        fx_push_sum += push_us;
        if (gpu_us > fx_gpu_max) {
            fx_gpu_max = gpu_us;
        }
        if (push_us > fx_push_max) {
            fx_push_max = push_us;
        }
    }
#else
    (void)is_effect;
#endif

    frames++;
    cur_time = get_system_ms();
    if (time_after(cur_time, time)) {
        printf("gpulist flush: %dfps\n", frames * 1000 / (1000 + (cur_time - time)));
#if LV_JLGPU_DISP_PROFILE
        printf("[jlgpu profile] gpu_avg=%u.%u ms  gpu_max=%u.%u ms  push_avg=%u.%u ms  push_max=%u.%u ms\n",
               gpu_sum / frames / 1000, gpu_sum / frames % 1000 / 100,
               gpu_max / 1000, gpu_max % 1000 / 100,
               push_sum / frames / 1000, push_sum / frames % 1000 / 100,
               push_max / 1000, push_max % 1000 / 100);
        /* 只有这一秒里跑过特效才打, 所以平时不会有这行噪音。
         * "N/M frames" = 这一秒里特效帧数 / 总帧数。 */
        if (fx_frames) {
            printf("[jlgpu profile] effect: %u/%u frames  gpu_avg=%u.%u ms  gpu_max=%u.%u ms  "
                   "push_avg=%u.%u ms  push_max=%u.%u ms\n",
                   fx_frames, frames,
                   fx_gpu_sum / fx_frames / 1000, fx_gpu_sum / fx_frames % 1000 / 100,
                   fx_gpu_max / 1000, fx_gpu_max % 1000 / 100,
                   fx_push_sum / fx_frames / 1000, fx_push_sum / fx_frames % 1000 / 100,
                   fx_push_max / 1000, fx_push_max % 1000 / 100);
        }
        gpu_sum = gpu_max = 0;
        push_sum = push_max = 0;
        fx_frames = fx_gpu_sum = fx_gpu_max = fx_push_sum = fx_push_max = 0;
#endif
        time = get_system_ms() + 1000;
        frames = 0;
    }
    return 0;
}

/* 普通帧(非特效)的入口。特效帧见 __lv_gpulist_flush_framerate_calc_ex()。 */
static int __lv_gpulist_flush_framerate_calc(u32 gpu_us, u32 push_us)
{
    return __lv_gpulist_flush_framerate_calc_ex(gpu_us, push_us, false);
}


static void lv_lcd_limit_align(lv_area_t *area, lv_coord_t hor_res, lv_coord_t ver_res)
{
    area->x1 = (area->x1 >= hor_res) ? (hor_res - 1) : area->x1;
    area->x2 = (area->x2 >= hor_res) ? (hor_res - 1) : area->x2;
    area->y1 = (area->y1 >= ver_res) ? (ver_res - 1) : area->y1;
    area->y2 = (area->y2 >= ver_res) ? (ver_res - 1) : area->y2;
}

static int lcd_flush_finish_callback(void *priv)
{
    return 0;
}

/**
 * @brief 首帧 flush 完成后打开背光（方案A：LVGL watch 专用）
 * @param is_last_flush 来自 disp_drv->flushing_last，表示最后一帧
 *
 * 关键逻辑：
 * 1) 已开背光直接返回
 * 2) LVGL 的 flushing_last 为 true 才继续
 * 3) 调用统一接口供 watch/flashlight 重写
 *
 * 背景：
 * - custom_init 和 apply_brightness 可能/VM 还未初始化 flashlight
 * - 统一调用 lvgl_ui_apply_brightness_setting() 和 VM 亮度设置
 * 确保背光在首帧已写入 GRAM 之后再开
 *
 * 在 watch 的init阶段仅初始化屏幕但不立即开背光
 * apply 阶段才由 watch custom 开 100%，watch 方案用 VM 设置
 *
 * 注意GPU link 模式下要等待 lcd_wait_busy() 防止 timeout 导致 lvgl死锁
 */
static void lvgl_try_open_backlight_after_first_frame(u8 is_last_flush)
{
    if (s_lvgl_first_frame_bl_opened) {
        return;
    }
    if (!is_last_flush) {
        return;
    }

    /* 统一接口 apply/watch 方案差异化 VM 与 lcd_drv_backlight_ctrl */
    s_lvgl_first_frame_bl_opened = 1;
    lvgl_ui_apply_brightness_setting();
    printf("[LVGL_BL] first frame flush done, apply brightness\n");
}


/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    printf("lvgl disp init...\n");


#if 1

    lv_jl_lcd.lcd = lcd_get_hdl();
    ASSERT(lv_jl_lcd.lcd);

    if (lv_jl_lcd.lcd->init) {
        extern const struct ui_devices_cfg ui_cfg_data;
        lv_jl_lcd.lcd->init((void *)&ui_cfg_data);
    }

    lcd_draw_set_callback(lcd_flush_finish_callback);

#endif

}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

#if LV_USE_DRAW_JLGPU_LIST
static int jlgpu_task_list_disp_run(lv_display_t *disp_drv, lv_draw_jlgpu_unit_t *unit, const lv_area_t *area, uint8_t *px_map)
{
    lv_area_t _area = {0};
    uint32_t area_width = lv_area_get_width(area);

    /* 逻辑整帧范围: 输出步长按它算而不是按 area(可能只是整帧里的一块)算 */
    lv_area_t frame_area = {0};
    frame_area.x1 = 0;
    frame_area.y1 = 0;
    frame_area.x2 = lv_jl_lcd.lcd_align_width - 1;
    frame_area.y2 = lv_jl_lcd.lcd_align_height - 1;

    lv_jlgpu_mutex_pend();

    if (disp_drv->render_mode == LV_DISPLAY_RENDER_MODE_FULL) {
        if (lv_jl_lcd.need_gxdma || lv_jl_lcd.gpu_link_dbi) {
            //显存压缩需要联动GXDMA
            /* printf("gxdma --> %d,%d,%d,%d ", area->x1, area->x2, area->y1, area->y2); */
            lv_jlgpu_sfr_top_set(unit, (lv_area_t *)area, lv_jl_lcd.jstc_pingping_buf_w, lv_jl_lcd.jstc_pingping_buf_h, lv_jl_lcd.jstc_pingping_buf0, lv_jl_lcd.jstc_pingping_buf1);
            _area.x1 = 0;
            _area.y1 = 0;
            /* 这个窗口只是 GPU 直写那一行(真正输出走 TOP->GXDMA), 宽度收窄到旋转输出区的宽度
             * (= 源矩形的高度), 否则窗口收窄后这行会写到输出区外面、把别的 tile 写坏。
             * 收窄后它正好落在输出区第一个 tile 行内, 会被 GXDMA 覆盖掉。 */
            _area.x2 = (is_gxdma_rotate(lv_jl_lcd.rotate) ? lv_area_get_height(area) : lv_area_get_width(area)) - 1;
            _area.y2 = 0;

            /* 第三个参数是行步长, 必须是 ping-pong 缓存的宽度, 不能跟着窗口一起收窄 */
            lv_jlgpu_sfr_set(unit, (lv_area_t *)&_area, lv_jl_lcd.jstc_pingping_buf_w, px_map, LV_JLGPU_OUT_FORMAT);

            if (lv_jl_lcd.need_gxdma) {
                lv_jlgpu_gxdma_sfr_set(unit, (lv_area_t *)area, lv_jl_lcd.buf_stride, px_map, LV_JLGPU_OUT_FORMAT, LV_JLGPU_GXDMA_OUT_FORMAT, lv_jl_lcd.jstc_en, lv_jl_lcd.rotate, &frame_area);
            }

        } else {
            lv_jlgpu_sfr_set(unit, (lv_area_t *)area, area_width, px_map, LV_JLGPU_OUT_FORMAT);
        }
    } else if (disp_drv->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
        if (lv_jl_lcd.need_gxdma || lv_jl_lcd.gpu_link_dbi) {
            //显存压缩需要联动GXDMA
            //TODO:
            /* printf("gxdma direct --> %d,%d,%d,%d ", area->x1, area->x2, area->y1, area->y2); */
            lv_jlgpu_sfr_top_set(unit, (lv_area_t *)area, lv_jl_lcd.jstc_pingping_buf_w, lv_jl_lcd.jstc_pingping_buf_h, lv_jl_lcd.jstc_pingping_buf0, lv_jl_lcd.jstc_pingping_buf1);
            _area.x1 = 0;
            _area.y1 = 0;
            /* 这个窗口只是 GPU 直写那一行(真正输出走 TOP->GXDMA), 宽度收窄到旋转输出区的宽度
             * (= 源矩形的高度), 否则窗口收窄后这行会写到输出区外面、把别的 tile 写坏。
             * 收窄后它正好落在输出区第一个 tile 行内, 会被 GXDMA 覆盖掉。 */
            _area.x2 = (is_gxdma_rotate(lv_jl_lcd.rotate) ? lv_area_get_height(area) : lv_area_get_width(area)) - 1;
            _area.y2 = 0;
            /* 第三个参数是行步长, 必须是 ping-pong 缓存的宽度, 不能跟着窗口一起收窄 */
            lv_jlgpu_sfr_set(unit, (lv_area_t *)&_area, lv_jl_lcd.jstc_pingping_buf_w, px_map, LV_JLGPU_OUT_FORMAT);

            if (lv_jl_lcd.need_gxdma) {
                lv_jlgpu_gxdma_sfr_set(unit, (lv_area_t *)area, lv_jl_lcd.buf_stride, px_map, LV_JLGPU_OUT_FORMAT, LV_JLGPU_GXDMA_OUT_FORMAT, lv_jl_lcd.jstc_en, lv_jl_lcd.rotate, &frame_area);
            }
        } else {
            lv_jlgpu_sfr_set(unit, (lv_area_t *)area, area_width, px_map, LV_JLGPU_OUT_FORMAT);
        }
    } else {

        lv_jlgpu_sfr_set(unit, (lv_area_t *)area, area_width, px_map, LV_JLGPU_OUT_FORMAT);
    }

    //统计帧率
#if 0
    static u32 time_lapse_hdl;
    static u8 fps_cnt;
    ++fps_cnt;
    u32 tdiff = time_after(get_system_ms(), time_lapse_hdl);
    if (tdiff) {
        printf("lv render pf %dfps\n", fps_cnt);
        fps_cnt = 0;
        time_lapse_hdl = get_system_ms() + 1000;
    }
#endif

    lv_jlgpu_task_list_run(unit);

    lv_jlgpu_mutex_post();

    return 0;

}

/* 逻辑坐标 -> 物理坐标。模式编号见 dbi.h 的 LCD_ROTATE_* 与 gxdma_driver.h 的
 * GXDMA_OUT_*(两边编号一致, 所以 lv_jl_lcd.rotate 可以直接当 rot_mode 用)。
 * w/h 是"源"那一侧的尺寸: 正向传逻辑帧, 反向传物理帧(90/270 两者互换)。
 * 只处理 0/镜像/翻转/90/180/270; 90_MIRROR / 90_FLIP 的合成顺序没有明确定义,
 * 那两个模式 partial_render 置 0, 退回全屏渲染。 */
static void lv_jlgpu_map_xy(u8 rot, lv_coord_t w, lv_coord_t h, lv_coord_t x, lv_coord_t y, lv_coord_t *px, lv_coord_t *py)
{
    switch (rot) {
    case LCD_ROTATE_90:
        *px = h - 1 - y;
        *py = x;
        break;
    case LCD_ROTATE_180:
        *px = w - 1 - x;
        *py = h - 1 - y;
        break;
    case LCD_ROTATE_270:
        *px = y;
        *py = w - 1 - x;
        break;
    case LCD_MIRROR:
        *px = w - 1 - x;
        *py = y;
        break;
    case LCD_FLIP:
        *px = x;
        *py = h - 1 - y;
        break;
    default:                    /* LCD_ROTATE_0 */
        *px = x;
        *py = y;
        break;
    }
}

/* 逻辑帧与物理帧的尺寸(都用对齐后的值), 90/270 交换宽高。
 * 必须用对齐后的尺寸: 映射里带着 (dim-1-x) 这种偏移, 取对齐前还是对齐后的值,
 * 同一个物理落点会差出对齐量。 */
static void lv_jlgpu_frame_size(lv_coord_t *lw, lv_coord_t *lh, lv_coord_t *pw, lv_coord_t *ph)
{
    *lw = lv_jl_lcd.lcd_align_width;
    *lh = lv_jl_lcd.lcd_align_height;
    if (is_gxdma_rotate(lv_jl_lcd.rotate)) {
        *pw = *lh;
        *ph = *lw;
    } else {
        *pw = *lw;
        *ph = *lh;
    }
}

/* 逆映射: 90 与 270 互换, 其余(0/镜像/翻转/180)的逆就是它自己 */
static u8 lv_jlgpu_inv_rotate(u8 rot)
{
    if (rot == LCD_ROTATE_90) {
        return LCD_ROTATE_270;
    }
    if (rot == LCD_ROTATE_270) {
        return LCD_ROTATE_90;
    }
    return rot;
}

/* 矩形按给定映射过去并归一化(两个对角映过去取 min/max) */
static void lv_jlgpu_area_map(u8 rot, lv_coord_t w, lv_coord_t h, const lv_area_t *src, lv_area_t *dst)
{
    lv_coord_t x1, y1, x2, y2;

    lv_jlgpu_map_xy(rot, w, h, src->x1, src->y1, &x1, &y1);
    lv_jlgpu_map_xy(rot, w, h, src->x2, src->y2, &x2, &y2);
    dst->x1 = LV_MIN(x1, x2);
    dst->y1 = LV_MIN(y1, y2);
    dst->x2 = LV_MAX(x1, x2);
    dst->y2 = LV_MAX(y1, y2);
}

/* 把脏区扩成"映射到物理后落点正好 tile 对齐"的逻辑矩形。
 * 旋转下 GXDMA 的 tile 输出要求落点和尺寸都是 8 的倍数, 而映射里带着 (dim-1-x)
 * 这种偏移, 直接在逻辑坐标上按 8 对齐并不等价 —— 90° 下要 y2+1 落在 8 的倍数上,
 * 180° 下却要 y2 自己落在 8 的倍数上。所以先映到物理空间对齐, 再映回来。
 * lv_refr.c 用它把脏区并成的 bbox 对齐后再交给 LVGL 渲染, 这样驱动用的窗口和
 * LVGL 实际渲染的区域完全一致, 不会留下没渲染到的边。
 * 注意夹紧用的是"对齐后的物理帧"而不是可见区: 落在可见区外的部分是缓存里的补齐行,
 * 不会显示; 若夹到可见区, 边界上的矩形尺寸就不是 8 的倍数了, 硬件按整 tile 写会出条纹。 */
void lv_jl_lcd_align_render_area(lv_area_t *area)
{
    lv_area_t phy;
    lv_area_t aligned;
    lv_coord_t lw, lh, pw, ph;

    if (!lv_jl_lcd.partial_render || lv_jl_lcd.rotate == 0) {
        /* 不收窄窗口, 或者目标就是逻辑帧本身(rotate=0 是行格式、字节粒度)时不用对齐 */
        return;
    }

    lv_jlgpu_frame_size(&lw, &lh, &pw, &ph);
    lv_jlgpu_area_map(lv_jl_lcd.rotate, lw, lh, area, &phy);

    aligned.x1 = phy.x1 & ~(lv_coord_t)0x7;
    aligned.y1 = phy.y1 & ~(lv_coord_t)0x7;
    aligned.x2 = ((phy.x2 + 8) & ~(lv_coord_t)0x7) - 1;
    aligned.y2 = ((phy.y2 + 8) & ~(lv_coord_t)0x7) - 1;
    if (aligned.x2 > pw - 1) {
        aligned.x2 = pw - 1;
    }
    if (aligned.y2 > ph - 1) {
        aligned.y2 = ph - 1;
    }

    lv_jlgpu_area_map(lv_jlgpu_inv_rotate(lv_jl_lcd.rotate), pw, ph, &aligned, area);
}

/* 脏区(已对齐)在物理缓存里的字节偏移。
 * 硬件是从 dst 基址开始写的, 不会自己按旋转后的坐标定位, 所以要自己算;
 * 整帧时结果恒为 0。
 * 旋转/镜像/翻转时目标是 tile 格式, 按 tile 行步长算; 无旋转时目标就是逻辑帧本身,
 * 是行格式, 按行步长算(行格式字节粒度, 不需要 8 对齐)。 */
static uint32_t lv_jlgpu_dirty_ofs(const lv_area_t *area)
{
    lv_area_t phy;
    lv_coord_t lw, lh, pw, ph;
    uint32_t tile_bytes;
    uint32_t tile_row_bytes;

    lv_jlgpu_frame_size(&lw, &lh, &pw, &ph);
    lv_jlgpu_area_map(lv_jl_lcd.rotate, lw, lh, area, &phy);

    if (lv_jl_lcd.rotate == 0) {
        return (uint32_t)phy.y1 * (uint32_t)lw * BYTE_PER_PIXEL + (uint32_t)phy.x1 * BYTE_PER_PIXEL;
    }

    tile_bytes = BYTE_PER_PIXEL * 64;                       /* 8x8 tile */
    tile_row_bytes = (uint32_t)(pw / 8) * tile_bytes;

    return (uint32_t)(phy.y1 / 8) * tile_row_bytes + (uint32_t)(phy.x1 / 8) * tile_bytes;
}

/* 定下本帧的 GPU 输出窗口与落点: 能收窄的场合收窄到脏区, 其余情况就是整帧(偏移恒为 0)。
 * 推屏在消费线程里做, 那边拿不到 LVGL 传下来的脏区, 所以结果存进 lv_jl_lcd;
 * 一帧只允许在飞一个, 由 jlgpu_run_disp_sem 保证不会被下一帧覆盖。 */
static void lv_jlgpu_prepare_render_area(const lv_area_t *full_area, const lv_area_t *dirty_area)
{
    lv_jl_lcd.render_area = *full_area;
    if (lv_jl_lcd.partial_render) {
        lv_jl_lcd.render_area = *dirty_area;
        lv_jl_lcd_align_render_area(&lv_jl_lcd.render_area);
    }
    lv_jl_lcd.render_ofs = lv_jlgpu_dirty_ofs(&lv_jl_lcd.render_area);
}
#endif

#if LV_UI_TRIPLE_BUFFER_EN
/**
 * @brief 检查显示缓冲区DMM是否正在使用
 * @param disp 显示设备指针
 * @return u8 0:未使用 1:正在使用
 */
static u8 is_display_buffer_busy(lv_display_t *disp)
{
    u32 dmm_addr = EVA_DMM->IN0_BASE;
    uint8_t *dmm_cache_addr = (uint8_t *)psram_no_cache_2_cache((void *)dmm_addr);
    if (dmm_cache_addr == NULL) {
        dmm_cache_addr = (uint8_t *)dmm_addr;
    }
    lv_draw_buf_t *next_act;
    if (disp->buf_act == disp->buf_1) {
        next_act = disp->buf_2;
    } else if (disp->buf_act == disp->buf_2) {
        next_act = disp->buf_3 ? disp->buf_3 : disp->buf_1;
    } else {
        next_act = disp->buf_1;
    }
    if (next_act->data == dmm_cache_addr) {
        printf("is_display_buffer_busy --> 1\n");
        return 1;
    }

    return 0;
}
#endif
void disp_flush_post_msg(void)
{
#if  LV_USE_DRAW_JLGPU_LIST == 2

    lv_display_t *disp_drv = NULL;
    disp_drv = lv_display_get_default();
    lv_draw_jlgpu_unit_t *unit = lv_get_draw_jlgpu_unit();
    uint8_t *px_map = disp_drv->buf_act->data;

    /* 这条路径没有脏区概念, 按整帧走(旋转屏就是整帧旋转), 免得消费线程用到上一帧残留的窗口 */
    lv_area_t full_area = {0};
    full_area.x2 = lv_jl_lcd.lcd_align_width - 1;
    full_area.y2 = lv_jl_lcd.lcd_align_height - 1;
    lv_jlgpu_prepare_render_area(&full_area, &full_area);

    int err = os_taskq_post(JLGPU_RUN_DISP_CB_NAME, 3, disp_drv, unit, px_map);
    if (err) {
        os_sem_post(&jlgpu_run_disp_sem);
        printf("disp_flush_post_msg: jlgpu_run_disp_cb_post err = %d\n", err);
    }
#endif

}

#if LV_USE_DRAW_JLGPU_LIST == 2
/* 等待异步渲染线程排空: LIST==2 下 disp_flush 在 flushing_last 时只把渲染任务
 * os_taskq_post 给 _jlgpu_run_disp_cb, GPU 渲染 + 显示写入在该线程异步完成,
 * 而 lv_display_flush_ready 会提前返回。若调用方紧接着释放渲染引用过的显示
 * buffer, 会与异步渲染竞态, 触发 use-after-free。释放 buffer 前调用本函数,
 * 阻塞到在飞渲染结束, 即可安全释放。
 * 语义: jlgpu_run_disp_sem(初值1) 在 disp_flush pend、在 run_disp_cb post,
 *       因此 sem==0 表示有任务在队列/在飞; pend 成功即表示渲染线程已空闲。 */
bool lv_jlgpu_run_disp_wait_idle(void)
{
    if (os_sem_pend(&jlgpu_run_disp_sem, 300) == OS_ERR_NONE) {
        /* lcd->draw 返回只代表提交了传输；其它推屏入口也可能留下在途 DBI。 */
        extern int lcd_busy(void);
        uint32_t begin = get_system_ms();
        while (lcd_busy()) {
            if ((uint32_t)(get_system_ms() - begin) >= 2000U) {
                os_sem_post(&jlgpu_run_disp_sem);
                printf("lv_jlgpu_run_disp_wait_idle: LCD timeout\n");
                return false;
            }
            os_time_dly(1);
        }
        os_sem_post(&jlgpu_run_disp_sem);
        return true;
    } else {
        printf("lv_jlgpu_run_disp_drain: timeout\n");
        return false;
    }
}

void lv_jlgpu_run_disp_drain(void)
{
    (void)lv_jlgpu_run_disp_wait_idle();
}
#endif

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{

    /* 拍照 VENC 编码期间(display suspend)禁止刷屏: 显示通路已拆、fb 已交给 VENC,
     * 此时若执行 GPU top 联动渲染, GXDMA top 无输出目标, GPU_PND_TOP_DONE 永不
     * 触发, jlgpu_hw_wait_top_pnd_hal 永久超时(贴纸拍照卡死)。suspend 期间直接
     * 告知 LVGL flush 完成, 跳过整段 GPU 渲染 + LCD 写入。 */
    if (!disp_flush_enabled) {
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* printf("disp_flush --> 0x%x,%d,%d,%d,%d last_area:%d flushing_last:%d", (uint32_t)px_map, area->x1, area->x2, area->y1, area->y2, disp_drv->last_area, disp_drv->flushing_last); */

//首先判断页面特效
#if LV_USE_DRAW_JLGPU_LIST
    lv_draw_jlgpu_unit_t *unit = lv_get_draw_jlgpu_unit();

    lv_area_t screen_area;
    screen_area.x1 = 0;
    screen_area.y1 = 0;
    screen_area.x2 = disp_drv->hor_res - 1;
    screen_area.y2 = disp_drv->ver_res - 1;

    //特效期间且联动模式下没有完整的帧buf需要申请整帧
    // JSTC tile 每块 8×8 像素，高度需 8 对齐；对齐多出的行填黑不影响显示
    uint32_t full_src_buffer_size = lv_jl_lcd.lcd_align_height * lv_jl_lcd.lcd_width * BYTE_PER_PIXEL;

    static uint8_t *page_effect_buf = NULL;
    if ((lv_jl_lcd.buffer_size < full_src_buffer_size) && ((g_page_effect_state.effect_enter || g_page_effect_state.effect_busy || g_page_effect_state.effect_exit) || lv_football_menu_active())) {
        if (!page_effect_buf) {
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
            page_effect_buf = malloc_psram(full_src_buffer_size);
#else
            page_effect_buf = lv_malloc(full_src_buffer_size);
#endif
            if (!page_effect_buf) {
                LV_LOG_ERROR("malloc page_effect_buf Failed\n");
                lv_display_flush_ready(disp_drv);
                return;
            }
        }
    }

    /* 足球特效：先断开 GPU 与 DBI/GXDMA 的联动，独占 GPU 输出到 page_effect_buf */
    extern void dbi_link_gpu(u8 en);
    uint8_t football_link_dbi = 0;
    if (lv_football_menu_active()) {
        football_link_dbi = lv_jl_lcd_get_need_gpu_link_dbi();
        if (football_link_dbi) {
            jlgpu_hw_set_sfr_top(NULL);
            lv_jlgpu_pnd_sem_clr();
            dbi_link_gpu(0);
        }
        if (lv_jl_lcd_get_need_gxdma()) {
            JLGXDMA_link_gpu(0);
            jlgpu_hw_set_sfr_top(NULL);
            lv_jlgpu_pnd_sem_clr();
            JLGXDMA_init();
        }
    }

    /* 特效/足球特效这一帧的耗时也纳入统计(它们不走下面 t0/t1 那条路, 直接 return 了)。
     * 拆分点: t0→t1 是 lv_page_effect_manager 里的全屏 3D 合成, t1→收尾是推屏链路。 */
#if LV_JLGPU_DISP_PROFILE
    int64_t fx_t0 = get_system_us();
#endif
    bool fx_handled = lv_page_effect_manager(disp_drv, unit, &screen_area, page_effect_buf ? page_effect_buf : px_map);
#if LV_JLGPU_DISP_PROFILE
    int64_t fx_t1 = get_system_us();
#endif
    if (!fx_handled || lv_football_menu_flush(unit, page_effect_buf ? page_effect_buf : px_map, &screen_area)) {

        // GPU/DMA写入psram后，需要无效化dcache，确保CPU读取到最新数据
        if (page_effect_buf) {
            if (psram_cache_check((void *)page_effect_buf)) {
                psram_flush_invaild_cache(page_effect_buf, full_src_buffer_size);
            }
        }
        // 恢复足球特效的 GPU 联动
        if (football_link_dbi) {
            dbi_link_gpu(1);
        }
        // 若开启页面特效/足球特效，执行下列操作
        if (lv_jl_lcd.gpu_link_dbi) {
            //gpu联动dbi
            lv_draw_buf_t src = {0};
            src.data = page_effect_buf;
            src.data_size = full_src_buffer_size;
            src.header.cf = LV_COLOR_FORMAT_RGB565;
            src.header.h = lv_jl_lcd.lcd_height;
            src.header.w = lv_jl_lcd.lcd_width;
            src.header.stride = lv_jl_lcd.lcd_width * 2;
            lv_jlgpu_link_dbi_draw_full_buf(disp_drv, unit, px_map, &src, &screen_area);
        } else if (lv_jl_lcd.jstc_en) {

            // 特效输出 page_effect_buf(RGB565 line) → GXDMA(JSTC) → px_map(tile)
            // 省掉 GPU copy，GXDMA 直读 PSRAM 中的 page_effect_buf
            // JSTC tile 每块 8×8，高度必须 8 对齐，否则底部 tile 不完整导致残留横条

            // 此时 GXDMA 已由特效管理器断开 GPU 链路（JLGXDMA_link_gpu(0)），
            // 配置 GXDMA 独立模式：src=page_effect_buf(PSRAM), dst=px_map(PSRAM, JSTC tile)
            JLGXDMA_init();

            gxdma_reg_param_t gxdma_param = {0};
            gxdma_param.src       = page_effect_buf;
            gxdma_param.src_fmt   = JLGPU_FORMAT_RGB565;
            gxdma_param.in_stride = lv_jl_lcd.lcd_width * 2;
            gxdma_param.width     = lv_jl_lcd.lcd_width;
            gxdma_param.height    = lv_jl_lcd.lcd_align_height;

            gxdma_param.dst          = px_map;
            gxdma_param.dst_fmt      = JLGPU_FORMAT_ARGB8888;
            gxdma_param.out_adr_mode = 0;   // tile 模式
            gxdma_param.out_stride   = lv_jl_lcd.lcd_align_width / 8 * 64;  // JSTC tile stride

            gxdma_param.jstc_en   = 1;
            gxdma_param.jstc_mode = GXDMA_JSTC_MODE_RGB888;
            gxdma_param.gray_th   = 5;
            gxdma_param.dither_en = 1;

            JLGXDMA_set_param(&gxdma_param);
            JLGXDMA_set_src_area(0, lv_jl_lcd.lcd_width, 0, lv_jl_lcd.lcd_align_height);

            JLGXDMA_run();
            JLGXDMA_wait_done();  // 阻塞等待压缩完成

            // lcd->draw 用实际屏幕区域，不能超 lcd_height 否则 DBI 断言失败
            // DMM 内部已按 lcd_height 从 px_map 读取对应行数的 JSTC tile 数据
            lv_jl_lcd.lcd->set_draw_area(screen_area.x1, screen_area.x2, screen_area.y1, screen_area.y2);
            lv_jl_lcd.lcd->draw((uint8_t *)px_map, screen_area.x1, screen_area.x2, screen_area.y1, screen_area.y2);
        } else {
            lv_jl_lcd.lcd->set_draw_area(screen_area.x1, screen_area.x2, screen_area.y1, screen_area.y2);
            lv_jl_lcd.lcd->draw((uint8_t *)px_map, screen_area.x1, screen_area.x2, screen_area.y1, screen_area.y2);
        }

        /* 这一帧的合成/推屏耗时入账。页面特效与足球特效都走这条路, 只有页面特效
         * 才算进 effect 行 —— 判据用 manager 的三个状态位(与 indev_pointer.c 的
         * ip_effect_idle() 同源), 任一为真即"页面特效在途"。 */
#if LV_JLGPU_DISP_PROFILE
        __lv_gpulist_flush_framerate_calc_ex((u32)(fx_t1 - fx_t0),
                                             (u32)(get_system_us() - fx_t1),
                                             (g_page_effect_state.effect_enter ||
                                              g_page_effect_state.effect_busy ||
                                              g_page_effect_state.effect_exit));
#else
        __lv_gpulist_flush_framerate_calc_ex(0, 0, false);
#endif

        lvgl_try_open_backlight_after_first_frame(disp_drv->flushing_last);
        lv_display_flush_ready(disp_drv);
        return;
    }

    //特效退出清理内存
    if (page_effect_buf) {
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
        free_psram(page_effect_buf);
#else
        lv_free(page_effect_buf);
#endif
        page_effect_buf = NULL;
    }

#endif//页面特效结束

    lv_area_t full_area = {0};
    u8 rotation = is_gxdma_rotate(lv_jl_lcd.rotate);
    if (disp_drv->render_mode == LV_DISPLAY_RENDER_MODE_FULL || disp_drv->render_mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
        full_area.x1 = 0;
        full_area.y1 = 0;
        full_area.x2 = lv_jl_lcd.lcd_align_width - 1;
        full_area.y2 = lv_jl_lcd.lcd_align_height - 1;

        if (disp_drv->flushing_last) {

#if  LV_USE_DRAW_JLGPU_LIST == 2

            /*
             * 必须拿到 token 才允许投递下一帧，这里绝不能带超时后继续投递。
             * 当 consumer 因在途帧 tgen_done 延迟(如插 SD 卡等长时间阻塞/关中断)
             * 卡在 lcd->set_draw_area() 的 lcd_lock() 里时，若生产者超时后仍
             * os_taskq_post + lv_draw_jlgpu_unit_swap()，会持续投递消息并反复
             * swap 双 unit，把尚未被 consumer 消费的链表/显存覆盖，pipeline 永久
             * 错乱而无法自愈。改成一直等 token，行为与 LIST==1 一致：阻塞就地
             * 发生，lcd_sem 一旦释放(该帧 tgen_done 到来)即可自动恢复，且对所有
             * 推屏通路(gpu_link_dbi/gxdma/jstc/普通)都成立。
             */
            if (os_sem_pend(&jlgpu_run_disp_sem, 0) != OS_ERR_NONE) {
                printf("disp_flush: os_sem_pend failed\n");
            }

            /*
             * 圆球特效: 把当帧的特效任务挂到本帧任务链上, 插在哪一层由
             * lv_ball_effect_set_layer() 决定。
             * 只能放在拿到 token 之后: 这时上一帧的链表和附着 buf 已被 consumer
             * 释放、内存池已 clean, 往 unit 上挂任务不会踩到在途帧。
             */
            lv_ball_effect_build_tasks(unit);
#if JLGPU_PUSH_SYNC_WITH_LVGL
            /* 清掉可能残留的计数(disp_flush_post_msg/video disp 也会走同一个消费线程) */
            os_sem_set(&jlgpu_push_done_sem, 0);
#endif

            /* 定下本帧的 GPU 输出窗口(旋转屏收窄到脏区)与 tile 落点, 消费线程要用 */
            lv_jlgpu_prepare_render_area(&full_area, area);

            int err = os_taskq_post(JLGPU_RUN_DISP_CB_NAME, 3, disp_drv, unit, px_map);
            if (err) {
                os_sem_post(&jlgpu_run_disp_sem);
                printf("jlgpu_run_disp_cb_post err = %d\n", err);
                /* 如果任务发送失败，直接在此处开背光 */
                lvgl_try_open_backlight_after_first_frame(1);
#if JLGPU_PUSH_SYNC_WITH_LVGL
                /* 消息没发出去,下面的等待直接放行,不要白等 300ms */
                os_sem_post(&jlgpu_push_done_sem);
#endif
            }
            /* 背光在 _jlgpu_run_disp_cb 里打开以防止闪屏 */

#if JLGPU_PUSH_SYNC_WITH_LVGL
            /* 关键:等推屏(lcd->draw)真正结束再放行 LVGL。
             * 原行为是 post 完就 flush_ready 返回,LVGL 立刻渲染下一帧,
             * 于是"渲染线程"和"推屏线程"会并发操作同一颗 GPU 与同一组 top SFR */
            if (os_sem_pend(&jlgpu_push_done_sem, 300) != OS_ERR_NONE) {
                printf("disp_flush: push_done sem_pend failed\n");
            }
#endif

            lv_draw_jlgpu_unit_swap();

            /* 在此处开背光可防止闪屏 */
            lv_display_flush_ready(disp_drv);

            return;
#endif

#if LV_USE_DRAW_JLGPU_LIST
            // if (psram_cache_check(px_map)) {
            //     DcuInvalidRegion((unsigned int *)px_map, lv_jl_lcd.buffer_size); // FIXME
            // }

            /*
             * 圆球特效: 单链表是同步跑完的, 在 task_list_run 之前把当帧的特效任务
             * 挂到本帧任务链上即可, 插在哪一层由 lv_ball_effect_set_layer() 决定。
             * 上一帧的链表、附着 buf、内存池都在 lv_jlgpu_task_list_free() 里
             * 释放/clean 过了, 这里挂任务不会踩到在途帧。放在 t0 之前, 免得把挂
             * 任务的时间算进 gpu_run 统计。
             */
            lv_ball_effect_build_tasks(unit);

            /* 旋转屏把 GPU 输出窗口收窄到脏区, 落点按 tile 布局自己算 */
            lv_jlgpu_prepare_render_area(&full_area, area);
#if LV_JLGPU_DISP_PROFILE
            int64_t t0 = get_system_us();
#endif
            jlgpu_task_list_disp_run(disp_drv, unit, &lv_jl_lcd.render_area, px_map + lv_jl_lcd.render_ofs);
#if LV_JLGPU_DISP_PROFILE
            int64_t t1 = get_system_us();
#endif
#elif !LV_USE_DRAW_JLGPU
            // run cpu
            DcuFlushRegion((unsigned int *)px_map, lv_jl_lcd.buffer_size);
#endif
            if (rotation) {
                full_area.x1 = 0;
                full_area.y1 = 0;
                full_area.x2 = lv_jl_lcd.lcd_height - 1;
                full_area.y2 = lv_jl_lcd.lcd_width - 1;
            } else {
                full_area.x1 = 0;
                full_area.y1 = 0;
                full_area.x2 = lv_jl_lcd.lcd_width - 1;
                full_area.y2 = lv_jl_lcd.lcd_height - 1;
            }
#if LV_UI_TRIPLE_BUFFER_EN
            if (is_display_buffer_busy(disp_drv)) {
                os_sem_set(&triple_buf_sem, 0);
                if (os_sem_pend(&triple_buf_sem, 100) != OS_ERR_NONE) {
                    printf("disp_flush: os_sem_pend failed\n");
                }
            }
            JLDMM_set_no_wait_vsync(1);
#endif

            lv_jl_lcd.lcd->set_draw_area(full_area.x1, full_area.x2, full_area.y1, full_area.y2);
            lv_jl_lcd.lcd->draw((uint8_t *)px_map, full_area.x1, full_area.x2, full_area.y1, full_area.y2);

#if LV_JLGPU_DISP_PROFILE
#if LV_USE_DRAW_JLGPU_LIST
            /* 单链表(LIST==1)同步渲染: 统计 gpu_run 与 push 耗时 */
            __lv_gpulist_flush_framerate_calc((u32)(t1 - t0), (u32)(get_system_us() - t1));
#else
            /* 纯 CPU 渲染: 无 gpu run, 暂不统计 */
            __lv_gpulist_flush_framerate_calc(0, 0);
#endif
#endif

            /* 在此处开背光可防止闪屏 */
            lvgl_try_open_backlight_after_first_frame(1);
        }

    } else {
#if LV_USE_DRAW_JLGPU_LIST
        jlgpu_task_list_disp_run(disp_drv, unit, area, px_map);
#endif
        lv_jl_lcd.lcd->set_draw_area(area->x1, area->x2, area->y1, area->y2);
        lv_jl_lcd.lcd->draw((uint8_t *)px_map, area->x1, area->x2, area->y1, area->y2);
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    /* partial / 非 last 时提前检测 flushing_last 确保开背光 */
    lvgl_try_open_backlight_after_first_frame(disp_drv->flushing_last);
    lv_display_flush_ready(disp_drv);

    if (disp_drv->flushing_last) {
#if LV_USE_DRAW_JLGPU_LIST
        //释放链表
        lv_jlgpu_task_list_free(unit);
#endif
    }

}


//特效期间且gpu联动dbi需要整帧buf通过gpu输出
#if LV_USE_DRAW_JLGPU_LIST
static void lv_jlgpu_link_dbi_draw_full_buf(lv_display_t *disp_drv, lv_draw_jlgpu_unit_t *unit, uint8_t *px_map, const lv_draw_buf_t *src, const lv_area_t *src_area)
{
    //src
    uint32_t src_width  = src->header.w;
    uint32_t src_height = src->header.h;
    uint32_t src_stride = src->header.stride;
    uint8_t  src_bpp    = lv_color_format_get_bpp(src->header.cf);
    uint8_t  in_format  = lv_jlgpu_get_color_format(src->header.cf);
    uint8_t *img_buf    = src->data;

    jlgpu_draw_img_t draw_img_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};

    //绘制(复制)在dest的位置
    jlgpu_area_t render_area;
    render_area.x_min = src_area->x1;
    render_area.x_max = src_area->x2 + 1;
    render_area.y_min = src_area->y1;
    render_area.y_max = src_area->y2 + 1;

    //src根据src_area裁剪
    jlgpu_area_t tex_clip_area;
    tex_clip_area.x_min = src_area->x1;
    tex_clip_area.x_max = src_area->x2 + 1;
    tex_clip_area.y_min = src_area->y1;
    tex_clip_area.y_max = src_area->y2 + 1;

    jlgpu_img_info_t img_info = {0};

    img_info.premult = 1;
    img_info.tex_clip = tex_clip_area;
    uint8_t adr_mode = JLGPU_ADRMODE_LINE;
    uint8_t *src_clut = NULL;
    uint16_t clut_format = JLGPU_FORMAT_ARGB8888;

    img_info.adr_mode = adr_mode;
    img_info.color_format = in_format;
    img_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;

    img_info.data = (uint8_t *)NO_CACHE_ADDR(img_buf);
    if ((src_bpp >> 3) != 0) {
        img_info.width = src_stride / (src_bpp >> 3);
    } else {
        img_info.width = src_width;
    }
    img_info.height = src_height;
    img_info.compress_size = 0;

    draw_img_params.clut_format = clut_format;
    draw_img_params.clut_adr = src_clut;
    draw_img_params.tex_trans_mode = JLGPU_TRANS_MODE_NONE;
    draw_img_params.img_info = img_info;

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area = render_area;
    render_params.blend_mode = JLGPU_BLEND_SRC;
    render_params.render_quality = JLGPU_RENDERING_QUALITY_NONANTIALIASED;//jlgpu_render_quality_t
    render_params.dither_en = 0;
    render_params.global_alpha = 128;
    mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
    mask_params.mask_ptr = 0;

    //替换
    lv_jlgpu_ctx_t *jlgpu_ctx_list_ori = unit->jlgpu_ctx;
    lv_jlgpu_ctx_t *jlgpu_ctx_list_temp = lv_malloc_zeroed(sizeof(lv_jlgpu_ctx_t));
    if (!jlgpu_ctx_list_temp) {
        LV_LOG_ERROR("malloc  Failed\n");
        return;
    }
    lv_jlgpu_ctx_init(jlgpu_ctx_list_temp);
    unit->jlgpu_ctx = jlgpu_ctx_list_temp;

    //lv_draw_jlgpu_unit_t *unit = lv_get_draw_jlgpu_unit();
    int task_len = jlgpu_draw_img_task_len(unit->jlgpu_ctx->ctx, &draw_img_params, &mask_params);
    //uint8_t *task = lv_jlgpu_mem_pool_get_buf(unit, NULL, task_len);
    uint8_t *task = lv_malloc(task_len);
    if (!task) {
        LV_LOG_ERROR("malloc  Failed\n");
        unit->jlgpu_ctx = jlgpu_ctx_list_ori;
        lv_jlgpu_ctx_uninit(jlgpu_ctx_list_temp);
        lv_free(jlgpu_ctx_list_temp);
        return;
    }
    lv_memset(task, 0, task_len);
    jlgpu_set_task(unit->jlgpu_ctx->ctx, task, task_len);
    jlgpu_draw_img(unit->jlgpu_ctx->ctx, &render_params, &draw_img_params, &mask_params);
    lv_jlgpu_add_task_node(unit, task);

    jlgpu_task_list_disp_run(disp_drv, unit, src_area, px_map);

    //内存清理
    lv_jlgpu_ctx_uninit(jlgpu_ctx_list_temp);
    lv_free(jlgpu_ctx_list_temp);
    lv_free(task);

    //还原
    unit->jlgpu_ctx = jlgpu_ctx_list_ori;
}
#endif


#if LV_USE_DRAW_JLGPU_LIST == 2
/* 等待上一帧 DBI 传输完成(tgen_done 中断清 dbi_busy)。
 * dbi_busy 与 lcd_sem 同生共死：set_draw_area 里 lcd_lock() 拿锁后置 dbi_busy=1，
 * tgen_done 中断里 lcd_unlock() 还锁后置 dbi_busy=0。因此这里 dbi_busy 长期为 1，
 * 即代表该帧的 tgen_done 已丢失(如插 SD 卡等长时间阻塞/关中断)，lcd_sem 被卡死帧
 * 永久占用，consumer 会永远卡在 lcd->set_draw_area() 的 lcd_lock() 里无法自愈。
 * 这里做一次有界等待 + 强制恢复，行为与 LIST==1(同步推屏，阻塞就地发生可自愈)对齐，
 * 且对所有推屏通路(gpu_link_dbi/gxdma/jstc/普通)都成立。 */
static void lv_jl_lcd_wait_or_recover_stuck(void)
{
    extern int lcd_busy(void);
    extern void lcd_unlock(void);
    extern void dbi_link_gpu(u8 en);

    if (!lcd_busy()) {
        return;
    }

    uint32_t begin = get_system_ms();
    while (lcd_busy()) {
        /* 2s 阈值：足够覆盖插卡等瞬时阻塞(期间 tgen_done 迟来即可自愈)，
         * 又不会误伤正常推屏；只有真正丢失 tgen_done 的永久卡死才会触发下面恢复。 */
        if ((get_system_ms() - begin) > 2000) {
            /* 断开 GPU 顶链路并清空 PND，使在途 GPU→DBI/GXDMA 传输失效，
             * 再强制释放被卡死帧占用的 lcd_sem，最后恢复联动继续推屏 */
            jlgpu_hw_set_sfr_top(NULL);
            lv_jlgpu_pnd_sem_clr();
            if (lv_jl_lcd_get_need_gpu_link_dbi()) {
                dbi_link_gpu(0);
                dbi_link_gpu(1);
            }
            if (lv_jl_lcd_get_need_gxdma()) {
                JLGXDMA_link_gpu(0);
                JLGXDMA_init();
            }
            lcd_unlock();
            printf("[lvgl] lcd stuck, force recover\n");
            return;
        }
        os_time_dly(1);
    }
}

static void _jlgpu_run_disp_cb(void *ptr)
{
    int res;
    int msg[8];

    lv_display_t *disp_drv = NULL;
    lv_draw_jlgpu_unit_t *unit = NULL;
    uint8_t *px_map = NULL;

    while (1) {

        //等待lvgl收集完成
        res = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (res != OS_TASKQ) {
            printf("_jlgpu_run_disp_cb err=%d %d\n", res, msg[0]);
            continue;
        }

        //得到可以run的链表上下文
        disp_drv = (lv_display_t *)msg[1];
        unit = (lv_draw_jlgpu_unit_t *)msg[2];
        px_map = (uint8_t *)msg[3];

        if (!disp_drv || !unit || !px_map) {
            printf("_jlgpu_run_disp_cb err 1\n");
            //无法运行本次任务：unit 有效时清理其链表防止泄漏，并归还 token 避免 producer 永久阻塞
            if (unit) {
                lv_jlgpu_task_list_free(unit);
            }
            os_sem_post(&jlgpu_run_disp_sem);
#if JLGPU_PUSH_SYNC_WITH_LVGL
            /* 生产者这时可能正卡在 push_done 上,必须补一次,否则它只能等到超时 */
            os_sem_post(&jlgpu_push_done_sem);
#endif
            continue;
        }

        lv_area_t screen_area;
        screen_area.x1 = 0;
        screen_area.y1 = 0;
        screen_area.x2 = disp_drv->hor_res - 1;
        screen_area.y2 = disp_drv->ver_res - 1;

        lv_area_t full_area = {0};
        u8 rotation = is_gxdma_rotate(lv_jl_lcd.rotate);

        full_area.x1 = 0;
        full_area.y1 = 0;
        full_area.x2 = lv_jl_lcd.lcd_align_width - 1;
        full_area.y2 = lv_jl_lcd.lcd_align_height - 1;

#if LV_JLGPU_DISP_PROFILE
        int64_t t0 = get_system_us();
#endif

        /* 占住"推屏独占门"必须从"配置 TOP/BUF 联动"就开始,一直到 lcd->draw 返回。
         * 只罩 lcd->draw 是不够的:配置联动到这里之前如果被渲染线程插入
         * (lv_jlgpu_ctx_init 里的 jlgpu_hw_set_sfr_top(NULL) + JLGXDMA_link_gpu(0)),
         * 联动会被打断,GPU top 不再产出,DMM 读不到数据 → lcd->draw 永久等待 → 整机冻死。 */
        jlgpu_push_gate_enter();

        /* 窗口与落点由生产者按脏区定好(见 lv_jlgpu_prepare_render_area), 这里直接用 */
        jlgpu_task_list_disp_run(disp_drv, unit, &lv_jl_lcd.render_area, px_map + lv_jl_lcd.render_ofs);

        /* GPU→DBI 联动期间仍可能读取任务链，释放点必须在真实推屏结束之后。 */

#if LV_JLGPU_DISP_PROFILE
        int64_t t1 = get_system_us();
#endif

#if LV_VIDEO_DISP_FB_MODE

        /* 获取第一个可用的 video disp 实例以兼容 FB 模式 */
        lv_video_disp_handle_t *video_handle = lv_port_video_disp_get_instance_by_fb_index(0);
        if (video_handle) {
            struct fb_map_user *fb_map = lv_get_next_video_frame(video_handle);
            //TODO: 处理fb_map数据，和UI渲染完成后的buffer进行blend
            lv_put_next_video_frame(video_handle, fb_map);
            lv_port_video_disp_ready(video_handle);
        }
#endif
        if (rotation) {
            full_area.x1 = 0;
            full_area.y1 = 0;
            full_area.x2 = lv_jl_lcd.lcd_height - 1;
            full_area.y2 = lv_jl_lcd.lcd_width - 1;
        } else {
            full_area.x1 = 0;
            full_area.y1 = 0;
            full_area.x2 = lv_jl_lcd.lcd_width - 1;
            full_area.y2 = lv_jl_lcd.lcd_height - 1;
        }

#if LV_UI_TRIPLE_BUFFER_EN
        if (is_display_buffer_busy(disp_drv)) {
            os_sem_set(&triple_buf_sem, 0);
            if (os_sem_pend(&triple_buf_sem, 100) != OS_ERR_NONE) {
                printf("disp_flush: os_sem_pend failed\n");
            }
        }
        JLDMM_set_no_wait_vsync(1);
#endif

        /* 推屏前先检测上帧 DBI 是否卡死(如插 SD 卡致 tgen_done 丢失)，卡死则强制恢复，
         * 避免 LIST==2 的 consumer 永久卡在下面 set_draw_area() 的 lcd_lock() 里 */
        lv_jl_lcd_wait_or_recover_stuck();

        lv_jl_lcd.lcd->set_draw_area(full_area.x1, full_area.x2, full_area.y1, full_area.y2);
        lv_jl_lcd.lcd->draw((uint8_t *)px_map, full_area.x1, full_area.x2, full_area.y1, full_area.y2);

        /* draw 是异步提交；等 tgen_done 清 busy 才能释放链表、独占门及完成信号。 */
        lv_jl_lcd_wait_or_recover_stuck();

        /* 推屏结束,放开独占门(见上面的 jlgpu_push_gate_enter) */
        jlgpu_push_gate_leave();

        lv_jlgpu_task_list_free(unit);


#if LV_JLGPU_DISP_PROFILE
        u32 gpu_us = (u32)(t1 - t0);
        u32 push_us = (u32)(get_system_us() - t1);
#endif

        /* 在所有硬件操作完成后开背光（LIST==2 专用） */
        lvgl_try_open_backlight_after_first_frame(1);

#if LV_JLGPU_DISP_PROFILE
        __lv_gpulist_flush_framerate_calc(gpu_us, push_us);
#else
        __lv_gpulist_flush_framerate_calc(0, 0);
#endif

        os_sem_post(&jlgpu_run_disp_sem);

#if JLGPU_PUSH_SYNC_WITH_LVGL
        /* 通知生产者:本帧推屏已结束,可以 flush_ready 继续下一帧 */
        os_sem_post(&jlgpu_push_done_sem);
#endif


    }
}

#endif

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif


