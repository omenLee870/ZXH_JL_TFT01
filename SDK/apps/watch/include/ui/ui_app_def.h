#ifndef _UI_APP_DEF_H_
#define _UI_APP_DEF_H_


#if (CONFIG_JL_UI_ENABLE || CONFIG_LVGL_UI_ENABLE)
#define TCFG_UI_ENABLE 						1 //UI配置
#else

// #ifdef TCFG_UI_ENABLE
// #undef TCFG_UI_ENABLE
// #endif
// #define TCFG_UI_ENABLE 						0

// #ifdef TCFG_SPI_LCD_ENABLE
// #undef TCFG_SPI_LCD_ENABLE
// #endif
// #define TCFG_SPI_LCD_ENABLE					0

#ifdef TCFG_TOUCH_PANEL_ENABLE
#undef TCFG_TOUCH_PANEL_ENABLE
#endif
#define TCFG_TOUCH_PANEL_ENABLE				0

#endif

#define STYLE_UI_SIMPLE             		(1)//没有ui框架
#define STYLE_JL_SOUNDBOX           		(2)//点阵屏demo
#define STYLE_JL_WTACH              		(3)//彩屏demo
#define STYLE_JL_WTACH_NEW          		(4)//彩屏demo
#define STYLE_JL_CHARGE             		(5)//点阵屏充电仓
#define STYLE_JL_LED7               		(6)//led7
#define STYLE_JL_SOUNDBAR           		(7)//soundbar点阵屏
#define STYLE_JL_WATCH_PUBLIC_MODLS			(8)//方屏
#define STYLE_JL_CSC_PUBLIC_MODLS	        (9)//彩屏仓


#if (TCFG_UI_ENABLE && CONFIG_JL_UI_ENABLE)
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
#define CONFIG_UI_STYLE 					STYLE_JL_CSC_PUBLIC_MODLS //彩屏仓UI类型
#else
#define CONFIG_UI_STYLE 					STYLE_JL_WATCH_PUBLIC_MODLS //UI类型
#endif
#else
#define CONFIG_UI_STYLE 					STYLE_UI_SIMPLE //UI类型
#endif

//*********************************************************************************//
//                          UI_APP配置（公共配置）                                 //
//*********************************************************************************//
#if defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE
#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH)
#define CONFIG_UI_STYLE_GPU_PUBLIC_MODLS_ENABLE
#endif
#endif

#if (CONFIG_UI_STYLE == STYLE_JL_WATCH_PUBLIC_MODLS)
#define TCFG_UI_ENABLE_STANDBY_DIAL			ENABLE // 表盘浏览
#define TCFG_UI_ENABLE_PULLDOWN_MENU		DISABLE // 下拉菜单
#define TCFG_UI_ENABLE_LEFT_MENU            ENABLE // 左滑侧边栏,ui_page_manager.h把SIDEBAR_LEFT_MENU_ENABLE置1关闭卡片管理表盘右滑功能
#define TCFG_UI_ENABLE_SYS_SET              ENABLE // 设置
#define TCFG_UI_ENABLE_SHORTCUT_MENU        ENABLE // 快捷菜单

#define TCFG_UI_ENABLE_UPGRATE				ENABLE // 升级

// 需要使能TCFG_APP_RTC_EN
#if TCFG_APP_RTC_EN
#define TCFG_UI_ENABLE_ALARM				ENABLE // 闹钟功能
#define TCFG_UI_ENABLE_STOPWATCH			ENABLE // 计时器
#define TCFG_UI_ENABLE_TIMER_ACTION			ENABLE // 倒计时
#else
#define TCFG_UI_ENABLE_ALARM				DISABLE // 闹钟功能
#define TCFG_UI_ENABLE_STOPWATCH			DISABLE // 计时器
#define TCFG_UI_ENABLE_TIMER_ACTION			DISABLE // 倒计时
#endif

// 需要使能TCFG_APP_PC_EN
#define TCFG_UI_ENABLE_PC				    DISABLE // PC模式

// 需要使能TCFG_APP_RECORD_EN
#define TCFG_UI_ENABLE_RECORD				DISABLE // RECORD模式

// 需要使能TCFG_APP_MUSIC_EN
#define TCFG_UI_ENABLE_MUSIC				ENABLE // 音乐
#define TCFG_UI_ENABLE_MUSIC_MENU			DISABLE // 音乐菜单
#define TCFG_UI_ENABLE_FILE				    DISABLE // 文件浏览

// 需要使能TCFG_USER_EMITTER_ENABLE
#define TCFG_UI_ENABLE_BT_EMITTER 	        ENABLE	//蓝牙耳机

#define TCFG_UI_ENABLE_PHONE_ACTION			ENABLE // 蓝牙通话
#define TCFG_UI_ENABLE_PHONEBOOK			DISABLE // 电话本
#define TCFG_UI_ENABLE_FINDPHONE			DISABLE // 找手机
#define TCFG_UI_ENABLE_NOTICE				ENABLE // 手机消息通知
#define TCFG_UI_ENABLE_WEATHER				ENABLE // 天气

#define TCFG_UI_ENABLE_FLASHLIGHT			ENABLE // 手电筒
#define TCFG_UI_ENABLE_QR_CODE				ENABLE // 二维码

#define TCFG_UI_ENABLE_CALENDAR				ENABLE  // 日历
#define TCFG_UI_ENABLE_CALCULATOR			ENABLE // 计算器
#define TCFG_UI_ENABLE_COMPASS              ENABLE // 指南针
#define TCFG_UI_ENABLE_POWERON_PASSWORD     ENABLE // 开机密码

// 需要使能TCFG_BMP280_ENABLE
#define TCFG_UI_ENABLE_ALTIMETER			DISABLE // 海拔高度
#define TCFG_UI_ENABLE_PRESSURE				DISABLE // 海拔气压

// 需要使能TCFG_GSENSOR_ENABLE、TCFG_HR_SENSOR_ENABLE
#define TCFG_UI_ENABLE_SPORT_INFO			DISABLE // 运动状态
#define TCFG_UI_ENABLE_SPORTRECORD			DISABLE // 运动记录
#define TCFG_UI_ENABLE_TRAIN				DISABLE // 锻炼
#define TCFG_UI_ENABLE_SPORTRECORD_BROWSE	DISABLE // 运动记录浏览
#define TCFG_UI_ENABLE_OXYGEN				ENABLE // 血氧饱和度
#define TCFG_UI_ENABLE_HEART				ENABLE // 心率
#define TCFG_UI_ENABLE_BREATH_TRAIN			ENABLE // 呼吸训练
#define TCFG_UI_ENABLE_HEAT					ENABLE // 压力
#define TCFG_UI_ENABLE_SLEEP				ENABLE // 睡眠
#define TCFG_UI_ENABLE_BLOODPRESS			ENABLE // 血压

#define TCFG_UI_ENABLE_ALIPAY_CODE			DISABLE // 支付宝
#define TCFG_UI_ENABLE_AI_INTERACTION		DISABLE // APP版本AI表盘
#define TCFG_UI_ENABLE_IFLYTEK				DISABLE // 科大讯飞网络版

#define TCFG_UI_ENABLE_DEMO                 DISABLE //示例

#define TCFG_UI_ENABLE_MENU_STAR_NEW		DISABLE //新满天星
#define TCFG_UI_ENABLE_SCREENSHOT			DISABLE	//截屏

#define TCFG_UI_DRAW_DEMO_ENABLE			ENABLE	//绘图demo
#define TCFG_UI_ANIM_DEMO_ENABLE			DISABLE	//动画demo

#define TCFG_UI_ENABLE_VOICE_ASSISTANT      ENABLE // 语音助手
#define	TCFG_UI_DIAL_ENABLE				    ENABLE	//表盘
#define	TCFG_UI_DIAL_SEL_ENABLE			    ENABLE	//表盘选择
#define TCFG_UI_MENU_LIST_ENABLE			ENABLE  //列表菜单
#define TCFG_UI_MENU_WATERFALLS_ENABLE		ENABLE  //瀑布菜单
#define TCFG_UI_MENSE_MANAGE_ENABLE			ENABLE  //女性
#define TCFG_UI_ENABLE_PHOTOGRAGH			ENABLE	//拍照
#define TCFG_UI_ENABLE_RUNLIGHT				ENABLE	//跑步灯

#define TCFG_UI_ENABLE_BATCHARGE            ENABLE	//充电
#define TCFG_UI_ENABLE_INDOORSPORTS         ENABLE	//室内运动
#define TCFG_UI_ENABLE_OUTDOORSPORTS        ENABLE	//室外运动
#define TCFG_UI_ENABLE_SPORTING             ENABLE	//运动
#define TCFG_UI_ENABLE_SPORT_INTENSITY      ENABLE	//运动强度
#define TCFG_UI_ENABLE_MOMENTUM             ENABLE	//运动量
#define TCFG_UI_ENABLE_SLEEPAID             ENABLE	//助眠
#define TCFG_UI_ENABLE_PULLUP_MENU			ENABLE	//上拉菜单
#define TCFG_UI_ENABLE_FOOTBALL				ENABLE  //足球
#define TCFG_UI_BEDSIDE_WATCH_ENABLE 		ENABLE	//床头时钟
#define TCFG_UI_ENABLE_ENGINEERING_MODE 	ENABLE	//工程模式
#define TCFG_UI_ENABLE_RUBIKS_CUBE 	        ENABLE	//魔方
#define TCFG_UI_SD_MUSIC_ENABLE 	        ENABLE	//sd music

#define TCFG_UI_UVC_SHOW_ENABLE             TCFG_HOST_UVC_ENABLE //uvc ui显示demo
#define TCFG_UI_CAMERA_ENABLE				TCFG_CAMERA_MANAGER_ENABLE		//摄像头、相册

#define TCFG_UI_APP_EFFECT_ENABLE			ENABLE  //特效
#define TCFG_UI_EFFECT_USED_DEMO			DISABLE	//用户自定义特效demo

#define TCFG_UI_ENABLE_SMARTWIN				ENABLE // 灵动岛
#define TCFG_UI_AVRCP_MUSIC_BG_ENABLE		DISABLE//avrcp传图（需使能USER_SUPPORT_PROFILE_BIP）

#define TCFG_UI_JL_PRODUCT_TEST				TCFG_PT_JL_TEST_ENABLE	//开发板产测

#if (!TCFG_APP_BT_EN) //关闭蓝牙相关UI
#undef TCFG_UI_ENABLE_BT_PAGE_ACTION
#define TCFG_UI_ENABLE_BT_PAGE_ACTION		DISABLE // 蓝牙耳机列表
#undef TCFG_UI_ENABLE_BT_SCAN
#define TCFG_UI_ENABLE_BT_SCAN				DISABLE // 搜索蓝牙耳机
#undef TCFG_UI_ENABLE_PAGE_TOUCH
#define TCFG_UI_ENABLE_PAGE_TOUCH			DISABLE // 已保存蓝牙耳机列表
#undef TCFG_UI_ENABLE_SCAN_TOUCH
#define TCFG_UI_ENABLE_SCAN_TOUCH			DISABLE // 搜索蓝牙耳机
#undef TCFG_UI_ENABLE_PHONE_ACTION
#define TCFG_UI_ENABLE_PHONE_ACTION			DISABLE // 蓝牙通话
#undef TCFG_UI_ENABLE_PHONEBOOK
#define TCFG_UI_ENABLE_PHONEBOOK			DISABLE // 电话本
#undef TCFG_UI_ENABLE_FINDPHONE
#define TCFG_UI_ENABLE_FINDPHONE			DISABLE // 找手机
#undef TCFG_UI_ENABLE_NOTICE
#define TCFG_UI_ENABLE_NOTICE				DISABLE // 手机消息通知
#undef TCFG_UI_ENABLE_WEATHER
#define TCFG_UI_ENABLE_WEATHER				DISABLE // 天气
#undef TCFG_UI_ENABLE_QR_CODE
#define TCFG_UI_ENABLE_QR_CODE				DISABLE // 二维码
#undef TCFG_UI_ENABLE_VOICE_ASSISTANT
#define TCFG_UI_ENABLE_VOICE_ASSISTANT      DISABLE // 语音助手
#undef TCFG_UI_ENABLE_PHOTOGRAGH
#define TCFG_UI_ENABLE_PHOTOGRAGH			DISABLE // 拍照
#endif

#elif (CONFIG_UI_STYLE == STYLE_JL_CSC_PUBLIC_MODLS)

#define TCFG_UI_ENABLE_SYS_SET              ENABLE // 设置
#define	TCFG_UI_DIAL_ENABLE				    ENABLE	//表盘

// // 转场动画特效
#define TCFG_UI_APP_EFFECT_ENABLE					ENABLE  //特效
#define TCFG_UI_PAGE_MOVE_MODE_SCALE_DOUBLE    		ENABLE  // 缩放
#define TCFG_UI_MOVE_MODE_FLIP                 		ENABLE  // 翻页
#define TCFG_UI_MOVE_MODE_CUBE                 		ENABLE  // 3D立方体
#define TCFG_UI_MOVE_MODE_REFLECTION           		ENABLE  // 3D灯笼 + 倒影( 暂时使用中心轴翻转的前端
#define TCFG_UI_MOVE_MODE_HEXAGON              		ENABLE  // 3D灯笼
#define TCFG_UI_MOVE_MODE_CUBE_REFLECTION     		ENABLE  // 3D立方体 + 倒影
#define TCFG_UI_PAGE_MOVE_MODE_CENTER_FLIP	  		ENABLE  //中心轴
#define TCFG_UI_PAGE_MOVE_MODE_EDGE_FLIP			ENABLE  //边沿
#define TCFG_UI_PAGE_MOVE_MODE_CUBE_FLIP    		ENABLE  //立方体翻页
#define TCFG_UI_PAGE_MOVE_MODE_DRIFT_FLIP 			ENABLE	//飘移翻页
#define TCFG_UI_PAGE_MOVE_MODE_BOARD_FLIP			ENABLE  //翻板
#define TCFG_UI_PAGE_MOVE_MODE_BOARD_SLICING_FLIP 	ENABLE  //切片翻板
#define TCFG_UI_SD_MUSIC_ENABLE 	        DISABLE		//sd music
#define TCFG_UI_ENABLE_BT_EMITTER 	       DISABLE	//蓝牙耳机

#define TCFG_UI_BG_ENABLE                             DISABLE    //背景图功能
#define TCFG_UI_USE_COMMON_BACKGROUND_SWITCH_ENABLE   ENABLE    //表盘和普通功能共用背景图，省空间使用
#define TCFG_UI_WEATHER_ENABLE                        DISABLE   //天气页面
#define TCFG_UI_PHONE_CALL_ENABLE                     ENABLE    //电话通话页面
#define TCFG_UI_PHONE_KEYPAD_ENABLE                   DISABLE   //电话键盘页面
#define TCFG_UI_PHONE_CALL_LOG_ENABLE                 DISABLE   //电话记录页面
#define TCFG_UI_ABOUT_ENABLE                          DISABLE   //关于页面
#define TCFG_UI_TIMER_ENABLE                          ENABLE    //计时器
#define TCFG_UI_PHOTOGRAHT_ENABLE                     ENABLE    //拍照页面
#define TCFG_UI_SETTING_ENABLE                        ENABLE    //设置页面
#define TCFG_UI_TOUCH_CTRL_ENABLE                     DISABLE   //触摸使能
#define TCFG_UI_TIME_SET_ENABLE                       ENABLE    //时间设置页面
#define TCFG_UI_FIND_EARPHONE_ENABLE                  ENABLE    //查找耳机页面
#define TCFG_UI_UP_MOVE_DEMO_ENABLE                   DISABLE   //上划演示页面
#define TCFG_UI_MUSIC_CTRL_ENABLE                     ENABLE    //音乐控制页面
#define TCFG_UI_DENOISE_ENABLE                        ENABLE    //anc页面
#define TCFG_UI_VOLUME_ENABLE                         ENABLE    //音量页面
#define TCFG_UI_MSG_NOTICE                            ENABLE   //消息通知页面
#define TCFG_UI_FOOTBALL_MENU                         ENABLE    //足球菜单页面
#define TCFG_UI_ARC_MENU                              DISABLE   //弧形菜单页面
#define TCFG_UI_SECOND_MENU                           DISABLE   //二级菜单页面
#define TCFG_UI_ALARM_CLOCK_ENABLE                    ENABLE    //闹钟页面
#define TCFG_UI_ENABLE_FLASHLIGHT	              	  DISABLE   //手电筒
#define TCFG_UI_LANGUAGE_SEL_ENABLE	              	  ENABLE    //语言选择页面
#define TCFG_UI_EQ_ENABLE	              	          ENABLE    //eq页面
#define TCFG_UI_TIKTOK_ENABLE	              	      ENABLE    //tiktok页面
#define TCFG_UI_ENABLE_BATCHARGE                      ENABLE	//充电页面
#define TCFG_UI_ENABLE_UPGRATE						  ENABLE	//升级
#define	TCFG_UI_DIAL_SEL_ENABLE			              ENABLE	//表盘选择
#define TCFG_UI_MENU_LIST_ENABLE			          DISABLE   //列表菜单
#define TCFG_UI_ENABLE_SMARTWIN				          DISABLE   // 灵动岛
#define TCFG_UI_AVRCP_MUSIC_BG_ENABLE				  DISABLE//avrcp传图（需使能USER_SUPPORT_PROFILE_BIP）

#else
//这里主要为了没有ui时候，避免编译错误
//转场动画特效
#define TCFG_UI_APP_EFFECT_ENABLE					ENABLE  //特效
#define TCFG_UI_PAGE_MOVE_MODE_SCALE_DOUBLE    		ENABLE  // 缩放
#define TCFG_UI_MOVE_MODE_FLIP                 		ENABLE  // 翻页
#define TCFG_UI_MOVE_MODE_CUBE                 		ENABLE  // 3D立方体
#define TCFG_UI_MOVE_MODE_REFLECTION           		ENABLE  // 3D灯笼 + 倒影( 暂时使用中心轴翻转的前端
#define TCFG_UI_MOVE_MODE_HEXAGON              		ENABLE  // 3D灯笼
#define TCFG_UI_MOVE_MODE_CUBE_REFLECTION     		ENABLE  // 3D立方体 + 倒影
#define TCFG_UI_PAGE_MOVE_MODE_CENTER_FLIP	  		ENABLE  //中心轴
#define TCFG_UI_PAGE_MOVE_MODE_EDGE_FLIP			ENABLE  //边沿
#define TCFG_UI_PAGE_MOVE_MODE_CUBE_FLIP    		ENABLE  //立方体翻页
#define TCFG_UI_PAGE_MOVE_MODE_DRIFT_FLIP 			ENABLE	//飘移翻页
#define TCFG_UI_PAGE_MOVE_MODE_BOARD_FLIP			ENABLE  //翻板
#define TCFG_UI_PAGE_MOVE_MODE_BOARD_SLICING_FLIP 	ENABLE  //切片翻板
#endif
//*********************************************************************************//
//                          UI_APP配置（差异项配置）                               //
//*********************************************************************************//

#if (CONFIG_UI_STYLE == STYLE_JL_WATCH_PUBLIC_MODLS)



#endif//(CONFIG_UI_STYLE == STYLE_JL_WATCH_PUBLIC_MODLS)

#if (CONFIG_UI_STYLE ==STYLE_JL_WTACH_NEW)


#endif//(CONFIG_UI_STYLE ==STYLE_JL_WTACH_NEW)

#endif
