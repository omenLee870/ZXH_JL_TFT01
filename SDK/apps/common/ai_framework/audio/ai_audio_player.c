
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ai_audio_player.data.bss")
#pragma data_seg(".ai_audio_player.data")
#pragma const_seg(".ai_audio_player.text.const")
#pragma code_seg(".ai_audio_player.text")
#endif

#include "ai_audio_player.h"
#include "classic/tws_api.h"
#include "jlstream.h"
#include "sdk_config.h"
#include "system/init.h"

#if TCFG_AI_RX_NODE_ENABLE

struct ai_audio_player {
    struct jlstream *stream;
    u8 *bt_addr;
    u8 source;
    struct list_head entry;
    u8 type;
    int channel;
    void (*event_cb)(u8 source, int event, void *priv);
    void *event_priv;
};

extern int CONFIG_BTCTLER_TWS_ENABLE;

static struct list_head g_ai_audio_player_list = LIST_HEAD_INIT(g_ai_audio_player_list);
static OS_MUTEX g_ai_audio_player_mutex;

#define list_for_each_ai_audio_player(p) \
    list_for_each_entry(p, &g_ai_audio_player_list, entry)

static int ai_audio_player_init(void)
{
    return os_mutex_create(&g_ai_audio_player_mutex);
}
__initcall(ai_audio_player_init);

static void ai_audio_player_mutex_enter(void)
{
    os_mutex_pend(&g_ai_audio_player_mutex, 0);
}

static void ai_audio_player_mutex_exit(void)
{
    os_mutex_post(&g_ai_audio_player_mutex);
}

static void ai_audio_player_callback(void *private_data, int event)
{
    struct ai_audio_player *player = (struct ai_audio_player *)private_data;

    printf("ai_audio_player_callback, event: %d\n", event);
    if (player->event_cb) {
        player->event_cb(player->source, event, player->event_priv);
    }
}

int ai_audio_player_open(u8 *bt_addr,
                         u8 source,
                         struct ai_rx_player_param *param,
                         enum stream_node_state(*rx_func)(void *file, struct stream_frame **pframe),
                         void (*event_cb)(u8 source, int event, void *priv),
                         void *event_priv)
{
    int err;
    struct ai_audio_player *player;
    u16 uuid = 0;
    u8 type;

    if (!param) {
        printf("ai_audio_player param is NULL\n");
        return -EFAULT;
    }

    type = param->type;
    ai_audio_player_mutex_enter();
    list_for_each_ai_audio_player(player) {
        if (player->source == source) {
            ai_audio_player_mutex_exit();
            printf("ai_audio_player id %x is opened\n", (u32)source);
            return 0;
        }
    }
    ai_audio_player_mutex_exit();

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

    player->bt_addr = bt_addr;
    player->source = source;
    player->type = type;
    player->event_cb = event_cb;
    player->event_priv = event_priv;

    jlstream_set_callback(player->stream, player, ai_audio_player_callback);
    if (type == AI_SERVICE_CALL_DOWNSTREAM || type == AI_SERVICE_CALL_UPSTREAM) {
        jlstream_set_scene(player->stream, STREAM_SCENE_ESCO);
    } else if (type == AI_SERVICE_MEDIA) {
        jlstream_set_scene(player->stream, STREAM_SCENE_A2DP);
    } else {
        jlstream_set_scene(player->stream, STREAM_SCENE_AI_VOICE);
    }

    if (CONFIG_BTCTLER_TWS_ENABLE &&
        (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
        player->channel = tws_api_get_local_channel() == 'L' ? AUDIO_CH_L : AUDIO_CH_R;
        jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, player->channel);
    } else {
        jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, AUDIO_CH_MIX);
    }
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

    ai_audio_player_mutex_enter();
    list_add_tail(&player->entry, &g_ai_audio_player_list);
    ai_audio_player_mutex_exit();
    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

int ai_audio_player_get_delay(u8 source)
{
    struct ai_audio_player *player;
    int delay = 0;

    ai_audio_player_mutex_enter();
    list_for_each_ai_audio_player(player) {
        if (player->source == source) {
            if (player->stream) {
                delay = jlstream_node_ioctl(player->stream, NODE_UUID_DAC,
                                            NODE_IOC_GET_DELAY, 0);
            }
            break;
        }
    }
    ai_audio_player_mutex_exit();

    return delay;
}

bool ai_audio_player_running(u8 source)
{
    struct ai_audio_player *player;
    bool running = 0;

    ai_audio_player_mutex_enter();
    list_for_each_ai_audio_player(player) {
        if (player->source == source) {
            running = 1;
            break;
        }
    }
    ai_audio_player_mutex_exit();
    return running;
}

void ai_audio_player_close(u8 source)
{
    struct ai_audio_player *player;
    struct ai_audio_player *found = NULL;
    const char *pipeline;

    ai_audio_player_mutex_enter();
    list_for_each_ai_audio_player(player) {
        if (player->source == source) {
            found = player;
            list_del(&player->entry);
            break;
        }
    }
    ai_audio_player_mutex_exit();
    if (!found) {
        return;
    }
    player = found;

    if (player->type == AI_SERVICE_CALL_DOWNSTREAM || player->type == AI_SERVICE_CALL_UPSTREAM) {
        pipeline = "ai_rx_call";
    } else if (player->type == AI_SERVICE_MEDIA) {
        pipeline = "ai_rx_media";
    } else {
        pipeline = "ai_voice";
    }
    jlstream_stop(player->stream, 0);
    jlstream_release(player->stream);
    free(player);
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)pipeline);
}

#else

int ai_audio_player_open(u8 *bt_addr,
                         u8 source,
                         struct ai_rx_player_param *param,
                         enum stream_node_state(*rx_func)(void *file, struct stream_frame **pframe),
                         void (*event_cb)(u8 source, int event, void *priv),
                         void *event_priv)
{
    return -EFAULT;
}

void ai_audio_player_close(u8 source)
{
}

int ai_audio_player_get_delay(u8 source)
{
    return 0;
}

bool ai_audio_player_running(u8 source)
{
    return 0;
}

#endif
