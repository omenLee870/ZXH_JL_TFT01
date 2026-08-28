#include "system/includes.h"
#include "app_config.h"
#include "ai_audio_url_player.h"
#include "ai_internal.h"
#include "audio_def.h"
#include "ai_audio_player.h"
#include "network_download/net_download.h"

#include <stdlib.h>
#include <string.h>

#define AI_AUDIO_URL_DEFAULT_VOLUME          10
#define AI_AUDIO_URL_DEFAULT_CBUF_SIZE       (80 * 1024)
#define AI_AUDIO_URL_DEFAULT_START_THRESHOLD 512
#define AI_AUDIO_URL_DEFAULT_SEEK_THRESHOLD  (20 * 1024)
#define AI_AUDIO_URL_DEFAULT_TIMEOUT_MS      10000
#define AI_AUDIO_URL_QUEUE_DEPTH             8
#define AI_AUDIO_URL_READY_POLL_TICKS        2
#define AI_AUDIO_URL_SOURCE                  0xA1
#define AI_AUDIO_URL_FRAME_BYTES             1024
#define AI_AUDIO_URL_MP3_PROBE_BYTES         2048

extern u32 timer_get_ms(void);

struct ai_audio_url_player_ctx {
    OS_MUTEX mutex;
    struct ai_audio_url_player_config cfg;
    char *queue[AI_AUDIO_URL_QUEUE_DEPTH];
    int head;
    int tail;
    int count;
    volatile int initialized;
    volatile int task_running;
    volatile int stop;
    volatile int play_done;
    volatile int play_error;
    void *net_file;
    char *current_url;
    unsigned char prefix[AI_AUDIO_URL_MP3_PROBE_BYTES];
    unsigned int prefix_len;
    unsigned int prefix_offset;
};

static struct ai_audio_url_player_ctx g_url_player;

static unsigned int ai_audio_url_cbuf_size(void)
{
    return g_url_player.cfg.cbuf_size ? g_url_player.cfg.cbuf_size : AI_AUDIO_URL_DEFAULT_CBUF_SIZE;
}

static unsigned int ai_audio_url_start_threshold(void)
{
    return g_url_player.cfg.start_play_threshold ?
           g_url_player.cfg.start_play_threshold : AI_AUDIO_URL_DEFAULT_START_THRESHOLD;
}

static unsigned int ai_audio_url_seek_threshold(void)
{
    return g_url_player.cfg.seek_threshold ? g_url_player.cfg.seek_threshold : AI_AUDIO_URL_DEFAULT_SEEK_THRESHOLD;
}

static unsigned int ai_audio_url_timeout_ms(void)
{
    return g_url_player.cfg.timeout_ms ? g_url_player.cfg.timeout_ms : AI_AUDIO_URL_DEFAULT_TIMEOUT_MS;
}

static unsigned char ai_audio_url_volume(void)
{
    return g_url_player.cfg.volume ? g_url_player.cfg.volume : AI_AUDIO_URL_DEFAULT_VOLUME;
}

static void ai_audio_url_emit(const char *url, enum ai_audio_url_event event, int error)
{
    if (g_url_player.cfg.cb) {
        g_url_player.cfg.cb(url, event, error, g_url_player.cfg.user);
    }
}

static u32 ai_audio_url_coding_type(const char *media_type)
{
    if (!media_type) {
        return AUDIO_CODING_UNKNOW;
    }
    if (!strcmp(media_type, "mp3")) {
        return AUDIO_CODING_STREAM_MP3;
    }
    if (!strcmp(media_type, "aac")) {
        return AUDIO_CODING_AAC;
    }
    if (!strcmp(media_type, "m4a")) {
        return AUDIO_CODING_M4A;
    }
    if (!strcmp(media_type, "opus") || !strcmp(media_type, "ogg")) {
        return AUDIO_CODING_OPUS;
    }
    if (!strcmp(media_type, "pcm")) {
        return AUDIO_CODING_PCM;
    }
    return AUDIO_CODING_UNKNOW;
}

static unsigned int ai_audio_url_mp3_sample_rate(const unsigned char *data,
        unsigned int len)
{
    static const unsigned int rates[] = {44100, 48000, 32000};
    unsigned int i;

    for (i = 0; i + 4 <= len; i++) {
        u32 header = ((u32)data[i] << 24) | ((u32)data[i + 1] << 16) |
                     ((u32)data[i + 2] << 8) | data[i + 3];
        unsigned int version = (header >> 19) & 0x3;
        unsigned int layer = (header >> 17) & 0x3;
        unsigned int bitrate_index = (header >> 12) & 0xf;
        unsigned int rate_index = (header >> 10) & 0x3;
        unsigned int rate;

        if ((header & 0xffe00000) != 0xffe00000 || version == 1 ||
            layer == 0 || bitrate_index == 0 || bitrate_index == 0xf ||
            rate_index == 0x3) {
            continue;
        }
        rate = rates[rate_index];
        if (version == 2) {
            rate /= 2;
        } else if (version == 0) {
            rate /= 4;
        }
        return rate;
    }

    return 0;
}

static unsigned int ai_audio_url_id3_data_offset(const unsigned char *data,
        unsigned int len)
{
    unsigned int size;

    if (len < 10 || memcmp(data, "ID3", 3) ||
        (data[6] | data[7] | data[8] | data[9]) & 0x80) {
        return 0;
    }
    size = ((unsigned int)data[6] << 21) | ((unsigned int)data[7] << 14) |
           ((unsigned int)data[8] << 7) | data[9];
    return 10 + size + ((data[5] & 0x10) ? 10 : 0);
}

static int ai_audio_url_probe_mp3(void *net_file, unsigned int *sample_rate)
{
    unsigned int data_offset;
    int rlen;

    g_url_player.prefix_len = 0;
    g_url_player.prefix_offset = 0;
    rlen = net_download_read(net_file, g_url_player.prefix, 10);
    if (rlen < 4) {
        return AI_ERR_PARSE;
    }
    g_url_player.prefix_len = (unsigned int)rlen;

    data_offset = ai_audio_url_id3_data_offset(g_url_player.prefix,
                  g_url_player.prefix_len);
    if (data_offset) {
        if (net_download_seek(net_file, data_offset, SEEK_SET)) {
            return AI_ERR_NET;
        }
        g_url_player.prefix_len = 0;
    }

    rlen = net_download_read(net_file,
                             g_url_player.prefix + g_url_player.prefix_len,
                             sizeof(g_url_player.prefix) - g_url_player.prefix_len);
    if (rlen > 0) {
        g_url_player.prefix_len += (unsigned int)rlen;
    }
    *sample_rate = ai_audio_url_mp3_sample_rate(g_url_player.prefix,
                   g_url_player.prefix_len);
    if (!*sample_rate) {
        printf("[ai_url_player] mp3 sample rate parse failed\n");
        return AI_ERR_PARSE;
    }
    printf("[ai_url_player] mp3 sample rate: %u\n", *sample_rate);
    return AI_OK;
}

static void ai_audio_url_player_event(u8 source, int event, void *priv)
{
    (void)priv;

    if (source == AI_AUDIO_URL_SOURCE && event == STREAM_EVENT_NEGOTIATE_FAILD) {
        g_url_player.play_error = 1;
        g_url_player.play_done = 1;
    }
}

static enum stream_node_state ai_audio_url_rx_frame(void *file, struct stream_frame **pframe)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;
    struct stream_frame *frame;
    int rlen;
    int download_status = 0;
    int http_err_status = 0;

    *pframe = NULL;
    if (!g_url_player.net_file) {
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    frame = jlstream_get_frame(hdl->node->oport, AI_AUDIO_URL_FRAME_BYTES);
    if (!frame) {
        return NODE_STA_RUN;
    }

    if (g_url_player.prefix_offset < g_url_player.prefix_len) {
        unsigned int remain = g_url_player.prefix_len - g_url_player.prefix_offset;

        rlen = remain > AI_AUDIO_URL_FRAME_BYTES ? AI_AUDIO_URL_FRAME_BYTES : remain;
        memcpy(frame->data, g_url_player.prefix + g_url_player.prefix_offset, rlen);
        g_url_player.prefix_offset += (unsigned int)rlen;
    } else {
        rlen = net_download_read(g_url_player.net_file, frame->data, AI_AUDIO_URL_FRAME_BYTES);
    }
    if (rlen > 0) {
        frame->len = rlen;
        *pframe = frame;
        return NODE_STA_RUN;
    }

    jlstream_free_frame(frame);
    net_download_get_status(g_url_player.net_file, &download_status, &http_err_status);
    if (download_status == NET_DOWNLOAD_COMPLETE && net_download_get_tmp_data_size(g_url_player.net_file) == 0) {
        g_url_player.play_done = 1;
    } else if (download_status < 0) {
        g_url_player.play_error = 1;
        g_url_player.play_done = 1;
    }
    return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
}

static void ai_audio_url_close_current(int stop_dec)
{
    (void)stop_dec;

    ai_audio_player_close(AI_AUDIO_URL_SOURCE);
    if (g_url_player.net_file) {
        net_download_buf_inactive(g_url_player.net_file);
        net_download_close(g_url_player.net_file);
        g_url_player.net_file = NULL;
    }
    if (g_url_player.current_url) {
        free(g_url_player.current_url);
        g_url_player.current_url = NULL;
    }
    g_url_player.play_done = 0;
    g_url_player.play_error = 0;
    g_url_player.prefix_len = 0;
    g_url_player.prefix_offset = 0;
}

static void ai_audio_url_clear_queue(void)
{
    int i;

    os_mutex_pend(&g_url_player.mutex, 0);
    for (i = 0; i < AI_AUDIO_URL_QUEUE_DEPTH; i++) {
        if (g_url_player.queue[i]) {
            free(g_url_player.queue[i]);
            g_url_player.queue[i] = NULL;
        }
    }
    g_url_player.head = 0;
    g_url_player.tail = 0;
    g_url_player.count = 0;
    os_mutex_post(&g_url_player.mutex);
}

static void ai_audio_url_clear_queue_locked(void)
{
    int i;

    for (i = 0; i < AI_AUDIO_URL_QUEUE_DEPTH; i++) {
        if (g_url_player.queue[i]) {
            free(g_url_player.queue[i]);
            g_url_player.queue[i] = NULL;
        }
    }
    g_url_player.head = 0;
    g_url_player.tail = 0;
    g_url_player.count = 0;
}

static char *ai_audio_url_pop(void)
{
    char *url = NULL;

    os_mutex_pend(&g_url_player.mutex, 0);
    if (g_url_player.count > 0) {
        url = g_url_player.queue[g_url_player.head];
        g_url_player.queue[g_url_player.head] = NULL;
        g_url_player.head = (g_url_player.head + 1) % AI_AUDIO_URL_QUEUE_DEPTH;
        g_url_player.count--;
    }
    os_mutex_post(&g_url_player.mutex);

    return url;
}

static int ai_audio_url_wait_ready(void *net_file)
{
    unsigned int start_ms = timer_get_ms();
    unsigned int timeout_ms = ai_audio_url_timeout_ms();
    int ready;

    while (!g_url_player.stop) {
        ready = net_download_check_ready(net_file);
        if (ready > 0) {
            return AI_OK;
        }
        if (ready < 0) {
            return AI_ERR_NET;
        }
        if (timeout_ms && (timer_get_ms() - start_ms) >= timeout_ms) {
            return AI_ERR_NET;
        }
        os_time_dly(AI_AUDIO_URL_READY_POLL_TICKS);
    }

    return AI_ERR_STATE;
}

static int ai_audio_url_start_decode(void *net_file)
{
    struct ai_rx_player_param param = {0};
    char *media_type;
    int ret;

    media_type = net_download_get_media_type(net_file);
    if (!media_type) {
        return AI_ERR_PARSE;
    }

    printf("[ai_url_player] media type: %s\n", media_type);
    param.coding_type = ai_audio_url_coding_type(media_type);
    if (param.coding_type == AUDIO_CODING_UNKNOW) {
        return AI_ERR_PARSE;
    }
    param.sample_rate = 16000;
    param.channel_mode = AUDIO_CH_MIX;
    param.type = AI_SERVICE_VOICE;
    net_download_set_read_timeout(net_file, 5000);
    if (param.coding_type == AUDIO_CODING_STREAM_MP3) {
        ret = ai_audio_url_probe_mp3(net_file, &param.sample_rate);
        if (ret != AI_OK) {
            net_download_set_read_timeout(net_file, 0);
            return ret;
        }
    }
    ret = ai_audio_player_open(NULL, AI_AUDIO_URL_SOURCE, &param, ai_audio_url_rx_frame, ai_audio_url_player_event, NULL);
    net_download_set_read_timeout(net_file, 0);
    if (ret) {
        return AI_ERR_NET;
    }

    extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);
    audio_app_volume_set(1, ai_audio_url_volume(), 1);
    net_download_set_pp(net_file, 0);
    return AI_OK;
}

static int ai_audio_url_play_one(char *url)
{
    struct net_download_parm parm;
    int ret;

    memset(&parm, 0, sizeof(parm));
    parm.url = url;
    parm.cbuf_size = ai_audio_url_cbuf_size();
    parm.timeout_millsec = ai_audio_url_timeout_ms();
    parm.start_play_threshold = ai_audio_url_start_threshold();
    parm.seek_threshold = ai_audio_url_seek_threshold();

    g_url_player.current_url = url;
    g_url_player.play_done = 0;
    g_url_player.play_error = 0;

    printf("[ai_url_player] open: %s\n", url);
    ret = net_download_open(&g_url_player.net_file, &parm);
    if (ret) {
        return AI_ERR_NET;
    }

    ret = ai_audio_url_wait_ready(g_url_player.net_file);
    if (ret == AI_OK) {
        ai_audio_url_emit(url, AI_AUDIO_URL_EVENT_START, AI_OK);
        ret = ai_audio_url_start_decode(g_url_player.net_file);
    }
    if (ret != AI_OK) {
        return ret;
    }

    while (!g_url_player.stop && !g_url_player.play_done) {
        os_time_dly(1);
    }

    if (g_url_player.stop) {
        return AI_ERR_STATE;
    }

    return g_url_player.play_error ? AI_ERR_NET : AI_OK;
}

static void ai_audio_url_task(void *priv)
{
    char *url;
    int ret;

    (void)priv;
    printf("[ai_url_player] task start\n");

    for (;;) {
        while (!g_url_player.stop) {
            url = ai_audio_url_pop();
            if (!url) {
                break;
            }

            ret = ai_audio_url_play_one(url);
            if (ret == AI_OK) {
                ai_audio_url_emit(url, AI_AUDIO_URL_EVENT_END, AI_OK);
            } else if (ret != AI_ERR_STATE) {
                ai_audio_url_emit(url, AI_AUDIO_URL_EVENT_ERROR, AI_ERR_NET);
            }
            ai_audio_url_close_current(1);
            os_time_dly(1);
        }

        if (g_url_player.stop) {
            ai_audio_url_clear_queue();
        }

        ai_audio_url_close_current(1);

        os_mutex_pend(&g_url_player.mutex, 0);
        if (!g_url_player.stop && g_url_player.count > 0) {
            os_mutex_post(&g_url_player.mutex);
            continue;
        }
        g_url_player.task_running = 0;
        g_url_player.stop = 0;
        os_mutex_post(&g_url_player.mutex);
        break;
    }
    printf("[ai_url_player] task exit\n");
}

int ai_audio_url_player_init(const struct ai_audio_url_player_config *cfg)
{
    if (!g_url_player.initialized) {
        memset(&g_url_player, 0, sizeof(g_url_player));
        os_mutex_create(&g_url_player.mutex);
        g_url_player.initialized = 1;
    }
    if (!cfg) {
        return AI_OK;
    }

    os_mutex_pend(&g_url_player.mutex, 0);
    if (g_url_player.task_running) {
        os_mutex_post(&g_url_player.mutex);
        return AI_ERR_STATE;
    }
    g_url_player.cfg = *cfg;
    os_mutex_post(&g_url_player.mutex);

    return AI_OK;
}

int ai_audio_url_player_enqueue(const char *url)
{
    char *copy;
    int ret;
    int need_start = 0;

    if (!url || !url[0]) {
        return AI_ERR_PARAM;
    }
    if (!g_url_player.initialized) {
        ai_audio_url_player_init(NULL);
    }

    copy = ai_strdup(url);
    if (!copy) {
        return AI_ERR_NOMEM;
    }

    os_mutex_pend(&g_url_player.mutex, 0);
    if (g_url_player.stop) {
        os_mutex_post(&g_url_player.mutex);
        free(copy);
        return AI_ERR_STATE;
    }
    if (g_url_player.count >= AI_AUDIO_URL_QUEUE_DEPTH) {
        os_mutex_post(&g_url_player.mutex);
        free(copy);
        return AI_ERR_STATE;
    }
    g_url_player.queue[g_url_player.tail] = copy;
    g_url_player.tail = (g_url_player.tail + 1) % AI_AUDIO_URL_QUEUE_DEPTH;
    g_url_player.count++;
    printf("[ai_url_player] enqueue count=%d url=%s\n", g_url_player.count, url);
    if (!g_url_player.task_running) {
        g_url_player.task_running = 1;
        need_start = 1;
    }
    os_mutex_post(&g_url_player.mutex);

    if (need_start) {
        ret = thread_fork("ai_url_play", 20, 10 * 1024, 0, 0, ai_audio_url_task, NULL);
        if (ret != OS_NO_ERR) {
            printf("[ai_url_player] thread_fork failed: %d\n", ret);
            os_mutex_pend(&g_url_player.mutex, 0);
            ai_audio_url_clear_queue_locked();
            g_url_player.task_running = 0;
            os_mutex_post(&g_url_player.mutex);
            return AI_ERR_NET;
        }
    }

    return AI_OK;
}

int ai_audio_url_player_stop(void)
{
    unsigned int start_ms;

    if (!g_url_player.initialized) {
        return AI_OK;
    }

    g_url_player.stop = 1;
    ai_audio_url_clear_queue();
    if (!g_url_player.task_running) {
        g_url_player.stop = 0;
        return AI_OK;
    }

    start_ms = timer_get_ms();
    while (g_url_player.task_running) {
        if ((timer_get_ms() - start_ms) > 3000) {
            return AI_ERR_STATE;
        }
        os_time_dly(1);
    }

    return AI_OK;
}

int ai_audio_url_player_is_running(void)
{
    return g_url_player.task_running ? 1 : 0;
}
