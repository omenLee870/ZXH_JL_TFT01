#ifndef APP_CONFIG_H
#define APP_CONFIG_H


// 未定义IO
#define NO_CONFIG_PORT						(-1)

// 使能、非使能
#define ENABLE_THIS_MOUDLE					1
#define DISABLE_THIS_MOUDLE					0

#define ENABLE								1
#define DISABLE								0

#include "audio_cvp_def.h"
#include "audio_def.h"
#include "btcontroller_mode.h"
#include "rcsp_define.h"
#include "board_config.h"
#include "lcd/lcd_conf.h"

/*#define CONFIG_CXX_SUPPORT */

/*
 * 系统打印总开关
 */

#define CONFIG_DEBUG_ENABLE 		TCFG_DEBUG_UART_ENABLE


#if CONFIG_DEBUG_ENABLE
#define LIB_DEBUG    1
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)
#define CONFIG_DEBUG_LITE_ENABLE    0//轻量级打印开关, 默认关闭
#else
#define LIB_DEBUG    0
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)
#define CONFIG_DEBUG_LITE_ENABLE    0//轻量级打印开关, 默认关闭
#endif

//*********************************************************************************//
//                                 在线调试宏定义                                  //
//*********************************************************************************//
#define TCFG_NULL_COMM						0				//不支持通信
#define TCFG_UART_COMM						1				//串口通信
#define TCFG_SPP_COMM						2				//SPP通信
#define TCFG_USB_COMM						3				//USB通信

//*********************************************************************************//
//                          文件系统相关配置                                       //
//*********************************************************************************//

#define TCFG_DEV_MANAGER_ENABLE                   1
#define CONFIG_FATFS_ENABLE                       1
#define CONFIG_RES_SDFILE_FS_ENABLE               0

#define SDFILE_DEV				"sdfile"
#undef SDFILE_MOUNT_PATH
#define SDFILE_MOUNT_PATH     	"mnt/sdfile"
#define SDFILE_APP_ROOT_PATH    SDFILE_MOUNT_PATH"/app/"  //app分区
#define SDFILE_RES_ROOT_PATH    SDFILE_MOUNT_PATH"/res/"  //资源文件分区

#if TCFG_SD0_ENABLE
#define CONFIG_STORAGE_PATH 	"storage/sd0"  //定义对应SD0的路径
#define SDX_DEV					"sd0"
#elif TCFG_SD1_ENABLE
#define CONFIG_STORAGE_PATH 	"storage/sd1" //定义对应SD1的路径
#define SDX_DEV					"sd1"
#else
#define CONFIG_STORAGE_PATH 	"storage/virfat_flash"
#define SDX_DEV					"virfat_flash"
#endif
#define CONFIG_ROOT_PATH        CONFIG_STORAGE_PATH"/C/" //定义对应SD文件系统的根目录路径
#define CONFIG_REC_PATH         CONFIG_ROOT_PATH"DCIM/VIDEO/"
#define VIDEO_REC_FILE_PATH     CONFIG_REC_PATH
#define CONFIG_PHOTO_PATH       CONFIG_ROOT_PATH"DCIM/PHOTO/"


#define LIB_SUPPORT_MULTI_SECTOER_READ  0 //对应配合库修改是否一次性多sector读
#if LIB_SUPPORT_MULTI_SECTOER_READ
#define MAX_READ_LEN 8192
#endif

#define CONFIG_VIDEO0_ENABLE
//默认网络视频为JPEG格式
#define CONFIG_NET_JPEG
// #define CONFIG_NET_TCP_ENABLE
#define CONFIG_NET_UDP_ENABLE
#define CONFIG_ENABLE_VLIST

/*-------------网络端口----------------*/
#define CTP_CTRL_PORT   	3333
#define CDP_CTRL_PORT   	2228
#define NET_SCR_PORT    	2230
#define VIDEO_PREVIEW_PORT 	2226
#define VIDEO_PLAYBACK_PORT 2223
#define HTTP_PORT           8080
#define RTSP_PORT           554

#define _DUMP_PORT          2229
#define _FORWARD_PORT    	2224
#define _BEHIND_PORT     	2225
#define _THIRD_PORT     	3225

#define CONFIG_NET_PKG_JPEG

#define CONFIG_REC_DIR_0       "DCIM/1/"
#define CONFIG_REC_DIR_1       "DCIM/2/"
#define CONFIG_REC_DIR_2       "DCIM/3/"
#define CONFIG_REC_DIR_4       "DCIM/4/"
#define CONFIG_REC_DIR_5       "DCIM/5/"

#if TCFG_SD0_ENABLE
#define CONFIG_STORAGE_PATH 	"storage/sd0"
#define SDX_DEV					"sd0"
#endif

#if TCFG_SD1_ENABLE
#define CONFIG_STORAGE_PATH 	"storage/sd1"
#define SDX_DEV					"sd1"
#endif

#ifndef CONFIG_STORAGE_PATH
#define CONFIG_STORAGE_PATH		"no_sd_card" //不使用SD定义对应别的路径，防止编译出错
#define SDX_DEV					"no_sd"
#endif

#define CONFIG_ROOT_PATH     	CONFIG_STORAGE_PATH"/C/"
#define CONFIG_REC_PATH_0       CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_0
#define CONFIG_REC_PATH_1       CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_1
#define CONFIG_REC_PATH_2       CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_2
#define CONFIG_REC_PATH_4       CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_4
#define CONFIG_REC_PATH_5       CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_5


//*********************************************************************************//
//                                  BREDR && BLE配置                               //
//*********************************************************************************//
#if !TCFG_APP_BT_EN
#undef TCFG_BT_BACKGROUND_ENABLE
#define TCFG_BT_BACKGROUND_ENABLE   0
#endif

#define TCFG_GATT_OVER_EDR_EN       0

#if !TCFG_USER_BLE_ENABLE && !TCFG_GATT_OVER_EDR_EN

#ifdef TCFG_BLE_BRIDGE_EDR_ENALBE
#undef TCFG_BLE_BRIDGE_EDR_ENALBE
#endif
#define TCFG_BLE_BRIDGE_EDR_ENALBE	0

#ifdef TCFG_BT_AI_ENABLE
#undef TCFG_BT_AI_ENABLE
#endif
#define TCFG_BT_AI_ENABLE			0

#endif

#define GATT_OVER_EDR_DEMO_EN    0

#if TCFG_BLE_BRIDGE_EDR_ENALBE   //一键连接必须同地址
#undef  TCFG_BT_BLE_BREDR_SAME_ADDR
#define  TCFG_BT_BLE_BREDR_SAME_ADDR 0x1
#define  DOUBLE_BT_SAME_MAC 0x1
#endif

#define TCFG_BT_SUPPORT_BIP         0X1
#if TCFG_BT_SUPPORT_BIP
#undef TCFG_BT_MUSIC_INFO_ENABLE
#define TCFG_BT_MUSIC_INFO_ENABLE 1 // 歌曲信息显示
#endif

#if (TCFG_USER_EMITTER_ENABLE && TCFG_VSTREAM_TRANSFER)
#define CONFIG_BREDR_STATIC_RAM_LEN (24*1024)
#elif (defined(TCFG_USER_EMITTER_ENABLE) && TCFG_USER_EMITTER_ENABLE)
#define CONFIG_BREDR_STATIC_RAM_LEN (12*1024)
#elif (CONFIG_BT_MODE != BT_NORMAL) || TCFG_NORMAL_SET_DUT_MODE
#define CONFIG_BREDR_STATIC_RAM_LEN (12*1024)
#elif (TCFG_USER_BT_CLASSIC_ENABLE)
#define CONFIG_BREDR_STATIC_RAM_LEN (6*1024)
#else
#define CONFIG_BREDR_STATIC_RAM_LEN (0)
#endif


#if (defined(TCFG_BLE_BRIDGE_EDR_ENALBE) && TCFG_BLE_BRIDGE_EDR_ENALBE)
#define BT_CTKD_CONN_SPEED          1
#endif

#define BT_PWR_MAX_ADD_LEVEL		0 //最大功率档位累加

#if TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
#define TCFG_BT_BACKGROUND_GOBACK   1 //蓝牙后台连接断开返回
#else
#define TCFG_BT_BACKGROUND_GOBACK   0 //蓝牙后台连接断开返回
#endif

#define WECHAT_SPORT_ENABLE         0//微信运动

//*********************************************************************************//
//                                 rtc配置                                   	   //
//*********************************************************************************//

#define TCFG_APP_RTC_EN           	1

#define TCFG_USE_VIRTUAL_RTC  		0

#ifndef TCFG_RTC_ENABLE
#define TCFG_RTC_ENABLE      		TCFG_APP_RTC_EN
#endif

#define RTC_CLK_RES_SEL	            CLK_SEL_LRC


//*********************************************************************************//
//                                  AI配置                                       //
//*********************************************************************************//
#define TCFG_RCSP_DUAL_CONN_ENABLE         	0

#define    RCSP_MODE_EN             (1 << 0)
#define    TRANS_DATA_EN            (1 << 1)
#define    LL_SYNC_EN               (1 << 2)
#define    TUYA_DEMO_EN             (1 << 3)
#define    ANCS_CLIENT_EN           (1 << 4)
#define    GFPS_EN                  (1 << 5)
#define    REALME_EN                (1 << 6)
#define    TME_EN                   (1 << 7)
#define    DMA_EN                   (1 << 8)
#define    GMA_EN                   (1 << 9)
#define    MMA_EN                   (1 << 10)
#define    FMNA_EN                  (1 << 11)
#define    SWIFT_PAIR_EN            (1 << 12)
#define    LE_AUDIO_CIS_RX_EN       (1 << 13)
#define    LE_AUDIO_CIS_TX_EN       (1 << 14)
#define    LE_AUDIO_BIS_RX_EN       (1 << 15)
#define    LE_AUDIO_BIS_TX_EN       (1 << 16)
#define    HONOR_EN                 (1 << 17)
#define    ONLINE_DEBUG_EN          (1 << 18)
#define    CUSTOM_DEMO_EN           (1 << 19)   // 第三方协议的demo，用于示例客户开发自定义协议
#define    ALIPAY_EN                (1 << 20)
#define    HID_ISO_EN               (1 << 21)

#if TCFG_BT_AI_ENABLE && TCFG_UI_ENABLE && CONFIG_JL_UI_ENABLE && TCFG_PAY_ALIOS_ENABLE
#define AI_SEL_ALIPAY				ALIPAY_EN
#else
#define AI_SEL_ALIPAY				0
#endif

#if TCFG_BT_AI_ENABLE
#if TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH
#if TCFG_UI_ENABLE || TCFG_EARPHONE_PROTOCOL
#define AI_SEL_RCSP					RCSP_MODE_EN
#else
#define AI_SEL_RCSP					0
#endif
#else
#define AI_SEL_RCSP					RCSP_MODE_EN
#endif //TCFG_SUB_CASE_TYPE
#else
#define AI_SEL_RCSP					0
#endif //TCFG_BT_AI_ENABLE

#if TCFG_BT_AI_ENABLE
#if TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH
#if (!TCFG_UI_ENABLE) && (!TCFG_EARPHONE_PROTOCOL)
#define AI_SEL_TRANS				TRANS_DATA_EN
#else
#define AI_SEL_TRANS				0
#endif
#else
#define AI_SEL_TRANS				0
#endif //TCFG_SUB_CASE_TYPE
#else
#define AI_SEL_TRANS				0
#endif //TCFG_BT_AI_ENABLE

#if TCFG_BT_AI_ENABLE && TCFG_UI_ENABLE && CONFIG_JL_UI_ENABLE && TCFG_FINDMY_ENABLE
#define AI_SEL_FMNA					FMNA_EN
#else
#define AI_SEL_FMNA					0
#endif


#define BT_AI_SEL_PROTOCOL  		(AI_SEL_TRANS | AI_SEL_RCSP | AI_SEL_ALIPAY | AI_SEL_FMNA)

#if (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_SPP_COMM))
#undef  BT_AI_SEL_PROTOCOL
#define BT_AI_SEL_PROTOCOL  		ONLINE_DEBUG_EN
#endif

#define RCSP_USE_BLE      			0
#define RCSP_USE_SPP      			1
#define RCSP_CHANNEL_SEL  			RCSP_USE_SPP

#if (BT_AI_SEL_PROTOCOL & FMNA_EN)
//存放token信息
#define CONFIG_FINDMY_INFO_ENABLE      		    1		//配置是否支持FINDMY存储
#define CONFIG_USE_RANDOM_ADDRESS_ENABLE   		0
#else
#define CONFIG_FINDMY_INFO_ENABLE      		    0
#define CONFIG_USE_RANDOM_ADDRESS_ENABLE   		0
#endif


#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
#define TRANS_ANCS_EN  			  	 1
#define TRANS_AMS_EN  			  	 0
#else
#define TRANS_ANCS_EN  			  	 0
#define TRANS_AMS_EN  			  	 0
#endif

#if TCFG_BLE_BRIDGE_EDR_ENALBE && TCFG_USER_BLE_CTRL_BREDR_EN && (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)&&(!TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE)
#define TCFG_EDR_SCAN_CONN_CTRL     1  /*一键连接时候,edr可发现可连接状态控制*/
#else
#define TCFG_EDR_SCAN_CONN_CTRL     0  /*一键连接时候,edr可发现可连接状态控制*/
#endif

/* -----------------移植了耳机的文件，定义一下那边的宏名字------ */
#define    LE_AUDIO_UNICAST_SOURCE_EN           (1 << 0)
#define    LE_AUDIO_UNICAST_SINK_EN             (1 << 1)
#define    LE_AUDIO_AURACAST_SOURCE_EN          (1 << 2)
#define    LE_AUDIO_AURACAST_SINK_EN            (1 << 3)
#define    LE_AUDIO_JL_UNICAST_SOURCE_EN        (1 << 4)
#define    LE_AUDIO_JL_UNICAST_SINK_EN          (1 << 5)
#define    LE_AUDIO_JL_AURACAST_SOURCE_EN       (1 << 6)
#define    LE_AUDIO_JL_AURACAST_SINK_EN         (1 << 7)

#define THIRD_PARTY_PROTOCOLS_SEL				BT_AI_SEL_PROTOCOL

#if (BT_AI_SEL_PROTOCOL & LE_AUDIO_CIS_RX_EN)
#define TCFG_LE_AUDIO_APP_CONFIG       			LE_AUDIO_UNICAST_SINK_EN
#else
#define TCFG_LE_AUDIO_APP_CONFIG       			0
#endif

#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
#define	   RCSP_MODE			     RCSP_MODE_WATCH
#else
#define	   RCSP_MODE			     RCSP_MODE_OFF
#endif

#include "rcsp_cfg.h" // 详细功能参考rcsp_cfg.h


//单双备份的配置在board_xxx_global_cfg里配置，需要注意只有RCSP才支持单双备份，其余升级都是只支持双备份升级
//支持TWS同步升级，OTA_TWS_SAME_TIME_NEW宏需要配置为1，旧的流程已不再支持
#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
#if TCFG_AI_VOICE_ENABLE
#define    BT_MIC_EN                 1
#endif
#if (!TCFG_USER_BLE_ENABLE) && (!TCFG_GATT_OVER_EDR_EN)
#error "RCSP(TCFG_BT_AI_ENABLE) must open TCFG_USER_BLE_ENABLE"
#endif

#elif (BT_AI_SEL_PROTOCOL & (GFPS_EN | REALME_EN | TME_EN | DMA_EN | GMA_EN))
#define    BT_MIC_EN                 0
#ifndef TCFG_ENC_OPUS_ENABLE
#define    TCFG_ENC_OPUS_ENABLE      0
#endif
#ifndef TCFG_ENC_SPEEX_ENABLE
#define    TCFG_ENC_SPEEX_ENABLE     0
#endif
#define    OTA_TWS_SAME_TIME_ENABLE  0     //是否支持TWS同步升级
#elif (BT_AI_SEL_PROTOCOL & LL_SYNC_EN)
#define    OTA_TWS_SAME_TIME_ENABLE  1
#define    OTA_TWS_SAME_TIME_NEW     1     //使用新的tws ota流程
#ifndef TCFG_ENC_OPUS_ENABLE
#define    TCFG_ENC_OPUS_ENABLE      0
#endif
#ifndef TCFG_ENC_SPEEX_ENABLE
#define    TCFG_ENC_SPEEX_ENABLE     0
#endif
#elif (BT_AI_SEL_PROTOCOL & TUYA_DEMO_EN)
#define    OTA_TWS_SAME_TIME_ENABLE  0
#define    OTA_TWS_SAME_TIME_NEW     0     //使用新的tws ota流程
#ifndef TCFG_ENC_OPUS_ENABLE
#define    TCFG_ENC_OPUS_ENABLE      0
#endif
#ifndef TCFG_ENC_SPEEX_ENABLE
#define    TCFG_ENC_SPEEX_ENABLE     0
#endif
#else
#define    OTA_TWS_SAME_TIME_ENABLE  0
#define    OTA_TWS_SAME_TIME_NEW     0     //使用新的tws ota流程
#ifndef TCFG_ENC_OPUS_ENABLE
#define    TCFG_ENC_OPUS_ENABLE      0
#endif
#ifndef TCFG_ENC_SPEEX_ENABLE
#define    TCFG_ENC_SPEEX_ENABLE     0
#endif
#endif


#define APP_ONLINE_DEBUG          	0//在线APP调试,发布默认不开

#include "btcontroller_mode.h"

#include "user_cfg_id.h"

#ifndef __LD__
#include "bt_profile_cfg.h"
#endif



//*********************************************************************************//
//                                 测试模式配置                                    //
//*********************************************************************************//
#if (CONFIG_BT_MODE == BT_NORMAL)
//enable dut mode,need disable sleep(TCFG_LOWPOWER_LOWPOWER_SEL = 0)
// #define TCFG_NORMAL_SET_DUT_MODE                  0
#if TCFG_NORMAL_SET_DUT_MODE
#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL                0
#endif

#else
#undef TCFG_BT_DUAL_CONN_ENABLE
#define TCFG_BT_DUAL_CONN_ENABLE 0x0 //一拖二

#undef  TCFG_USER_TWS_ENABLE
#define TCFG_USER_TWS_ENABLE                      0     //tws功能使能

#undef  TCFG_USER_BLE_ENABLE
#define TCFG_USER_BLE_ENABLE                      1     //BLE功能使能

#undef TCFG_BT_BLE_BREDR_SAME_ADDR
#define  TCFG_BT_BLE_BREDR_SAME_ADDR     0

#undef  TCFG_AUTO_SHUT_DOWN_TIME
#define TCFG_AUTO_SHUT_DOWN_TIME		          0

#undef  TCFG_SYS_LVD_EN
#define TCFG_SYS_LVD_EN						      0

#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL                0

#undef TCFG_AUDIO_DAC_LDO_VOLT
#define TCFG_AUDIO_DAC_LDO_VOLT			   DUT_AUDIO_DAC_LDO_VOLT

#undef TCFG_LOWPOWER_POWER_SEL
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15

#undef  TCFG_PWMLED_ENABLE
#define TCFG_PWMLED_ENABLE					DISABLE_THIS_MOUDLE

#undef TCFG_TEST_BOX_ENABLE
#define TCFG_TEST_BOX_ENABLE			    0

#undef TCFG_AUTO_POWERON_ENABLE
#define TCFG_AUTO_POWERON_ENABLE    1

#undef TCFG_CFG_TOOL_ENABLE
#define TCFG_CFG_TOOL_ENABLE 0
#undef TCFG_USB_HOST_ENABLE
#define TCFG_USB_HOST_ENABLE 0 //U盘使能
/* #undef TCFG_UART0_ENABLE
#define TCFG_UART0_ENABLE					DISABLE_THIS_MOUDLE */

#endif //(CONFIG_BT_MODE != BT_NORMAL)


#if TCFG_USER_TWS_ENABLE

#define CONFIG_TWS_COMMON_ADDR_AUTO             0       /* 自动生成TWS配对后的MAC地址 */
#define CONFIG_TWS_COMMON_ADDR_USED_LEFT        1       /* 使用左耳的MAC地址作为TWS配对后的地址
                                                           可配合烧写器MAC地址自增功能一起使用
                                                           多台交叉配对会出现MAC地址相同情况 */
#define CONFIG_TWS_COMMON_ADDR_SELECT           CONFIG_TWS_COMMON_ADDR_AUTO

//*********************************************************************************//
//                                 对耳配置方式配置                                    //
//*********************************************************************************//
#define CONFIG_TWS_CONNECT_SIBLING_TIMEOUT    4    /* 开机或超时断开后对耳互连超时时间，单位s */
// #define CONFIG_TWS_REMOVE_PAIR_ENABLE              [> 不连手机的情况下双击按键删除配对信息 <]
//#define CONFIG_TWS_POWEROFF_SAME_TIME         1    /*按键关机时两个耳机同时关机*/

#define ONE_KEY_CTL_DIFF_FUNC                 1    /*通过左右耳实现一个按键控制两个功能*/
#define CONFIG_TWS_SCO_ONLY_MASTER			  0	   /*通话的时候只有主机出声音*/

/* 配对方式选择 */
#define CONFIG_TWS_PAIR_BY_CLICK            0      /* 按键发起配对 */
#define CONFIG_TWS_PAIR_BY_AUTO             1      /* 开机自动配对 */
#define CONFIG_TWS_PAIR_BY_BOX              2      /* 测试盒/充电仓配对 */
#define CONFIG_TWS_PAIR_MODE                TCFG_BT_TWS_PAIR_MODE


/* 声道确定方式选择 */
#define CONFIG_TWS_MASTER_AS_LEFT               0 //主机作为左耳
#define CONFIG_TWS_MASTER_AS_RIGHT              1 //主机作为右耳
#define CONFIG_TWS_AS_LEFT                      2 //固定左耳
#define CONFIG_TWS_AS_RIGHT                     3 //固定右耳
#define CONFIG_TWS_START_PAIR_AS_LEFT           4 //双击发起配对的耳机做左耳
#define CONFIG_TWS_START_PAIR_AS_RIGHT          5 //双击发起配对的耳机做右耳
#define CONFIG_TWS_EXTERN_UP_AS_LEFT            6 //外部有上拉电阻作为左耳
#define CONFIG_TWS_EXTERN_DOWN_AS_LEFT          7 //外部有下拉电阻作为左耳
#define CONFIG_TWS_EXTERN_UP_AS_RIGHT           8 //外部有上拉电阻作为右耳
#define CONFIG_TWS_EXTERN_DOWN_AS_RIGHT         9 //外部有下拉电阻作为右耳
#define CONFIG_TWS_CHANNEL_SELECT_BY_BOX        10 //充电仓/测试盒决定左右耳
#define CONFIG_TWS_CHANNEL_SELECT             TCFG_BT_TWS_CHANNEL_SELECT //配对方式选择

#define CONFIG_TWS_CHANNEL_CHECK_IO           IO_PORTA_07					//上下拉电阻检测引脚


#if CONFIG_TWS_PAIR_MODE != CONFIG_TWS_PAIR_BY_CLICK
#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_START_PAIR_AS_LEFT) ||\
    (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_START_PAIR_AS_RIGHT)
#undef CONFIG_TWS_CHANNEL_SELECT
#define CONFIG_TWS_CHANNEL_SELECT             CONFIG_TWS_MASTER_AS_LEFT
#endif
#endif

#if CONFIG_TWS_USE_COMMMON_ADDR == 0
#undef TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE
#define TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE 0
#endif

#if TCFG_CHARGESTORE_ENABLE
#undef CONFIG_TWS_CHANNEL_SELECT
#define CONFIG_TWS_CHANNEL_SELECT             CONFIG_TWS_CHANNEL_SELECT_BY_BOX //充电仓区分左右
#endif


#if TCFG_TEST_BOX_ENABLE && (!TCFG_CHARGESTORE_ENABLE)
#define CONFIG_TWS_SECECT_CHARGESTORE_PRIO    1 //测试盒配置左右耳优先
#else
#define CONFIG_TWS_SECECT_CHARGESTORE_PRIO    0
#endif //TCFG_TEST_BOX_ENABLE

#define TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END 1     //tws播完开机提示音再初始化，处理提示音不同步问题


#define CONFIG_A2DP_GAME_MODE_ENABLE            0  //游戏模式
#define CONFIG_A2DP_GAME_MODE_DELAY_TIME        35  //游戏模式延时ms

//*********************************************************************************//
//      低延时游戏模式脚步声、枪声增强,需使能蓝牙音乐10段eq以及蓝牙音乐drc
//      用户开关宏AUDIO_GAME_EFFECT_CONFIG(开关蓝牙低延时模式的游戏音效)
//      低延时eq效果文件使用eq_game_eff.bin,调试时需保存成该文件,并在批处理-res后添加
//      非低延时eq效果文件使用eq_cfg_hw.bin,也需在批处理-res后添加
//*********************************************************************************//
#if CONFIG_A2DP_GAME_MODE_ENABLE
#define AUDIO_GAME_EFFECT_CONFIG  1  //低延时游戏模式脚步声、枪声增强 1:使能、0：关闭
#else
#define AUDIO_GAME_EFFECT_CONFIG  0  //低延时游戏模式脚步声、枪声增强 1:使能、0：关闭
#endif


//*********************************************************************************//
//                                 对耳电量显示方式                                //
//*********************************************************************************//

#if TCFG_BT_DISPLAY_BAT_ENABLE
#define CONFIG_DISPLAY_TWS_BAT_LOWER          1 //对耳手机端电量显示，显示低电量耳机的电量
#define CONFIG_DISPLAY_TWS_BAT_HIGHER         2 //对耳手机端电量显示，显示高电量耳机的电量
#define CONFIG_DISPLAY_TWS_BAT_LEFT           3 //对耳手机端电量显示，显示左耳的电量
#define CONFIG_DISPLAY_TWS_BAT_RIGHT          4 //对耳手机端电量显示，显示右耳的电量

#define CONFIG_DISPLAY_TWS_BAT_TYPE           CONFIG_DISPLAY_TWS_BAT_LOWER
#endif

#define CONFIG_DISPLAY_DETAIL_BAT             0 //BLE广播显示具体的电量
#define CONFIG_NO_DISPLAY_BUTTON_ICON         1 //BLE广播不显示按键界面,智能充电仓置1

#else //TCFG_USER_TWS_ENABLE

#define TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END 	0
#define CONFIG_A2DP_GAME_MODE_ENABLE            	0

#endif //TCFG_USER_TWS_ENABLE



//*********************************************************************************//
//                                  系统配置                                       //
//*********************************************************************************//

#ifndef TCFG_IO_CFG_AT_POWER_ON
#define TCFG_IO_CFG_AT_POWER_ON  0 //开机时IO配置
#endif


#ifndef TCFG_IO_CFG_AT_POWER_OFF
#define TCFG_IO_CFG_AT_POWER_OFF 0 //关机时IO配置
#endif


#ifndef TCFG_DYNAMIC_SWITCHING_IOVDDM_ENABLE
#define TCFG_DYNAMIC_SWITCHING_IOVDDM_ENABLE 0 //动态切换IOVDD
#endif


//*********************************************************************************//
//                                 电源切换配置                                    //
//*********************************************************************************//

#define PHONE_CALL_USE_LDO15	CONFIG_PHONE_CALL_USE_LDO15

//*********************************************************************************//
//                                 时钟切换配置                                    //
//*********************************************************************************//
#define BT_NORMAL_HZ	            24000000

#define MAX_LIMIT_SYS_CLOCK_128M	128000000L
#define MAX_LIMIT_SYS_CLOCK_160M	160000000L

#ifndef TCFG_MAX_LIMIT_SYS_CLOCK
#define TCFG_MAX_LIMIT_SYS_CLOCK	MAX_LIMIT_SYS_CLOCK_160M
#endif

//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
#if TCFG_IRKEY_ENABLE
#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL			0                     //开红外不进入低功耗
#endif  /* #if TCFG_IRKEY_ENABLE */

//*********************************************************************************//
//                                 batter配置                                      //
//*********************************************************************************//
#define TCFG_VBAT_TRIM_EN			0 // vbat trim
#if TCFG_VBAT_TRIM_EN
#define TCFG_BATTER_OFFSET_EN		1 // vbat补偿
#endif


//*********************************************************************************//
//                            LED使用 16SLOT TIMER 同步                            //
//*********************************************************************************//
//LED模块使用slot timer同步使用注意点:
//	1.watch不开该功能, 原因: 默认打开osc时钟, 使用原来的osc流程同步即可
//	2.带sd卡earphone不开该功能, 一般为单耳, 不需要同步, 使用原来的流程(lrc)
//	3.一般用在tws应用中, 而且默认关闭osc;
#if TCFG_USER_TWS_ENABLE
#define TCFG_PWMLED_USE_SLOT_TIME			ENABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//                                 升级配置                                        //
//*********************************************************************************//
//升级LED显示使能
#define UPDATE_LED_REMIND
//升级提示音使能
#define UPDATE_VOICE_REMIND


// #undef CONFIG_UPDATE_JUMP_TO_MASK
// #ifndef CONFIG_UPDATE_JUMP_TO_MASK
// #define CONFIG_UPDATE_JUMP_TO_MASK              0   	//配置升级到loader的方式0为直接reset,1为跳转(适用于芯片电源由IO口KEEP住的方案,需要注意检查跳转前是否将使用DMA的硬件模块全部关闭)
// #endif

// //用户串口主从机升级
// #define USER_UART_UPDATE_ENABLE           0//用于客户开发上位机或者多MCU串口升级方案
// #define UART_UPDATE_SLAVE	0
// #define UART_UPDATE_MASTER	1
// //配置串口升级的角色
// #define UART_UPDATE_ROLE	UART_UPDATE_SLAVE

#if TCFG_UPDATE_UART_IO_EN
#undef TCFG_CHARGESTORE_ENABLE
#undef TCFG_TEST_BOX_ENABLE
#define TCFG_CHARGESTORE_ENABLE				DISABLE_THIS_MOUDLE       //用户串口升级也使用了UART1
#endif

//设备升级
#if TCFG_APP_MUSIC_EN
#define CONFIG_SD_UPDATE_ENABLE
#define CONFIG_USB_UPDATE_ENABLE
#endif
#define TCFG_DEV_UPDATE_IF_NOFILE_ENABLE   0//0：设备上线直接查找升级文件 1：无音乐文件时才查找升级文件


//*********************************************************************************//
//                                 Audio配置                                        //
//*********************************************************************************//

#define CONFIG_AUDIO_ENABLE     1
#define TCFG_DEC_OGG_OPUS_ENABLE			DISABLE

#ifndef TCFG_AUDIO_MIC_PWR_PORT
#define TCFG_AUDIO_MIC_PWR_PORT NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_MIC1_PWR_PORT
#define TCFG_AUDIO_MIC1_PWR_PORT NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_MIC2_PWR_PORT
#define TCFG_AUDIO_MIC2_PWR_PORT NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_PLNK_SCLK_PIN
#define TCFG_AUDIO_PLNK_SCLK_PIN NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_PLNK_DAT0_PIN
#define TCFG_AUDIO_PLNK_DAT0_PIN NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_PLNK_DAT1_PIN
#define TCFG_AUDIO_PLNK_DAT1_PIN NO_CONFIG_PORT
#endif

#define  SYS_VOL_TYPE 	VOL_TYPE_DIGITAL/*目前仅支持软件数字音量模式*/



#if (TCFG_AUDIO_CVP_SMS_ANS_MODE || TCFG_AUDIO_CVP_SMS_DNS_MODE)
/*1mic-CVP：单mic通话算法*/
#define TCFG_AUDIO_SINGLE_MIC_ENABLE	ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#if (TCFG_AUDIO_CVP_SMS_ANS_MODE)			/*单MIC+ANS通话*/
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_ANS_MODE
#elif (TCFG_AUDIO_CVP_SMS_DNS_MODE) 		/*单MIC+DNS通话*/
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#endif

#elif (TCFG_AUDIO_CVP_DMS_ANS_MODE || TCFG_AUDIO_CVP_DMS_DNS_MODE || \
	TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE ||TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE || \
	TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE  || TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE )
/*2mic-CVP:双mic通话算法*/
#define TCFG_AUDIO_SINGLE_MIC_ENABLE	DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE) /*双MIC+ANS通话*/
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL

#elif (TCFG_AUDIO_CVP_DMS_DNS_MODE) /*双MIC+DNS通话*/
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL

#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) /*话务双MIC+ANS通话*/
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_FLEXIBLE

#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE) /*话务双MIC+DNS通话*/
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	        DMS_FLEXIBLE

#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE) /*dms_hybrid通话*/
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_HYBRID

#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE) /*dms_AWN通话*/
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_AWN
#endif

#elif (TCFG_AUDIO_CVP_3MIC_MODE)
/*3mic-CVP:3mic通话算法*/
#define TCFG_AUDIO_SINGLE_MIC_ENABLE	DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#else
#define TCFG_AUDIO_SINGLE_MIC_ENABLE	DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#endif

#if TCFG_ESCO_DL_CVSD_SR_USE_16K
#define TCFG_AUDIO_CVP_BAND_WIDTH_CFG   (CVP_WB_EN) //只保留16k参数
#else
#define TCFG_AUDIO_CVP_BAND_WIDTH_CFG   (CVP_NB_EN | CVP_WB_EN)  //同时保留8k和16k的参数
#endif

/*Audio数据导出配置:通过蓝牙spp导出/sd写卡导出/uart写卡导出*/
#define AUDIO_DATA_EXPORT_VIA_UART	1
#define AUDIO_DATA_EXPORT_VIA_SPP 	2
#define AUDIO_DATA_EXPORT_VIA_SD	3
#define TCFG_AUDIO_DATA_EXPORT_DEFINE		TCFG_AUDIO_DATA_EXPORT_TYPE

/*
 *蓝牙spp数据导出的mic 通道，调试双麦ENC时,需要和ENC的mic通道保持一致
 *目前支持导出2路mic数据
 */
#if TCFG_AUDIO_TRIPLE_MIC_ENABLE
#define TCFG_SPP_DATA_EXPORT_ADC_MIC_CHA	(AUDIO_ADC_MIC_0 | AUDIO_ADC_MIC_1 | AUDIO_ADC_MIC_2)
#else
#define TCFG_SPP_DATA_EXPORT_ADC_MIC_CHA	(AUDIO_ADC_MIC_0 | AUDIO_ADC_MIC_1)
#endif

#if(defined(TCFG_AUDIO_CVP_DEVELOP_ENABLE)&& TCFG_AUDIO_CVP_DEVELOP_ENABLE)/*通话第三方算法*/
#define TCFG_CVP_DEVELOP_ENABLE 		CVP_CFG_USER_DEFINED
#endif/*TCFG_AUDIO_CVP_DEVELOP_ENABLE*/

#if defined(TCFG_CVP_DEVELOP_ENABLE) && (TCFG_CVP_DEVELOP_ENABLE == CVP_CFG_AIS_3MIC)
#define CONFIG_BOARD_AISPEECH_NR
#endif /*TCFG_CVP_DEVELOP_ENABLE*/


/*Audio Smart Voice*/
#ifndef TCFG_KWS_VOICE_RECOGNITION_ENABLE
#define TCFG_KWS_VOICE_RECOGNITION_ENABLE   DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_SMART_VOICE_ENABLE
#define TCFG_SMART_VOICE_ENABLE 		    DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_SMART_VOICE_USE_AEC
#define TCFG_SMART_VOICE_USE_AEC            DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_AUDIO_ASR_ALGO_SELECT
#define TCFG_AUDIO_ASR_ALGO_SELECT          DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_AUDIO_KWS_LANGUAGE_SEL
#define TCFG_AUDIO_KWS_LANGUAGE_SEL         KWS_CH//近场中文
#endif

#define TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE 	TCFG_SMART_VOICE_ENABLE //语音事件处理流程开关

#if TCFG_SMART_VOICE_ENABLE
#define TCFG_VAD_LOWPOWER_CLOCK             VAD_CLOCK_USE_RC_AND_BTOSC
#else
#define TCFG_VAD_LOWPOWER_CLOCK             VAD_CLOCK_USE_LRC
#endif/*TCFG_AUDIO_SMART_VOICE_ENABLE*/

#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
#define TCFG_CALL_KWS_SWITCH_ENABLE         DISABLE_THIS_MOUDLE
#else
#define TCFG_CALL_KWS_SWITCH_ENABLE         TCFG_SMART_VOICE_ENABLE
#endif /*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

/*
 *第三方ASR（语音识别）配置
 *(1)默认跑jl平台算法
 *#define TCFG_AUDIO_ASR_DEVELOP             DISABLE_THIS_MOUDLE
 *(2)用户自己开发算法
 *#define TCFG_AUDIO_ASR_DEVELOP             ASR_CFG_USER_DEFINED
 *(3)使用思必驰ASR算法
 *#define TCFG_AUDIO_ASR_DEVELOP             ASR_CFG_AIS
 */
#define ASR_CFG_AIS                         1
#define ASR_CFG_USER_DEFINED                2
#define TCFG_AUDIO_ASR_DEVELOP				TCFG_AUDIO_ASR_ALGO_SELECT

#if (TCFG_AUDIO_ASR_DEVELOP	== ASR_CFG_AIS)
#undef TCFG_SMART_VOICE_ENABLE
#undef TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE
#undef TCFG_VAD_LOWPOWER_CLOCK
#define TCFG_SMART_VOICE_ENABLE 		    DISABLE_THIS_MOUDLE
#define TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE 	TCFG_SMART_VOICE_ENABLE //语音事件处理流程开关
#define TCFG_VAD_LOWPOWER_CLOCK             VAD_CLOCK_USE_RC_AND_BTOSC
#define CONFIG_BOARD_AISPEECH_VAD_ASR
#elif (TCFG_AUDIO_ASR_DEVELOP == ASR_CFG_USER_DEFINED)
/*用户平台算法配置*/
#else
/*jl平台算法配置*/
#if TCFG_UI_ENABLE
#define TCFG_VAD_LP_CLOSE				    ENABLE // 进入低功耗的时候关闭；亮屏的时候打开
#define TCFG_KWS_HOLD_TIME					20 // 息屏后多长时间不让进低功耗。单位：秒
#endif
#endif /*TCFG_AUDIO_ASR_DEVELOP*/

//关键词KWS是离线识别的一个子功能，不能同时开启
#if TCFG_SMART_VOICE_ENABLE && TCFG_KWS_VOICE_RECOGNITION_ENABLE
#error "smart voice and kws voice recognition can not be enabled at the same time"
#endif

#ifndef TCFG_AUDIO_ANC_ENABLE
#define TCFG_AUDIO_ANC_ENABLE 				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_AUDIO_DUT_ENABLE
#define TCFG_AUDIO_DUT_ENABLE 				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_ANC_TOOL_DEBUG_ONLINE
#define TCFG_ANC_TOOL_DEBUG_ONLINE 			DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
#define TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE 	DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_VAD_LP_CLOSE
#define TCFG_VAD_LP_CLOSE				    DISABLE
#endif

#ifndef TCFG_KWS_HOLD_TIME
#define TCFG_KWS_HOLD_TIME			        0
#endif

/*
 *蓝牙音频能量检测使能配置
 *(1)1t2抢断播放需要使能能量检测
 *(2)蓝牙后台需要使能能量检测
 */
#ifndef TCFG_A2DP_PREEMPTED_ENABLE
#define TCFG_A2DP_PREEMPTED_ENABLE 			DISABLE_THIS_MOUDLE
#endif
#if (((TCFG_A2DP_PREEMPTED_ENABLE == 1) || (TCFG_BT_BACKGROUND_ENABLE == 1)))
#ifndef CONFIG_CPU_BR29
#define TCFG_A2DP_SBC_SILENCE_DETECT_ENABLE	1
#define TCFG_A2DP_AAC_SILENCE_DETECT_ENABLE TCFG_BT_SUPPORT_AAC
#define TCFG_A2DP_LDAC_SILENCE_DETECT_ENABLE TCFG_BT_SUPPORT_LDAC
#else
#define TCFG_A2DP_SBC_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_AAC_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_LDAC_SILENCE_DETECT_ENABLE 0
#endif
#else
#define TCFG_A2DP_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_SBC_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_AAC_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_LDAC_SILENCE_DETECT_ENABLE 0
#endif

/*
 * <<蓝牙音频播放使能控制>>
 * 比如关闭A2DP播放器，则手机播歌的时候，蓝牙数据传输是正常的，但是因为没有使能解码器，
 * 所以没有声音
 */
#define TCFG_BT_A2DP_PLAYER_ENABLE		1
#define TCFG_BT_ESCO_PLAYER_ENABLE		1
#ifdef CONFIG_FPGA_ENABLE
#define TCFG_TONE_PLAYER_ENABLE 0
#else
#define TCFG_TONE_PLAYER_ENABLE 1
#endif

#if (TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC)
#if (TCFG_AUDIO_GLOBAL_SAMPLE_RATE == 0) || (TCFG_AUDIO_GLOBAL_SAMPLE_RATE % 16000)
#error "TCFG_AUDIO_GLOBAL_SAMPLE_RATE 需要设置16000的倍数"
#endif /*TCFG_AUDIO_GLOBAL_SAMPLE_RATE*/
#endif /*TCFG_SMART_VOICE_ENABLE*/

#if (TCFG_AUDIO_SINGLE_MIC_ENABLE + TCFG_AUDIO_DUAL_MIC_ENABLE + TCFG_AUDIO_TRIPLE_MIC_ENABLE > 1)
#error "整个SDK数据流里面只能使用一种模式的CVP通话节点"
#endif

#if (TCFG_ADC_LOW_LATENCY_FRAME_SIZE > 0) && (TCFG_ADC_LOW_LATENCY_FRAME_SIZE < 256)
#if (TCFG_ADC_LOW_LATENCY_FRAME_SIZE & (TCFG_ADC_LOW_LATENCY_FRAME_SIZE - 1)) != 0
#error "TCFG_ADC_LOW_LATENCY_FRAME_SIZE 需是2的幂次方"
#endif
#define TCFG_AUDIO_DHA_AND_CALL_COEXIST_LOW_LATENCY_ENABLE 1
#define CVP_ALGO_FRAME_SIZE 256
#endif


//FM 一部分代码动态加载到ram
#define TCFG_CODE_RUN_RAM_FM_CODE            1
//BT 一部分代码加载到ram
#define TCFG_CODE_RUN_RAM_BT_CODE            0

//AAC 一部分代码加载到ram
#define TCFG_CODE_RUN_RAM_AAC_CODE           0

//AEC 一部分代码加载到ram
#define TCFG_CODE_RUN_RAM_AEC_CODE           0

#define TCFG_CODE_RUN_RAM_MIC_EFF_CODE       1

#ifndef EQ_SECTION_MAX
#define EQ_SECTION_MAX 						0xa
#endif

#ifndef TCFG_TONE_EN_ENABLE
#define TCFG_TONE_EN_ENABLE 				0x1 //
#endif
#ifndef TCFG_TONE_ZH_ENABLE
#define TCFG_TONE_ZH_ENABLE 				0x1 //
#endif

#ifndef TCFG_REC_FOLDER_NAME
#define TCFG_REC_FOLDER_NAME 				"JL_REC"
#endif
#ifndef TCFG_REC_FILE_NAME
#define TCFG_REC_FILE_NAME 					"aud_****"
#define TCFG_REC_FILE_NAME_PREFIX			"aud_" //录音文件前缀名
#endif

#define CONFIG_MEDIA_LIB_USE_MALLOC    		ENABLE


#define DAC_OBUF_OVERLAY_LP_BSS_EN			1 // dac输出buf和低功耗buf互用


//*********************************************************************************//
//                    充电中按键清除手机配对信息配置                               //
//*********************************************************************************//

#define CHARGING_CLEAN_PHONE_INFO       0


//*********************************************************************************//
//                                 PC配置                                          //
//*********************************************************************************//

#ifndef TCFG_PC_ENABLE
#define TCFG_PC_ENABLE       				TCFG_APP_PC_EN
#endif

#define USB_PC_NO_APP_MODE                  0


#include "audio_config_def.h"

#include "usb_std_class_def.h"
#if TCFG_CFG_TOOL_ENABLE
#if (TCFG_COMM_TYPE == TCFG_USB_COMM)
#undef TCFG_USB_CDC_BACKGROUND_RUN
#define TCFG_USB_CDC_BACKGROUND_RUN         ENABLE
#endif
#if (TCFG_COMM_TYPE == TCFG_UART_COMM)
#undef TCFG_USB_CDC_BACKGROUND_RUN
#define TCFG_USB_CDC_BACKGROUND_RUN         DISABLE
#endif
#endif
#include "usb_common_def.h"


//*********************************************************************************//
//                                 调音工具                                        //
//*********************************************************************************//
#define TCFG_ONLINE_ENABLE                  TCFG_CFG_TOOL_ENABLE    //是否支持音效在线调试功能

/*spp数据导出配置*/
#if ((TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_SPP) || TCFG_AUDIO_MIC_DUT_ENABLE)
#undef TCFG_USER_TWS_ENABLE
#undef TCFG_USER_BLE_ENABLE
#undef TCFG_BD_NUM
#undef TCFG_BT_SUPPORT_SPP
#undef TCFG_BT_SUPPORT_A2DP
#undef APP_ONLINE_DEBUG
#define TCFG_USER_TWS_ENABLE        0//spp数据导出，关闭tws
#define TCFG_USER_BLE_ENABLE        0//spp数据导出，关闭ble
#define TCFG_BD_NUM					1//连接设备个数配置
#define TCFG_BT_SUPPORT_SPP	1
#define TCFG_BT_SUPPORT_A2DP   0
#define APP_ONLINE_DEBUG            1//通过spp导出数据
#endif/*TCFG_AUDIO_DATA_EXPORT_DEFINE*/

#if ( \
        TCFG_AEC_TOOL_ONLINE_ENABLE                                     || \
        TCFG_AUDIO_DUT_ENABLE   	                                    || \
        TCFG_ANC_TOOL_DEBUG_ONLINE                                      || \
        TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE                                || \
        (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_SPP_COMM))     || \
        (defined(TCFG_SPEAKER_EQ_NODE_ENABLE)&& TCFG_SPEAKER_EQ_NODE_ENABLE)\
    )
#undef TCFG_BT_SUPPORT_SPP
#undef APP_ONLINE_DEBUG
#define TCFG_BT_SUPPORT_SPP	1
#define APP_ONLINE_DEBUG            1
#endif

#if TCFG_CFG_TOOL_ENABLE
#undef EQ_SECTION_MAX
#define EQ_SECTION_MAX 32
#endif

#ifndef EQ_SECTION_MAX
#if TCFG_EQ_ENABLE
#define EQ_SECTION_MAX 32
#endif
#endif

#define  MEDIA_UNIFICATION_EN  1 //音乐播放器统一使用一个流程框图(媒体)

#define TCFG_USER_RSSI_TEST_EN   0   //通过spp获取耳机RSSI值，需要使能USER_SUPPORT_PROFILE_SPP

//*********************************************************************************//
//                               UI配置                                            //
//*********************************************************************************//
#if (TCFG_UI_ENABLE && (CONFIG_JL_UI_ENABLE || CONFIG_LVGL_UI_ENABLE) &&TCFG_LCD_BUF_IN_STATIC_RAM_ENABLE)
// 行缓存使用静态BUF
#ifdef CONFIG_BOARD_JL707N_CSC_DEMO    //彩屏仓屏幕配置
#define CONFIG_LCD_BUF_STATIC_RAM_LEN 		(LCD_WIDTH * 2 * 32 * 2) // 屏幕宽度 * RGB565 * 显存行数 * 双BUF
#else
#define CONFIG_LCD_BUF_STATIC_RAM_LEN 		(LCD_WIDTH * 2 * 16 * 2) // 屏幕宽度 * RGB565 * 显存行数 * 双BUF
#endif
#endif

//*********************************************************************************//
//                               default配置                                       //
//*********************************************************************************//
#include "ui_app_def.h"
#include "audio_app_def.h"
#include "macro_default.h"

// PMC 不使能不可使用蓝牙
#if (PMC_CPU_CORE_NUM == 0) && (TCFG_APP_BT_EN)
#undef TCFG_APP_BT_EN
#define TCFG_APP_BT_EN                            0                // 蓝牙模式
#endif

//*********************************************************************************//
//                               错误判断配置                                       //
//*********************************************************************************//
// PMC 不使能，不可使用蓝牙
#if (PMC_CPU_CORE_NUM == 0) && (TCFG_APP_BT_EN)
#error "pmc num is 0, nonsupport BT"
#endif


#endif
