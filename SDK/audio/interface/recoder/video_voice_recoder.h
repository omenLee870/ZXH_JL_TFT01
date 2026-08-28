
#ifndef VIDEO_VOICE_RECODER_H
#define VIDEO_VOICE_RECODER_H


#include "asm/cpu.h"

int video_voice_recoder_open(u32 code_type, u8 ai_type, u32 sample_rate, u32 bit_rate);

void video_voice_recoder_close();

void video_voice_recoder_set_video_tx_node_func(int (*func)(u8 *, u32));





#endif
