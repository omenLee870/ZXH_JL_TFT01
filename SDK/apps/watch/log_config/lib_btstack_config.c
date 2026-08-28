#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_btstack_config.data.bss")
#pragma data_seg(".lib_btstack_config.data")
#pragma const_seg(".lib_btstack_config.text.const")
#pragma code_seg(".lib_btstack_config.text")
#endif
/*********************************************************************************************
    *   Filename        : btstack_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-16 11:49

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "app_config.h"
#include "debug.h"
#include "btcontroller_config.h"
#include "bt_common.h"

/**
 * @brief Bluetooth Stack Module
 */

// *INDENT-OFF*
#ifdef CONFIG_SOUNDBOX_FLASH_256K
	const int CONFIG_BTSTACK_BIG_FLASH_ENABLE     = 0;
#else
	const int CONFIG_BTSTACK_BIG_FLASH_ENABLE     = 1;
#endif


#if TCFG_BT_SUPPORT_AAC
	const int CONFIG_BTSTACK_SUPPORT_AAC    = 1;
#else
	const int CONFIG_BTSTACK_SUPPORT_AAC    = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LDAC)
	const int CONFIG_BTSTACK_SUPPORT_LDAC       = TCFG_BT_SUPPORT_LDAC;
#else
	const int CONFIG_BTSTACK_SUPPORT_LDAC       = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LHDC)
	const int CONFIG_BTSTACK_SUPPORT_LHDC       = TCFG_BT_SUPPORT_LHDC;
#else
	const int CONFIG_BTSTACK_SUPPORT_LHDC       = 0;
#endif

#if defined(TCFG_BT_SUPPORT_APTX)
	const int CONFIG_BTSTACK_SUPPORT_APTX       = TCFG_BT_SUPPORT_APTX;
#else
	const int CONFIG_BTSTACK_SUPPORT_APTX       = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LHDC_V5)
	const int CONFIG_BTSTACK_SUPPORT_LHDC_V5    = TCFG_BT_SUPPORT_LHDC_V5;
#else
	const int CONFIG_BTSTACK_SUPPORT_LHDC_V5    = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LC3)
	const int CONFIG_BTSTACK_SUPPORT_LC3        = TCFG_BT_SUPPORT_LC3;
#else
	const int CONFIG_BTSTACK_SUPPORT_LC3        = 0;
#endif
#if CONFIG_USE_RANDOM_ADDRESS_ENABLE
	const int CONFIG_RANDOM_ADDRESS_ENABLE 		=1;
#else
	const int  CONFIG_RANDOM_ADDRESS_ENABLE		=0;
#endif

//协议栈接收到命令是否自动退出sniff
const int config_btstask_auto_exit_sniff = 1;
/* #if TCFG_TWS_AUDIO_SHARE_ENABLE */
/* const int CONFIG_BTSTACK_TWS_AUDIO_SHARE_ENABLE  = 1; */
/* #else */
const int CONFIG_BTSTACK_TWS_AUDIO_SHARE_ENABLE  = 0;
/* #endif */

#if (BT_AI_SEL_PROTOCOL & LE_AUDIO_CIS_RX_EN)
	const int CONFIG_BTSTACK_LE_AUDIO_ENABLE     = 1;
#else
	const int CONFIG_BTSTACK_LE_AUDIO_ENABLE     = 0;
#endif

#if (BT_AI_SEL_PROTOCOL & LE_AUDIO_CIS_RX_EN)
	const int config_le_sm_sub_sc_bridge_edr_enable = 1;
	const int config_le_sm_sub_sc_enable = 1;
#else
   #if TCFG_BLE_BRIDGE_EDR_ENALBE
   		const int config_le_sm_sub_sc_enable = 1;            /*SC加密模式使能,need config_le_sm_support_enable = 1*/
   		const int config_le_sm_sub_sc_bridge_edr_enable = 1; /*SC加密模式下,ios app ble配对快连edr,need config_le_sm_sub_sc_enable = 1*/
   #else
   		const int config_le_sm_sub_sc_enable = 0;
   		const int config_le_sm_sub_sc_bridge_edr_enable = 0;
   #endif
#endif

u8 config_btstack_watch_policy = 1; //手表项目，用于控制btstack一些手表的特殊策略


#if RCSP_MODE
int app_info_debug_enable = 0; // | BIT(4); 				//	rcsp库内部打印
/* #define RCSP_API_LOG		0 */							// rcsp认证相关
/* #define RCSP_PACKET_LOG		1 */
/* #define RCSP_PROTOCOL_LOG	2 */						// rcsp发送接收数据相关
/* #define SPP_USER_LOG		3 */
/* #define RCSP_INTERFACE_LOG	4 */						// rcsp连接相关
/* 鸿蒙系统会出现ble断开晚于spp连接的情况 */
u8 rcsp_allow_ble_spp_connect_simultaneously = 1;			// 1t1时，是否允许ble连接的同时连接spp
const int config_rcsp_stack_enable = 1;
#else
const int config_rcsp_stack_enable = 0;
#endif

//le 配置,可以优化代码和RAM
#if WECHAT_SPORT_ENABLE
const int config_le_hci_connection_num = MAX_HW_NUM +1;//支持同时连接个数
#else
const int config_le_hci_connection_num = MAX_HW_NUM;//支持同时连接个数
#endif
const int config_le_sm_support_enable = 1; //是否支持加密配对

const int config_le_gatt_server_num = 2;   //支持server角色个数
#if (defined(RCSP_BLE_CLIENT_EN)) && RCSP_BLE_CLIENT_EN
const int config_le_gatt_client_num = 2;   //支持client角色个数
#else
const int config_le_gatt_client_num = 1;   //支持client角色个数
#endif

// *INDENT-ON*
//               verb   debug   info  warn  error char
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, APP_BLE)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, APP_SPP)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, SPP)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, AVCTP)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, SCREEN_TRANS)
