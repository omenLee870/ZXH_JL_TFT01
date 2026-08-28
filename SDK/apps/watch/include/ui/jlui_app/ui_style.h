#ifndef UI_STYLE_H
#define UI_STYLE_H

#include "app_config.h"
#if (CONFIG_UI_STYLE == STYLE_JL_WATCH_PUBLIC_MODLS)
#include "jlui_app/style_JL_new.h"
#include "jlui_app/style_DIAL_new.h"
#include "jlui_app/style_upgrade_new.h"
#include "jlui_app/style_sidebar.h"


#define ID_WINDOW_DIAL                  DIAL_PAGE_0
#define ID_WINDOW_POWER_ON              PAGE_1
#define ID_WINDOW_POWER_OFF		        PAGE_2
#define ID_WINDOW_MENU_LIST		        PAGE_3
#define ID_WINDOW_MENU_STAR		        PAGE_4
#define ID_WINDOW_DAYACTIVE		        PAGE_5
#define ID_WINDOW_SETTING               PAGE_7
#define ID_WINDOW_UNDISTURB             PAGE_8
#define ID_WINDOW_SHUTDOWN              PAGE_9
#define ID_WINDOW_REBOOT                PAGE_10
#define ID_WINDOW_SETTING_PASSWORD      PAGE_11
#define ID_WINDOW_HEALTH_TIPS           PAGE_12
#define ID_WINDOW_LOW_POWER             PAGE_13
#define ID_WINDOW_BACK2FACTORY          PAGE_14
#define ID_WINDOW_TIMER                 PAGE_15
#define ID_WINDOW_STOPWATCH             PAGE_16
#define ID_WINDOW_ALIPAY   		        PAGE_17
#define ID_WINDOW_APP_QRCODE   		    PAGE_18
#define ID_WINDOW_ABOUT                 PAGE_19
#define ID_WINDOW_POWERON_PASSWORD      PAGE_20
#define ID_WINDOW_SIRI                  PAGE_21
#define ID_WINDOW_SETTING_BRIGHTNESS    PAGE_22
#define ID_WINDOW_SETTING_VOICE         PAGE_23
#define ID_WINDOW_CONN_NEW_PHONE        PAGE_24
#define ID_WINDOW_COMPASS               PAGE_25
#define ID_WINDOW_PHONE                 PAGE_26
#define ID_WINDOW_PHONE_KEYPAD          PAGE_27
#define ID_WINDOW_PHONE_CALL_STATUS		PAGE_28
#define ID_WINDOW_ALARM					PAGE_29
#define ID_WINDOW_ALARM_RINGING         PAGE_30
#define ID_WINDOW_MUSIC_PLAYER		    PAGE_31
#define ID_WINDOW_WEATHER				PAGE_32
#define ID_WINDOW_PHOTOGRAGH            PAGE_33
#define ID_WINDOW_CALCULATOR			PAGE_34
#define ID_WINDOW_CALENDAR				PAGE_35
#define ID_WINDOW_MENU_WATERFALLS	    PAGE_36
#define ID_WINDOW_MENU_WOMEN_HEALTH	    PAGE_37
#define ID_WINDOW_BATCHARGE             PAGE_38
#define ID_WINDOW_SPORT_TARGET          PAGE_39
#define ID_WINDOW_SPORT_TARGET_KM_SET   PAGE_40
#define ID_WINDOW_SPORT_RESULT          PAGE_41
#define ID_WINDOW_MOMENTUM              PAGE_42
#define ID_WINDOW_SPORTING              PAGE_43
#define ID_WINDOW_INDOOR_SPORTS         PAGE_44
#define ID_WINDOW_OUTDOOR_SPORTS        PAGE_45
#define ID_WINDOW_SPORT_INTENSITY       PAGE_46
#define ID_WINDOW_RUNLIGHT              PAGE_47
#define ID_WINDOW_BREATH_TRAIN          PAGE_48
#define ID_WINDOW_HEART		            PAGE_49
#define ID_WINDOW_OXYGEN	            PAGE_50
#define ID_WINDOW_SLEEPAID	            PAGE_51
#define ID_WINDOW_SLEEP		            PAGE_52
#define ID_WINDOW_BLOODPRESSURE         PAGE_53
#define ID_WINDOW_HEAT			        PAGE_54
#define ID_WINDOW_SETTING_COMMON        PAGE_55
#define ID_WINDOW_COMPONENT             PAGE_56
#define ID_WINDOW_DIAL_SEL				PAGE_57
#define ID_WINDOW_SHORTCUT_MENU         PAGE_59
#define ID_WINDOW_NOTICE				PAGE_60
#define ID_WINDOW_SPORT_TARGET_TIME_SET PAGE_62
#define ID_WINDOW_SPORT_TARGET_CAL_SET  PAGE_63
#define ID_WINDOW_FOOTBALL				PAGE_64
#define ID_WINDOW_RESTART_SHUTDOWN      PAGE_65
#define ID_WINDOW_BEDSIDE_WATCH			PAGE_67
#define ID_WINDOW_FLASHLIGHT            PAGE_68
#define ID_WINDOW_ENGINEERING_MODE      PAGE_69
#define ID_WINDOW_LOW_POWER_TIPS        PAGE_70
#define ID_WINDOW_BT_DISCONN_TIPS       PAGE_71
#define ID_WINDOW_WOMEN_HEALTH_WARNING	PAGE_72
#define ID_WINDOW_RUBIKS_CUBE	        PAGE_73
#define ID_WINDOW_FINDMY               	PAGE_74
#define ID_WINDOW_BT_EMITTER            PAGE_75
#define ID_WINDOW_SMARTWIN		        PAGE_76
#define ID_WINDOW_DIAL_MODE_SEL		    PAGE_77
#define ID_WINDOW_APP_IFLYTEK		    PAGE_78
#define ID_WINDOW_AI_DIAL		        PAGE_79
#define ID_WINDOW_NET_IFLY		        PAGE_80
#define ID_WINDOW_UVC					PAGE_81
#define ID_WINDOW_CAMERA				PAGE_82
#define ID_WINDOW_PC					PAGE_83
#define ID_WINDOW_PT					PAGE_85
#define ID_WINDOW_FINDPHONE             0
#define ID_WINDOW_UPGRADE		        UPGRADE_PAGE_0



#define ID_WINDOW_DEFAULT				ID_WINDOW_DIAL//没有页面时返回


#define CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE
#endif//STYLE_JL_WATCH_PUBLIC_MODLS

#if (CONFIG_UI_STYLE == STYLE_JL_CSC_PUBLIC_MODLS)
#include "jlui_app/style_JL_new_csc.h"
#include "jlui_app/style_DIAL_new_csc.h"
#include "jlui_app/style_upgrade_new_csc.h"
#include "jlui_app/style_sidebar_csc.h"
#define PAGE_NULL                       (-1)
#define ID_WINDOW_DIAL                  DIAL_PAGE_0
#define ID_WINDOW_POWER_ON              PAGE_1
#define ID_WINDOW_POWER_OFF		        PAGE_2
#define ID_WINDOW_APP_QRCODE            PAGE_NULL //APP下载二维码
#define ID_WINDOW_PAGE_EFFECTS_SW       PAGE_NULL
#define ID_WINDOW_MENU_EFFECTS_SW       PAGE_NULL
#define ID_WINDOW_TIMER                 PAGE_NULL
#define ID_WINDOW_ALARM_CLOCK           PAGE_3
#define ID_WINDOW_QRCODE   		        PAGE_NULL //APP连接二维码
#define ID_WINDOW_ABOUT                 PAGE_NULL
#define ID_WINDOW_PHONE                 PAGE_NULL
#define ID_WINDOW_PHONE_KEYPAD          PAGE_NULL
#define ID_WINDOW_PHONE_CALL_STATUS		PAGE_4
#define ID_WINDOW_ALARM_RINGING         PAGE_5
#define ID_WINDOW_WEATHER				PAGE_NULL
#define ID_WINDOW_PHOTOGRAGH            PAGE_6
#define ID_WINDOW_CALENDAR				PAGE_NULL
#define ID_WINDOW_MENU_WOMEN_HEALTH	    PAGE_NULL
#define ID_WINDOW_BATCHARGE             PAGE_7
#define ID_WINDOW_NOTICE				PAGE_21
#define ID_WINDOW_LANGUAGE_SELECT       PAGE_NULL
#define ID_WINDOW_FOOTBALL				PAGE_NULL
#define ID_WINDOW_FLASHLIGHT            PAGE_NULL
#define ID_WINDOW_ENGINEERING_MODE      PAGE_NULL
#define ID_WINDOW_VOLUME                PAGE_9
#define ID_WINDOW_EARPHONE_DISNOISE     PAGE_10
#define ID_WINDOW_EQUALIZER             PAGE_11
#define ID_WINDOW_MUSIC_PLAYER          PAGE_12
#define ID_WINDOW_H_LIST                PAGE_NULL
#define ID_WINDOW_TOPBAR                PAGE_13
#define ID_WINDOW_MUSIC_LRC             PAGE_NULL
#define ID_WINDOW_BG_SELECT             PAGE_NULL
#define ID_WINDOW_LOCK_SELECT           PAGE_14
#define ID_WINDOW_POP_UP                PAGE_NULL
#define ID_WINDOW_TIKTOK                PAGE_15
#define ID_WINDOW_LANGUAGE              PAGE_16
#define ID_WINDOW_FIND_EARPHONE         PAGE_17
#define ID_WINDOW_LCD_BRIGHTNESS        PAGE_18
#define ID_WINDOW_LCD_LIGHT_TIME        PAGE_NULL
#define ID_WINDOW_APP_MENU              PAGE_NULL
#define ID_WINDOW_SET_MENU              PAGE_NULL
#define ID_WINDOW_SURE                  PAGE_NULL
#define ID_WINDOW_TOUCH_AWAKE           PAGE_NULL
#define ID_WINDOW_TIME_SETTING          PAGE_19
#define ID_WINDOW_RESTART_SHUTDOWN      PAGE_20
#define ID_WINDOW_KEY_SET               PAGE_NULL
#define ID_WINDOW_EFFECT_SEL            PAGE_NULL
#define ID_WINDOW_BEDSIDE_WATCH         PAGE_NULL
#define ID_WINDOW_SPORTING              PAGE_NULL
#define ID_WINDOW_SPORT_RESULT          PAGE_NULL
#define ID_WINDOW_POWERON_PASSWORD      PAGE_NULL
#define ID_WINDOW_COMPONENT             PAGE_NULL
#define ID_WINDOW_SHORTCUT_MENU         PAGE_NULL
#define ID_WINDOW_STOPWATCH             PAGE_NULL
#define ID_WINDOW_INDOOR_SPORTS         PAGE_NULL
#define ID_WINDOW_OUTDOOR_SPORTS        PAGE_NULL

#define ID_WINDOW_FINDPHONE             0
#define ID_WINDOW_UPGRADE		        UPGRADE_PAGE_0

#define ID_WINDOW_DEFAULT				ID_WINDOW_MUSIC_PLAYER//没有页面时返回

#define CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
#endif//STYLE_JL_CSC_PUBLIC_MODLS


#if (CONFIG_UI_STYLE == STYLE_JL_WTACH_NEW)

#include "jlui_app/style_JL_new.h"
#include "jlui_app/style_DIAL_new.h"
#include "jlui_app/style_upgrade_new.h"
#include "jlui_app/style_sidebar.h"

#ifdef PAGE_0
#undef PAGE_0
#define PAGE_0 DIAL_PAGE_0
#endif

// #define ID_WINDOW_BT                    PAGE_76
#define ID_WINDOW_DIAL                  PAGE_0
#define ID_WINDOW_BT                    PAGE_0
#define ID_WINDOW_CLOCK                 PAGE_0
#define ID_WINDOW_ACTIVERECORD          PAGE_1
#define ID_WINDOW_SLEEP                 PAGE_2
#define ID_WINDOW_MAIN                  PAGE_3
#define ID_WINDOW_POWER_ON              PAGE_4
#define ID_WINDOW_POWER_OFF             PAGE_5
#define ID_WINDOW_VMENU                 PAGE_7
#define ID_WINDOW_PHONE                 PAGE_6
#define ID_WINDOW_MUSIC                 PAGE_10
#define ID_WINDOW_CALL_DIAL             PAGE_13
#define ID_WINDOW_IDLE                  PAGE_15
#define ID_WINDOW_STAR_MENU 			PAGE_17
#define ID_WINDOW_PHONEBOOK             PAGE_19
#define ID_WINDOW_PHONEBOOK_SYNC        PAGE_20
#define ID_WINDOW_MUSIC_SET             PAGE_21
#define ID_WINDOW_CALLRECORD            PAGE_22
#define ID_WINDOW_PAGE                  PAGE_23
#define ID_WINDOW_MUSIC_BROWER          PAGE_28
#define ID_WINDOW_PC                    PAGE_32
#define ID_WINDOW_STOPWATCH             PAGE_33
#define ID_WINDOW_CALCULAGRAPH          PAGE_34
#define ID_WINDOW_SET					PAGE_41
#define ID_WINDOW_SCREEN_DISP			PAGE_42
#define ID_WINDOW_VOICE_SET				PAGE_43
#define ID_WINDOW_SHAKE_LEVEL			PAGE_44
#define ID_WINDOW_UNDISTURB_MODE		PAGE_45
#define ID_WINDOW_ABOUT					PAGE_46
#define ID_WINDOW_TRAIN_SET				PAGE_47
#define ID_WINDOW_TOUCH_SEND			PAGE_48
#define ID_WINDOW_USER_GUIDE			PAGE_49
#define ID_WINDOW_SYS_MENU				PAGE_50
#define ID_WINDOW_ALARM                 PAGE_51
#define ID_WINDOW_FLASHLIGHT            PAGE_52
#define ID_WINDOW_FINDPHONE             PAGE_53
#define ID_WINDOW_TRAIN                 PAGE_54
#define ID_WINDOW_SPORT_SHOW            PAGE_55
#define ID_WINDOW_BREATH_TRAIN          PAGE_57
#define ID_WINDOW_ALTIMETER				PAGE_63
#define ID_WINDOW_BARO                  PAGE_64
#define ID_WINDOW_WEATHER               PAGE_66
#define ID_WINDOW_PRESSURE              PAGE_67
#define ID_WINDOW_COMPASS               PAGE_73
#define ID_WINDOW_ALARM_RING_START		PAGE_74
#define ID_WINDOW_STYLE               	PAGE_75
#define ID_WINDOW_HEART                 PAGE_76
#define ID_WINDOW_BLOOD_OXYGEN          PAGE_77
#define ID_WINDOW_SPORT_INFO            PAGE_78
#define ID_WINDOW_SPORT_CTRL			PAGE_79
#define ID_WINDOW_SPORT_RECORD          PAGE_80
#define ID_WINDOW_MESS                  PAGE_81
#define ID_WINDOW_FALL					PAGE_83
#define ID_WINDOW_DETECTION				PAGE_84
#define ID_WINDOW_LANGUAGE				PAGE_85
#define ID_WINDOW_SHUTDOWN_OR_RESET     PAGE_86
#define ID_WINDOW_TRAIN_STATUS			PAGE_87
#define ID_WINDOW_SPORT_COURSE			PAGE_88
#define ID_WINDOW_ALARM_RING_STOP		PAGE_89
#define ID_WINDOW_ALARM_RING_SOON		PAGE_90
#define ID_WINDOW_ALIPAY			    PAGE_94
#define ID_WINDOW_BT_SETTING			PAGE_95
#define ID_WINDOW_RECORD				PAGE_96
#define ID_WINDOW_CALENDAR				PAGE_97
#define ID_WINDOW_CALCULATOR			PAGE_98
#define ID_WINDOW_DRAWER                PAGE_99
#define ID_WINDOW_CARD_BAG				PAGE_100

#define ID_WINDOW_AI					PAGE_103
#define ID_WINDOW_AI_DIAL				PAGE_104

#define ID_WINDOW_CALL_CHANNEL_SEL      PAGE_105
#define ID_WINDOW_CAT1_SETTING      	PAGE_106
#define ID_WINDOW_CAT1_AICXTEK_ENGINEERING      PAGE_107
#define ID_WINDOW_CAT1_CALL_SMS_SEL     PAGE_108
#define ID_WINDOW_CAT1_SMS_REPLY        PAGE_109

#define ID_WINDOW_CAT1_UNISOC_ENGINEERING      PAGE_110

#define ID_WINDOW_FINDMY_SETTING		PAGE_111

#define ID_WINDOW_NET_IFLY		        PAGE_112

#define ID_WINDOW_UPGRADE			    UPGRADE_PAGE_0

#define CONFIG_UI_STYLE_JL_ENABLE
#endif

#if(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX)
#include "ui/style_jl02.h"//点阵//
#define ID_WINDOW_MAIN     PAGE_0
#define ID_WINDOW_BT       PAGE_1
#define ID_WINDOW_FM       PAGE_2
#define ID_WINDOW_CLOCK    PAGE_3
#define ID_WINDOW_MUSIC    PAGE_4
#define ID_WINDOW_LINEIN   PAGE_0
#define ID_WINDOW_POWER_ON     PAGE_5
#define ID_WINDOW_POWER_OFF    PAGE_6
#define ID_WINDOW_SYS      PAGE_7
#endif

#if(CONFIG_UI_STYLE == STYLE_JL_LED7)//led7 显示
#define ID_WINDOW_BT           UI_BT_MENU_MAIN
#define ID_WINDOW_FM           UI_FM_MENU_MAIN
#define ID_WINDOW_CLOCK        UI_RTC_MENU_MAIN
#define ID_WINDOW_MUSIC        UI_MUSIC_MENU_MAIN
#define ID_WINDOW_LINEIN       UI_AUX_MENU_MAIN
#define ID_WINDOW_PC           UI_PC_MENU_MAIN
#define ID_WINDOW_POWER_ON     UI_IDLE_MENU_MAIN
#define ID_WINDOW_POWER_OFF    UI_IDLE_MENU_MAIN
#define ID_WINDOW_SPDIF        UI_IDLE_MENU_MAIN
#define ID_WINDOW_IDLE         UI_IDLE_MENU_MAIN
#endif


#if ((CONFIG_UI_STYLE == STYLE_JL_WTACH) || (CONFIG_UI_STYLE == STYLE_JL_WTACH_NEW))

#if (defined(ID_WINDOW_ACTIVERECORD) && defined(TCFG_UI_ENABLE_SPORTRECORD) && (!TCFG_UI_ENABLE_SPORTRECORD))
#undef ID_WINDOW_ACTIVERECORD
#define ID_WINDOW_ACTIVERECORD          0
#endif

#if (defined(ID_WINDOW_MUSIC) && defined(TCFG_UI_ENABLE_MUSIC) && (!TCFG_UI_ENABLE_MUSIC))
#undef ID_WINDOW_MUSIC
#define ID_WINDOW_MUSIC					0
#endif

#if (defined(ID_WINDOW_MUSIC_SET) && defined(TCFG_UI_ENABLE_MUSIC_MENU) && (!TCFG_UI_ENABLE_MUSIC_MENU))
#undef ID_WINDOW_MUSIC_SET
#define ID_WINDOW_MUSIC_SET             0
#endif

#if (defined(ID_WINDOW_MUSIC_BROWER) && defined(TCFG_UI_ENABLE_FILE) && (!TCFG_UI_ENABLE_FILE))
#undef ID_WINDOW_MUSIC_BROWER
#define ID_WINDOW_MUSIC_BROWER          0
#endif

#if (defined(ID_WINDOW_PHONEBOOK) && defined(TCFG_UI_ENABLE_PHONEBOOK) && (!TCFG_UI_ENABLE_PHONEBOOK))
#undef ID_WINDOW_PHONEBOOK
#define ID_WINDOW_PHONEBOOK				0
#undef ID_WINDOW_PHONEBOOK_SYNC
#define ID_WINDOW_PHONEBOOK_SYNC        0
#endif

#if (defined(ID_WINDOW_PC) && defined(TCFG_UI_ENABLE_PC) && (!TCFG_UI_ENABLE_PC))
#undef ID_WINDOW_PC
#define ID_WINDOW_PC                    0
#endif

#if (defined(ID_WINDOW_STOPWATCH) && defined(TCFG_UI_ENABLE_STOPWATCH) && (!TCFG_UI_ENABLE_STOPWATCH))
#undef ID_WINDOW_STOPWATCH
#define ID_WINDOW_STOPWATCH             0
#endif

#if (defined(ID_WINDOW_CALCULAGRAPH) && defined(TCFG_UI_ENABLE_TIMER_ACTION) && (!TCFG_UI_ENABLE_TIMER_ACTION))
#undef ID_WINDOW_CALCULAGRAPH
#define ID_WINDOW_CALCULAGRAPH          0
#endif

#if (defined(ID_WINDOW_ALARM) && defined(TCFG_UI_ENABLE_ALARM) && (!TCFG_UI_ENABLE_ALARM))
#undef ID_WINDOW_ALARM
#define ID_WINDOW_ALARM                 0
#endif

#if (defined(ID_WINDOW_FLASHLIGHT) && defined(TCFG_UI_ENABLE_FLASHLIGHT) && (!TCFG_UI_ENABLE_FLASHLIGHT))
#undef ID_WINDOW_FLASHLIGHT
#define ID_WINDOW_FLASHLIGHT            0
#endif

#if (defined(ID_WINDOW_FINDPHONE) && defined(TCFG_UI_ENABLE_FINDPHONE) && (!TCFG_UI_ENABLE_FINDPHONE))
#undef ID_WINDOW_FINDPHONE
#define ID_WINDOW_FINDPHONE             0
#endif

#if (defined(ID_WINDOW_WEATHER) && defined(TCFG_UI_ENABLE_WEATHER) && (!TCFG_UI_ENABLE_WEATHER))
#undef ID_WINDOW_WEATHER
#define ID_WINDOW_WEATHER               0
#endif

#if (defined(ID_WINDOW_MESS) && defined(TCFG_UI_ENABLE_NOTICE) && (!TCFG_UI_ENABLE_NOTICE))
#undef ID_WINDOW_MESS
#define ID_WINDOW_MESS                  0
#endif

#if (defined(ID_WINDOW_ALTIMETER) && defined(TCFG_UI_ENABLE_ALTIMETER) && (!TCFG_UI_ENABLE_ALTIMETER))
#undef ID_WINDOW_ALTIMETER
#define ID_WINDOW_ALTIMETER				0
#endif

#if (defined(ID_WINDOW_BARO) && defined(TCFG_UI_ENABLE_PRESSURE) && (!TCFG_UI_ENABLE_PRESSURE))
#undef ID_WINDOW_BARO
#define ID_WINDOW_BARO                  0
#endif

#if (defined(ID_WINDOW_SPORT_INFO) && defined(TCFG_UI_ENABLE_SPORT_INFO) && (!TCFG_UI_ENABLE_SPORT_INFO))
#undef ID_WINDOW_SPORT_INFO
#define ID_WINDOW_SPORT_INFO            0
#endif

#if (defined(ID_WINDOW_BLOOD_OXYGEN) && defined(TCFG_UI_ENABLE_OXYGEN) && (!TCFG_UI_ENABLE_OXYGEN))
#undef ID_WINDOW_BLOOD_OXYGEN
#define ID_WINDOW_BLOOD_OXYGEN          0
#endif

#if (defined(ID_WINDOW_SPORT_RECORD) && defined(TCFG_UI_ENABLE_SPORTRECORD) && (!TCFG_UI_ENABLE_SPORTRECORD))
#undef ID_WINDOW_SPORT_RECORD
#define ID_WINDOW_SPORT_RECORD          0
#endif

#if (defined(ID_WINDOW_TRAIN) && defined(TCFG_UI_ENABLE_TRAIN) && (!TCFG_UI_ENABLE_TRAIN))
#undef ID_WINDOW_TRAIN
#define ID_WINDOW_TRAIN                 0
#undef ID_WINDOW_SPORT_SHOW
#define ID_WINDOW_SPORT_SHOW            0
#endif


#if (defined(ID_WINDOW_BREATH_TRAIN) && defined(TCFG_UI_ENABLE_BREATH_TRAIN) && (!TCFG_UI_ENABLE_BREATH_TRAIN))
#undef ID_WINDOW_BREATH_TRAIN
#define ID_WINDOW_BREATH_TRAIN          0
#endif

#if (defined(ID_WINDOW_PRESSURE) && defined(TCFG_UI_ENABLE_HEAT) && (!TCFG_UI_ENABLE_HEAT))
#undef ID_WINDOW_PRESSURE
#define ID_WINDOW_PRESSURE          	0
#endif

#if (defined(ID_WINDOW_HEART) && defined(TCFG_UI_ENABLE_HEART) && (!TCFG_UI_ENABLE_HEART))
#undef ID_WINDOW_HEART
#define ID_WINDOW_HEART                 0
#endif

#if (defined(ID_WINDOW_SLEEP) && defined(TCFG_UI_ENABLE_SLEEP) && (!TCFG_UI_ENABLE_SLEEP))
#undef ID_WINDOW_SLEEP
#define ID_WINDOW_SLEEP                 0
#endif

#if (defined(ID_WINDOW_CALL_CHANNEL_SEL) && defined(TCFG_APP_CAT1_EN) && (!TCFG_APP_CAT1_EN))
#undef ID_WINDOW_CALL_CHANNEL_SEL
#define ID_WINDOW_CALL_CHANNEL_SEL      0
#endif

#endif // #if ((CONFIG_UI_STYLE == STYLE_JL_WTACH) || (CONFIG_UI_STYLE == STYLE_JL_WTACH_NEW))

#endif
