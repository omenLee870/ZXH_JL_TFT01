#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_video_control.data.bss")
#pragma data_seg(".audio_video_control.data")
#pragma const_seg(".audio_video_control.text.const")
#pragma code_seg(".audio_video_control.text")
#endif

#include "system/includes.h"
#include "rcsp_config.h"
#include "rcsp_functions/rcsp_define.h"
#include "audio_video_control.h"

#define LOG_TAG_CONST       RCSP
#define LOG_TAG             "[rcsp av ctrl]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#if RCSP_MODE

static struct rcsp_audio_video_ops g_audio_video_ops;

static u8 rcsp_audio_video_result_normalize(u8 result)
{
    if (result > RCSP_AV_RESULT_IN_PROGRESS) {
        return RCSP_AV_RESULT_UNSUPPORTED;
    }
    return result;
}

static u8 rcsp_audio_video_result_to_status(u8 result)
{
    switch (rcsp_audio_video_result_normalize(result)) {
    case RCSP_AV_RESULT_SUCCESS:
        return JL_PRO_STATUS_SUCCESS;
    case RCSP_AV_RESULT_BUSY:
    case RCSP_AV_RESULT_IN_PROGRESS:
        return JL_PRO_STATUS_BUSY;
    default:
        return JL_PRO_STATUS_PARAM_ERR;
    }
}

static bool rcsp_audio_video_ltv_valid(const u8 *data, u16 len)
{
    u16 offset = 0;

    if (!data || !len) {
        return false;
    }

    while (offset < len) {
        u8 ltv_len = data[offset];

        /* Length contains Type + Value, but does not contain itself. */
        if (ltv_len < 1 || (u32)offset + 1 + ltv_len > len) {
            return false;
        }
        offset += 1 + ltv_len;
    }

    return offset == len;
}

static void rcsp_audio_video_response(u8 OpCode, u8 OpCode_SN, u8 status,
                                      u8 *resp, u16 resp_len,
                                      u16 ble_con_handle, u8 *spp_remote_addr)
{
    JL_CMD_response_send(OpCode, status, OpCode_SN, resp, resp_len,
                         ble_con_handle, spp_remote_addr);
}

void rcsp_audio_video_register_ops(const struct rcsp_audio_video_ops *ops)
{
    memset(&g_audio_video_ops, 0, sizeof(g_audio_video_ops));
    if (ops) {
        memcpy(&g_audio_video_ops, ops, sizeof(g_audio_video_ops));
    }
}

void rcsp_audio_video_cmd_deal(void *priv, u8 OpCode, u8 OpCode_SN,
                               u8 *data, u16 len, u16 ble_con_handle,
                               u8 *spp_remote_addr)
{
    u8 status = JL_PRO_STATUS_SUCCESS;
    u8 result = RCSP_AV_RESULT_UNSUPPORTED;
    u8 operation;
    u8 *resp;
    u8 audio_start_result_valid = 0;
#if RCSP_AUDIO_VIDEO_RESPONSE_FIRST
    u8 notify_audio_state = 0;
#endif
    u16 resp_len = 1;
    u16 config_len = 0;
    struct rcsp_audio_video_state av_state = {0};
#if RCSP_AUDIO_VIDEO_RESPONSE_FIRST
    struct rcsp_audio_video_resource av_resource = {0};
#endif

    (void)priv;

    if (!data || len < 1) {
        rcsp_audio_video_response(OpCode, OpCode_SN, JL_PRO_STATUS_PARAM_ERR,
                                  NULL, 0, ble_con_handle, spp_remote_addr);
        return;
    }

    operation = data[0];
    resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    if (!resp) {
        rcsp_audio_video_response(OpCode, OpCode_SN, JL_PRO_STATUS_FAIL,
                                  NULL, 0, ble_con_handle, spp_remote_addr);
        return;
    }
    resp[0] = operation;

    switch (operation) {
    case RCSP_AV_OP_TAKE_PHOTO:
        if (len != 2 && len != 3) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (g_audio_video_ops.take_photo) {
            result = g_audio_video_ops.take_photo(data[1],
                                                  len == 3 ? data[2] : RCSP_AV_DEVICE_NUMBER_DEFAULT);
        }
        resp[1] = rcsp_audio_video_result_normalize(result);
        resp_len = 2;
        break;

    case RCSP_AV_OP_VIDEO_CONFIG_GET:
        if (len != 1) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (!g_audio_video_ops.get_video_config) {
            status = JL_PRO_STATUS_UNKOWN_CMD;
        } else {
            result = g_audio_video_ops.get_video_config(resp + 1,
                     TARGET_FEATURE_RESP_BUF_SIZE - 1,
                     &config_len);
            if (config_len > TARGET_FEATURE_RESP_BUF_SIZE - 1) {
                status = JL_PRO_STATUS_RESP_DATA_OVER_LIMIT;
                config_len = 0;
            } else {
                status = rcsp_audio_video_result_to_status(result);
            }
            resp_len += config_len;
        }
        break;

    case RCSP_AV_OP_VIDEO_CONFIG_SET:
        if (len < 2 || !rcsp_audio_video_ltv_valid(data + 1, len - 1)) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (g_audio_video_ops.set_video_config) {
            result = g_audio_video_ops.set_video_config(data + 1, len - 1);
        }
        resp[1] = rcsp_audio_video_result_normalize(result);
        resp_len = 2;
        break;

    case RCSP_AV_OP_VIDEO_START:
        if (len != 2 && len != 3) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (g_audio_video_ops.start_video) {
            result = g_audio_video_ops.start_video(data[1],
                                                   len == 3 ? data[2] : RCSP_AV_DEVICE_NUMBER_DEFAULT);
        }
        resp[1] = rcsp_audio_video_result_normalize(result);
        resp_len = 2;
        break;

    case RCSP_AV_OP_VIDEO_STOP:
        if (len != 3 || data[2] > RCSP_AV_STOP_IO_ERROR) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (g_audio_video_ops.stop_video) {
            result = g_audio_video_ops.stop_video(data[1], data[2]);
        }
        resp[1] = rcsp_audio_video_result_normalize(result);
        resp_len = 2;
        break;

    case RCSP_AV_OP_VIDEO_PAUSE:
        if (len != 2) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (g_audio_video_ops.pause_video) {
            result = g_audio_video_ops.pause_video(data[1]);
        }
        resp[1] = rcsp_audio_video_result_normalize(result);
        resp_len = 2;
        break;

    case RCSP_AV_OP_VIDEO_STATE_SYNC:
        if (len != 1) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (!g_audio_video_ops.get_video_state) {
            status = JL_PRO_STATUS_UNKOWN_CMD;
        } else {
            result = g_audio_video_ops.get_video_state(&av_state);
            status = rcsp_audio_video_result_to_status(result);
            if (status == JL_PRO_STATUS_SUCCESS) {
                resp[1] = av_state.session_id;
                resp[2] = av_state.state;
                WRITE_BIG_U32(resp + 3, av_state.duration_sec);
                resp_len = 7;
            }
        }
        break;

    case RCSP_AV_OP_AUDIO_CONFIG_GET:
        if (len != 1) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (!g_audio_video_ops.get_audio_config) {
            status = JL_PRO_STATUS_UNKOWN_CMD;
        } else {
            result = g_audio_video_ops.get_audio_config(resp + 1,
                     TARGET_FEATURE_RESP_BUF_SIZE - 1,
                     &config_len);
            if (config_len > TARGET_FEATURE_RESP_BUF_SIZE - 1) {
                status = JL_PRO_STATUS_RESP_DATA_OVER_LIMIT;
                config_len = 0;
            } else {
                status = rcsp_audio_video_result_to_status(result);
            }
            resp_len += config_len;
        }
        break;

    case RCSP_AV_OP_AUDIO_CONFIG_SET:
        if (len < 2 || !rcsp_audio_video_ltv_valid(data + 1, len - 1)) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (g_audio_video_ops.set_audio_config) {
            result = g_audio_video_ops.set_audio_config(data + 1, len - 1);
        }
        resp[1] = rcsp_audio_video_result_normalize(result);
        resp_len = 2;
        break;

    case RCSP_AV_OP_AUDIO_START:
        if (len != 2 && len != 3) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (g_audio_video_ops.start_audio) {
            result = g_audio_video_ops.start_audio(data[1],
                                                   len == 3 ? data[2] : RCSP_AV_DEVICE_NUMBER_DEFAULT);
            audio_start_result_valid = 1;
#if RCSP_AUDIO_VIDEO_RESPONSE_FIRST
            if (!g_audio_video_ops.audio_start_response_sent) {
                notify_audio_state = (result == RCSP_AV_RESULT_SUCCESS);
            }
#endif
        }
        resp[1] = rcsp_audio_video_result_normalize(result);
        resp_len = 2;
        break;

    case RCSP_AV_OP_AUDIO_STOP:
        if (len != 3 || data[2] > RCSP_AV_STOP_IO_ERROR) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (g_audio_video_ops.stop_audio) {
            result = g_audio_video_ops.stop_audio(data[1], data[2]);
#if RCSP_AUDIO_VIDEO_RESPONSE_FIRST
            notify_audio_state = (result == RCSP_AV_RESULT_SUCCESS);
#endif
        }
        resp[1] = rcsp_audio_video_result_normalize(result);
        resp_len = 2;
        break;

    case RCSP_AV_OP_AUDIO_STATE_SYNC:
        if (len != 1) {
            status = JL_PRO_STATUS_PARAM_ERR;
        } else if (!g_audio_video_ops.get_audio_state) {
            status = JL_PRO_STATUS_UNKOWN_CMD;
        } else {
            result = g_audio_video_ops.get_audio_state(&av_state);
            status = rcsp_audio_video_result_to_status(result);
            if (status == JL_PRO_STATUS_SUCCESS) {
                resp[1] = av_state.session_id;
                resp[2] = av_state.state;
                WRITE_BIG_U32(resp + 3, av_state.duration_sec);
                resp_len = 7;
            }
        }
        break;

    /* These are DEVICE -> SDK notifications and are not valid downlink commands. */
    case RCSP_AV_OP_RESOURCE_NOTIFY:
    case RCSP_AV_OP_VIDEO_STATE_NOTIFY:
    case RCSP_AV_OP_AUDIO_STATE_NOTIFY:
    default:
        status = JL_PRO_STATUS_UNKOWN_CMD;
        break;
    }

    log_info("op=0x%x, status=%d, resp_len=%d\n", operation, status, resp_len);
    rcsp_audio_video_response(OpCode, OpCode_SN, status, resp, resp_len,
                              ble_con_handle, spp_remote_addr);
    free(resp);

    if (audio_start_result_valid && g_audio_video_ops.audio_start_response_sent) {
        g_audio_video_ops.audio_start_response_sent(result);
    }

#if RCSP_AUDIO_VIDEO_RESPONSE_FIRST
    if (notify_audio_state && g_audio_video_ops.get_audio_state &&
        g_audio_video_ops.get_audio_state(&av_state) == RCSP_AV_RESULT_SUCCESS) {
        rcsp_audio_video_notify_audio_state(&av_state);
    }

    if (operation == RCSP_AV_OP_AUDIO_STOP && notify_audio_state &&
        g_audio_video_ops.take_audio_resource &&
        g_audio_video_ops.take_audio_resource(&av_resource) == RCSP_AV_RESULT_SUCCESS) {
        rcsp_audio_video_notify_resource(&av_resource);
    }
#endif
}

static JL_ERR rcsp_audio_video_notify_state(u8 operation,
        const struct rcsp_audio_video_state *state)
{
    u8 data[7];

    if (!state) {
        return JL_ERR_EXIT;
    }

    data[0] = operation;
    data[1] = state->session_id;
    data[2] = state->state;
    WRITE_BIG_U32(data + 3, state->duration_sec);

    return JL_CMD_send(JL_OPCODE_AUDIO_VIDEO_CONTROL, data, sizeof(data),
                       JL_NOT_NEED_RESPOND, 0, NULL);
}

JL_ERR rcsp_audio_video_notify_video_state(const struct rcsp_audio_video_state *state)
{
    if (!state || state->state > RCSP_VIDEO_STATE_PAUSED) {
        return JL_ERR_EXIT;
    }
    return rcsp_audio_video_notify_state(RCSP_AV_OP_VIDEO_STATE_NOTIFY, state);
}

JL_ERR rcsp_audio_video_notify_audio_state(const struct rcsp_audio_video_state *state)
{
    if (!state || state->state > RCSP_AUDIO_STATE_RECORDING) {
        return JL_ERR_EXIT;
    }
    return rcsp_audio_video_notify_state(RCSP_AV_OP_AUDIO_STATE_NOTIFY, state);
}

JL_ERR rcsp_audio_video_request_audio_stop(u8 session_id, u8 reason)
{
    u8 data[3];

    if (reason > RCSP_AV_STOP_IO_ERROR) {
        return JL_ERR_EXIT;
    }

    data[0] = RCSP_AV_OP_AUDIO_STOP;
    data[1] = session_id;
    data[2] = reason;
    return JL_CMD_send(JL_OPCODE_AUDIO_VIDEO_CONTROL, data, sizeof(data),
                       JL_NEED_RESPOND, 0, NULL);
}

void rcsp_audio_video_cmd_response(u8 status, u8 *data, u16 len)
{
    u8 result = RCSP_AV_RESULT_UNSUPPORTED;

    if (!g_audio_video_ops.audio_stop_response) {
        return;
    }
    if (len && (!data || data[0] != RCSP_AV_OP_AUDIO_STOP)) {
        return;
    }
    if (status == JL_PRO_STATUS_SUCCESS && data && len >= 2) {
        result = rcsp_audio_video_result_normalize(data[1]);
    }
    g_audio_video_ops.audio_stop_response(status, result);
}

JL_ERR rcsp_audio_video_notify_resource(const struct rcsp_audio_video_resource *resource)
{
    JL_ERR err;
    u8 *data;
    u16 data_len;

    if (!resource || (resource->file_path_len && !resource->file_path)) {
        return JL_ERR_EXIT;
    }

    /* Outer operation(1) + resource information fixed fields(18) + path. */
    data_len = 19 + resource->file_path_len;
    data = malloc(data_len);
    if (!data) {
        return JL_ERR_EXIT;
    }

    data[0] = RCSP_AV_OP_RESOURCE_NOTIFY;
    data[1] = resource->operation;
    data[2] = resource->session_id;
    data[3] = resource->file_type;
    WRITE_BIG_U32(data + 4, resource->storage_handle);
    WRITE_BIG_U32(data + 8, resource->cluster);
    WRITE_BIG_U32(data + 12, resource->file_size);
    WRITE_BIG_U16(data + 16, resource->crc);
    data[18] = resource->file_path_len;
    if (resource->file_path_len) {
        memcpy(data + 19, resource->file_path, resource->file_path_len);
    }

    err = JL_CMD_send(JL_OPCODE_AUDIO_VIDEO_CONTROL, data, data_len,
                      JL_NOT_NEED_RESPOND, 0, NULL);
    free(data);
    return err;
}

#endif /* RCSP_MODE */
