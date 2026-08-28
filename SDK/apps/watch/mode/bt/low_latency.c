#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".low_latency.data.bss")
#pragma data_seg(".low_latency.data")
#pragma const_seg(".low_latency.text.const")
#pragma code_seg(".low_latency.text")
#endif
#include "system/includes.h"
#include "btstack/a2dp_media_codec.h"
#include "a2dp_player.h"
#include "app_tone.h"
#include "low_latency.h"
#include "app_config.h"

#if TCFG_APP_BT_EN
#if(TCFG_USER_TWS_ENABLE == 0)

enum {
    API_ENTER_LOW_LATENCY,
    API_EXIT_LOW_LATENCY,
    API_RESTART_A2DP_DEC,
};

static u8 low_latency_mode = 0;
static u16 low_latency_timer = 0;
static u8 g_btaddr[6];
static void *g_a2dp_file = NULL;

static void get_a2dp_packet_timer(void *p)
{
    struct a2dp_media_frame frame;

    if (!g_a2dp_file) {
        return;
    }

    int len = a2dp_media_try_get_packet(g_a2dp_file, &frame);
    if (len == 0) {
        return;
    }
    u16 seqn = (frame.packet[2] << 8) | frame.packet[3];
    int type = a2dp_media_get_codec_type(g_a2dp_file);
    if (type == A2DP_CODEC_SBC) {
        seqn += 200 / 15;
    } else {
        seqn += 200 / 20;
    }
    a2dp_media_clear_packet_before_seqn(g_a2dp_file, seqn);

    if (!tone_player_runing()) {
        sys_timer_del(low_latency_timer);
        low_latency_timer = 0;
        a2dp_player_open(g_btaddr);
    }
}

static void set_low_latency_mode(int enable)
{
    if (low_latency_timer) {
        return;
    }

    printf("set_low_latency: enable = %d\n", enable);

    a2dp_player_low_latency_enable(enable);
    if (a2dp_player_runing()) {
        a2dp_player_get_btaddr(g_btaddr);
        a2dp_player_close(g_btaddr);
        g_a2dp_file = a2dp_open_media_file(g_btaddr);
        low_latency_timer = sys_timer_add(NULL, get_a2dp_packet_timer, 100);
    }

    low_latency_mode = enable;
}

void bt_set_low_latency_mode(int enable)
{
    set_low_latency_mode(enable);
}

int bt_get_low_latency_mode()
{
    return low_latency_mode;
}

#endif
#endif /* #if TCFG_APP_BT_EN */
