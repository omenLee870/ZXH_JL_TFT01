#ifndef MACRO_DEFAULT_H
#define MACRO_DEFAULT_H

//*********************************************************************************//
//						        CACHE CONFIG                                       //
//*********************************************************************************//
#ifndef TCFG_FREE_ICACHE_WAY
#define TCFG_FREE_ICACHE_WAY					0
#endif
#ifndef TCFG_FREE_DCACHE_WAY
#define TCFG_FREE_DCACHE_WAY					0
#endif
#ifndef TCFG_ICACHE_RUN_DATA_CODE
#define TCFG_ICACHE_RUN_DATA_CODE				DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_ICACHE_RUN_BT_STATIC_RAM
#define TCFG_ICACHE_RUN_BT_STATIC_RAM			DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_DCACHE_RUN_GPU_BUF
#define TCFG_DCACHE_RUN_GPU_BUF					DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_DCACHE_RUN_BT_STATIC_RAM
#define TCFG_DCACHE_RUN_BT_STATIC_RAM			DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_ICACHE_RUN_GPU_BUF
#define TCFG_ICACHE_RUN_GPU_BUF					DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_ICACHE_DYNAMIC_SWITCH
#define TCFG_ICACHE_DYNAMIC_SWITCH				DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_ICACHE_RUN_FTL_BUF
#define TCFG_ICACHE_RUN_FTL_BUF					DISABLE_THIS_MOUDLE
#endif
//*********************************************************************************//
//						        KEY CONFIG                                         //
//*********************************************************************************//
#ifndef MULT_KEY_ENABLE
#define MULT_KEY_ENABLE							DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_CTMU_TOUCH_KEY_ENABLE
#define TCFG_CTMU_TOUCH_KEY_ENABLE          	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_TOUCH_KEY_ENABLE
#define TCFG_TOUCH_KEY_ENABLE               	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_UART_KEY_ENABLE
#define TCFG_UART_KEY_ENABLE					DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_RDEC_KEY_ENABLE
#define TCFG_RDEC_KEY_ENABLE					DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_ADKEY_RTCVDD_ENABLE
#define TCFG_ADKEY_RTCVDD_ENABLE             	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_EAR_DETECT_ENABLE
#define TCFG_EAR_DETECT_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_KEY_TONE_EN
#define TCFG_KEY_TONE_EN						DISABLE_THIS_MOUDLE
#endif

#ifndef MOUSE_KEY_SCAN_MODE
#define MOUSE_KEY_SCAN_MODE						DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_LP_TOUCH_KEY_ENABLE
#define TCFG_LP_TOUCH_KEY_ENABLE 				DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//						        SD CONFIG                                          //
//*********************************************************************************//
#ifndef TCFG_SD0_ENABLE
#define TCFG_SD0_ENABLE							DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_SD1_ENABLE
#define TCFG_SD1_ENABLE							DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_IO_MULTIPLEX_WITH_SD
#define TCFG_IO_MULTIPLEX_WITH_SD               DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_SD0_SD1_USE_THE_SAME_HW
#define TCFG_SD0_SD1_USE_THE_SAME_HW            DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//						        USB CONFIG                                         //
//*********************************************************************************//
#ifndef TCFG_USB_SLAVE_USER_HID
#define TCFG_USB_SLAVE_USER_HID					DISABLE_THIS_MOUDLE
#endif
#ifndef USB_EP_PROTECT
#define USB_EP_PROTECT							DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//						        FLASH CONFIG                                       //
//*********************************************************************************//
#ifndef FLASH_INSIDE_REC_ENABLE
#define FLASH_INSIDE_REC_ENABLE             	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_NOR_FAT
#define TCFG_NOR_FAT                        	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_NOR_FS
#define TCFG_NOR_FS                         	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_NOR_VM
#define TCFG_NOR_VM								DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_NOR_REC
#define TCFG_NOR_REC                        	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_VIR_UDISK_ENABLE
#define TCFG_VIR_UDISK_ENABLE					DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_VIRFAT_FLASH_ENABLE
#define TCFG_VIRFAT_FLASH_ENABLE				DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_NANDFLASH_DEV_ENABLE
#define TCFG_NANDFLASH_DEV_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_NORFLASH_SFC_DEV_ENABLE
#define TCFG_NORFLASH_SFC_DEV_ENABLE 			DISABLE_THIS_MOUDLE

#endif
#ifndef TCFG_UI_RES_USE_FAT_FTL_ENABLE
#define TCFG_UI_RES_USE_FAT_FTL_ENABLE 			DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_NORFLASH_DEV_ENABLE
#define TCFG_NORFLASH_DEV_ENABLE            DISABLE
#endif

#ifndef TCFG_NANDFLASH_UI_FAT_ENABLE
#define TCFG_NANDFLASH_UI_FAT_ENABLE 			DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_NANDFLASH_UI_FAT_LOGO
#define TCFG_NANDFLASH_UI_FAT_LOGO				"UI_FAT"
#endif
//*********************************************************************************//
//						        PSRAM CONFIG                                       //
//*********************************************************************************//
#ifndef TCFG_PSRAM_DEV_ENABLE
#define TCFG_PSRAM_DEV_ENABLE					DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_PSRAM_UI_EFFECT
#define TCFG_PSRAM_UI_EFFECT					DISABLE_THIS_MOUDLE
#endif


//*********************************************************************************//
//						        DEV CONFIG                                         //
//*********************************************************************************//
#ifndef TCFG_RECORD_FOLDER_DEV_ENABLE
#define TCFG_RECORD_FOLDER_DEV_ENABLE          	DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//						        DRIVER CONFIG                                      //
//*********************************************************************************//
#ifndef TCFG_UART0_ENABLE
#define TCFG_UART0_ENABLE 						DISABLE_THIS_MOUDLE
#endif

#ifndef NTC_DET_EN
#define NTC_DET_EN								DISABLE_THIS_MOUDLE
#define NTC_POWER_IO    	IO_PORTC_03
#define NTC_DETECT_IO   	IO_PORTC_04
#define NTC_DET_AD_CH   	(0x4)   //根据adc_api.h修改通道号
#define NTC_DET_UPPER       235  //正常范围AD值上限，0度时
#define NTC_DET_LOWER       34  //正常范围AD值下限，45度时
#endif

#ifndef TCFG_BACKLIGHT_PWM_MODE
#define TCFG_BACKLIGHT_PWM_MODE 				0
#endif

#ifndef TCFG_GX8002_NPU_ENABLE
#define TCFG_GX8002_NPU_ENABLE                  DISABLE_THIS_MOUDLE
#endif
//*********************************************************************************//
//						        CHARGE CONFIG                                      //
//*********************************************************************************//
#ifndef TCFG_CHARGESTORE_ENABLE
#define TCFG_CHARGESTORE_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_CHARGE_BOX_ENABLE
#define TCFG_CHARGE_BOX_ENABLE                  DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_CHARGE_OFF_POWERON_EN
#define TCFG_CHARGE_OFF_POWERON_EN              DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_CHARGE_BOX_UI_ENABLE
#define TCFG_CHARGE_BOX_UI_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_CHARGE_MOUDLE_OUTSIDE
#define TCFG_CHARGE_MOUDLE_OUTSIDE 				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_SHORT_PROTECT_ENABLE
#define TCFG_SHORT_PROTECT_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_WIRELESS_ENABLE
#define TCFG_WIRELESS_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_LDO_DET_ENABLE
#define TCFG_LDO_DET_ENABLE						DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_CURRENT_LIMIT_ENABLE
#define TCFG_CURRENT_LIMIT_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_TEMPERATURE_ENABLE
#define TCFG_TEMPERATURE_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_HANDSHAKE_ENABLE
#define TCFG_HANDSHAKE_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_USB_KEY_UPDATE_ENABLE
#define TCFG_USB_KEY_UPDATE_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_USB_KEY_UPDATE_ENABLE
#define TCFG_USB_KEY_UPDATE_ENABLE				DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//						        OTA CONFIG                                         //
//*********************************************************************************//
#ifndef CONFIG_UPDATE_JUMP_TO_MASK
#define CONFIG_UPDATE_JUMP_TO_MASK				DISABLE_THIS_MOUDLE
#endif

#ifndef CONFIG_REUSABLE_RESERVE
#define CONFIG_REUSABLE_RESERVE					DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//						        PRODUCT CONFIG                                     //
//*********************************************************************************//
#ifndef PRODUCT_TEST_ENABLE
#define PRODUCT_TEST_ENABLE						DISABLE_THIS_MOUDLE
#endif


//*********************************************************************************//
//						        MODE & USER CONFIG                                 //
//*********************************************************************************//
#ifndef TCFG_APP_RTC_EN
#define TCFG_APP_RTC_EN							DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_APP_RECORD_EN
#define TCFG_APP_RECORD_EN						DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_APP_FM_EMITTER_EN
#define TCFG_APP_FM_EMITTER_EN                  DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_APP_MUSIC_EN
#define TCFG_APP_MUSIC_EN						DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_APP_PC_EN
#define TCFG_APP_PC_EN							DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_APP_LINEIN_EN
#define TCFG_APP_LINEIN_EN 						DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_APP_FM_EN
#define TCFG_APP_FM_EN							DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_FM_INSIDE_ENABLE
#define TCFG_FM_INSIDE_ENABLE				    DISABLE_THIS_MOUDLE
#endif

#ifndef SOUNDCARD_ENABLE
#define SOUNDCARD_ENABLE				        DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_DATA_STORAGE_ENABLE
#define TCFG_DATA_STORAGE_ENABLE		    	DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_AUDIO_LINEIN_ENABLE
#define TCFG_AUDIO_LINEIN_ENABLE   				TCFG_APP_LINEIN_EN
#endif
#ifndef TCFG_AUDIO_FM_ENABLE
#define TCFG_AUDIO_FM_ENABLE     				TCFG_APP_FM_EN
#endif
#ifndef TCFG_APP_MUSIC_EN
#define TCFG_APP_MUSIC_EN						TCFG_APP_MUSIC_EN
#endif

//*********************************************************************************//
//                                  pay 配置                                       //
//*********************************************************************************//
#ifndef TCFG_PAY_ALIOS_ENABLE
#define TCFG_PAY_ALIOS_ENABLE				0
#endif

#ifndef TCFG_PAY_TRANSITCODE_ENABLE
#define TCFG_PAY_TRANSITCODE_ENABLE			0
#endif

#ifndef TCFG_PAY_ALIOS_WAY_T_HEAD
#define TCFG_PAY_ALIOS_WAY_T_HEAD			1
#endif

#ifndef TCFG_PAY_ALIOS_WAY_SEL
#define TCFG_PAY_ALIOS_WAY_SEL				0
#endif

#ifndef TCFG_PAY_ALIOS_PRODUCT_MODEL
#define TCFG_PAY_ALIOS_PRODUCT_MODEL		" "
#endif

#ifndef ALIPAY_SE_USE_RESET_PIN
#define ALIPAY_SE_USE_RESET_PIN             0
#endif

//*********************************************************************************//
//						        UI CONFIG                                         //
//*********************************************************************************//
#ifndef TCFG_LCD_QSPI_RM69330_454x454_ENABLE
#define TCFG_LCD_QSPI_RM69330_454x454_ENABLE         	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_LCD_QSPI_SH8601A_454x454_ENABLE
#define TCFG_LCD_QSPI_SH8601A_454x454_ENABLE         	DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_LCD_SPI_ST7789V_ENABLE
#define TCFG_LCD_SPI_ST7789V_ENABLE				DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_LCD_SPI_ST7789_BOE1_54_ENABLE
#define TCFG_LCD_SPI_ST7789_BOE1_54_ENABLE  	DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_SIMPLE_LCD_ENABLE
#define TCFG_SIMPLE_LCD_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_TP_SLEEP_EN
#define TCFG_TP_SLEEP_EN						DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_LUA_ENABLE
#define TCFG_LUA_ENABLE							DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_UI_SHUT_DOWN_TIME
#define TCFG_UI_SHUT_DOWN_TIME					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_TP_BL6133_ENABLE
#define TCFG_TP_BL6133_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef LCD_POWER_DOWN_EN
#define LCD_POWER_DOWN_EN  						DISABLE_THIS_MOUDLE
#endif

#ifndef TP_POWER_DOWN_EN
#define TP_POWER_DOWN_EN  						DISABLE_THIS_MOUDLE
#endif

#ifndef CONFIG_LCD_BUF_STATIC_RAM_LEN
#define CONFIG_LCD_BUF_STATIC_RAM_LEN			0
#endif


//*********************************************************************************//
//						        第三方CONFIG                                       //
//*********************************************************************************//
#ifndef TCFG_PAY_ALIOS_ENABLE
#define TCFG_PAY_ALIOS_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_PAY_TRANSITCODE_ENABLE
#define TCFG_PAY_TRANSITCODE_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_LP_NFC_TAG_ENABLE
#define TCFG_LP_NFC_TAG_ENABLE 					DISABLE_THIS_MOUDLE
#endif

#ifndef USE_DMA_TONE
#define USE_DMA_TONE							DISABLE_THIS_MOUDLE
#endif

#ifndef AI_APP_PROTOCOL
#define AI_APP_PROTOCOL							DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_AI_INTERACTION_ENABLE
#define TCFG_AI_INTERACTION_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_IFLYTEK_ENABLE
#define TCFG_IFLYTEK_ENABLE						DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_APP_CAT1_EN
#define TCFG_APP_CAT1_EN						DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_CAT1_AICXTEK_ENABLE
#define TCFG_CAT1_AICXTEK_ENABLE 				DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_CAT1_UNISOC_ENABLE
#define TCFG_CAT1_UNISOC_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef USE_DMA_UART_TEST
#define USE_DMA_UART_TEST						DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//                              sensor CONFIG                                      //
//*********************************************************************************//
#ifndef TCFG_GSENSOR_ENABLE
#define TCFG_GSENSOR_ENABLE                		DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_HR_SENSOR_ENABLE
#define TCFG_HR_SENSOR_ENABLE                   DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_SPO2_SENSOR_ENABLE
#define TCFG_SPO2_SENSOR_ENABLE					DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_IRSENSOR_ENABLE
#define TCFG_IRSENSOR_ENABLE                  	DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_JSA1221_ENABLE
#define TCFG_JSA1221_ENABLE 					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_IMUSENSOR_ENABLE
#define TCFG_IMUSENSOR_ENABLE					DISABLE_THIS_MOUDLE
#endif

/* PMC(小核) 算法总开关及 P11 侧运动健康算法/佩戴检测开关，未定义时默认关闭 */
#ifndef TCFG_SPORT_HEALTH_PMC_ALGO
#define TCFG_SPORT_HEALTH_PMC_ALGO				DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_SPORT_HEALTH_P11_ALGO_GSENSOR
#define TCFG_SPORT_HEALTH_P11_ALGO_GSENSOR		TCFG_SPORT_HEALTH_PMC_ALGO
#endif
#ifndef TCFG_SPORT_HEALTH_P11_ALGO_HEART_RATE
#define TCFG_SPORT_HEALTH_P11_ALGO_HEART_RATE	TCFG_SPORT_HEALTH_PMC_ALGO
#endif
#ifndef TCFG_SPORT_HEALTH_P11_ALGO_SPO2
#define TCFG_SPORT_HEALTH_P11_ALGO_SPO2			TCFG_SPORT_HEALTH_PMC_ALGO
#endif
#ifndef TCFG_SPORT_HEALTH_P11_DET_WEAR
#define TCFG_SPORT_HEALTH_P11_DET_WEAR			TCFG_SPORT_HEALTH_PMC_ALGO
#endif
#ifndef TCFG_SPORT_HEALTH_DET_WEAR
#define TCFG_SPORT_HEALTH_DET_WEAR				DISABLE_THIS_MOUDLE
#endif
/* 关机前保存 sensor 数据：默认打开。实际生效还须 TCFG_SPORT_HEALTH_ENABLE 打开，
 * 两条件都满足才会在关机路径请求各模块落盘（见 mode/idle/idle.c）。 */
#ifndef TCFG_SPORT_HEALTH_SAVE_ON_POWEROFF
#define TCFG_SPORT_HEALTH_SAVE_ON_POWEROFF		ENABLE
#endif
/* pmc 算法数据更新节奏 ms，默认 1s */
#ifndef TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS
#define TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS		1000
#endif

//*********************************************************************************//
//						        BT CONFIG                                          //
//*********************************************************************************//
#ifndef CONFIG_DOUBLE_BANK_ENABLE
#define CONFIG_DOUBLE_BANK_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_BT_BLE_ADV_ENABLE
#define TCFG_BT_BLE_ADV_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_USER_EMITTER_ENABLE
#define TCFG_USER_EMITTER_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef USER_SUPPORT_PROFILE_PBAP
#define USER_SUPPORT_PROFILE_PBAP				DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_BT_SUPPORT_MAP
#define TCFG_BT_SUPPORT_MAP						DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_USER_BT_CLASSIC_ENABLE
#define TCFG_USER_BT_CLASSIC_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef SMART_BOX_EN
#define SMART_BOX_EN							DISABLE_THIS_MOUDLE
#endif

#ifndef FINDMY_EN
#define FINDMY_EN								DISABLE_THIS_MOUDLE
#endif

#ifndef LEA_BIG_CTRLER_TX_EN
#define LEA_BIG_CTRLER_TX_EN					DISABLE_THIS_MOUDLE
#endif
#ifndef LEA_BIG_CTRLER_RX_EN
#define LEA_BIG_CTRLER_RX_EN					DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_EARPHONE_PROTOCOL
#define TCFG_EARPHONE_PROTOCOL				    DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_BLE_DEMO_SELECT
#define TCFG_BLE_DEMO_SELECT 					DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_MULTI_BLE_EN
#define RCSP_MULTI_BLE_EN                  		DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_BTMATE_EN
#define RCSP_BTMATE_EN      					DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_DEVICE_STATUS_ENABLE
#define RCSP_DEVICE_STATUS_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_BT_CONTROL_ENABLE
#define RCSP_BT_CONTROL_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_EN
#define RCSP_ADV_EN         					DISABLE_THIS_MOUDLE
#endif
#ifndef RCSP_ADV_ANC_VOICE
#define RCSP_ADV_ANC_VOICE     					DISABLE_THIS_MOUDLE
#endif

#ifndef UPDATE_MD5_ENABLE
#define UPDATE_MD5_ENABLE            			DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_NAME_SET_ENABLE
#define RCSP_ADV_NAME_SET_ENABLE                DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_KEY_SET_ENABLE
#define RCSP_ADV_KEY_SET_ENABLE                 DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_LED_SET_ENABLE
#define RCSP_ADV_LED_SET_ENABLE                	DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_MIC_SET_ENABLE
#define RCSP_ADV_MIC_SET_ENABLE                 DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_WORK_SET_ENABLE
#define RCSP_ADV_WORK_SET_ENABLE                DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_HIGH_LOW_SET
#define RCSP_ADV_HIGH_LOW_SET                   DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_MUSIC_INFO_ENABLE
#define RCSP_ADV_MUSIC_INFO_ENABLE              DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_KARAOKE_SET_ENABLE
#define RCSP_ADV_KARAOKE_SET_ENABLE             DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_PRODUCT_MSG_ENABLE
#define RCSP_ADV_PRODUCT_MSG_ENABLE        		DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_ASSISTED_HEARING
#define RCSP_ADV_ASSISTED_HEARING				DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_AI_NO_PICK
#define RCSP_ADV_AI_NO_PICK						DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_SCENE_NOISE_REDUCTION
#define RCSP_ADV_SCENE_NOISE_REDUCTION			DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_WIND_NOISE_DETECTION
#define RCSP_ADV_WIND_NOISE_DETECTION			DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_VOICE_ENHANCEMENT_MODE
#define RCSP_ADV_VOICE_ENHANCEMENT_MODE			DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_COLOR_LED_SET_ENABLE
#define RCSP_ADV_COLOR_LED_SET_ENABLE   		DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_KARAOKE_EQ_SET_ENABLE
#define RCSP_ADV_KARAOKE_EQ_SET_ENABLE			DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_EQ_SET_ENABLE
#define RCSP_ADV_EQ_SET_ENABLE          		DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_FIND_DEVICE_ENABLE
#define RCSP_ADV_FIND_DEVICE_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_FILE_OPT
#define RCSP_FILE_OPT       					DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_UPDATE_EN
#define RCSP_UPDATE_EN                  		DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_MULTI_BLE_MASTER_NUMS
#define RCSP_MULTI_BLE_MASTER_NUMS				DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_MULTI_BLE_SLAVE_NUMS
#define RCSP_MULTI_BLE_SLAVE_NUMS				DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_APP_MUSIC_EN
#define RCSP_APP_MUSIC_EN						DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_APP_RTC_EN
#define RCSP_APP_RTC_EN							DISABLE_THIS_MOUDLE
#endif

#ifndef JL_RCSP_EXTRA_FLASH_OPT
#define JL_RCSP_EXTRA_FLASH_OPT					DISABLE_THIS_MOUDLE
#endif

#ifndef WATCH_FILE_TO_FLASH
#define WATCH_FILE_TO_FLASH						DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_ADV_ADAPTIVE_NOISE_REDUCTION
#define RCSP_ADV_ADAPTIVE_NOISE_REDUCTION 		DISABLE_THIS_MOUDLE
#endif

#ifndef JL_RCSP_SIMPLE_TRANSFER
#define JL_RCSP_SIMPLE_TRANSFER					DISABLE_THIS_MOUDLE
#endif

#ifndef JL_RCSP_SENSORS_DATA_OPT
#define JL_RCSP_SENSORS_DATA_OPT				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_USER_BLE_CTRL_BREDR_EN
#define TCFG_USER_BLE_CTRL_BREDR_EN				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_CONNECTED_ENABLE
#define TCFG_CONNECTED_ENABLE			    	DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_BROADCAST_ENABLE
#define TCFG_BROADCAST_ENABLE			    	DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_BLE_AUDIO_TEST_EN
#define TCFG_BLE_AUDIO_TEST_EN			    	DISABLE_THIS_MOUDLE
#endif

#ifndef PRINT_DMA_DATA_EN
#define PRINT_DMA_DATA_EN						DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_MULTI_BLE_SLAVE_N
#define RCSP_MULTI_BLE_SLAVE_N					DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_BLE_MASTER
#define RCSP_BLE_MASTER							DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_BLE_CLIENT_EN
#define RCSP_BLE_CLIENT_EN 						DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_BT_VOL_SYNC_ENABLE
#define TCFG_BT_VOL_SYNC_ENABLE					DISABLE_THIS_MOUDLE
#endif

#ifndef BT_SUPPORT_MUSIC_VOL_SYNC
#define BT_SUPPORT_MUSIC_VOL_SYNC					DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_REVERBERATION_SETTING
#define RCSP_REVERBERATION_SETTING				DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_DRC_VAL_SETTING
#define RCSP_DRC_VAL_SETTING					DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_KARAOKE_SOUND_EFFECT
#define RCSP_KARAOKE_SOUND_EFFECT				DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_KARAOKE_ATMOSPHERE
#define RCSP_KARAOKE_ATMOSPHERE					DISABLE_THIS_MOUDLE
#endif

#ifndef RCSP_KARAOKE_SOUND_PARAM
#define RCSP_KARAOKE_SOUND_PARAM				DISABLE_THIS_MOUDLE
#endif


#ifndef USER_SUPPORT_DUAL_A2DP_SOURCE
#define USER_SUPPORT_DUAL_A2DP_SOURCE           DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_WIRELESS_MIC_ENABLE
#define TCFG_WIRELESS_MIC_ENABLE				DISABLE_THIS_MOUDLE
#endif

#ifndef OTA_TWS_SAME_TIME_NEW
#define OTA_TWS_SAME_TIME_NEW				    DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_BT_BLE_BREDR_SAME_ADDR
#define TCFG_BT_BLE_BREDR_SAME_ADDR			    DISABLE_THIS_MOUDLE
#endif

#ifndef DOUBLE_BT_SAME_MAC
#define DOUBLE_BT_SAME_MAC			            DISABLE_THIS_MOUDLE
#endif

#ifndef DOUBLE_BT_SAME_NAME
#define DOUBLE_BT_SAME_NAME						DISABLE_THIS_MOUDLE
#endif

#ifndef BT_CTKD_CONN_SPEED
#define BT_CTKD_CONN_SPEED          			DISABLE_THIS_MOUDLE
#endif

#ifndef CONFIG_DISPLAY_DETAIL_BAT
#define CONFIG_DISPLAY_DETAIL_BAT			    DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_COLORLED_ENABLE
#define TCFG_COLORLED_ENABLE			        DISABLE_THIS_MOUDLE
#endif


#ifndef CONFIG_NO_DISPLAY_BUTTON_ICON
#define CONFIG_NO_DISPLAY_BUTTON_ICON           1 //BLE广播不显示按键界面,智能充电仓置1
#endif //CONFIG_NO_DISPLAY_BUTTON_ICON

#ifndef USER_SUPPORT_PROFILE_PAN
#define USER_SUPPORT_PROFILE_PAN				0
#endif

#ifndef TCFG_GATT_OVER_EDR_EN
#define TCFG_GATT_OVER_EDR_EN			        DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_USER_SOUNDBAR_ENABLE
#define TCFG_USER_SOUNDBAR_ENABLE  				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_USER_SOUNDBOX_GLOBAL_ENABLE
#define TCFG_USER_SOUNDBOX_GLOBAL_ENABLE  		DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_BT_DONGLE_ENABLE
#define TCFG_BT_DONGLE_ENABLE   				DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_BT_DUAL_CONN_ENABLE
#define TCFG_BT_DUAL_CONN_ENABLE 				DISABLE_THIS_MOUDLE//一拖二
#endif

#ifndef TCFG_TWS_PAIR_BY_BOTH_SIDES
#define TCFG_TWS_PAIR_BY_BOTH_SIDES 			0x0 //两边同时按配对键进入配对
#endif


#ifndef TCFG_DUAL_CONN_PAGE_SCAN_TIME
#define TCFG_DUAL_CONN_PAGE_SCAN_TIME 			0x0 //等待第二台连接时间(s)
#endif


#ifndef TCFG_TWS_POWER_BALANCE_ENABLE
#define TCFG_TWS_POWER_BALANCE_ENABLE 			0x0 //主从电量平衡
#endif

#ifndef CONFIG_TWS_POWEROFF_SAME_TIME
#define CONFIG_TWS_POWEROFF_SAME_TIME 			0x0 //同步关机
#endif

#ifndef TCFG_BT_PA_ENABLE
#define TCFG_BT_PA_ENABLE 				        DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_CLOCK_OSC_CAP
#define TCFG_CLOCK_OSC_CAP                      7
#endif

#ifndef TCFG_CLOCK_OSC_CAP_FINE
#define TCFG_CLOCK_OSC_CAP_FINE                 3
#endif
//*********************************************************************************//
//                               power CONFIG                                      //
//*********************************************************************************//
#ifndef TCFG_VBAT_TRIM_EN
#define TCFG_VBAT_TRIM_EN						DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_BATTER_OFFSET_EN
#define TCFG_BATTER_OFFSET_EN					DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//                              彩屏仓 CONFIG                                      //
//*********************************************************************************//

#ifndef TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
#define TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE			       DISABLE_THIS_MOUDLE
#endif

#ifndef TCFG_CHARGE_BOX_ENABLE
#define TCFG_CHARGE_BOX_ENABLE			                       DISABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//						        VIDEO CONFIG                                       //
//*********************************************************************************//
#ifndef TCFG_APP_VIDEO_EN
#define TCFG_APP_VIDEO_EN	    								DISABLE
#endif
#ifndef TCFG_APP_VIDEO_REC_EN
#define TCFG_APP_VIDEO_REC_EN                                DISABLE
#endif
#ifndef TCFG_VIDEO_DIAL_ENABLE
#define TCFG_VIDEO_DIAL_ENABLE 									DISABLE
#endif
#ifndef TCFG_APP_VIDEO_DEC_EN
#define TCFG_APP_VIDEO_DEC_EN                                DISABLE
#endif
#ifndef TCFG_APP_PHOTO_EN
#define TCFG_APP_PHOTO_EN                                    DISABLE
#endif
#ifndef UVC_JPG_DATA_WRITE2SD
#define UVC_JPG_DATA_WRITE2SD       							DISABLE
#endif
#ifndef TCFG_CAMERA_MANAGER_ENABLE
#define TCFG_CAMERA_MANAGER_ENABLE								DISABLE
#endif
#ifndef TCFG_CAMERA_DEV_BF30A2
#define TCFG_CAMERA_DEV_BF30A2									DISABLE
#endif
#ifndef TCFG_VIDEO0_ENABLE
#define TCFG_VIDEO0_ENABLE										DISABLE
#endif
#ifndef TCFG_CAMERA_SPI_ENABLE
#define TCFG_CAMERA_SPI_ENABLE                    				DISABLE
#endif
#ifndef TCFG_CAMERA_DVP_ENABLE
#define TCFG_CAMERA_DVP_ENABLE									DISABLE
#endif
#ifndef TCFG_CAMERA_SPI_POWER_IO_CTRL
#define TCFG_CAMERA_SPI_POWER_IO_CTRL					1		// SPI摄像头VCC有效电平: 1=高电平有效
#endif
#ifndef TCFG_CAMERA_DVP_POWER_IO_CTRL
#define TCFG_CAMERA_DVP_POWER_IO_CTRL					1		// DVP摄像头VCC有效电平: 1=高电平有效
#endif
#ifndef TCFG_VSTREAM_TRANSFER
#define TCFG_VSTREAM_TRANSFER									DISABLE
#endif

//*********************************************************************************//
//						        NET CONFIG                                         //
//*********************************************************************************//

#ifndef MBEDTLS_SELF_TEST
#define MBEDTLS_SELF_TEST   						DISABLE
#endif
#ifndef MBEDTLS_AES_SELF_TEST
#define MBEDTLS_AES_SELF_TEST   					DISABLE
#endif
#ifndef TCFG_SELF_WIFI_ENABLE
#define TCFG_SELF_WIFI_ENABLE						DISABLE
#endif
#ifndef TCFG_EXT_WIFI_ENABLE
#define TCFG_EXT_WIFI_ENABLE						DISABLE
#endif


#endif //MACRO_DEFAULT_H
