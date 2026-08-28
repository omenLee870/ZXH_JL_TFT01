#ifndef _AUDIO_GENERAL_CONFIG_H_
#define _AUDIO_GENERAL_CONFIG_H_

#include "generic/typedef.h"
#include "sdk_config.h"

#if defined(TCFG_MIDI_CONTROLLER_NODE_ENABLE) && TCFG_MIDI_CONTROLLER_NODE_ENABLE
#define MIDI_CTRL_OPORT_NUM    4 // midi输出输出的通道数 1:(旧的用法) ,4:(新的用法配合MIDI 节点使用)
#else
#define MIDI_CTRL_OPORT_NUM    1 // midi输出输出的通道数 1:(旧的用法) ,4:(新的用法配合MIDI 节点使用)
#endif


#ifndef ALINK0_SAMPLE_RATE
#define ALINK0_SAMPLE_RATE                        0                // Sample Rate
#endif
#ifndef ALINK0_CH_NUM
#define ALINK0_CH_NUM                             2                // Channels
#endif
#ifndef ALINK0_WORK_MODE
#define ALINK0_WORK_MODE                          1                // Work Mode
#endif
#ifndef ALINK0_CLK_MODE
#define ALINK0_CLK_MODE                           0                // Clk Mode
#endif
#ifndef ALINK0_BIT_WIDTH
#define ALINK0_BIT_WIDTH                          -1               // Bit Width
#endif
#ifndef ALINK0_IO_DRIVE_STRENGTH_MODE
#define ALINK0_IO_DRIVE_STRENGTH_MODE            0xff
#endif
#ifndef ALINK1_SAMPLE_RATE
#define ALINK1_SAMPLE_RATE                        0                // Sample Rate
#endif
#ifndef ALINK1_CH_NUM
#define ALINK1_CH_NUM                             2                // Channels
#endif
#ifndef ALINK1_WORK_MODE
#define ALINK1_WORK_MODE                          1                // Work Mode
#endif
#ifndef ALINK1_CLK_MODE
#define ALINK1_CLK_MODE                           0                // Clk Mode
#endif
#ifndef ALINK1_BIT_WIDTH
#define ALINK1_BIT_WIDTH                          -1               // Bit Width
#endif
#ifndef ALINK1_IO_DRIVE_STRENGTH_MODE
#define ALINK1_IO_DRIVE_STRENGTH_MODE            0xff
#endif

#if (TCFG_AUDIO_CVP_3MIC_MODE || \
     TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE || \
     TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE || \
     TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE || \
     TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE || \
     TCFG_AUDIO_CVP_DMS_DNS_MODE || \
     TCFG_AUDIO_CVP_DMS_ANS_MODE || \
     TCFG_AUDIO_CVP_SMS_DNS_MODE || \
     TCFG_AUDIO_CVP_SMS_ANS_MODE || \
     TCFG_AUDIO_CVP_V3_MODE || \
     TCFG_AUDIO_CVP_DEVELOP_ENABLE)

#define AUDIO_CVP_ALGO_ENABLE 1

#endif

#if (TCFG_AUDIO_CVP_3MIC_MODE || \
     TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE || \
     TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE || \
     TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE || \
     TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE || \
     TCFG_AUDIO_CVP_DMS_DNS_MODE || \
     TCFG_AUDIO_CVP_DMS_ANS_MODE || \
     TCFG_AUDIO_CVP_SMS_DNS_MODE || \
     TCFG_AUDIO_CVP_SMS_ANS_MODE || \
	 TCFG_AUDIO_CVP_SMS_VF_MODE)

#define AUDIO_CVP_GEN1_ALGO_ENABLE 1
#endif
#endif
