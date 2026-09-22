/* BR33 translation: recording, face-to-face and single-glasses simultaneous. */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_translator.data.bss")
#pragma data_seg(".rcsp_translator.data")
#pragma const_seg(".rcsp_translator.text.const")
#pragma code_seg(".rcsp_translator.text")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_functions/rcsp_define.h"
#include "rcsp_manage.h"
#include "JL_rcsp_protocol.h"
#include "rcsp_translator.h"
#include "rcsp_translator_audio.h"
#include "../stream_media/ai_dialogue_control.h"
#include "app_protocol_common.h"
#include "btstack/avctp_user.h"
#include "ai_rx_player.h"
#include "mix_record_api.h"
#include "rcsp_mix_record.h"

#if RCSP_MODE && RCSP_TRANSLATOR_ENABLE

#define TRANSLATOR_MODE_SIZE       7
#define TRANSLATOR_AUDIO_HEAD_SIZE 7

static u16 g_translator_ble_handle;
static u8 g_translator_spp_addr[6];
static u8 g_translator_spp_addr_valid;

static struct rcsp_translator_mode_info g_translator_mode = {
    .mode = RCSP_TRANSLATOR_MODE_IDLE,
    .encode = RCSP_TRANSLATOR_ENCODE_OPUS,
    .channel = 1,
    .sample_rate = 16000,
};

static u32 translator_read_be32(const u8 *data)
{
    return ((u32)data[0] << 24) | ((u32)data[1] << 16) |
           ((u32)data[2] << 8) | data[3];
}

static void translator_pack_mode(u8 *data)
{
    data[0] = g_translator_mode.mode;
    data[1] = g_translator_mode.encode;
    data[2] = g_translator_mode.channel;
    data[3] = g_translator_mode.sample_rate >> 24;
    data[4] = g_translator_mode.sample_rate >> 16;
    data[5] = g_translator_mode.sample_rate >> 8;
    data[6] = g_translator_mode.sample_rate;
}

void rcsp_translator_reset(void)
{
    /* Runtime reset releases only the translator's recorder/player. */
    rcsp_translator_record_force_stop();
    rcsp_translator_audio_close();
    g_translator_mode.mode = RCSP_TRANSLATOR_MODE_IDLE;
    g_translator_mode.encode = RCSP_TRANSLATOR_ENCODE_OPUS;
    g_translator_mode.channel = 1;
    g_translator_mode.sample_rate = 16000;
}

void rcsp_translator_disconnect(void)
{
    /* Do not send notifications after disconnect. */
    rcsp_translator_reset();
}

int rcsp_translator_is_busy(void)
{
    return g_translator_mode.mode != RCSP_TRANSLATOR_MODE_IDLE;
}

void rcsp_translator_abort(void)
{
    if (!rcsp_translator_is_busy()) {
        return;
    }
    rcsp_translator_reset();
    if (!rcsp_translator_is_busy()) {
        rcsp_translator_notify_mode();
    }
}

void rcsp_translator_record_result(u8 success)
{
    rcsp_translator_audio_record_result(success);
}

void rcsp_translator_record_cancel_request(void)
{
    rcsp_translator_audio_cancel_record_request();
}

int rcsp_translator_key_record(u8 start)
{
    if (g_translator_mode.mode != RCSP_TRANSLATOR_MODE_FACE_TO_FACE) {
        return -EINVAL;
    }
    return rcsp_translator_audio_request_record(start);
}

void rcsp_translator_cmd_response(u8 opcode, u8 status, u16 handle,
                                  const u8 *addr)
{
    rcsp_translator_audio_cmd_response(opcode, status, handle, addr);
}

int rcsp_translator_notify_mode(void)
{
    u8 data[2 + TRANSLATOR_MODE_SIZE] = {
        RCSP_TRANSLATOR_OP_NOTIFY_MODE, TRANSLATOR_MODE_SIZE,
    };

    if (!get_rcsp_connect_status()) {
        return JL_ERR_SEND_NOT_READY;
    }
    translator_pack_mode(data + 2);
    return JL_CMD_send(JL_OPCODE_TRANSLATOR, data, sizeof(data),
                       JL_NOT_NEED_RESPOND, g_translator_ble_handle,
                       g_translator_spp_addr_valid ? g_translator_spp_addr : NULL);
}

static u8 translator_set_mode(const u8 *param)
{
    u8 mode = param[0];
    u32 sample_rate;

    /* Exit does not depend on codec fields; APP may fill them with zero. */
    if (mode == RCSP_TRANSLATOR_MODE_IDLE) {
        rcsp_translator_reset();
        return rcsp_translator_is_busy() ? RCSP_TRANSLATOR_RESULT_BUSY :
               RCSP_TRANSLATOR_RESULT_SUCCESS;
    }

    sample_rate = translator_read_be32(param + 3);
    if (mode > RCSP_TRANSLATOR_MODE_SIMULTANEOUS ||
        param[1] > RCSP_TRANSLATOR_ENCODE_JLA_V2 ||
        (param[2] != 1 && param[2] != 2) ||
        (sample_rate != 8000 && sample_rate != 16000)) {
        return RCSP_TRANSLATOR_RESULT_PARAM_ERR;
    }

    if (rcsp_translator_is_busy()) {
        return RCSP_TRANSLATOR_RESULT_IN_MODE;
    }
    if (mode != RCSP_TRANSLATOR_MODE_RECORD &&
        mode != RCSP_TRANSLATOR_MODE_RECORD_TRANSLATION &&
        mode != RCSP_TRANSLATOR_MODE_FACE_TO_FACE &&
        mode != RCSP_TRANSLATOR_MODE_SIMULTANEOUS) {
        return RCSP_TRANSLATOR_RESULT_FAIL;
    }
    if (param[1] != RCSP_TRANSLATOR_ENCODE_OPUS || param[2] != 1 ||
        sample_rate != 16000) {
        return RCSP_TRANSLATOR_RESULT_PARAM_ERR;
    }
    if (bt_get_call_status() != BT_CALL_HANGUP) {
        return RCSP_TRANSLATOR_RESULT_CALL_BUSY;
    }
#if RCSP_AI_DIALOGUE_ENABLE
    if (rcsp_ai_dialogue_is_busy()) {
        return RCSP_TRANSLATOR_RESULT_BUSY;
    }
#endif
    if (rcsp_realtime_record_is_busy() || ai_mic_is_busy()) {
        return RCSP_TRANSLATOR_RESULT_BUSY;
    }
#if TCFG_MIX_RECORD_ENABLE
    if (get_mix_recorder_status()) {
        return RCSP_TRANSLATOR_RESULT_BUSY;
    }
#endif
#if RCSP_MIX_RECORD_ENABLE
    if (rcsp_mix_recorder_get_status()) {
        return RCSP_TRANSLATOR_RESULT_BUSY;
    }
#endif
    if (ai_rx_player_any_running()) {
        return RCSP_TRANSLATOR_RESULT_PLAY_BUSY;
    }
    if (rcsp_translator_audio_prepare(mode)) {
        return RCSP_TRANSLATOR_RESULT_FAIL;
    }
    g_translator_mode.mode = mode;
    g_translator_mode.encode = param[1];
    g_translator_mode.channel = param[2];
    g_translator_mode.sample_rate = sample_rate;
    return RCSP_TRANSLATOR_RESULT_SUCCESS;
}

static int translator_audio_header_valid(const u8 *param, u16 len)
{
    u16 audio_len;

    if (len < TRANSLATOR_AUDIO_HEAD_SIZE) {
        return 0;
    }
    if (param[0] > RCSP_TRANSLATOR_SOURCE_ESCO_MIX ||
        param[1] > RCSP_TRANSLATOR_ENCODE_JLA_V2) {
        return 0;
    }
    audio_len = ((u16)param[5] << 8) | param[6];
    return audio_len == len - TRANSLATOR_AUDIO_HEAD_SIZE;
}

void rcsp_translator_cmd_deal(u8 sn, const u8 *data, u16 len,
                              u8 need_response, u16 ble_con_handle,
                              u8 *spp_remote_addr)
{
    u8 response[2 + TRANSLATOR_MODE_SIZE] = {0};
    u8 status = JL_PRO_STATUS_PARAM_ERR;
    u16 response_len = 0;
    u16 param_len;
    const u8 *param;
    u8 op;

    if (!data || len < 2) {
        goto reply;
    }
    op = data[0];
    param_len = len - 2;
    if (!(op == RCSP_TRANSLATOR_OP_AUDIO && data[1] == 0xff) &&
        data[1] != param_len) {
        goto reply;
    }
    param = data + 2;

    /* Only audio is an APP -> device command without a response. Drop
     * control operations on that path rather than silently changing state.
     */
    if (!need_response) {
        if (op == RCSP_TRANSLATOR_OP_AUDIO &&
            translator_audio_header_valid(param, param_len)) {
            u16 audio_len = param_len - TRANSLATOR_AUDIO_HEAD_SIZE;
            u16 crc = ((u16)param[3] << 8) | param[4];
            u16 crc_actual = CRC16(param + TRANSLATOR_AUDIO_HEAD_SIZE, audio_len);
            if (!rcsp_translator_is_busy()) {
                return;
            }
            if (param[1] != RCSP_TRANSLATOR_ENCODE_OPUS ||
                crc != crc_actual) {
                rcsp_translator_abort();
                return;
            }
            int ret = rcsp_translator_audio_receive(param[0],
                                                    param + TRANSLATOR_AUDIO_HEAD_SIZE,
                                                    audio_len, param[2]);
            if (ret) {
                rcsp_translator_abort();
            }
            return;
        }
        goto reply;
    }

    response[0] = op;
    switch (op) {
    case RCSP_TRANSLATOR_OP_GET_MODE:
        if (param_len != 0) {
            break;
        }
        response[1] = TRANSLATOR_MODE_SIZE;
        translator_pack_mode(response + 2);
        response_len = sizeof(response);
        status = JL_PRO_STATUS_SUCCESS;
        break;
    case RCSP_TRANSLATOR_OP_SET_MODE:
        if (param_len != TRANSLATOR_MODE_SIZE) {
            break;
        }
        response[1] = 1;
        response[2] = translator_set_mode(param);
        if (response[2] == RCSP_TRANSLATOR_RESULT_SUCCESS && param[0]) {
            g_translator_ble_handle = ble_con_handle;
            memset(g_translator_spp_addr, 0, sizeof(g_translator_spp_addr));
            g_translator_spp_addr_valid = spp_remote_addr != NULL;
            if (spp_remote_addr) {
                memcpy(g_translator_spp_addr, spp_remote_addr,
                       sizeof(g_translator_spp_addr));
            }
            rcsp_translator_audio_set_link(ble_con_handle, spp_remote_addr);
        }
        response_len = 3;
        status = JL_PRO_STATUS_SUCCESS;
        break;
    case RCSP_TRANSLATOR_OP_GET_CACHE:
        if (param_len != 1 || param[0] > RCSP_TRANSLATOR_SOURCE_ESCO_MIX) {
            break;
        }
        response[1] = 5;
        response[2] = param[0];
        {
            u32 free_size = rcsp_translator_audio_cache(param[0]);
            response[3] = free_size >> 24;
            response[4] = free_size >> 16;
            response[5] = free_size >> 8;
            response[6] = free_size;
        }
        response_len = 7;
        status = JL_PRO_STATUS_SUCCESS;
        break;
    default:
        status = JL_PRO_STATUS_UNKOWN_CMD;
        break;
    }

reply:
    if (need_response) {
        JL_CMD_response_send(JL_OPCODE_TRANSLATOR, status, sn,
                             response_len ? response : NULL, response_len,
                             ble_con_handle, spp_remote_addr);
    } else {
        if (data && len && data[0] == RCSP_TRANSLATOR_OP_AUDIO &&
            rcsp_translator_is_busy()) {
            rcsp_translator_abort();
        }
    }
}

#endif
