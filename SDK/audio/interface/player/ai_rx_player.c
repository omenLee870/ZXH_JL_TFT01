
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ai_rx_player.data.bss")
#pragma data_seg(".ai_rx_player.data")
#pragma const_seg(".ai_rx_player.text.const")
#pragma code_seg(".ai_rx_player.text")
#endif
#include "jlstream.h"
#include "sdk_config.h"
#include "audio_config_def.h"
#include "ai_rx_player.h"
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
#include "rcsp_translator.h"
#endif
#include "classic/tws_api.h"

#if  TCFG_AI_RX_NODE_ENABLE

struct ai_rx_player {
    struct jlstream *stream;
    u8 *bt_addr;
    u8 source;
    struct list_head entry;
    u8 type;
    int channel; //记录当前是tws左声道还是右声道
};

extern int CONFIG_BTCTLER_TWS_ENABLE;

static struct list_head g_ai_rx_player_list = LIST_HEAD_INIT(g_ai_rx_player_list);

#define list_for_each_ai_rx_player(p) \
    list_for_each_entry(p, &g_ai_rx_player_list, entry)

static void ai_rx_player_callback(void *private_data, int event)
{
    struct ai_rx_player *player = (struct ai_rx_player *)private_data;

    printf("ai_rx_player_callback, event: %d\n", event);
    switch (event) {
    case STREAM_EVENT_START:
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
        JL_rcsp_translator_update_play_volume((u8)player->source);
#endif
        break;
    }
}


int ai_rx_player_open(u8 *bt_addr, u8 source, struct ai_rx_player_param *param, enum stream_node_state(*rx_func)(void *file, struct stream_frame **pframe))
{
    int err;
    struct ai_rx_player *player;
    u16 uuid = 0;
    if (!param) {
        printf("ai_rx_player param is NULL \n");
        return -EFAULT;
    }

    u8 type = param->type;

    list_for_each_ai_rx_player(player) {
        if (player->source == source) {
            printf("ai_rx_player id %x is opened\n", (u32)source);
            return 0;
        }
    }

    if (type == AI_SERVICE_CALL_DOWNSTREAM || type == AI_SERVICE_CALL_UPSTREAM) {
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"ai_rx_call");
    } else if (type == AI_SERVICE_MEDIA) {
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"ai_rx_media");
    } else {
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"ai_voice");
    }
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    if (type == AI_SERVICE_CALL_DOWNSTREAM) {
        player->stream = jlstream_pipeline_parse_by_node_name(uuid, "AI_RX_ESCO_DOWN");
    } else if (type == AI_SERVICE_CALL_UPSTREAM) {
        player->stream = jlstream_pipeline_parse_by_node_name(uuid, "AI_RX_ESCO_UP");
    } else {
        player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_AI_RX);
    }
    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    list_add_tail(&player->entry, &g_ai_rx_player_list);
    player->bt_addr = bt_addr;
    player->source = source;
    player->type = type;

    jlstream_set_callback(player->stream, player, ai_rx_player_callback);
    if (type == AI_SERVICE_CALL_DOWNSTREAM || type == AI_SERVICE_CALL_UPSTREAM) {
        jlstream_set_scene(player->stream, STREAM_SCENE_ESCO);
    } else if (type == AI_SERVICE_MEDIA) {
        jlstream_set_scene(player->stream, STREAM_SCENE_A2DP);
    } else {
        jlstream_set_scene(player->stream, STREAM_SCENE_AI_VOICE);

    }
    if (CONFIG_BTCTLER_TWS_ENABLE) {
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            player->channel = tws_api_get_local_channel() == 'L' ? AUDIO_CH_L : AUDIO_CH_R;
            jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, player->channel);
        } else {
            jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, AUDIO_CH_MIX);
        }
    }
    jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, AUDIO_CH_MIX);
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_BTADDR, (int)bt_addr);
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PARAM, (int)source);
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_FMT, (int)param);
    if (rx_func) {
        jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, (int)rx_func);
    }
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool ai_rx_player_runing(u8 source)
{
    struct ai_rx_player *player;
    list_for_each_ai_rx_player(player) {
        if (player->source == source) {
            return 1;
        }
    }
    return 0;
}


void ai_rx_player_close(u8 source)
{
    struct ai_rx_player *player;
    u8 found = 0;
    const char *pipeline;

    list_for_each_ai_rx_player(player) {
        if (player->source == source) {
            found = 1;
            break;
        }
    }
    if (!found) {
        return;
    }
    if (player->type == AI_SERVICE_CALL_DOWNSTREAM || player->type == AI_SERVICE_CALL_UPSTREAM) {
        pipeline = "ai_rx_call";
    } else if (player->type == AI_SERVICE_MEDIA) {
        pipeline = "ai_rx_media";
    } else {
        pipeline = "ai_voice";
    }
    jlstream_stop(player->stream, 0);
    jlstream_release(player->stream);

    list_del(&player->entry);
    free(player);

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)pipeline);
}
/************************测试代码******************************/
#if 0
#include "system/includes.h"
#define SD_ROOT_PATH "storage/sd0/C/"
/* #define FILE_NAME SD_ROOT_PATH"zt.bin" */
/* #define FILE_NAME SD_ROOT_PATH"bd.amr" */
#define FILE_NAME SD_ROOT_PATH"opus.ogg"

FILE *opus_debug = NULL;

FILE *ai_get_file()
{
    opus_debug = fopen(FILE_NAME, "r");
    if (opus_debug == NULL) {
        ASSERT(0, "opus_debug %p ", opus_debug);
    }
    return opus_debug;
}

static enum stream_node_state net_get_frame(void *file, struct stream_frame **pframe)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;
    struct stream_frame *frame;
    u16 fram_len = 0;
    /* 外部注册了 rx_func 则直接替代本函数的取帧流程 */


    fram_len = 80;
    frame = jlstream_get_frame(hdl->node->oport, fram_len);
    if (frame == NULL) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }
    int rlen = fread(frame->data, fram_len, 1, opus_debug);
    frame->len = rlen;
    *pframe = frame;
    return NODE_STA_RUN;

}
void ai_rx_debug()
{
    ai_get_file();
#if 1
    struct ai_rx_player_param param = {0};
    /* param.coding_type = AUDIO_CODING_AMR; */
    param.coding_type = AUDIO_CODING_STENC_OPUS;
    param.channel_mode =  AUDIO_CH_MIX;
    param.sample_rate = 16000;
    param.bit_rate = 16000;
    param.type = AI_SERVICE_VOICE;
    param.frame_dms = 200;  //20ms一帧
    extern FILE *ai_get_file();
    ai_rx_player_open(NULL, 0, &param, net_get_frame);

    extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);
    audio_app_volume_set(APP_AUDIO_STATE_MUSIC, 100, 1);
#else

    struct ai_rx_player_param param = {0};
    param.coding_type = AUDIO_CODING_STREAM_MP3;
    param.sample_rate = 16000;
    param.channel_mode = AUDIO_CH_MIX;
    param.type = AI_SERVICE_VOICE;
    ai_rx_player_open(NULL, 0, &param, net_get_frame);
    extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);
    audio_app_volume_set(1, 100, 1);
#endif

}


void ai_rx_close()
{
    ai_rx_player_close(0);
    fclose(opus_debug);
}
#endif

#endif /*TCFG_AI_RX_NODE_ENABLE*/
