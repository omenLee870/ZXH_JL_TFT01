#ifndef __RCSP_AUDIO_VIDEO_CONTROL_H__
#define __RCSP_AUDIO_VIDEO_CONTROL_H__

#include "typedef.h"
#include "JL_rcsp_protocol.h"

/* 0x38 Operation */
enum rcsp_audio_video_operation {
    RCSP_AV_OP_RESOURCE_NOTIFY       = 0x00,
    RCSP_AV_OP_TAKE_PHOTO            = 0x01,

    RCSP_AV_OP_VIDEO_CONFIG_GET      = 0x10,
    RCSP_AV_OP_VIDEO_CONFIG_SET      = 0x11,
    RCSP_AV_OP_VIDEO_STATE_NOTIFY    = 0x12,
    RCSP_AV_OP_VIDEO_START           = 0x13,
    RCSP_AV_OP_VIDEO_STOP            = 0x14,
    RCSP_AV_OP_VIDEO_PAUSE           = 0x15,
    RCSP_AV_OP_VIDEO_STATE_SYNC      = 0x16,

    RCSP_AV_OP_AUDIO_CONFIG_GET      = 0x20,
    RCSP_AV_OP_AUDIO_CONFIG_SET      = 0x21,
    RCSP_AV_OP_AUDIO_START           = 0x22,
    RCSP_AV_OP_AUDIO_STOP            = 0x23,
    RCSP_AV_OP_AUDIO_STATE_NOTIFY    = 0x24,
    RCSP_AV_OP_AUDIO_STATE_SYNC      = 0x25,
};

/* Operation Response Data result */
enum rcsp_audio_video_result {
    RCSP_AV_RESULT_SUCCESS           = 0x00,
    RCSP_AV_RESULT_BUSY              = 0x01,
    RCSP_AV_RESULT_UNSUPPORTED       = 0x02,
    RCSP_AV_RESULT_IN_PROGRESS       = 0x03,
};

enum rcsp_audio_video_stop_reason {
    RCSP_AV_STOP_ACTIVE              = 0x00,
    RCSP_AV_STOP_TIMEOUT             = 0x01,
    RCSP_AV_STOP_STORAGE_OFFLINE     = 0x02,
    RCSP_AV_STOP_IO_ERROR            = 0x03,
};

enum rcsp_video_state_value {
    RCSP_VIDEO_STATE_STOPPED         = 0x00,
    RCSP_VIDEO_STATE_RECORDING       = 0x01,
    RCSP_VIDEO_STATE_PAUSED          = 0x02,
};

enum rcsp_audio_state_value {
    RCSP_AUDIO_STATE_STOPPED         = 0x00,
    RCSP_AUDIO_STATE_RECORDING       = 0x01,
};

enum rcsp_audio_video_config_type {
    RCSP_AV_CFG_RECORD_DURATION      = 0x01,
    RCSP_VIDEO_CFG_ORIENTATION       = 0x02,
    RCSP_VIDEO_CFG_LOCATION_ENABLE   = 0x03,
};

#define RCSP_AV_DEVICE_NUMBER_DEFAULT    0xFF

struct rcsp_audio_video_state {
    u8 session_id;
    u8 state;
    u32 duration_sec;
};

struct rcsp_audio_video_resource {
    u8 operation;
    u8 session_id;
    u8 file_type;
    u32 storage_handle;
    u32 cluster;
    u32 file_size;
    u16 crc;
    const u8 *file_path;
    u8 file_path_len;
};

/*
 * Callbacks return enum rcsp_audio_video_result.
 * get_xxx_config writes an LTV array to buf and returns its size in out_len.
 * An omitted device number is reported as RCSP_AV_DEVICE_NUMBER_DEFAULT.
 */
struct rcsp_audio_video_ops {
    u8(*take_photo)(u8 session_id, u8 device_number);

    u8(*get_video_config)(u8 *buf, u16 buf_size, u16 *out_len);
    u8(*set_video_config)(const u8 *ltv, u16 len);
    u8(*start_video)(u8 session_id, u8 device_number);
    u8(*stop_video)(u8 session_id, u8 reason);
    u8(*pause_video)(u8 session_id);
    u8(*get_video_state)(struct rcsp_audio_video_state *state);

    u8(*get_audio_config)(u8 *buf, u16 buf_size, u16 *out_len);
    u8(*set_audio_config)(const u8 *ltv, u16 len);
    u8(*start_audio)(u8 session_id, u8 device_number);
    u8(*stop_audio)(u8 session_id, u8 reason);
    u8(*get_audio_state)(struct rcsp_audio_video_state *state);
    u8(*take_audio_resource)(struct rcsp_audio_video_resource *resource);
    void (*audio_start_response_sent)(u8 result);
    void (*audio_stop_response)(u8 status, u8 result);
};

/*
 * 按字段合并注册: 每个模块只填自己关心的回调, 未填(NULL)的保持原值,
 * 因此多个模块可以各自注册而不互相覆盖。传 NULL 表示整体注销。
 */
void rcsp_audio_video_register_ops(const struct rcsp_audio_video_ops *ops);

void rcsp_audio_video_cmd_deal(void *priv, u8 OpCode, u8 OpCode_SN,
                               u8 *data, u16 len, u16 ble_con_handle,
                               u8 *spp_remote_addr);

JL_ERR rcsp_audio_video_notify_resource(const struct rcsp_audio_video_resource *resource);
JL_ERR rcsp_audio_video_notify_video_state(const struct rcsp_audio_video_state *state);
JL_ERR rcsp_audio_video_notify_audio_state(const struct rcsp_audio_video_state *state);
JL_ERR rcsp_audio_video_request_audio_stop(u8 session_id, u8 reason);
void rcsp_audio_video_cmd_response(u8 status, u8 *data, u16 len);

#endif /* __RCSP_AUDIO_VIDEO_CONTROL_H__ */
