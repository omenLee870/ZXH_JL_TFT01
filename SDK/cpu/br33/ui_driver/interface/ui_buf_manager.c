#include "system/includes.h"
#include "app_config.h"
#include "ui_core.h"
#include "control.h"
#include "asm/psram_api.h"

//UI框架RAM统计使能
/* #define UI_BUF_DEBUG						//内存debug */
/* #define UI_BUF_CALC_LITE 				//轻量级统计 */
/* #define UI_BUF_CALC						//分模块统计 */
#define LOG_TAG_CONST       UI_BUF
#define LOG_TAG     		"[ui_buf]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_UI_ENABLE


extern size_t xPortGetPhysiceMemorySize(void);
extern const u32 config_ui_alloc_psram_mod_sel;

/***************************************************************/
//			UI_BUF_CALC
/***************************************************************/


#ifdef UI_BUF_CALC
struct buffer_calc {
    struct list_head list;
    u8 *buf;
    int size;
    u32 ram_type;
    u32 module_type;
};
static struct buffer_calc buffer_used = {
    .list = {
        .next = &buffer_used.list,
        .prev = &buffer_used.list,
    },
};
static int jlui_malloc_cnt = 0;

/* RAM 类型名 */
const static char *ui_ram_type_name[] = {
    "sram",
    "param",
    "cache",
};

/* 统计各模块内存占用数量 */
struct ui_module_name_def {
    char name[24];	// 模块名称
    int module;		// 模块ID
    int sram;		// 模块申请sram数量
    int other_ram;	// psram or cache
    int count;		// 模块申请内存次数
};
#define WIDGET(type)	((type << 16) | UI_MODULE_WIDGET)

static struct ui_module_name_def ui_module_name[] = {
    {"ui_core",			UI_MODULE_CORE,						0, 0, 0},
    {"ui_frame",		UI_MODULE_FRAME,					0, 0, 0},
    {"ui_resource", 	UI_MODULE_RESOURCE,					0, 0, 0},
    {"ui_gpu",			UI_MODULE_GPU,						0, 0, 0},
    {"ui_dbi",			UI_MODULE_DBI,						0, 0, 0},
    {"ui_font",			UI_MODULE_FONT,						0, 0, 0},
    {"buf_manager", 	UI_MODULE_BUFFER,					0, 0, 0},
    {"gpu_cache", 		UI_MODULE_CACHE,					0, 0, 0},
    {"jpeg  ",			UI_MODULE_JPEG,						0, 0, 0},
    {"window",			WIDGET(CTRL_TYPE_WINDOW),			0, 0, 0},
    {"layout",			WIDGET(CTRL_TYPE_LAYOUT),			0, 0, 0},
    {"layer",			WIDGET(CTRL_TYPE_LAYER),			0, 0, 0},
    {"grid  ",			WIDGET(CTRL_TYPE_GRID),				0, 0, 0},
    {"list   ",			WIDGET(CTRL_TYPE_LIST),				0, 0, 0},
    {"button",			WIDGET(CTRL_TYPE_BUTTON),			0, 0, 0},
    {"pic   ",			WIDGET(CTRL_TYPE_PIC),				0, 0, 0},
    {"battery",			WIDGET(CTRL_TYPE_BATTERY),			0, 0, 0},
    {"time  ",			WIDGET(CTRL_TYPE_TIME),				0, 0, 0},
    {"camera",			WIDGET(CTRL_TYPE_CAMERA_VIEW), 		0, 0, 0},
    {"text  ",			WIDGET(CTRL_TYPE_TEXT),				0, 0, 0},
    {"animation",		WIDGET(CTRL_TYPE_ANIMATION), 		0, 0, 0},
    {"player",			WIDGET(CTRL_TYPE_PLAYER),			0, 0, 0},
    {"number",			WIDGET(CTRL_TYPE_NUMBER),			0, 0, 0},
    {"progress",		WIDGET(CTRL_TYPE_PROGRESS),		 	0, 0, 0},
    {"multiprogress", 	WIDGET(CTRL_TYPE_MULTIPROGRESS), 	0, 0, 0},
    {"watch",			WIDGET(CTRL_TYPE_WATCH), 			0, 0, 0},
    {"slider",			WIDGET(CTRL_TYPE_SLIDER),			0, 0, 0},
    {"vslider",			WIDGET(CTRL_TYPE_VSLIDER),			0, 0, 0},
    {"compass",			WIDGET(CTRL_TYPE_COMPASS),			0, 0, 0},
};

static void ui_module_ram_change(int ram_type, int module_type, int size, int add)	// add 表示size符号，+1/-1
{
    int i;
    int operator = ((add > 0) ? (1) : (-1));

    for (i = 0; i < ARRAY_SIZE(ui_module_name); i++) {
        if (ui_module_name[i].module == module_type) {
            if (ram_type == UI_RAM_SRAM) {
                ui_module_name[i].sram += (operator * size);
            } else {
                ui_module_name[i].other_ram += (operator * size);
            }
            ui_module_name[i].count += (operator);
            printf("[BUF_CALC]=====>>> %s %s sram %d,other_ram:%d total: %d, count: %d\n",
                   ui_module_name[i].name, ((operator > 0) ? "malloc" : "free"), size, ui_module_name[i].sram, ui_module_name[i].other_ram, ui_module_name[i].count);
            return;
        }
    }
    printf("[BUF_CALC]=====>>> unknow module type: %d, size: %d, add: %d\n", module_type, size, add);
}

void ui_module_ram_dump()
{
    int i;
    for (i = 0; i < ARRAY_SIZE(ui_module_name); i++) {
        printf("[BUF_CALC]=====>>> module: %s \t sram_size: %d \t other_ram:%d \t  count: %d\n",
               ui_module_name[i].name, ui_module_name[i].sram, ui_module_name[i].other_ram, ui_module_name[i].count);
    }
}

void ui_buf_calc_add(void *buf, int size, u32 ram_type, u32 module_type)
{
    jlui_malloc_cnt += 1;
    struct buffer_calc *new = (struct buffer_calc *)malloc(sizeof(struct buffer_calc));
    new->buf = buf;
    new->size = size;
    new->ram_type = ram_type;
    new->module_type = module_type;

    list_add_tail((struct list_head *)new, (struct list_head *)&buffer_used);
    /* 打印申请的buf地址，buf大小，buf类型，申请模块 */
    printf("platform_malloc : 0x%x, size: %d, ram: %s\n", (int)buf, size, ui_ram_type_name[ram_type]);
    int real_type = -1;
    if (memory_in_vtl(buf)) {
        real_type = UI_RAM_SRAM;
    }
    /* 如果是UI控件申请，打印该UI控件申请的buf总大小 */
    ui_module_ram_change(real_type, module_type, size, 1);
    /* ui_module_ram_dump(); */

    /* 统计整个UI框架申请的buf大小 */
    struct buffer_calc *p;
    int buffer_used_total = 0;
    list_for_each_entry(p, &buffer_used.list, list) {
        buffer_used_total += p->size;
    }
    printf("%s() =====>>> used buffer size:%d, malloc count: %d\n\n", __func__, buffer_used_total, jlui_malloc_cnt);
}
void ui_buf_calc_sub(void *buf, u32 ram_type, u32 module_type)
{
    jlui_malloc_cnt -= 1;
    struct buffer_calc *p, *n;
    int size = 0;
    list_for_each_entry_safe(p, n, &buffer_used.list, list) {
        if (p->buf == buf) {
            /* 如果释放的ram类型和记录的不一样，打印警报 */
            if (p->ram_type != ram_type) {
                printf("[Warning]: platform_free ram type, excepted: %s, actual: %s\n",
                       ui_ram_type_name[p->ram_type], ui_ram_type_name[ram_type]);
            }
            /* 如果释放的模块和记录的不一样，打印警报 */
            if (p->module_type != module_type) {
                printf("[Warning]: platform_free module type, excepted: %d, actual: %d\n", p->module_type, module_type);
                module_type = p->module_type;//使用记录的module
            }
            size = p->size;
            /* 打印释放的buf地址，buf大小，buf类型，释放模块 */
            printf("platform_free : 0x%x, size: %d, ram: %s, module: %d\n",
                   (int)p->buf, p->size, ui_ram_type_name[ram_type], module_type);
            __list_del_entry((struct list_head *)p);
            free(p);
        }
    }
    int real_type = -1;
    if (memory_in_vtl(buf)) {
        real_type = UI_RAM_SRAM;
    }
    /* 如果是UI控件释放，打印该UI控件剩余的buf总大小 */
    if (size) {
        ui_module_ram_change(real_type, module_type, size, -1);
    }
    /* ui_module_ram_dump(); */

    int buffer_used_total = 0;
    list_for_each_entry(p, &buffer_used.list, list) {
        buffer_used_total += p->size;
    }
    printf("%s() =====>>> used buffer size:%d, malloc count: %d\n\n", __func__, buffer_used_total, jlui_malloc_cnt);
}

#endif

/***************************************************************/
//			UI_BUF_CALC_LITE
/***************************************************************/
#ifdef UI_BUF_CALC_LITE
static u32 jlui_malloc_total_ram_count = 0;			//heap ram size
static u32 jlui_malloc_total_ram_max  = 0;			//heap ram峰值
static u32 jlui_malloc_total_ram_unuse = 0;			//未使用最大数量
static u32 jlui_malloc_total_ram_cnt_max  = 0;		//ram申请数量统计峰值
static u32 jlui_malloc_total_ram_cnt = 0;			//ram申请数量统计

static u32 jlui_malloc_sram_count = 0;
static u32 jlui_malloc_sram_max  = 0;
static u32 jlui_malloc_sram_unuse = 0;
static u32 jlui_malloc_sram_cnt_max  = 0;
static u32 jlui_malloc_sram_cnt = 0;
#endif
void jlui_malloc_ram_info_dump()
{
#ifdef UI_BUF_CALC_LITE
    printf("%s [totalram] curr_size:%d max_size%d curr_block:%d max_block:%d\n", __func__, \
           jlui_malloc_total_ram_count, jlui_malloc_total_ram_max, jlui_malloc_total_ram_cnt_max, jlui_malloc_total_ram_cnt);
    printf("%s [sram] curr_size:%d max_size%d curr_block:%d max_block:%d unuse_in_max:%d\n", __func__, \
           jlui_malloc_sram_count, jlui_malloc_sram_max, jlui_malloc_sram_cnt_max, jlui_malloc_sram_cnt, jlui_malloc_sram_unuse);
#endif
}

void jlui_malloc_ram_info_clr(u8 clr)
{
#ifdef UI_BUF_CALC_LITE
    if (clr & BIT(0)) {
        jlui_malloc_total_ram_count = 0;
        jlui_malloc_sram_count = 0;
    }
    if (clr & BIT(1)) {
        jlui_malloc_total_ram_max = 0;
        jlui_malloc_total_ram_cnt_max = 0;
        jlui_malloc_sram_max = 0;
        jlui_malloc_sram_cnt_max = 0;
    }
#endif
}

/***************************************************************/
//				CACHE_BUF
/***************************************************************/

#if TCFG_DCACHE_RUN_GPU_BUF || TCFG_ICACHE_RUN_GPU_BUF
#include "lbuf.h"

extern u32 dcache_ram_bss_remain_begin;
extern u32 dcache_ram_bss_remain_end;
#define DCACHE_GPU_START	(int)(&dcache_ram_bss_remain_begin)
#define DCACHE_GPU_END		(int)(&dcache_ram_bss_remain_end)
#define DCACHE_GPU_ADDR		(void *)(DCACHE_GPU_START)
#define DCACHE_GPU_SIZE		(int)(DCACHE_GPU_END - DCACHE_GPU_START)

extern u32 icache_ram_bss_remain_begin;
extern u32 icache_ram_bss_remain_end;
#define ICACHE_GPU_START	(int)(&icache_ram_bss_remain_begin)
#define ICACHE_GPU_END		(int)(&icache_ram_bss_remain_end)
#define ICACHE_GPU_ADDR		(void *)(ICACHE_GPU_START)
#define ICACHE_GPU_SIZE		(int)(ICACHE_GPU_END - ICACHE_GPU_START)

static struct lbuff_head *dcache_gpu_lbuf = NULL;
static struct lbuff_head *icache_gpu_lbuf = NULL;

#if TCFG_ICACHE_DYNAMIC_SWITCH
extern void icache_way_switch(u8 num);
static void jlgpu_icache_ram_check(void)
{
    if (icache_gpu_lbuf == NULL) {
        icache_way_switch(4 - TCFG_FREE_ICACHE_WAY);
        icache_gpu_lbuf = lbuf_init(ICACHE_GPU_ADDR, ICACHE_GPU_SIZE, 4, 0);
    }
}
static u8 jlgpu_icache_ram_enter(u32 timeout)
{
    if (icache_gpu_lbuf) {
        struct lbuff_state state = {0};
        lbuf_state(icache_gpu_lbuf, &state);
        if (((int)state.avaliable + (int)sizeof(struct lbuff_head)) == ICACHE_GPU_SIZE) {
            icache_gpu_lbuf = NULL;
            icache_way_switch(4);
            return 1;
        } else {
            putchar('C');
        }
    }
    return 0;
}
REGISTER_LP_REQUEST(power_jlui_icache) = {
    .name = "ich_ram",
    .request_enter = jlgpu_icache_ram_enter,
};
#endif

static int jlgpu_cache_init(void)
{
#if TCFG_DCACHE_RUN_GPU_BUF
    dcache_gpu_lbuf = lbuf_init(DCACHE_GPU_ADDR, DCACHE_GPU_SIZE, 4, 0);
    /* lbuf_dump(dcache_gpu_lbuf); */
#endif
#if TCFG_ICACHE_RUN_GPU_BUF
    icache_gpu_lbuf = lbuf_init(ICACHE_GPU_ADDR, ICACHE_GPU_SIZE, 4, 0);
    /* lbuf_dump(icache_gpu_lbuf); */
#endif
    return 0;
}
early_initcall(jlgpu_cache_init);

static int jlgpu_buf_is_cache(void *buf)
{
#if TCFG_DCACHE_RUN_GPU_BUF
    if (((int)buf >= DCACHE_GPU_START) && ((int)buf < DCACHE_GPU_END)) {
        return true;
    }
#endif
#if TCFG_ICACHE_RUN_GPU_BUF
    if (((int)buf >= ICACHE_GPU_START) && ((int)buf < ICACHE_GPU_END)) {
        return true;
    }
#endif
    return false;
}

static void *jlgpu_cache_malloc(int size)
{
    void *ptr = NULL;

#if TCFG_ICACHE_DYNAMIC_SWITCH
    jlgpu_icache_ram_check();
#endif

#if TCFG_DCACHE_RUN_GPU_BUF
    if (ptr == NULL) {
        ptr = lbuf_alloc(dcache_gpu_lbuf, size);
        /* if (ptr == NULL) { */
        /* printf("dcache gpu over \n"); */
        /* lbuf_dump(dcache_gpu_lbuf); */
        /* } */
    }
#endif
#if TCFG_ICACHE_RUN_GPU_BUF
    if (ptr == NULL) {
        ptr = lbuf_alloc(icache_gpu_lbuf, size);
        /* if (ptr == NULL) { */
        /* printf("icache gpu over \n"); */
        /* lbuf_dump(icache_gpu_lbuf); */
        /* } */
    }
#endif
    return ptr;
}

static void jlgpu_cache_free(void *buf)
{
    lbuf_free(buf);
}
#endif



/***************************************************************/
//				PSRAM
/***************************************************************/

int ui_buf_is_psram(void *buf)
{
#if TCFG_PSRAM_DEV_ENABLE
    if (psram_cache_check(buf)) {
        return true;
    }
    if (psram_no_cache_check(buf)) {
        return true;
    }
#endif
    return false;
}
#ifdef UI_BUF_CALC_LITE
static void *jlui_malloc_calc_lite(int size, u32 ram_type, u32 module_type)
{
    u8 *buf = NULL;
    u8 sram_flag = 0;
    size += 8;
    do {
#if TCFG_DCACHE_RUN_GPU_BUF || TCFG_ICACHE_RUN_GPU_BUF
        if (module_type == UI_MODULE_GPU) {
            buf = jlgpu_cache_malloc(size);
            if (buf) {
                break;
            }
        }
#endif
#if TCFG_PSRAM_DEV_ENABLE	//psram buf
        if (config_ui_alloc_psram_mod_sel & BIT((module_type & 0xffff))) {
            buf  = malloc_psram(size);
            if (buf) {
                break;
            }
        }
#endif
        buf = malloc(size); //sram buf
        sram_flag  = 1;
    } while (0);

    size -= 8;
    int magic = 0xaabbccdd;
    memcpy(buf, &magic, 4);		//free时识别
    memcpy(buf + 4, &size, 4);	//free时统计

    jlui_malloc_total_ram_count += size;
    jlui_malloc_total_ram_cnt++;
    if (jlui_malloc_total_ram_count > jlui_malloc_total_ram_max) {
        jlui_malloc_total_ram_max = jlui_malloc_total_ram_count;
        jlui_malloc_total_ram_cnt_max  = jlui_malloc_total_ram_cnt;
    }
    if (sram_flag) {
        jlui_malloc_sram_count += size;
        jlui_malloc_sram_cnt++;
        if (jlui_malloc_sram_count > jlui_malloc_sram_max) {
            jlui_malloc_sram_max = jlui_malloc_sram_count;
            jlui_malloc_sram_cnt_max  = jlui_malloc_sram_cnt;
            jlui_malloc_sram_unuse =  xPortGetPhysiceMemorySize();
        }
    }
    buf += 8;
    return (void *)buf;
}
static void jlui_free_calc_lite(void *buf)
{
    int size = 0;
    u8 *tmp_buf = (u8 *)buf;
    tmp_buf -= 8;
    u32 magic = 0;
    memcpy(&magic, tmp_buf, 4);
    if (magic == 0xaabbccdd) {
        memcpy(&size, tmp_buf + 4, 4);
        jlui_malloc_total_ram_cnt--;
        jlui_malloc_total_ram_count -= size;
        buf = tmp_buf;
    }
#if TCFG_DCACHE_RUN_GPU_BUF || TCFG_ICACHE_RUN_GPU_BUF
    if (jlgpu_buf_is_cache(buf)) {
        jlgpu_cache_free(buf);
        return;
    }
#endif
#if TCFG_PSRAM_DEV_ENABLE
    if (ui_buf_is_psram(buf)) {
        buf = psram_no_cache_2_cache(buf);
        free_psram(buf);
        return;
    }
#endif
    if (size) {
        jlui_malloc_sram_cnt--;
        jlui_malloc_sram_count -= size;
    }
    free(buf);
}
#endif
static void *jlui_malloc_normal(int size, u32 ram_type, u32 module_type)
{
    void *buf = NULL;
    do {
#if TCFG_DCACHE_RUN_GPU_BUF || TCFG_ICACHE_RUN_GPU_BUF
        if (module_type == UI_MODULE_GPU) {
            buf = jlgpu_cache_malloc(size);
            if (buf) {
                break;
            }
        }
#endif
#if TCFG_PSRAM_DEV_ENABLE	//psram buf
        if ((config_ui_alloc_psram_mod_sel & BIT(module_type)) || (ram_type == UI_RAM_PSRAM)) {
            buf = malloc_psram(size);
            if (buf) {
                break;
            }
        }
#endif
        buf = malloc(size); //sram buf
    } while (0);

    return buf;
}

static void jlui_free_normal(void *buf)
{
#if TCFG_DCACHE_RUN_GPU_BUF || TCFG_ICACHE_RUN_GPU_BUF
    if (jlgpu_buf_is_cache(buf)) {
        jlgpu_cache_free(buf);
        return;
    }
#endif
#if TCFG_PSRAM_DEV_ENABLE
    if (ui_buf_is_psram(buf)) {
        buf = psram_no_cache_2_cache(buf);
        free_psram(buf);
        return;
    }
#endif
    if (buf) {
        free(buf);
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlui_malloc 杰理UI框架内存申请接口
 *
 * @Params size 申请内存大小
 * @Params ram_type 申请的ram类型
 * @Params module_type 发起申请的模块
 *
 * @return 内存指针
 */
/* ------------------------------------------------------------------------------------*/
void *jlui_malloc(int size, u32 ram_type, u32 module_type)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
#ifdef UI_BUF_CALC_LITE
    void *buf = jlui_malloc_calc_lite(size, ram_type, module_type);
#else
    void *buf =  jlui_malloc_normal(size, ram_type, module_type);
#endif//UI_BUF_CALC_LITE
#ifdef UI_BUF_CALC
    ui_buf_calc_add(buf, size, ram_type, module_type);
#endif//UI_BUF_CALC
#ifdef UI_BUF_DEBUG
    printf("[%s]buf:0x%x rets:0x%x size:0x%x ram_type:0x%x module_type:0x%x", __func__, (u32)buf, rets, size, ram_type, module_type);
#endif
    return (void *)buf;
}

void *jlui_zalloc(int size)
{
    void *p = jlui_malloc(size, UI_RAM_SRAM, UI_MODULE_FRAME);
    if (p) {
        memset(p, 0x00, size);
    }
    return p;
}

void jlui_free(void *buf, u32 ram_type, u32 module_type)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
#ifdef UI_BUF_DEBUG
    printf("[%s]buf:0x%x rets:0x%x ram_type:0x%x module_type:0x%x", __func__, (u32)buf, rets, ram_type, module_type);
#endif
#ifdef UI_BUF_CALC_LITE
    jlui_free_calc_lite(buf);
#else
    jlui_free_normal(buf);
#endif//UI_BUF_CALC_LITE
#ifdef UI_BUF_CALC
    ui_buf_calc_sub(buf, ram_type, module_type);
#endif//UI_BUF_CALC
}

u8 buf_is_heap_addr(void *p)
{
    if (memory_in_vtl(p)) {
        return true;
    }
#if TCFG_DCACHE_RUN_GPU_BUF || TCFG_ICACHE_RUN_GPU_BUF
    if (jlgpu_buf_is_cache(p)) {
        return true;
    }
#endif
#if TCFG_PSRAM_DEV_ENABLE
    if (ui_buf_is_psram(p)) {
        return true;
    }
#endif
    return false;
}




#endif
