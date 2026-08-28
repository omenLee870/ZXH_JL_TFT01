#ifndef _USER_CFG_ID_H_
#define _USER_CFG_ID_H_

//=================================================================================//
//                            与APP CASE相关配置项[1 ~ 50]                         //
//=================================================================================//
#define 	CFG_EARTCH_ENABLE_ID	        1
#define 	CFG_PBG_MODE_INFO		        2
#define     VM_FM_INFO				        3
#define    	CFG_SCENE_INDEX                 4
#define     VM_LP_NFC_TAG_BUF_DATA          5
#define     CFG_SPK_EQ_SEG_SAVE             6
#define     CFG_SPK_EQ_GLOBAL_GAIN_SAVE     7
#define 	CFG_BCS_MAP_WEIGHT	    	    8
#define     ADV_SEQ_RAND                    9
#define 	CFG_HAVE_MASS_STORAGE           10
#define     CFG_MUSIC_MODE                  11
#define		LP_KEY_EARTCH_TRIM_VALUE        12

#define     CFG_RCSP_ADV_ANC_VOICE          13
#define     CFG_RCSP_ADV_ANC_VOICE_MODE     14
#define     CFG_RCSP_ADV_ANC_VOICE_KEY      15
#define     CFG_VOLUME_ENHANCEMENT_MODE     16
#define 	CFG_UI_SYS_INFO		            17
#define     CFG_DMS_MALFUNC_STATE_ID        18//dms故障麦克风检测默认使用哪个mic的参数id
#define     CFG_EQ0_INDEX                   19
#define     CFG_MIC_EFF_VOLUME_INDEX        20
#define 	VM_WATCH_SELECT				    21
#define 	VM_UI_SYS_INFO				    22
#define     VM_UI_FILE_LIST				    23
#define     VM_RESET_EX_FLASH_FLAG          24



#define     VM_ALARM_0                  	25
#define     VM_ALARM_1                  	26
#define     VM_ALARM_2                      27
#define     VM_ALARM_3                      28
#define     VM_ALARM_4                      29
#define     VM_ALARM_SNOOZE                 30
#define     VM_ALARM_MASK                   31
#define     VM_ALARM_NAME_0                 32
#define     VM_ALARM_NAME_1                 33
#define     VM_ALARM_NAME_2                 34
#define     VM_ALARM_NAME_3                 35
#define     VM_ALARM_NAME_4                 36

#define     VM_CHARGE_PROGI_VOLT            37	//保留的progi口的恒流时候电压

// findmy
#define     CFG_FMNA_BLE_ADDRESS_INFO       38
#define     CFG_FMNA_SOFTWARE_AUTH_START    39
#define     CFG_FMNA_SOFTWARE_AUTH_END      (CFG_FMNA_SOFTWARE_AUTH_START + 4)
#define     CFG_FMNA_SOFTWARE_AUTH_FLAG     44
#define     CFG_FMY_INFO                    45

#define     PT_TEST_RESULT             		46//

#define     CFG_VBG_TRIM                    47	//保存VBG配置参数id

#define     CFG_DIAL_TYPE_SEL                    48	//保存VBG配置参数id
#define     CFG_DUER_ACCESS_TOKEN                    49	//小度token

#define     CFG_LE_AUDIO_EN             	50//

//#MAX 50
//=================================================================================//
//            	SDK配置拓展配置项 使用 [145 ~200]        	                       //
//=================================================================================//
#define		CFG_RCSP_ADV_HIGH_LOW_VOL		 		145
#define     CFG_RCSP_ADV_EQ_MODE_SETTING     		146
#define     CFG_RCSP_ADV_EQ_DATA_SETTING     		147
#define     CFG_RCSP_ADV_TIME_STAMP          		148
#define     CFG_RCSP_ADV_WORK_SETTING        		149
#define     CFG_RCSP_ADV_MIC_SETTING         		150
#define     CFG_RCSP_ADV_LED_SETTING         		151
#define     CFG_RCSP_ADV_KEY_SETTING         		152
#define     CFG_RCSP_MISC_DRC_SETTING        		153
#define     CFG_RCSP_MISC_REVERB_ON_OFF      		154
#define     VM_ALARM_RING_NAME_0             		155
#define     VM_ALARM_RING_NAME_1             		156
#define     VM_ALARM_RING_NAME_2             		157
#define     VM_ALARM_RING_NAME_3             		158
#define     VM_ALARM_RING_NAME_4             		159
#define     VM_COLOR_LED_SETTING			 		160
#define     VM_EXTRA_FLASH_UPDATE_FLAG		 		161
#define     VM_EXTRA_FLASH_ALL_UPDATE_FLAG	 		162
#define     USER_PASSWORD                    		163    // 用户开机密码
#define     USER_PASSWORD_ON                 		164    // 用户开机密码功能是否开启
#define     VM_SPORT_INFO_SWITCH_FLAG        		165
#define     VM_SPORT_INFO_MODE_FLAG          		166
#define     VM_SPORT_INFO_EXERCISE_HEART_RATE 		167
#define     VM_SPORT_INFO_PERSONAL_INFO_FLAG 		168
#define     VM_SPORT_INFO_SEDENTARY          		169
#define     VM_SPORT_INFO_SLEEP_DETECTION	 		170
#define     VM_SPORT_INFO_FALL_DETECTION     		171
#define     VM_SPORT_INFO_RAISE_WRIST        		172
#define 	VM_SPORT_INFO_DAILY_DATA		 		173
#define 	VM_SHM_DAILY_ACTIVE						174
#define 	VM_SHM_DAILY_ACTIVE_TARGET				175
#define 	VM_MENSE_INFO							176
/*手表应用数据存储*/
#define     VM_SMALL_FILE_MANAGER                   177
#define     VM_SMALL_FILE_PHONEBOOK                 178
#define     VM_SMALL_FILE_CALL_LOG                  179
#define     VM_SMALL_FILE_WEATHER                   180
#define     VM_SMALL_FILE_MESSAGE_0                 181
#define     VM_SMALL_FILE_MESSAGE_1                 182
#define     VM_SMALL_FILE_MESSAGE_2                 183
#define     VM_SMALL_FILE_MESSAGE_3                 184
#define     VM_SMALL_FILE_MESSAGE_4                 185
#define     VM_SMALL_FILE_SPORTRECORD_0             186
#define     VM_SMALL_FILE_SPORTRECORD_1             187
#define     VM_SMALL_FILE_BLOOD_OXYGEN_0            188
#define     VM_SMALL_FILE_BLOOD_OXYGEN_1            189
#define     VM_SMALL_FILE_BLOOD_OXYGEN_2            190
#define     VM_SMALL_FILE_BLOOD_OXYGEN_3            191
#define     VM_SMALL_FILE_SLEEP_0                   192
#define     VM_SMALL_FILE_SLEEP_1                   193
#define     VM_SMALL_FILE_SLEEP_2                   194
#define     VM_SMALL_FILE_SLEEP_3                   195
#define     VM_SMALL_FILE_HEART						196
#define     VM_SMALL_FILE_START						VM_SMALL_FILE_MANAGER
#define     VM_SMALL_FILE_END						VM_SMALL_FILE_HEART
/*手表应用数据存储*/
#define		VM_BATTERY_INFO  						197
/*蓝牙耳机列表数据*/
#define     CFG_BT_PAGE_LIST                        198
/*手表升级相关数据*/
#define		VM_REUSABLE_SPECIAL_UPDATE_INFO 		199		// 用于记录先推loader再升级资源的状态位
#define     CFG_VBG_I_TRIM                          200
//MAX200

//=================================================================================//
//             用户自定义配置项 使用 [201 ~ 255]            		               //
//=================================================================================//
#define     VM_FM_BG_TRIM               			201
#define     VM_FM_CBPF_TRIM             			202

#define		CFG_SYS_VOL                 			203
#define     CFG_MUSIC_VOL               			204

#define 	CFG_TWS_LOCAL_ADDR						205
#define 	CFG_TWS_REMOTE_ADDR						206
#define     CFG_TWS_COMMON_ADDR         			207
#define     CFG_TWS_CHANNEL             			208
#define     CFG_TWS_DUAL_CONFIG          			209
#define     CFG_TWS_CONNECT_AA          			210
#define     CFG_CHARGESTORE_TWS_CHANNEL 			211

#define 	CFG_USB_BREAKPOINT0						212
#define 	CFG_SD0_BREAKPOINT0		    			213
#define 	CFG_SD1_BREAKPOINT0		    			214

#define 	VM_ID_MODE_MEMORY  						215

/*彩屏仓配置 begin*/
#define     VM_CSBG_SEQUENCE                       	216
#define     CFG_CHGBOX_ADDR                        	217//记忆耳机的mac地址
#define     VM_BAT_PRESENT                         	218//记忆电量
#define     FIRST_START_INFO                       	219//首次上电
/*彩屏仓配置 end*/


#define     VM_ID_KIDCAM_SETTINGS                   220
//MAX255

#endif /* #ifndef _USER_CFG_ID_H_ */
