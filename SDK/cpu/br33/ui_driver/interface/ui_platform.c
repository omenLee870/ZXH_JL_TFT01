#include "timer.h"	// 定时器
#include "ui/lcd/lcd_drive.h"	// lcd 驱动

#include "res_config.h"	// res 文件管理
#include <math.h>
#include "ascii.h"
#include "res/font_ascii.h"
#include "res/mem_var.h"
#include "res/zz.h"
#include "dev_manager.h"
#include "fs/fs.h"
#include "watch_syscfg_manage.h"

#include "ui_core.h"
#include "control.h"

#include "dbi.h"	// 推屏模块
/* #include "jlgpu_math.h"		// matrix */
#include "jlgpu_driver.h"	// gpu 驱动

#include "gpu_port.h"	// GPU 接口，依赖于 jlgpu_driver.h
#include "gpu_task.h"
#include "gpu_draw.h"
#include "ui_resource.h"	// ui资源管理

#include "jlui_effect.h"		//特效相关
#include "buffer_manager.h"		// buffer管理
#include "ui_expand.h"		// 部分宏定义
#include "ui_addr.h"
#include "ui_math/ui_prime.h"

#include "font/font_all.h"
#include "font/font_textout.h"

#include "ui_draw/ui_circle.h"
#include "ui_draw/ui_figure.h"
#include "ui_animation.h"
#include "font/language_list.h"

#include "ui_measure.h"
#include "jljpeg_decode.h"
#include "gpu_draw.h"
#include "ui_page_switch.h"
#include "ui_draw/ui_basic.h"
#include "ui_text.h"
#include "jlui_app/ui_style.h"
#include "res/resfile.h"
#include "gif/gif.h"
#include "jljpg.h"
#include "gxdec.h"
#include "ui_multi_page_manager.h"
#include "generic/list.h"
#include "ui.h"
#include "ui_draw/ui_ring.h"

#include "ui_color.h"

/* #define DBG_PRINT_POS */
/* #include "ui_debug.h" */

#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".jlui_platform.data.bss")
#pragma data_seg(".jlui_platform.data")
#pragma const_seg(".jlui_platform.text.const")
#pragma code_seg(".jlui_platform.text")
#endif

/* debug 打印配置 */
#define LOG_TAG_CONST		PLATFORM
#define LOG_TAG             "[UI_PLATFORM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


/* --------------- */
/*  macro switch   */
/* --------------- */

#if CONFIG_LVGL_UI_ENABLE
#define GPU_GET_IMAGE_STRIDE(fmt, img, adr, cmp, cmp_size) \
    lvgl_res_get_image_stride(fmt, img, adr, cmp, cmp_size)
#else
#define GPU_GET_IMAGE_STRIDE(fmt, img, adr, cmp, cmp_size) \
    res_get_image_stride(fmt, img, adr, cmp, cmp_size)
#endif

/* --------------- */
/*  macro define   */
/* --------------- */
#define TEXT_MONO_CLR	0x555aaa
#define TEXT_MONO_INV	0xaaa555
#define RECT_MONO_CLR	0x555aaa
#define BGC_MONO_SET	0x555aaa


/* GPU task id 产生 */
#define GPU_TASK_ID(page, id, index)	((page << 26) | (0 << 24) | (id << 8) | index)

#define PRINTF_RECT(s,r)	{printf("[]%s %d %s(%d %d %d %d)",__func__,__LINE__,s,r->left,r->top,r->width,r->height);}

/* --------------- */
/* function define */
/* --------------- */
static int jlui_load_style(struct ui_style *style);
static void *jlui_load_window(int id);
static void jlui_unload_window(void *ui);
static int jlui_open_draw_context(struct draw_context *dc);
static int jlui_get_draw_context(struct draw_context *dc);
static int jlui_put_draw_context(struct draw_context *dc);
static int jlui_close_draw_context(struct draw_context *dc);
static int jlui_set_draw_context(struct draw_context *dc);
static int jlui_show_element(struct draw_context *dc);
static int jlui_hide_element(struct draw_context *dc);
static int jlui_delete_element(struct draw_context *dc);
static int jlui_set_element_prior(struct draw_context *dc);
static int jlui_fill_rect(struct draw_context *dc, u32 acolor);
static int jlui_draw_image(struct draw_context *dc, u32 src, u8 quadrant, u8 *ext_parm);
static int jlui_show_text(struct draw_context *dc, struct ui_text_attrs *text);
static int jlui_draw_rect(struct draw_context *dc, struct css_border *border);
static u32 jlui_read_point(struct draw_context *dc, u16 x, u16 y);
static int jlui_draw_point(struct draw_context *dc, u16 x, u16 y, u32 pixel);
static int jlui_invert_rect(struct draw_context *dc, u32 acolor);
static int jlui_read_image_info(struct draw_context *dc, u32 id, struct ui_image_attrs *attrs);
static int jlui_set_timer(void *priv, void (*callback)(void *), u32 msec);
static int jlui_del_timer(int timer_id);

static u16 get_mixed_pixel(u16 backcolor, u16 forecolor, u8 alpha);
static int open_resource_file();

#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
void *cache_gpu_input_data(void *head, pJLGPUTaskParam_t task_param, void *fp, int index);
#endif
/* --------------- */
/*  extern define  */
/* --------------- */
extern struct ui_load_info ui_load_info_table[];
extern const int font_scroll_circular_interval;
extern const int strpic_scroll_circular_deafult_enable;
extern const int config_gpu_cache_psram_jpeg_en ;
extern const int config_gpu_cache_psram_file_res_en;
extern const int JLUI_CUSTOM_DRAW_CACHE;
extern const int GPU_INPUT_TO_NOCACHE;
extern UI_RESFILE *res_file;
extern UI_RESFILE *str_file;
extern void *jlui_malloc(int size, u32 ram_type, u32 module_type);
extern void *jlui_zalloc(int size);
extern void jlui_free(void *buf, u32 ram_type, u32 module_type);
extern void jlui_malloc_ram_info_clr(u8 clr);
extern void ui_module_ram_dump();
extern u32 psram_get_used_block(void);
extern int jpeg_module_opj_clr_all();
extern void *cache_gpu_input_jpeg_data(void *head, pJLGPUTaskParam_t task_param, void *fp, int index);
extern void jpeg_draw_cb_gpu(int id, u8 *dst_buf, struct rect *dst_r, struct rect *src_r, u8 bytes_per_pixel, void *priv, void *matrix);
extern void *jpeg_module_opj_create(void *dev, int dev_len, u32 check, u32 index, u32 task_id, u32 element_id);
extern int jpeg_module_opj_release_async(void *jpg);
extern void *jpeg_hd_create_priv(u32 task_id, u32 elm_id);
extern int jpeg_hd_get_priv_len();
extern void psram_flush_cache(void *begin, u32 len);
extern char *ui_get_res_path_by_pj_id(int prj);
extern int ui_hide_multi_page();
extern int ui_in_effect();
/* --------------- */
/* private define  */
/* --------------- */
struct fb_map_user {
    u16 xoffset;
    u16 yoffset;
    u16 width;
    u16 height;
    u8  *baddr;
    u8  *yaddr;
    u8  *uaddr;
    u8  *vaddr;
    u8 transp;
    u8 format;
};

struct fb_var_screeninfo {
    u16 s_xoffset;		//显示区域x坐标
    u16 s_yoffset;		//显示区域y坐标
    u16 s_xres;			//显示区域宽度
    u16 s_yres;			//显示区域高度
    u16 v_xoffset;      //屏幕的虚拟x坐标
    u16 v_yoffset;      //屏幕的虚拟y坐标
    u16 v_xres;         //屏幕的虚拟宽度
    u16 v_yres;         //屏幕的虚拟高度
    u16 rotate;
};

struct window_head {
    u32 offset;
    u32 len;
    u32 ptr_table_offset;
    u16 ptr_table_len;
    u16 crc_data;
    u16 crc_table;
    u16 crc_head;
};

struct ui_file_head {
    u8  res[16];
    u8 type;
    u8 window_num;
    u16 prop_len;
    u8 rotate;
    u8 rev[3];
};


/*
 * {页面id, 页面索引, 显示优先级}
 * 页面索引范围: 2 ~ 255, 不允许重复
 * 相同显示优先级的页面按以下列表的先后顺序显示
 * */
const struct multi_page_info multi_page_info_table[] = {
    /* {PAGE_2, 3, PAGE_PRIO_BOTTOM}, */
    /* {PAGE_77, 2, PAGE_PRIO_TOP}, */
#if TCFG_UI_ENABLE_SMARTWIN
    {ID_WINDOW_SMARTWIN, 2, PAGE_PRIO_TOP},
#endif
};


/* --------------- */
/*  static define  */
/* --------------- */
static const struct ui_platform_api jlui_platform_api;

struct ui_priv {
    struct ui_platform_api *api;

    /* lcd 屏幕相关接口和信息 */
    struct lcd_interface *lcd;	// lcd api
    struct lcd_info info;		// lcd info

    u32 ui_rotate;
    u32 ui_hori_mirror;
    u32 ui_vert_mirror;
    u32 ui_file_len;

    UI_RESFILE *ui_file;
    UI_RESFILE *ui_file1;

    /* 背景颜色，允许修改背景任务颜色 */
    u32 background;		// rgb888 的背景色

    /* 双buf异步推屏时的buf管理句柄和地址，用于dbi模块中断时设置buf状态 */
    /* void *buffer_hdl;	// buffer 管理句柄 */
    /* void *buffer_adr;	// buffer 推屏地址 */
    /* OS_SEM buffer_wait; */
    /* OS_SEM lcd_te_sem; */
    /* pJLGPUMultTaskList_t mult_list; */

    int dc_flag;
    u8 *buf;
    u32 len;
    int buf_cnt;
    u8 ui_res_init;
    /* u8 line_end_flag; */

    u8 cache_mmu; // mmu_tab表缓存标志，置1时，prj_id等于1的资源mmu_tab表会被缓存
};
static struct ui_priv priv ALIGNED(4) = {0};
#define __this (&priv)
struct gif_timer gif_timer_list = {0};

extern const int JLUI_MULTI_PAGE_OVERLAY_SUPPORT;

//异步buffer释放管理模块
struct async_buffer {
    struct list_head entry;
    u8 *buf;
};
struct async_buffer_head {
    struct list_head entry;
    spinlock_t lock;
};
static struct async_buffer_head async_buffer_head_t = {0};

/* JPEG UI解码硬件引用计数 — 与camera编码共用硬件，按需初始化/释放 */
static void *g_gxdec_yuv_base = NULL;
static void *g_gxdec_a_base = NULL;
static int g_jpg_ui_dec_ref_cnt = 0;
static spinlock_t g_jpg_ui_dec_lock;

void async_buffer_init()
{
    INIT_LIST_HEAD(&async_buffer_head_t.entry);
    spin_lock_init(&async_buffer_head_t.lock);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpg_ui_dec_hw_init UI JPEG解码硬件按需初始化（引用计数）
 *        第一个申请者初始化GXDEC和JPEG解码器硬件，与camera编码共享硬件，
 *        仅在UI需要解码JPEG图片时才占用硬件
 *
 * @return 0
 */
/* ------------------------------------------------------------------------------------*/
int jpg_ui_dec_hw_init(void)
{
    spin_lock(&g_jpg_ui_dec_lock);
    if (g_jpg_ui_dec_ref_cnt == 0) {
        if (g_gxdec_yuv_base && g_gxdec_a_base) {
            printf("JPG DEC HW INIT (lazy) ----- \n");
            jpg_gxdec_init((u32)g_gxdec_yuv_base, (u32)g_gxdec_a_base, GXDEC_CACHE_CNT_4);
            jpg_dec_init();
        }
    }
    g_jpg_ui_dec_ref_cnt++;
    spin_unlock(&g_jpg_ui_dec_lock);
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpg_ui_dec_hw_deinit UI JPEG解码硬件释放（引用计数）
 *        最后一个释放者标记硬件空闲。注意：硬件CON0清零由jljpeg_decode_release
 *        负责，camera编码器重新配置硬件时会覆盖所有寄存器
 *
 * @return 0
 */
/* ------------------------------------------------------------------------------------*/
int jpg_ui_dec_hw_deinit(void)
{
    spin_lock(&g_jpg_ui_dec_lock);
    if (g_jpg_ui_dec_ref_cnt > 0) {
        g_jpg_ui_dec_ref_cnt--;
    }
    spin_unlock(&g_jpg_ui_dec_lock);
    return 0;
}

int async_buffer_add(void *p)
{
    spin_lock(&async_buffer_head_t.lock);
    struct async_buffer *new = zalloc(sizeof(struct async_buffer));
    if (!new) {
        spin_unlock(&async_buffer_head_t.lock);
        return -1;
    }
    new->buf = p;
    /* printf("%s(), ready release 0x%08x\n", __func__, (u32)p); */
    list_add_tail(&new->entry, &async_buffer_head_t.entry);
    spin_unlock(&async_buffer_head_t.lock);

    return 0;
}

int async_buffer_clr()
{
    struct async_buffer *p, *n;

    spin_lock(&async_buffer_head_t.lock);
    list_for_each_entry_safe(p, n, &async_buffer_head_t.entry, entry) {
        if (p->buf) {
            /* printf("%s(), release 0x%08x\n", __func__, (u32)p->buf); */
            jlui_free(p->buf, UI_RAM_SRAM, UI_MODULE_FRAME);
            list_del(&p->entry);
            free(p);
        }
    }
    spin_unlock(&async_buffer_head_t.lock);

    return 0;
}


/* 加载表盘缩略图时用于保存mmu表 */
static u32 mmu_hash(u32 hash, u8 *data, int len)
{
    /* u32 hash = 5383; */
    while (len--) {
        hash += (hash << 5) + (*data++);
    }
    return hash;
}
struct mmu_tab_cache {
    struct list_head entry;
    u32 check;
    int tab_size;
    void *tab;
};
static LIST_HEAD(mmu_cache);

void mmu_tab_cache_start()
{
    __this->cache_mmu = true;
}

struct mmu_tab_cache *mmu_tab_cache_check(u32 check)
{
    struct mmu_tab_cache *c = NULL;

    if (check && (check != (u32) - 1)) { // 0 和 -1 无效, 不做校验
        list_for_each_entry(c, &mmu_cache, entry) {
            if (c->check == check) {
                return c;
            }
        }
    }

    return NULL;
}

struct mmu_tab_cache *mmu_tab_cache_add(void *tab, int tab_size, u32 check)
{
    struct mmu_tab_cache *c = mmu_tab_cache_check(check);

    if (!c) {
        /* printf("%s(), tab: 0x%x, size: %d, check: 0x%x\n", __func__, (u32)tab, tab_size, check); */
        c = jlui_malloc(sizeof(struct mmu_tab_cache) + tab_size, UI_RAM_SRAM, UI_MODULE_FRAME);
        if (!c) {
            ASSERT(0, "Error, malloc buf fail, size: %d\n", tab_size);
        }
        c->check = check;
        c->tab_size = tab_size;
        memcpy((void *) & (c->tab), tab, tab_size);
        list_add_tail(&(c->entry), &mmu_cache);
    }

    return c;
}
struct mmu_tab_cache *mmu_flash_info_cache_add(void *__info, int tab_size, u32 check)
{
    struct mmu_tab_cache *c = mmu_tab_cache_check(check);
    struct flash_file_info *info = __info;
    if (!c) {
        /* printf("%s(), tab: 0x%x, size: %d, check: 0x%x\n", __func__, (u32)tab, tab_size, check); */
        c = jlui_malloc(sizeof(struct mmu_tab_cache) + sizeof(struct flash_file_info) + info->tab_size, UI_RAM_SRAM, UI_MODULE_FRAME);
        if (!c) {
            ASSERT(0, "Error, malloc buf fail, size: %d\n", tab_size);
        }
        c->check = check;
        c->tab_size = tab_size;
        struct flash_file_info *new_info = (struct flash_file_info *) & (c->tab);
        memcpy(new_info, __info, sizeof(struct flash_file_info));
        new_info->tab = (u32 *)(((u8 *)c) + (sizeof(struct mmu_tab_cache) + sizeof(struct flash_file_info)));
        /* printf("%s c:0x%x new_info:0x%x tab:0x%x", __func__, (u32)c, (u32)new_info, (u32)new_info->tab); */
        memcpy(new_info->tab, info->tab, info->tab_size);
        list_add_tail(&(c->entry), &mmu_cache);
    }

    return c;
}
void mmu_tab_cache_clear()
{
    /* printf("%s %d", __func__, __LINE__); */
    __this->cache_mmu = false;
    struct mmu_tab_cache *c = NULL;
    struct list_head *pos, *note;

    list_for_each_safe(pos, note, &mmu_cache) {
        c = list_entry(pos, struct mmu_tab_cache, entry);
        list_del(&(c->entry));
        jlui_free(c, UI_RAM_SRAM, UI_MODULE_FRAME);
    }
}



/* ------------------------------------------------------------------------------------*/
/**
 * @brief JLGPU_TASK_ROTATE_CONFIG 配置GPU任务的旋转参数
 */
/* ------------------------------------------------------------------------------------*/
#define JLGPU_TASK_ROTATE_CONFIG(tran) \
{ \
	task_param.rotate_en = dc->elm->rotate_en; \
	if (task_param.rotate_en && dc->elm->css.part) { \
		(tran)->rotate_cx = dc->elm->css.part->rotate.cent_x; \
		(tran)->rotate_cy = dc->elm->css.part->rotate.cent_y; \
		(tran)->rotate_dx = dc->elm->css.part->rotate.dx; \
		(tran)->rotate_dy = dc->elm->css.part->rotate.dy; \
		(tran)->rotate_angle = dc->elm->css.part->rotate.angle; \
		log_debug("%s(), rotate_en: %d, cx:%d, cy:%d, dx:%d, dy:%d, angle:%f\n", __func__, \
				task_param.rotate_en, (tran)->rotate_cx, (tran)->rotate_cy, \
				(tran)->rotate_dx, (tran)->rotate_dy, (tran)->rotate_angle); \
	} \
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief JLGPU_TASK_SCALE_CONFIG 配置GPU任务的缩放参数
 */
/* ------------------------------------------------------------------------------------*/
#define JLGPU_TASK_SCALE_CONFIG(tran) \
{ \
	task_param.scale_en = dc->elm->ratio_en; \
	if (task_param.scale_en && dc->elm->css.part) { \
		(tran)->ratio_w = dc->elm->css.part->ratio.ratio_w; \
		(tran)->ratio_h = dc->elm->css.part->ratio.ratio_h; \
		log_debug("%s(), scale_en: %d, ratio_w: %f, ratio_h: %f\n", __func__, \
				task_param.scale_en, (tran)->ratio_w, (tran)->ratio_h); \
	} \
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief INHERIT_PARENT_AFFINE 继承父控件的变换矩阵
 */
/* ------------------------------------------------------------------------------------*/
#define INHERIT_PARENT_AFFINE(tran) \
{ \
	for (; elm; elm = elm->parent) { \
		pJLGPUTaskUnit_t task_parent = jlgpu_get_task_by_id(dc->gpu_task_head, JLGPU_ID_NONE, elm->id); \
		if (task_parent && task_parent->info.matrix) { \
			log_info("%s(), get parent affine! parent elm_id: 0x%x\n", __func__, elm->id); \
			task_param.matrix = task_parent->info.matrix; \
			struct rect parent_rect = {0}; \
			ui_core_get_element_rect_relative_screen(elm, &parent_rect); \
			/* ui_core_get_element_rect_relative_parent(elm, &parent_rect); */ \
			/* DUMP_RECT(__func__, __LINE__, "parent", &parent_rect); */ \
			task_param.draw.left -= (/* parent_rect.left < 0 ? 0 : */ parent_rect.left); \
			task_param.draw.top -= (/* parent_rect.top < 0 ? 0 : */ parent_rect.top); \
			if (task_param.draw.height <= 0 || task_param.draw.width <= 0) { \
				task_param.invisible = true; \
			} \
			if (task_param.rotate_en) { \
				(tran)->rotate_dx -= parent_rect.left; \
				(tran)->rotate_dy -= parent_rect.top; \
			} \
		} \
	} \
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief JLGPU_TASK_INHERIT_PARENT 继承父控件的GPU参数
 *
 * 注意：这里可能会修改task_param的draw参数，应放到draw参数赋值之后
 */
/* ------------------------------------------------------------------------------------*/
#define JLGPU_TASK_INHERIT_PARENT(tran) \
{ \
	if (dc->elm && dc->elm->parent) { \
		log_debug("%s(), get parent info!:%x\n", __func__,dc->elm->id); \
		struct element *elm = dc->elm->parent; \
		struct element *par = elm->parent; \
		if ((par) && (ui_id2type(par->id) == CTRL_TYPE_GRID)) { \
			elm = par; \
		} \
		u8 is_overstepping_parent; \
		pJLGPUTaskUnit_t task_parent = jlgpu_get_task_by_id(dc->gpu_task_head, JLGPU_ID_NONE, elm->id); \
		if (task_parent) { \
			is_overstepping_parent = false; \
            struct element *elm = ui_core_get_element_by_id(task_parent->info.element_id); \
            if (elm) { \
                ui_core_get_element_rect_relative_screen(elm, &task_param.area); \
                jlgpu_shift_rect(&task_param.area); \
            } else { \
			    jlgpu_get_task_rect(task_parent, &task_param.area); \
            } \
		} else { \
			is_overstepping_parent = true; \
			/* ui_core_get_element_abs_rect(elm, &task_param.area); */ \
			ui_core_get_element_rect_relative_screen(elm, &task_param.area); \
		} \
		log_info("%s(), parent area[%d, %d, %d, %d]\n", __func__, \
				task_param.area.left, task_param.area.top, task_param.area.width, task_param.area.height); \
		INHERIT_PARENT_AFFINE(tran); \
		if (task_param.matrix && is_overstepping_parent) { \
			gpu_boundbox_t parent_area; \
			parent_area.minx = task_param.area.left; \
			parent_area.maxx = task_param.area.width + parent_area.minx; \
			parent_area.miny = task_param.area.top; \
			parent_area.maxy = task_param.area.height + parent_area.miny; \
			gpu_matrix_get_boundbox(task_param.matrix, &parent_area, &parent_area); \
			task_param.area.left = parent_area.minx; \
			task_param.area.width = parent_area.maxx - parent_area.minx; \
			task_param.area.top = parent_area.miny; \
			task_param.area.height = parent_area.maxy - parent_area.miny; \
		} else if (is_overstepping_parent) { \
			jlgpu_shift_rect(&task_param.area); \
			/* struct lcd_info *lcd_info = &(((struct ui_priv *)__this)->info); */ \
			/* task_param.area.left += ((GPU_BOUND_BOX_MAX - lcd_info->width) / 2); */ \
			/* task_param.area.top += ((GPU_BOUND_BOX_MAX - lcd_info->height) / 2); */ \
		} \
	} \
}


static void draw_rect_range_check(struct rect *r, struct fb_map_user *map)
{
    if (r->left < map->xoffset) {
        r->left = map->xoffset;
    }
    if (r->left > (map->xoffset + map->width)) {
        r->left = map->xoffset + map->width;
    }
    if ((r->left + r->width) > (map->xoffset + map->width)) {
        r->width = map->xoffset + map->width - r->left;
    }
    if (r->top < map->yoffset) {
        r->top = map->yoffset;
    }
    if (r->top > (map->yoffset + map->height)) {
        r->top = map->yoffset + map->height;
    }
    if ((r->top + r->height) > (map->yoffset + map->height)) {
        r->height = map->yoffset + map->height - r->top;
    }

    ASSERT(r->left >= map->xoffset);
    ASSERT(r->top  >= map->yoffset);
    ASSERT((r->left + r->width) <= (map->xoffset + map->width));
    ASSERT((r->top + r->height) <= (map->yoffset + map->height));
}

void l1_data_transformation(u8 *pix, u8 *pix_buf, u32 stride, int x, int y, int height, int width)
{
    int i, j, k = -1, h;

    for (j = 0; j < (height + 7) / 8; j++) { /* 纵向8像素为1字节 */
        for (i = 0; i < width; i++) {
            if ((i % 8) == 0) {
                k++;
            }
            u8 pixel = pix[j * width + i];
            int hh = height - (j * 8);
            if (hh > 8) {
                hh = 8;
            }
            for (h = 0; h < hh; h++) {
                u16 clr = (pixel & BIT(h)) ? 1 : 0;
                if (clr) {
                    pix_buf[(j * 8 + h) * stride + k] |= (1 << (7 - (i % 8)));
                }
            } /* endof for h */
        }/* endof for i */
        k = -1;
    }/* endof for j */
}

u32 __attribute__((weak)) set_retry_cnt()
{
    return 10;
}


UI_RESFILE *platform_get_file(int prj)
{
    return ui_load_sty_by_pj_id(prj);
}


int context_get_lcd_format()
{
    return 	__this->info.color_format;
}

__attribute__((always_inline))
static inline void __draw_vertical_line(struct draw_context *dc, int x, int y, int width, int height, int color, int format)
{
    int i, j;
    struct rect r = {0};
    struct rect disp = {0};

    disp.left  = x;
    disp.top   = y;
    disp.width = width;
    disp.height = height;
    if (!get_rect_cover(&dc->draw, &disp, &r)) {
        return;
    }

    switch (format) {
    case DC_DATA_FORMAT_OSD16:
        for (i = 0; i < r.width; i++) {
            for (j = 0; j < r.height; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + i, r.top + j, color);
                }
            }
        }
        break;
    case DC_DATA_FORMAT_MONO:
        for (i = 0; i < r.width; i++) {
            for (j = 0; j < r.height; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + i, r.top + j, color);
                }
            }
        }
        break;

    }
}


__attribute__((always_inline))
static inline void __draw_line(struct draw_context *dc, int x, int y, int width, int height, int color, int format)
{
    int i, j;
    struct rect r = {0};
    struct rect disp = {0};

    disp.left  = x;
    disp.top   = y;
    disp.width = width;
    disp.height = height;
    if (!get_rect_cover(&dc->draw, &disp, &r)) {
        return;
    }

    switch (format) {
    case DC_DATA_FORMAT_OSD16:
        for (i = 0; i < r.height; i++) {
            for (j = 0; j < r.width; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + j, r.top + i, color);
                }
            }
        }
        break;
    case DC_DATA_FORMAT_MONO:
        for (i = 0; i < r.height; i++) {
            for (j = 0; j < r.width; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + j, r.top + i, color);
                }
            }
        }
        break;
    }
}

__attribute__((always_inline_when_const_args))
AT_UI_RAM
static u16 get_mixed_pixel(u16 backcolor, u16 forecolor, u8 alpha)
{
    u16 mixed_color;
    u8 r0, g0, b0;
    u8 r1, g1, b1;
    u8 r2, g2, b2;

    if (alpha == 255) {
        return (forecolor >> 8) | (forecolor & 0xff) << 8;
    } else if (alpha == 0) {
        return (backcolor >> 8) | (backcolor & 0xff) << 8;
    }

    r0 = ((backcolor >> 11) & 0x1f) << 3;
    g0 = ((backcolor >> 5) & 0x3f) << 2;
    b0 = ((backcolor >> 0) & 0x1f) << 3;

    r1 = ((forecolor >> 11) & 0x1f) << 3;
    g1 = ((forecolor >> 5) & 0x3f) << 2;
    b1 = ((forecolor >> 0) & 0x1f) << 3;

    r2 = (alpha * r1 + (255 - alpha) * r0) / 255;
    g2 = (alpha * g1 + (255 - alpha) * g0) / 255;
    b2 = (alpha * b1 + (255 - alpha) * b0) / 255;

    mixed_color = ((r2 >> 3) << 11) | ((g2 >> 2) << 5) | (b2 >> 3);

    return (mixed_color >> 8) | (mixed_color & 0xff) << 8;
}

AT_UI_RAM
int line_update(u8 *mask, u16 y, u16 width)
{
    int i;
    if (!mask) {
        return true;
    }
    for (i = 0; i < (width + 7) / 8; i++) {
        if (mask[y * ((width + 7) / 8) + i]) {
            return true;
        }
    }
    return false;
}


void ui_draw_cb(u8 *dst_buf, struct rect *dst_r, u8 *src_buf, struct rect *src_r, u8 bytes_per_pixel)
{
    int h;
    struct rect r;
    int dst_stride = (dst_r->width * bytes_per_pixel + 3) / 4 * 4;
    int src_stride = (src_r->width * bytes_per_pixel + 3) / 4 * 4;

    if (get_rect_cover(dst_r, src_r, &r)) {
        for (h = 0; h < r.height; h++) {
            memcpy(&dst_buf[(r.top + h - dst_r->top) * dst_stride + (r.left - dst_r->left) * bytes_per_pixel],
                   &src_buf[(r.top + h - src_r->top)*src_stride + (r.left - src_r->left) * bytes_per_pixel],
                   r.width * bytes_per_pixel);
        }
    }
}

static inline void *jlgpu_get_task_addr(pJLGPUTaskUnit_t taskp)
{
    return (taskp) ? ((void *)&taskp->task_addr) : (NULL);
}


void ui_custom_draw_curve_grad(void *_dc, int task_id, int elm_id, int x, int y, int w, int h, void *priv)
{
    struct draw_context *dc = (struct draw_context *)_dc;
    struct element *elm = dc->elm;
    struct curve_grad_info *info = (struct curve_grad_info *)priv;

    /* 计算校验值 */
    u32 check = get_curve_grad_check(info->points, info->points_num, w, h, info->line_width, info->color);
    u8 *a8;
    /* printf("@@@ %s(), check: 0x%x\n", __func__, check); */

    pJLGPUTaskHead_t head = dc->gpu_task_head;
    pJLGPUTaskUnit_t task = jlgpu_get_task_by_id(head, task_id, elm_id);

    if (task) {
        u32 mmu_en, mmu_tb, addr = 0, rle_limit, tex_stride;
        jlgpu_driver_get_texture_addr(jlgpu_get_task_addr(task), &mmu_en, &mmu_tb, &addr, &rle_limit, &tex_stride);
        if (task->info.rev1 == (check & 0xffff)) {
            a8 = (u8 *)addr;
            goto __updtate_task;
        } else {
            jlui_free((void *)addr, UI_RAM_SRAM, UI_MODULE_FRAME);
        }
    }

    u8 dither = 3;
    u8 *grad = create_a8_gradient(0, h + dither, 200); // 获取渐变数组
    int x_max = info->points[info->points_num - 1].x;
    int *curve = compute_smooth_curve(info->points, info->points_num, x_max, h); // 将心率点转换为平滑曲线点
    a8 = (u8 *)jlui_malloc(w * h, UI_RAM_SRAM, UI_MODULE_FRAME); // A8格式图片缓存buf
    int half_line_width = abs(info->line_width) / 2;

    u8 *line_start;
    for (int i = 0; i < w; i++) {
        line_start = a8 + i * h;
#if 1
        if (i < x_max) {
            memset(line_start + curve[i], 0x00, h - curve[i]);
            if (i & 0x01) {
                memcpy(line_start, grad, curve[i]);
            } else {
                memcpy(line_start, grad + dither, curve[i]);
            }
            /* 描线宽，TODO */
            if (info->line_width < 0) { // 如果配置的线宽是负数，则直接描点
                for (int j = 0; j < abs(info->line_width); j++) {
                    int index = curve[i] - half_line_width + j;
                    if (index < 0) {
                        break;
                    }
                    /* line_start[index] = (line_start[index] + 200) >= 0xff ? 0xff : line_start[index] + 200; */
                    line_start[index] = 0xff;
                }
            }
        } else {
            memset(line_start, 0x00, h);
        }
#else
        memset(line_start, 0xff, h); // 纯色填充，用于debug
#endif
    }
    free(grad);
    free(curve);

__updtate_task:
    jlgpu_task_texture_param_init(task_id, elm_id);
    task_param.draw.left = x;
    task_param.draw.top  = y;
    task_param.draw.width = h;
    task_param.draw.height = w;
    jlgpu_get_win_rect(&task_param.area);

    task_param.alpha = 0xff;
    task_param.red = RGB565_R(info->color);
    task_param.green = RGB565_G(info->color);
    task_param.blue = RGB565_B(info->color);
    /* task_param.blend_mode = GPU_BLEND_SRC; */
    task_param.gpu_free_data = true;
    /* task_param.priority = 255; */
    task_param.priority = elm ? elm->prior + 1 : 0xff;

    task_param.format = GPU_FORMAT_A8;
    task_param.texture.data = a8;
    task_param.texture.not_compress = true;
    task_param.image.width = h;
    task_param.image.height = w;

    task_param.rotate_en = true;
    task_param.texture.tran.rotate_cx = h / 2;
    task_param.texture.tran.rotate_cy = w / 2;
    task_param.texture.tran.rotate_dx = x + w / 2;
    task_param.texture.tran.rotate_dy = y + h / 2;
    task_param.texture.tran.rotate_angle = 270;

    jlgpu_task_texture_param_map_707n();
    task = jlgpu_update_task_by_id(head, task_id, elm_id, &task_param);
    task->info.rev1 = check & 0xffff; // 记录校验值
}


static void gpu_draw_ring_full(struct draw_context *dc,
                               int center_x, int center_y,
                               int inner_radius, int outer_radius,
                               int start_angle, int curr_angle,
                               u32 fill_color)
{
    u16 id = 0;
    int elm_id = dc->elm->id;
    int task_id = GPU_TASK_ID(dc->page, id, dc->elm_index++);

    u8 a = ((fill_color >> 24) & 0xff);
    u8 r = ((fill_color >> 11) & 0x1f) << 3;
    u8 g = ((fill_color >> 5) & 0x3f) << 2;
    u8 b = (fill_color & 0x1f) << 3;


    /* 渐变任务参数初始化 */
    jlgpu_task_gradient_param_init(task_id/* GPU_GRAD_TASK_ID */, elm_id/*GPU_GRAD_ELEMENT*/);

    /* 设置渐变任务的任务类型为径向+角度渐变 */
    jlgpu_task_gradient_set_grad_type(JLGPU_DRAW_RADIALGRAD_ANGLEGRAD);

    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    jlgpu_task_gradient_set_quality(JLGPU_RENDERING_QUALITY_MSAA_8X);

    /* 计算圆环中线半径和弧段线宽 */
    float line_width = (float)(outer_radius - inner_radius);
    float half_lw = line_width / 2.0f;
    float radius  = (inner_radius + outer_radius) / 2.0f;
    float inner_r = radius - half_lw;
    float outer_r = radius + half_lw;

    /* 渐变任务颜色表 */
    /* static uint32_t clut[] = { */
    /* ARGB8888(0xff, 0xff, 0xff, 0xff), */
    /* ARGB8888(0xff, 0xff, 0xff, 0xff), */
    /* ARGB8888(0xff, 0xff, 0xff, 0xff), */
    /* }; */

    /* static uint32_t radial_clut[] = { */
    /* ARGB8888(0xff, 0xcd, 0xcd, 0xcd), */
    /* ARGB8888(0xff, 0xcd, 0xcd, 0xcd), */
    /* ARGB8888(0xff, 0xcd, 0xcd, 0xcd), */
    /* }; */

    struct rect win = {0};
    jlgpu_get_win_rect(&win);

    /* 绘制区域 (向外扩 1 像素避免浮点截边) */
    float fill_x = (float)(center_x - outer_r) - 0;
    float fill_y = (float)(center_y - outer_r) - 0;
    float fill_w = (float)(outer_r * 2) + 0;
    float fill_h = (float)(outer_r * 2) + 0;

    uint32_t *clut = (uint32_t *)malloc(sizeof(uint32_t) * 3);
    extern int gpu_buf_add(u8 * buf, u32 size);
    gpu_buf_add((u8 *)clut, sizeof(uint32_t) * 3);
    clut[0] = 0xffffffff;
    clut[1] = 0xffffffff;
    clut[2] = 0xffffffff;

    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    uint32_t *radial_clut = (uint32_t *)malloc(sizeof(uint32_t) * 3);
    extern int gpu_buf_add(u8 * buf, u32 size);
    gpu_buf_add((u8 *)radial_clut, sizeof(uint32_t) * 3);
    radial_clut[0] = (a << 24) | (r << 16) | (g << 8) | b;
    radial_clut[1] = (a << 24) | (r << 16) | (g << 8) | b;
    radial_clut[2] = (a << 24) | (r << 16) | (g << 8) | b;
    jlgpu_task_gradient_set_radial_clut(radial_clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变的填充默认 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO);

    jlgpu_task_gradient_set_affine_translate(win.left + fill_x, win.top + fill_y);

    jlgpu_task_gradient_set_ring(win.left + center_x, win.top + center_y, inner_r, outer_r);

    /* 设置角度渐变参数 */
    jlgpu_task_gradient_set_angle(win.left + center_x, win.top + center_y, start_angle/* GPU_GRAD_S_ANGLE */, curr_angle/* GPU_GRAD_E_ANGLE */);

    /* 复合指令时，只需设置径向渐变的模式，根据径向渐变的参数设置 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RING_GRADIENT);


    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(win.left + fill_x, win.top + fill_y, fill_w, fill_h);

    /* 配置渐变的父控件区域为整个屏幕 */
    /* jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen)); */
    jlgpu_task_gradient_set_area(win.left + fill_x, win.top + fill_y, fill_w, fill_h);

    /* 缩放使能 */
    JLGPU_TASK_SCALE_CONFIG(&task_param.fill.tran);

    /* 旋转使能 */
    JLGPU_TASK_ROTATE_CONFIG(&task_param.fill.tran);

    /* 继承父控件变换 */
    JLGPU_TASK_INHERIT_PARENT(&task_param.fill.tran);

    /* 707N 兼容映射: 完全参照 gpu_fill_rect */
    jlgpu_task_fill_param_map_707n();

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id, &task_param);
}

/* GPU 圆环矢量绘制:
 * 严格参照 ui_platform.c 中 gpu_fill_rect() 的任务参数设置步骤,
 * 在此基础上加入 PATH 矢量路径描述圆环弧段形状 */
static void gpu_draw_ring_arc(struct draw_context *dc,
                              int center_x, int center_y,
                              int inner_radius, int outer_radius,
                              int start_angle, int curr_angle,
                              u32 fill_color)
{
    /* task_id / element_id: 完全参照 gpu_fill_rect */
    /* int elm_id  = dc->elm->id; */
    /* u16 id_hash = CRC16(&fill_color, 4); */
    /* int task_id = ((dc->page << 26) | (0 << 24) | (id_hash << 8) | dc->elm_index++); */

    u16 id = 0;
    int elm_id = dc->elm->id;
    int task_id = GPU_TASK_ID(dc->page, id, dc->elm_index++);

    /* ARGB 分量: 完全参照 gpu_fill_rect 的位操作方式 */
    u8 a = ((fill_color >> 24) & 0xff);
    u8 r = ((fill_color >> 11) & 0x1f) << 3;
    u8 g = ((fill_color >> 5) & 0x3f) << 2;
    u8 b = (fill_color & 0x1f) << 3;

    jlgpu_task_fill_param_init(task_id, elm_id, a, r, g, b);

    jlgpu_task_gradient_set_quality(JLGPU_RENDERING_QUALITY_MSAA_8X);

    if (COLOR_VALID(fill_color)) {
        log_debug("%s(), fill task is visible, color: 0x%x\n", __func__, fill_color);
        task_param.invisible = false;
    } else {
        task_param.invisible = dc->elm->css.invisible;
    }

    /* 计算圆环中线半径和弧段线宽 */
    float line_width = (float)(outer_radius - inner_radius);
    float half_lw = line_width / 2.0f;
    float radius  = (inner_radius + outer_radius) / 2.0f;
    float inner_r = radius - half_lw;
    float outer_r = radius + half_lw;

    /* 绘制区域 (向外扩 1 像素避免浮点截边) */
    float fill_x = (float)(center_x - outer_r) - 1;
    float fill_y = (float)(center_y - outer_r) - 1;
    float fill_w = (float)(outer_r * 2) + 2;
    float fill_h = (float)(outer_r * 2) + 2;

    jlgpu_task_fill_set_draw(fill_x, fill_y, fill_w, fill_h);
    jlgpu_task_fill_set_area(fill_x, fill_y, fill_w, fill_h);

    /* draw / area / priority: 完全参照 gpu_fill_rect */
    task_param.draw.left   = fill_x;
    task_param.draw.top    = fill_y;
    task_param.draw.width  = fill_w;
    task_param.draw.height = fill_h;
    /* memcpy(&task_param.area, &task_param.draw, sizeof(struct rect)); */
    /* jlgpu_shift_rect(&task_param.area); */
    task_param.priority = dc->elm->prior;

    /* task_type / global_alpha: 完全参照 gpu_fill_rect */
    task_param.task_type = GPU_TASK_FILL;
    /* int global_alpha = dc->elm->css.alpha * 128 / 100; */
    /* task_param.global_alpha = global_alpha; */

    /* 计算弧段端点坐标 */
    float start_cx = center_x + radius * _trigo_cos(start_angle) * 1.0f / 32768;
    float start_cy = center_y + radius * _trigo_sin(start_angle) * 1.0f / 32768;
    float end_cx   = center_x + radius * _trigo_cos(curr_angle)  * 1.0f / 32768;
    float end_cy   = center_y + radius * _trigo_sin(curr_angle)  * 1.0f / 32768;

    float out_sx = center_x + outer_r * _trigo_cos(start_angle) * 1.0f / 32768;
    float out_sy = center_y + outer_r * _trigo_sin(start_angle) * 1.0f / 32768;
    float out_ex = center_x + outer_r * _trigo_cos(curr_angle)  * 1.0f / 32768;
    float out_ey = center_y + outer_r * _trigo_sin(curr_angle)  * 1.0f / 32768;

    float in_sx  = center_x + inner_r * _trigo_cos(start_angle) * 1.0f / 32768;
    float in_sy  = center_y + inner_r * _trigo_sin(start_angle) * 1.0f / 32768;
    float in_ex  = center_x + inner_r * _trigo_cos(curr_angle)  * 1.0f / 32768;
    float in_ey  = center_y + inner_r * _trigo_sin(curr_angle)  * 1.0f / 32768;

    struct rect win;
    jlgpu_get_win_rect(&win);

    /* 构建圆环弧段矢量路径 */
    jl_path_t *path = jl_path_create();
    jl_path_move_to(path, win.left + in_sx, win.top + in_sy);
    jl_path_line_to(path, win.left + out_sx, win.top + out_sy);
    jl_path_append_arc(path, (float)(win.left + center_x), (float)(win.top + center_y), outer_r,
                       (float)start_angle, (float)(curr_angle - start_angle), 0);
    jl_path_line_to(path, win.left + in_ex, win.top + in_ey);
    jl_path_append_arc(path, (float)(win.left + center_x), (float)(win.top + center_y), inner_r,
                       (float)curr_angle, (float)(start_angle - curr_angle), 0);
    jl_path_append_circle(path, win.left + start_cx, win.top + start_cy, half_lw, half_lw);
    jl_path_append_circle(path, win.left + end_cx, win.top + end_cy, half_lw, half_lw);
    /* jl_path_dump(path); */
    jl_path_end(path);

    jlgpu_task_path_set_enable();
    jlgpu_task_path_set_path(path);


    /* 缩放使能 */
    JLGPU_TASK_SCALE_CONFIG(&task_param.fill.tran);

    /* 旋转使能 */
    JLGPU_TASK_ROTATE_CONFIG(&task_param.fill.tran);

    /* 继承父控件变换 */
    JLGPU_TASK_INHERIT_PARENT(&task_param.fill.tran);

    /* 707N 兼容映射: 完全参照 gpu_fill_rect */
    jlgpu_task_fill_param_map_707n();

    /* log_print("%s(), draw[%d, %d, %d, %d]\n", __func__, */
    /* task_param.draw.left, task_param.draw.top, task_param.draw.width, task_param.draw.height); */

    /* log_print("%s(), area [%d, %d, %d, %d]\n", __func__, */
    /* task_param.area.left, task_param.area.top, task_param.area.width, task_param.area.height); */

    /* 提交任务: 完全参照 gpu_fill_rect */
    jlgpu_update_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id, &task_param);
}

int ui_gpu_draw(struct draw_context *dc, u8 *buf, int x, int y, int width, int height, void *cb, void *priv, int priv_len, int id)
{

    if (id == DRAW_RING) {
        struct gpu_ring_info *info = (struct gpu_ring_info *)priv;
        if ((info->curr_angle - info->start_angle) == 360) {
            gpu_draw_ring_full(dc, info->center_x, info->center_y, info->inner_radius, info->outer_radius, info->start_angle, info->curr_angle, info->fill_color);
        } else {
            gpu_draw_ring_arc(dc, info->center_x, info->center_y, info->inner_radius, info->outer_radius, info->start_angle, info->curr_angle, info->fill_color);
        }
    }
    return 0;
}

int gpu_fill_rect(struct draw_context *dc, int left, int top, int width, int height, u32 acolor);
int ui_draw(struct draw_context *dc, u8 *buf, int x, int y, int width, int height, void *cb, void *priv, int priv_len, int id)
{
    struct element *elm = dc->elm;
    int elm_id = elm->id;
    int task_id = GPU_TASK_ID(dc->page, id, dc->elm_index++);

    if (ui_in_effect()) {
        return 0;
    }

    if (id == DRAW_CURVE_GRAD) { // 绘制渐变图片
        ui_custom_draw_curve_grad(dc, task_id, elm_id, x, y, width, height, priv);
        return 0;
    }

    if (JLUI_CUSTOM_DRAW_CACHE) {
        if ((id == FILL_RECT) || (id == DRAW_RECT)) {
            gpu_fill_rect(dc, x, y, width, height, ui_draw_get_color(id, priv));
            return 0;
        }
        u32 check = ((u32)dc->gpu_task_head) ^ ((u32)task_id); // 检测标志，保证每条任务链都有单独的缓存
        /* 自定义绘图绘制 */
        int temp_buf_size = width * height; // 自定义绘图CPU绘制固定A8格式，如果添加了其它格式，这里要跟着改
        void *temp_buf = jlui_malloc(temp_buf_size, UI_RAM_PSRAM, UI_MODULE_CUSTOM_DRAW);
        ASSERT(temp_buf, "Error, realloc cache buf faild!\n");
        memset(temp_buf, 0x00, temp_buf_size);
        struct rect dst_rect = {0}, src_rect = {0};
        dst_rect.width = src_rect.width = width;
        dst_rect.height = src_rect.height = height;
        u32 color = ui_draw_get_color(id, priv); // 获取自定义绘图颜色
        ui_disp_out_stride_set(width * 1);
        custom_draw_func draw_func = (custom_draw_func)cb;
        draw_func(id, temp_buf, &dst_rect, &src_rect, 1, priv);

        /* 转储为块排列 */
        int tile_w = (width + 15) / 16 * 16; // 16对齐
        int tile_h = (height + 15) / 16 * 16;
        int tile_size = tile_w * tile_h;

        /* 注意，这里将tile_buf放到gpu_cache模块管理，会随gpu_cache模块清空，不需要主动释放 */
        void *tile_buf = gpu_cache_realloc(tile_size, check);
        ASSERT(temp_buf, "Error, realloc cache buf faild!\n");
        gpu_cache_valid_value_set((u32)tile_buf, 3);
        texture_line_to_tile_base(tile_buf, temp_buf, width, height, GPU_FORMAT_A8); // 将自定义绘图的图片转为块存储
        jlui_free(temp_buf, UI_RAM_PSRAM, UI_MODULE_CUSTOM_DRAW);


        jlgpu_task_texture_param_init(task_id, elm_id);
        task_param.priority = elm->prior;
        task_param.draw.left = x;
        task_param.draw.top = y;
        task_param.draw.width = width;
        task_param.draw.height = height;
        jlgpu_get_win_rect(&task_param.area);
        task_param.texture.data = tile_buf;
        task_param.format = GPU_FORMAT_A8;
        task_param.alpha = ARGB8565_A(color);
        task_param.red   = ARGB8565_R(color);
        task_param.green = ARGB8565_G(color);
        task_param.blue  = ARGB8565_B(color);
        task_param.image.width = tile_w;//width;
        task_param.image.height = tile_h;//height;
        task_param.texture.not_compress = true;
        task_param.has_clut = CLUT_TAB_NULL;
        /* task_param.texture.adr_mode = 0; */
        task_param.texture.adr_mode = JLGPU_ADRMODE_LINE;
        task_param.draw_en = false;

        task_param.texture.crop.width = width;
        task_param.texture.crop.height = height;

        if (dc->gpu_task_mode == GPU_TASK_MODE_PERSPECTIVE) {
            /* task_param.task_type = GPU_TASK_TRANS | GPU_TEXTURE_PERSPECTIVE; */
            task_param.task_type = GPU_TASK_IMAGE_TRANS(JLGPU_TRANS_MODE_PROJECTION);
        } else {
            task_param.task_type = GPU_TASK_IMAGE;
        }
        int global_alpha =  dc->elm->css.alpha * 128 / 100;
        task_param.global_alpha = global_alpha;

        /* 缩放使能 */
        JLGPU_TASK_SCALE_CONFIG(&task_param.fill.tran);

        /* 旋转使能 */
        JLGPU_TASK_ROTATE_CONFIG(&task_param.fill.tran);

        /* 继承父控件变换 */
        JLGPU_TASK_INHERIT_PARENT(&task_param.fill.tran);

        jlgpu_task_fill_param_map_707n();
        jlgpu_update_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id, &task_param);
    } else {
        jlgpu_task_basic_param_init(GPU_TASK_DRAW, task_id, elm_id);
        task_param.priority = elm->prior;
        task_param.draw.left = x;
        task_param.draw.top = y;
        task_param.draw.width = width;
        task_param.draw.height = height;
        task_param.cb_func = cb;

        pJLGPUTaskUnit_t taskp = jlgpu_get_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id);
        if (taskp) {
            /* if (taskp->info.draw_info) { */
            /* task_param.priv = taskp->info.draw_info->priv; */
            /* } */
            if (jlgpu_chain_task_custom_data(taskp)) {
                task_param.priv = jlgpu_chain_task_custom_data(taskp);
            }
        }
        if (!task_param.priv) {
            task_param.priv = jlui_zalloc(priv_len);//在任务销毁时释放
        }
        memcpy(task_param.priv, priv, priv_len);
        task_param.priv_len = priv_len;
        task_param.draw_id = id;
        task_param.draw_en = 1;

        /* 缩放使能 */
        JLGPU_TASK_SCALE_CONFIG(&task_param.fill.tran);

        /* 旋转使能 */
        JLGPU_TASK_ROTATE_CONFIG(&task_param.fill.tran);

        /* 继承父控件变换 */
        JLGPU_TASK_INHERIT_PARENT(&task_param.fill.tran);
        if (dc->gpu_task_mode == GPU_TASK_MODE_PERSPECTIVE) {
            /* task_param.task_type = GPU_TASK_TRANS | GPU_FILL_PERSPECTIVE; */
            task_param.task_type = GPU_TASK_FILL_TRANS(JLGPU_TRANS_MODE_PROJECTION);
        }
        jlgpu_task_fill_param_map_707n();
        pJLGPUTaskUnit_t new_taskp =  jlgpu_update_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id, &task_param);
        if (taskp && (new_taskp != taskp)) {
            /* new_taskp->info.draw_info->priv = jlui_zalloc(priv_len); */
            /* memcpy(new_taskp->info.draw_info->priv, priv, priv_len); */
            pJLGPUTaskDraw_t task_draw = jlgpu_chain_task_custom_data(taskp);
            task_draw->priv = jlui_zalloc(priv_len);
            memcpy(task_draw->priv, priv, priv_len);
        }
    }
    return 0;
}
static int get_strpic_width(struct draw_context *dc, u8 *str, int *str_width, int *str_height, int str_num)
{
    struct image_file file = {0};
    u16 *p = (u16 *)str;
    int width = 0;
    int max_h = 0;
    int i;

    for (i = 0; i < str_num; i++) {
        if (open_string_pic2(dc->prj, &file, *p)) {
            return -EINVAL;
        }

        printf("file_w %d, file_h %d\n", file.width, file.height);
        width += file.width;
        max_h = (MAX(file.height, max_h));
        p++;
    }
    *str_width = width;
    *str_height = max_h;

    if ((*str_width == 0) || (*str_height == 0)) {
        printf("get_strpic_width_err\n");
        return -EINVAL;
    }

    return 0;
}



static int get_multi_strs_width(struct draw_context *dc, u8 *str, int *str_width, int *str_height, int *strnum)
{
    struct image_file file = {0};
    u8 *str_index_buf = NULL;
    u16 *pstr = NULL;
    int max_h = 0;
    int w, h;
    u16 *p = (u16 *)str;
    int width = 0;
    int num = 0;

    while (*p != 0) {
        if (open_string_pic(dc->prj, &file, *p)) {
            printf("now here3\n");
            if ((width > 0) && (max_h > 0) && (num > 0)) {
                break;
            }
            return -EINVAL;
        }
        if (!file.len) {
            printf("now here2\n");
            p++;
            continue;
        }

        printf("ones len %d\n", file.len);

        w = file.width;
        h = file.height;

        /* str_index_buf = (u8 *)jlui_malloc(file.len); */

        /* read_str_data(&file, str_index_buf, file.len); */
        /* pstr = (u16 *)str_index_buf; */

        /* if (get_strpic_width(dc, &pstr[1], &w, &h, pstr[0])) { */
        /* jlui_free(str_index_buf, UI_RAM_SRAM, UI_MODULE_FRAME); */
        /* printf("now here\n"); */
        /* p++; */
        /* continue; */
        /* } */

        width += w;
        max_h = (MAX(h, max_h));
        p++;
        num++;
        /* jlui_free(str_index_buf, UI_RAM_SRAM, UI_MODULE_FRAME); */
    }
    *str_width = width;
    *str_height = max_h;
    *strnum = num;

    if ((*str_width == 0) || (*str_height == 0)) {
        printf("now here4\n");
        return -EINVAL;
    }

    return 0;
}


int get_multi_string_width(struct draw_context *dc, u8 *str, int *str_width, int *str_height)
{
    struct image_file file = {0};
    u16 *p = (u16 *)str;
    int width = 0;
    while (*p != 0) {
        if (open_string_pic(dc->prj, &file, *p)) {
            return -EINVAL;
        }
        width += file.width;
        p++;
    }
    *str_width = width;
    *str_height = file.height;

    return 0;
}

#if 0
void ui_draw_circle(struct draw_context *dc, int center_x, int center_y,
                    int radius_small, int radius_big, int angle_begin,
                    int angle_end, int color, int percent)
{
    struct circle_info info;
    info.center_x = center_x;
    info.center_y = center_y;
    info.radius_big = radius_big;
    info.radius_small = radius_small;
    info.angle_begin = angle_begin;
    info.angle_end = angle_end;
    info.left = dc->draw.left;
    info.top = dc->draw.top;
    info.width = dc->draw.width;
    info.height = dc->draw.height;

    info.x = 0;
    info.y = 0;
    info.disp_x = dc->disp.left;
    info.disp_y = dc->disp.top;

    info.color = color;
    info.bitmap_width = dc->disp.width;
    info.bitmap_height = dc->disp.height;
    info.bitmap_pitch = dc->disp.width * 2;
    info.bitmap = dc->buf;
    info.bitmap_size = dc->len;
    info.bitmap_depth = 16;

    draw_circle_by_percent(&info, percent);
}
#endif//不使用


//清除自定义绘图任务
void ui_custom_draw_clear(struct draw_context *dc)
{
    if (!ui_in_effect()) {
        jlgpu_scheduler_wait_sync();
        jlgpu_task_clean_up_by_id(dc->gpu_task_head, dc->elm->id, 0x20);
    }
}

int ui_draw_image(struct draw_context *dc, int page, int id, int x, int y)
{
    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;
    dc->draw_img.id = id;
    dc->draw_img.page = page;
    dc->draw_img.rect.left = 0;
    dc->draw_img.rect.top = 0;
    dc->draw_img.rect.width = -1;//INT_MAX;
    dc->draw_img.rect.height = -1;//INT_MAX;

    return jlui_draw_image(dc, 0, 0, NULL);
}

int ui_draw_ram_image(struct draw_context *dc, struct image_file *image, int custom_color_en, int color)
{
    /*
     	至少需要
    	format(PIXEL_FMT_XXX)
    	width
    	height
    	offset(src_addr,注意变量生命周期)
    	len
    	compress(0:unzip)
     */
    if (custom_color_en) {
        dc->custom_color |= BIT(UI_CUSTOM_COLOR_BIT_IMAGE);
        dc->custom_argb8888 = color;
    }
    return jlui_draw_image(dc, 0, 0, (u8 *)image);
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_draw_image_invert 绘制镜像图片（图片倒影）
 *
 * @param dc		绘图句柄
 * @param page		页面
 * @param id		资源id
 * @param x			left
 * @param y			top
 * @param x_en		使能水平翻转：0不翻转	1向右翻转 	2向左翻转
 * @param y_en		使能垂直翻转：0不翻转	1向下翻转	2向上翻转
 * @param x_dist	翻转后距离原图最近边界距离
 * @param y_dist	翻转后距离原图最近边界距离
 * @param alpha		反转后图片透明度
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_draw_image_invert(struct draw_context *dc, int page, int id, int x, int y, int x_en, int y_en, int x_dist, int y_dist, int alpha)
{
    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;
    dc->draw_img.id = id;
    dc->draw_img.page = page;
    dc->draw_img.rect.left = 0;
    dc->draw_img.rect.top = 0;
    dc->draw_img.rect.width = -1;//INT_MAX;
    dc->draw_img.rect.height = -1;//INT_MAX;

    dc->draw_img.invert_x_en = x_en;
    dc->draw_img.invert_y_en = y_en;
    dc->draw_img.invert_x_dist = x_dist;
    dc->draw_img.invert_y_dist = y_dist;
    ui_core_set_element_ratio((void *)dc->elm, 1.0f, 1.0f, 1);	//affine textture
    int rec_alpha = dc->elm->css.alpha;							//记录原透明度
    dc->elm->css.alpha = alpha;									//修改图片透明度
    int ret = jlui_draw_image(dc, 0, 0, NULL);					//绘图
    dc->elm->css.alpha = rec_alpha;								//还原透明度

    return ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlui_image_invert_deal 图片翻转处理
 *
 * @param dc		绘图句柄
 * @param taskp		gpu任务
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jlui_image_invert_deal(struct draw_context *dc, pJLGPUTaskUnit_t taskp)
{
    gpu_matrix_t matrix;
    struct rect win_rect;
    jlgpu_get_win_rect(&win_rect);
    jlgpu_get_matrix(taskp, &matrix);													//获取任务矩阵
    gpu_matrix_translate(&matrix, win_rect.left, win_rect.top);							//回到gpu原点

    gpu_matrix_flip(&matrix, dc->draw_img.invert_x_en, dc->draw_img.invert_y_en);		//翻转

    int invert_x_offset = 0;
    int invert_y_offset = 0;
    if (dc->draw_img.invert_x_en == 1) {
        invert_x_offset += 2 * taskp->info.image_width;
        invert_x_offset += dc->draw_img.invert_x_dist;
    } else if (dc->draw_img.invert_x_en == 2) {
        invert_x_offset -= dc->draw_img.invert_x_dist;
    }
    if (dc->draw_img.invert_y_en == 1) {
        invert_y_offset += 2 * taskp->info.image_height;
        invert_y_offset += dc->draw_img.invert_y_dist;
    } else if (dc->draw_img.invert_y_en == 2) {
        invert_y_offset -= dc->draw_img.invert_y_dist;
    }
    gpu_matrix_translate(&matrix, invert_x_offset, invert_y_offset);			//翻转后偏移

    int trans_x = -win_rect.left;
    int trans_y = -win_rect.top;
    if (dc->draw_img.invert_x_en) {
        trans_x *= -1;
    }
    if (dc->draw_img.invert_y_en) {
        trans_y *= -1;
    }
    gpu_matrix_translate(&matrix, trans_x, trans_y);									//回到gpu中心

    jlgpu_set_matrix(taskp, &matrix);													//配置任务矩阵

    gpu_boundbox_t bbox;
    bbox.minx = 0;
    bbox.miny = 0;
    bbox.maxx = taskp->info.image_width;
    bbox.maxy = taskp->info.image_height;
    gpu_matrix_get_boundbox(&matrix, &bbox, &bbox);										//计算翻转后的有效区域
    jlgpu_driver_set_bbox(&taskp->task_addr, &bbox);									//配置任务有效区域
    return 0;
}
int ui_draw_image_large(struct draw_context *dc, int page, int id, int x, int y, int width, int height, int image_x, int image_y)
{
    struct rect image_r;
    struct rect r;
    image_r.left = x;
    image_r.top = y;
    image_r.width = width;
    image_r.height = height;
    if (get_rect_cover(&dc->rect, &image_r, &r)) {
        dc->draw_img.en = true;
        dc->draw_img.x = r.left;
        dc->draw_img.y = r.top;
        dc->draw_img.id = id;
        dc->draw_img.page = page;
        ASSERT(image_x < 512);
        ASSERT(image_y < 512);
        dc->draw_img.rect.left = image_x;
        dc->draw_img.rect.top = image_y;
        dc->draw_img.rect.width = r.width;
        dc->draw_img.rect.height = r.height;
    } else {
        return -1;
    }

    return jlui_draw_image(dc, 0, 0, NULL);
}


int ui_draw_ascii(struct draw_context *dc, char *str, int strlen, int x, int y, int color)
{
    static struct ui_text_attrs text = {0};
    /* text.str = str; */
    /* text.format = "ascii"; */
    /* text.color = color; */
    /* text.strlen = strlen; */
    /* text.flags = FONT_DEFAULT; */

    /* dc->draw_img.en = true; */
    /* dc->draw_img.x = x; */
    /* dc->draw_img.y = y; */

    return jlui_show_text(dc, &text);
}

int ui_draw_text(struct draw_context *dc, int encode, int endian, char *str, int strlen, int x, int y, int color)
{
    /* static  */struct ui_text_attrs text = {0};
    text.str = str;
    text.format = UI_TEXT_ENCODE_TEXT;
    text.color = color;
    text.strlen = strlen;
    text.encode = encode;
    text.endian = endian;
    text.flags = FONT_DEFAULT;

    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;
    dc->gpu_free_data = true;

    return jlui_show_text(dc, &text);
}

int ui_draw_strpic(struct draw_context *dc, int id, int x, int y, int color)
{
    static struct ui_text_attrs text = {0};
    /* static u16 strbuf ALIGNED(32); */

    /* strbuf = id; */
    /* text.str = &strbuf; */
    /* text.format = "strpic"; */
    /* text.color = color; */
    /* text.strlen = 0; */
    /* text.encode = 0; */
    /* text.endian = 0; */
    /* text.flags = 0; */

    /* dc->draw_img.en = true; */
    /* dc->draw_img.x = x; */
    /* dc->draw_img.y = y; */

    return jlui_show_text(dc, &text);
}

u32 ui_draw_get_pixel(struct draw_context *dc, int x, int y)
{
    return 0;
}

u16 ui_draw_get_mixed_pixel(u16 backcolor, u16 forecolor, u8 alpha)
{
    return get_mixed_pixel(backcolor, forecolor, alpha);
}


static int open_resource_file()
{
    int ret;

    printf("open_resouece_file...\n");

    ret = open_resfile(RES_PATH"JL/JL.res");
    if (ret) {
        return -EINVAL;
    }
    ret = open_str_file(RES_PATH"JL/JL.str");
    if (ret) {
        return -EINVAL;
    }
    ret = font_ascii_init(FONT_PATH"ascii.res");
    if (ret) {
        return -EINVAL;
    }
    return 0;
}

int __attribute__((weak)) lcd_get_screen_info(struct fb_var_screeninfo *info)
{
    info->s_xoffset = 0;
    info->s_yoffset = 0;
    info->s_xres = __this->info.width;//240;
    info->s_yres = __this->info.height;//240;

    return 0;
}


int get_cur_srreen_width_and_height(u16 *screen_width, u16 *screen_height)
{
    memcpy((u8 *)screen_width, (u8 *)&__this->info.width, sizeof(__this->info.width));
    memcpy((u8 *)screen_height, (u8 *)&__this->info.height, sizeof(__this->info.height));
    return 0;
}

int get_cur_srreen_radius_and_fill_argb(u16 *screen_radius, u32 *screen_fill_argb)
{
    if (__this->info.width == __this->info.height) {
        *screen_radius = __this->info.width / 2;
    } else {
        *screen_radius = __this->info.width / 4;
    }
    *screen_fill_argb = 0xffffffff;
    return 0;
}



int ui_style_file_version_compare(int version)
{
    /* int v; */
    int len;
    struct ui_file_head head;
    static u8 checked = 0;

    if (checked == 0) {
        if (!__this->ui_file) {
            puts("ui version_compare ui_file null!\n");
            ASSERT(0);
            return 0;
        }
        res_fseek(__this->ui_file, 0, SEEK_SET);
        len = sizeof(struct ui_file_head);
        res_fread(__this->ui_file, &head, len);
        printf("style file version is: 0x%x,UI_VERSION is: 0x%x\n", *(u32 *)(head.res), version);
        if (*(u32 *)head.res != version) {
            puts("style file version is not the same as UI_VERSION !!\n");
            ASSERT(0);
        }
        checked = 1;
    }
    return 0;
}
static int jlui_load_style(struct ui_style *style)
{
    int err;
    int i, j;
    int len;
    char name[64];
    char style_name[16];
    static char cur_style = 0xff;

    if (!style->file && cur_style == 0) {
        return 0;
    }

    if (style->file == NULL) {
        ASSERT(0);
        cur_style = 0;
        err = open_resource_file();
        if (err) {
            return -EINVAL;
        }
        __this->ui_file1 = res_fopen(RES_PATH"JL/JL.sty", "r");
        if (!__this->ui_file1) {
            return -ENOENT;
        }
        __this->ui_file = __this->ui_file1;
        __this->ui_file_len = 0x7fffffff;//res_flen(ui_file1);
        len = 6;
        strcpy(style_name, "JL.sty");
        style_name[len - 4] = 0;
        ui_core_set_style(style_name);

    } else {
#if 0
        cur_style = 1;
        ui_set_sty_path_by_pj_id(0, style->file);
        __this->ui_file = ui_load_sty_by_pj_id(0);
        ASSERT(__this->ui_file);

        __this->ui_file_len = 0x7fffffff;//res_flen(ui_file1);
        for (i = 0; style->file[i] != '.'; i++) {
            name[i] = style->file[i];
        }
        printf("open resfile 0 :%s\n", name);
        name[i++] = '.';
        name[i++] = 'r';
        name[i++] = 'e';
        name[i++] = 's';
        name[i] = '\0';
        printf("open resfile %s\n", name);
        res_file = ui_load_res_by_pj_id(0);

        ASSERT(res_file);
        name[--i] = 'r';
        name[--i] = 't';
        name[--i] = 's';
        printf("open strfile %s\n", name);
        str_file = ui_load_str_by_pj_id(0);
        ASSERT(str_file);

        name[i++] = 'a';
        name[i++] = 's';
        name[i++] = 'i';
        printf("open asciifile %s\n", name);
#endif
        font_ascii_init(FONT_PATH"ascii.res");

        for (i = strlen(style->file) - 5; i >= 0; i--) {
            if (style->file[i] == '/') {
                break;
            }
        }

        for (i++, j = 0; style->file[i] != '\0'; i++) {
            if (style->file[i] == '.') {
                name[j] = '\0';
                break;
            }
            name[j++] = style->file[i];
        }
        ASCII_ToUpper(name, j);
        if (!strncmp(name, "WATCH", strlen("WATCH"))) {
            strcpy(name, "JL");
        }
        if (!strncmp(name, "UPGRADE", strlen("UPGRADE"))) {
            strcpy(name, "JL");
        }

        err = ui_core_set_style(name);
        if (err) {
            printf("style_err: %s\n", name);
        }
    }

    return 0;
}



static void *jlui_load_window(int id)
{
    u8 *ui;
    int i;
    u32 *ptr;
    u16 *ptr_table;
    struct ui_file_head head ALIGNED(4);
    struct window_head window ALIGNED(4);
    int len = sizeof(struct ui_file_head);
    int retry;
    static const int rotate[] = {0, 90, 180, 270};

    if (!__this->ui_file) {
        printf("ui_file : 0x%x\n", (int)__this->ui_file);
        return NULL;
    }

    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(__this->ui_file, 0, SEEK_SET);
        res_fread(__this->ui_file, &head, len);

        if (id >= head.window_num) {
            return NULL;
        }

        res_fseek(__this->ui_file, sizeof(struct window_head)*id, SEEK_CUR);
        res_fread(__this->ui_file, &window, sizeof(struct window_head));

        u16 crc = CRC16(&window, (u32) & (((struct window_head *)0)->crc_data));
        if (crc == window.crc_head) {
            __this->ui_rotate = rotate[head.rotate];
            ui_core_set_rotate(__this->ui_rotate);
            switch (head.rotate) {
            case 1: /* 旋转90度 */
                __this->ui_hori_mirror = true;
                __this->ui_vert_mirror = false;
                break;
            case 3:/* 旋转270度 */
                __this->ui_hori_mirror = false;
                __this->ui_vert_mirror = true;
                break;
            default:
                __this->ui_hori_mirror = false;
                __this->ui_vert_mirror = false;
                break;
            }
            goto __read_data;
        }
    }

    return NULL;

__read_data:
    ui = (u8 *)jlui_malloc(window.len, UI_RAM_SRAM, UI_MODULE_FRAME);
    if (!ui) {
        return NULL;
    }
    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(__this->ui_file, window.offset, SEEK_SET);
        res_fread(__this->ui_file, ui, window.len);

        u16 crc = CRC16(ui, window.len);
        if (crc == window.crc_data) {
            goto __read_table;
        }
    }

    jlui_free(ui, UI_RAM_SRAM, UI_MODULE_FRAME);
    return NULL;

__read_table:
    ptr_table = (u16 *)jlui_malloc(window.ptr_table_len, UI_RAM_SRAM, UI_MODULE_FRAME);
    if (!ptr_table) {
        jlui_free(ui, UI_RAM_SRAM, UI_MODULE_FRAME);
        return NULL;
    }
    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(__this->ui_file, window.ptr_table_offset, SEEK_SET);
        res_fread(__this->ui_file, ptr_table, window.ptr_table_len);

        u16 crc = CRC16(ptr_table, window.ptr_table_len);
        if (crc == window.crc_table) {
            u16 *offset = ptr_table;
            for (i = 0; i < window.ptr_table_len; i += 2) {
                ptr = (u32 *)(ui + *offset++);
                if (*ptr != 0) {
                    *ptr += (u32)ui;
                }
            }
            jlui_free(ptr_table, UI_RAM_SRAM, UI_MODULE_FRAME);
            return ui;
        }
    }

    jlui_free(ui, UI_RAM_SRAM, UI_MODULE_FRAME);
    jlui_free(ptr_table, UI_RAM_SRAM, UI_MODULE_FRAME);

    return NULL;
}

static void jlui_unload_window(void *ui)
{
    if (ui) {
        jlui_free(ui, UI_RAM_SRAM, UI_MODULE_FRAME);
    }
}

static int lcd_color_format_to_gpu(int lcd_format)
{
    if (lcd_format == LCD_COLOR_RGB888) {
        return GPU_FORMAT_RGB888;
    } else {
        return GPU_FORMAT_RGB565;
    }
}


static int jlui_open_draw_context(struct draw_context *dc)
{
    if (!dc->gpu_task_head) {
        dc->gpu_task_head = jlgpu_create_task_list_head();
    }
    int i;

    if (ui_multi_page_check_id(ui_page2id(ui_id2page(dc->elm->id)))) {
        i = ui_multi_page_get_index_by_id(ui_page2id(ui_id2page(dc->elm->id)));
        dc->index = i;
        __this->dc_flag |= BIT(i);
    } else {
        for (i = 0; i < 2; i++) {
            if (!(__this->dc_flag & BIT(i))) {
                dc->index = i;
                __this->dc_flag |= BIT(i);
                break;
            }
        }
    }

#if (!TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE)
    if (__this->lcd->buffer_malloc) {
        if (__this->buf_cnt++ == 0) {
            __this->lcd->buffer_malloc(&__this->buf, &__this->len);
            ASSERT(__this->buf, "oh buf=null\n");
        }
        ASSERT(__this->buf && __this->len);

        dc->buf = __this->buf;
        dc->len = __this->len;

        printf("malloc disp buf: 0x%p, len:%d, buf_num: %d\n", __this->buf, __this->len, __this->info.buf_num);
        buffer_manager_init(dc->buffer_hdl, __this->buf, __this->len, __this->info.buf_num);
        dc->buffer_adr = get_idle_buffer(dc->buffer_hdl);

    }
#else
    __this->lcd->buffer_malloc(&__this->buf, &__this->len);
    dc->buf = NULL;
    dc->buffer_adr = NULL;
    dc->buffer_hdl = NULL;
    dc->len = __this->len;
#endif//TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE
    dc->width = __this->info.width;
    dc->height = __this->info.height;
    dc->col_align = __this->info.col_align;
    dc->row_align = __this->info.row_align;
    dc->colums = dc->width;
    dc->lines = dc->len / __this->info.buf_num / __this->info.stride;
    dc->lcd_rect.left = 0;
    dc->lcd_rect.top = 0;
    dc->lcd_rect.width = __this->info.width;
    dc->lcd_rect.height = __this->info.height;
    dc->draw_state = 0;
    dc->buf_num = 1;

    /* 设置任务链输出信息 */
    int gpu_out_format = lcd_color_format_to_gpu(__this->info.color_format);
    jlgpu_set_task_list_out_format(dc->gpu_task_head, gpu_out_format);
    /* 创建背景任务 */
    u8 a = 0x00;
    u8 r = (__this->background >> 16) & 0xff;
    u8 g = (__this->background >> 8) & 0xff;
    u8 b = __this->background & 0xff;

    /* jlgpu_task_fill_param_init(0, 0, a, r, g, b); */
    /* memcpy(&task_param.draw, &dc->lcd_rect, sizeof(struct rect));	// 绘制区域是整个LCD区域 */
    /* jlgpu_get_win_rect(&task_param.area);	// 限制区域是整个GPU窗口区域 */
    /* jlgpu_create_task(dc->gpu_task_head, &task_param); */

    dc->gpu_mult_list = jlgpu_scheduler_create_mult_task_list(dc);

    return 0;
}

static int jlui_get_draw_context(struct draw_context *dc)
{
    dc->disp.left  = 0;
    dc->disp.width = dc->width;
    dc->disp.top   = 0;
    dc->disp.height = dc->height;

    return 0;
}

static int jlui_put_draw_context(struct draw_context *dc)
{
    /* printf("%s dc->refresh:%d",__func__,dc->refresh); */
    if (!dc->refresh) {
        if (jlgpu_scheduler_get_redraw_mode() == GPU_ASYN_REDRAW) {
            pJLGPUTaskHead_t head = jlgpu_mult_task_head_idle_by_index(dc->gpu_mult_list, dc->index, dc->gpu_task_head);
            if (head) {
                dc->gpu_task_head = head;
            }
        }
        ASSERT(dc->gpu_task_head);
        return -1;
    }

    if (ui_multi_page_check_index(dc->index)) {
        int list_total = jlgpu_mult_task_list_get_head_num(dc->gpu_mult_list);
        if (list_total == 0) {
            if (jlgpu_scheduler_get_redraw_mode() == GPU_ASYN_REDRAW) {
                pJLGPUTaskHead_t head = jlgpu_mult_task_head_idle_by_index(dc->gpu_mult_list, dc->index, dc->gpu_task_head);
                if (head) {
                    dc->gpu_task_head = head;
                }
            }
            ASSERT(dc->gpu_task_head);
            return -1;
        }

    }

#if LCD_POWER_DOWN_EN
    extern void lcd_reinit_wait_finish(void);
    lcd_reinit_wait_finish();
#endif

    dc->gpu_mult_list = jlgpu_scheduler_draw_mult_task_list(dc);

    if (!dc->gpu_mult_list) {
        ASSERT(0);
        return -1;
    } else {
        return 0;
    }
}


extern int ui_gif_msg_type_get();

void jlui_delete_gif_timer(struct draw_context *dc)
{
    struct gif_timer *p, *n;
    list_for_each_entry_safe(p, n, &gif_timer_list.head, head) {
        if (dc->page == p->page) {
            /* printf("[GIF]%s(), p : 0x%x, p->timer : 0x%x, p->task : 0x%x, dc->page : %d, p->page : %d, element_id : 0x%x, task_id : 0x%x\n", __func__, (u32)p, p->timer_id, (u32)p->task, dc->page, p->page, p->element_id, p->task_id); */
            if (p->timer_id) {
                sys_timer_del(p->timer_id);
            }

            u32 msg_type =  ui_gif_msg_type_get();
            msg_type |= (p->timer_id & 0xffff);
            os_taskq_del_type("ui", msg_type);

            gif_info_del(p->element_id, p->task_id);

            list_del(&p->head);
            free(p);
        }
    }
}

static int jlui_close_draw_context(struct draw_context *dc)
{
    jlui_delete_gif_timer(dc);

    /* 释放GPU任务链链 */
    if (dc->gpu_task_head) {
        jlgpu_scheduler_delete_mult_task_list(dc);
        dc->gpu_task_head = NULL;
    }

    /* 等待GPU和DBI模块空闲，防止BUF使用中被释放 */
    lcd_wait_busy();
    /* buf管理模块和句柄释放 */
    if (dc->buffer_hdl) {
        buffer_manager_free(dc->buffer_hdl);
        dc->buffer_hdl = NULL;
    }
    /* 释放显存buf */
    if (__this->lcd->buffer_free) {
        if (--__this->buf_cnt == 0) {
            /* 释放内存管理buf */
            if (dc->buf) {
                __this->lcd->buffer_free(dc->buf);
                dc->buf = NULL;
                __this->buf = NULL;
                __this->len = 0;
            }
        }
    }
    /* 使用PSRAM缓存UI资源时，每次切换页面就把上一个页面的缓存数据释放掉 */
    /* 如果 dc->refresh 没有使能，说明是3D特效创建GPU任务链，此时不释放缓存，由特效退出时主动释放 */
    if (dc->refresh && (config_gpu_cache_psram_jpeg_en || config_gpu_cache_psram_file_res_en)) {
        gpu_cache_reset();
    }

    __this->dc_flag &= ~BIT(dc->index);
    return 0;
}

static int jlui_set_draw_context(struct draw_context *dc)
{
    return 0;
}

static int jlui_show_element(struct draw_context *dc)
{
    log_info("%s(), elm_id: 0x%x\n", __func__, dc->elm->id);
    jlgpu_task_enable_by_id(dc->gpu_task_head, JLGPU_ID_NONE, dc->elm->id, true);
    return 0;
}

static int jlui_hide_element(struct draw_context *dc)
{
    log_info("%s(), elm_id: 0x%x\n", __func__, dc->elm->id);
    jlgpu_task_enable_by_id(dc->gpu_task_head, JLGPU_ID_NONE, dc->elm->id, false);
    if (!ui_in_effect()) {
        jlgpu_scheduler_wait_sync();
        jlgpu_task_clean_up_by_id(dc->gpu_task_head, dc->elm->id, 0x20);
    }
    return 0;
}

static int jlui_delete_element(struct draw_context *dc)
{
    log_info("%s(), elm_id: 0x%x\n", __func__, dc->elm->id);
    jlgpu_scheduler_wait_sync();

    if (ui_id2type(dc->elm->id) == CTRL_TYPE_TEXT) {
        struct ui_text *text = container_of(dc->elm, struct ui_text, elm);
        if (text->attrs.mulstr) {
            jlui_free((void *)text->attrs.mulstr, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
            text->attrs.mulstr = NULL;
        }

        if (text->attrs.copy_mulstr) {
            jlui_free((void *)text->attrs.copy_mulstr, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
            text->attrs.copy_mulstr = NULL;
        }

        if (text->attrs.font_out_buf) {
            jlui_free((void *)text->attrs.font_out_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
            text->attrs.font_out_buf = NULL;
        }
    }

    void *cur_gpu_head = dc->gpu_task_head;
    jlgpu_task_delete_by_id(dc->gpu_task_head, JLGPU_ID_NONE, dc->elm->id);
    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT == 0) {
        /*删除影子链表*/
        pJLGPUMultTaskList_t mult_list = dc->gpu_mult_list;
__refind:
        pJLGPUMultTaskList_t next_mult = NULL;
        next_mult = jlgpu_mult_task_list_switch_root(mult_list);
        if (next_mult) {
            void *gpu_task_head = jlgpu_mult_task_head_by_index(next_mult, dc->index);
            if (gpu_task_head == cur_gpu_head) {
                mult_list = next_mult;
                goto __refind;
            }
            if (gpu_task_head) {
                jlgpu_task_delete_by_id(gpu_task_head, JLGPU_ID_NONE, dc->elm->id);
            }
        }
    }
    return 0;
}

static int jlui_release_element(struct draw_context *dc)
{
    log_info("%s(), elm_id: 0x%x\n", __func__, dc->elm->id);

    /* 先在任务链中查找，如果没有指定任务，不需要跑删除动作 */
    void *cur_gpu_head = dc->gpu_task_head;
    if (!jlgpu_get_task_by_id(cur_gpu_head, JLGPU_ID_NONE, dc->elm->id)) {
        /* printf("not find task elm id: 0x%x\n", dc->elm->id); */
        return -1;
    }
    /* printf("del task elm id: 0x%x\n", dc->elm->id); */

    /* 等待当前GPU任务合成完毕 */
    jlgpu_scheduler_wait_sync();

    /* 删除当前GPU任务链中指定的GPU任务 */
    jlgpu_task_delete_by_id(cur_gpu_head, JLGPU_ID_NONE, dc->elm->id);

    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT == 0) {
        /* 删除影子链表中指定的GPU任务 */
        pJLGPUMultTaskList_t mult_list = dc->gpu_mult_list;
__refind:
        pJLGPUMultTaskList_t next_mult = NULL;
        next_mult = jlgpu_mult_task_list_switch_root(mult_list);
        if (next_mult) {
            void *gpu_task_head = jlgpu_mult_task_head_by_index(next_mult, dc->index);
            if (gpu_task_head == cur_gpu_head) {
                mult_list = next_mult;
                goto __refind;
            }
            if (gpu_task_head) {
                jlgpu_task_delete_by_id(gpu_task_head, JLGPU_ID_NONE, dc->elm->id);
            }
        }
    }
    return 0;
}



static int jlui_set_element_prior(struct draw_context *dc)
{
    log_info("%s(), elm_id: 0x%x, prior: %d\n", __func__, dc->elm->id, dc->elm->prior);
    jlgpu_task_set_prior_by_id(dc->gpu_task_head, JLGPU_ID_NONE, dc->elm->id, dc->elm->prior, false);
    return 0;
}

static int jlui_fill_rect(struct draw_context *dc, u32 acolor)
{
    log_info("%s(), enter. dc:0x%p, elm_id: 0x%x\n", __func__, dc, dc->elm->id);

    /* u16 id = CRC16(&acolor, 4); */
    u16 id = 0;
    int elm_id = dc->elm->id;
    int task_id = GPU_TASK_ID(dc->page, id, dc->elm_index++);

    u8 a = ((acolor >> 24) & 0xff)/*  * 128 / 100 */;
    u8 r = ((acolor >> 11) & 0x1f) << 3;
    u8 g = ((acolor >> 5) & 0x3f) << 2;
    u8 b = (acolor & 0x1f) << 3;
    log_info("%s(), a: 0x%x, r: 0x%x, g: 0x%x, b: 0x%x\n", __func__, a, r, g, b);

    jlgpu_task_fill_param_init(task_id, elm_id, a, r, g, b);	// 里面创建了task_param结构体

    task_param.priority = dc->elm->prior;

    if (COLOR_VALID(acolor)) {
        log_debug("%s(), fill task is visible, color: 0x%x\n", __func__, acolor);
        task_param.invisible = false;
    } else {
        task_param.invisible = dc->elm->css.invisible;
    }

    /* 缩放使能 */
    JLGPU_TASK_SCALE_CONFIG(&task_param.fill.tran);

    /* 旋转使能 */
    JLGPU_TASK_ROTATE_CONFIG(&task_param.fill.tran);

    if (((!task_param.rotate_en) && (!task_param.scale_en)) || (dc->crop_en)) {
        memcpy(&task_param.draw, &dc->draw, sizeof(struct rect));
    } else {
        memcpy(&task_param.draw, &dc->rect, sizeof(struct rect));
    }
    log_debug("%s(), draw rect[%d, %d, %d, %d]\n", __func__,
              task_param.draw.left, task_param.draw.top, task_param.draw.width, task_param.draw.height);

    /* 继承父控件变换 */
    JLGPU_TASK_INHERIT_PARENT(&task_param.fill.tran);

    log_info("%s(), task_id: 0x%x, elm_id: 0x%x, priority: %d\n", __func__,
             task_param.task_id, task_param.element_id, task_param.priority);
    if (dc->gpu_task_mode == GPU_TASK_MODE_PERSPECTIVE) {
        /* task_param.task_type = GPU_TASK_TRANS | GPU_FILL_PERSPECTIVE; */
        task_param.task_type = GPU_TASK_FILL_TRANS(JLGPU_TRANS_MODE_PROJECTION);
    }
    /* 更新GPU填充任务 */
    jlgpu_task_fill_param_map_707n();
    jlgpu_update_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id, &task_param);

    log_info("%s(), leave.\n", __func__);
    return 0;
}

int gpu_fill_rect(struct draw_context *dc, int left, int top, int width, int height, u32 acolor)
{
    u16 id = CRC16(&acolor, 4);
    int elm_id = dc->elm->id;
    int task_id = GPU_TASK_ID(dc->page, id, dc->elm_index++);

    u8 a = ((acolor >> 24) & 0xff)/*  * 255 / 100 */;
    u8 r = ((acolor >> 11) & 0x1f) << 3;
    u8 g = ((acolor >> 5) & 0x3f) << 2;
    u8 b = (acolor & 0x1f) << 3;

    jlgpu_task_fill_param_init(task_id, elm_id, a, r, g, b);

    task_param.draw.left = left;
    task_param.draw.top = top;
    task_param.draw.width = width;
    task_param.draw.height = height;
    memcpy(&task_param.area, &task_param.draw, sizeof(struct rect));
    jlgpu_shift_rect(&task_param.area);
    task_param.priority = dc->elm->prior;
    /* printf("%s task_id:%x elm:%x",__func__,task_param.task_id,task_param.element_id); */

    if (dc->gpu_task_mode == GPU_TASK_MODE_PERSPECTIVE) {
        /* task_param.task_type = GPU_TASK_TRANS | GPU_TEXTURE_PERSPECTIVE; */
        task_param.task_type = GPU_TASK_IMAGE_TRANS(JLGPU_TRANS_MODE_PROJECTION);
    } else {
        task_param.task_type = GPU_TASK_FILL;
    }
    int global_alpha =  dc->elm->css.alpha * 128 / 100;
    task_param.global_alpha = global_alpha;

    /* 缩放使能 */
    JLGPU_TASK_SCALE_CONFIG(&task_param.fill.tran);

    /* 旋转使能 */
    JLGPU_TASK_ROTATE_CONFIG(&task_param.fill.tran);

    /* 继承父控件变换 */
    JLGPU_TASK_INHERIT_PARENT(&task_param.fill.tran);


    /* 更新GPU填充任务 */
    jlgpu_task_fill_param_map_707n();
    jlgpu_update_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id, &task_param);

    return 0;
}


int ui_gif_play_post(u16 timer_id, u32 msec, u32 delay, u32 elm_id, u32 task_id);

int gif_play_flag = 0;

void gif_play(int elm_id, int task_id)
{
    struct element *elm = ui_core_get_element_by_id(elm_id);
    if (elm) {
        ui_core_redraw(elm);
    }
}

static void gif_timer_cb(void *priv)
{
    extern int dial_sel_state();
    if (dial_sel_state()) {
        return;
    }
    struct gif_info *gif_info = (struct gif_info *)priv;
    ASSERT(gif_info->gif_delay > 0);
    ui_gif_play_post(gif_info->gif_timer_id, jiffies_msec(), gif_info->gif_delay, gif_info->element_id, gif_info->task_id);

    gif_info->gif_frame_index++;
    /* 到最后一帧帧之后，如果配置了单次播放，则停留在最后一帧 */
    if (gif_info->gif_frame_index > gif_info->gif_frame_num && gif_info->play_single) {
        gif_info->gif_frame_index = gif_info->gif_frame_num;
        return;
    }
    if (gif_info->gif_frame_index > gif_info->gif_frame_num) {
        gif_info->gif_frame_index = 1;
    }
}
static int jlui_draw_image(struct draw_context *dc, u32 src, u8 quadrant, u8 *ext_parm)
{
    int id;
    int page;
    UI_RESFILE *fp = NULL;
    struct flash_file_info *info  = NULL;
    log_info("%s(), enter. dc: 0x%p\n", __func__, dc);
    if (dc->elm->css.invisible) {
        printf("%s() %d, image is invisible\n", __func__, __LINE__);
        return -1;
    }

    if (dc->preview.file) {
        fp = dc->preview.file;
        id = dc->preview.id;
        page = dc->preview.page;
        info = dc->preview.file_info;
    } else if (dc->draw_img.en) {
        fp = NULL;
        id = dc->draw_img.id;
        page = dc->draw_img.page;
        info = &ui_load_info_table[dc->prj].image_file_info;
    } else {
        fp = NULL;
        id = src;
        page = dc->page;
        info = &ui_load_info_table[dc->prj].image_file_info;
    }
    /* printf("@@@@@@@@ %s, %d, dc: 0x%p, elm_id:0x%08x id:%x\n", __func__, __LINE__, dc, dc->elm->id,id); */

    if (((u16) - 1 == id) || ((u32) - 1 == id)) {
        printf("%s() %d, image id[0x%08x] err\n", __func__, __LINE__, id);
        return -1;
    }

    int elm_id = dc->elm->id;
    u8 elm_index = 1;
    elm_index = dc->elm_index++;
    int task_id = GPU_TASK_ID(page, id, elm_index);
    jlgpu_task_texture_param_init(task_id, elm_id);
    if (dc->gpu_task_mode == GPU_TASK_MODE_PERSPECTIVE) {
        /* task_param.task_type = GPU_TASK_TRANS | GPU_TEXTURE_PERSPECTIVE; */
        task_param.task_type = GPU_TASK_IMAGE_TRANS(JLGPU_TRANS_MODE_PROJECTION);
    } else {
        task_param.task_type = GPU_TASK_IMAGE;
    }
    int global_alpha =  dc->elm->css.alpha * 128 / 100;
    task_param.global_alpha = global_alpha;
    int err = 0;
    if (!ext_parm) {
        err = jlui_res_get_image_info(dc->prj, (page << 16 | id), fp, &task_param.image, &task_param.info);
    } else {
        info = NULL;
        struct image_file *img = (struct image_file *)ext_parm;
        memcpy(&task_param.image, img, sizeof(struct image_file));
        task_param.texture.not_compress = !task_param.image.compress;
        task_param.info.offset = task_param.image.offset;
        if (dc->custom_color & BIT(UI_CUSTOM_COLOR_BIT_IMAGE)) {
            memcpy(&task_param.texture.color, &dc->custom_argb8888, sizeof(int));
        }
        if (img->format == PIXEL_FMT_A1) {
            jlgpu_task_img_set_endian(1);
        }
        int gpu_format = image_file_get_gpu_format(img);
        if (gpu_format  == GPU_FORMAT_YUV420_BT601 || gpu_format == GPU_FORMAT_YUV420_BT709) {
            int width = img->width;
            int height = img->height;
            u32 uv_adr = task_param.info.offset + width * height;
            jlgpu_task_img_set_uv_adr(dc->gpu_task_head, task_id, elm_id, dc->elm->prior, ((u8 *)uv_adr));
            elm_index = dc->elm_index++;
            task_id = GPU_TASK_ID(page, id, elm_index);
            task_param.task_id = task_id;
            jlgpu_task_img_set_compress(0);
        }

    }
    if (err) {
        printf("%s() %d, read file err\n", __func__, __LINE__);
        return -EFAULT;
    }
    if (dc->custom_color & BIT(UI_CUSTOM_COLOR_BIT_IMAGE)) {
        if ((task_param.image.format == PIXEL_FMT_L1)
            || (task_param.image.format == PIXEL_FMT_A8)
            || (task_param.image.format == PIXEL_FMT_A4)
            || (task_param.image.format == PIXEL_FMT_A2)
            || (task_param.image.format == PIXEL_FMT_A1)) {
            task_param.custom_color = true;
            task_param.alpha = (dc->custom_argb8888 >> 24) & 0xff;
            task_param.red   = (dc->custom_argb8888 >> 16) & 0xff;
            task_param.green = (dc->custom_argb8888 >> 8) & 0xff;
            task_param.blue  = (dc->custom_argb8888) & 0xff;
            /* task_param.alpha = dc->alpha * 255 / 100; */
            /* task_param.red   = RGB565_R(dc->custom_rgb565); */
            /* task_param.green = RGB565_B(dc->custom_rgb565); */
            /* task_param.blue  = RGB565_B(dc->custom_rgb565); */
            /* log_info("custom_color: 0x%x, a: 0x%x, r: 0x%x, g: 0x%x, b: 0x%x\n", \ */
            /* dc->custom_argb8888, task_param.alpha, task_param.red, task_param.green, task_param.blue); */
        } else {
            task_param.custom_color = false;
            log_warn("%s(), image format err: %d, Only L1, A8, A4, A2, A1 can modified colors!\n", __func__, task_param.image.format);
        }
    } else {
        task_param.custom_color = false;
    }
    if (((task_param.image.format == PIXEL_FMT_AL88)
         || (task_param.image.format == PIXEL_FMT_AL44)
         || (task_param.image.format == PIXEL_FMT_AL22)
         || (task_param.image.format == PIXEL_FMT_L8)
         || (task_param.image.format == PIXEL_FMT_L4)
         || (task_param.image.format == PIXEL_FMT_L2)
         || (task_param.image.format == PIXEL_FMT_L1)
         || (task_param.image.format == PIXEL_FMT_A8)
         || (task_param.image.format == PIXEL_FMT_A4)
         || (task_param.image.format == PIXEL_FMT_A2)
         || (task_param.image.format == PIXEL_FMT_A1)
        ) && (!ext_parm)) {
        int tab_size = get_clut_format_tabsize(task_param.image.format, task_param.image.clut_format);
        task_param.image.offset += tab_size;
        task_param.image.len -= tab_size;
    }
    if (!ext_parm) {
        err = ui_res_get_image_flash_info(&task_param.info, info, &task_param.image);
    }
    if (__this->cache_mmu && (dc->prj == 1)) {
        if (info->tab) {
            char file_name[17] = {0};
            char *prj_path = NULL;
            if (fp) {
                fget_name(fp->file, (u8 *)file_name, 13);
                prj_path = file_name;
            } else {
                prj_path = ui_get_res_path_by_pj_id(dc->prj);
            }
            u32 res_hash = 5383;
            res_hash =  mmu_hash(res_hash, (u8 *)prj_path, strlen(prj_path));
            u32 val[] = {res_hash, task_param.element_id, task_param.task_id};
            res_hash = ui_prime_hash_int(val, ARRAY_SIZE(val));
            /* struct mmu_tab_cache *c = mmu_flash_info_cache_add(info, info->tab_size, res_hash); */
            struct mmu_tab_cache *c = mmu_flash_info_cache_add(&task_param.info, info->tab_size, res_hash);
            info = (struct flash_file_info *) & (c->tab);
        }
    }


    /* 缩放使能 */
    JLGPU_TASK_SCALE_CONFIG(&task_param.texture.tran);

    /* 旋转使能 */
    JLGPU_TASK_ROTATE_CONFIG(&task_param.texture.tran);

    /* 获取图片实际宽高 */
    struct rect image_rect;
    image_rect.width = task_param.image.width;
    image_rect.height = task_param.image.height;
    if (task_param.scale_en && dc->elm->css.part) {
        image_rect.width *= task_param.texture.tran.ratio_w;
        image_rect.height *= task_param.texture.tran.ratio_h;
    }
    if (dc->draw_img.en) {
        struct rect draw_img_rect;
        draw_img_rect.left = dc->draw_img.x;
        draw_img_rect.top  = dc->draw_img.y;
        draw_img_rect.width = task_param.image.width;
        draw_img_rect.height = task_param.image.height;
        if (!get_rect_align(&draw_img_rect, &image_rect, dc->hori_align, dc->vert_align)) {
            return -EINVAL;
        }
    } else if (!get_rect_align(&dc->rect, &image_rect, dc->hori_align, dc->vert_align)) {
        return -EINVAL;
    }
    /* DUMP_RECT(__func__, __LINE__, "img", &image_rect); */
    /* DUMP_RECT(__func__, __LINE__, "dc rect", &dc->rect); */
    /* DUMP_RECT(__func__, __LINE__, "dc draw", &dc->draw); */
    /* 计算图片裁剪和绘制区域 */
    task_param.crop_en = dc->crop_en;
    if ((!task_param.scale_en && !task_param.rotate_en) || task_param.crop_en) {
        if (get_rect_crop(&dc->draw, &image_rect, &task_param.texture.crop)) {
            task_param.crop_en = true;
        }
        get_rect_cover(&dc->draw, &image_rect, &task_param.draw);
    } else {
        if (get_rect_crop(&dc->rect, &image_rect, &task_param.texture.crop)) {
            task_param.crop_en = true;
        }
        get_rect_cover(&dc->rect, &image_rect, &task_param.draw);
    }

    struct gif_frame_info gif_frame_info = {0};
    struct gif_file_info gif_file_info = {0};
    struct gif_file gif_file = {0}; /*注意要清零*/
    struct gif_info *gif_info = NULL;
    u32 gif_keyframe_addr = 0;

    if (task_param.scale_en && task_param.crop_en) {
        /* DUMP_RECT(__func__, __LINE__, "img", &image_rect); */
        /* DUMP_RECT(__func__, __LINE__, "dc rect", &dc->rect); */
        /* DUMP_RECT(__func__, __LINE__, "dc draw", &dc->draw); */
        gpu_matrix_t matrix;
        gpu_matrix_set_identity(&matrix);
        gpu_matrix_scale(&matrix, task_param.texture.tran.ratio_w, task_param.texture.tran.ratio_h);
        gpu_matrix_translate(&matrix, image_rect.left, image_rect.top);
        gpu_boundbox_t bbox;
        bbox.minx = task_param.draw.left;
        bbox.miny = task_param.draw.top;
        bbox.maxx = task_param.draw.left + task_param.draw.width;
        bbox.maxy = task_param.draw.top + task_param.draw.height;

        /* DUMP_RECT(__func__,__LINE__,"bbox1",((struct rect *)&bbox)); */
        /* void dump_matrix(gpu_matrix_t *matrix); */
        /* dump_matrix(&matrix); */
        gpu_matrix_get_boundbox(&matrix, &bbox, &bbox);
        /* DUMP_RECT(__func__,__LINE__,"bbox2",((struct rect *)&bbox)); */
        task_param.texture.crop.top  = bbox.miny;
        task_param.texture.crop.left  = bbox.minx;
        task_param.texture.crop.width = bbox.maxx - bbox.minx;
        task_param.texture.crop.height = bbox.maxy - bbox.miny;
    }
    if ((is_jljpeg(&task_param.image)) && (!config_gpu_cache_psram_jpeg_en)) {
        /* if (task_param.rotate_en) { */
        /* task_param.rotate_en = false;	// jpeg 暂不支持旋转 */
        /* } */
        /* JLGPU_TASK_SCALE_CONFIG(&task_param.fill.tran); */
        /* get_rect_cover(&dc->rect, &image_rect, &task_param.draw); */
        task_param.task_type = GPU_TASK_JPEG;
        /* task_param.cb_func = jpeg_draw_cb; */
        /* task_param.cb_exit = jpeg_draw_cb_exit; */
        /* task_param.priv = jpeg_module_opj_create(&task_param.info, sizeof(struct flash_file_info), task_param.image.data_crc, dc->index); */
        /* task_param.priv_len = sizeof(jdec_opj); */
        /* task_param.draw_en = 1; */
        JLGPU_TASK_INHERIT_PARENT(&task_param.texture.tran);
        task_param.clut_format = task_param.image.clut_format;
        task_param.has_clut = task_param.image.has_clut;
        struct jljpeg_head *jpg_head = (struct jljpeg_head *)task_param.info.offset;
        /* jpg_dump_jljpg_info((u32)jpg_head); */
        u32 jpg_addr = (u32)task_param.info.offset + (u32)jpg_head->file_info[0].addr;
        /* jpg_dump_info(jpg_addr); */
        /* put_buf((u8 *)jpg_addr, jpg_head->file_info[0].len); */
        task_param.texture.data = (u8 *)jpg_addr;
        /* task_param.texture.data = (u8 *)task_param.info.offset; */
        task_param.texture.mmu_tab_base = (u8 *)task_param.info.tab;
    } else if ((is_jljpeg(&task_param.image)) && (config_gpu_cache_psram_jpeg_en)) {
        task_param.task_type = GPU_TASK_JPEG;
        JLGPU_TASK_INHERIT_PARENT(&task_param.texture.tran);
        task_param.clut_format = task_param.image.clut_format;
        task_param.has_clut = task_param.image.has_clut;
        task_param.texture.data = (u8 *)task_param.info.offset;
        task_param.texture.mmu_tab_base = (u8 *)task_param.info.tab;
    } else if (task_param.image.format == PIXEL_FMT_GIF) {
        gif_info = gif_info_get(task_param.element_id, task_param.task_id);
        if (gif_info) {
            if (gif_info->gif_timer_id) {
                task_param.texture.mmu_tab_base = gif_info->gif_keyframe_mmu_tab_base;
                task_param.texture.data = gif_info->gif_keyframe_addr;
                task_param.image.len = gif_info->gif_keyframe_size;
            }
            if (task_param.clut_tab) {
                memcpy(task_param.clut_tab, gif_info->gif_palette1, 256 * 3);
            }
        } else {
            gif_file.fp =  fp ? fp : ui_load_res_by_pj_id(dc->prj);;
            gif_file.offset = task_param.image.offset;
            gif_file.len = task_param.image.len;
            gif_file.flash_addr = task_param.info.offset;
            gif_file.file_info = info;
            gif_read_head_info(&gif_file, &gif_file_info, &gif_frame_info);
            // ASSERT(!gif_file.file_info);

            if (!gif_file_info.delay) {
                gif_file_info.delay = 100;
            }

            task_param.image.width = gif_frame_info.width;
            task_param.image.height = gif_frame_info.height;

            struct rect rect;
            rect.left = image_rect.left + gif_frame_info.x;
            rect.top = image_rect.top + gif_frame_info.y;
            rect.width = gif_frame_info.width;
            rect.height = gif_frame_info.height;

            task_param.crop_en = false;
            if (get_rect_crop(&dc->draw, &rect, &task_param.texture.crop)) {
                task_param.crop_en = true;
            }
            get_rect_cover(&dc->draw, &rect, &task_param.draw);

            if (gif_file.file_info) {
                struct image_file image;
                image.offset = task_param.image.offset + gif_frame_info.addr;
                image.len = gif_frame_info.size;
                ui_res_get_image_flash_info(&task_param.info, gif_file.file_info, &image);
                task_param.texture.data = (u8 *)task_param.info.offset;
                task_param.image.len = gif_frame_info.size;
            } else {
                task_param.texture.data = (u8 *)(task_param.info.offset + gif_frame_info.addr);
            }
            gif_keyframe_addr = (u32)task_param.texture.data;
        }
        task_param.has_clut = CLUT_TAB_FROM_RAM;
        task_param.clut_tab = (u8 *)zalloc(gpu_format_to_lut_size(GPU_FORMAT_L8, GPU_CLUT_FORMAT_ARGB8565));
        task_param.format = res_format_to_gpu(PIXEL_FMT_L8);
        task_param.task_type = GPU_TASK_IMAGE;
        task_param.clut_format = GPU_CLUT_FORMAT_ARGB8565;
        task_param.texture.mmu_tab_base = (u8 *)task_param.info.tab;
        JLGPU_TASK_INHERIT_PARENT(&task_param.texture.tran);
    } else {
        task_param.format = image_file_get_gpu_format(&task_param.image);
        JLGPU_TASK_INHERIT_PARENT(&task_param.texture.tran);
        task_param.clut_format = task_param.image.clut_format;
        task_param.has_clut = task_param.image.has_clut;
        if (__this->cache_mmu && (dc->prj == 1) && info->tab) {
            task_param.texture.data = (u8 *)info->offset;
            task_param.texture.mmu_tab_base = (u8 *)info->tab;
        } else {
            task_param.texture.data = (u8 *)task_param.info.offset;
            task_param.texture.mmu_tab_base = (u8 *)task_param.info.tab;
        }
    }

    task_param.priority = dc->elm->prior;
    log_info("%s(), task_id: 0x%x, elm_id: 0x%x, priority: %d\n", __func__,
             task_param.task_id, task_param.element_id, task_param.priority);

#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
    if (!fp) {
        fp = ui_load_info_table[dc->prj].res;
    }
    if (task_param.image.format == PIXEL_FMT_JPEG) {
        if (config_gpu_cache_psram_jpeg_en) {
            if (task_param.image.format == PIXEL_FMT_JPEG) {
                /* task_param.texture.data = cache_gpu_input_jpeg_data(dc->gpu_task_head, &task_param, fp, dc->index); */
            }
        }
    } else {
        task_param.texture.data = cache_gpu_input_data(dc->gpu_task_head, &task_param, (void *)fp, dc->index);
    }
#else
    if (config_gpu_cache_psram_jpeg_en) {
        if (task_param.image.format == PIXEL_FMT_JPEG) {
            /* task_param.texture.data = cache_gpu_input_jpeg_data(dc->gpu_task_head, &task_param, NULL, dc->index); */
        }
    }
#endif
    /* DUMP_RECT(__func__, __LINE__, "draw", &task_param.draw); */
    /* DUMP_RECT(__func__, __LINE__, "crop", &task_param.crop); */
    /* DUMP_RECT(__func__, __LINE__, "area", &task_param.area); */
    /* printf("@@@ %s(), w: %d, h: %d, format: %d, compress: %d, clut_format: %d\n", __func__, task_param.image.width, task_param.image.height, task_param.image.format, task_param.image.compress, task_param.image.clut_format); */
    jlgpu_task_texture_param_map_707n();
    pJLGPUTaskUnit_t taskp = jlgpu_update_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id, &task_param);
//打印任务节点信息
    void jlgpu_chain_dump_task_unit(pJLGPUChainTaskUnit_t task, int count);
    //jlgpu_chain_dump_task_unit(taskp, 0);


    if (dc->draw_img.invert_x_en || dc->draw_img.invert_y_en) {
        jlui_image_invert_deal(dc, taskp);
    }

    if (task_param.image.format == PIXEL_FMT_GIF) {
        if (gif_info) {
            gif_info->head = dc->gpu_task_head;
            gif_info->matrix = task_param.matrix;
            memcpy(&gif_info->draw, &dc->draw, sizeof(struct rect));
            memcpy(&gif_info->rect, &image_rect, sizeof(struct rect));
            int ret = gif_decode(task_param.element_id, task_param.task_id);
        } else {
            get_rect_cover(&dc->rect, &image_rect, &task_param.draw);
            struct gif_param gif_param = {0};
            gif_param.file = &gif_file;
            gif_param.file_info = &gif_file_info;
            gif_param.frame_info = &gif_frame_info;
            gif_param.p = taskp;
            gif_param.gif_timer_cb = gif_timer_cb;
            gif_param.head = dc->gpu_task_head;
            gif_param.gpu_param = &task_param;
            gif_param.rect = &image_rect;
            gif_param.draw = &dc->draw;
            gif_param.fp = (void *)fp;
            gif_param.index = dc->index;
            gif_param.keyframe_addr = gif_keyframe_addr;
#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
            gif_param.cache_data_cb = cache_gpu_input_data;
#endif

            struct gif_timer *gif_timer = gif_decode_init(&gif_param);
            if (gif_timer) {
                gif_timer->page = dc->page;
                list_add_tail(&gif_timer->head, &gif_timer_list.head);
            }
        }
    }

#if  0//资源校验demo，默认不开
    if ((taskp) && (task_param.image.format != PIXEL_FMT_JPEG)) {
        /* int res_len = jlgpu_task_get_res_len((void*)&taskp->task_addr); */
        /* if(res_len > 0){ */
        /* ASSERT(res_len <= task_param.image.len,"res_len:%d img_len:%d",res_len, task_param.image.len); */
        /* } */
        int res_len = 0;
        void *res_addr = jlgpu_task_get_res_info((void *)&taskp->task_addr, &res_len);
        if ((res_len > 0) && (res_addr != NULL)) {
            ASSERT(res_len <= task_param.image.len, "res_len:%d img_len:%d\n", res_len, task_param.image.len);
            if (task_param.texture.mmu_tab_base) {
                ASSERT((u32)res_addr == (u32)task_param.texture.mmu_tab_base, "res_addr:0x%x param_addr:0x%x\n", (u32)res_addr, (u32)task_param.texture.mmu_tab_base);
            } else {
                ASSERT((u32)res_addr == (u32) task_param.texture.data, "res_addr:0x%x param_addr:0x%x\n", (u32)res_addr, (u32)task_param.texture.data);
            }
        }
    }
#endif
    log_info("%s(), leave. dc: 0x%p taskp:%x\n", __func__, dc, (u32)taskp);
    return 0;
}

void font_get_text_width(u8 encode, struct font_info *info, u8 *str, u16 strlen)
{
    if (encode == FONT_ENCODE_ANSI) {
        font_text_width(info, str, strlen);
    } else if (encode == FONT_ENCODE_UNICODE) {
        font_textw_width(info, str, strlen);
    } else {
        font_textu_width(info, str, strlen);
    }
}

u16 font_text_out(u8 encode, struct font_info *info, u8 *str,  u16 strlen,  u16 x,  u16 y)
{
    u16 len;
    if (encode == FONT_ENCODE_ANSI) {
        len = font_textout(info, str, strlen, x, y);
    } else if (encode == FONT_ENCODE_UNICODE) {
        len = font_textout_unicode(info, str, strlen, x, y);
    } else {
        len = font_textout_utf8(info, str, strlen, x, y);
    }

    return len;
}

struct font_info *text_font_init(u8 init)
{
    static struct font_info *info = NULL;
    static int language = -1;

    if (init) {
        if (!info || (language != font_lang_get())) {
            language = font_lang_get();
            if (info) {
                font_close(info);
            }
            info = font_open(NULL, language);
            ASSERT(info, "font_open fail!");
        }
    } else {
        if (info) {
            font_close(info);
            info = NULL;
        }
    }
    return info;
}


struct image_unit {
    u16 image_id;		// 图片ID
    u16 invisible: 1;	// 是否隐藏
    u16 crop_en: 1;		// 是否裁剪
    u16 used_ram: 1;	// 是否在用RAM缓存
    u32 color;
    float offset_x;
    struct rect rect;	// 图片原本的大小位置
    struct rect draw;	// 图片绘制的大小位置
    struct rect crop;	// 图片裁剪的大小位置
    struct image_file image;	// 图片信息
    struct flash_file_info flash;	// flash信息
    struct image_unit *next;
};

struct image_list {
    struct rect rect;	// 控件的矩形区域
    int image_num;		// 总的图片数量
    int image_width;	// 总的图片宽度
    int image_height;	// 总的图片高度
    struct image_unit *img;	// 图片信息列表
};


static int jlui_show_text_text(struct draw_context *dc, struct ui_text_attrs *text, struct image_list *img_list)
{
    struct font_info *info = NULL;
    u16 curr_crc;
    u8 roll_step = 6;
    u8 strpic_mode = UI_TEXT_STRPIC_MODE_IMAGE;
    struct ui_text *text_elm = (struct ui_text *)dc->elm;

    int interval = font_scroll_circular_interval;

    if ((text->format == UI_TEXT_ENCODE_STRPIC) || (text->format == UI_TEXT_ENCODE_MULSTR)) {
        strpic_mode = read_string_type(dc->prj, *((u16 *)text->str));
    }

    if ((text->format == UI_TEXT_ENCODE_TEXT) || \
        ((text->format == UI_TEXT_ENCODE_STRPIC || text->format == UI_TEXT_ENCODE_MULSTR) && strpic_mode == UI_TEXT_STRPIC_MODE_ENCODE)) {

        char *text_str = (char *)text->str;
        u16 text_strlen = text->strlen;
        u8 text_encode = text->encode;
        u8 text_flags = text->flags;
        u8 text_endian = text->endian;

        /* mulstr strpic_mode == 2 */
        char *text_buf = NULL;
        if ((text->format == UI_TEXT_ENCODE_STRPIC || text->format == UI_TEXT_ENCODE_MULSTR) && strpic_mode == UI_TEXT_STRPIC_MODE_ENCODE) {
            struct image_file file;
            /* strpic_mode */
            if (text->format == UI_TEXT_ENCODE_STRPIC) {
                u16 id = *((u16 *)text_str);
                if (open_string_pic(dc->prj, &file, id)) {
                    return -EFAULT;
                }
                if (!file.width || !file.height || !file.len) {
                    return -EFAULT;
                }
                text_buf = jlui_malloc(file.len, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                if (!text_buf) {
                    return -EFAULT;
                }
                read_str_data(&file, (u8 *)text_buf, file.len);
                /*拿到显示文本*/
                text_str = &text_buf[2];
                text_strlen = file.len - 2;

                text_flags |= FONT_SHOW_PIXEL;

                if (file.width > dc->rect.width) {
                    text_flags |= FONT_SHOW_SCROLL;
                    if (strpic_scroll_circular_deafult_enable) {
                        text_flags |= FONT_SCROLL_CIRCULAR;
                    }
                    text_elm->scroll = true;
                } else {
                    text_flags &= ~FONT_SHOW_SCROLL;
                    text_flags &= ~FONT_SCROLL_CIRCULAR;
                    text_elm->scroll = false;
                }

                text->flags = text_flags;
            }
            /* strpic_mode */

            /* mulstr_mode */
            if (text->format == UI_TEXT_ENCODE_MULSTR) {
                u16 *p = (u16 *)text->str;
                int text_buf_len = 128;
                text_buf = jlui_malloc(text_buf_len, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                u16 offset = 0;
                char *tmp_buf;
                while (*p != 0) {
                    if (open_string_pic(dc->prj, &file, *p)) {
                        return -EFAULT;
                    }
                    tmp_buf = jlui_malloc(file.len, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                    if (!tmp_buf) {
                        return -EFAULT;
                    }
                    read_str_data(&file, (u8 *)tmp_buf, file.len);
                    if (offset + file.len - 2 >= text_buf_len) {
                        //一般不超过128，确有长字符的，把text_buf_len 改大
#if 1
                        ASSERT(0);
#else
                        log_warn("%s %d strlen is more than text_buf_len \n", __func__, __LINE__);
#endif

                        jlui_free((void *)tmp_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                        break;
                    }
                    memcpy(text_buf + offset, &tmp_buf[2], file.len - 2);
                    jlui_free((void *)tmp_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                    offset += (file.len - 2);
                    p++;
                }
                text_str = text_buf;
                text_strlen = offset;
                text_flags |= (FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE);
            }
            /* mulstr_mode */

            text_encode = FONT_ENCODE_UNICODE;
            text_endian = FONT_ENDIAN_SMALL;
        }
        /* mulstr strpic_mode == 2 */

        if (text_str == NULL || text_strlen == 0) {
            printf("func: %s(), line: %d text param null id:%x\n", __func__, __LINE__, dc->elm->id);
            jlgpu_task_delete_by_id(dc->gpu_task_head, JLGPU_ID_NONE, dc->elm->id);
            return -EFAULT;
        }
        info = text_font_init(true);
        if (info == NULL || info->sta != FT_ERROR_NONE) {
            printf("func: %s(), line: %d font_info init err\n", __func__, __LINE__);
            /* mulstr strpic_mode == 2 */
            if ((text->format == UI_TEXT_ENCODE_STRPIC || text->format == UI_TEXT_ENCODE_MULSTR) && strpic_mode == UI_TEXT_STRPIC_MODE_ENCODE) {
                if (text_buf) {
                    jlui_free((void *)text_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                }
            }
            /* mulstr strpic_mode == 2 */
            return -EFAULT;
        }
        info->flags = text_flags;
        info->text_width  = dc->rect.width;
        info->text_height = dc->rect.height;

        u8 language_change = 0;
        static u8 prev_language = 0;
        if (prev_language != info->language_id) {
            language_change = 1;
            prev_language = info->language_id;
        }
        info->default_code = text->default_code;
        info->line_space = text->line_space;
        info->word_space = text->word_space;
        info->extra_word_space_for_dep = text->extra_word_space_for_dep;
        info->top_extra_fill = text->top_extra_fill;
        info->bottom_extra_fill = text->bottom_extra_fill;

        /* >>>>>>>>>>> FONT FREETYPE >>>>>>>>>>> */
        if (FONT_FREETYPE_SWITCH) {
            info->gpu_head = dc->gpu_task_head;
            info->elm_id = dc->elm->id;
            info->task_id = 0;
            info->alpha = dc->alpha;
            info->rgb565 = text->color;
        }
        /* <<<<<<<<<<< FONT FREETYPE <<<<<<<<<<< */

        /* if (text->copy_mulstr) { */
        /* jlui_free((void *)text->copy_mulstr, UI_RAM_SRAM, UI_MODULE_FRAME); */
        /* text->copy_mulstr = NULL; */
        /* } */

        curr_crc = CRC16(text_str, text_strlen);

        if ((curr_crc != text->last_crc) || language_change || (info->flags & FONT_VERTICAL_SCROLL)) {
            if (info->flags & FONT_SHOW_SCROLL) {
                info->text_width = 0xffff;
            }
            if ((info->flags & FONT_SHOW_MULTI_LINE) && (dc->hori_align == HORI_ALIGN_CENTER)) {
                font_create_each_line_width_info(info);
            }
            if (text_encode == FONT_ENCODE_UNICODE) {
                if (text_endian == FONT_ENDIAN_BIG) {
                    info->bigendian = true;
                } else {
                    info->bigendian = false;
                }
            }
            font_get_text_width(text_encode, info, (u8 *)text_str, text_strlen);
            if ((info->string_width > 1024) && (info->flags & FONT_SHOW_SCROLL)) {
                printf("warning! info->string_width == %d", info->string_width);
            }
            text->last_crc = curr_crc;
            text->width = info->string_width > 1024 ? 1024 : info->string_width;
            text->height = info->string_height;
            if (info->flags & FONT_SHOW_SCROLL) {
                info->text_width = text->width > dc->rect.width ? text->width : dc->rect.width;
            }
            if (text->width == 0 || text->height == 0) {
                printf("string width or height == 0\n");
                if ((info->flags & FONT_SHOW_MULTI_LINE) && (dc->hori_align == HORI_ALIGN_CENTER)) {
                    font_release_each_line_width_info(info);
                }
                /* mulstr strpic_mode == 2 */
                if ((text->format == UI_TEXT_ENCODE_STRPIC || text->format == UI_TEXT_ENCODE_MULSTR) && strpic_mode == UI_TEXT_STRPIC_MODE_ENCODE) {
                    if (text_buf) {
                        jlui_free((void *)text_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                    }
                }
                /* mulstr strpic_mode == 2 */
                return -EFAULT;
            }

            if (text->width > dc->rect.width) {
                if (text->flags & FONT_SCROLL_CIRCULAR) {
                    text->displen = text->width + interval;
                    text_elm->scroll = true;
                } else if (info->flags & FONT_SHOW_SCROLL) {
                    if (text->format == UI_TEXT_ENCODE_STRPIC) {
                        text->displen = text->width; //文本完全移出控件再回滚
                    } else {
                        text->displen = text->width - dc->rect.width;//最后一个字符出现后回滚
                    }
                    text_elm->scroll = true;

                } else {
                    text->width = dc->rect.width;
                    text_elm->scroll = false;
                    text->displen = 0;
                }
            } else {
                text_elm->scroll = 0;
                text->displen = 0;
            }

            if (text->height > dc->rect.height) {
                text->height = dc->rect.height;
            }

            if (font_lang_get() == Arabic || font_lang_get() == Hebrew || font_lang_get() == UnicodeMixLeftword) { //显示方向从右到左
                if (dc->rect.width > text->width) {
                    if (dc->hori_align == HORI_ALIGN_CENTER) {
                        info->text_width = dc->rect.width - (dc->rect.width - text->width) / 2;
                    }
                    text->width = dc->rect.width;
                }
            }

            /* >>>>>>>>>>> FONT FREETYPE >>>>>>>>>>> */
            if (FONT_FREETYPE_SWITCH) {
                if (dc->hori_align == HORI_ALIGN_CENTER) {
                    info->x = (info->text_width - text->width) / 2 + dc->rect.left;
                } else {
                    info->x = dc->rect.left;
                }
                if (dc->vert_align == VERT_ALIGN_CENTER) {
                    info->y = (info->text_height - text->height) / 2 + dc->rect.top;
                } else {
                    info->y = dc->rect.top;
                }
            }
            /* <<<<<<<<<<< FONT FREETYPE <<<<<<<<<<< */

            info->text_image_width = text->width;
            info->text_image_height = text->height;
            if ((info->tool_version == 0) || (info->tool_version == 0x0100)) {
                info->text_image_stride = (text->width + 7) / 8;
            } else {
                if (Font_GetBitDepth() == BIT_DEPTH_1BPP) {
                    info->text_image_stride = (text->width + 7) / 8;
                } else if (Font_GetBitDepth() == BIT_DEPTH_2BPP) {
                    info->text_image_stride = (text->width * 2 + 7) / 8;
                } else if (Font_GetBitDepth() == BIT_DEPTH_4BPP) {
                    info->text_image_stride = (text->width * 4 + 7) / 8;
                } else {
                    info->text_image_stride = text->width;
                }
            }
            info->text_image_buf_size = info->text_image_stride * text->height;
            info->text_image_buf = (u8 *)jlui_zalloc(info->text_image_buf_size);
            font_text_out(text_encode, info, (u8 *)text_str, text_strlen, 0, 0);
            if (GPU_INPUT_TO_NOCACHE) {
                if (UI_ADDR_IN_PSRAM_CACHE(info->text_image_buf)) {
                    psram_flush_cache(info->text_image_buf, info->text_image_buf_size);
                }
            }
            if (text->font_out_buf) {
                //拷贝到copey_mulstr释放
                /* text->copy_mulstr = text->font_out_buf; */
                //立即释放
                /* jlui_free((void *)text->font_out_buf, UI_RAM_SRAM, UI_MODULE_FRAME); */
                if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
                    async_buffer_add((void *)text->font_out_buf);
                } else {
                    //post到gpu task释放
                    jlgpu_scheduler_async_free_buf((void *)text->font_out_buf);
                }
                text->font_out_buf = NULL;
            }
            text->font_out_buf = (const char *)info->text_image_buf;
            if ((info->flags & FONT_SHOW_MULTI_LINE) && (dc->hori_align == HORI_ALIGN_CENTER)) {
                font_release_each_line_width_info(info);
            }
        }
        /* >>>>>>>>>>> FONT FREETYPE >>>>>>>>>>> */
        if (FONT_FREETYPE_SWITCH) {
            // 用作以下清除多余gpu任务(自定义绘图)
            /* 清空多余的GPU任务，避免上一次显示内容比这次多时，部分内容没有清空 */
            dc->elm_index += info->task_id--;
            /* mulstr strpic_mode == 2 */
            if ((text->format == UI_TEXT_ENCODE_STRPIC || text->format == UI_TEXT_ENCODE_MULSTR) && strpic_mode == UI_TEXT_STRPIC_MODE_ENCODE) {
                if (text_buf) {
                    jlui_free((void *)text_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                }

            }
            /* mulstr strpic_mode == 2 */
            return 0; //直接返回，freetype字库流程已经处理了gpu任务
        }
        /* <<<<<<<<<<< FONT FREETYPE <<<<<<<<<<< */
        if (info->flags & FONT_VERTICAL_SCROLL) {
            dc->vert_align = 0;
        }
        /* 计算GPU任务配置 */
        img_list->image_num = 1;
        img_list->rect.width = text->width;
        img_list->rect.height = text->height;
        if (dc->draw_img.en) {
            img_list->rect.left = dc->draw_img.x;
            img_list->rect.top = dc->draw_img.y;
        } else {
            get_rect_align(&dc->rect, &img_list->rect, dc->hori_align, dc->vert_align);
        }
        /* DUMP_RECT(__func__, __LINE__, "img_list", &img_list->rect); */
        if ((info->flags & FONT_SHOW_SCROLL) && (text->width > dc->rect.width)) {
            /* 滚动使能时，重置left */
            if (font_lang_get() == Arabic || font_lang_get() == Hebrew || font_lang_get() == UnicodeMixLeftword) { //显示方向从右到左
                img_list->rect.left = dc->rect.left - (text->width - dc->rect.width) + text->offset;
            } else {
                img_list->rect.left = dc->rect.left - text->offset;
            }
            /* DUMP_RECT(__func__, __LINE__, "img_list", &img_list->rect); */
        }

        struct rect draw;
        struct rect *valid_r;
        if (!dc->elm->rotate_en && !dc->elm->ratio_en) {
            valid_r = &dc->draw;
        } else {
            valid_r = &dc->rect;
        }
        get_rect_cover(valid_r, &img_list->rect, &draw);
        /* DUMP_RECT(__func__, __LINE__, "draw", &draw); */
        struct image_unit *img = jlui_malloc(sizeof(struct image_unit), UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
        memset(img, 0, sizeof(struct image_unit));
        img_list->img = img;
        img->color = text->color;
        memcpy(&img->rect, &img_list->rect, sizeof(struct rect));
        if (!get_rect_cover(&draw, &img->rect, &img->draw)) {
            img->invisible = true;
        }
        /* DUMP_RECT(__func__, __LINE__, "img draw", &img->draw); */

        if (get_rect_crop(&img->draw, &img->rect, &img->crop)) {
            img->crop_en = true;
        }
        /* DUMP_RECT(__func__, __LINE__, "img crop", &img->crop); */
        if ((info->flags & FONT_SHOW_SCROLL) && dc->elm->ratio_en) {
            img->draw.width = dc->rect.width;
        }

        if ((info->tool_version == 0) || (info->tool_version == 0x0100)) {
            img->image.format = PIXEL_FMT_L1;
        } else {
            if (Font_GetBitDepth() == BIT_DEPTH_1BPP) {
                img->image.format = PIXEL_FMT_L1;
            } else if (Font_GetBitDepth() == BIT_DEPTH_2BPP) {
                img->image.format = PIXEL_FMT_A2;
            } else if (Font_GetBitDepth() == BIT_DEPTH_4BPP) {
                img->image.format = PIXEL_FMT_A4;
            } else {
                img->image.format = PIXEL_FMT_A8;
            }
        }

        img->image.width  = img->rect.width;
        img->image.height = img->rect.height;
        img->flash.offset = (u32)text->font_out_buf;
        img->used_ram = true;	// 字库使用RAM缓存，后续不需要再缓存了
        if (text_elm->scroll) {
            img->draw.width = valid_r->width; //缩放滚动时对齐控件中心
        }
        if (info->flags & FONT_SCROLL_CIRCULAR) {
            int right_distance = rect_right(valid_r) - (rect_right(&img->rect) + interval);
            if (right_distance > 0) { // need_scroll_circular
                struct image_unit *img_copy = jlui_malloc(sizeof(struct image_unit), UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                /* memset(img_copy, 0, sizeof(struct image_unit)); */
                memcpy(img_copy, img, sizeof(struct image_unit));
                img->next = img_copy;
                img_copy->invisible = false;
                img_copy->rect.left = 0;
                img_copy->crop.left = 0;
                img_copy->crop.width = right_distance ;
                img_copy->crop.height = img->rect.height;
                /* img_copy->draw.left = rect_right(valid_r) - right_distance; */
                /* img_copy->draw.width = right_distance; */
                img_copy->offset_x = valid_r->width - right_distance;
                img_copy->crop_en = true;
            }
        }
        /* mulstr strpic_mode == 2 */
        if ((text->format == UI_TEXT_ENCODE_STRPIC || text->format == UI_TEXT_ENCODE_MULSTR) && strpic_mode == UI_TEXT_STRPIC_MODE_ENCODE) {
            if (text_buf) {
                jlui_free((void *)text_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
            }

        }
        /* mulstr strpic_mode == 2 */
        return 1;
    }
    return 0;
}

static int jlui_show_ascii_text(struct draw_context *dc, struct ui_text_attrs *text, struct image_list *img_list)
{
    const char *str;

    if ((!text->str) || ((u8)text->str[0] == 0xff)) {
        return 0;
    }

    int fbuf_len = 0;
    u8 *fbuf = NULL;
    int width;
    int height;
    u32 realsize_w = 0;
    u32 realsize_h = 0;
    u8 *pixbuf_temp = NULL;

    int x = dc->rect.left;
    int y = dc->rect.top;

    fbuf_len = 256;
    fbuf = jlui_malloc(fbuf_len, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
    if (!fbuf) {
        return 0;
    }

    str = text->str;
    while (*str) {
        font_ascii_get_pix(*str, fbuf, fbuf_len, &height, &width);
        realsize_w += width;
        str++;
    }
    realsize_h = height;

    if (text->font_out_buf) {
        log_warn("%s(), delete mulstr buf\n", __func__);
        jlui_free((void *)text->font_out_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
    }
    u16 stride = (realsize_w + 31) / 32 * 32 / 8;
    u8 *pixbuf = jlui_malloc(stride * realsize_h, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
    if (!pixbuf) {
        return 0;
    }
    memset(pixbuf, 0, stride * realsize_h);

    pixbuf_temp = jlui_malloc((realsize_w + 7) / 8 * realsize_h, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
    if (!pixbuf_temp) {
        return 0;
    }

    str = text->str;
    int offset = 0;
    while (*str) {
        memset(fbuf, 0, fbuf_len);

        font_ascii_get_pix(*str, fbuf, fbuf_len, &height, &width);

        for (int i = 0; i < height / 8; i ++) {
            memcpy(pixbuf_temp + realsize_w * i + offset, fbuf + width * i, width);
        }

        offset += width;
        str++;
    }

    l1_data_transformation(pixbuf_temp, pixbuf, stride, x, y, realsize_h, realsize_w);

    if (pixbuf_temp) {
        jlui_free(pixbuf_temp, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
    }

    if (fbuf) {
        jlui_free(fbuf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
    }

    img_list->image_num = 1;
    img_list->rect.width = realsize_w;
    img_list->rect.height = realsize_h;
    if (dc->draw_img.en) {
        img_list->rect.left = dc->draw_img.x;
        img_list->rect.top = dc->draw_img.y;
    } else {
        get_rect_align(&dc->rect, &img_list->rect, dc->hori_align, dc->vert_align);
    }
    /* DUMP_RECT(__func__, __LINE__, "img_list", &img_list->rect); */

    struct rect draw;
    if (!dc->elm->rotate_en && !dc->elm->ratio_en) {
        get_rect_cover(&dc->draw, &img_list->rect, &draw);
    } else {
        get_rect_cover(&dc->rect, &img_list->rect, &draw);
    }
    /* DUMP_RECT(__func__, __LINE__, "draw", &draw); */

    struct image_unit *img = jlui_malloc(sizeof(struct image_unit), UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
    memset(img, 0, sizeof(struct image_unit));
    img_list->img = img;
    img->color = text->color;
    memcpy(&img->rect, &img_list->rect, sizeof(struct rect));

    if (!get_rect_cover(&draw, &img->rect, &img->draw)) {
        img->invisible = true;
    }
    /* DUMP_RECT(__func__, __LINE__, "img draw", &img->draw); */

    if (get_rect_crop(&img->draw, &img->rect, &img->crop)) {
        img->crop_en = true;
    }
    /* DUMP_RECT(__func__, __LINE__, "img crop", &img->crop); */

    img->image.format = PIXEL_FMT_L1;
    img->image.width  = img->rect.width;
    img->image.height = img->rect.height;
    img->flash.offset = (u32)pixbuf;
    img->used_ram = true;
    text->font_out_buf = (const char *)pixbuf;	// 把buf设置到TEXT控件，当TEXT控件释放时释放buf

    return 1;
}

static int jlui_show_strpic_text(struct draw_context *dc, struct ui_text_attrs *text, struct image_list *img_list, int strpic_mode)
{
    int err;
    u16 id = *((u16 *)text->str);
    struct image_file file = {0};
    struct flash_file_info flash = {0};
    struct ui_text *text_elm = (struct ui_text *)dc->elm;
    int interval = font_scroll_circular_interval;
    /* 获取文本总的宽高信息 */
    err = jlui_res_get_strpic_info(dc->prj, 0, id, &file, &flash);
    if ((err) || (!file.width) || (!file.height) || (!file.len)) {
        log_error("%s(), get strpic info faild!, err: %d, w: %d, h: %d, len: %d\n",
                  __func__, err, file.width, file.height, file.len);
        return -EINVAL;
    }
    log_info("%s(), file w: %d, h: %d, len: %d\n", __func__, file.width, file.height, file.len);

    /* 图片整体的绘制区域并相对于控件对齐 */
    img_list->rect.width = file.width;
    img_list->rect.height = file.height;
    get_rect_align(&dc->rect, &img_list->rect, dc->hori_align, dc->vert_align);

    /* 如果文本长度超过控件，强制开启滚动。若无需自动滚动功能，注释这段if代码即可 */
    if (file.width > dc->rect.width) {
        text->displen = file.width;//文本完全移出控件再回滚
        /* text->displen = file.width - dc->rect.width;//最后一个字符出现即回滚 */
        text->flags |= FONT_SHOW_SCROLL;
        if (strpic_scroll_circular_deafult_enable) {
            text->flags |= FONT_SCROLL_CIRCULAR;
        }

        if (text->flags & FONT_SCROLL_CIRCULAR) {
            text->displen = file.width + interval;
        }
        text_elm->scroll = true;
        img_list->rect.left = dc->rect.left - text->offset;
    } else {
        text->flags &= ~FONT_SHOW_SCROLL;
        text->flags &= ~FONT_SCROLL_CIRCULAR;
        text_elm->scroll = false;
    }

    /* 获取图片实际的绘制区域，如果没有旋转缩放，则是图片相对于控件的交集 */
    struct rect draw;
    struct rect *valid_r;
    if (!dc->elm->rotate_en && !dc->elm->ratio_en) {
        valid_r = &dc->draw;
    } else {
        valid_r = &dc->rect;
    }
    get_rect_cover(valid_r, &img_list->rect, &draw);
    /* DUMP_RECT(__func__, __LINE__, "draw", &draw); */

    /* 根据模式获取具体图片信息 */
    if (strpic_mode == UI_TEXT_STRPIC_MODE_IMAGE) {		// image 图片
        img_list->image_num = 1;

        /* 申请图片信息缓存buf */
        struct image_unit *img = jlui_malloc(sizeof(struct image_unit), UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
        memset(img, 0, sizeof(struct image_unit));
        img_list->img = img;
        img->color = text->color;
        memcpy(&img->rect, &img_list->rect, sizeof(struct rect));	// image格式时，整张图片就是图片的绘制区域

        /* 计算图片的实际绘制区域 */
        if (!get_rect_cover(&draw, &img->rect, &img->draw)) {
            img->invisible = true;
        }

        /* 计算图片的裁剪区域 */
        if (get_rect_crop(&img->draw, &img->rect, &img->crop)) {
            img->crop_en = true;
        }
        if (text_elm->scroll) {
            /* memcpy(&img->draw,valid_r,sizeof(struct rect)); */
            img->draw.width = valid_r->width; //缩放滚动时对齐控件中心
        }
        /* 保存图片的image, flash结构体信息 */
        memcpy(&img->image, &file, sizeof(struct image_file));
        memcpy(&img->flash, &flash, sizeof(struct flash_file_info));
        if (text->flags & FONT_SCROLL_CIRCULAR) {
            int right_distance = rect_right(valid_r) - (rect_right(&img->rect) + interval);
            if (right_distance > 0) { // need_scroll_circular
                struct image_unit *img_copy = jlui_malloc(sizeof(struct image_unit), UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                /* memset(img_copy, 0, sizeof(struct image_unit)); */
                memcpy(img_copy, img, sizeof(struct image_unit));
                img->next = img_copy;
                img_copy->invisible = false;
                img_copy->rect.left = 0;
                img_copy->crop.left = 0;
                img_copy->crop.width = right_distance;
                img_copy->crop.height = img->rect.height;
                /* img_copy->draw.left = rect_right(valid_r) - right_distance; */
                /* img_copy->draw.width = right_distance; */
                img_copy->offset_x = valid_r->width - right_distance;
                img_copy->crop_en = true;
            }
        }

        return 1;	// 返回1，配置GPU任务时不做偏移

    } else if (strpic_mode == UI_TEXT_STRPIC_MODE_INDEX) {		// index 索引
        u8 *str_index_buf = (u8 *)jlui_malloc(file.len, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
        if (!str_index_buf) {
            log_error("%s(), malloc str index buf faild!\n", __func__);
            return -ENOSPC;
        }
        memset(str_index_buf, 0, file.len);
        read_str_data(&file, str_index_buf, file.len);
        u16 *pstr = (u16 *)str_index_buf;
        if (!pstr) {
            return -ENOSPC;
        }
        img_list->image_num = pstr[0];

        int strpic_line_h;
        int x = 0, y = 0;
        struct image_unit *curr = img_list->img;
        int circular_flag = 0;
        if (text->flags & FONT_SCROLL_CIRCULAR) {
            circular_flag = 1;
        }
__circular:
        u8 n, idx = 0;
        u16 line_info[32] = {0}; //多行时用来存每行的对齐信息
        if (dc->hori_align) {
            for (n = 0; n < img_list->image_num; n++) {
                id = pstr[1 + n];
                err = jlui_res_get_strpic_info(dc->prj, strpic_mode, id, &file, &flash);
                if (err) {
                    log_error("%s(), err pstr!\n", __func__);
                    if (str_index_buf) {
                        jlui_free(str_index_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                    }
                    return -1;
                }
                if (!file.width || !file.height) {	/* 换行符 */
                    idx++;
                    ASSERT(idx < 32, "strpic line_num over 32, need expand the length of the line_info\n");
                    continue;
                } else {
                    line_info[idx] += file.width;
                }
            }
            for (n = 0; n <= idx; n++) {
                line_info[n] = dc->rect.left + (dc->rect.width - line_info[n]) / 2;
            }
            idx = 0;
        }
        for (int i = 0; i < img_list->image_num; i++) {
            id = pstr[1 + i];
            err = jlui_res_get_strpic_info(dc->prj, strpic_mode, id, &file, &flash);
            if (err) {
                log_error("%s(), err pstr!\n", __func__);
                if (str_index_buf) {
                    jlui_free(str_index_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                }
                return -1;
            }
            if (!file.width || !file.height) {	/* 换行符 */
                x = 0;	// X坐标回到起始点，Y坐标换行
                y += strpic_line_h;
                idx++;
                continue;
            } else {
                strpic_line_h = file.height;	// 行高就是文字图片的高度，避免文字头尾叠加到一起
            }
            /* 申请图片信息缓存buf，配置完GPU任务后释放 */
            struct image_unit *img = jlui_malloc(sizeof(struct image_unit), UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
            memset(img, 0, sizeof(struct image_unit));
            if (!img_list->img) {
                img_list->img = img;
            } else {
                curr->next = img;
            }
            curr = img;
            img->color = text->color;
            if (dc->hori_align) {
                img_list->rect.left = line_info[idx];
            }

            /* 计算预估的当前图片的显示位置 */
            img->rect.left = x + img_list->rect.left;	// 偏移到图片对齐的绘制区域
            img->rect.top = y + img_list->rect.top;
            img->rect.width = file.width;
            img->rect.height = file.height;
            /* DUMP_RECT(__func__, __LINE__, "img rect", &img->rect); */
            /* DUMP_RECT(__func__, __LINE__, "valid_r", valid_r); */
            if (!get_rect_cover(valid_r, &img->rect, &img->draw)) {
                img->invisible = true;
            }
            /* DUMP_RECT(__func__, __LINE__, "img draw", &img->draw); */

            /* 计算当前图片的裁剪区域 */
            if (get_rect_crop(&img->draw, &img->rect, &img->crop)) {
                img->crop_en = true;
            }

            /* 记录image,flash信息 */
            memcpy(&img->image, &file, sizeof(struct image_file));
            memcpy(&img->flash, &flash, sizeof(struct flash_file_info));

            x += file.width;	// x 坐标偏移
        }
        if (circular_flag == 1) {
            x += interval;
            circular_flag = 0;
            goto __circular;
        }

        if (str_index_buf) {	// 释放缓存buf
            jlui_free(str_index_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
        }

    } else {
        log_error("%s(), unknow strpic mode: %d\n", __func__, strpic_mode);
        return -EINVAL;
    }
    return 0;
}


static int jlui_show_mulstr_text(struct draw_context *dc, struct ui_text_attrs *text, struct image_list *img_list, int strpic_mode)
{
    int err;
    u16 id;

    int w = 0, h = 0, num = 0;
    u16 *str_p = ((u16 *)text->str);
    struct image_file file = {0};
    struct flash_file_info flash = {0};

    if (strpic_mode == UI_TEXT_STRPIC_MODE_IMAGE) {
        err = get_multi_string_width(dc, (u8 *)text->str, &w, &h);
        if (err) {
            return -EINVAL;
        }
        log_info("%s(), w: %d, h: %d\n", __func__, w, h);
    } else if (strpic_mode == UI_TEXT_STRPIC_MODE_INDEX) {
        err = get_multi_strs_width(dc, (u8 *)text->str, &w, &h, &num);
        if (err) {
            return -EINVAL;
        }
        log_info("%s(), w: %d, h: %d, num: %d\n", __func__, w, h, num);
    } else {
        log_error("%s(), unknow strpic mode: %d\n", __func__, strpic_mode);
        return -EINVAL;
    }

    img_list->rect.width = w;
    img_list->rect.height = h;
    get_rect_align(&dc->rect, &img_list->rect, dc->hori_align, dc->vert_align);
    /* DUMP_RECT(__func__, __LINE__, "img_list->rect", &img_list->rect); */

    /* 获取图片实际的绘制区域，如果没有旋转缩放，则是图片相对于控件的交集 */
    struct rect draw;
    if (!dc->elm->rotate_en && !dc->elm->ratio_en) {
        get_rect_cover(&dc->draw, &img_list->rect, &draw);
    } else {
        get_rect_cover(&dc->rect, &img_list->rect, &draw);
    }
    /* DUMP_RECT(__func__, __LINE__, "draw", &draw); */

    int strpic_num, all_strpic_num = 0;
    struct image_unit *curr = img_list->img;
    u8 *str_index_buf;
    int x = 0, y = 0;
    int strpic_line_h;
    u16 *pstr = NULL;

    while (*str_p != 0) {
        if (strpic_mode == UI_TEXT_STRPIC_MODE_INDEX) {
            if (num-- <= 0) {
                break;
            }
        }

        id = *str_p;
        if ((id == 0xffff) || (id == 0)) {
            return -EINVAL;
        }
        err = jlui_res_get_strpic_info(dc->prj, 0, id, &file, &flash);
        if ((err) || (!file.len)) {
            return -EINVAL;
        }

        if (strpic_mode == UI_TEXT_STRPIC_MODE_IMAGE) {
            strpic_num = 1;
        } else if (strpic_mode == UI_TEXT_STRPIC_MODE_INDEX) {
            /* 可能每个字符的长度都不一样，所以每次都重新申请buf */
            str_index_buf = (u8 *)jlui_malloc(file.len, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
            if (!str_index_buf) {
                return -EINVAL;
            }
            memset(str_index_buf, 0, file.len);
            read_str_data(&file, str_index_buf, file.len);
            pstr = (u16 *)str_index_buf;
            strpic_num = pstr[0];
        }

        for (int i = 0; i < strpic_num; i++) {
            if (strpic_mode == UI_TEXT_STRPIC_MODE_INDEX) {
                id = pstr[1 + i];
                if (jlui_res_get_strpic_info(dc->prj, strpic_mode, id, &file, &flash)) {
                    log_error("%s(), err open mulstrw!\n", __func__);
                    if (str_index_buf) {
                        jlui_free(str_index_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                    }
                    return -EINVAL;
                }
                if ((id == 1) || (!file.width) || (!file.height)) {
                    /* 换行，x回到初始位置，y换行，如果需要滚动，这里重置X位置 */
                    x = 0;
                    y += strpic_line_h;
                } else {
                    strpic_line_h = file.height;
                }
            }

            /* 申请图片信息缓存buf，配置完GPU任务后释放 */
            struct image_unit *img = jlui_malloc(sizeof(struct image_unit), UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
            memset(img, 0, sizeof(struct image_unit));
            if (!img_list->img) {
                img_list->img = img;
            } else {
                curr->next = img;
            }
            curr = img;
            img->color = text->color;

            /* 理论上当前文字显示的位置 */
            img->rect.left = x + img_list->rect.left;
            img->rect.top = y + img_list->rect.top;
            img->rect.width = file.width;
            img->rect.height = file.height;
            /* DUMP_RECT(__func__, __LINE__, "img rect", &img->rect); */

            /* 计算当前图片的绘制区域 */
            if (!get_rect_cover(&draw, &img->rect, &img->draw)) {
                img->invisible = true;
            }
            /* DUMP_RECT(__func__, __LINE__, "img draw", &img->draw); */

            /* 计算当前图片的裁剪区域 */
            if (get_rect_crop(&img->draw, &img->rect, &img->crop)) {
                img->crop_en = true;
            }
            /* DUMP_RECT(__func__, __LINE__, "img crop", &img->crop); */

            /* 记录image,flash信息 */
            memcpy(&img->image, &file, sizeof(struct image_file));
            memcpy(&img->flash, &flash, sizeof(struct flash_file_info));

            x += file.width;
        }

        if (str_index_buf) {
            jlui_free(str_index_buf, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
            str_index_buf = NULL;
        }

        all_strpic_num += strpic_num;
        str_p ++;
    }
    img_list->image_num = all_strpic_num;	// 记录总的图片个数

    /* 如果只有一张图片，返回1。旋转、缩放时不再调整位置 */
    if (img_list->image_num == 1) {
        return 1;
    } else {
        return 0;
    }
}

static int jlui_show_image_text(struct draw_context *dc, struct ui_text_attrs *text, struct image_list *img_list)
{
    int err;
    int w = 0, h = 0;
    int image_num = 0;
    int page_num = dc->page;
    const char *txt = text->str;
    struct element *elm = dc->elm;

    log_info("%s(), page_num: %d, id: 0x%x\n", __func__, page_num, elm->id);

    u16 cnt = 0;
    u16 id = ((u8)txt[1] << 8) | (u8)txt[0];
    struct image_unit *curr = img_list->img;

    /* 获取需要显示的所有图片信息 */
    while ((id != 0x00ff) && (id != 0xffff)) {

        /* 申请图片信息缓存buf，创建完GPU任务再释放 */
        struct image_unit *img = jlui_malloc(sizeof(struct image_unit), UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
        memset(img, 0, sizeof(struct image_unit));

        /* 使用链表串联所有图片信息 */
        if (!img_list->img) {
            img_list->img = img;
        } else {
            curr->next = img;
        }
        curr = img;

        img->image_id = id;		// 图片ID

        /* 从res文件读取图片信息到 image 成员 */
        err = open_image_by_id(dc->prj, NULL, &img->image, id, page_num);
        if (err) {
            /* 如果读取失败，释放掉所有的图片信息缓存，并返回异常 */
            struct image_unit *temp, *unit = img_list->img;
            while (unit) {
                temp = unit->next;
                jlui_free(unit, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                unit = temp;
            }
            img_list->img = NULL;
            return -EFAULT;
        }
        log_info("%s(), image id: 0x%x, w: %d, h: %d\n", __func__, id, img->image.width, img->image.height);

        if ((img->image.format == PIXEL_FMT_AL88)
            || (img->image.format == PIXEL_FMT_AL44)
            || (img->image.format == PIXEL_FMT_AL22)
            || (img->image.format == PIXEL_FMT_L8)
            || (img->image.format == PIXEL_FMT_L4)
            || (img->image.format == PIXEL_FMT_L2)
            || (img->image.format == PIXEL_FMT_L1)
            || (img->image.format == PIXEL_FMT_A8)
            || (img->image.format == PIXEL_FMT_A4)
            || (img->image.format == PIXEL_FMT_A2)
            || (img->image.format == PIXEL_FMT_A1)
           ) {
            int tab_size = get_clut_format_tabsize(img->image.format, img->image.clut_format);
            img->image.offset += tab_size;
            img->image.len -= tab_size;
        }
        log_info("%s(), format: %d, clut_format: %d, offset: %d, len: %d\n", __func__,
                 img->image.format, img->image.clut_format, img->image.offset, img->image.len);

        /* 读取图片数据在flash中的位置及相关信息 */
        err = ui_res_get_image_flash_info(&img->flash, &ui_load_info_table[dc->prj].image_file_info, &img->image);
        if (err) {
            struct image_unit *temp, *unit = img_list->img;
            while (unit) {
                temp = unit->next;
                jlui_free(unit, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));
                unit = temp;
            }
            return -EFAULT;
        }

        /* 计算图片预设显示位置，后面计算绘制区域时再偏移到控件位置，所以这里从0开始 */
        img->rect.left = w;
        img->rect.top = 0;
        img->rect.width = img->image.width;
        img->rect.height = img->image.height;
        /* DUMP_RECT(__func__, __LINE__, "rect", &img->rect); */

        /* 记录已绘图的总长度，获取下一个图片的ID */
        w += (img->image.width + text->x_interval);
        cnt += 2;
        id = ((u8)txt[cnt + 1] << 8) | (u8)txt[cnt];
        if (img->image.height > h) {
            h = img->image.height;
        }
        image_num ++;
    }

    /* 获取控件区域，保持图片数量、绘图总宽度、高度等 */
    img_list->image_num = image_num;
    img_list->image_width = w;
    img_list->image_height = h;

    /* 根据对齐方式，计算理论上所有图片的总绘制区域 */
    img_list->rect.width = w;
    img_list->rect.height = h;
    get_rect_align(&dc->rect, &img_list->rect, dc->hori_align, dc->vert_align);	// 计算对齐后的区域
    /* DUMP_RECT(__func__, __LINE__, "img_list->rect", &img_list->rect); */

    /* 如果有滚动，这里重置一下 img_list->rect.left，设置到滚动的起始坐标 */
    // TODO

    /* 根据理论图片绘制区域，计算实际的绘图区域; */
    struct rect draw;
    if (!dc->elm->rotate_en && !dc->elm->ratio_en) {
        /* 没有旋转缩放时由UI框架的draw计算绘制区域; */
        get_rect_cover(&dc->draw, &img_list->rect, &draw);
    } else {
        /* 有旋转缩放时，由控件决定绘制区域 */
        get_rect_cover(&dc->rect, &img_list->rect, &draw);
    }
    /* DUMP_RECT(__func__, __LINE__, "draw", &draw); */

    /* 计算所有图片的显示位置和裁剪区域 */
    curr = img_list->img;
    while (curr) {
        /* 理论上完整的图片绘制区域，由于原先从0坐标开始排序，这里加上图片的实际绘制起始地址进行偏移 */
        curr->rect.left += img_list->rect.left;
        curr->rect.top  += img_list->rect.top;

        /* 通过实际的绘制区域和单张图片的理论绘制区域计算该图片的实际绘制区域 */
        if (!get_rect_cover(&draw, &curr->rect, &curr->draw)) {
            curr->invisible = true;
        }

        /* 通过图片的绘制区域和图片完整区域，计算图片的裁剪区域 */
        if (get_rect_crop(&curr->draw, &curr->rect, &curr->crop)) {
            curr->crop_en = true;
        }
        /* DUMP_RECT(__func__, __LINE__, "rect", &curr->rect); */
        /* DUMP_RECT(__func__, __LINE__, "draw", &curr->draw); */
        /* DUMP_RECT(__func__, __LINE__, "crop", &curr->crop); */

        curr = curr->next;	// 计算下一个图片位置
    }

    return 0;
}

static int jlui_show_text(struct draw_context *dc, struct ui_text_attrs *text)
{
    log_info("%s(). enter. dc: 0x%p, elm_id: 0x%x\n", __func__, dc, dc->elm->id);
    if ((!text) || (text->format > UI_TEXT_ENCODE_IMAGE)  || (!text->str)) {
        return -EFAULT;	// 如果不满足条件，直接退出
    }

    int i = 0;
    int ret = 0;
    struct image_list img_list = {0};
    struct image_unit *img;
    u8 strpic_flag = 0;
    u8 *strpic_idbuf = NULL;
    u8 *strpic_tbuf = NULL;

    /* DUMP_RECT(__func__, __LINE__, "dc rect", &dc->rect); */
    /* DUMP_RECT(__func__, __LINE__, "dc draw", &dc->draw); */

    /* 计算每种编码格式时各纹理的显示位置和裁剪信息、flash信息等，
       这里只计算各参数，用链表形式保存到img_list，后面统一配置GPU任务 */
    log_info("%s(), text format: %d\n", __func__, text->format);

    if (text->format == UI_TEXT_ENCODE_TEXT) {
__strpic_with_text:	// 如果多国语言用编码模式，则在这里用字库显示

        ret = jlui_show_text_text(dc, text, &img_list);

    } else if (text->format == UI_TEXT_ENCODE_ASCII) {
        ret = jlui_show_ascii_text(dc, text, &img_list);
    } else if (text->format == UI_TEXT_ENCODE_STRPIC) {
        /* 获取多国语言模式：0图片，1索引，2编码 */
        u16 id = *((u16 *)text->str);
        if (id == 0) {
            return -1;
        }
        int strpic_mode = read_string_type(dc->prj, id);
        if (strpic_mode == UI_TEXT_STRPIC_MODE_ENCODE) {
            /* 使用编码方式，跳转到字库显示逻辑 */
            goto __strpic_with_text;
        }
        ret = jlui_show_strpic_text(dc, text, &img_list, strpic_mode);
    } else if (text->format == UI_TEXT_ENCODE_MULSTR) {
        u16 id = *((u16 *)text->str);
        if (id == 0) {
            return -1;
        }
        int strpic_mode = read_string_type(dc->prj, id);
        if (strpic_mode == UI_TEXT_STRPIC_MODE_ENCODE) {
            /* 使用编码方式，跳转到字库显示逻辑 */
            goto __strpic_with_text;
        }
        ret = jlui_show_mulstr_text(dc, text, &img_list, strpic_mode);
    } else if (text->format == UI_TEXT_ENCODE_IMAGE) {
        ret = jlui_show_image_text(dc, text, &img_list);
    } else {
        log_error("%s(), unknow text format: %d\n", __func__, text->format);
        return -1;
    }
    if (ret && (ret != 1)) {
        log_error("%s(), config text info faild! ret: %d\n", __func__, ret);
        return ret;
    }

    /* 这里只通过img_list配置GPU任务，多余的GPU任务会删掉，每个纹理应该在前面已经算好 */
    img = img_list.img;
    u8 elm_index = dc->elm_index ++;//dc->draw_img.en ? dc->elm_index ++ : 3;
    while (img) {
        int elm_id = dc->elm->id;
        int task_id = GPU_TASK_ID(dc->page, dc->elm->id, elm_index);
        jlgpu_task_basic_param_init(GPU_TASK_TEXT, task_id, elm_id);


        JLGPU_TASK_SCALE_CONFIG(&task_param.texture.tran);
        JLGPU_TASK_ROTATE_CONFIG(&task_param.texture.tran);

        /* 拷贝图片的image, flash, draw, crop信息 */
        memcpy(&task_param.image, &img->image, sizeof(struct image_file));
        memcpy(&task_param.info, &img->flash, sizeof(struct flash_file_info));
        memcpy(&task_param.draw, &img->draw, sizeof(struct rect));
        memcpy(&task_param.texture.crop, &img->crop, sizeof(struct rect));
        task_param.offset_x = img->offset_x;
        if (__this->cache_mmu && (dc->prj == 1)) {
            struct mmu_tab_cache *c = mmu_tab_cache_add(task_param.info.tab, task_param.info.tab_size, task_param.image.data_crc);
            task_param.info.tab = (void *) & (c->tab);
            task_param.info.tab_size = c->tab_size;
        }
        task_param.crop_en = img->crop_en;
        task_param.invisible = img->invisible;
        /* DUMP_RECT(__func__, __LINE__, "task draw", &task_param.draw); */
        /* DUMP_RECT(__func__, __LINE__, "task crop", &task_param.texture.crop); */

        /* task_param.task_type = GPU_TASK_TEXT;//GPU_TASK_IMAGE; */
        task_param.priority = dc->elm->prior;
        task_param.gpu_free_data = dc->gpu_free_data;
        /* task_param.task_id = GPU_TASK_ID(dc->page, dc->elm->id, elm_index); */
        /* task_param.element_id = dc->elm->id; */
        task_param.clut_format = task_param.image.clut_format;
        task_param.has_clut = task_param.image.has_clut;
        task_param.format = res_format_to_gpu(task_param.image.format);
        task_param.texture.data = (u8 *)task_param.info.offset;
        task_param.texture.mmu_tab_base = (u8 *)task_param.info.tab;
        /* task_param.acolor = (((dc->alpha * 255 / 100) << 24) | img->color); */
        task_param.alpha = dc->alpha;
        task_param.red   = RGB565_R(img->color);
        task_param.green = RGB565_G(img->color);
        task_param.blue  = RGB565_B(img->color);
        int global_alpha =  dc->elm->css.alpha * 128 / 100;
        task_param.global_alpha = global_alpha;
        if (dc->custom_color & BIT(UI_CUSTOM_COLOR_BIT_TEXT)) {
            if ((task_param.image.format == PIXEL_FMT_L1)
                || (task_param.image.format == PIXEL_FMT_A8)
                || (task_param.image.format == PIXEL_FMT_A4)
                || (task_param.image.format == PIXEL_FMT_A2)
                || (task_param.image.format == PIXEL_FMT_A1)) {
                task_param.custom_color = true;
                /* task_param.acolor = (((dc->alpha * 255 / 100) << 24) | dc->custom_rgb565); */
                /* task_param.alpha = dc->alpha * 255 / 100; */
                /* task_param.red   = RGB565_R(dc->custom_rgb565); */
                /* task_param.green = RGB565_B(dc->custom_rgb565); */
                /* task_param.blue  = RGB565_B(dc->custom_rgb565); */
                task_param.alpha = (dc->custom_argb8888 >> 24) & 0xff;
                task_param.red   = (dc->custom_argb8888 >> 16) & 0xff;
                task_param.green = (dc->custom_argb8888 >> 8) & 0xff;
                task_param.blue  = (dc->custom_argb8888) & 0xff;
                /* log_info("custom_color: 0x%x, a:0x%x, r:0x%x, g:0x%x, b:0x%x\n", dc->custom_argb8888, task_param.alpha, task_param.red, task_param.green, task_param.blue); */

            } else {
                task_param.custom_color = false;
                log_warn("%s(), image format err: %d, Only L1, A8, A4, A2, A1 can modified colors!\n", __func__, task_param.image.format);
            }
        } else {
            task_param.custom_color = false;
        }

        if (task_param.format == GPU_FORMAT_L1) {
            task_param.clut_tab = (u8 *)jlui_zalloc(gpu_format_to_lut_size(GPU_FORMAT_L1, GPU_CLUT_FORMAT_ARGB8888));
            if (!task_param.clut_tab) {
                log_error("malloc clut_tab buf faild!\n");
                ASSERT(task_param.clut_tab != NULL, "Insufficient memory!");
            }
            task_param.clut_tab[4] = task_param.blue;//task_param.acolor;
            task_param.clut_tab[5] = task_param.green;//task_param.acolor >> 8;
            task_param.clut_tab[6] = task_param.red;//task_param.acolor >> 16;
            task_param.clut_tab[7] = 0xff;

            task_param.clut_format = GPU_CLUT_FORMAT_ARGB8888;
            task_param.has_clut = CLUT_TAB_FROM_RAM;
            task_param.texture.mmu_tab_base = NULL;
            /* task_param.acolor |= (0x00 << 24);	// L1格式没有透明度 */
            task_param.alpha = 0x00;	// L1格式没有透明度
            task_param.image.len = 0;
            jlgpu_task_texture_big_endian_enable(true);
        }

        /* 缩放时，一排图片位置需要根据缩放中心进行调整，保持整体相对于缩放中心缩放 */
        if (task_param.scale_en && (ret != 1)) {
            //缩放中心
            float scale_cx = (dc->rect.width / 2) + dc->rect.left;
            //图片中心
            float img_center = task_param.draw.left + task_param.draw.width / 2;
            //计算缩放后的偏移距离
            float scale_offset = (scale_cx - img_center) * (1.0f - task_param.texture.tran.ratio_w);
            //赋值缩放前偏移
            task_param.offset_x = scale_offset / task_param.texture.tran.ratio_w;
        }
        /* 旋转时，一排图片需要调整旋转中心，保持相对位置作为一个整体 */
        if (task_param.rotate_en && (ret != 1)) {
            task_param.texture.tran.rotate_cx = task_param.texture.tran.rotate_cx + dc->rect.left - task_param.draw.left;
            task_param.texture.tran.rotate_cy = task_param.texture.tran.rotate_cy + dc->rect.top - task_param.draw.top;
        }
        jlgpu_task_param_3x3_gaussian_low_level(&task_param, &task_param.texture.tran);
        // jlgpu_task_param_3x3_gaussian_middle_level(&task_param, &task_param.texture.tran);
        // jlgpu_task_param_3x3_gaussian_high_level(&task_param, &task_param.texture.tran);
        /* 继承父控件的变换效果 */
        JLGPU_TASK_INHERIT_PARENT(&task_param.texture.tran);

#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
        /* 本来就在RAM中缓存的数据不需要放到PSRAM，需要从flash取的数据再用PSRAM缓存 */
        if (!img->used_ram) {
            void *fp = NULL;
            if (text->format == UI_TEXT_ENCODE_IMAGE) {
                fp = (void *)ui_load_info_table[dc->prj].res;
            } else  {
                fp = (void *)ui_load_info_table[dc->prj].str;
            }
            task_param.texture.data = cache_gpu_input_data(dc->gpu_task_head, &task_param, (void *)fp, dc->index);

        }
#endif
        if (!task_param.gaussian_en) {
            /* task_param.task_type = GPU_TASK_TRANS | GPU_TEXTURE_AFFINE; */
            task_param.task_type = GPU_TASK_IMAGE_TRANS(JLGPU_TRANS_MODE_AFFINE);
        }
        if (dc->gpu_task_mode == GPU_TASK_MODE_PERSPECTIVE) {
            /* task_param.task_type = GPU_TASK_TRANS | GPU_TEXTURE_PERSPECTIVE; */
            task_param.task_type = GPU_TASK_IMAGE_TRANS(JLGPU_TRANS_MODE_PROJECTION);
        }

        jlgpu_task_texture_param_map_707n();
        jlgpu_update_task_by_id(dc->gpu_task_head, task_param.task_id, task_param.element_id, &task_param);

        elm_index++;
        void *curr_img = img;
        img = img->next;
        jlui_free(curr_img, UI_FRAME_RAM, (CTRL_TYPE_TEXT << 16 | UI_MODULE_WIDGET));	// 释放缓存buf
    }

    /* 清空多余的GPU任务，避免上一次显示内容比这次多时，部分内容没有清空 */
    jlgpu_scheduler_wait_sync();
    jlgpu_task_clean_up_by_id(dc->gpu_task_head, dc->elm->id, elm_index);

    log_info("%s(). leave. dc: 0x%p, elm_id: 0x%x\n", __func__, dc, dc->elm->id);
    return 0;
}

static int jlui_draw_rect(struct draw_context *dc, struct css_border *border)
{
    int color = border->color & 0xffff;

    if (border->left) {
        if (dc->rect.left >= dc->draw.left &&
            dc->rect.left < rect_right(&dc->draw)) {
            __draw_vertical_line(dc, dc->draw.left, dc->draw.top,
                                 border->left, dc->draw.height, color, DC_DATA_FORMAT_MONO);
        }
    }
    if (border->right) {
        if (rect_right(&dc->rect) >= dc->draw.left &&
            rect_right(&dc->rect) <= rect_right(&dc->draw)) {
            __draw_vertical_line(dc, dc->draw.left + dc->draw.width - border->right, dc->draw.top,
                                 border->right, dc->draw.height, color, DC_DATA_FORMAT_MONO);
        }
    }

    if (border->top) {
        if (dc->rect.top >= dc->draw.top &&
            dc->rect.top <= rect_bottom(&dc->draw)) {
            __draw_line(dc, dc->draw.left, dc->draw.top,
                        dc->draw.width, border->top, color, DC_DATA_FORMAT_MONO);
        }
    }

    if (border->bottom) {
        if (rect_bottom(&dc->rect) >= dc->draw.top &&
            rect_bottom(&dc->rect) <= rect_bottom(&dc->draw)) {
            __draw_line(dc, dc->draw.left, dc->draw.top + dc->draw.height - border->bottom,
                        dc->draw.width, border->bottom, color, DC_DATA_FORMAT_MONO);
        }
    }

    return 0;
}

AT_UI_RAM
u32 jlui_read_point(struct draw_context *dc, u16 x, u16 y)
{
    log_debug("%s(), dc: 0x%p, x: %d, y: %d\n", __func__, dc, x, y);
    u32 pixel = 0;
    u16 *pdisp = (u16 *)dc->buf;

    int offset = (y - dc->disp.top) * dc->disp.width + (x - dc->disp.left);
    ASSERT((offset * 2 + 1) < dc->len, "dc->len:%d", dc->len);
    if ((offset * 2 + 1) >= dc->len) {
        return -1;
    }
    pixel = pdisp[offset];//(dc->buf[offset * 2] << 8) | dc->buf[offset * 2 + 1];

    return pixel;
}

__attribute__((always_inline_when_const_args))
AT_UI_RAM
int jlui_draw_point(struct draw_context *dc, u16 x, u16 y, u32 pixel)
{
    log_debug("%s(), dc: 0x%p, x:%d, y:%d, pixel:0x%x\n", __func__, dc, x, y, pixel);
#if 1
    int offset = (y - dc->disp.top) * dc->disp.width + (x - dc->disp.left);
    if ((offset * 2 + 1) >= dc->len) {
        return -1;
    }
    dc->buf[offset * 2    ] = pixel >> 8;
    dc->buf[offset * 2 + 1] = pixel;
#else
    if ((x >= __this->info.width) || (y >= __this->info.height)) {
        return -1;
    }
    if (pixel & BIT(31)) {
        dc->buf[y / 8 * __this->info.width + x] ^= BIT(y % 8);
    } else if (pixel == 0x55aa) {
        dc->buf[y / 8 * __this->info.width + x] &= ~BIT(y % 8);
    } else if (pixel) {
        dc->buf[y / 8 * __this->info.width + x] |= BIT(y % 8);
    } else {
        dc->buf[y / 8 * __this->info.width + x] &= ~BIT(y % 8);
    }
#endif
    return 0;
}

static int jlui_invert_rect(struct draw_context *dc, u32 acolor)
{
    log_debug("%s(), dc: 0x%p, acolor: 0x%x\n", __func__, dc, acolor);
    int w, h;
    int color = acolor & 0xffff;
    color |= BIT(31);
    for (h = 0; h < dc->draw.height; h++) {
        for (w = 0; w < dc->draw.width; w++) {
            if (platform_api->draw_point) {
                platform_api->draw_point(dc, dc->draw.left + w, dc->draw.top + h, color);
            }
        }
    }
    return 0;
}

static int jlui_read_image_info(struct draw_context *dc, u32 id, struct ui_image_attrs *attrs)
{
    struct image_file file = {0};

    if (((u16) - 1 == id) || ((u32) - 1 == id)) {
        return -1;
    }

    int err = open_image_by_id(dc->prj, NULL, &file, id, dc->page);
    if (err) {
        return -EFAULT;
    }
    if ((file.format == PIXEL_FMT_AL88)
        || (file.format == PIXEL_FMT_AL44)
        || (file.format == PIXEL_FMT_AL22)
        || (file.format == PIXEL_FMT_L8)
        || (file.format == PIXEL_FMT_L4)
        || (file.format == PIXEL_FMT_L2)
        || (file.format == PIXEL_FMT_L1)
        || (file.format == PIXEL_FMT_A8)
        || (file.format == PIXEL_FMT_A4)
        || (file.format == PIXEL_FMT_A2)
        || (file.format == PIXEL_FMT_A1)
       ) {
        int tab_size = get_clut_format_tabsize(file.format, file.clut_format);
        file.offset += tab_size;
        file.len -= tab_size;
    }
    struct flash_file_info file_info;	// flash 信息
    ui_res_get_image_flash_info(&file_info, &ui_load_info_table[dc->prj].image_file_info, &file);

    attrs->width = file.width;
    attrs->height = file.height;
    attrs->format = file.format;
    attrs->clut_format = file.clut_format;
    attrs->compress = file.compress;
    attrs->has_clut = file.has_clut;
    attrs->len = file.len;
    attrs->gpu_format = res_format_to_gpu(file.format);
    attrs->data = (u8 *)file_info.offset;
    attrs->tab = (u8 *)file_info.tab;

#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
    void *cache_addr = gpu_cache_check(file.data_crc);
    int tab_size = get_clut_format_tabsize(attrs->format, attrs->clut_format);
    if (cache_addr) {
        attrs->data = cache_addr;
    } else {
        if (attrs->has_clut == CLUT_TAB_FROM_FLASH) {
            attrs->data = (u8 *)((u32)attrs->data - tab_size);
            attrs->len += tab_size;
        }
        void *fp = ui_load_info_table[dc->prj].res;
        cache_addr = gpu_cache_add(fp, attrs->data, attrs->len, file.data_crc, file_info.tab, file_info.tab_size);
        attrs->data = cache_addr;
    }
    if (attrs->has_clut == CLUT_TAB_FROM_FLASH) {
        attrs->data = (u8 *)((u32)attrs->data + tab_size);
        attrs->len -= tab_size;
    }
    if (cache_addr) {
        gpu_cache_valid_value_set((u32)cache_addr, 0xff);
    }
    attrs->tab = NULL;
#endif

    return 0;
}
void gpu_free_frame_buf(void *p)
{
    jlui_free(p, UI_RAM_SRAM, UI_MODULE_FRAME);
}

static int jlui_set_timer(void *priv, void (*callback)(void *), u32 msec)
{
    int timer_id = (int)sys_timer_add(priv, callback, msec);
    log_debug("%s(), priv: 0x%p, msec: %d, timer_id: %d\n", __func__, priv, msec, timer_id);
    return timer_id;
}

static int jlui_del_timer(int timer_id)
{
    log_debug("%s(), timer_id: %d\n", __func__, timer_id);
    if (timer_id) {
        sys_timer_del(timer_id);
    }
    return 0;
}


static const struct ui_platform_api jlui_platform_api = {
    .malloc             = jlui_malloc,
    .free               = jlui_free,

    .load_style         = jlui_load_style,
    .load_window        = jlui_load_window,
    .unload_window      = jlui_unload_window,

    .open_draw_context  = jlui_open_draw_context,
    .get_draw_context   = jlui_get_draw_context,
    .put_draw_context   = jlui_put_draw_context,
    .close_draw_context = jlui_close_draw_context,
    .set_draw_context   = jlui_set_draw_context,

    .show_element       = jlui_show_element,
    .hide_element       = jlui_hide_element,
    .delete_element     = jlui_delete_element,
    .release_element	= jlui_release_element,
    .set_element_prior  = jlui_set_element_prior,

    .fill_rect          = jlui_fill_rect,
    .draw_image         = jlui_draw_image,
    .show_text          = jlui_show_text,

    .draw_rect          = jlui_draw_rect,
    .read_point         = jlui_read_point,
    .draw_point         = jlui_draw_point,
    .invert_rect        = jlui_invert_rect,

    .read_image_info    = jlui_read_image_info,

    .set_timer          = jlui_set_timer,
    .del_timer          = jlui_del_timer,
};


#include "ui.h"
static void jlui_calc_old_struct_size()
{
    printf("[BUF_CALC]@@@@@@@@@@@@@@@ =====>>> %s() old struct size:\n", __func__);
    printf("draw_context: %lu\n",	sizeof(struct draw_context));
    printf("element_css: %lu\n",	sizeof(struct element_css));
    printf("element: %lu\n",		sizeof(struct element));
    printf("element_text: %lu\n",	sizeof(struct element_text));
    printf("text_attrs: %lu\n",		sizeof(struct ui_text_attrs));
    printf("window: %lu\n",			sizeof(struct window));
    printf("layer: %lu\n",			sizeof(struct layer));
    printf("layout: %lu\n",			sizeof(struct layout));
    printf("text: %lu\n",			sizeof(struct ui_text));
    printf("pic: %lu\n", 			sizeof(struct ui_pic));
    printf("number: %lu\n", 		sizeof(struct ui_number));
    printf("grid: %lu\n", 			sizeof(struct ui_grid));
    printf("button: %lu\n", 		sizeof(struct button));
    printf("time: %lu\n", 			sizeof(struct ui_time));
    printf("slider: %lu\n", 		sizeof(struct ui_slider));
    printf("vslider: %lu\n", 		sizeof(struct ui_vslider));
    printf("progress: %lu\n", 		sizeof(struct ui_progress));
    printf("multiprogress: %lu\n", 	sizeof(struct ui_multiprogress));
    printf("compass: %lu\n", 		sizeof(struct ui_compass));
    printf("battery: %lu\n", 		sizeof(struct ui_battery));
    printf("watch: %lu\n", 			sizeof(struct ui_watch));
}

#if 0
static void jlui_calc_new_struct_size()
{
    printf("[BUF_CALC]@@@@@@@@@@@@@@@ =====>>> %s() new struct size:\n", __func__);
    printf("draw_context: %lu\n",	sizeof(struct draw_context_renew));
    printf("element_css: %lu\n",	sizeof(struct element_css_renew));
    printf("element: %lu\n",		sizeof(struct element_renew));
    printf("element_text: %lu\n",	sizeof(struct element_text_renew));
    printf("text_attrs: %lu\n",		sizeof(struct ui_text_attrs_new));
    printf("window: %lu\n",			sizeof(struct window_renew));
    printf("layer: %lu\n",			sizeof(struct layer_renew));
    printf("layout: %lu\n",			sizeof(struct layout_renew));
    printf("button: %lu\n", 		sizeof(struct button_renew));
    printf("time: %lu\n", 			sizeof(struct ui_time_renew));
    printf("slider: %lu\n", 		sizeof(struct ui_slider_renew));
    printf("button: %lu\n", 		sizeof(struct button_renew));
    printf("time: %lu\n", 			sizeof(struct ui_time_renew));
    printf("slider: %lu\n", 		sizeof(struct ui_slider_renew));
    printf("vslider: %lu\n", 		sizeof(struct ui_vslider_renew));
    printf("progress: %lu\n", 		sizeof(struct ui_progress_renew));
    printf("multiprogress: %lu\n", 	sizeof(struct ui_multiprogress_renew));
    printf("compass: %lu\n", 		sizeof(struct ui_compass_renew));
    printf("battery: %lu\n", 		sizeof(struct ui_battery_renew));
    printf("watch: %lu\n", 			sizeof(struct ui_watch_renew));
}
#endif



int ui_platform_init(void *lcd)
{
    struct rect rect;

    __this->api = (struct ui_platform_api *)&jlui_platform_api;
    ASSERT(__this->api->open_draw_context);
    ASSERT(__this->api->get_draw_context);
    ASSERT(__this->api->put_draw_context);
    ASSERT(__this->api->set_draw_context);
    ASSERT(__this->api->close_draw_context);
    __this->background = 0x000000;	// 设置背景颜色

    __this->lcd = lcd_get_hdl();
    ASSERT(__this->lcd);
    ASSERT(__this->lcd->init);
    ASSERT(__this->lcd->get_screen_info);
    ASSERT(__this->lcd->buffer_malloc);
    ASSERT(__this->lcd->buffer_free);
    ASSERT(__this->lcd->draw);
    ASSERT(__this->lcd->set_draw_area);
    ASSERT(__this->lcd->backlight_ctrl);

    if (__this->lcd->init) {
        __this->lcd->init(lcd);
    }

    if (__this->lcd->get_screen_info) {
        __this->lcd->get_screen_info(&__this->info);
    }

    ui_resfile_info_init(jlui_malloc, jlui_free);		// UI 资源管理
    jlgpu_scheduler_init(jlui_malloc, jlui_free, __this->info.width, __this->info.height);
    buffer_manager_callback(jlui_malloc, jlui_free);	// buffer 管理模块初始化

    /* jpg联动模块缓存预分配（硬件按需初始化，避免与camera编码互斥） */
    printf("JPG DEC CACHE ALLOC ----- \n");
    int a_size, yuv_size;
    jpg_gxdec_get_cache_size(GXDEC_BLOCK_32X32, GXDEC_CACHE_CNT_4, &yuv_size, &a_size); // 获取联动模块所需缓存
    g_gxdec_yuv_base = (u8 *)jlui_malloc(yuv_size, UI_FRAME_RAM, UI_MODULE_JPEG); // 申请联动模块缓存
    g_gxdec_a_base = (u8 *)jlui_malloc(a_size, UI_FRAME_RAM, UI_MODULE_JPEG);
    spin_lock_init(&g_jpg_ui_dec_lock);


    if (__this->lcd->clear_screen) {
        __this->lcd->clear_screen(__this->background, 0, __this->info.width - 1, 0, __this->info.height - 1);
    }

    os_time_dly(4);

    if (__this->lcd->backlight_ctrl) {
        __this->lcd->backlight_ctrl(100);
    }

    rect.left   = 0;
    rect.top    = 0;
    rect.width  = __this->info.width;
    rect.height = __this->info.height;
    printf("ui_platform_init :: [%d,%d,%d,%d]\n", rect.left, rect.top, rect.width, rect.height);


#ifdef UI_BUF_CALC
    INIT_LIST_HEAD(&buffer_used.list);
#endif

    /* gpu_manager_test(jlui_malloc, jlui_free); */

    ui_core_init(__this->api, &rect);
    ui_anim_core_init();
    ui_anim_set_run_task("ui");
    ui_multi_page_init(multi_page_info_table, sizeof(multi_page_info_table) / sizeof(multi_page_info_table[0]));


#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
    int lcd_fps_us = lcd_get_te_frame_period_us();
    ui_anim_set_interval(lcd_fps_us * (100 - 20) / 100 / 1000);
#else
    /* int lcd_fps_us = 16666; //lcd_fps_us * (100 - 20) / 100 / 1000 */
    ui_anim_set_interval(13);
#endif

    /* jlui_calc_old_struct_size(); */
    /* jlui_calc_new_struct_size(); */
    printf("@@@@@@@@@@@@@@@@@@@@ \n\n\n");

    INIT_LIST_HEAD(&gif_timer_list.head);
    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
        async_buffer_init();
    }

    return 0;
}

int ui_relate_init()
{
    if (__this->ui_res_init) {
        return 0;
    }
    printf("ui relate init.\n");
    /* font_ascii_init(RES_PATH"font/ascii.res");//打开ascii.res */
    /* ui_qrcode_init(); */
    ui_disp_line_buf_init();
    /* ui_font_arabic_init(); */
    __this->ui_res_init = 1;
    return 0;
}

int ui_relate_uninit()
{
    if (!__this->ui_res_init) {
        return 0;
    }
    printf("ui relate uninit.\n");
    gpu_free_all_tasks();
    /* font_ascii_init(NULL); 				//关闭ascii.res */
    text_font_init(false); 					//关闭字库文件
    ui_unload_file_info(); 					//关闭ui_load_info_table里的所有文件
    /* ui_qrcode_uninit(); */

    /* jpeg_module_opj_clr_all(); */
    /* jljpeg_get_decode_hd_clr(); */

    ui_disp_line_buf_uninit();				//释放绘图buf
    /* ui_font_arabic_uninit(); */
    ui_gpu_buf_release();					//释放绘图gpu加速buf
    /* void malloc_list_status_printf_all(int index, int count); */
    /* malloc_list_status_printf_all(0, 200);	//ram内存统计 */
    /* malloc_list_status_printf_all(1, 200);	//psram内存统计 */
    mem_stats();								//sram内存统计
    /* jlui_malloc_ram_info_clr(0x2); */
    /* void ui_module_ram_dump(); */			//ui内存统计
    /* ui_module_ram_dump(); */					//ui内存统计
    __this->ui_res_init = 0;

    if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
        async_buffer_clr();
    }

#if TCFG_PSRAM_DEV_ENABLE
    psram_heap_stats();							//psram内存打印
    if (psram_get_used_block()) { 				//存在psram buf未释放
        printf("psram stanby");					//维持数据,功耗会高一些
    } else {
        printf("psram power_down");
    }
#endif
    return 0;
}

#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
void *cache_gpu_input_data(void *head, pJLGPUTaskParam_t task_param, void *fp, int index)
{
    /* SRAM里面的数据不需要缓存 */
    /* if ((u32)task_param->texture.data < 0x04000000) { */
    /* return task_param->texture.data; */
    /* } */
    /* 如果有颜色表，需要把颜色表也缓存到PSRAM */
    int tab_size = get_clut_format_tabsize(gpu_format_to_res(task_param->format), task_param->clut_format);
    if (task_param->has_clut == CLUT_TAB_FROM_FLASH) {
        task_param->texture.data = (u8 *)((u32)task_param->texture.data - tab_size);
        task_param->image.len += tab_size;
    }

    u32 tab[3] ALIGNED(4);
    tab[0] = (u32)fp;
    tab[1] = (u32)task_param->texture.data;
    tab[2] = task_param->image.len;
    u16 data_crc = CRC16(&tab[0], sizeof(tab));
    /* 检查是否已经有缓存，如果需要缓存的图片没在cache里，释放原来的，缓存新的 */
    void *cache_addr = gpu_cache_check(data_crc);
    int vaild_index = index;
    /* printf("@@@@@ %s cache_addr: 0x%x\n", __func__,(u32)cache_addr); */
    if (cache_addr) {
        task_param->texture.data = cache_addr;
        task_param->texture.mmu_tab_base = NULL;
    } else {
#if 0//改为统一释放
        void *gpu_task_unit = jlgpu_get_task_by_id(head, task_param->task_id, task_param->element_id);
        if (gpu_task_unit) {
            extern void *jlgpu_unit_to_instruction(pJLGPUTaskUnit_t taskp);
            void *gpu_task_instruct = jlgpu_unit_to_instruction(gpu_task_unit);
            u32 mmu_en, mmu_tb, addr = 0, rle_limit;
            jlgpu_driver_get_texture_addr(gpu_task_instruct, &mmu_en, &mmu_tb, &addr, &rle_limit);
            if (addr && memory_in_psram((void *)addr)) {
                gpu_cache_del((void *)addr);
            }
        }
#endif
        cache_addr =  gpu_cache_add(fp, task_param->texture.data, task_param->image.len, data_crc, task_param->texture.mmu_tab_base, (task_param->info.tab_size >> 2));
        task_param->texture.data = cache_addr;
        task_param->texture.mmu_tab_base = NULL;
    }
    /* 如果有颜色表，图片数据的起始坐标要偏移回去 */
    if (task_param->has_clut == CLUT_TAB_FROM_FLASH) {
        task_param->has_clut = CLUT_TAB_FROM_RAM;
        task_param->clut_tab = (u8 *)jlui_malloc(tab_size, UI_RAM_SRAM, UI_MODULE_CACHE);
        memcpy(task_param->clut_tab, task_param->texture.data, tab_size);
        task_param->texture.data = (u8 *)((u32)task_param->texture.data + tab_size);
        task_param->image.len -= tab_size;
    }

    /* printf("texture data addr: 0x%x\n", (u32)task_param->texture.data); */
    if (cache_addr) {
        gpu_cache_valid_value_set((u32)cache_addr, 3);
    }
    return task_param->texture.data;
}
#endif // TCFG_NANDFLASH_DEV_ENABLE


pJLGPUTaskHead_t jlui_load_win_task_list(pJLGPUTaskHead_t head, u32 win_id, int index, int hide)
{
    pJLGPUTaskHead_t new_head;
    u32 cur_id = ui_get_current_window_id();
    if (cur_id != win_id) {
        extern int window_init(int id);
        window_init(win_id);
    }

    struct element *elm = ui_core_get_element_by_id(win_id);
    if (!elm) {
        ASSERT(0, "Can not get window element, window id: 0x%x\n", win_id);
        return NULL;
    }

    elm->dc->refresh = false;
    ui_core_show(elm, true);
    new_head = jlgpu_task_list_copy_create(head, elm->dc->gpu_task_head, NULL, index);
    /* BR33 创建链表头，将窗口设定到0点 */
    new_head->win_rect.left = 0;
    new_head->win_rect.top  = 0;

    /* if (cur_id != win_id) { */
    if (hide) {
        ui_hide(win_id);
    }

    return new_head;
}

#endif




