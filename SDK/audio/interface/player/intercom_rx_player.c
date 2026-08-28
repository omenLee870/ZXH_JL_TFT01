

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".intercom_rx_player.data.bss")
#pragma data_seg(".intercom_rx_player.data")
#pragma const_seg(".intercom_rx_player.text.const")
#pragma code_seg(".intercom_rx_player.text")
#endif
#include "jlstream.h"
#include "sdk_config.h"
#include "audio_config_def.h"
#include "intercom_rx_player.h"
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
#include "rcsp_translator.h"
#endif
#include "classic/tws_api.h"

#if  TCFG_INTERCOM_RX_NODE_ENABLE

struct intercom_rx_player {
    struct jlstream *stream;
    u8 *bt_addr;
    u8 source;
    struct list_head entry;
    u8 type;
    int channel; //记录当前是tws左声道还是右声道
};

extern int CONFIG_BTCTLER_TWS_ENABLE;

static struct list_head g_intercom_rx_player_list = LIST_HEAD_INIT(g_intercom_rx_player_list);

#define list_for_each_intercom_rx_player(p) \
    list_for_each_entry(p, &g_intercom_rx_player_list, entry)

static void intercom_rx_player_callback(void *private_data, int event)
{
    struct intercom_rx_player *player = (struct intercom_rx_player *)private_data;

    printf("intercom_rx_player_callback, event: %d\n", event);
    switch (event) {
    case STREAM_EVENT_START:
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
        JL_rcsp_translator_update_play_volume((u8)player->source);
#endif
        break;
    }
}


int intercom_rx_player_open(u8 *bt_addr, u8 source, struct intercom_rx_player_param *param, enum stream_node_state(*rx_func)(void *file, struct stream_frame **pframe))
{
    int err;
    struct intercom_rx_player *player;
    u16 uuid = 0;
    if (!param) {
        printf("intercom_rx_player param is NULL \n");
        return -EFAULT;
    }

    u8 type = param->type;

    list_for_each_intercom_rx_player(player) {
        if (player->source == source) {
            printf("intercom_rx_player id %x is opened\n", (u32)source);
            return 0;
        }
    }

    uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"intercom");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_INTERCOM_RX);
    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    list_add_tail(&player->entry, &g_intercom_rx_player_list);
    player->bt_addr = bt_addr;
    player->source = source;
    player->type = type;

    jlstream_set_callback(player->stream, player, intercom_rx_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_INTERCOM);
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
    list_del(&player->entry);
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool intercom_rx_player_runing(u8 source)
{
    struct intercom_rx_player *player;
    list_for_each_intercom_rx_player(player) {
        if (player->source == source) {
            return 1;
        }
    }
    return 0;
}


void intercom_rx_player_close(u8 source)
{
    struct intercom_rx_player *player;
    u8 found = 0;
    const char *pipeline;

    list_for_each_intercom_rx_player(player) {
        if (player->source == source) {
            found = 1;
            break;
        }
    }
    if (!found) {
        return;
    }
    pipeline = "intercom";
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

FILE *intercom_get_file()
{
    opus_debug = fopen(FILE_NAME, "r");
    if (opus_debug == NULL) {
        ASSERT(0, "opus_debug %p ", opus_debug);
    }
    return opus_debug;
}

static enum stream_node_state net_get_frame(void *file, struct stream_frame **pframe)
{
    struct intercom_rx_file_handle *hdl = (struct intercom_rx_file_handle *)file;
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
void intercom_rx_debug()
{
    intercom_get_file();
#if 1
    struct intercom_rx_player_param param = {0};
    /* param.coding_type = AUDIO_CODING_AMR; */
    param.coding_type = AUDIO_CODING_STENC_OPUS;
    param.channel_mode =  AUDIO_CH_MIX;
    param.sample_rate = 16000;
    param.bit_rate = 16000;
    param.type = INTERCOM_SERVICE_VOICE;
    param.frame_dms = 200;  //20ms一帧
    extern FILE *intercom_get_file();
    intercom_rx_player_open(NULL, 0, &param, net_get_frame);

    extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);
    audio_app_volume_set(APP_AUDIO_STATE_MUSIC, 100, 1);
#else

    struct intercom_rx_player_param param = {0};
    param.coding_type = AUDIO_CODING_STREAM_MP3;
    param.sample_rate = 16000;
    param.channel_mode = AUDIO_CH_MIX;
    param.type = INTERCOM_SERVICE_VOICE;
    intercom_rx_player_open(NULL, 0, &param, net_get_frame);
    extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);
    audio_app_volume_set(1, 100, 1);
#endif

}


void intercom_rx_close()
{
    intercom_rx_player_close(0);
    fclose(opus_debug);
}
#endif

#endif /*TCFG_INTERCOM_RX_NODE_ENABLE*/
