#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".fm_player.data.bss")
#pragma data_seg(".fm_player.data")
#pragma const_seg(".fm_player.text.const")
#pragma code_seg(".fm_player.text")
#endif
#include "jlstream.h"
#include "fm_player.h"
#include "app_config.h"
#include "effects/audio_pitchspeed.h"
#include "scene_switch.h"
#include "audio_config_def.h"
#include "effects/audio_vbass.h"
#if AUDIO_EQ_LINK_VOLUME
#include "effects/eq_config.h"
#endif
#include "audio_effect_demo.h"
struct fm_player {
    struct jlstream *stream;
    s8 fm_pitch_mode;
};

static struct fm_player *g_fm_player = NULL;

static void fm_player_callback(void *private_data, int event)
{
    struct fm_player *player = g_fm_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
        music_vocal_remover_update_parm();
#endif
#if AUDIO_VBASS_LINK_VOLUME
        vbass_link_volume();
#endif
#if AUDIO_EQ_LINK_VOLUME
        eq_link_volume();
#endif
#if AUDIO_AUTODUCK_LINK_VOLUME
        autoduck_link_volume();
#endif
        break;
    }
}

int fm_player_open()
{
    int err = 0;
    struct fm_player *player = g_fm_player;
    if (player) {
        return -EFAULT;
    }

#if TCFG_LOCAL_TWS_ENABLE
    struct local_tws_stream_params *local_tws_fmt = {0};

    struct stream_enc_fmt fmt = {
        .channel = LOCAL_TWS_CODEC_CHANNEL,
        .bit_width = LOCAL_TWS_CODEC_BIT_WIDTH,
        .frame_dms = LOCAL_TWS_CODEC_FRAME_LEN,
        .sample_rate = LOCAL_TWS_CODEC_SAMPLERATE,
        .bit_rate = LOCAL_TWS_CODEC_BIT_RATE,
        .coding_type = LOCAL_TWS_CODEC_TYPE,
    };
#endif
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"fm");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->fm_pitch_mode = PITCH_0;

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_FM);

    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    //设置拼包点数,不直接设置中断点数是因为有些芯片fm会申请16 * 中断点数 *4的接收buffer,导致内存不够
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 256);

    jlstream_set_callback(player->stream, player->stream, fm_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_FM);
#if defined(TCFG_VIRTUAL_SURROUND_EFF_MODULE_NODE_ENABLE) && TCFG_VIRTUAL_SURROUND_EFF_MODULE_NODE_ENABLE
    //解码帧长短得情况下，使用三线程推数
    jlstream_add_thread(player->stream, "media0");
    jlstream_add_thread(player->stream, "media1");
#if defined(CONFIG_CPU_BR28)
    jlstream_add_thread(player->stream, "media2");
#endif
#endif

#if TCFG_LOCAL_TWS_ENABLE
    err = jlstream_ioctl(player->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
    if (err == 0) {
        err = jlstream_start(player->stream);
    }
#else
    err = jlstream_start(player->stream);
#endif
    if (err) {
        goto __exit1;
    }

    g_fm_player = player;

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool fm_player_runing()
{
    return g_fm_player != NULL;
}

void fm_player_close()
{
    struct fm_player *player = g_fm_player;

    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);

    free(player);
    g_fm_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"fm");
}


//变调接口
int fm_file_pitch_up()
{
    struct fm_player *player = g_fm_player;
    if (!player) {
        return -1;
    }
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    player->fm_pitch_mode++;
    if (player->fm_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        player->fm_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    printf("play pitch up+++%d\n", player->fm_pitch_mode);
    int ret = fm_file_set_pitch(player->fm_pitch_mode);
    ret = (ret == true) ? player->fm_pitch_mode : -1;
    return ret;
}

int fm_file_pitch_down()
{
    struct fm_player *player = g_fm_player;
    if (!player) {
        return -1;
    }
    player->fm_pitch_mode--;
    if (player->fm_pitch_mode < 0) {
        player->fm_pitch_mode = 0;
    }
    printf("play pitch down---%d\n", player->fm_pitch_mode);
    int ret = fm_file_set_pitch(player->fm_pitch_mode);
    ret = (ret == true) ? player->fm_pitch_mode : -1;
    return ret;
}

int fm_file_set_pitch(enum _pitch_level pitch_mode)
{
    struct fm_player *player = g_fm_player;
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    if (pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    pitch_speed_param_tool_set pitch_param = {
        .pitch = pitch_param_table[pitch_mode],
        .speed = 1,
    };
    if (player) {
        player->fm_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

void fm_file_pitch_mode_init(enum _pitch_level pitch_mode)
{
    struct fm_player *player = g_fm_player;
    if (player) {
        player->fm_pitch_mode = pitch_mode;
    }
}
