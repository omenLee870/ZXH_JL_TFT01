/*****************************************************************
>file name : smart_voice.h
>create time : Thu 17 Jun 2021 02:07:32 PM CST
*****************************************************************/
#ifndef _SMART_VOICE_H_
#define _SMART_VOICE_H_
#include "media/includes.h"
#include "app_config.h"
#include "audio_adc.h"

/*是否支持VAD唤醒*/
#define CONFIG_VAD_PLATFORM_SUPPORT_EN  0

/*
 * 1、使能VAD唤醒
 * 2、使能AEC
 * 3、媒体数据流为IIS输出
 * 需要打开该宏，并配置对应的ALINK模块以及通道
 */
#define CONFIG_VAD_AEC_REF_SUPPORT_IIS          0 // 是否获取IIS回采数据
#define CONFIG_VAD_AEC_REF_SUPPORT_IIS_MODULE   0 // 模块0/1
#define CONFIG_VAD_AEC_REF_SUPPORT_IIS_CH       0 // 通道0/1/2/3

/*若支持AEC，需要填写媒体音频流的 DAC/IIS 节点名称*/
#define SMART_VOICE_AEC_REF_NODE_NAME    "DAC3"

#define ASR_CORE            "audio_vad"

#define ASR_CORE_WAKEUP     0
#define ASR_CORE_STANDBY    1

#define ASR_CLK             24000000

enum {
    SMART_VOICE_MSG_WAKE = 0,            /*唤醒*/
    SMART_VOICE_MSG_STANDBY,             /*待机*/
    SMART_VOICE_MSG_DMA,                 /*语音数据DMA传输*/
    SMART_VOICE_MSG_MIC_OPEN,            /*MIC打开*/
    SMART_VOICE_MSG_SWITCH_SOURCE,       /*MIC切换*/
    SMART_VOICE_MSG_MIC_CLOSE,           /*MIC关闭*/
};

/*
*********************************************************************
*       audio smart voice detect create
* Description: 创建智能语音检测引擎
* Arguments  : model         - KWS模型
*			   task_name     - VAD引擎task
*			   mic           - MIC的选择（低功耗MIC或主控MIC）
*			   buffer_size   - MIC的DMA数据缓冲大小
* Return	 : 0 - 创建成功，非0 - 失败.
* Note(s)    : None.
*********************************************************************
*/
int audio_smart_voice_detect_create(u8 model, u8 mic, int buffer_size);

/*
*********************************************************************
*       audio smart voice detect open
* Description: 智能语音检测打开接口
* Arguments  : mic           - MIC的选择（低功耗MIC或主控MIC）
* Return	 : void.
* Note(s)    : None.
*********************************************************************
*/
void audio_smart_voice_detect_open(u8 mic);

/*
*********************************************************************
*       audio smart voice detect close
* Description: 智能语音检测关闭接口
* Arguments  : void.
* Return	 : void.
* Note(s)    : 关闭引擎的所有资源（无论使用哪个mic）.
*********************************************************************
*/
void audio_smart_voice_detect_close(void);

/*
*********************************************************************
*       audio smart voice detect init
* Description: 智能语音检测配置初始化
* Arguments  : mic_data - P11 mic初始化配置.
* Return	 : void.
* Note(s)    : None.
*********************************************************************
*/
int audio_smart_voice_detect_init(void *vad_mic_data);

/*
*********************************************************************
*       audio phone call kws start
* Description: 启动通话来电关键词识别
* Arguments  : void.
* Return	 : 0 - 成功，非0 - 失败.
* Note(s)    : 接口会将VAD的低功耗mic切换至通话使用的主控mic.
*********************************************************************
*/
int audio_phone_call_kws_start(void);

/*
*********************************************************************
*       audio phone call kws close
* Description: 关闭通话来电关键词识别
* Arguments  : void.
* Return	 : 0 - 成功，非0 - 出错.
* Note(s)    : 关闭来电关键词识别，通常用于接通后或挂断/拒接后.
*********************************************************************
*/
int audio_phone_call_kws_close(void);


#define smart_voice_core_post_msg(num, ...)     os_taskq_post_msg(ASR_CORE, num, __VA_ARGS__)

int smart_voice_core_create(void *priv);

int smart_voice_core_free(void);

/*语音识别回音消除相关函数*/
void audio_smart_voice_aec_open_by_kws_task();
void audio_smart_voice_aec_close_by_kws_task();

#endif
