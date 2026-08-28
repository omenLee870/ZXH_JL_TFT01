#ifndef INTERCOM_VOICE_RECODER_H
#define INTERCOM_VOICE_RECODER_H



int intercom_voice_recoder_open(u32 code_type, u8 ai_type);

void intercom_voice_recoder_close();

void intercom_voice_recoder_set_intercom_tx_node_func(int (*func)(u8 *, u32));








#endif
