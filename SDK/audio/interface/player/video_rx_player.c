

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_rx_player.data.bss")
#pragma data_seg(".video_rx_player.data")
#pragma const_seg(".video_rx_player.text.const")
#pragma code_seg(".video_rx_player.text")
#endif
#include "jlstream.h"
#include "sdk_config.h"
#include "audio_config_def.h"
#include "video_rx_player.h"
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
#include "rcsp_translator.h"
#endif
#if  TCFG_VIDEO_RX_NODE_ENABLE

struct video_rx_player {
    struct jlstream *stream;
    u8 source;
    struct list_head entry;
    u8 type;
};

static struct list_head g_video_rx_player_list = LIST_HEAD_INIT(g_video_rx_player_list);

#define list_for_each_video_rx_player(p) \
    list_for_each_entry(p, &g_video_rx_player_list, entry)

static void video_rx_player_callback(void *private_data, int event)
{
    struct video_rx_player *player = (struct video_rx_player *)private_data;

    printf("video_rx_player_callback, event: %d\n", event);
    switch (event) {
    case STREAM_EVENT_START:
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
        JL_rcsp_translator_update_play_volume((u8)player->source);
#endif
        break;
    }
}


int video_rx_player_open(u8 source, struct video_rx_player_param *param,
                         enum stream_node_state(*rx_func)(void *file, struct stream_frame **pframe))
{
    int err;
    struct video_rx_player *player;
    u16 uuid = 0;
    if (!param) {
        printf("video_rx_player param is NULL \n");
        return -EFAULT;
    }

    u8 type = param->type;

    list_for_each_video_rx_player(player) {
        if (player->source == source) {
            printf("video_rx_player id %x is opened\n", (u32)source);
            return 0;
        }
    }

    uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"video_voice");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_VIDEO_RX);
    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    list_add_tail(&player->entry, &g_video_rx_player_list);
    player->source = source;
    player->type = type;

    jlstream_set_callback(player->stream, player, video_rx_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_VIDEO_VOICE);
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

bool video_rx_player_runing(u8 source)
{
    struct video_rx_player *player;
    list_for_each_video_rx_player(player) {
        if (player->source == source) {
            return 1;
        }
    }
    return 0;
}


void video_rx_player_close(u8 source)
{
    struct video_rx_player *player;
    u8 found = 0;
    list_for_each_video_rx_player(player) {
        if (player->source == source) {
            found = 1;
            break;
        }
    }
    if (!found) {
        return;
    }
    jlstream_stop(player->stream, 0);
    jlstream_release(player->stream);

    list_del(&player->entry);
    free(player);

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"video_voice");
}

__attribute__((weak))int pcm_read_debug(void *data, int len)
{
    return 0;
}
#if 0
#include "system/includes.h"
#define SD_ROOT_PATH "storage/sd0/C/"
#define FILE_NAME SD_ROOT_PATH"pcm.bin"
/* #define FILE_NAME SD_ROOT_PATH"bd.amr" */
/* #define FILE_NAME SD_ROOT_PATH"silk.ogg" */
FILE *video_pcm_debug = NULL;

FILE *video_get_file()
{
    video_pcm_debug = fopen(FILE_NAME, "r");
    if (video_pcm_debug == NULL) {
        ASSERT(0, "video_opus_debug %p ", video_pcm_debug);
    }
    return video_pcm_debug;
}
int pcm_read_debug(void *data, int len)
{

    int rlen = fread(data, len, 1, video_pcm_debug);
    return rlen;
}
void video_rx_debug()
{
    struct video_rx_player_param param = {0};
    /* param.coding_type = AUDIO_CODING_AMR; */
    param.coding_type = AUDIO_CODING_PCM;
    param.channel_mode =  AUDIO_CH_MIX;
    param.sample_rate = 16000;
    param.bit_rate = 16000;
    param.type = VIDEO_SERVICE_VOICE;
    param.frame_dms = 200;  //20ms一帧
    extern FILE *video_get_file();
    video_get_file();
    video_rx_player_open(0, &param);

    extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);
    audio_app_volume_set(APP_AUDIO_STATE_MUSIC, 100, 1);
}


void video_rx_close()
{
    video_rx_player_close(0);
    fclose(video_pcm_debug);
}
#endif

#endif /*TCFG_VIDEO_RX_NODE_ENABLE*/
