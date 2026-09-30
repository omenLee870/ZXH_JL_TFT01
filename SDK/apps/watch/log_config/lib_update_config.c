#include "app_config.h"
#include "system/includes.h"
#include "update_loader_download.h"
#include "debug.h"

/* #ifdef CONFIG_256K_FLASH */
/* const int config_update_mode = UPDATE_UART_EN; */
/* #else */
/* const int config_update_mode = UPDATE_BT_LMP_EN \ */
/*                                | UPDATE_STORAGE_DEV_EN | UPDATE_BLE_TEST_EN | UPDATE_APP_EN | UPDATE_UART_EN; */
/* #endif */

//是否采用双备份升级方案:0-单备份;1-双备份
#if CONFIG_DOUBLE_BANK_ENABLE
const int support_dual_bank_update_en = 1;
#else
const int support_dual_bank_update_en = 0;
#endif  //CONFIG_DOUBLE_BANK_ENABLE

//是否双备份升级方案，但appcore1的区域相比appcore0更小
#if CONFIG_DOUBLE_BANK_LESS
const int support_dual_bank_less_en = 1;
#else
const int support_dual_bank_less_en = 0;
#endif  //CONFIG_DOUBLE_BANK_ENABLE

//是否双备份升级方案，使用另外的小ota包,一般适用于tws升级
#if CONFIG_DB_UPDATE_SMALL
const int support_db_update_small_package = 1;
#else
const int support_db_update_small_package = 0;
#endif  //CONFIG_DOUBLE_BANK_ENABLE

// 是否支持双备份升级前和升级失败对升级区域全擦，升级过程只写的功能
// 即允许边升级边播歌
#ifdef TCFG_DUAL_BANK_UPDATE_NO_ERASE
const int support_dual_bank_update_no_erase = TCFG_DUAL_BANK_UPDATE_NO_ERASE;
#else
const int support_dual_bank_update_no_erase = 0;
#endif

const int support_jlota_ext_param_len = 256;


// 是否支持双备份断点续传升级
const int support_dual_bank_update_breakpoint = 0;

// 是否支持tws双备份升级，从机收错包重发机制
const int support_dual_bank_slave_recv_again = 1;

// 双备份主动升级模式下不升级资源区(设置为1则不升级资源区)
const int support_dual_bank_not_update_reserve = 0;

// 双备份tws升级主从同步校验使能
const int support_dual_bank_sync_verify_en = 1;

#if OTA_TWS_SAME_TIME_NEW       //使用新的同步升级流程
const int support_ota_tws_same_time_new =  1;
#else
const int support_ota_tws_same_time_new =  0;
#endif
//是否支持升级之后保留vm数据
const int support_vm_data_keep = 1;

//是否支持外挂flash升级,需要打开Board.h中的TCFG_NOR_FS_ENABLE
#if defined(TCFG_NORFLASH_UPDATE_EN) && (TCFG_NORFLASH_UPDATE_EN)
const int support_norflash_update_en  = 1;
#else
const int support_norflash_update_en  = 0;
#endif

//支持从外挂flash读取ufw文件升级使能
#if defined(TCFG_COMPELTE_UFW_NOR_EN) && (TCFG_COMPELTE_UFW_NOR_EN)
const int support_norflash_ufw_update_en = 1;
#else
const int support_norflash_ufw_update_en = 0;
#endif

//支持从内置flash读取ufw文件升级使能
#if defined(TCFG_COMPELTE_UFW_LC_EN) && (TCFG_COMPELTE_UFW_LC_EN)
const int support_lc_flash_ufw_update_en = 1;
#else
const int support_lc_flash_ufw_update_en = 0;
#endif

//支持从nand_flash读取ufw文件升级使能
#if defined(TCFG_COMPELTE_UFW_NAND_EN) && (TCFG_COMPELTE_UFW_NAND_EN)
const int support_nandflash_ufw_update_en = 1;
#else
const int support_nandflash_ufw_update_en = 0;
#endif

// 单备份升级时是否支持检查升级文件是否一致(BIT(0))
const int support_config_update_features = 0;

// 双备份压缩升级速度优化
const int support_speed_optimization_en = 0;//BIT(UPDATE_SPEED_WRITE_REQ_ASYNC) | BIT(UPDATE_SPEED_CACHE_EN);


//支持计算推loader数据时的进度
const int support_update_loader_precent = 0;

//支持对btif区域中的蓝牙地址升级前备份，升级后还原的操作
const int support_update_backup_btif_addr = 0;

//支持根据外挂flash完整的ota.bin文件，直接走推送loader的流程
const int support_norflash_update_loader_only = 0;

//支持升级放本地形式的压缩升级
const int support_ufw_com_update_en = 0;

//支持外挂flash和预留区域一起升级，关闭只升级外挂flash
#if (defined(TCFG_UPDATE_RESOURCE_EN) && (TCFG_UPDATE_RESOURCE_EN)) && ((defined(TCFG_VIRFAT_INSERT_FLASH_ENABLE) && (TCFG_VIRFAT_INSERT_FLASH_ENABLE)) || (defined(TCFG_NANDFLASH_DEV_ENABLE) && (TCFG_NANDFLASH_DEV_ENABLE)) || (defined(TCFG_SDFILE_INSERT_FLASH_ENABLE) && (TCFG_SDFILE_INSERT_FLASH_ENABLE)))
const int support_user_file_update_v2_en = 1;
#else
const int support_user_file_update_v2_en = 0;
#endif

// 针对单备份升级的特殊操作，默认不开启功能
u32 config_update_features = 0;
/* 如果没有包含资源情况下，3个标志位都开启，则执行优先级是 BIT(0) > BIT(1) > BIT(2) */
/* BIT(CONFIG_UPDATE_FEATRUES_CONTENT_COMPARE_EN) - 升级文件不包含资源情况下，sdk开启判断远端程序是否一致功能 */
/* BIT(CONFIG_UPDATE_FEATRUES_PUSH_LOADER_ONLY_EN) - 只推送loader */
/* BIT(CONFIG_UPDATE_FEATRUES_UPDATE_RES_ONLY_EN) - 只升级资源 */

// 支持使用预留区域作为复用区域的特殊升级操作，默认不开启功能
const int support_reusable_special_update = 0;

//               verb   debug   info  warn  error char
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, UPDATE)
