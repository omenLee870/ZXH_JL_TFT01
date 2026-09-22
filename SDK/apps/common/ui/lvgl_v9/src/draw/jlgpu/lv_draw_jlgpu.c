/*********************
 *      INCLUDES
 *********************/
/*#include "lvgl.h"*/
#include "lv_draw_jlgpu.h"
#include "fs/fs.h"
#include "../../misc/lv_area_private.h"
#include "../../core/lv_refr_private.h"
#include "jlgpu_utils.h"
#include "gxdma_driver.h"
#include "ui/lcd/lcd_conf.h"
#include "cpu.h"

#include "app_config.h"
#include "asm/psram_api.h"
#include "malloc.h"

#if CONFIG_LVGL_UI_ENABLE


#if LV_USE_DRAW_JLGPU

#if LV_USE_DRAW_JLGPU_LIST
/* 链表内存申请失败(池+堆都失败)时打印详细的上下文，便于区分
 * "单帧链表占用过大" 还是 "整个堆已耗尽" */
static void jlgpu_list_alloc_fail_report(lv_draw_jlgpu_unit_t *unit, uint32_t buf_size)
{
    uint32_t task_num = unit->jlgpu_ctx ? unit->jlgpu_ctx->jlgpu_task_num : 0;
    size_t free_ram0 = memory_get_size(P_MEMORY_UNUSED);
    size_t free_ram1 = memory_get_size(P_VLT_MEMORY_UNUSED);

    printf("!!! jlgpu list alloc failed: want=%u pool_pos=%u/%u attach_bytes=%u task_num=%u free_ram0=%u free_ram1=%u\n",
           (unsigned)buf_size, (unsigned)unit->pool_pos, (unsigned)unit->pool_size,
           (unsigned)unit->attach_buf_bytes, (unsigned)task_num,
           (unsigned)free_ram0, (unsigned)free_ram1);
}
#endif

/*********************
 *      DEFINES
 *********************/

#define DRAW_UNIT_ID_JLGPU 102
#define ALIGN_UP_4(x) (((x) + 3) & ~3)

// 儿童相机专属：链表内存池调小以省 RAM（公共 lv_conf.h 默认 30*1024）
#if (defined(SUB_CASE_TYPE_KID_CAMERA) && (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_KID_CAMERA))
#undef JLGPU_LIST_MEM_POOL_SIZE
#define JLGPU_LIST_MEM_POOL_SIZE (10*1024)
#endif

/* attach_buf 大于该大小(字节)时优先用 PSRAM 申请，可用宏覆盖；0 表示禁用 */
#ifndef JLGPU_ATTACH_BUF_PSRAM_THRESHOLD
#define JLGPU_ATTACH_BUF_PSRAM_THRESHOLD   (10 * 1024)
#endif
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#include "gpu_port.h"
/*#include "gpu_debug.h"*/

/* ------------------------------------------------------------------------------------*/
/* GPU 中断处理，开始 */
extern const int UI_GPU_IRQ_MODE;      		 /*! 中断模式   */

static  OS_SEM						pnd_sem; /*! 异步信号量 */
static  OS_SEM						top_pnd_sem; /*! 异步信号量 */
static volatile unsigned int isr_status = 0; /*! GPU中断状态 */
static int gpu_init_sta = 0;                    /*gpu初始化状态*/

/* 未定义先调用的函数，统一在此处前置声明 */
static int __jlgpu_hw_init_hal(void);
static inline int jlgpu_hw_get_init_sta(void);

#define GPU_SFR                     EVA_GPU
#define GPU_PEND_TIMEOUT			50 		 /*! 中断超时ms */
#define GPU_IRQ_PRIORITY			6  		 /*! 中断优先级 */
#define GPU_IRQ_CPU_CORE			0  		 /*! 中断CPU 核 */

#define GPU_INTERRUPT_EN(bit)		(GPU_SFR->PND_IE  |= BIT(bit)) /*! 中断使能 */
#define GPU_INTERRUPT_ACQ(bit)		(GPU_SFR->PND_CON &  BIT(bit)) /*! 中断检查 */
#define GPU_INTERRUPT_CLEAR(bit)	(GPU_SFR->PND_CLR |= BIT(bit)) /*! 中断清理 */
#define EMPTY_STATEMENT				;  //! 空语句

#define GPU_INTERRUPT_HDL(bit, record, process) \
{ \
	if (GPU_INTERRUPT_ACQ(bit)) { \
		GPU_INTERRUPT_CLEAR(bit); \
		(record) |= BIT(bit); \
		process; \
	} \
}

___interrupt
static void gpu_isr(void)
{
    /* printf("enter gpu isr:\n"); */
    /* printf("gpu irq  en: 0x%x\n", EVA_GPU->PND_IE); */
    /* printf("gpu irq con: 0x%x\n", EVA_GPU->PND_CON); */
    /* printf("gpu irq clr: 0x%x\n", EVA_GPU->PND_CLR); */

    GPU_INTERRUPT_HDL(GPU_PND_LAY_DONE, isr_status, EMPTY_STATEMENT);
    GPU_INTERRUPT_HDL(GPU_PND_WIN_DONE, isr_status, os_sem_post(&pnd_sem));
    GPU_INTERRUPT_HDL(GPU_PND_BREAK,    isr_status, os_sem_post(&pnd_sem));
    GPU_INTERRUPT_HDL(GPU_PND_PAUSE,    isr_status, EMPTY_STATEMENT);
    GPU_INTERRUPT_HDL(GPU_PND_RLE_ERR,  isr_status, EMPTY_STATEMENT);
    GPU_INTERRUPT_HDL(GPU_PND_WCNT_OV,  isr_status, EMPTY_STATEMENT);
    GPU_INTERRUPT_HDL(GPU_PND_ECNT_OV,  isr_status, EMPTY_STATEMENT);
    GPU_INTERRUPT_HDL(GPU_PND_SPH_DONE, isr_status, EMPTY_STATEMENT);
    GPU_INTERRUPT_HDL(GPU_PND_CFG_ERR,  isr_status, EMPTY_STATEMENT);
    GPU_INTERRUPT_HDL(GPU_PND_TOP_DONE, isr_status, os_sem_post(&top_pnd_sem));
    /* __this->pnd = isr_status; */

    /* printf("leave gpu isr:\n"); */
    /* printf("gpu irq  en: 0x%x\n", EVA_GPU->PND_IE); */
    /* printf("gpu irq con: 0x%x\n", EVA_GPU->PND_CON); */
    /* printf("gpu irq clr: 0x%x\n", EVA_GPU->PND_CLR); */
}


static int lv_jlgpu_wait_done(void)
{
    int error = OS_NO_ERR;

    if (UI_GPU_IRQ_MODE) {
        int count = 3;
        /* 异步等待，信号量同步 */
        do {
            error = os_sem_pend(&pnd_sem, GPU_PEND_TIMEOUT);
            __asm_csync();

            if (error == OS_NO_ERR) {
                break;
            } else {
                printf("@@@ %s() sem_pend err: %d\n", __func__, error);
            }
            count --;
        } while (count);

    } else {
        /* 同步等待，会停住 CPU */
        JLGPU_SFR_TypeDef *JLGPU2D = (JLGPU_SFR_TypeDef *)EVA_GPU;
        while (!(JLGPU2D->pnd_con.win_done_pnd)) {
            /*os_time_dly(1);*/
        }

        JLGPU2D->pnd_clr.win_done_clr = 1;
    }
    return error;
}

int lv_jlgpu_pnd_sem_clr(void)
{
    if (UI_GPU_IRQ_MODE) {
        if (!os_sem_valid(&pnd_sem)) {
            return 0;
        }
        return os_sem_set(&pnd_sem, 0);
    } else {
        return 0;
    }
}

static void lv_jlgpu_pnd_sem_init()
{
    // GPU_INTERRUPT_EN(GPU_PND_LAY_DONE);
    GPU_INTERRUPT_EN(GPU_PND_WIN_DONE);
    GPU_INTERRUPT_EN(GPU_PND_BREAK);
    GPU_INTERRUPT_EN(GPU_PND_PAUSE);
    GPU_INTERRUPT_EN(GPU_PND_RLE_ERR);
    GPU_INTERRUPT_EN(GPU_PND_WCNT_OV);
    GPU_INTERRUPT_EN(GPU_PND_ECNT_OV);
    GPU_INTERRUPT_EN(GPU_PND_SPH_DONE);
    GPU_INTERRUPT_EN(GPU_PND_CFG_ERR);
    GPU_INTERRUPT_EN(GPU_PND_TOP_DONE);

    if (UI_GPU_IRQ_MODE) {
        os_sem_create(&pnd_sem, 0);
        os_sem_create(&top_pnd_sem, 0);
    }

    request_irq(IRQ_GPU_IDX, GPU_IRQ_PRIORITY, gpu_isr, GPU_IRQ_CPU_CORE);
}

int lv_jlgpu_get_isr_state(void)
{
    return isr_status;
}

void lv_jlgpu_clr_isr_state(void)
{
    isr_status = 0x00;
}

int lv_jlgpu_check_isr_state(gpu_pnd_type_t pnd)
{
    return !!(isr_status & BIT(pnd));
}

void lv_jlgpu_clear_isr_state(gpu_pnd_type_t pnd)
{
    isr_status &= ~BIT(pnd);
}
/* GPU 中断处理，结束 */
/* ------------------------------------------------------------------------------------*/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_draw_jlgpu_unit_t *global_draw_jlgpu_unit = NULL;

static jlgpu_global_path_t *jlgpu_global_path = NULL;

static lv_jlgpu_ctx_t jlgpu_base_ctx = {0};

static OS_MUTEX jlgpu_mutex;


/* 页面切换特效全局状态（多unit共享，避免swap导致的标志混乱） */
lv_page_effect_state_t g_page_effect_state = {0};


#if LV_USE_DRAW_JLGPU_LIST == 2

static lv_draw_jlgpu_unit_t *global_draw_jlgpu_unit_1 = NULL;
static lv_draw_jlgpu_unit_t *global_draw_jlgpu_unit_2 = NULL;
void lv_draw_jlgpu_unit_swap(void)
{
    if (global_draw_jlgpu_unit == global_draw_jlgpu_unit_1) {
        global_draw_jlgpu_unit = global_draw_jlgpu_unit_2;
    } else {
        global_draw_jlgpu_unit = global_draw_jlgpu_unit_1;
    }
}

#endif

void lv_jlgpu_mutex_pend(void)
{
    os_mutex_pend(&jlgpu_mutex, 0);
}

void lv_jlgpu_mutex_post(void)
{
    os_mutex_post(&jlgpu_mutex);
}

/* ================== 推屏期 GPU 独占门 ==================
 * 背景:开了 gpu_link_dbi/jstc 联动后,GPU 是"边算边喂 DBI/GXDMA"的。推屏(lcd->draw)
 * 期间若被别的线程改了 TOP/BUF/WIN 等 SFR,或者被别的 GPU 任务抢走硬件,联动就会断流,
 * DBI/DMM 永久等待 → 整机冻死(心跳停、无异常打印、不复位)。
 *
 * LIST==2 下"渲染线程(LVGL)"与"推屏线程(消费线程)"是两个线程。原实现只保证
 * "配SFR + jlgpu_run" 那一段互斥,不保证推屏期间没人动 GPU —— 这就是偶发冻死的根因。
 *
 * 这里用一对"意图声明"标志实现互斥(渲染侧只有 LVGL 一个线程会来,推屏侧只有一个消费线程):
 *   渲染侧:进入"立即绘制"(离屏 layer/canvas)段前 jlgpu_gpu_gate_enter(),
 *           段末 jlgpu_gpu_gate_leave()。
 *   推屏侧:必须从"配置 TOP/BUF 联动"之前 jlgpu_push_gate_enter(),
 *           一直到 lcd->draw 返回之后 jlgpu_push_gate_leave()。
 *           ⚠ 只罩 lcd->draw 是不够的:配联动和 lcd->draw 之间若被渲染线程插入
 *             (ctx_init 里的 jlgpu_hw_set_sfr_top(NULL) + JLGXDMA_link_gpu(0)),
 *             联动会被打断 → GPU top 不产出 → DMM 读不到数据 → lcd->draw 永久等待。
 * 渲染侧所有真正提交 GPU 的路径(所有 jlgpu_run)都位于 lv_jlgpu_ctx_init/uninit 之间,
 * 所以在这两个函数里挂钩即可全覆盖。
 *
 * 注意:构建 GPU 链表(显存目标路径)是纯 CPU 操作,不经过这道门,
 *      因此仍与推屏、与 GPU 执行并行 —— 双链路的性能收益得以保留。
 * ====================================================*/
#if LV_USE_DRAW_JLGPU_LIST

/* get_system_ms() 在预编译库里,本文件不经过 system/includes.h,按 lv_port_disp.c 的做法就地声明 */
extern uint32_t get_system_ms(void);

/* 门的最长等待时间(毫秒)。必须大于 lv_jl_lcd_wait_or_recover_stuck() 的 2000ms 上界:
 * 那条"lcd 卡死恢复"流程是在门内跑的,它自己会重配 TOP 联动并 dbi_link_gpu(0/1);
 * 渲染侧若在它跑完之前就超时强行进门,ctx_init 里的 jlgpu_hw_set_sfr_top(NULL) +
 * JLGXDMA_link_gpu(0) 会把它刚建好的联动拆掉 —— 正是本门要防的那种冻死。
 * 所以宁可多等:等待有界且可恢复,拆联动不可恢复。
 * 另:这里不能再用"数 tick"实现 —— 本平台 tick 周期没有公开配置,数不出墙钟时间。 */
#define JLGPU_GATE_WAIT_MS      2500

static volatile uint8_t s_jlgpu_push_busy  = 0;     /* 1=正在推屏(lcd->draw 中) */
static volatile uint8_t s_jlgpu_gpu_submit = 0;     /* 正在做"立即绘制"的线程数 */

void jlgpu_gpu_gate_enter(void)
{
    uint32_t begin = get_system_ms();
    uint32_t waited;

    for (;;) {
        s_jlgpu_gpu_submit++;               /* 先声明自己的意图 */
        if (!s_jlgpu_push_busy) {
            return;                         /* 推屏还没开始,放行 */
        }
        s_jlgpu_gpu_submit--;               /* 推屏中,撤回意图,等一会儿再来 */
        waited = get_system_ms() - begin;   /* 无符号相减,回绕自然成立 */
        if (waited > JLGPU_GATE_WAIT_MS) {
            printf("!!! jlgpu_gpu_gate_enter timeout %ums, push_busy=%d\n",
                   (unsigned)waited, (int)s_jlgpu_push_busy);
            s_jlgpu_gpu_submit++;
            return;
        }
        os_time_dly(1);
    }
}

void jlgpu_gpu_gate_leave(void)
{
    if (s_jlgpu_gpu_submit) {
        s_jlgpu_gpu_submit--;
    }
}

void jlgpu_push_gate_enter(void)
{
    uint32_t begin = get_system_ms();
    uint32_t waited;

    for (;;) {
        s_jlgpu_push_busy = 1;              /* 先声明自己的意图 */
        if (!s_jlgpu_gpu_submit) {
            return;                         /* 没有人在做立即绘制,放行 */
        }
        s_jlgpu_push_busy = 0;              /* 有人正在改 SFR/抢 GPU,撤回等一会儿 */
        waited = get_system_ms() - begin;
        if (waited > JLGPU_GATE_WAIT_MS) {
            /* 等这么久还等不到,按异常处理:强制重新同步,避免计数泄漏后每帧都白等 */
            printf("!!! jlgpu_push_gate_enter timeout %ums, submit=%d\n",
                   (unsigned)waited, (int)s_jlgpu_gpu_submit);
            s_jlgpu_gpu_submit = 0;
            s_jlgpu_push_busy = 1;
            return;
        }
        os_time_dly(1);
    }
}

void jlgpu_push_gate_leave(void)
{
    s_jlgpu_push_busy = 0;
}

#else   /* 非链表模式:没有独立推屏线程,门恒开 */

void jlgpu_gpu_gate_enter(void)  {}
void jlgpu_gpu_gate_leave(void)  {}
void jlgpu_push_gate_enter(void) {}
void jlgpu_push_gate_leave(void) {}

#endif


static void jlgpu_hw_wait_done_hal(void)
{

    lv_jlgpu_wait_done();

#if 0

    JLGPU_SFR_TypeDef *JLGPU2D = (JLGPU_SFR_TypeDef *)EVA_GPU;

#if !GPU_PLATFORM_PC
    // task_end_pnd
    //__asm_csync();
    while (!(JLGPU2D->pnd_con.win_done_pnd)) {
        /*putchar('b');*/
        /* os_time_dly(1); */
    }

    /*if(JLGPU2D->pnd_con.ecnt_ov_pnd){*/
    /*JLGPU2D->pnd_clr.ecnt_ov_clr = 1;*/
    /*}*/

    //clear task_end_pnd
    //JL_GPU->CON0 |= BIT(7) | BIT(8) | BIT(9) | BIT(14) | BIT(18);
    JLGPU2D->pnd_clr.win_done_clr = 1;
    /*printf("wait done finish\n");*/
#else

#endif
#endif

}

static void jlgpu_hw_wait_top_pnd_hal(void)
{
    int error = OS_NO_ERR;

    if (UI_GPU_IRQ_MODE) {
        int count = 3;
        do {
            error = os_sem_pend(&top_pnd_sem, GPU_PEND_TIMEOUT);
            __asm_csync();

            if (error == OS_NO_ERR) {
                break;
            } else {
                printf("@@@ %s() sem_pend err: %d\n", __func__, error);
            }
            count --;
        } while (count);
    } else {
        JLGPU_SFR_TypeDef *JLGPU2D = (JLGPU_SFR_TypeDef *)EVA_GPU;
        // task_end_pnd
        //__asm_csync();
        while (!(JLGPU2D->pnd_con.top_down_pnd)) {
            /*putchar('b');*/
            /* os_time_dly(1); */
        }
        JLGPU2D->pnd_clr.top_down_clr = 1;
    }

}

void jlgpu_hw_sfr_register_dump(void)
{
    printf("==================================\n");
    printf("GPU->PND_CON=0x%x\n", EVA_GPU->PND_CON);
    printf("GPU->STA_CON=0x%x\n", EVA_GPU->STA_CON);
    printf("GPU->OUT_FMT=0x%x\n", EVA_GPU->OUT_FMT);
    printf("GPU->OUT_STRIDE=0x%x\n", EVA_GPU->OUT_STRIDE);
    printf("GPU->WIN_BASE=0x%x\n", EVA_GPU->WIN_BASE);
    printf("GPU->WIN_X=0x%x\n", EVA_GPU->WIN_X);
    printf("GPU->WIN_Y=0x%x\n", EVA_GPU->WIN_Y);
    printf("GPU->COLORKEY=0x%x\n", EVA_GPU->CKEY_RGB);
    printf("GPU->UV_BASE_ADR=0x%x\n", EVA_GPU->UV_BASE_ADR);
    printf("GPU->VTX_CON0=0x%x\n", EVA_GPU->VTX_CON0);
    printf("GPU->VTX_CON1=0x%x\n", EVA_GPU->VTX_CON1);
    printf("GPU->E_BASE=0x%x\n", EVA_GPU->EBUF_BASE);
    printf("GPU->C_BASE=0x%x\n", EVA_GPU->CBUF_BASE);
    printf("GPU->GB_TRANS_CON=0x%x\n", EVA_GPU->GBT_CON);

    printf("GPU->TOP_CON=0x%x\n", EVA_GPU->TOP_CON);
    printf("GPU->TOP_INFO=0x%x\n", EVA_GPU->TOP_INFO);
    printf("GPU->TOP_OFS=0x%x\n", EVA_GPU->TOP_OFS);
    printf("GPU->BUF_INFO=0x%x\n", EVA_GPU->BUF_INFO);
    printf("GPU->BUF_BASE0=0x%x\n", EVA_GPU->BUF_BASE0);
    printf("GPU->BUF_BASE1=0x%x\n", EVA_GPU->BUF_BASE1);

}

static void jlgpu_hw_set_top_run(void *task_base)
{
    EVA_GPU->NXT_INST_ADR = (unsigned int)task_base; // 联动GXDMA top kstart

    // jlgpu_hw_sfr_register_dump();

    SFR(EVA_GPU->TOP_CON, 0, 1, 1); // top kick_start
}

static void jlgpu_hw_run_hal(void *task_base)
{
    if (!jlgpu_hw_get_init_sta()) {
        if (UI_GPU_IRQ_MODE) {
            lv_jlgpu_pnd_sem_init();
        }
        __jlgpu_hw_init_hal();
    }
    os_mutex_pend(&jlgpu_mutex, 0);

    JLGPU_SFR_TypeDef *JLGPU2D = (JLGPU_SFR_TypeDef *)EVA_GPU;

    u32 top_width = EVA_GPU->TOP_INFO & 0xfff;
    u32 top_height = (EVA_GPU->TOP_INFO >> 16) & 0xfff;
    if (top_width || top_height) {
        //配置了TOP 宽高说明开启了联动
        jlgpu_hw_set_top_run(task_base);
        jlgpu_hw_wait_top_pnd_hal();
        os_mutex_post(&jlgpu_mutex);
        return;
    }

    // jlgpu_hw_sfr_register_dump();
    EVA_GPU->NXT_INST_ADR = (uint32_t)task_base;
    JLGPU2D->sta_con.kick_start = 1; //run
    jlgpu_hw_wait_done_hal();

    os_mutex_post(&jlgpu_mutex);
}
static int __jlgpu_hw_init_hal()
{
    JLGPU_SFR_TypeDef *JL_GPU2D = (JLGPU_SFR_TypeDef *)EVA_GPU;

#if !GPU_PLATFORM_PC
    // SFR(JL_VDOCOM->EVA_CON, 9, 1, 1);
    // SFR(JL_VDOCOM->EVA_CON, 10, 1, 1);
    // SFR(JL_VDOCOM->EVA_CON, 0, 4, 1);

    SFR(EVA_GPU->COM_CON, 0, 1, 1); //cken
    SFR(EVA_GPU->COM_CON, 1, 1, 0);
    asm("nop");
    SFR(EVA_GPU->COM_CON, 1, 1, 1); //nrst
    asm("nop");
#endif

#if !GPU_PLATFORM_PC
    EVA_GPU->PND_CLR = 0xffff;
#else
    gpu_init(JLGPU2D);
#endif
    gpu_init_sta = 1;
    return 0;
}

static int jlgpu_hw_init_hal(void)
{
    os_mutex_pend(&jlgpu_mutex, 0);
    __jlgpu_hw_init_hal();

    os_mutex_post(&jlgpu_mutex);

    return 0;
}
static inline int jlgpu_hw_get_init_sta()
{
    return  gpu_init_sta;
}
static int jlgpu_hw_free_hal(void)
{
    return 0;
}

void jlgpu_hw_set_sfr_top(JLGPU_TOP_INFO_TypeDef *sfr_top)
{

    os_mutex_pend(&jlgpu_mutex, 0);

    SFR(EVA_GPU->TOP_CON, 1, 1, 1); // clean vbus
    SFR(EVA_GXDMA->STA_CON, 2, 1, 1); // clean vbus
    if (sfr_top) {
        SFR(EVA_GPU->TOP_INFO, 0, 12, sfr_top->top_info.top_w); // top_w
        SFR(EVA_GPU->TOP_INFO, 16, 12, sfr_top->top_info.top_h); // top_h
        SFR(EVA_GPU->TOP_OFS, 0, 12, sfr_top->top_ofs.top_x_ofs); // top_x_ofs
        SFR(EVA_GPU->TOP_OFS, 16, 12, sfr_top->top_ofs.top_y_ofs); // top_y_ofs
        SFR(EVA_GPU->BUF_INFO, 0, 12, sfr_top->buf_info.buf_w);
        SFR(EVA_GPU->BUF_INFO, 16, 12, sfr_top->buf_info.buf_h);

        EVA_GPU->BUF_BASE0 = (u32)sfr_top->buf_base0.buf_base0;
        EVA_GPU->BUF_BASE1 = (u32)sfr_top->buf_base1.buf_base1;
    } else {
        SFR(EVA_GPU->TOP_INFO, 0, 12, 0); // top_w
        SFR(EVA_GPU->TOP_INFO, 16, 12, 0); // top_h
        SFR(EVA_GPU->TOP_OFS, 0, 12, 0); // top_x_ofs
        SFR(EVA_GPU->TOP_OFS, 16, 12, 0); // top_y_ofs
        SFR(EVA_GPU->BUF_INFO, 0, 12, 0);
        SFR(EVA_GPU->BUF_INFO, 16, 12, 0);

        EVA_GPU->BUF_BASE0 = 0;
        EVA_GPU->BUF_BASE1 = 0;
    }

    os_mutex_post(&jlgpu_mutex);
}


lv_jlgpu_ctx_t *lv_get_base_jlgpu_ctx(void)
{
    return &jlgpu_base_ctx;
}

void jlgpu_ctx_buffer_init(lv_jlgpu_ctx_t *jlgpu_ctx)
{

    int size = get_jlgpu_context_size();

    if (jlgpu_ctx->ctx_buf == NULL) {
        jlgpu_ctx->ctx_buf = malloc(size);
        if (jlgpu_ctx->ctx_buf == NULL) {
            LV_LOG_ERROR("malloc ctx_buf error");
            return;
        }
        memset(jlgpu_ctx->ctx_buf, 0, size);
        jlgpu_ctx->ctx = jlgpu_create_context(jlgpu_ctx->ctx_buf);
    }

    jlgpu_ctx->jlgpu_ops.gpu_init_hal = jlgpu_hw_init_hal;
    jlgpu_ctx->jlgpu_ops.gpu_uninit_hal = jlgpu_hw_free_hal;
    jlgpu_ctx->jlgpu_ops.gpu_run_hal = jlgpu_hw_run_hal;

    jlgpu_set_ops(jlgpu_ctx->ctx, &jlgpu_ctx->jlgpu_ops);

}

void lv_jlgpu_ctx_init(lv_jlgpu_ctx_t *jlgpu_ctx)
{
    /* 与推屏互斥:本函数会拆 GPU↔DBI 联动(清 TOP/BUF 等 SFR),推屏期间做这个会让
     * DBI 直接断流冻死;紧随其后的段内 jlgpu_run 也会抢 GPU。
     * 对应 lv_jlgpu_ctx_uninit() 里的 gate_leave,必须成对。 */
    jlgpu_gpu_gate_enter();

    if (jlgpu_ctx == NULL) {
        LV_LOG_ERROR("lv_jlgpu_init error");
        jlgpu_gpu_gate_leave();
        return;
    }

    os_mutex_pend(&jlgpu_mutex, 0);

    JLGXDMA_link_gpu(0);
    jlgpu_hw_set_sfr_top(NULL);
    lv_jlgpu_pnd_sem_clr();
    JLGXDMA_init();

    os_mutex_post(&jlgpu_mutex);

    lv_memzero(jlgpu_ctx, sizeof(lv_jlgpu_ctx_t));
    jlgpu_ctx_buffer_init(jlgpu_ctx);

    jlgpu_init(jlgpu_ctx->ctx);

}


static int jlgpu_hw_get_status(void)
{
    if (!os_sem_valid(&pnd_sem)) {
        return 0;
    }
    return ((EVA_GPU->STA_CON >> 2) & 0x03);
}


void sfc_wait_jlgpu_idle(u32 id)
{
    while (jlgpu_hw_get_status());
}
REGISTER_TZSPI_TARGET(lv_jlgpu_tzspi) = {
    .suspend = sfc_wait_jlgpu_idle,
};

void lv_jlgpu_ctx_uninit(lv_jlgpu_ctx_t *jlgpu_ctx)
{
    if (jlgpu_ctx) {
//fixme:存在重复释放问题
#if 0
        if (jlgpu_ctx->ctx_buf) {
            lv_free(jlgpu_ctx->ctx_buf);
        }
#endif
        jlgpu_destroy_context(jlgpu_ctx->ctx);
        jlgpu_ctx->ctx_buf = NULL;
        jlgpu_ctx->ctx = NULL;
        memset(&jlgpu_ctx->jlgpu_ops, 0, sizeof(jlgpu_ops_t));
        /* 快照随 ctx 一起销毁; 不释放的话 lv_jlgpu_ctx_init() 里的 memzero 会把指针抹掉, 直接泄漏 */
        lv_jlgpu_task_list_snapshot_free(jlgpu_ctx);
    }

    /* 与 lv_jlgpu_ctx_init() 里的 gate_enter 配对,释放推屏独占门 */
    jlgpu_gpu_gate_leave();
}


void lv_jlgpu_reset_ctx_buf(lv_jlgpu_ctx_t *jlgpu_ctx)
{
    if (jlgpu_ctx) {
        if (jlgpu_ctx->ctx_buf) {
            int size = get_jlgpu_context_size();
            memset(jlgpu_ctx->ctx_buf, 0, size);
        }
    }
}


/*
 * Evaluate a task and set the score and preferred JL GPU unit.
 * Return 1 if task is preferred, 0 otherwise (task is not supported).
 */
static int32_t _jlgpu_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task);

/*
 * Dispatch a task to the JL GPU unit.
 * Return 1 if task was dispatched, 0 otherwise (task not supported).
 */
static int32_t _jlgpu_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer);

/*
 * Delete the JL GPU draw unit.
 */
static int32_t _jlgpu_delete(lv_draw_unit_t *draw_unit);

#if LV_DRAW_JLGPU_ASYNC && LV_USE_OS
static void _jlgpu_render_thread_cb(void *ptr);
#endif

static void _jlgpu_execute_drawing(lv_draw_jlgpu_unit_t *u);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool check_image_cf_is_supported(lv_color_format_t cf)
{
    uint8_t gpu_cf = lv_jlgpu_get_color_format(cf);
    if (gpu_cf == 0xff) {
        return false;
    } else {
        return true;
    }
}

static void lv_draw_buf_jlgpu_init_handlers(void)
{
    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb = _jlgpu_buf_invalidate_cache_cb;
}

void _jlgpu_buf_invalidate_cache_cb(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
    uint8_t *dest_buf;
    int32_t i = 0;
    int32_t width = lv_area_get_width(area);
    int32_t lines = lv_area_get_height(area);

    uint8_t *buf = draw_buf->data;
    uint32_t stride = draw_buf->header.stride;
    lv_color_format_t color_format = draw_buf->header.cf;
    uint32_t bytes_per_pixel = lv_color_format_get_size(color_format);
    uint32_t width_bytes = width * bytes_per_pixel;
    //printf("flush area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d). stride = %d.", area->x1, area->y1, area->x2, area->y2, stride);

    dest_buf = buf + (area->y1 * stride) + (area->x1 * bytes_per_pixel);
    for (i = 0; i < lines; i++) {
        DcuFlushinvRegion((u32 *)dest_buf, width_bytes);    //帧buff,清除 area 大小的相对地址空间
        dest_buf += stride;
    }
}

static int32_t _jlgpu_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task)
{
    //log_debug("%s()", __func__);

#if LV_USE_DRAW_JLGPU_LIST == 2
    lv_draw_jlgpu_unit_t *draw_jlgpu_unit = lv_get_draw_jlgpu_unit();
#else
    lv_draw_jlgpu_unit_t *draw_jlgpu_unit = (lv_draw_jlgpu_unit_t *) draw_unit;
#endif

    if ((lv_area_get_size(&(task->area)) <= LV_JL_GPU_MIN_AREA_SIZE) && (task->type != LV_DRAW_TASK_TYPE_LABEL)) {
        return 0;   /* 需要渲染的区域太小则放弃使用 GPU */
    }

    switch (task->type) {

    case LV_DRAW_TASK_TYPE_FILL: {

#if (LV_USE_DRAW_JLGPU_FILL_ENABLE == 0)
        return 0;
#endif

        lv_draw_fill_dsc_t *draw_dsc = (lv_draw_fill_dsc_t *)task->draw_dsc;

        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }
    case LV_DRAW_TASK_TYPE_IMAGE:
    case LV_DRAW_TASK_TYPE_LAYER: {
#if (LV_USE_DRAW_JLGPU_IMG_ENABLE == 0 || LV_USE_DRAW_JLGPU_LAYER_ENABLE == 0)
        return 0;
#endif
        lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *)task->draw_dsc;
        lv_result_t  res;
        lv_image_header_t img_header;
        lv_layer_t *layer_to_draw = (lv_layer_t *)draw_dsc->src;
        if (task->type == LV_DRAW_TASK_TYPE_LAYER) {
            res = lv_image_decoder_get_info(layer_to_draw->draw_buf, &img_header);
        } else {
            res = lv_image_decoder_get_info(draw_dsc->src, &img_header);
        }
        if (res != LV_RESULT_OK) {
            //LV_LOG_WARN("get image info failed");
            return 0;
        }

        lv_color_format_t img_cf = img_header.cf;

        if (!check_image_cf_is_supported(img_cf)) {     // 判断图像源的颜色格式是否支持
            LV_LOG_ERROR("img_cf not support: img_cf == %x\n", img_cf);
            return 0;
        }

        bool mode_flag;
        lv_jlgpu_get_blend_mode(draw_dsc->blend_mode, &mode_flag);
        if (!mode_flag) {
            LV_LOG_ERROR("blend_mode not support: blend_mode == %x\n", draw_dsc->blend_mode);
            return 0;
        }

        /* not support skew */
        if (draw_dsc->skew_x != 0 || draw_dsc->skew_y != 0) {
            LV_LOG_ERROR("skew not support");
            return 0;
        }

        bool transformed = draw_dsc->rotation != 0 || draw_dsc->scale_x != LV_SCALE_NONE ||
                           draw_dsc->scale_y != LV_SCALE_NONE ? true : false;

        bool masked = draw_dsc->bitmap_mask_src != NULL;

        if (masked) {
            //todo
            LV_LOG_ERROR("masked not support yet");
            return 0;
        }

        lv_color_format_t cf = draw_dsc->header.cf;

        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }
    case LV_DRAW_TASK_TYPE_LABEL: {
#if (LV_USE_DRAW_JLGPU_LABEL_ENABLE == 0)
        return 0;
#endif
        lv_draw_label_dsc_t *draw_dsc = (lv_draw_label_dsc_t *)task->draw_dsc;

        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }
    case LV_DRAW_TASK_TYPE_BORDER: {
#if (LV_USE_DRAW_JLGPU_BORDER_ENABLE == 0)
        return 0;
#endif
        lv_draw_label_dsc_t *draw_dsc = (lv_draw_label_dsc_t *)task->draw_dsc;

        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }

    case LV_DRAW_TASK_TYPE_BOX_SHADOW: {
#if (LV_USE_DRAW_JLGPU_BOX_SHADOW_ENABLE == 0)
        return 0;
#endif
        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }

    case LV_DRAW_TASK_TYPE_LINE: {
#if (LV_USE_DRAW_JLGPU_LINE_ENABLE == 0)
        return 0;
#endif
        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }

    case LV_DRAW_TASK_TYPE_ARC: {
#if (LV_USE_DRAW_JLGPU_ARC_ENABLE == 0)
        return 0;
#endif
        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }

    case LV_DRAW_TASK_TYPE_TRIANGLE: {
#if (LV_USE_DRAW_JLGPU_TRIANGLE_ENABLE == 0)
        return 0;
#endif
        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }

    case LV_DRAW_TASK_TYPE_MASK_RECTANGLE: {
#if (LV_USE_DRAW_JLGPU_MASK_RECTANGLE_ENABLE == 0)
        return 0;
#endif
        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }

    case LV_DRAW_TASK_TYPE_BLUR: {
#if (LV_USE_DRAW_JLGPU_BLUR_ENABLE == 0)
        return 0;
#endif
        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }

#if LV_USE_VECTOR_GRAPHIC
    case LV_DRAW_TASK_TYPE_VECTOR: {
#if (LV_USE_DRAW_JLGPU_VECTOR_ENABLE == 0)
        return 0;
#endif
        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLGPU;
        }
        return 1;
    }
#endif

    default:
        return 0;
    }

    return 0;
}

static int32_t _jlgpu_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{

#if LV_USE_DRAW_JLGPU_LIST == 2
    lv_draw_jlgpu_unit_t *draw_jlgpu_unit = lv_get_draw_jlgpu_unit();
#else
    lv_draw_jlgpu_unit_t *draw_jlgpu_unit = (lv_draw_jlgpu_unit_t *) draw_unit;
#endif

    /* Return immediately if it's busy with draw task. */
    if (draw_jlgpu_unit->task_act) {
        return 0;
    }

    /* Try to get an ready to draw. */
    lv_draw_task_t *t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_JLGPU);

    if (t == NULL || t->preferred_draw_unit_id != DRAW_UNIT_ID_JLGPU) {
        return -1;
    }

    void *buf = lv_draw_layer_alloc_buf(layer);
    if (buf == NULL) {
        return -1;
    }

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    draw_jlgpu_unit->task_act = t;

#if LV_DRAW_JLGPU_ASYNC && LV_USE_OS
    /* Let the render thread work. */
    if (draw_jlgpu_unit->inited) {
        lv_thread_sync_signal(&draw_jlgpu_unit->sync);
    }
#else
    _jlgpu_execute_drawing(draw_jlgpu_unit);

    draw_jlgpu_unit->task_act->state = LV_DRAW_TASK_STATE_FINISHED;
    draw_jlgpu_unit->task_act = NULL;

    /* The draw unit is free now. Request a new dispatching as it can get a new task. */
    lv_draw_dispatch_request();
#endif

    return 1;
}

static int32_t _jlgpu_delete(lv_draw_unit_t *draw_unit)
{

    lv_draw_jlgpu_unit_t *draw_jlgpu_unit = (lv_draw_jlgpu_unit_t *) draw_unit;

    jlgpu_path_destroy(draw_jlgpu_unit->global_path);
    draw_jlgpu_unit->global_path = NULL;

#if LV_DRAW_JLGPU_ASYNC && LV_USE_OS

    LV_LOG_INFO("Cancel JL GPU draw thread.");
    draw_jlgpu_unit->exit_status = true;

    if (draw_jlgpu_unit->inited) {
        lv_thread_sync_signal(&draw_jlgpu_unit->sync);
    }

    lv_result_t res = lv_thread_delete(&draw_jlgpu_unit->thread);

    return res;
#else
    LV_UNUSED(draw_unit);

    return 0;
#endif
}

static void _jlgpu_execute_drawing(lv_draw_jlgpu_unit_t *u)
{
    lv_draw_task_t *t = u->task_act;
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;
    t->draw_unit = (lv_draw_unit_t *)u;

    /* Set target buffer */
    lv_area_t clip_area;
    lv_area_copy(&clip_area, &t->clip_area);
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t draw_area;
    lv_area_copy(&draw_area, &t->area);
    lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);

#if 0
    if (!lv_area_intersect(&draw_area, &draw_area, &clip_area)) {
        return;    /*Fully clipped, nothing to do*/
    }
#endif

    /* Invalidate the drawing area */
    lv_draw_buf_invalidate_cache(draw_buf, &draw_area);

    //特效变换时需要拦截（检查全局状态，避免多unit场景下标志不一致）
    if (g_page_effect_state.effect_busy) {
        return;
    }

    switch (t->type) {
    case LV_DRAW_TASK_TYPE_FILL:
#if (LV_USE_DRAW_JLGPU_FILL_ENABLE == 1)
        /*putchar('f');*/
        lv_draw_jlgpu_fill(t, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_IMAGE:
#if (LV_USE_DRAW_JLGPU_IMG_ENABLE == 1)
        /*putchar('i');*/
        lv_draw_jlgpu_img(t, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_LABEL:
#if (LV_USE_DRAW_JLGPU_LABEL_ENABLE == 1)
        /*putchar('t');*/
        lv_draw_jlgpu_label(t, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_BORDER:
#if (LV_USE_DRAW_JLGPU_BORDER_ENABLE == 1)
        /*putchar('b');*/
        lv_draw_jlgpu_border(t, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_BOX_SHADOW:
        /*putchar('s');*/
#if (LV_USE_DRAW_JLGPU_BOX_SHADOW_ENABLE == 1)
        lv_draw_jlgpu_box_shadow(t, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_LAYER:
#if (LV_USE_DRAW_JLGPU_IMG_ENABLE && LV_USE_DRAW_JLGPU_LAYER_ENABLE)
        lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *)(t->draw_dsc);
        lv_layer_t *layer_to_draw = (lv_layer_t *)draw_dsc->src;
        /*It can happen that nothing was draw on a layer and therefore its buffer is not allocated.
         *In this case just return. */
        if (layer_to_draw->draw_buf == NULL) {
            return;
        }
        if (draw_dsc->bitmap_mask_src) {
            //todo
            return;
        }
        /*The source should be a draw_buf, not a layer*/
        lv_draw_image_dsc_t new_draw_dsc = *draw_dsc;
        new_draw_dsc.src = layer_to_draw->draw_buf;
        lv_image_dsc_t *image = (lv_image_dsc_t *)new_draw_dsc.src;
        /*DcuFlushRegion(image->data, image->data_size);*/
        /*putchar('y');*/

#if LV_USE_DRAW_JLGPU_LIST
        uint8_t *ori_image_data = (uint8_t *)image->data;
        uint8_t *attach_data = lv_jlgpu_attach_buf_list_add(u, (uint8_t *)image->data, image->data_size);
        if (attach_data == NULL) {
            /* 附加buf申请失败:不能把 image->data 置成 NULL 交给GPU(会读到非法纹理地址导致GPU挂死),
             * 直接跳过本层 */
            jlgpu_list_alloc_fail_report(u, image->data_size);
            break;
        }
        image->data = attach_data;
        lv_draw_jlgpu_img(t, &new_draw_dsc, &t->area);
        image->data = ori_image_data;
#else
        lv_draw_jlgpu_img(t, &new_draw_dsc, &t->area);
#endif

#endif
        break;
    case LV_DRAW_TASK_TYPE_LINE:
#if (LV_USE_DRAW_JLGPU_LINE_ENABLE == 1)
        /*putchar('l');*/
        lv_draw_line_iterate(t, t->draw_dsc, lv_draw_jlgpu_line);
#endif
        break;
    case LV_DRAW_TASK_TYPE_ARC:
#if (LV_USE_DRAW_JLGPU_ARC_ENABLE == 1)
        /*putchar('a');*/
        lv_draw_jlgpu_arc(t, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_TRIANGLE:
#if (LV_USE_DRAW_JLGPU_TRIANGLE_ENABLE == 1)
        /*putchar('r');*/
        lv_draw_jlgpu_triangle(t, t->draw_dsc);
#endif
        break;
    case LV_DRAW_TASK_TYPE_MASK_RECTANGLE:
#if (LV_USE_DRAW_JLGPU_MASK_RECTANGLE_ENABLE == 1)
        /*putchar('m');*/
        lv_draw_jlgpu_mask_rect(t, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_BLUR:
#if (LV_USE_DRAW_JLGPU_BLUR_ENABLE == 1)
        /*putchar('r');*/
        printf("LV_DRAW_TASK_TYPE_BLUR todoing\n");
#endif
        break;
#if LV_USE_VECTOR_GRAPHIC
    case LV_DRAW_TASK_TYPE_VECTOR:
#if (LV_USE_DRAW_JLGPU_VECTOR_ENABLE == 1)
        lv_draw_jlgpu_vector(t, t->draw_dsc, &t->area);
#endif
        break;
#endif
    default:
        break;
    }
}

#if LV_DRAW_JLGPU_ASYNC && LV_USE_OS
static void _jlgpu_render_thread_cb(void *ptr)
{
    lv_draw_jlgpu_unit_t *u = ptr;

    lv_thread_sync_init(&u->sync);
    LV_LOG("jlgpu render async thread run\n");
    u->inited = true;

    while (1) {
        /* Wait for sync if there is no task set. */
        while (u->task_act == NULL) {
            if (u->exit_status) {
                break;
            }

            lv_thread_sync_wait(&u->sync);
        }

        if (u->exit_status) {
            LV_LOG_INFO("Ready to exit JL DMA2D draw thread.");
            break;
        }

        _jlgpu_execute_drawing(u);

        /* Signal the ready state to dispatcher. */
        u->task_act->state = LV_DRAW_TASK_STATE_FINISHED;

        /* Cleanup. */
        u->task_act = NULL;

        /* The draw unit is free now. Request a new dispatching as it can get a new task. */
        lv_draw_dispatch_request();
    }

    u->inited = false;
    lv_thread_sync_delete(&u->sync);
    LV_LOG_INFO("Exit JL GPU draw thread.");
}
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_draw_jlgpu_init(void)
{

    os_mutex_create(&jlgpu_mutex);

    lv_draw_jlgpu_unit_t *draw_jlgpu_unit = lv_draw_create_unit(sizeof(lv_draw_jlgpu_unit_t));
    draw_jlgpu_unit->base_unit.evaluate_cb = _jlgpu_evaluate;
    draw_jlgpu_unit->base_unit.dispatch_cb = _jlgpu_dispatch;
    draw_jlgpu_unit->base_unit.delete_cb = _jlgpu_delete;
    draw_jlgpu_unit->base_unit.name = "jlgpu";

    /* JL gpu hw init */
    /*g_jlgpu = jlgpu_create();*/
    /*log_info("jl gpu hw init. >>>>>>>>>>>");*/

#if LV_DRAW_JLGPU_ASYNC && LV_USE_OS
    lv_thread_init(&draw_jlgpu_unit->thread, "lv_draw_jlgpu", LV_THREAD_PRIO_HIGH, _jlgpu_render_thread_cb, 8 * 1024, draw_jlgpu_unit);
#endif

    draw_jlgpu_unit->global_path = jlgpu_path_create();

#if LV_USE_DRAW_JLGPU_LABEL_ENABLE
    //注册freetype事件回调
    lv_draw_jlgpu_label_init(draw_jlgpu_unit);
#endif

#if LV_USE_DRAW_JLGPU_LIST
    //gpu_list init
    if (draw_jlgpu_unit->jlgpu_ctx == NULL) {
        draw_jlgpu_unit->jlgpu_ctx = malloc(sizeof(lv_jlgpu_ctx_t));
        if (draw_jlgpu_unit->jlgpu_ctx == NULL) {
            LV_LOG_ERROR("malloc jlgpu_ctx error");
            return;
        }
    }
    lv_jlgpu_ctx_init(draw_jlgpu_unit->jlgpu_ctx);
    /* 常驻 ctx:不会立刻 uninit,这里立刻还门,避免长期占住推屏独占门 */
    jlgpu_gpu_gate_leave();

    uint8_t *buf = malloc(JLGPU_LIST_MEM_POOL_SIZE);
    if (buf == NULL) {
        LV_LOG_ERROR("malloc buf error");
        return;
    }
    lv_jlgpu_mem_pool_init(draw_jlgpu_unit, buf, JLGPU_LIST_MEM_POOL_SIZE);

    global_draw_jlgpu_unit = draw_jlgpu_unit;

    lv_jlgpu_global_path_init();

#endif

    if (UI_GPU_IRQ_MODE) {
        lv_jlgpu_pnd_sem_init();
    }

#if LV_USE_DRAW_JLGPU_LIST == 2
    lv_draw_jlgpu_unit_t *draw_jlgpu_unit_2 = (lv_draw_jlgpu_unit_t *)malloc(sizeof(lv_draw_jlgpu_unit_t));
    if (draw_jlgpu_unit_2 == NULL) {
        LV_LOG_ERROR("malloc draw_jlgpu_unit_2 error");
        return;
    }
    memcpy(draw_jlgpu_unit_2, draw_jlgpu_unit, sizeof(lv_draw_jlgpu_unit_t));
    draw_jlgpu_unit_2->pool_buf = NULL;

    //需要初始化的部分
    //global_path
    draw_jlgpu_unit_2->global_path = jlgpu_path_create();

#if LV_USE_DRAW_JLGPU_LABEL_ENABLE
    //注册freetype事件回调
    lv_draw_jlgpu_label_init(draw_jlgpu_unit_2);
#endif

    //gpu_list init
    draw_jlgpu_unit_2->jlgpu_ctx = malloc(sizeof(lv_jlgpu_ctx_t));
    if (draw_jlgpu_unit_2->jlgpu_ctx == NULL) {
        LV_LOG_ERROR("malloc jlgpu_ctx_2 error");
        return;
    }
    lv_jlgpu_ctx_init(draw_jlgpu_unit_2->jlgpu_ctx);
    /* 常驻 ctx:同上,立刻还门 */
    jlgpu_gpu_gate_leave();

    uint8_t *buf_2 = malloc(JLGPU_LIST_MEM_POOL_SIZE);
    if (buf_2 == NULL) {
        LV_LOG_ERROR("malloc buf_2 error");
        return;
    }
    lv_jlgpu_mem_pool_init(draw_jlgpu_unit_2, buf_2, JLGPU_LIST_MEM_POOL_SIZE);

    global_draw_jlgpu_unit_2 = draw_jlgpu_unit_2;

    global_draw_jlgpu_unit_1 = draw_jlgpu_unit;
#endif

    return;
}

void lv_draw_jlgpu_deinit(void)
{
    /* JL gpu hw deinit */
    /*jlgpu_free(g_jlgpu);*/
    /*log_info("jl gpu hw deinit.");*/
    /*jlgpu_path_destroy(jlgpu_vec_t *path);*/

    return;
}


#if LV_USE_DRAW_JLGPU_LIST
/**************************gpu链表管理接口****************************************/

//申请buf_size大小的buf,如果src_buf不为空则复制src_buf
uint8_t *lv_jlgpu_attach_buf_list_add(lv_draw_jlgpu_unit_t *unit, uint8_t *src_buf, uint32_t buf_size)
{
    jlgpu_attach_buf_t *new_node = malloc(sizeof(jlgpu_attach_buf_t));
    if (new_node == NULL) {
        LV_LOG_ERROR("malloc new_node error");
        return NULL;
    }
    memset(new_node, 0, sizeof(jlgpu_attach_buf_t));
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE) && (JLGPU_ATTACH_BUF_PSRAM_THRESHOLD > 0)
    if (buf_size > JLGPU_ATTACH_BUF_PSRAM_THRESHOLD) {
        new_node->data = malloc_psram(buf_size);
    } else
#endif
    {
        new_node->data = lv_malloc(buf_size);
    }
    if (new_node->data == NULL) {
        LV_LOG_ERROR("malloc new_node->data error!!!");
        free(new_node);
        return NULL;
    }

    unit->attach_buf_bytes += buf_size;
    if (src_buf) {
        lv_memcpy(new_node->data, src_buf, buf_size);
    }

    if (unit->jlgpu_attach_buf_head == NULL) {
        unit->jlgpu_attach_buf_head = new_node;
    } else {
        jlgpu_attach_buf_t *tail = unit->jlgpu_attach_buf_head;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = new_node;
    }

    return new_node->data;
}

//释放掉所有申请的attach_buf
void lv_jlgpu_attach_buf_list_free(lv_draw_jlgpu_unit_t *unit)
{
    jlgpu_attach_buf_t *free_list = (jlgpu_attach_buf_t *)unit->jlgpu_attach_buf_head;

    while (free_list) {

        if (free_list->data) {
#if (defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE)
            if (psram_cache_check(free_list->data)) {
                free_psram(free_list->data);
            } else
#endif
            {
                lv_free(free_list->data);
            }
        }

        jlgpu_attach_buf_t *free_list_temp = free_list->next;
        free(free_list);
        free_list = free_list_temp;
    }
    /*printf("-----unit->attach_buf_bytes = %d\n",unit->attach_buf_bytes);*/
    unit->attach_buf_bytes = 0;
    unit->jlgpu_attach_buf_head = NULL;
}

//释放GPU任务链表
void lv_jlgpu_task_list_free(lv_draw_jlgpu_unit_t *unit)
{
    if (unit->jlgpu_ctx->task_base) {
        unit->jlgpu_ctx->task_base = NULL;
        unit->jlgpu_ctx->cur_task  = NULL;
        unit->jlgpu_ctx->jlgpu_task_num = 0;
    } else {
        /*printf("wraning!! lv_gpu_task_free: no gpu_task_head\n");*/
    }
    /* 快照依附于这条任务链, 链没了快照也必须一起清掉, 否则下一帧收集会以为还有旧快照 */
    lv_jlgpu_task_list_snapshot_free(unit->jlgpu_ctx);
    /*mem_stats();*/
    lv_jlgpu_attach_buf_list_free(unit);
    lv_jlgpu_mem_pool_clean(unit);
    /*mem_stats();*/
}

void lv_jlgpu_global_path_init(void)
{
    uint32_t bg_width = LCD_WIDTH;

    if (jlgpu_global_path) {
        LV_LOG_WARN("jlgpu_global_path has inited");
        return;
    } else {
        jlgpu_global_path = (jlgpu_global_path_t *)malloc(sizeof(jlgpu_global_path_t));
        if (jlgpu_global_path) {
            memset(jlgpu_global_path, 0, sizeof(jlgpu_global_path_t));
        } else {
            LV_LOG_ERROR("malloc err");
            return;
        }
    }

    int edge_num = 1024;//temp
    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;
    jlgpu_global_path->cbuf_split = 0;
    jlgpu_global_path->cbuf_stride = bg_width;//!
    jlgpu_global_path->max_bez_div = global_max_bez_div;
    jlgpu_global_path->max_edge_cfg = edge_num;
    jlgpu_global_path->msaa_lvl = JLGPU_RENDER_QUALITY;

    int mask_h = jlgpu_global_path->cbuf_split ? 16 : 32;
    uint8_t *cbuf = (uint8_t *)malloc(mask_h * bg_width);
    if (cbuf == NULL) {
        LV_LOG_ERROR("malloc cbuf error");
        free(jlgpu_global_path);
        jlgpu_global_path = NULL;
        return;
    }
    memset(cbuf, 0, mask_h * bg_width);
    uint8_t *ebuf = (uint8_t *)malloc(8 * edge_num); //edge 2word
    if (ebuf == NULL) {
        LV_LOG_ERROR("malloc ebuf error");
        free(cbuf);
        free(jlgpu_global_path);
        jlgpu_global_path = NULL;
        return;
    }
    memset(ebuf, 0, 8 * edge_num);

    jlgpu_global_path->c_data = cbuf;
    jlgpu_global_path->e_data = ebuf;

    jlgpu_set_global_path_param(jlgpu_global_path);
}

void lv_jlgpu_global_path_uninit(void)
{
    if (jlgpu_global_path == NULL) {
        LV_LOG_WARN("jlgpu_global_path has uninited");
        return;
    }

    if (jlgpu_global_path->c_data) {
        free(jlgpu_global_path->c_data);
    }

    if (jlgpu_global_path->e_data) {
        free(jlgpu_global_path->e_data);
    }

    free(jlgpu_global_path);
    jlgpu_global_path = NULL;
}

void lv_jlgpu_global_path_reset(void)
{
    if (jlgpu_global_path) {
        jlgpu_set_global_path_param(jlgpu_global_path);
    }
}

//初始化gpu链表的sfr,初始化gpu
void lv_jlgpu_sfr_set(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint32_t bg_width, uint8_t *out_buf, uint8_t out_format)
{
    jlgpu_surface_t surface = {0};

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
    surface.data = (u8 *)  NO_CACHE_ADDR(out_buf);
#else
    surface.data = out_buf;
#endif
    surface.window.x_min = window->x1;
    surface.window.x_max = window->x2 + 1;
    surface.window.y_min = window->y1;
    surface.window.y_max = window->y2 + 1;

    surface.format = out_format;
    surface.width  = bg_width;

    jlgpu_set_win_param(&surface);
    lv_jlgpu_global_path_reset();
}

void lv_jlgpu_sfr_top_set(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint16_t pingpong_buf_w, uint16_t pingpong_buf_h, uint8_t *pingpong_buf0, uint8_t *pingpong_buf1)
{

    if (pingpong_buf_h == 0 && pingpong_buf0 == NULL && pingpong_buf1 == NULL) {
        jlgpu_hw_set_sfr_top(NULL);
        return;

    }
    JLGPU_TOP_INFO_TypeDef sfr_top = {0};

    sfr_top.top_ofs.top_x_ofs = window->x1;
    sfr_top.top_ofs.top_y_ofs = window->y1;
    sfr_top.top_info.top_w = lv_area_get_width(window);
    sfr_top.top_info.top_h = lv_area_get_height(window);
    sfr_top.buf_info.buf_w = pingpong_buf_w;
    sfr_top.buf_info.buf_h = pingpong_buf_h;
    sfr_top.buf_base0.buf_base0 = (uint32_t)pingpong_buf0;
    sfr_top.buf_base1.buf_base1 = (uint32_t)pingpong_buf1;
    jlgpu_hw_set_sfr_top(&sfr_top);
}

void lv_jlgpu_list_uninit(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint8_t *out_buf, uint8_t out_format)
{
    if (unit->jlgpu_ctx) {
        /* 常驻 ctx 的 init 已配平还门,这里补占门以匹配 ctx_uninit 里的 gate_leave */
        jlgpu_gpu_gate_enter();
        lv_jlgpu_ctx_uninit(unit->jlgpu_ctx);
    }
    lv_jlgpu_task_list_free(unit);
}

uint8_t lv_jlgpu_get_gxdma_bpp(uint8_t out_format)
{
    if (out_format == 0xf) {
        return 16;
    }
    return jlgpu_get_format_bpp(out_format);
}

void lv_jlgpu_gxdma_sfr_set(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint16_t in_stride, uint8_t *out_buf, uint8_t in_format, uint8_t out_format, uint8_t out_jstc_en, uint8_t out_rotate)
{

    os_mutex_pend(&jlgpu_mutex, 0);

    JLGXDMA_link_gpu(1);
    //GXDMA Setting
    gxdma_reg_param_t gxdma_param = {0};
    JLGXDMA_init();
    /* JLGXDMA_irq_init(); */
    gxdma_param.src_fmt = in_format;
    gxdma_param.dst_fmt = out_format;
    gxdma_param.rot_mode = out_rotate;
    int out_w = lv_area_get_width(window);
    int out_h = lv_area_get_height(window);

    u8 rotation = is_gxdma_rotate(gxdma_param.rot_mode);

    gxdma_param.jstc_en = out_jstc_en; //JSTC压缩

    gxdma_param.dither_en = 1;
    gxdma_param.out_adr_mode = 1;
    if (gxdma_param.jstc_en) {
        gxdma_param.jstc_mode = GXDMA_JSTC_MODE_RGB888; //0:A压缩 1:RGB压缩 2:ARGB压缩
        gxdma_param.gray_th = 5;
        //JSTC 压缩只支持ARGB8888/RGB888 tile格式
        gxdma_param.out_adr_mode = 0; //tile
        gxdma_param.dst_fmt = JLGPU_FORMAT_ARGB8888; //JSTC压缩输出格式只能是ARGB8888
        gxdma_param.out_stride = (rotation ? out_h : out_w) / 8 * ((gxdma_param.jstc_mode == 2) ? 96 : 64);
    }
    if (gxdma_param.rot_mode) {
        gxdma_param.out_adr_mode = 0;
    }
    gxdma_param.dst = out_buf;
    if (gxdma_param.dst_fmt == JLGPU_FORMAT_YUV420_BT601 || gxdma_param.dst_fmt == JLGPU_FORMAT_YUV420_BT709) {

        gxdma_param.dst_uv = out_buf + out_w * out_h;
    }
    /* printf("gxdma_param.dst=0x%x dmm_src0_base=%x\n", (uint32_t)gxdma_param.dst, EVA_DMM->IN0_BASE); */
    gxdma_param.width = out_w;
    gxdma_param.height = out_h;
    gxdma_param.in_stride = in_stride;
    if (gxdma_param.jstc_en == 0) {
        if (gxdma_param.out_adr_mode) {
            gxdma_param.out_stride = out_w * (jlgpu_get_format_bpp(out_format) / 8);
        } else {
            //tile
            gxdma_param.out_stride = (rotation ? out_h : out_w) * jlgpu_get_format_bpp(out_format) / 8 * ((out_format <= 0x0e) ? 8 : 16);
        }
    }

    JLGXDMA_set_param(&gxdma_param);
    JLGXDMA_set_src_area(0, 0, 0, 0);

    os_mutex_post(&jlgpu_mutex);
}

//仅配置窗口与设置输出格式
void lv_jlgpu_sfr_set_surface(lv_draw_jlgpu_unit_t *unit, lv_area_t *window, uint8_t *out_buf, uint8_t out_format)
{
    jlgpu_surface_t surface;
    memset(&surface, 0, sizeof(surface));

    surface.data = out_buf;
    surface.window.x_min = window->x1;
    surface.window.x_max = window->x2 + 1;
    surface.window.y_min = window->y1;
    surface.window.y_max = window->y2 + 1;

    uint32_t fb_width = lv_area_get_width(window);

    surface.format = out_format;
    surface.width  = fb_width;

    os_mutex_pend(&jlgpu_mutex, 0);

    jlgpu_set_params(unit->jlgpu_ctx->ctx, &surface, 0, 0, 0, 0);

    os_mutex_post(&jlgpu_mutex);

}


//执行GPU任务链
void lv_jlgpu_task_list_run(lv_draw_jlgpu_unit_t *unit)
{
    /* 启动GPU合成 */
    /*printf("-----jlgpu_task_num = %d\n", unit->jlgpu_ctx->jlgpu_task_num);*/
    if (unit->jlgpu_ctx->task_base && unit->jlgpu_ctx->jlgpu_task_num != 0) {
        /* jlgpu_dumcp_all_tasks(unit->jlgpu_ctx->task_base); */
        jlgpu_run(unit->jlgpu_ctx->ctx, unit->jlgpu_ctx->task_base);
    }
}

/*设置GPU任务链上所有节点的全局透明度。
 * 与 lib 侧 gpu_hw.h 的 TASK_SET_GLO_ALPHA 一致: 指令字 bit24..31, 范围 0~128, 128 为不透明。
 * 注意: 倒影类特效的镜像槽与本体槽复用同一条任务链, 必须在每次重放前重新设置,
 * 否则后跑的本体槽会沿用镜像槽的暗化值。*/
void lv_jlgpu_task_list_set_global_alpha(lv_jlgpu_ctx_t *jlgpu_ctx, uint8_t alpha)
{
    if (jlgpu_ctx == NULL) {
        return;
    }
    if (alpha > 128) {
        alpha = 128;
    }
    void *task = jlgpu_ctx->task_base;
    while (task) {
        SFR(((uint32_t *)task)[0], 24, 8, alpha);
        task = jlgpu_task_get_next(task);
    }
}

/*设置GPU任务链上所有节点的混合模式。
 * 与 lib 侧 gpu_hw.h 的 TASK_SET_BLEND_MODE 一致: 指令字 bit6..9, 取值见 jlgpu_blendmode_t。
 * 用途: 页面特效需要"两页交叉淡入淡出"时, 被淡入/淡出的那一页必须走 SRC_OVER 才会与
 * 底图做 alpha 混合。而 lv_obj_redraw 收集全屏图层走的是 lv_draw_jlgpu_fill.c 的列表分支,
 * 那里写死 JLGPU_BLEND_SRC(直接覆盖), 只改 global_alpha 看不到任何渐变效果。
 * 任务链是常驻复用的, 用完必须恢复成收集时的模式, 否则会影响后续帧。*/
void lv_jlgpu_task_list_set_blend_mode(lv_jlgpu_ctx_t *jlgpu_ctx, jlgpu_blendmode_t blend_mode)
{
    if (jlgpu_ctx == NULL) {
        return;
    }
    void *task = jlgpu_ctx->task_base;
    while (task) {
        SFR(((uint32_t *)task)[0], 6, 4, blend_mode);
        task = jlgpu_task_get_next(task);
    }
}

/* 快照里每一项的编码: bit0..3 = 混合模式, bit8..15 = 全局透明度 */
#define JLGPU_SNAP_BLEND_GET(v) ((uint8_t)((v) & 0x0F))
#define JLGPU_SNAP_ALPHA_GET(v) ((uint8_t)(((v) >> 8) & 0xFF))
#define JLGPU_SNAP_PACK(blend, alpha) ((uint16_t)(((blend) & 0x0F) | (((uint16_t)(alpha) & 0xFF) << 8)))
/* SFR 只提供了写宏, 读位域自己来: 取 cmd 的 [shift, shift+width) 位 */
#define JLGPU_SFR_GET(cmd, shift, width) ((uint32_t)(((cmd) >> (shift)) & ((1u << (width)) - 1u)))

/* 记下任务链上每个节点当前的(混合模式, 全局透明度)。
 * 整链覆盖式改写是不可逆的: 链上各节点的值本来互不相同(文字 SRC_OVER/128、图片按 dsc->blend_mode、
 * 透明度取 dsc->opa/2), 被页级特效统一覆盖后就没法再逐节点恢复。 */
void lv_jlgpu_task_list_snapshot(lv_jlgpu_ctx_t *jlgpu_ctx)
{
    if (jlgpu_ctx == NULL || jlgpu_ctx->task_base == NULL) {
        return;
    }
    if (jlgpu_ctx->node_snap != NULL) {
        /* 已有快照: 说明链还是收集时那条, 不能拿被改写过的值覆盖掉原值 */
        return;
    }
    uint8_t node_num = jlgpu_ctx->jlgpu_task_num;
    if (node_num == 0) {
        return;
    }
    uint16_t *snap = (uint16_t *)lv_malloc((uint32_t)node_num * sizeof(uint16_t));
    if (snap == NULL) {
        return;
    }
    void *task = jlgpu_ctx->task_base;
    uint8_t i = 0;
    while (task && i < node_num) {
        uint32_t cmd = ((uint32_t *)task)[0];
        snap[i] = JLGPU_SNAP_PACK(JLGPU_SFR_GET(cmd, 6, 4), JLGPU_SFR_GET(cmd, 24, 8));
        task = jlgpu_task_get_next(task);
        i++;
    }
    jlgpu_ctx->node_snap = snap;
    jlgpu_ctx->node_snap_num = i;
}

/* 按快照逐节点还原。任务链节点数在特效期间不会变, 所以顺序一一对应。 */
void lv_jlgpu_task_list_restore(lv_jlgpu_ctx_t *jlgpu_ctx)
{
    if (jlgpu_ctx == NULL || jlgpu_ctx->node_snap == NULL) {
        return;
    }
    void *task = jlgpu_ctx->task_base;
    uint8_t i = 0;
    while (task && i < jlgpu_ctx->node_snap_num) {
        uint16_t v = jlgpu_ctx->node_snap[i];
        SFR(((uint32_t *)task)[0], 6, 4, JLGPU_SNAP_BLEND_GET(v));
        SFR(((uint32_t *)task)[0], 24, 8, JLGPU_SNAP_ALPHA_GET(v));
        task = jlgpu_task_get_next(task);
        i++;
    }
}

/* 与页级透明度相乘: 节点最终 alpha = 收集时的 alpha * scale / 128。
 * 直接 set_global_alpha 会把元素自身的 dsc->opa/2 一起冲掉, 页级调光应当用这个。 */
void lv_jlgpu_task_list_scale_global_alpha(lv_jlgpu_ctx_t *jlgpu_ctx, uint8_t scale)
{
    if (jlgpu_ctx == NULL) {
        return;
    }
    if (scale > 128) {
        scale = 128;
    }
    void *task = jlgpu_ctx->task_base;
    uint8_t i = 0;
    while (task) {
        uint32_t cmd = ((uint32_t *)task)[0];
        /* 有快照就按"收集时的透明度"乘, 每帧调用都是幂等的。
         * 没有快照(分配失败等)只能退化成绝对值: 读当前值再乘会在每帧调用下逐帧累乘,
         * 几帧之内就把整页乘成全黑, 比不生效严重得多。 */
        uint8_t base = JLGPU_MAX_GLOBAL_ALPHA;
        if (jlgpu_ctx->node_snap != NULL) {
            if (i < jlgpu_ctx->node_snap_num) {
                base = JLGPU_SNAP_ALPHA_GET(jlgpu_ctx->node_snap[i]);
            } else {
                break;
            }
        } else {
            base = JLGPU_SFR_GET(cmd, 24, 8);
            if (base > scale) {
                /* 退化成"取较小值"而非乘, 保证多帧调用不会累乘 */
                SFR(((uint32_t *)task)[0], 24, 8, scale);
                task = jlgpu_task_get_next(task);
                i++;
                continue;
            }
        }
        SFR(((uint32_t *)task)[0], 24, 8, (uint8_t)(((uint32_t)base * scale) / 128));
        task = jlgpu_task_get_next(task);
        i++;
    }
}

/* 只把"收集时就是 JLGPU_BLEND_SRC(直接覆盖)"的节点改成 SRC_OVER, 其余节点保持原样。
 * 页级交叉淡入淡出需要被淡的那页半透明地叠在底图上, 而填充类节点收集时写死 SRC, 不改就看不到渐变;
 * 但文字(SRC_OVER)和按 dsc->blend_mode 走的图片节点本来就参与混合, 必须原样保留。 */
void lv_jlgpu_task_list_set_blend_src_over(lv_jlgpu_ctx_t *jlgpu_ctx)
{
    if (jlgpu_ctx == NULL) {
        return;
    }
    void *task = jlgpu_ctx->task_base;
    uint8_t i = 0;
    while (task) {
        uint32_t cmd = ((uint32_t *)task)[0];
        uint8_t cur = JLGPU_SFR_GET(cmd, 6, 4);
        uint8_t orig = cur;
        if (jlgpu_ctx->node_snap != NULL && i < jlgpu_ctx->node_snap_num) {
            orig = JLGPU_SNAP_BLEND_GET(jlgpu_ctx->node_snap[i]);
        }
        if (orig == JLGPU_BLEND_SRC) {
            SFR(((uint32_t *)task)[0], 6, 4, JLGPU_BLEND_SRC_OVER);
        }
        task = jlgpu_task_get_next(task);
        i++;
    }
}

void lv_jlgpu_task_list_snapshot_free(lv_jlgpu_ctx_t *jlgpu_ctx)
{
    if (jlgpu_ctx == NULL) {
        return;
    }
    if (jlgpu_ctx->node_snap != NULL) {
        lv_free(jlgpu_ctx->node_snap);
        jlgpu_ctx->node_snap = NULL;
    }
    jlgpu_ctx->node_snap_num = 0;
}

//添加GPU任务链表节点
void lv_jlgpu_add_task_node(lv_draw_jlgpu_unit_t *unit, void *new_task)
{
    if (unit->jlgpu_ctx->task_base) {
        jlgpu_task_set_next(unit->jlgpu_ctx->cur_task, new_task);
    } else {
        unit->jlgpu_ctx->task_base = new_task;
    }

    unit->jlgpu_ctx->jlgpu_task_num++;
    unit->jlgpu_ctx->cur_task = new_task;
}

//将新任务节点插入到 GPU 任务链表的头部
void lv_jlgpu_insert_task_head(lv_draw_jlgpu_unit_t *unit, void *new_task)
{
    if (!unit || !unit->jlgpu_ctx || !new_task) {
        return;
    }

    lv_jlgpu_ctx_t *ctx = unit->jlgpu_ctx;
    jlgpu_task_set_next(new_task, ctx->task_base);
    ctx->task_base = new_task;
    ctx->jlgpu_task_num++;

    if (ctx->jlgpu_task_num == 1) {
        ctx->cur_task = new_task;
    }
}

/* 取链表里 node 的前驱节点; node 是头节点 / 不在链表上 / 链表为空时返回 NULL */
static void *jlgpu_task_list_prev(lv_jlgpu_ctx_t *ctx, void *node)
{
    if (ctx->task_base == NULL || node == NULL || node == ctx->task_base) {
        return NULL;
    }

    void *prev = ctx->task_base;
    while (prev && jlgpu_task_get_next(prev) != node) {
        prev = jlgpu_task_get_next(prev);
    }
    return prev;
}

//取 GPU 任务链表的节点个数
uint16_t lv_jlgpu_task_list_count(lv_draw_jlgpu_unit_t *unit)
{
    if (!unit || !unit->jlgpu_ctx) {
        return 0;
    }
    return unit->jlgpu_ctx->jlgpu_task_num;
}

//按层号取 GPU 任务链表节点: layer=0 是链表头(最先画=最底层), 越界返回 NULL
void *lv_jlgpu_task_list_get(lv_draw_jlgpu_unit_t *unit, uint16_t layer)
{
    if (!unit || !unit->jlgpu_ctx) {
        return NULL;
    }

    void *task = unit->jlgpu_ctx->task_base;
    while (task && layer > 0) {
        task = jlgpu_task_get_next(task);
        layer--;
    }
    return task;
}

//在 anchor_task 之前插入新任务节点(新节点画在 anchor 之下)。
//anchor_task == NULL 表示"插到链表末尾之后", 等价于 lv_jlgpu_add_task_node()。
//anchor_task 不在链表上时什么都不做, 避免把链表接坏。
void lv_jlgpu_insert_task_before(lv_draw_jlgpu_unit_t *unit, void *anchor_task, void *new_task)
{
    if (!unit || !unit->jlgpu_ctx || !new_task) {
        return;
    }

    lv_jlgpu_ctx_t *ctx = unit->jlgpu_ctx;

    if (anchor_task == NULL) {
        lv_jlgpu_add_task_node(unit, new_task);
        return;
    }
    if (ctx->task_base == NULL) {
        return;
    }
    /* anchor 就是头节点: 没有前驱, 等价于前插 */
    if (ctx->task_base == anchor_task) {
        lv_jlgpu_insert_task_head(unit, new_task);
        return;
    }

    void *prev = jlgpu_task_list_prev(ctx, anchor_task);
    if (prev == NULL) {
        return;
    }

    jlgpu_task_set_next(new_task, anchor_task);
    jlgpu_task_set_next(prev, new_task);
    ctx->jlgpu_task_num++;
}

//在 anchor_task 之后插入新任务节点(新节点画在 anchor 之上)。
//anchor_task == NULL 表示"插到链表头之前", 等价于 lv_jlgpu_insert_task_head()。
void lv_jlgpu_insert_task_after(lv_draw_jlgpu_unit_t *unit, void *anchor_task, void *new_task)
{
    if (!unit || !unit->jlgpu_ctx || !new_task) {
        return;
    }

    lv_jlgpu_ctx_t *ctx = unit->jlgpu_ctx;

    if (anchor_task == NULL) {
        lv_jlgpu_insert_task_head(unit, new_task);
        return;
    }

    /* 找不到 anchor 就不动, 免得把节点插到别处去 */
    bool found = false;
    for (void *t = ctx->task_base; t; t = jlgpu_task_get_next(t)) {
        if (t == anchor_task) {
            found = true;
            break;
        }
    }
    if (!found) {
        return;
    }

    jlgpu_task_set_next(new_task, jlgpu_task_get_next(anchor_task));
    jlgpu_task_set_next(anchor_task, new_task);
    ctx->jlgpu_task_num++;

    /* anchor 原本是尾节点的话, 尾指针要跟着挪 */
    if (ctx->cur_task == anchor_task) {
        ctx->cur_task = new_task;
    }
}

//按层号插入新任务节点: layer=0 插到链表头(最底层), layer>=节点数则追加到链表尾(最上层)。
//注意层号是"当帧"的: 链表节点数每帧都可能变, 跨帧不保证指向同一个 UI 元素。
void lv_jlgpu_insert_task_at(lv_draw_jlgpu_unit_t *unit, uint16_t layer, void *new_task)
{
    if (!unit || !unit->jlgpu_ctx || !new_task) {
        return;
    }

    lv_jlgpu_ctx_t *ctx = unit->jlgpu_ctx;

    if (layer == 0 || ctx->task_base == NULL) {
        lv_jlgpu_insert_task_head(unit, new_task);
        return;
    }

    /* 走到第 layer-1 个节点(0 起算)的后面; layer 超过节点数时停在尾节点, 效果就是追加 */
    void *node = ctx->task_base;
    for (uint16_t i = 0; (i + 1) < layer && jlgpu_task_get_next(node) != NULL; i++) {
        node = jlgpu_task_get_next(node);
    }

    jlgpu_task_set_next(new_task, jlgpu_task_get_next(node));
    jlgpu_task_set_next(node, new_task);
    ctx->jlgpu_task_num++;

    if (ctx->cur_task == node) {
        ctx->cur_task = new_task;
    }
}

lv_draw_jlgpu_unit_t *lv_get_draw_jlgpu_unit(void)
{
    return global_draw_jlgpu_unit;
}

void lv_jlgpu_mem_pool_init(lv_draw_jlgpu_unit_t *unit, uint8_t *pool_buf, uint16_t buf_size)
{
    if (unit->pool_buf) {
        LV_LOG_ERROR("pool_buf not NULL");
    } else {
        unit->pool_buf = pool_buf;
        if (unit->pool_buf == NULL) {
            LV_LOG_ERROR("pool_buf malloc failed");
            return;
        }
        unit->pool_size = buf_size;
        unit->pool_pos = 0;
    }
}

void lv_jlgpu_mem_pool_uninit(lv_draw_jlgpu_unit_t *unit)
{
    unit->pool_buf = NULL;
    unit->pool_size = 0;
    unit->pool_pos = 0;
}

uint8_t *lv_jlgpu_mem_pool_get_buf(lv_draw_jlgpu_unit_t *unit, uint8_t *src_buf, uint16_t buf_size)
{

    if (unit->pool_buf == NULL || unit->pool_size == 0) {
        LV_LOG_ERROR("pool_buf not init");
        return NULL;
    }

    unit->pool_pos = ALIGN_UP_4(unit->pool_pos);

    if ((unit->pool_pos + buf_size) < unit->pool_size) {
        uint8_t *ptr = unit->pool_buf + unit->pool_pos;
        if (src_buf) {
            memcpy(ptr, src_buf, buf_size);
        }
        unit->pool_pos += buf_size;
        return ptr;
    } else {
        uint8_t *ptr = lv_jlgpu_attach_buf_list_add(unit, src_buf, buf_size);
        if (ptr == NULL) {
            /* 内存池已满且堆也申请失败:调用方拿到 NULL 后如果直接 memset/交给GPU,
             * 会表现为静默黑屏或GPU挂死,所以在这里把上下文打出来 */
            jlgpu_list_alloc_fail_report(unit, buf_size);
        }
        return ptr;
    }
}

void lv_jlgpu_mem_pool_clean(lv_draw_jlgpu_unit_t *unit)
{
    /*printf("-----unit->pool_pos = %d\n",unit->pool_pos); */
    unit->pool_pos = 0;
}

#endif

//判断是否是显存
bool lv_is_draw_buf(uint8_t *buf)
{

    lv_display_t *disp_refr = lv_refr_get_disp_refreshing();

    if (disp_refr == NULL) {
        return false;
    }

    if (disp_refr->buf_1 == NULL && disp_refr->buf_2 == NULL) {
        return false;
    }

    if (disp_refr->buf_1) {
        if (buf == disp_refr->buf_1->data) {
            return true;
        }
    }

    if (disp_refr->buf_2) {
        if (buf == disp_refr->buf_2->data) {
            return true;
        }
    }

    if (disp_refr->buf_3) {
        if (buf == disp_refr->buf_3->data) {
            return true;
        }
    }

    return false;
}

#include "asm/power_interface.h"
u8 lv_gpu_hw_get_status(void)
{
    if (!gpu_init_sta) {
        return 0;
    }
    return ((GPU_SFR->STA_CON >> 2) & 0x03);
}
void lv_sfc_wait_gpu_idle(u32 id)
{
    while (lv_gpu_hw_get_status());
}

u8 lv_wait_gpu_idle(void)
{
    lv_sfc_wait_gpu_idle(0);
    return 1;
}

REGISTER_MEMORY_DEFRAG_REQUEST(lv_gpu_driver_defrag) = {
    .name = "lvgpu",
    .wait_is_idle = lv_wait_gpu_idle,
};

REGISTER_TZSPI_TARGET(lv_gpu_tzspi) = {
    .suspend = lv_sfc_wait_gpu_idle,
};
static u8 lv_gpu_enter_deepsleep(void)
{
    gpu_init_sta = 0;
    return 0;
}
#if 0
AT_VOLATILE_CACHE_CODE_LOWPOWER
u8 lv_gpu_exit_deepsleep(void)
{
    return 0;
}
#endif
DEEPSLEEP_TARGET_REGISTER(lv_gpu) = {
    .name   = "lvgpu",
    .enter  = lv_gpu_enter_deepsleep,
    .exit   = NULL,
};

#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
