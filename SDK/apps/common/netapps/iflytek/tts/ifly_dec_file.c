#include "circular_buf.h"
#include "app_config.h"
/* #include "audio_dec.h" */
/* #include "media/pcm_decoder.h" */
#include "tts_main.h"
#include "jlstream.h"
#include "ai_rx_player.h"

#if TCFG_IFLYTEK_ENABLE

#define LOG_TAG_CONST       NET_IFLY
#define LOG_TAG             "[IFLY_DEC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

// 从tts cbuf中取一帧mp3数据喂给ai_rx播放节点
static enum stream_node_state iflytek_tts_ai_rx_get_frame(void *file, struct stream_frame **pframe)
{
#define FIX_LEN 1024
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;
    extern int iflytek_tts_cbuf_data_len();
    int available = iflytek_tts_cbuf_data_len();
    int read_size = MIN(available, FIX_LEN);
    if (read_size <= 0) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }
    struct stream_frame *frame = jlstream_get_frame(hdl->node->oport, read_size);
    if (!frame) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }
    extern int iflytek_tts_cbuf_read_data(u8 * data, int size);
    int rlen = iflytek_tts_cbuf_read_data(frame->data, read_size);
    frame->len = MAX(rlen, 0);
    if (rlen != read_size) {
        printf("cbuf_read mismatch: req=%d, act=%d\n", read_size, rlen);
    }
    *pframe = frame;
    return NODE_STA_RUN;
}

static void iflytek_tts_audio_begin(void)
{
    struct ai_rx_player_param param = {0};
    param.coding_type = AUDIO_CODING_STREAM_MP3;
    param.sample_rate = 16000;
    param.channel_mode = AUDIO_CH_MIX;
    param.type = AI_SERVICE_VOICE;
    ai_rx_player_open(NULL, 0, &param, iflytek_tts_ai_rx_get_frame);
    extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);
    audio_app_volume_set(1, 100, 1);
}

static void iflytek_tts_audio_start(void *arg)
{
    iflytek_tts_audio_begin();
}

void iflytek_tts_audio_play()
{
    iflytek_tts_audio_start(NULL);
}

void iflytek_tts_audio_stop(void *arg)
{
    ai_rx_player_close(0);
}

bool ifly_net_tts_dec_check_run(void)
{
    extern bool ai_rx_player_runing(u8 source);
    if (ai_rx_player_runing(0)) {
        return true;
    }
    return false;
}

#endif
