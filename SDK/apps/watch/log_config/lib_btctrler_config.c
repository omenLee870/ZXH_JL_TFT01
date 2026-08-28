#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_btctrler_config.data.bss")
#pragma data_seg(".lib_btctrler_config.data")
#pragma const_seg(".lib_btctrler_config.text.const")
#pragma code_seg(".lib_btctrler_config.text")
#endif
/*********************************************************************************************
    *   Filename        : btctrler_config.c

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

// *INDENT-OFF*
/**
 * @brief Bluetooth Module
 */


const int CONFIG_BT_PMC_MSG_INFO  = 0;

#if TCFG_BT_DONGLE_ENABLE
	const int CONFIG_DONGLE_SPEAK_ENABLE  = 1;
#else
	const int CONFIG_DONGLE_SPEAK_ENABLE  = 0;
#endif
#if TCFG_BT_DUAL_CONN_ENABLE || TCFG_USER_EMITTER_ENABLE
const int CONFIG_LMP_CONNECTION_NUM = 2;
const int CONFIG_LMP_CONNECTION_LIMIT_NUM = 2;
#else
const int CONFIG_LMP_CONNECTION_NUM = 1;
const int CONFIG_LMP_CONNECTION_LIMIT_NUM = 1;
#endif

const int CONFIG_BTCTLER_JL_DONGLE_SOURCE_ENABLE=0;
const int CONFIG_DISTURB_SCAN_ENABLE=0;
#if TCFG_USER_EMITTER_ENABLE
const int config_master_qos_poll=1;
#else
const int config_master_qos_poll=0;
#endif

const int CONFIG_BT_DUAL_MODE_MANAGER_ENABLE =0;
#define TWS_PURE_MONITOR_MODE    0//1:纯监听模式

#if TWS_PURE_MONITOR_MODE
	u8 get_extws_nack_adjust(u8 per_v, int a2dp_dly_paly_time, int msec)
	{
		return 0;
	}
#endif
/* #if TCFG_TWS_AUDIO_SHARE_ENABLE */
	/* const int CONFIG_TWS_AUDIO_SHARE_ENABLE  = 1; */
/* #else */
	const int CONFIG_TWS_AUDIO_SHARE_ENABLE  = 0;
/* #endif */

const int CONFIG_TWS_FORWARD_TIMES = 1;
const int CONFIG_TWS_RUN_SLOT_MAX = 48;
const int CONFIG_TWS_RUN_SLOT_AT_A2DP_FORWARD = 8;
const int CONFIG_TWS_RUN_SLOT_AT_LOW_LATENCY = 8;
const int CONFIG_TWS_RUN_SLOT_AT_LOCAL_MEDIA_TRANS = 48;

#ifdef TCFG_LE_AUDIO_PLAY_LATENCY
const int CONFIG_LE_AUDIO_PLAY_LATENCY = TCFG_LE_AUDIO_PLAY_LATENCY; // le_audio延时（us）
#else
const int CONFIG_LE_AUDIO_PLAY_LATENCY = 0; // le_audio延时（us）
#endif

#ifdef TCFG_JL_DONGLE_PLAYBACK_LATENCY
const int CONFIG_JL_DONGLE_PLAYBACK_LATENCY = TCFG_JL_DONGLE_PLAYBACK_LATENCY; // dongle下行播放延时(msec)
#else
const int CONFIG_JL_DONGLE_PLAYBACK_LATENCY = 0; // dongle下行播放延时(msec)
#endif
#ifndef CONFIG_BREDR_STATIC_RAM_LEN
#error "no define CONFIG_BREDR_STATIC_RAM_LEN"
#else
#if CONFIG_BREDR_STATIC_RAM_LEN
const int CONFIG_BREDR_STATIC_RAM_EN = 1;
#else
const int CONFIG_BREDR_STATIC_RAM_EN = 0;
#endif
#endif
//固定使用正常发射功率的等级:0-使用不同模式的各自等级;1~10-固定发射功率等级
const int config_force_bt_pwr_tab_using_normal_level  = 0;
//配置BLE广播发射功率的等级:0-最大功率等级;1~10-固定发射功率等级
const int config_ble_adv_tx_pwr_level  = 0;

const int CONFIG_BLE_SYNC_WORD_BIT = 30;
const int CONFIG_LNA_CHECK_VAL = -80;

#if TCFG_USER_TWS_ENABLE
	#if (BT_FOR_APP_EN || TCFG_USER_BLE_ENABLE)
		const int config_btctler_modules        = BT_MODULE_CLASSIC | BT_MODULE_LE;
	#else
 		#ifdef CONFIG_NEW_BREDR_ENABLE
 	    	#if (BT_FOR_APP_EN)
 		    	const int config_btctler_modules        = (BT_MODULE_CLASSIC | BT_MODULE_LE);
 	        #else
 		        const int config_btctler_modules        = (BT_MODULE_CLASSIC);
 	        #endif
        #else
 	        const int config_btctler_modules        = (BT_MODULE_CLASSIC | BT_MODULE_LE);
        #endif
    #endif


	#ifdef CONFIG_NEW_BREDR_ENABLE
		const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;

	     #if TCFG_DEC2TWS_ENABLE
	     	const int CONFIG_TWS_WORK_MODE                  = 1;
	     #else
	     	const int CONFIG_TWS_WORK_MODE                  = 2;
	     #endif

		#ifdef CONFIG_SUPPORT_EX_TWS_ADJUST
			const int CONFIG_EX_TWS_ADJUST                  = 1;
		#else
			const int CONFIG_EX_TWS_ADJUST                  = 0;
		#endif


	#else//CONFIG_NEW_BREDR_ENABLE
		const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;
	#endif//end CONFIG_NEW_BREDR_ENABLE

	const int CONFIG_BTCTLER_TWS_ENABLE     = 1;

	#if TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE
		const int CONFIG_TWS_AUTO_ROLE_SWITCH_ENABLE = 1;
	#else
		const int CONFIG_TWS_AUTO_ROLE_SWITCH_ENABLE = 0;
	#endif

    const int CONFIG_TWS_POWER_BALANCE_ENABLE   = TCFG_TWS_POWER_BALANCE_ENABLE;
    const int CONFIG_LOW_LATENCY_ENABLE         = 1;
    const int CONFIG_TWS_DATA_TRANS_ENABLE = 0;
#else //TCFG_USER_TWS_ENABLE
	#if (TCFG_USER_BLE_ENABLE)
	#if (TCFG_USER_BT_CLASSIC_ENABLE)
		const int config_btctler_modules        = BT_MODULE_CLASSIC | BT_MODULE_LE;
	#else
		const int config_btctler_modules        = BT_MODULE_LE;
	#endif
	#else
		const int config_btctler_modules        = BT_MODULE_CLASSIC;
	#endif

	const int config_btctler_le_tws         = 0;
	const int CONFIG_BTCTLER_TWS_ENABLE     = 0;
	const int CONFIG_LOW_LATENCY_ENABLE     = 0;
	const int CONFIG_TWS_POWER_BALANCE_ENABLE   = 0;
	const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;
    const int CONFIG_TWS_DATA_TRANS_ENABLE = 0;
#endif//end TCFG_USER_TWS_ENABLE

/* #if (TCFG_BT_SUPPORT_LHDC_V5 || TCFG_BT_SUPPORT_LHDC || TCFG_BT_SUPPORT_LDAC) //LHDC/LDAC使用较高码率时需要增大蓝牙buf */
	/* const int CONFIG_A2DP_MAX_BUF_SIZE          = 30 * 1024; */
	/* const int CONFIG_EXTWS_NACK_LIMIT_INT_CNT       = 40; */
/* #else */
	const int CONFIG_A2DP_MAX_BUF_SIZE          = 15 * 1024;
	#if TWS_PURE_MONITOR_MODE
		const int CONFIG_EXTWS_NACK_LIMIT_INT_CNT       = 63;
	#else
		const int CONFIG_EXTWS_NACK_LIMIT_INT_CNT       = 8;
	#endif
/* #endif */

#if 0
// 可重写函数实时调试qos硬件开关状态，判断当前qos是开还是关
void user_qos_run_debug(u8 en)
{
	if(en){
		putchar('<');//qos open ing
	}else{
		putchar('>');//qos close ing
	}
}
#endif
#if 0
//可重写函数,a2dp播放时，根据当前延时，控制qos开关。延时太低关闭qos，延时高，开启qos省功耗
//delay_set_timer =CONFIG_A2DP_DELAY_TIME_AAC or CONFIG_A2DP_DELAY_TIME_SBC
//delay_lo_timer =CONFIG_A2DP_DATA_CACHE_LOW_AAC or CONFIG_A2DP_DATA_CACHE_LOW_AAC
//delay_hi_timer = CONFIG_A2DP_DATA_CACHE_HI_AAC or CONFIG_A2DP_DATA_CACHE_HI_SBC
u8 auto_check_a2dp_play_control_qos(u16 cur_delay_timer,u16 delay_set_timer,u16 delay_lo_timer,u16 delay_hi_timer)
{

	u8 ret=0;
	if ( cur_delay_timer <= delay_lo_timer || cur_delay_timer >= (delay_set_timer + 100)) {
		/* r_printf("QOS dis %d\n", msec); */
		ret = 1;
	} else if (cur_delay_timer >= delay_hi_timer) {
		ret = 2;
		/* r_printf("QOS en=%d\n",msec ); */
	}
	return ret;

}
#endif
const int CONFIG_TWS_SUPER_TIMEOUT          = 4000;
const int CONFIG_BTCTLER_QOS_ENABLE         = 1;
const int CONFIG_A2DP_DATA_CACHE_LOW_AAC    = 100;
const int CONFIG_A2DP_DATA_CACHE_HI_AAC     = 250;
const int CONFIG_A2DP_DATA_CACHE_LOW_SBC    = 150;
const int CONFIG_A2DP_DATA_CACHE_HI_SBC     = 260;
/* const int CONFIG_A2DP_DELAY_TIME_AAC = TCFG_A2DP_DELAY_TIME_AAC; */
/* const int CONFIG_A2DP_DELAY_TIME_SBC = TCFG_A2DP_DELAY_TIME_SBC; */
/* const int CONFIG_A2DP_DELAY_TIME_SBC_LO = TCFG_A2DP_DELAY_TIME_SBC_LO; */
/* const int CONFIG_A2DP_DELAY_TIME_AAC_LO = TCFG_A2DP_DELAY_TIME_AAC_LO; */
/* const int CONFIG_A2DP_ADAPTIVE_MAX_LATENCY = TCFG_A2DP_ADAPTIVE_MAX_LATENCY; */
const int CONFIG_JL_DONGLE_PLAYBACK_DYNAMIC_LATENCY_ENABLE  = 1;    //jl_dongle 动态延时

const int CONFIG_PAGE_POWER                 = 9;
const int CONFIG_PAGE_SCAN_POWER            = 9;
const int CONFIG_PAGE_SCAN_POWER_DUT        = 4;
const int CONFIG_INQUIRY_POWER              = 7;
const int CONFIG_INQUIRY_SCAN_POWER         = 7;
const int CONFIG_DUT_POWER                  = 10;

#if (CONFIG_BT_MODE != BT_NORMAL)
	const int config_btctler_hci_standard   = 1;
#else
	const int config_btctler_hci_standard   = 0;
#endif

const int config_btctler_mode        = CONFIG_BT_MODE;
const int CONFIG_BTCTLER_TWS_FUN     = TWS_ESCO_FORWARD ; // TWS_ESCO_FORWARD

/*-----------------------------------------------------------*/

/**
 * @brief Bluetooth Classic setting
 */
const u8 rx_fre_offset_adjust_enable = 1;

const int config_bredr_fcc_fix_fre = 0;
const int ble_disable_wait_enable = 1;

const int config_btctler_eir_version_info_len = 21;

#ifdef CONFIG_256K_FLASH
	const int CONFIG_TEST_DUT_CODE            = 1;
	const int CONFIG_TEST_FCC_CODE            = 0;
	const int CONFIG_TEST_DUT_ONLY_BOX_CODE   = 1;
#else
	const int CONFIG_TEST_DUT_CODE            = 1;
	const int CONFIG_TEST_FCC_CODE            = 1;
	const int CONFIG_TEST_DUT_ONLY_BOX_CODE   = 0;
#endif//end CONFIG_256K_FLASH

const int CONFIG_ESCO_MUX_RX_BULK_ENABLE  =  0;

#if TCFG_USER_EMITTER_ENABLE
const int CONFIG_BREDR_INQUIRY   =  1;
const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST =  1;
#else
const int CONFIG_BREDR_INQUIRY   =  0;
const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST =  0;
#endif
#if TCFG_RCSP_DUAL_CONN_ENABLE
	const int CONFIG_LMP_NAME_REQ_ENABLE  =  1;
#elif (BT_AI_SEL_PROTOCOL & REALME_EN)
	const int CONFIG_LMP_NAME_REQ_ENABLE  =  1;
#else
	const int CONFIG_LMP_NAME_REQ_ENABLE  =  0;
#endif
const int CONFIG_LMP_PASSKEY_ENABLE  =  0;
const int CONFIG_LMP_OOB_ENABLE  =  0;
const int CONFIG_LMP_MASTER_ESCO_ENABLE  =  0;
#ifdef CONFIG_SUPPORT_AES_CCM_FOR_EDR
    #if (BT_AI_SEL_PROTOCOL & LE_AUDIO_CIS_RX_EN)
        const int CONFIG_AES_CCM_FOR_EDR_ENABLE     = 0;
    #else
        const int CONFIG_AES_CCM_FOR_EDR_ENABLE     = 0;
    #endif
#else
    const int CONFIG_AES_CCM_FOR_EDR_ENABLE     = 0;
#endif

#ifdef CONFIG_SUPPORT_WIFI_DETECT
	#if TCFG_USER_TWS_ENABLE
		const int CONFIG_WIFI_DETECT_ENABLE = 1;
		const int CONFIG_TWS_AFH_ENABLE     = 1;
	#else
		const int CONFIG_WIFI_DETECT_ENABLE = 0;
		const int CONFIG_TWS_AFH_ENABLE     = 0;
	#endif

#else

#if defined CONFIG_CPU_BR27 || defined CONFIG_CPU_BR28 || defined CONFIG_CPU_BR36 || defined CONFIG_CPU_BR42

        const int CONFIG_WIFI_DETECT_ENABLE = 0;
        const int CONFIG_TWS_AFH_ENABLE     = 0;
#else
        const int CONFIG_WIFI_DETECT_ENABLE = 3;

#if TCFG_USER_TWS_ENABLE
        const int CONFIG_TWS_AFH_ENABLE     = 1;
#else
        const int CONFIG_TWS_AFH_ENABLE     = 0;
#endif

#endif
#endif//end CONFIG_SUPPORT_WIFI_DETECT

const int ESCO_FORWARD_ENABLE = 0;


const int config_bt_function  = 0;

///bredr 强制 做 maseter
const int config_btctler_bredr_master = 0;
const int config_btctler_dual_a2dp  = 0;

///afh maseter 使用app设置的map 通过USER_CTRL_AFH_CHANNEL 设置
const int config_bredr_afh_user = 0;
//bt PLL 温度跟随trim
const int config_bt_temperature_pll_trim = 0;
/*security check*/
const int config_bt_security_vulnerability = 0;

const int config_delete_link_key          = 1;           //配置是否连接失败返回PIN or Link Key Missing时删除linkKey

/*-----------------------------------------------------------*/

/**
 * @brief Bluetooth LE setting
 */

#if (TCFG_USER_BLE_ENABLE)
	#define DEFAULT_LE_FEATURES (LE_ENCRYPTION | LE_DATA_PACKET_LENGTH_EXTENSION | LL_FEAT_LE_EXT_ADV)

	#if (BT_AI_SEL_PROTOCOL & LE_AUDIO_CIS_RX_EN)
	    #define LE_AUDIO_CIS_LE_FEATURES (LE_ENCRYPTION | LE_FEATURES_CIS | LE_2M_PHY|CHANNEL_SELECTION_ALGORITHM_2|LL_FEAT_LE_EXT_ADV)
    #else
		#define LE_AUDIO_CIS_LE_FEATURES  0
	#endif

	#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
	    #define RCSP_MODE_LE_FEATURES (LE_ENCRYPTION | LE_DATA_PACKET_LENGTH_EXTENSION | LE_2M_PHY | LL_FEAT_LE_EXT_ADV)
    #else
       #define RCSP_MODE_LE_FEATURES 0
	#endif

	#if (BT_AI_SEL_PROTOCOL & LE_AUDIO_BIS_RX_EN)
       #define LE_AUDIO_BIS_RX_LE_FEATURES (LE_FEATURES_BIS | LE_CORE_V50_FEATURES)
       #define LE_AUDIO_BIS_RX_LE_ROLE (LE_MASTER  | LE_SCAN)
    #elif RCSP_BLE_CLIENT_EN
       #define LE_AUDIO_BIS_RX_LE_FEATURES 0
       #define LE_AUDIO_BIS_RX_LE_ROLE     (LE_MASTER  | LE_SCAN | LE_INIT)
    #else
       #define LE_AUDIO_BIS_RX_LE_FEATURES 0
       #define LE_AUDIO_BIS_RX_LE_ROLE     0
	#endif

	const int config_btctler_le_roles    = (LE_SLAVE  | LE_ADV|LE_AUDIO_BIS_RX_LE_ROLE);
	const uint64_t config_btctler_le_features = LE_AUDIO_CIS_LE_FEATURES|DEFAULT_LE_FEATURES|RCSP_MODE_LE_FEATURES|LE_AUDIO_BIS_RX_LE_FEATURES;

#else /* TCFG_USER_BLE_ENABLE */
	const int config_btctler_le_roles    = 0;
	const uint64_t config_btctler_le_features = 0;
#endif//end TCFG_USER_BLE_ENABLE


// Slave multi-link
const int config_btctler_le_slave_multilink = 1;

// Master multi-link
const int config_btctler_le_master_multilink = 0;
// LE RAM Control

const int config_btctler_le_hw_nums = MAX_HW_NUM + 1;


#define BLE_STATIC_RAM

#ifdef BLE_STATIC_RAM
#define MAX_STATIC_RAM_T_RTX_BUF MAX_HW_NUM * 2560

u8 bb_static_ram[MAX_STATIC_RAM_T_RTX_BUF] SEC(.ble_static_ram);
const int config_btctler_le_rtx_static_ram = MAX_STATIC_RAM_T_RTX_BUF;

#else
const int config_btctler_le_rtx_static_ram = 0;
u8 bb_static_ram[0];
#endif


const int config_btctler_le_slave_conn_update_winden = 800;//range:100 to 2500 限制最大值
#if TCFG_LOWPOWER_LOWPOWER_SEL
const int config_btctler_le_clock_accuracy = 300;  //允许低功耗晶振偏差每秒300ppm (300us)  低功耗场景配置300，非低功耗应用场景给100即可
#else
const int config_btctler_le_clock_accuracy = 100;  //允许低功耗晶振偏差每秒300ppm (300us)  低功耗场景配置300，非低功耗应用场景给100即可
#endif

const int config_bb_optimized_ctrl = VENDOR_BB_ISO_DIRECT_PUSH;//BIT(7);//|BIT(8);


#if (BT_AI_SEL_PROTOCOL & LE_AUDIO_CIS_RX_EN)
    #define TWS_LE_AUDIO_LE_ROLE_SW_EN (0)
#else
    #define TWS_LE_AUDIO_LE_ROLE_SW_EN (0)
#endif

#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
    #define TWS_RCSP_LE_ROLE_SW_EN (1)
#else
    #define TWS_RCSP_LE_ROLE_SW_EN (0)
#endif

const int config_btctler_le_afh_en = 0;
const u32 config_vendor_le_bb = 0;
const bool config_le_high_priority = 1;  //ecso下 想保证ble 建立连接 和 主从切换正常 必须置为1
const bool config_tws_le_role_sw =(TWS_LE_AUDIO_LE_ROLE_SW_EN|TWS_RCSP_LE_ROLE_SW_EN);
const int config_btctler_le_rx_nums = 18;
const int config_btctler_le_acl_packet_length = 251;
const int config_btctler_le_acl_total_nums = 20;

const int config_btctler_le_hrate_feature 	= CONFIG_BLE_HRATE_FEATURE;
const int config_btctler_le_cs_en 			= CONFIG_BLE_CS_EN;


/*-----------------------------------------------------------*/
/**
 * @brief Bluetooth Analog setting
 */
/*-----------------------------------------------------------*/
#if ((!TCFG_USER_BT_CLASSIC_ENABLE) && TCFG_USER_BLE_ENABLE)
	const int config_btctler_single_carrier_en = 1;   ////单模ble才设置
#else
	const int config_btctler_single_carrier_en = 0;
#endif

const int sniff_support_reset_anchor_point = 0;   //sniff状态下是否支持reset到最近一次通信点，用于HID
const int sniff_long_interval = (500 / 0.625);    //sniff状态下进入long interval的通信间隔(ms)
const int config_rf_oob = 0;

//               verb   debug   info  warn  error char
//RF part
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, Analog)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, RF)
REGISTER_LOG_TAG(TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, BDMGR)

//Classic part
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, HCI_LMP)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, LMP)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, LINK)

//LE part
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, LE_BB)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LE5_BB)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, HCI_LL)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, LL)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_E)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_M)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_ADV)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_SCAN)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_INIT)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_EXT_ADV)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_EXT_SCAN)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_EXT_INIT)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_TWS_ADV)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_TWS_SCAN)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, LL_S)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, LL_RL)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, LL_WL)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, AES)
REGISTER_LOG_TAG(TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, AES128)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_PADV)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_DX)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_PHY)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, LL_AFH)

//HCI part
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, Thread)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, HCI_STD)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, HCI_LL5)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, LL_ISO)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_BIS)
REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, LL_CIS)

REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  TRUE,  BL)
REGISTER_LOG_TAG(TRUE,  TRUE,  FALSE, FALSE, FALSE, TRUE,  TWS_LE)
REGISTER_LOG_TAG(TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE, TWS_LMP)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, TWS)
REGISTER_LOG_TAG(TRUE,  TRUE,  TRUE,  FALSE, TRUE,  FALSE, TWS_ESCO)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, QUICK_CONN)

