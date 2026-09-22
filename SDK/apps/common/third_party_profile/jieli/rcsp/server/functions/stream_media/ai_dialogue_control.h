#ifndef __RCSP_AI_DIALOGUE_CONTROL_H__
#define __RCSP_AI_DIALOGUE_CONTROL_H__

#include "typedef.h"

/* Except A2DP stream notifications, use these entry points in app_core.
 * RCSP commands/responses, link-state cleanup and key/photo handlers already
 * run there in the SDK. reset() also serves pre-protocol initialization. */
enum rcsp_ai_dialogue_state {
    RCSP_AI_DIALOGUE_STATE_IDLE = 0,
    RCSP_AI_DIALOGUE_STATE_WAITING,
    RCSP_AI_DIALOGUE_STATE_RECORDING,
    RCSP_AI_DIALOGUE_STATE_PROCESSING,
    RCSP_AI_DIALOGUE_STATE_UPLOADING,
    RCSP_AI_DIALOGUE_STATE_PLAYING,
    RCSP_AI_DIALOGUE_STATE_CAPTURING,
};

void rcsp_ai_dialogue_cmd_deal(void *priv, u8 OpCode, u8 OpCode_SN,
                               u8 *data, u16 len, u16 ble_con_handle,
                               u8 *spp_remote_addr);
void rcsp_ai_dialogue_cmd_response(u8 status, u8 *data, u16 len);

/* 调试触发入口：语音对话(场景A) / 按键识图(场景B)。
 * 将来接入本地唤醒词后, 语音入口应由唤醒事件调用, 不再依赖按键。 */
int rcsp_ai_dialogue_key_trigger(void);
int rcsp_ai_dialogue_photo_trigger(void);

void rcsp_ai_dialogue_reset(void);
void rcsp_ai_dialogue_disconnect(void);
int rcsp_ai_dialogue_is_busy(void);
int rcsp_ai_dialogue_record_start_allowed(void);

/* Pause phone A2DP music before opening an AI dialogue recording stream. */
void rcsp_ai_dialogue_pause_a2dp_before_record(void);

/* Existing 0x04/0x05 real-time recording path reports state here. */
void rcsp_ai_dialogue_record_start_result(u8 success);
void rcsp_ai_dialogue_record_stop_result(u8 success);
int rcsp_ai_dialogue_record_force_stop(void);

/* Existing A2DP player reports the APP-side TTS playback lifecycle here. */
void rcsp_ai_dialogue_a2dp_play_start(void);
void rcsp_ai_dialogue_a2dp_play_stop(void);

#endif /* __RCSP_AI_DIALOGUE_CONTROL_H__ */
