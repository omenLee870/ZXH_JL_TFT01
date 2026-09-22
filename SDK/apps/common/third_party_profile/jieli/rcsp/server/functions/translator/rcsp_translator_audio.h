/* Bounded OPUS downlink adapter using the SDK AI_RX player. */
#ifndef RCSP_TRANSLATOR_AUDIO_H
#define RCSP_TRANSLATOR_AUDIO_H
#include "typedef.h"

int rcsp_translator_audio_prepare(u8 mode);
void rcsp_translator_audio_close(void);
int rcsp_translator_audio_receive(u8 source, const u8 *data, u16 len, u8 count);
u32 rcsp_translator_audio_cache(u8 source);
void rcsp_translator_audio_record_result(u8 success);
void rcsp_translator_audio_set_link(u16 handle, const u8 *addr);
int rcsp_translator_audio_request_record(u8 start);
void rcsp_translator_audio_cancel_record_request(void);
void rcsp_translator_audio_cmd_response(u8 opcode, u8 status, u16 handle,
                                        const u8 *addr);
#endif
