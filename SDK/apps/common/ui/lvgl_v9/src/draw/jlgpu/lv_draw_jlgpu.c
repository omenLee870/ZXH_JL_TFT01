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

#if CONFIG_LVGL_UI_ENABLE


#if LV_USE_DRAW_JLGPU

/*********************
 *      DEFINES
 *********************/

#define DRAW_UNIT_ID_JLGPU 102
#define ALIGN_UP_4(x) (((x) + 3) & ~3)

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

static int jlgpu_hw_init_hal(void)
{
    os_mutex_pend(&jlgpu_mutex, 0);

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

    os_mutex_post(&jlgpu_mutex);

    return 0;
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

    if (jlgpu_ctx == NULL) {
        LV_LOG_ERROR("lv_jlgpu_init error");
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
    }
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
        image->data = lv_jlgpu_attach_buf_list_add(u, (uint8_t *)image->data, image->data_size);
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
        return lv_jlgpu_attach_buf_list_add(unit, src_buf, buf_size);
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

#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
