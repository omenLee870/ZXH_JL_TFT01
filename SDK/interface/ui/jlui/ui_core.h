/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_core.h
 *
 * @brief 杰理UI框架核心
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2024-06-04
 *
 * 注意：本文件修改会影响到UI资源结构，UI工具等内容，禁止随意改动
 */
/* ------------------------------------------------------------------------------------*/

#ifndef UI_ELEMENT_CORE_H
#define UI_ELEMENT_CORE_H

#include "typedef.h"
#include "rect.h"
#include "system/event.h"
#include "res/resfile.h"
#include "jlui/ui_effect.h"
#include "font/font_all.h"


#define UI_CTRL_BUTTON  0

struct element;

#define UI_COLOR_RGB555_T0_RGB565(color)   (((((color) >> 10) & 0x1f) << 11) | ((((color) >> 5) & 0x1f) << 6) | ((color) & 0x1f))

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 使能PSRAM标记
 */
/* ------------------------------------------------------------------------------------*/
extern const int ENABLE_PSRAM_UI_FRAME;


#define COLOR_INVALID_VALUE		0xffff	// 无效的颜色
#define COLOR_DEFAULT_VALUE		0x0000	// 默认的颜色

#define COLOR_VALID(c)		(((c) & COLOR_INVALID_VALUE) != COLOR_INVALID_VALUE)	// 判断颜色是有效的
#define COLOR_INVALID(c)	(((c) & COLOR_INVALID_VALUE) == COLOR_INVALID_VALUE)	// 判断颜色是无效的



#define IMAGE_INVALID_VALUE		0xffff	// 无效的图片
#define IMAGE_DEFAULT_VALUE		0x0000	// 默认的图片

#define IMAGE_VALID(p)		(((p) != IMAGE_INVALID_VALUE) && ((p) > 0))	// 判断图片是有效的
#define IMAGE_INVALID(p)	(((p) == IMAGE_INVALID_VALUE) || ((p) <= 0))	// 判断图片是无效的


#ifdef offsetof
#undef offsetof
#endif
#ifdef container_of
#undef container_of
#endif

#define offsetof(type, memb) \
	((unsigned long)(&((type *)0)->memb))

#define container_of(ptr, type, memb) \
	((type *)((char *)ptr - offsetof(type, memb)))

#define ui_element_for_id(id, type) \
	({ \
	 struct element *elm = ui_core_get_element_by_id(id); \
	 elm ? container_of(elm, type, elm) : NULL; \
	 })


/* ------------------------------------------------------------------------------------*/
/**
 * @brief UI使用的RAM类型定义
 */
/* ------------------------------------------------------------------------------------*/
typedef enum ui_ram_type {
    UI_RAM_SRAM,
    UI_RAM_PSRAM,
    UI_RAM_CACHE,
} UI_RamType;

#define UI_FRAME_RAM	UI_RAM_SRAM		// 定义UI框架使用的ram类型


/* ------------------------------------------------------------------------------------*/
/**
 * @brief UI各模块类型定义
 */
/* ------------------------------------------------------------------------------------*/
typedef enum ui_module_type {
    UI_MODULE_CORE,		// UI核心
    UI_MODULE_FRAME,	// UI框架
    UI_MODULE_WIDGET,	// UI控件
    UI_MODULE_RESOURCE,	// RES
    UI_MODULE_GPU,		// GPU
    UI_MODULE_DBI,		// DBI
    UI_MODULE_FONT,		// UI字库
    UI_MODULE_BUFFER,	// BUFFER管理
    UI_MODULE_JPEG,		// JPEG
    UI_MODULE_CACHE,    // gpu_cache
    UI_MODULE_CUSTOM_DRAW, // 自定义绘图

    UI_MODULE_MAX = 31,//
} UI_ModuleType;


enum ui_direction {
    UI_DIR_UP,
    UI_DIR_DOWN,
    UI_DIR_LEFT,
    UI_DIR_RIGHT,
};

#define  	KEY_LEFT 			(0x44)//KEY_UI_MINUS
#define  	KEY_RIGHT 			(0x45)//KEY_UI_PLUS
#define  	KEY_UP 				(0x46)//KEY_UI_MINUS
#define  	KEY_DOWN 			(0x47)//KEY_UI_PLUS

enum {
    KEY_EVENT_CLICK,
    KEY_EVENT_LONG,
    KEY_EVENT_HOLD,
    KEY_EVENT_UP,
    KEY_EVENT_DOUBLE_CLICK,
    KEY_EVENT_TRIPLE_CLICK,
    KEY_EVENT_FOURTH_CLICK,
    KEY_EVENT_FIRTH_CLICK,
    KEY_EVENT_USER,
    KEY_EVENT_MAX,
};
enum {
    POSITION_ABSOLUTE = 0,
    POSITION_RELATIVE = 1,
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 触摸事件类型
 */
/* ------------------------------------------------------------------------------------*/
enum {
    ELM_EVENT_TOUCH_DOWN,
    ELM_EVENT_TOUCH_MOVE,
    ELM_EVENT_TOUCH_R_MOVE,
    ELM_EVENT_TOUCH_L_MOVE,
    ELM_EVENT_TOUCH_D_MOVE,
    ELM_EVENT_TOUCH_U_MOVE,
    ELM_EVENT_TOUCH_HOLD,
    ELM_EVENT_TOUCH_UP,
    ELM_EVENT_TOUCH_ENERGY,
    ELM_EVENT_TOUCH_STOP,
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 按键事件类型
 */
/* ------------------------------------------------------------------------------------*/
enum {
    ELM_EVENT_KEY_CLICK,
    ELM_EVENT_KEY_LONG,
    ELM_EVENT_KEY_HOLD,
};

enum {
    ELM_STA_INITED,
    ELM_STA_READY,
    //ELM_STA_SHOW_PROBE,
    //ELM_STA_SHOW_POST,
    ELM_STA_HIDE,
    ELM_STA_SHOW,
    ELM_STA_PAUSE,
};

enum {
    ELM_FLAG_NORMAL,
    ELM_FLAG_HEAD,
};

enum {
    DC_DATA_FORMAT_OSD8 = 0,
    DC_DATA_FORMAT_YUV420 = 1,
    DC_DATA_FORMAT_OSD16 = 2,
    DC_DATA_FORMAT_OSD8A = 3,
    DC_DATA_FORMAT_MONO = 4,
};


struct element_touch_event {
    int event;
    int xoffset;
    int yoffset;
    u8  hold_up;
    u8  onfocus;
    u8  move_flag;
    struct position pos;
    struct position origin;
    void *private_data;
    int has_energy;
};

struct element_key_event {
    u8 event;
    u8 value;
    void *private_data;
};

#define ELM_KEY_EVENT(e) 		(0x0000 | (e->event) | (e->value << 8))
#define ELM_TOUCH_EVENT(e) 		(0x1000 | (e->event))
#define ELM_CHANGE_EVENT(e) 	(0x2000 | (e->event))


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ON_CHANGE事件类型
 */
/* ------------------------------------------------------------------------------------*/
enum element_change_event {
    ON_CHANGE_INIT_PROBE,
    ON_CHANGE_INIT,
    ON_CHANGE_TRY_OPEN_DC,
    ON_CHANGE_FIRST_SHOW,
    ON_CHANGE_SHOW_PROBE,
    ON_CHANGE_SHOW,
    ON_CHANGE_SHOW_POST,
    ON_CHANGE_HIDE,
    ON_CHANGE_HIGHLIGHT,
    ON_CHANGE_RELEASE_PROBE,
    ON_CHANGE_RELEASE,
    ON_CHANGE_ANIMATION_END,
    ON_CHANGE_SHOW_COMPLETED,
    ON_CHANGE_UPDATE_ITEM,
};


struct element_event_handler {
    int id;
    int (*ontouch)(void *, struct element_touch_event *);
    int (*onkey)(void *, struct element_key_event *);
    int (*onchange)(void *, enum element_change_event, void *);
};

struct jaction {
    u32 show;
    u32 hide;
};

enum {
    ELM_ACTION_HIDE = 0,
    ELM_ACTION_SHOW,
    ELM_ACTION_TOGGLE,
    ELM_ACTION_HIGHLIGHT,
};
enum {
    UI_CUSTOM_COLOR_BIT_IMAGE,
    UI_CUSTOM_COLOR_BIT_TEXT,
    //预留1
    //预留2
};//max4bit
enum {
    GPU_TASK_MODE_DEFAULT,
    GPU_TASK_MODE_AFFINE,
    GPU_TASK_MODE_PERSPECTIVE,
};
struct event_action {
    u16 event;
    u16 action;
    int id;
    u8  argc;
    char argv[];
};

struct element_event_action {
    u16 num;
    struct event_action action[0];
};

struct image_preview {
    struct flash_file_info *file_info;
    UI_RESFILE *file;
    int id;
    int page;
};


struct image {
    s16 x;
    s16 y;
    u16 id;
    u16 page;
    u32 en: 1;
    u32 invert_x_en: 2;
    u32 invert_y_en: 2;
    u32 invert_x_dist: 12;
    u32 invert_y_dist: 12;
    struct rect rect;
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 显示上下文句柄结构
 */
/* ------------------------------------------------------------------------------------*/
struct draw_context {
    u8 ref;			// 递归计数，防止嵌套
    u8 alpha;		// 透明度
    u8 hori_align;	// 水平对齐
    u8 vert_align;	// 垂直对齐

    u8 prj;			// 工程ID，与UI资源对应
    u8 page;		// 页面ID，与UI资源对应
    u8 buf_num;		// UI框架支持的单、双buf模式，底层用到
    u8 refresh;

    u32 background_color;	// 背景颜色
    struct element *elm;	// 控件成员指针
    struct rect rect;
    struct rect draw;
    void *dc;

    struct image_preview preview;	// 预览图结构体

    struct rect page_draw;
    struct rect need_draw;
    struct rect disp;
    struct rect rect_orig;
    struct rect rect_ref;//referance of page rect
    struct rect lcd_rect;
    u16 width;	// DC绘图宽高
    u16 height;

    u8 *buf;	// 显存buf地址
    u32 len;	// 显存buf长度
    u16 lines;	// 分块行数
    u16 colums;

    u8 col_align;	// 行、列对齐
    u8 row_align;
    u8 index;
    u8 elm_index;

    u8 draw_state;	// 绘制状态，合成、推屏、合成并推屏

    // BR35 GPU 任务
    u8 crop_en : 4;		// 裁剪使能，替代crop
    u8 custom_color : 4;	// 自定义颜色使能
    u32 custom_argb8888;	// 自定义颜色值
    // u16 custom_rgb565;		// 自定义颜色值
    void *gpu_task_head;	// GPU 任务链
    int gpu_task_dont_sort; // GPU 任务不排序
    void *gpu_mult_list;	// 多链表管理
    void *buffer_hdl;		// 显存管理句柄
    void *buffer_adr;		// 当前显存地址
    u32 asyn_redraw: 1;		// GPU 和 UI 任务异步
    u32	gpu_task_mode: 2;	// 指定GPU任务大类，默认/仿射/透视,特效使用
    struct image draw_img;

    /*********psram和特效相关变量*************/
    //滑动页面时,当新页第一次复制到psram时为1，其他时候为0
    //可用于滑动时,特效只需要处理一次的情况
    u8 new_copy;
    u8 new_page;
    u8 slider;
    u8 just_record;

    u8 lr_status;//0:left 1:right
    u8 copy_to_psram;
    u16 effect_mode;
    effect_cb effect_user;//用户自定义特效处理的回调函数
    void *effect_priv; //页面特效私有值,可以用来传递特效参数
    /********************************************/

    float scale_ratio;
    int scale_mode;
    int scale_prior;

    struct rect page_r;
    struct rect gpu_r;
    struct rect lcd_r;

    u8 gpu_free_data;

};	// 296byte


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 边框属性
 */
/* ------------------------------------------------------------------------------------*/
struct css_border {
    u16 left: 4;
    u16 top: 4;
    u16 right: 4;
    u16 bottom: 4;
    u16 color/* : 16 */;
};	// 4byte


struct css_border1 {
    u16 left: 4;
    u16 top: 4;
    u16 right: 4;
    u16 bottom: 4;
    u16 color;
}; // 4 bytes


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 旋转属性
 */
/* ------------------------------------------------------------------------------------*/
struct css_rotate {
    s16 cent_x;
    s16 cent_y;

    s16 dx;
    s16 dy;

    float angle;
};	// 12byte



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 缩放属性
 */
/* ------------------------------------------------------------------------------------*/
struct css_ratio {
    float ratio_w;
    float ratio_h;
};	// 8byte


struct css_part {
    // int background_image;		// 可以使用u16，这里给int用于对齐，还是应放到css结构体，因所有图片显示是都是通过背景图片传递的ID，如果放到外部，则任何需要显示图片的位置都需额外申请这个buf
    struct css_border border;	// 边框
    struct css_rotate rotate;	// 旋转
    struct css_ratio ratio;		// 缩放
};	// 28byte


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 元素属性结构
 */
/* ------------------------------------------------------------------------------------*/
struct element_css {
    u16 invisible : 1;	// 默认隐藏
    u16 z_order : 3;	// 排列顺序
    u16 hori_align: 2;		// 对齐方式，9种
    u16 vert_align: 2;		// 对齐方式，9种
    u16 alpha: 8;		// 透明度
    u16 background_color;		// 背景颜色
    u16 background_image;		// 背景图片
    u16 change_rect: 1;		//修改控件rect,缩放时有效

    u16 unused: 15;			// 未使用的保留位

    s16 left;			// 位置坐标
    s16 top;

    s16 width;
    s16 height;

    struct css_part *part;		// 旋转缩放等参数
};

struct element_css1 {
    u32 hori_align: 2;  //水平对齐 {左中右}
    u32 vert_align: 2;  //垂直对齐 {上中下}
    u32 invisible: 1;
    u32 z_order: 3;
    u32 alpha: 8;
    u32 background_color: 16; //RGB565
    s16 left;
    s16 top;
    s16 width;
    s16 height;
    struct css_border1 border; //控件边框大小
    u16 background_image;
} __attribute__((packed)); // 18 bytes

struct element_ops {
    int (*show)(struct element *);
    int (*redraw)(struct element *, struct rect *);
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 元素结构
 */
/* ------------------------------------------------------------------------------------*/
struct element {
    u32 highlight : 1;	// 高亮状态
    u32 hide_action: 1;	// 隐藏时是否刷新
    u32 rotate_en: 1;	// 旋转使能
    u32 ratio_en : 1;	// 缩放使能
    u32 css_num: 2;		// css 属性数量
    u32 state: 3;		// 控件状态
    u32 ref: 5;			// 防止重入嵌套
    u32 prior: 9;		// 优先级，调整GPU混合顺序
    u32 page: 9;		// 页面ID

    u32 id;				// 控件ID

    struct element *parent;		// 父控件指针
    struct list_head sibling;	// 兄弟控件列表
    struct list_head child;		// 子控件列表
    struct element *focus;		// 聚焦控件，特别是列表等多个控件需决策响应控件时用到。
    struct element_css css;		// 控件 CSS 属性
    struct draw_context *dc;	// DC 结构体指针
    const struct element_event_handler *handler;	// 事件句柄
};	// 80byte


struct ui_style {
    const char *file;
    u32 version;
};

enum {
    UI_FTYPE_VIDEO = 0,
    UI_FTYPE_IMAGE,
    UI_FTYPE_AUDIO,
    UI_FTYPE_DIR,
    UI_FTYPE_UNKNOW = 0xff,
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief UI文件属性
 */
/* ------------------------------------------------------------------------------------*/
struct ui_file_attrs {
    char *format;
    char fname[128];
    struct vfs_attr attr;
    u8 ftype;
    u16 file_num;
    u32 film_len;
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief UI图片属性
 */
/* ------------------------------------------------------------------------------------*/
struct ui_image_attrs {
    u16 width;
    u16 height;
    u8 format;
    u8 clut_format;
    u8 compress;
    u8 has_clut;
    u8 *tab;
    u8 *data;
    u8 *clut_tab;
    u32 len;
    u8 gpu_format;
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief UI文本属性
 */
/* ------------------------------------------------------------------------------------*/
struct ui_text_attrs {
    const char *str;

    // const char *format;	// 改为枚举
    u16 format: 3;	// 改为枚举，3bit
    u16 displen: 13;	// 显示长度类型改为u16，防止文本长度溢出导致显示异常
    // int color;		// 文本颜色
    u16 color;		// 采用RGB565颜色格式

    u16 strlen;		// 字符串长度
    short offset;

    const char *mulstr;	// 多个多国语言词条拼接时使用
    const char *font_out_buf;	// 字库描点字模的缓存buf指针，给GPU使用，文本控件释放时释放

    u16 mulstr_len;
    u16 default_code;

    u8 rev: 3;
    u8 scroll_update : 1; //定时器滚动步进更新标志
    u8 scroll_en : 1; //定时器滚动使能标志
    u8 endian: 1;	// 大小端，兼容蓝牙发过来的数据
    u8 encode: 2;	// 编码格式
    u8 flags;	// 显示单行、多行、滚动显示等, 在font_all定义。由于功能可同时开启，1bit作为一个开关
    short extra_word_space_for_dep;

    short word_space;	// 词间隔
    short line_space;	// 行间隔

    u16 width;		// 文本宽度
    u16 height;		// 文本高度

    short scroll_cnt; //滚动缓冲数
    short offsetx; //滚动缓冲内x偏移

    u16 last_crc;  //记录上一次text->str的校验值
    u16 str_width; //当前字符串显示宽度

    struct font_new_cb_priv font_priv;

    u8 charNumInSyllable;  //印地语单音节内字符数
    u8 x_interval;	// 字符间隔
    int top_extra_fill : 16;
    int bottom_extra_fill : 16;

    const char *copy_mulstr;
};	// 64 byte



struct ui_file_browser {
    int file_number;
    u8  dev_num;
    void *private_data;
};



struct ui_effect_ctrl {
    u8 has_new_page;
    void *new_page_elm;

    u16 effect_mode;
    void *user_effect_cb;
    void *effect_priv;
    int stop_redraw;
    u8 redraw_page;
};
extern struct ui_effect_ctrl ui_core_effcet;



#define ELEMENT_ALIVE 		0x53547a7b

#define element_born(elm) \
		elm->alive = ELEMENT_ALIVE

#define element_alive(elm) \
		(elm->alive == ELEMENT_ALIVE)


/* 扫描子控件链表 */
#define list_for_each_child_element(p, elm) \
	list_for_each_entry(p, &(elm)->child, sibling)

#define list_for_each_child_element_reverse(p, n, elm) \
	list_for_each_entry_reverse_safe(p, n, &(elm)->child, sibling)

#define list_for_each_child_element_safe(p, n, elm) \
	list_for_each_entry_safe(p, n, &(elm)->child, sibling)

struct ui_platform_api {
    void *(*malloc)(int size, u32 ram_type, u32 module_type);
    void (*free)(void *addr, u32 ram_type, u32 module_type);

    int (*load_style)(struct ui_style *);

    void *(*load_window)(int id);
    void (*unload_window)(void *);

    int (*open_draw_context)(struct draw_context *);
    int (*get_draw_context)(struct draw_context *);
    int (*put_draw_context)(struct draw_context *);
    int (*set_draw_context)(struct draw_context *);
    int (*close_draw_context)(struct draw_context *);

    int (*show_element)(struct draw_context *);
    int (*hide_element)(struct draw_context *);
    int (*delete_element)(struct draw_context *);
    int (*release_element)(struct draw_context *);
    int (*set_element_prior)(struct draw_context *);

    int (*fill_rect)(struct draw_context *, u32 color);
    int (*draw_rect)(struct draw_context *, struct css_border *border);
    int (*draw_image)(struct draw_context *, u32 src, u8 quadrant, u8 *mask);
    int (*draw_point)(struct draw_context *, u16 x, u16 y, u32 color);
    u32(*read_point)(struct draw_context *dc, u16 x, u16 y);
    int (*invert_rect)(struct draw_context *, u32 color);
    int (*draw_primitive)(struct draw_context *dc, u8 *buf, int x, int y, int width, int height, void *cb, void *priv, int priv_len, int id);

    void *(*load_widget_info)(void *_head, u8 page);
    void *(*load_css)(u8 page, void *_css);
    void *(*load_image_list)(u8 page, void *_list);
    void *(*load_text_list)(u8 page, void *__list);

    //int (*highlight)(struct draw_context *);
    int (*show_text)(struct draw_context *, struct ui_text_attrs *);
    int (*read_image_info)(struct draw_context *, u32, struct ui_image_attrs *);

    int (*open_device)(struct draw_context *, const char *device);
    int (*close_device)(int);

    int (*set_timer)(void *, void (*callback)(void *), u32 msec);
    int (*del_timer)(int);

    struct ui_file_browser *(*file_browser_open)(struct rect *r,
            const char *path, const char *ftype, int show_mode);

    int (*get_file_attrs)(struct ui_file_browser *, struct ui_file_attrs *attrs);

    int (*set_file_attrs)(struct ui_file_browser *, struct ui_file_attrs *attrs);

    int (*clear_file_preview)(struct ui_file_browser *, struct rect *r);

    int (*show_file_preview)(struct ui_file_browser *, struct rect *r, struct ui_file_attrs *attrs);

    int (*flush_file_preview)(struct ui_file_browser *);

    void *(*open_file)(struct ui_file_browser *, struct ui_file_attrs *attrs);
    int (*delete_file)(struct ui_file_browser *, struct ui_file_attrs *attrs);

    int (*move_file_preview)(struct ui_file_browser *_bro, struct rect *dst, struct rect *src);

    void (*file_browser_close)(struct ui_file_browser *);

};

extern /* const */ struct ui_platform_api *platform_api;

extern /* const */ struct element_event_handler dumy_handler;

struct janimation {
    u8  persent[5];
    u8  direction;
    u8  play_state;
    u8  iteration_count;
    u16 delay;
    u16 duration;
    struct element_css css[0];
};


extern struct element_event_handler *elm_event_handler_begin;
extern struct element_event_handler *elm_event_handler_end;


#define ___REGISTER_UI_EVENT_HANDLER(style, _id) \
    static const struct element_event_handler element_event_handler_##_id \
			sec(.elm_event_handler_##style) __attribute__((used)) = { \
				.id = _id,

#define __REGISTER_UI_EVENT_HANDLER(style, _id) \
		___REGISTER_UI_EVENT_HANDLER(style, _id)

#define REGISTER_UI_EVENT_HANDLER(id) \
	__REGISTER_UI_EVENT_HANDLER(STYLE_NAME, id)



struct ui_style_info {
    const char *name;
    struct element_event_handler *begin;
    struct element_event_handler *end;
};

extern struct ui_style_info ui_style_begin[];
extern struct ui_style_info ui_style_end[];

#define __REGISTER_UI_STYLE(style_name) \
	extern struct element_event_handler elm_event_handler_begin_##style_name[]; \
	extern struct element_event_handler elm_event_handler_end_##style_name[]; \
    static const struct ui_style_info ui_style_##style_name sec(.ui_style) __attribute__((used)) = { \
		.name 	= #style_name, \
		.begin 	= elm_event_handler_begin_##style_name, \
		.end 	= elm_event_handler_end_##style_name, \
	};

#define REGISTER_UI_STYLE(style_name)

#define REGISTER_UI_GLOBAL_STYLE(style_name) \
	__REGISTER_UI_STYLE(style_name)

extern struct element_event_handler *element_event_handler_for_id(u32 id);
// {
//     struct element_event_handler *p;
//
//     for (p = elm_event_handler_begin; p < elm_event_handler_end; p++) {
//         if (p->id == id) {
//             return p;
//         }
//     }
//
//     return NULL;
// }



/* 获取控件的CSS属性 */
#define ui_core_get_element_css(elm)   \
	&((struct element *)(elm))->css

/* 设置控件的invisible属性 */
#define ui_core_element_invisable(elm, i)  \
		((struct element *)(elm))->css.invisible = i


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_init 杰理UI框架核心初始化
 *
 * @param api UI框架API
 * @param rect UI显示区域
 *
 * @return 0 正常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_init(const struct ui_platform_api *api, struct rect *rect);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_style 设置UI风格
 *
 * @param style UI资源路径
 *
 * @return 0 正常, -22 失败
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_set_style(const char *style);

void ui_core_set_rotate(int _rotate);

int ui_core_get_rotate();


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_malloc UI内存申请
 *
 * @param size 待申请内存大小
 * @param ram_type 预计申请的内存类型
 * @param module_type 发出申请请求的模块
 *
 * @return 申请的内存指针
 */
/* ------------------------------------------------------------------------------------*/
void *ui_core_malloc(int size, UI_RamType ram_type, UI_ModuleType module_type);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_free UI内存释放
 *
 * @param buf 待释放内存指针
 * @param buf 待释放的内存类型
 * @param buf 发出释放申请的模块
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_free(void *p, UI_RamType ram_type, UI_ModuleType module_type);


void ui_core_element_init(struct element *,
                          u32 id,
                          u8 page,
                          u8 prj,
                          u8 css_num,
                          struct element_css1 *,
                          const struct element_event_handler *
                         );

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_element_abs_rect 获取控件绝对位置(UI工具上设置的位置)
 *
 * @param elm 控件元素句柄
 * @param rect 绝对位置缓存指针（输出）
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_get_element_abs_rect(struct element *elm, struct rect *rect);

#define ui_core_get_element_own_rect(elm, rect) \
{ \
	(rect)->left = elm->css.left; \
	(rect)->top = elm->css.top; \
	(rect)->width = elm->css.width; \
	(rect)->height = elm->css.height; \
}

// 获取控件相对于屏幕的坐标
#define ui_core_get_element_rect_relative_screen(elm, rect) \
	ui_core_get_element_abs_rect(elm, rect)

// 获取控件相对于容器的坐标
#define ui_core_get_element_rect_relative_parent(elm, rect) \
	ui_core_get_element_own_rect(elm, rect)


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_append_child 添加子元素，按照z_order从小到大排序，也就是从底层往上层排列。
 * 重绘时从前往后遍历，onkey和ontouch时从后往前遍历
 *
 * @param _child 待添加的子元素句柄
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_append_child(void *_child);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_first_child 获取第一个子控件元素句柄
 *
 * @return 元素句柄
 */
/* ------------------------------------------------------------------------------------*/
struct element *ui_core_get_first_child();


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_remove_element 从链表中移除子控件
 *
 * @param _child 待移除的子控件句柄
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_remove_element(void *_child);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_open_draw_context 打开显示上下文
 *
 * @param dc 显示上下文
 * @param elm 控件元素
 *
 * @return 0 正常，其他异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_open_draw_context(struct draw_context *dc, struct element *elm);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_close_draw_context 关闭显示上下文
 *
 * @param dc 显示上下文指针
 *
 * @return 0 正常，其他异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_close_draw_context(struct draw_context *dc);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_show 显示控件元素
 *
 * @param _elm 控件元素
 * @param init 是否已经初始化(true 是，false 否)
 *
 * @return 0 正常，其他异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_show(void *_elm, int init);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_hide 隐藏控件元素
 *
 * @param _elm 控件元素
 *
 * @return 0 正常，其他异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_hide(void *_elm);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief get_element_by_id 通过ID获取子控件元素
 *
 * @param elm 父控件元素
 * @param id 子控件ID
 *
 * @return 子控件元素
 */
/* ------------------------------------------------------------------------------------*/
struct element *get_element_by_id(struct element *elm, u32 id);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_element_nowarning_by_id 通过ID获取控件元素(不提供warning警告)
 *
 * @param id 控件ID
 *
 * @return 控件元素
 */
/* ------------------------------------------------------------------------------------*/
struct element *ui_core_get_element_nowarning_by_id(u32 id);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_element_by_id 通过ID获取控件元素
 *
 * @param id 控件ID
 *
 * @return 控件元素
 */
/* ------------------------------------------------------------------------------------*/
struct element *ui_core_get_element_by_id(u32 id);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_disp_status_by_id 获取指定ID的控件显示状态
 *
 * @param id 控件ID
 *
 * @return 0 隐藏，1 显示，其他异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_get_disp_status_by_id(u32 id);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_disp_status_by_id 设置指定ID的控件显示状态
 *
 * @param id 控件ID
 *
 * @param visiable 0 隐藏，1 显示，其他异常
 *
 * @return 0 成功，其他 异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_set_disp_status_by_id(u32 id, int visiable);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_up_element 获取指定元素上方的元素
 *
 * @param elm 指定元素
 *
 * @return 指定元素上方的元素
 */
/* ------------------------------------------------------------------------------------*/
struct element *ui_core_get_up_element(struct element *elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_down_element 获取指定元素下方的元素
 *
 * @param elm 指定元素
 *
 * @return 指定元素下方的元素
 */
/* ------------------------------------------------------------------------------------*/
struct element *ui_core_get_down_element(struct element *elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_left_element 获取指定元素左边的元素
 *
 * @param elm 指定元素
 *
 * @return 指定元素左边的元素
 */
/* ------------------------------------------------------------------------------------*/
struct element *ui_core_get_left_element(struct element *elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_right_element 获取指定元素右边的元素
 *
 * @param elm 指定元素
 *
 * @return 指定元素右边的元素
 */
/* ------------------------------------------------------------------------------------*/
struct element *ui_core_get_right_element(struct element *elm);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_put_element
 *
 * @param elm
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_put_element(struct element *elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_element
 *
 * @param elm
 *
 * @return 其他正常，-22异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_get_element(struct element *elm);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_element_ontouch 触发元素触摸回调
 *
 * @param elm 控件元素
 * @param e 触摸事件
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_element_ontouch(struct element *, struct element_touch_event *e);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_ontouch 触摸事件分发处理
 *
 * @param e 触摸事件
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_ontouch(struct element_touch_event *e);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_element_onkey 元素按键事件响应
 *
 * @param elm 控件元素
 * @param e 按键事件
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_element_onkey(struct element *elm, struct element_key_event *e);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_onkey 按键事件分发处理
 *
 * @param e 按键事件
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_onkey(struct element_key_event *e);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_element_append_child 添加子元素，按照z_order从小到大排序，也就是从底层往上层排列，重绘时从前往后遍历，onkey和ontouch是从后往前遍历
 *
 * @param parent 父控件元素
 * @param child 子控件元素
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_element_append_child(struct element *parent, struct element *child);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_element_css 设置控件元素的CSS属性
 *
 * @param _elm 控件元素
 * @param css 待设置的CSS属性
 *
 * @return 设置后的CSS属性
 */
/* ------------------------------------------------------------------------------------*/
struct element_css *ui_core_set_element_css(void *_elm, const struct element_css1 *css);

int ui_core_invert_rect(struct draw_context *dc);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_release_child_probe
 *
 * @param elm
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_release_child_probe(struct element *elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_release_child 释放子控件
 *
 * @param elm 待释放的子控件元素
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_release_child(struct element *elm);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_redraw 重绘控件元素
 *
 * @param _elm 待重绘的控件元素
 *
 * @return 0 正常，其他异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_redraw(void *_elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_highlight_element 设置控件元素的高亮状态
 *
 * @param elm 待高亮的控件元素
 * @param yes 是否设置为为高亮状态
 *
 * @return 0 正常，其他异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_highlight_element(struct element *elm, int yes);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_focus 获取当前焦点
 *
 * @return 焦点存在返回焦点，焦点不存在返回NULL
 */
/* ------------------------------------------------------------------------------------*/
struct element *ui_core_get_focus();
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_element_on_focus 控件元素焦点设置
 *
 * @param elm 控件元素
 * @param yes 是否设置为焦点
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_element_on_focus(struct element *elm, int yes);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_element_ontouch_focus 控件元素触摸事件聚焦
 *
 * @param level 等级（一般为 int level = 0; &level 进行设置）
 * @param elm 控件元素
 * @param e 触摸事件
 *
 * @return true正常，false异常
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_element_ontouch_focus(int level, struct element *elm, struct element_touch_event *e);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_ontouch_lose_focus 控件元素失去焦点
 *
 * @param elm 控件元素
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_ontouch_lose_focus(struct element *elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_ontouch_lock 控件触摸事件锁定(如果设置了锁定，触摸事件会发送到该控件)
 *
 * @param elm 控件元素
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_ontouch_lock(struct element *elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_ontouch_unlock 控件触摸事件解锁
 *
 * @param elm 控件元素
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_ontouch_unlock(struct element *elm);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_draw_context 获取绘制上下文句柄
 *
 * @param dc 显示上下文
 * @param elm 控件元素
 * @param draw 绘制区域
 *
 * @return 0
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_get_draw_context(struct draw_context *dc, struct element *elm, struct rect *draw);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_get_screen_draw_rect 获取屏幕绘制区域
 *
 * @return rect 结构体，表示屏幕绘制区域
 */
/* ------------------------------------------------------------------------------------*/
struct rect *ui_core_get_screen_draw_rect();

void ui_core_set_group(struct element *elm, int group, int chile_redo);


void *ui_core_load_css(u8 page, void *_css);

void *ui_core_load_imagelist(u8 page, void *_list);

void *ui_core_load_textlist(u8 page, void *__list);

void *ui_core_load_widget_info(void *__head, u8 page);


int ui_core_get_dc(struct element *elm);

int ui_core_set_element_hide_action(void *_elm, int redraw);

int ui_core_set_element_background_image(void *_elm, int background_image);
int ui_core_set_element_border(void *_elm, struct css_border *border);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_element_rotate_en 设置控件elm旋转使能
 *
 * @Params _elm	控件elm
 * @Params enable 是否使能(true 使能，false 不使能)
 *
 * @return false 未使能/控件不存在，ture 已使能
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_set_element_rotate_en(void *_elm, u8 enable);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_element_rotate 设置控件elm旋转参数
 *
 * @Params _elm 控件elm
 * @Params cx 图片旋转中心X
 * @Params cy 图片旋转中心Y
 * @Params dx 屏幕旋转中心X，相对于父控件
 * @Params dy 屏幕旋转中心Y，相对于父控件
 * @Params angle 旋转角度
 * @Params en 是否使能
 *
 * @return true 设置成功，false 未使能/控件不存在，未设置
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_set_element_rotate(void *_elm, s16 cx, s16 cy, s16 dx, s16 dy, float angle, u8 en);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_element_ratio_en 设置控件缩放使能
 *
 * @Params _elm 控件
 * @Params enable 是否使能(true 使能，false 不使能)
 *
 * @return true 使能，false 未使能/控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_set_element_ratio_en(void *_elm, u8 enable);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_element_ratio 设置控件缩放参数
 *
 * @Params _elm 控件
 * @Params ratio_w 水平缩放倍率
 * @Params ratio_h 垂直缩放倍率
 * @Params en 是否使能
 *
 * @return true 设置完成，false 未使能/控件不存在，未设置
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_set_element_ratio(void *_elm, float ratio_w, float ratio_h, u8 en);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_element_ratio_change_rect 缩放的同时按缩放系数修改控件rect
 *
 * @param _elm			elm 控件
 * @param yes		是否修改
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_core_set_element_ratio_change_rect(void *_elm, int yes);
int ui_core_release_element_css(void *_elm);

int ui_core_get_rotate_abs_rect(struct element *elm, struct rect *rect);
int ui_core_element_hide(struct element *elm);
int ui_core_element_show(struct element *elm);
int ui_core_put_element(struct element *elm);
int ui_get_current_window_id();
int ui_core_element_delete(struct element *elm);

u32 *get_ui_mem_id();
u32  get_ui_mem_id_size();
void set_ui_open_flag(u8 flag);
u8 get_ui_open_flag();
u8 get_ui_init_status();
void ui_backlight_close(void);
void ui_backlight_open(u8 recover_cur_page);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_core_set_effect_redraw_time 设置特效刷新间隔（在跟手刷屏基础上多久插入一次）
 *
 * @Params ms_time 以ms为单位
 *
 * @return 无
 */
/* ------------------------------------------------------------------------------------*/
void ui_core_set_effect_redraw_time(int ms_time);


// ~~~~~~~~~~~ 以下为lua相关接口
// extern int run_lua_string(int len, const char *string);

// void *ui_core_load_lua(u8 page, void *_lua);

// void ui_core_free_lua(void *_lua);



#endif



