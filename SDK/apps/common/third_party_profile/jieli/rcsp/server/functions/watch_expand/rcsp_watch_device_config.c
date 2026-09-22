#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_watch_expand.data.bss")
#pragma data_seg(".rcsp_watch_expand.data")
#pragma const_seg(".rcsp_watch_expand.text.const")
#pragma code_seg(".rcsp_watch_expand.text")
#endif

#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp.h"
#include "rcsp_event.h"
#include "JL_rcsp_attr.h"
#include "ble_rcsp_server.h"
#include "rcsp_setting_opt.h"
#include "rcsp_manage.h"
#include "rcsp_watch_device_config.h"

#if (RCSP_MODE )

/* 智能眼镜设备能力(0xD9)的LTV定义 */
#define DEV_CFG_PRODUCT_AI_GLASSES                    0x04
#define DEV_CFG_AI_GLASSES_VERSION                    0x00
#define DEV_CFG_TYPE_VIDEO                            0x05
#define DEV_CFG_VIDEO_SUPPORT                         BIT(0)
#define DEV_CFG_VIDEO_PHOTO                           BIT(1)     // Byte0 Bit1: 拍照(0x38 op:0x01)
#define DEV_CFG_VIDEO_RECORD                          BIT(2)     // Byte0 Bit2: 录像(0x38 op:0x13)
#define DEV_CFG_VIDEO_STREAM                          BIT(3)     // Byte0 Bit3: 流媒体(0x37)
#define DEV_CFG_TYPE_TRANSLATOR                       0x03
#define DEV_CFG_TRANSLATOR_SUPPORT                    BIT(0)
#define DEV_CFG_TYPE_AUDIO                            0x06
#define DEV_CFG_AUDIO_MORE_CONFIG                     BIT(7)
#define DEV_CFG_AUDIO_SUPPORT                         BIT(0)
#define DEV_CFG_AUDIO_EQ                              BIT(1)     // Byte0 Bit1: 音效调节(EQ)
#define DEV_CFG_AUDIO_REALTIME_RECORD                 BIT(2)     // Byte0 Bit2: 实时录音(0x04/0x05)
#define DEV_CFG_AUDIO_FILE_RECORD                     BIT(5)     // Byte1 Bit5: 录音文件(mix recorder, 0x38)
#define DEV_CFG_AUDIO_MULTI_SCENE_TUNING              BIT(4)     // Byte1 Bit4: 多场景调音(公有模式3通道音量)
#define DEV_CFG_TYPE_FILE_MANAGER                     0x07
#define DEV_CFG_FILE_SUPPORT                          BIT(0)
#define DEV_CFG_FILE_BROWSE                           BIT(1)
#define DEV_CFG_FILE_LARGE_TRANSFER                   BIT(3)
#define DEV_CFG_TYPE_MASS_DATA                        0x08
#define DEV_CFG_MASS_DATA_SUPPORT                     BIT(0)
#define DEV_CFG_TYPE_AI                               0x09
#define DEV_CFG_AI_FUNC_SUPPORT                       BIT(0)     // Byte0 Bit0: 支持 AI 功能
#define DEV_CFG_AI_FUNC_CLOUD                         BIT(1)     // Byte0 Bit1: AI 云服务
#define DEV_CFG_AI_FUNC_DIAL                          BIT(2)     // Byte0 Bit2: AI 表盘
#define DEV_CFG_AI_FUNC_DIALOG                        BIT(3)     // Byte0 Bit3: AI 对话

// u16
#define DEV_CFG_OTA					BIT(0) //OTA
#define DEV_CFG_RES_UPDATE			BIT(1) //资源更新
#define DEV_CFG_WATCH_ADD			BIT(2) //表盘增加、删除、查询
#define DEV_CFG_WATCH_SWITCH		BIT(3) //表盘切换
#define DEV_CFG_WATCH_SHOW			BIT(4) //表盘预览

// u16
#define DEV_CFG_SCREEN_DISP			BIT(0) //屏幕设置
#define DEV_CFG_MOTOR_SET			BIT(1) //振动强度
#define DEV_CFG_UNDISTURB			BIT(2) //勿扰模式
#define DEV_CFG_TRAIN_SET			BIT(3) //锻炼设置
#define DEV_CFG_BT_SETTING			BIT(4) //蓝牙断开设置

// u16
#define DEV_CFG_CONTACTA			BIT(0) //常用联系人
#define DEV_CFG_MUSIC_BROWSE		BIT(1) //音乐文件浏览
#define DEV_CFG_MUSIC_TRANSFER		BIT(2) //音乐文件删除、传输
#define DEV_CFG_ALARM				BIT(3) //闹钟设置
#define DEV_CFG_NOTICE				BIT(4) //信息同步
#define DEV_CFG_WEATHER				BIT(5) //天气同步
#define DEV_CFG_FINDPHONE			BIT(6) //找手机
#define DEV_CFG_AI					BIT(7) //ai
#define DEV_CFG_AI_DIAL				BIT(8) //AI表盘
#define DEV_CFG_GET_DEV_INFO		BIT(9) //平台接口信息获取
#define DEV_CFG_UPDATE_CAT1			BIT(10) //是否支持4G模块
#define DEV_CFG_WATCH_DIAL_EXT		BIT(11) //是否支持表盘参数扩展
#define DEV_CFG_MEDIA_STREAM		BIT(12) //是否支持流媒体传输

// sport LTV
#define DEV_CFG_SPORT_TYPE_GSENSOR		0
#define DEV_CFG_GSENSOR_ONOFF				BIT(0) //传感器开关
#define DEV_CFG_GSENSOR_STEPS				BIT(1) //运动计步

#define DEV_CFG_SPORT_TYPE_HR			1
#define DEV_CFG_HR_ONOFF					BIT(0) //传感器开关
#define DEV_CFG_HR_CONTINUE					BIT(1) //连续心率

#define DEV_CFG_SPORT_TYPE_MR			2
#define DEV_CFG_MR_ONOFF					BIT(0) //传感器开关

#define DEV_CFG_SPORT_TYPE_PRESSURE		3
#define DEV_CFG_PRESSURE_ONOFF				BIT(0) //传感器开关

#define DEV_CFG_SPORT_TYPE_GPS			4
#define DEV_CFG_GPS_ONOFF					BIT(0) //传感器开关

#define DEV_CFG_SPORT_TYPE_MODE			0xFE
#define DEV_CFG_SPORT_RECORD				BIT(0) //运动记录
#define DEV_CFG_SPORT_STATISTICS			BIT(1) //运动统计
#define DEV_CFG_SPORT_OUTDOOR				BIT(2) //室外运动
#define DEV_CFG_SPORT_INDOOR				BIT(3) //室内运动


#define DEV_CFG_SPORT_TYPE_COMMON		0xFF
#define DEV_CFG_HEALTH						BIT(0) //健康监控
#define DEV_CFG_PIM							BIT(1) //个人信息
#define DEV_CFG_SLEEP						BIT(2) //睡眠监控
#define DEV_CFG_BURN_THE_PLANKS				BIT(3) //久坐提醒
#define DEV_CFG_SPORT_HR					BIT(4) //运动心率提醒
#define DEV_CFG_HEAT						BIT(5) //压力自动检测
#define DEV_CFG_FALL_DOWN					BIT(6) //跌倒检测
#define DEV_CFG_SENSOR_SET					BIT(7) //传感器设置



u32 rcsp_watch_get_device_config(void *priv, u8 *buf, u16 buf_size)
{
    u16 wlen = 0;
#if RCSP_TRANSLATOR_ENABLE
    u8 translator_config = DEV_CFG_TRANSLATOR_SUPPORT;
#endif
#if ((defined(TCFG_AI_GLASSES_SPECIAL_ENABLE)) && TCFG_AI_GLASSES_SPECIAL_ENABLE)|| RCSP_MIX_RECORD_ENABLE
    u8 audio_config[2] = {0};
#endif
#if (defined(TCFG_AI_GLASSES_SPECIAL_ENABLE)) && TCFG_AI_GLASSES_SPECIAL_ENABLE
    u8 file_config = 0;
    u8 video_config = 0;
    u8 ai_config = 0;
#if RCSP_AI_DIALOGUE_ENABLE
    u8 mass_data_config = 0;
#endif
#else
    u16 rlen;
    u8  u8val;
    u16 u16val;
#endif

    if (!buf || (buf_size < 2)) {
        return 0;
    }

#if (defined(TCFG_AI_GLASSES_SPECIAL_ENABLE)) && TCFG_AI_GLASSES_SPECIAL_ENABLE
    buf[wlen++] = DEV_CFG_PRODUCT_AI_GLASSES;
    buf[wlen++] = DEV_CFG_AI_GLASSES_VERSION;
#else
    buf[wlen++] = 0; //产品标识
    buf[wlen++] = 0; //版本号
#endif

#if !((defined(TCFG_AI_GLASSES_SPECIAL_ENABLE)) && TCFG_AI_GLASSES_SPECIAL_ENABLE)
    u16val = DEV_CFG_OTA | DEV_CFG_RES_UPDATE | DEV_CFG_WATCH_ADD | DEV_CFG_WATCH_SWITCH | DEV_CFG_WATCH_SHOW;
    if ((wlen + 2) <= buf_size) {
        memcpy(&buf[wlen], &u16val, 2);
        wlen += 2;
    }

    u16val = DEV_CFG_SCREEN_DISP | DEV_CFG_MOTOR_SET | DEV_CFG_UNDISTURB
#if TCFG_UI_ENABLE_TRAIN
             | DEV_CFG_TRAIN_SET
#endif /* #if TCFG_UI_ENABLE_TRAIN */
             | DEV_CFG_BT_SETTING;
    if ((wlen + 2) <= buf_size) {
        memcpy(&buf[wlen], &u16val, 2);
        wlen += 2;
    }

    u16val = DEV_CFG_CONTACTA
#if TCFG_UI_ENABLE_MUSIC
             | DEV_CFG_MUSIC_BROWSE | DEV_CFG_MUSIC_TRANSFER
#endif /* #if TCFG_UI_ENABLE_MUSIC */
#if TCFG_UI_ENABLE_ALARM
             | DEV_CFG_ALARM
#endif /* #if TCFG_UI_ENABLE_ALARM */
#if TCFG_UI_ENABLE_NOTICE
             | DEV_CFG_NOTICE
#endif /* #if TCFG_UI_ENABLE_NOTICE */
#if TCFG_UI_ENABLE_WEATHER
             | DEV_CFG_WEATHER
#endif /* #if TCFG_UI_ENABLE_WEATHER */
#if TCFG_UI_ENABLE_FINDPHONE
             | DEV_CFG_FINDPHONE
#endif /* #if TCFG_UI_ENABLE_FINDPHONE */

#if (defined(TCFG_AI_INTERACTION_ENABLE) && TCFG_AI_INTERACTION_ENABLE)
             | DEV_CFG_AI
             | DEV_CFG_AI_DIAL
#endif

#if (defined(TCFG_CAT1_MODULE_UPDATE_ENABLE) && TCFG_CAT1_MODULE_UPDATE_ENABLE)
             | DEV_CFG_UPDATE_CAT1
#endif
             | DEV_CFG_WATCH_DIAL_EXT
             | DEV_CFG_MEDIA_STREAM
             ;
    if ((wlen + 2) <= buf_size) {
        memcpy(&buf[wlen], &u16val, 2);
        wlen += 2;
    }

#if TCFG_GSENSOR_ENABLE
    u8val = DEV_CFG_GSENSOR_ONOFF | DEV_CFG_GSENSOR_STEPS;
    rlen = add_one_attr(buf, buf_size, wlen, DEV_CFG_SPORT_TYPE_GSENSOR, &u8val, 1);
    wlen += rlen;
#endif /* #if TCFG_GSENSOR_ENABLE */

#if TCFG_HR_SENSOR_ENABLE
    u8val = DEV_CFG_HR_ONOFF | DEV_CFG_HR_CONTINUE;
    rlen = add_one_attr(buf, buf_size, wlen, DEV_CFG_SPORT_TYPE_HR, &u8val, 1);
    wlen += rlen;

    u8val = DEV_CFG_MR_ONOFF;
    rlen = add_one_attr(buf, buf_size, wlen, DEV_CFG_SPORT_TYPE_MR, &u8val, 1);
    wlen += rlen;
#endif /* #if TCFG_HR_SENSOR_ENABLE */

#if (defined(TCFG_BMP280_ENABLE) && TCFG_BMP280_ENABLE)
    u8val = DEV_CFG_PRESSURE_ONOFF;
    rlen = add_one_attr(buf, buf_size, wlen, DEV_CFG_SPORT_TYPE_PRESSURE, &u8val, 1);
    wlen += rlen;
#endif /* #if TCFG_BMP280_ENABLE */

#if 0
    u8val = DEV_CFG_GPS_ONOFF;
    rlen = add_one_attr(buf, buf_size, wlen, DEV_CFG_SPORT_TYPE_GPS, &u8val, 1);
    wlen += rlen;
#endif

    u8val = 0
#if TCFG_UI_ENABLE_SPORTRECORD
            | DEV_CFG_SPORT_RECORD | DEV_CFG_SPORT_STATISTICS
#endif /* #if TCFG_UI_ENABLE_SPORTRECORD */
            | DEV_CFG_SPORT_OUTDOOR | DEV_CFG_SPORT_INDOOR;
    rlen = add_one_attr(buf, buf_size, wlen, DEV_CFG_SPORT_TYPE_MODE, &u8val, 1);
    wlen += rlen;

    u8val = DEV_CFG_HEALTH | DEV_CFG_PIM
#if TCFG_UI_ENABLE_SLEEP
            | DEV_CFG_SLEEP
#endif /* #if TCFG_UI_ENABLE_SLEEP */
            | DEV_CFG_BURN_THE_PLANKS | DEV_CFG_SPORT_HR
#if TCFG_UI_ENABLE_HEAT
            | DEV_CFG_HEAT
#endif /* #if TCFG_UI_ENABLE_HEAT */
            | DEV_CFG_FALL_DOWN
            | DEV_CFG_SENSOR_SET
            ;
    rlen = add_one_attr(buf, buf_size, wlen, DEV_CFG_SPORT_TYPE_COMMON, &u8val, 1);
    wlen += rlen;
#endif

#if RCSP_TRANSLATOR_ENABLE
    /* 仅声明当前翻译能力；译文走报文音频，不声明 A2DP、通话立体声或音视频翻译。 */
    wlen += add_one_attr(buf, buf_size, wlen, DEV_CFG_TYPE_TRANSLATOR,
                         &translator_config, sizeof(translator_config));
#endif

#if (defined(TCFG_AI_GLASSES_SPECIAL_ENABLE)) && TCFG_AI_GLASSES_SPECIAL_ENABLE
    /* 智能眼镜 0x06 音频能力恒上报(与 mix recorder 解耦)。
     * Byte0 Bit1(0x02) = 音效调节(EQ): App 依据该位开放 7 种音效模式设置
     * (公有模式 Bit4=EQ信息, 0x07/0x08/0x09, 见 rcsp_eq_setting.c)。
     * Byte1 Bit4(0x10) = 多场景调音: 手机 App 依据该位决定走"公有模式 Bit1 音量"
     * 的 [sys][media][call] 三通道交互(0x07/0x08/0x09, 见 rcsp_device_status.c)。
     * 录音文件能力(Byte1 Bit5)仍仅随 mix recorder 使能。 */
    audio_config[0] |= DEV_CFG_AUDIO_SUPPORT | DEV_CFG_AUDIO_EQ | DEV_CFG_AUDIO_MORE_CONFIG;
    audio_config[1] |= DEV_CFG_AUDIO_MULTI_SCENE_TUNING;
#if RCSP_MIX_RECORD_ENABLE
    audio_config[1] |= DEV_CFG_AUDIO_FILE_RECORD;
#endif
#if RCSP_AI_DIALOGUE_ENABLE
    /* Byte0 Bit2 = 实时录音(0x04/0x05): 场景 A 语音对话靠它上传麦克风音频。
     * 场景 B(识图)不使用录音, 由 ai_dialogue_control 侧拒绝 0xC004,
     * 与这一能力位无关 —— APP 是按状态序列判流程的。
     * 0x30 大数据通道用于识别文本/TTS 下发。 */
    audio_config[0] |= DEV_CFG_AUDIO_REALTIME_RECORD;
    mass_data_config = DEV_CFG_MASS_DATA_SUPPORT;
#endif
    if (audio_config[0] & DEV_CFG_AUDIO_SUPPORT) {
        wlen += add_one_attr(buf, buf_size, wlen, DEV_CFG_TYPE_AUDIO,
                             audio_config, sizeof(audio_config));
    }
#elif RCSP_MIX_RECORD_ENABLE
    /* 0x38录音文件流程只依赖mix recorder。 */
    audio_config[0] |= DEV_CFG_AUDIO_SUPPORT | DEV_CFG_AUDIO_MORE_CONFIG;
    audio_config[1] |= DEV_CFG_AUDIO_FILE_RECORD;
    if (audio_config[0] & DEV_CFG_AUDIO_SUPPORT) {
        wlen += add_one_attr(buf, buf_size, wlen, DEV_CFG_TYPE_AUDIO,
                             audio_config, sizeof(audio_config));
    }
#endif

#if (((defined(TCFG_AI_GLASSES_SPECIAL_ENABLE)) && TCFG_AI_GLASSES_SPECIAL_ENABLE)&&RCSP_FILE_OPT && TCFG_DEV_MANAGER_ENABLE)
    /* 0x38录音资源需要通过文件浏览和大文件传输读取。 */
    file_config = DEV_CFG_FILE_SUPPORT |
                  DEV_CFG_FILE_BROWSE |
                  DEV_CFG_FILE_LARGE_TRANSFER;
    wlen += add_one_attr(buf, buf_size, wlen, DEV_CFG_TYPE_FILE_MANAGER,
                         &file_config, sizeof(file_config));
#endif

#if (defined(TCFG_AI_GLASSES_SPECIAL_ENABLE)) && TCFG_AI_GLASSES_SPECIAL_ENABLE
    /* 0x05 视频能力：拍照/录像均通过 0x38 命令实现。
     * 流媒体(Bit3)当前未启用 0x37 命令，故不置位。 */
#if (TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE))
    video_config = DEV_CFG_VIDEO_SUPPORT | DEV_CFG_VIDEO_PHOTO | DEV_CFG_VIDEO_RECORD;
#endif
    if (video_config & DEV_CFG_VIDEO_SUPPORT) {
        wlen += add_one_attr(buf, buf_size, wlen, DEV_CFG_TYPE_VIDEO,
                             &video_config, sizeof(video_config));
    }

    /* 0x09 AI 功能：Byte0 Bit0=支持 AI 功能，Bit3=支持 AI 对话。
     * 识图流程(场景 B)依赖 APP 依据 Bit3 下发 0x32 ENTER。 */
    ai_config = DEV_CFG_AI_FUNC_SUPPORT | DEV_CFG_AI_FUNC_DIALOG;
    wlen += add_one_attr(buf, buf_size, wlen, DEV_CFG_TYPE_AI,
                         &ai_config, sizeof(ai_config));

#if RCSP_AI_DIALOGUE_ENABLE
    wlen += add_one_attr(buf, buf_size, wlen, DEV_CFG_TYPE_MASS_DATA,
                         &mass_data_config, sizeof(mass_data_config));
#endif
#endif

    return wlen;
}
#endif//SMART_BOX_EN


