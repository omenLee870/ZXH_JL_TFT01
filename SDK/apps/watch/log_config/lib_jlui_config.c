#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_jlui_config.data.bss")
#pragma data_seg(".lib_jlui_config.data")
#pragma const_seg(".lib_jlui_config.text.const")
#pragma code_seg(".lib_jlui_config.text")
#endif


#include "app_config.h"
#include "debug.h"
#include "system/includes.h"
#if CONFIG_JL_UI_ENABLE
#include "jlui/ui_core.h"
#endif

#if TCFG_UI_ENABLE
#include "gpu.h"
#endif

//================================================//
//              UI版本和CPU类型  				  //
//	使用一个8bit值表示							  //
// 	高四位表示UI类型，0表示点阵屏，1表示彩屏	  //
// 	低四位表示CPU，0表示BR23和BR27，1表示BR28	  //
//================================================//
#if ((defined CONFIG_CPU_BR23) || (defined CONFIG_CPU_BR27))
#if (defined TCFG_DOT_LCD_ENABLE)
const int JLUI_TYPE_AND_VERSION = 0x00;
#else
const int JLUI_TYPE_AND_VERSION = 0x10;
#endif

#else
#if (defined TCFG_DOT_LCD_ENABLE)
const int JLUI_TYPE_AND_VERSION = 0x01;

#else
const int JLUI_TYPE_AND_VERSION = 0x11;
#endif
#endif


//================================================//
//                LVGL 						  //
//================================================//
#if CONFIG_LVGL_UI_ENABLE
const u8 lvgl_ui_enable = 1;
const u8 TCFG_DEBUG_RENDER_LCD_TIME = 0; //统计刷屏帧率,以及UI框架渲染一帧时间,以及显示屏驱动造成的延时时间

//启动推屏时te可以越过推屏区域起始y坐标的最大值;如果页面合成速度小于1TE周期则该值较大更好，
//如果合成速度较慢则该值较小更好;如果出现切线尽量减小改值
const u16 lvgl_te_contenue = 100;

const char *const lv_src_base_file = "storage/res_nor_mode/C/lvgl/lv_res.zip";

#else
const u8 lvgl_ui_enable = 0;
#endif


//================================================//
//                JLUI内核 						  //
//================================================//
const int UI_DATA_STORE_IN_NORFLASH = !TCFG_NANDFLASH_DEV_ENABLE;	//norflash，默认开启
const int ENABLE_PSRAM_UI_FRAME = 0;				//psram特效，暂不使用
const int JLUI_CORE_GRID_KEY_VER = 0x1;				//列表编码器转动版本
const int UI_NANDFLASH_RES_BY_PACKRES = TCFG_NANDFLASH_UI_FAT_ENABLE;	//使用标准fat+packres打包资源
const int UI_CORE_FOCUS_FILTER_ENABLE  = 1;
#if CONFIG_DOUBLE_BANK_ENABLE
const int UI_RES_FLASH_TAB_OFFSET = 0x20;			//双备份内置资源偏移
#else
const int UI_RES_FLASH_TAB_OFFSET = 0x0;
#endif
const int touch_move_fliter_distance = 3;			//xy滑动矢量和小于该值，过滤move事件
//================================================//
//                JPEG  						  //
//================================================//
const int config_jpeg_isr_delay_us = 50;			//参考值100,最小50
const int JLJPEG_STREAM_ENABLE  = 1;
#if TCFG_HOST_UVC_ENABLE||TCFG_UI_AVRCP_MUSIC_BG_ENABLE
const int config_jpeg_sync_wait_in_irq = 0;			//在中断同步等待，效率低
const int config_jpeg_ff_rst_enable = 1;			//支持带DRI标志的图片，部分摄像头只输出这种类型的图片
#else
const int config_jpeg_sync_wait_in_irq = 0;			//在中断同步等待
const int config_jpeg_ff_rst_enable = 0;			//支持带DRI标志的图片，非UVC功能不开，由app等转码。带DRI标志解码效率低
#endif
const int config_jpeg_pend_timeout = 20;			//

//================================================//
//                GPU中断 						  //
//================================================//
#if (TCFG_UI_ENABLE && (CONFIG_JL_UI_ENABLE || CONFIG_LVGL_UI_ENABLE))
const int UI_GPU_IRQ_MODE  	  = 1;
#else
const int UI_GPU_IRQ_MODE  	  = 0;
#endif


//================================================//
//                GPU异常打印 					  //
//================================================//
const int GPU_EXCEPTION_ANALYZE_ENABLE	= 	0;

const unsigned int GPU_DEBUG_ENABLE = 0;//GPU_DB_TASK | GPU_DB_FILL | GPU_DB_MASK | GPU_DB_GRAD | GPU_DB_ARG | GPU_DB_FUNC | GPU_DB_RECT; /*! GPU调试打印使能 */

//================================================//
//              GPU 纹理地址配置                  //
//  0: 忽略地址段指定，应用层传入什么，GPU用什么  //
//  1: 强制使用cache地址，底层强制转换            //
//  2: 强制使用eva cache地址，底层强制转换        //
//  3: 强制使用no cache地址，底层强制转换         //
//================================================//
const unsigned int GPU_FLASH_ADDR_RANGE = 0; /*! FLASH地址段定义 */
const unsigned int GPU_PSRAM_ADDR_RANGE = 0; /*! PSRAM地址段定义 */


#define GPU_PATH_MIN_SIZE	128
#define GPU_PATH_ADD_SIZE	64
const unsigned int GPU_PATH_MEMORY = (GPU_PATH_MIN_SIZE << 16) | GPU_PATH_ADD_SIZE;


//================================================//
//                字库 							  //
//================================================//
const int ARABIC_MODE_SWITCH      = 0;
const int HEBREW_MODE_SWITCH      = 0;
const int THAI_MODE_SWITCH        = 0;
const int MYANMAR_MODE_SWITCH     = 0;
const int BENGALI_MODE_SWITCH     = 0;
const int KHMER_MODE_SWITCH       = 0;
const int INDIC_MODE_SWITCH       = 0;
const int TIBETAN_MODE_SWITCH     = 0;
const int MIXLEFT_MODE_SWITCH     = 1;
#if TCFG_IFLYTEK_ENABLE
const int MIXRIGHT_MODE_SWITCH    = 1;
#else
const int MIXRIGHT_MODE_SWITCH    = 1;
#endif
const int FONT_UNIC_SWITCH        = 1;
const int FONT_FREETYPE_SWITCH    = 0;
const int FONT_USE_PTR = !TCFG_NANDFLASH_DEV_ENABLE;
const int SCALE_EFFECT_WITHOUT_PSRAM_ENABLE = 0;

const int font_scroll_circular_interval = 40;			//循环滚动距离
const int strpic_scroll_circular_deafult_enable = 0;		//是否默认开启strpic循环滚动
//================================================//
//           优先使用psram的模块配置		  	  //
//================================================//
#if TCFG_PSRAM_DEV_ENABLE && TCFG_UI_ENABLE && CONFIG_JL_UI_ENABLE
const u32 config_ui_alloc_psram_mod_sel	=	\
    /* BIT(UI_MODULE_CORE) | */ \
    BIT(UI_MODULE_FRAME) | \
    BIT(UI_MODULE_WIDGET) | \
    BIT(UI_MODULE_RESOURCE) | \
    BIT(UI_MODULE_GPU) | \
    BIT(UI_MODULE_FONT) | \
    BIT(UI_MODULE_JPEG) | \
    BIT(UI_MODULE_CACHE) | \
    BIT(UI_MODULE_CUSTOM_DRAW);
#else
const u32 config_ui_alloc_psram_mod_sel	=	0;
#endif


//================================================//
//        杰理UI使用PSRAM作为显存buf         	  //
// 注意：这个标志使能时GPU直接输出到PSRAM，速度会 //
// 比较慢一点，谨慎使用。                         //
// 空间占用：LCD屏驱配置的显存buf                 //
// 使用本功能必须使能PSRAM设备！                  //
//================================================//
#if TCFG_PSRAM_DEV_ENABLE && TCFG_UI_ENABLE
const int JLUI_USED_PSRAM_DISPLAY_BUF = 0;
#else
const int JLUI_USED_PSRAM_DISPLAY_BUF = 0;
#endif


//================================================//
//        杰理UI GPU输出拷贝到PSRAM         	  //
// 注意：这个标志使能时，GPU依旧使用SRAM作为分块合//
// 成的输出buf，每个分块合成完毕后会使用DMA拷贝到 //
// PSRAM并拼接成一帧数据，最后使用DBI模块推这一整 //
// 帧数据到屏幕。                                 //
// 空间占用：屏宽 * 屏高 * 2(RGB565) * 2(双buf)   //
// 本功能与 JLUI_USED_PSRAM_DISPLAY_BUF 不互斥,但 //
// 不建议同时使用，否则对帧率影响较大。           //
// 使用本功能必须使能PSRAM设备！                  //
// 使用nandflash时占用较多PSRAM，不建议开启本功能 //
//================================================//
const int JLUI_GPU_DMA_TO_PSRAM = 0;


//================================================//
//        杰理UI QSPI/RGB RAMLESS屏使能          	  //
// 注意：JLUI_GPU_DMA_TO_PSRAM需要开启              //
//================================================//
const int JLUI_LCD_RAMLESS_ENABLE = 0;


//================================================//
//             杰理UI多页面叠加支持               //
//================================================//
#if TCFG_UI_ENABLE_SMARTWIN
const int JLUI_MULTI_PAGE_OVERLAY_SUPPORT = 1;
#else
const int JLUI_MULTI_PAGE_OVERLAY_SUPPORT = 0;
#endif

//================================================//
//         杰理UI框架自定义绘图内容缓存           //
// 注意：当开启自定义绘图缓存功能时，自定义绘图内 //
// 容将使用 jlui_malloc 接口申请 buf 进行缓存。会 //
// 占用更多的ram或psram空间，但开启此功能可将自定 //
// 义绘图内容添加到页面中用于2.5D特效。           //
//                                                //
// 注意：目前仅支持ui_draw_bar功能缓存 2025-4-24  //
//================================================//
const int JLUI_CUSTOM_DRAW_CACHE = 0;


//================================================//
//        GPU 输入PSRAM地址转为nocache        	  //
// 注意：开启本功能后，底层在配置GPU指令时，会自  //
// 动将PSRAM 的cache地址转为 nocache 地址进行访问 //
//================================================//
const int GPU_INPUT_TO_NOCACHE = 0;


//================================================//
//	将资源加载到psram
//================================================//
#if (TCFG_PSRAM_DEV_ENABLE && TCFG_UI_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
const int config_gpu_cache_psram_jpeg_en = 1;
const int config_gpu_cache_psram_file_res_en = 1;
#elif (TCFG_PSRAM_DEV_ENABLE && TCFG_UI_ENABLE && !TCFG_NANDFLASH_DEV_ENABLE)
const int config_gpu_cache_psram_jpeg_en = 1;
const int config_gpu_cache_psram_file_res_en = 0;
#else
const int config_gpu_cache_psram_jpeg_en = 0;
const int config_gpu_cache_psram_file_res_en = 0;
#endif

//================================================//
//                GPU任务释放异常检查 			  //
//================================================//
const int GPU_TASK_FREE_DEBUG_EN = 0;


//================================================//
//          不可屏蔽中断使能配置(UNMASK_IRQ)      //
//================================================//
const int CONFIG_CPU_UNMASK_IRQ_ENABLE = 1;


//================================================//
//          		特效					      //
//================================================//
const int config_page_flip_deep = 500;				//500~1000,值越小立体感越强

//               verb   debug   info  warn  error char
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, UI_CORE)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, WINDOW)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, LAYER)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, LAYOUT)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, BATTERY)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, PIC)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, TEXT)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, TIME)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, NUMBER)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, BUTTON)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, GRID)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, COMPASS)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, MULTIPROGRESS)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, PROGRESS)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, SLIDER)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, VSLIDER)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, WATCH)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, FONT)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, PLATFORM)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, UI_DRAW)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, UI_RES)
REGISTER_LOG_TAG(TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, GPU_DEMO)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, JL_GPU)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  TRUE,  UI_EFFECT)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  TRUE,  UI_BUF)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  TRUE,  AVI_VIDEO)
//               verb   debug   info  warn  error char
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, MODEL_MGR)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, UI_COMMON)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, CACHE_POOL)
