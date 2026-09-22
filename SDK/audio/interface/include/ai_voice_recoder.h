#ifndef AI_VOICE_RECODER_H
#define AI_VOICE_RECODER_H

#include "typedef.h"

int ai_voice_recoder_open(u32 code_type, u8 ai_type);

void ai_voice_recoder_close();

void ai_voice_recoder_set_ai_tx_node_func(int (*func)(u8 *, u32));

void ai_voice_recoder_set_event_callback(void (*callback)(int event));






#endif
