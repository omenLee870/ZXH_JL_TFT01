/* RCSP 0x34 translation protocol and SDK real-time microphone ownership. */
#ifndef RCSP_TRANSLATOR_H
#define RCSP_TRANSLATOR_H

#include "typedef.h"

enum rcsp_translator_operation {
    RCSP_TRANSLATOR_OP_GET_MODE = 0x00,
    RCSP_TRANSLATOR_OP_SET_MODE = 0x01,
    RCSP_TRANSLATOR_OP_NOTIFY_MODE = 0x02,
    RCSP_TRANSLATOR_OP_AUDIO = 0x03,
    RCSP_TRANSLATOR_OP_GET_CACHE = 0x04,
    RCSP_TRANSLATOR_OP_NOTIFY_CACHE = 0x05,
};

enum rcsp_translator_mode {
    RCSP_TRANSLATOR_MODE_IDLE = 0x00,
    RCSP_TRANSLATOR_MODE_RECORD = 0x01,
    RCSP_TRANSLATOR_MODE_RECORD_TRANSLATION = 0x02,
    RCSP_TRANSLATOR_MODE_CALL_TRANSLATION = 0x03,
    RCSP_TRANSLATOR_MODE_A2DP_TRANSLATION = 0x04,
    RCSP_TRANSLATOR_MODE_FACE_TO_FACE = 0x05,
    RCSP_TRANSLATOR_MODE_CALL_STEREO = 0x06,
    RCSP_TRANSLATOR_MODE_CALL_RECORD = 0x07,
    /* Reference EARBUDS_TO_EARBUDS, adapted to one glasses device (no TWS). */
    RCSP_TRANSLATOR_MODE_SIMULTANEOUS = 0x08,
};

enum rcsp_translator_encode {
    RCSP_TRANSLATOR_ENCODE_PCM = 0x00,
    RCSP_TRANSLATOR_ENCODE_SPEEX = 0x01,
    RCSP_TRANSLATOR_ENCODE_OPUS = 0x02,
    RCSP_TRANSLATOR_ENCODE_MSBC = 0x03,
    RCSP_TRANSLATOR_ENCODE_JLA_V2 = 0x04,
};

enum rcsp_translator_source {
    RCSP_TRANSLATOR_SOURCE_FILE = 0x00,
    RCSP_TRANSLATOR_SOURCE_DEVICE_MIC = 0x01,
    RCSP_TRANSLATOR_SOURCE_PHONE_MIC = 0x02,
    RCSP_TRANSLATOR_SOURCE_ESCO_UP = 0x03,
    RCSP_TRANSLATOR_SOURCE_ESCO_DOWN = 0x04,
    RCSP_TRANSLATOR_SOURCE_A2DP = 0x05,
    RCSP_TRANSLATOR_SOURCE_ESCO_MIX = 0x06,
};

enum rcsp_translator_result {
    RCSP_TRANSLATOR_RESULT_SUCCESS = 0x00,
    RCSP_TRANSLATOR_RESULT_IN_MODE = 0x01,
    RCSP_TRANSLATOR_RESULT_PARAM_ERR = 0x02,
    RCSP_TRANSLATOR_RESULT_CALL_BUSY = 0x03,
    RCSP_TRANSLATOR_RESULT_PLAY_BUSY = 0x04,
    RCSP_TRANSLATOR_RESULT_BUSY = 0x05,
    RCSP_TRANSLATOR_RESULT_FAIL = 0x06,
};

/* Host structure, not the wire layout. Serialize the seven bytes explicitly. */
struct rcsp_translator_mode_info {
    u8 mode;
    u8 encode;
    u8 channel;
    u32 sample_rate;
};

/* All runtime entry points run in app_core; reset also serves initialization. */
void rcsp_translator_reset(void);
void rcsp_translator_disconnect(void);
int rcsp_translator_is_busy(void);
void rcsp_translator_abort(void);
void rcsp_translator_record_result(u8 success);
void rcsp_translator_record_cancel_request(void);
/* Bridges implemented by the existing SDK 0x04/0x05 recording dispatcher. */
int rcsp_realtime_record_is_busy(void);
int rcsp_translator_record_force_stop(void);
int rcsp_translator_record_start(u16 handle, const u8 *addr);
/* Called in app_core by the device key and RCSP response dispatcher. */
int rcsp_translator_key_record(u8 start);
void rcsp_translator_cmd_response(u8 opcode, u8 status, u16 handle,
                                  const u8 *addr);
void rcsp_translator_record_poll_events(void);
int rcsp_translator_notify_mode(void);
void rcsp_translator_cmd_deal(u8 sn, const u8 *data, u16 len,
                              u8 need_response, u16 ble_con_handle,
                              u8 *spp_remote_addr);

#endif
