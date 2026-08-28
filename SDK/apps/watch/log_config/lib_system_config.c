
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_system_config.data.bss")
#pragma data_seg(".lib_system_config.data")
#pragma const_seg(".lib_system_config.text.const")
#pragma code_seg(".lib_system_config.text")
#endif
/*********************************************************************************************
    *   Filename        : lib_system_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-18 15:22

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/

#include "app_config.h"
#include "debug.h"
#include "system/includes.h"

///打印是否时间打印信息
const int config_printf_time         = 1;

///异常中断，asser打印开启
#if CONFIG_DEBUG_ENABLE
const int config_asser         = TCFG_EXCEPTION_LOG_ENABLE;  // 1:使能异常打印; 2:追加额外调试信息(dump另外一个CPU寄存器信息)
const int config_exception_reset_enable = TCFG_EXCEPTION_RESET_ENABLE;//异常打印完毕能否复位
const int CONFIG_LOG_OUTPUT_ENABLE = 1;
const int config_ulog_enable = 0;
const int config_dlog_enable = 0;
#else
const int config_asser         = 0;
const int config_exception_reset_enable = 1;
const int CONFIG_LOG_OUTPUT_ENABLE = 0;
const int config_ulog_enable = 0;
const int config_dlog_enable = 0;
#endif

//操作系统mutex在关中断操作的debug,
//量产版本建议关闭检查
const int config_mutex_irq_asser         = config_asser;




//================================================//
//                 异常信息记录使能               //
//================================================//
//注意: 当config_asser变量为0时才有效.
#if	(defined TCFG_CONFIG_DEBUG_RECORD_ENABLE && TCFG_CONFIG_DEBUG_RECORD_ENABLE)
const int config_debug_exception_record = (!config_asser) && 1; 	//异常记录功能总开关
const int config_debug_exception_record_dump_info = (!config_asser) && 1; 		//小机上电输出异常信息使能
const int config_debug_exception_record_p11 = (!config_asser) && 1; 			//P11异常信息使能
const int config_debug_exception_record_stack = (!config_asser) && 1; 			//堆栈异常信息使能
const int config_debug_exception_record_ret_instrcion = (!config_asser) && 1; 	//指令数据异常信息使能
#else /* #if	(define CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE) */
const int config_debug_exception_record = 0; 				//异常记录功能总开关
const int config_debug_exception_record_dump_info = 0; 		//小机上电输出异常信息使能
const int config_debug_exception_record_p11 = 0; 			//P11异常信息使能
const int config_debug_exception_record_stack = 0; 			//堆栈异常信息使能
const int config_debug_exception_record_ret_instrcion = 0; 	//指令数据异常信息使能
#endif /* #if (define CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE) */

//================================================//
//当hsb的频率小于等于48MHz时，在否自动关闭所有pll //
//================================================//
const int clock_tree_auto_disable_all_pll = 0;

//================================================//
//                  SDFILE 精简使能               //
//================================================//
const int SDFILE_VFS_REDUCE_ENABLE = 1;


#if TCFG_UPDATE_ENABLE
const int CONFIG_UPDATE_ENABLE  = 0x1;
#else
const int CONFIG_UPDATE_ENABLE  = 0x0;
#endif


//================================================//
//                  dev使用异步读使能             //
//================================================//
#ifdef TCFG_DEVICE_BULK_READ_ASYNC_ENABLE
const int device_bulk_read_async_enable = 0;
#else
const int device_bulk_read_async_enable = 0;
#endif


//================================================//
//                  异常打印p11 ram              //
//正常是p11异常自动打印p11 ram的                   //
//如果碰见大小核调试问题，也可以主动开的来debug, //
//================================================//
const int config_exception_dump_p11_ram = 0;

//================================================//
//          heap内存记录功能                      //
// 使能建议设置为256（表示最大记录256项）         //
// 空间需求：CONFIG_HEAP_MEMORY_TRACE*12          //
//   CONFIG_HEAP_MEMORY_MAX_TRACE_EN使能时空间翻倍//
// 通过void mem_unfree_dump()输出实时记录         //
// 通过void mem_max_unfree_dump()输出最大记录     //
// void mem_max_unfree_recount()最大记录重新统计  //
//================================================//
const u32 CONFIG_HEAP_MEMORY_TRACE = 0;
const u32 CONFIG_HEAP_MEMORY_MAX_TRACE_EN = 0; // 0 or 1


//================================================//
//          malloc 越界debug和记录功能            //
//const_malloc_list_check_debug_en             = 1;      //开启配置内存越界检查，注意，本功能和heap内存记录功能互斥，更注重debug性质和主动检查性质，开启后会申请额外信息，本功能仅仅适用开发后用于内存安全性检查和内存越界的检查
//const_malloc_list_check_add_extlen           = 0;     //用来做写穿检查，如果内存写穿比较厉害导致rets打印异常，可以增加长度
//const int const_malloc_list_check_for_each   = 1;     //每次申请内存都做全局性检查，效率低，但是可以快速回溯出异常地址
//内存泄漏debug：通过 malloc_list_status_printf_all    //可以打印最后没有释放的内存,cout 是需要打印是数量,存在内存泄漏一般看最后几个申请是否有rets重复即可
//void malloc_list_status_printf_all(u32 index,  int count);
//index = 0 是sram、1是psram
//================================================//
const int const_malloc_list_check_debug_en   = 0;
const int const_malloc_list_check_add_extlen = 0;
const int const_malloc_list_check_for_each   = 0;


const int const_psram_malloc_list_check_debug_en     = 0;
const int const_psram_malloc_list_check_add_extlen   = 0;
const int const_psram_malloc_list_check_for_each   = 0;


//================================================//
//                  ftl 						  //
//================================================//
const int const_ftl_free_page_buf_enter_sleep = 0;		//低功耗释放buf
const int const_ftl_block_alloc_psram_en = 0;			//block管理使用psram内存,低功耗不释放(会增加psram stanby的功耗)
//================================================//
//                  FS功能控制 					  //
//================================================//
const int FATFS_WRITE = 1; // 控制fatfs写功能开关。
const int FILT_0SIZE_ENABLE = 1; //是否过滤0大小文件
const int FATFS_LONG_NAME_ENABLE = 1; //是否支持长文件名
const int FATFS_RENAME_ENABLE = 1; //是否支持重命名
const int FATFS_FGET_PATH_ENABLE = 1; //是否支持获取路径
const int FATFS_SAVE_FAT_TABLE_ENABLE = 1; //是否支持seek加速

const int FATFS_SUPPORT_OVERSECTOR_RW = 1; //是否支持超过一个sector向设备拿数
const int FATFS_TIMESORT_TURN_ENABLE = 1; //按时排序翻转，由默认从小到大变成从大到小
const int FATFS_TIMESORT_NUM = 128; //按时间排序,记录文件数量, 每个占用14 byte

const int FILE_TIME_HIDDEN_ENABLE = 0; //创建文件是否隐藏时间
const int FILE_TIME_USER_DEFINE_ENABLE = 0;//用户自定义时间，每次创建文件前设置，如果置0 需要确定芯片是否有RTC功能。

const int FATFS_SUPPORT_WRITE_SAVE_MEANTIME = 0; //每次写同步目录项使能，会降低连续写速度。

const int FATFS_SUPPORT_WRITE_CUTOFF = 1; //支持fseek截断文件。 打开后fseek后指针位置会决定文件大小
const int FATFS_RW_MAX_CACHE = 32 * 1024; //设置读写申请的最大cache大小 .note: 小于512会被默认不生效

const int FATFS_GET_SPACE_USE_RAM = 0;//32 * 1024;  //获取剩余容量使用大Buf缓存,加快速度, 必须512倍数

const int FATFS_FORMAT_USE_RAM = 0; //32 * 1024;  //格式化功能使用大Buf缓存,加快速度, 必须512倍数

const int FATFS_SUPPORT_DEV_READ_SUCCESSION = 0; //是否保存非对齐读时，一个扇区多余的数据。

const int SDFILE_STORAGE_ENABLE = 1; //sdfile做设备接入VFS,扫描功能使能

const int FATFS_BACKUP_ENABLE = 0; //文件系统安全备份功能使能

const int VIRFAT_FLASH_ENABLE = 0; //精简jifat代码,不使用。

const int FLASH_ALIGNED_MODE = 16; // 对齐, 只需填1或者16。 align * 256

//================================================//
//phy_malloc碎片整理使能:            			  //
//配置为0: phy_malloc申请不到不整理碎片           //
//配置为1: phy_malloc申请不到会整理碎片           //
//配置为2: phy_malloc申请总会启动一次碎片整理     //
//================================================//
const int PHYSIC_MALLOC_DEFRAG_ENABLE = 1;

//================================================//
//低功耗流程添加内存碎片整理使能:                 //
//配置为0: 低功耗流程不整理碎片                   //
//配置为1: 低功耗流程会整理碎片                   //
//================================================//
const int MALLOC_MEMORY_DEFRAG_ENABLE = 1;

//============================================================================//
//配置malloc时使用psram还是ram:        			                              //
//0x00 ~ 0xFFFFFFFF:当malloc内存大于CONFIG_MALLOC_PSRAM_LEVEL则分配psram的内存//
//注意:需要使能psram并且psram不经过mmu映射                                    //
//============================================================================//
const unsigned int CONFIG_MALLOC_PSRAM_LEVEL = (500 * 1024);
//============================================================================//
//datacode放cache        						                              //
//============================================================================//
#if ((defined TCFG_ICACHE_RUN_DATA_CODE)&& TCFG_ICACHE_RUN_DATA_CODE)
const u8 config_icache_run_data_code_enable = 1;
#else
const u8 config_icache_run_data_code_enable = 0;
#endif

//============================================================================//
//配置任务的栈内存总大小        			                                  //
//============================================================================//
const u32 total_stack_heap_size = TCFG_TOTAL_STACK_HEAP_SIZE;

/**
 * @brief Log (Verbose/Debug/Info/Warn/Error/Char)
 */
/*-----------------------------------------------------------*/
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, SYS_TMR)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, JLFS)
//FreeRTOS
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, PORT)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, KTASK)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, uECC)
REGISTER_LOG_TAG(FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, HEAP_MEM)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, V_MEM)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, P_MEM)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, P_MEM_C)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, MEM) //打印malloc地址
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, PSRAM_HEAP)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, DEBUG_RECORD)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FTL)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, SERVER_CORE)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, EVENT)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, JLDTP)
