#include "system/includes.h"
#include "media/includes.h"

#include "app_config.h"
#include "app_action.h"
#include "app_main.h"
#include "audio_config.h"

#include "jlstream.h"

#include "intercom_voice_recoder.h"
#include "intercom_rx_player.h"
#include "circular_buf.h"
#include "vstream_common.h"
#include "kid_wt_app.h"

//#define LOG_TAG_CONST       KID_APP
#define LOG_TAG             "[kid_aud]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_VSTREAM_TRANSFER

/* ====== OPUS 帧参数 (与编码侧 intercom_voice_recoder_open(OPUS,0) 对齐) ======
 * 编码内部固定: bit_rate=16000, frame_dms=200(20ms), format=0(百度无头)
 * => 16kbps * 20ms / 8 = 40 字节/帧 */
#define KID_AUDIO_CODING_TYPE        AUDIO_CODING_OPUS
#define KID_AUDIO_FRAME_BYTE         40
#define KID_AUDIO_FRAME_MS           20

#define KID_AUDIO_BUF_LEN            ((10 * 1024 / KID_AUDIO_FRAME_BYTE) * KID_AUDIO_FRAME_BYTE)
#define KID_AUDIO_SEND_FRAME         600                     /* 300ms */
#define KID_AUDIO_SEND_BUF_LEN       (KID_AUDIO_SEND_FRAME * 2) /* 须为 80 的整数倍 */
#define KID_AUDIO_SEND_CBUF_EN       1


/* NO_DATA 期间周期唤醒解码流, 与单帧 20ms 对齐 (参照 opus_loop_demo) */
#define KID_AUDIO_WAKE_TIMER_MS      20

extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);


struct kid_audio_test {
    cbuffer_t cbuf;                       /* 解码输入缓冲 (网络收数 / 文件读取) */
    u8 data[KID_AUDIO_BUF_LEN];
#if KID_AUDIO_SEND_CBUF_EN
    cbuffer_t send_cbuf;                  /* 编码输出缓冲, 供 audio_app_data_get 取用 */
    u8 send_data[KID_AUDIO_SEND_BUF_LEN];
#endif
    FILE *audio_file;                     /* 文件播放句柄, 非 NULL 表示文件解码模式 */
    u8 enc_opened;
    u8 dec_opened;
    struct stream_node *dec_node;         /* 解码侧 intercom_rx 节点, 供 NO_DATA 唤醒 */
    u32 wake_timer;
};

/* kid_aud: 对讲 (enc + dec); kid_audio_play: 文件播放 (dec).
 * intercom_rx_player 为单例, 同一时刻只有一个解码流, 由 g_dec_ctx 指向当前取帧上下文. */
static struct kid_audio_test *kid_aud = NULL;
static struct kid_audio_test *kid_audio_play = NULL;
static struct kid_audio_test *g_dec_ctx = NULL;


/* NO_DATA 期间周期唤醒解码流, 保证文件/网络数据到达后能恢复取帧 */
static void kid_audio_wake_timer(void *_p)
{
    struct kid_audio_test *t = g_dec_ctx;
    if (t && t->dec_node && (t->dec_node->state & NODE_STA_SOURCE_NO_DATA)) {
        jlstream_wakeup_thread(NULL, t->dec_node, NULL);
    }
}

/* 编码输出回调: intercom_tx 每编出一帧 OPUS 调用一次.
 * 写入 send_cbuf 供 audio_app_data_get 取用; SPP 已连接时再批量 vstream_send 上行. */
static int kid_audio_tx_output(u8 *buf, u32 len)
{
    if (!buf || !len) {
        return len;
    }
    // putchar('C');
#if KID_AUDIO_SEND_CBUF_EN
    if (kid_aud) {

        // putchar('E');
        if (kwt_get_spp_connect()) {
            cbuf_write(&kid_aud->send_cbuf, buf, len);
            int cbuf_len = cbuf_get_data_len(&kid_aud->send_cbuf);
            // printf("cbuf_len=%d",cbuf_len);
            if (cbuf_len >= KID_AUDIO_SEND_FRAME) {
                u8 *tmp_data = zalloc(KID_AUDIO_SEND_FRAME);
                if (tmp_data) {
                    cbuf_read(&kid_aud->send_cbuf, tmp_data, KID_AUDIO_SEND_FRAME);
                    vstream_send_frame(tmp_data, KID_AUDIO_SEND_FRAME, VSTREAM_PCM_TYPE_AUDIO);
                    free(tmp_data);
                } else {
                    log_error("tmp_data zalloc fail\n");
                }
            }
        }
    }
#else
    if (kwt_get_spp_connect()) {
        vstream_send_frame(buf, len, VSTREAM_PCM_TYPE_AUDIO);
    }
#endif
    return len;
}

/* 解码取帧回调: intercom_rx 每需要一帧 OPUS 调用一次.
 * 先按当前模式补充 cbuf (文件 fread / 网络 vstream 收数), 再从 cbuf 取一整帧交解码器.
 * cbuf 不足一帧 -> 返回 NO_DATA + NULL, 解码流挂起, 由 wake_timer / tx_output 唤醒重试.
 * (参照 opus_loop_demo opus_loop_rx_input) */
static enum stream_node_state kid_audio_rx_input(void *file, struct stream_frame **pframe)
{
    struct intercom_rx_file_handle *hdl = (struct intercom_rx_file_handle *)file;
    struct kid_audio_test *t = g_dec_ctx;
    struct stream_frame *frame;

    if (!t) {
        *pframe = NULL;
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }
    if (!t->dec_node) {
        t->dec_node = hdl->node;
    }

    /* 补充 cbuf */
    if (t->audio_file) {
        /* 文件播放: 每次读一帧写入 cbuf; 读不到(EOF)则不再补充, cbuf 耗尽即 NO_DATA.
         * BR33 fread(buf, size, count, file) 返回读到的 count 数, 故用 size=帧长 count=1 */
        u8 rbuf[KID_AUDIO_FRAME_BYTE];
        int rcnt = fread(rbuf, KID_AUDIO_FRAME_BYTE, 1, t->audio_file);
        if (rcnt > 0) {
            cbuf_write(&t->cbuf, rbuf, KID_AUDIO_FRAME_BYTE);
        }
    } else if (kwt_get_spp_connect()) {
        /* 对讲: 等待懒初始化完成后再取数据 */
        if (!vstream_recv_audio_ready()) {
            *pframe = NULL;
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        }
        /* 从网络收数写入 cbuf */
        u8 *rbuf = NULL;
        int rlen = 0;
        vstream_recv_acquire_frame(0, &rbuf, &rlen, AUDIO_FRAME);
        if (rbuf && rlen > 0) {
            cbuf_write(&t->cbuf, rbuf, rlen);
            video_stream_recv_release_frame(rbuf);
        }
    }

    if (cbuf_get_data_size(&t->cbuf) < KID_AUDIO_FRAME_BYTE) {
        *pframe = NULL;
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    frame = jlstream_get_frame(hdl->node->oport, KID_AUDIO_FRAME_BYTE);
    if (!frame) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }
    cbuf_read(&t->cbuf, frame->data, KID_AUDIO_FRAME_BYTE);
    frame->len = KID_AUDIO_FRAME_BYTE;
    *pframe = frame;
    return NODE_STA_RUN;
}

/* ====== 内部: 编/解码开关 ====== */
static int kid_audio_enc_open_internal(void)
{
    int err;
    if (kid_aud && kid_aud->enc_opened) {
        return 0;
    }
    err = intercom_voice_recoder_open(KID_AUDIO_CODING_TYPE, 0);
    if (err) {
        log_error("recorder open fail %d\n", err);
        return err;
    }
    if (kid_aud) {
        kid_aud->enc_opened = 1;
    }
    /* 编码输出回调须在 recoder open 之后注册 */
    intercom_voice_recoder_set_intercom_tx_node_func(kid_audio_tx_output);
    return 0;
}

static void kid_audio_enc_close_internal(void)
{
    intercom_voice_recoder_close();
    if (kid_aud) {
        kid_aud->enc_opened = 0;
    }
}

static void kid_audio_dec_close_internal(struct kid_audio_test *t)
{
    if (!t) {
        return;
    }
    if (t->wake_timer) {
        sys_hi_timer_del(t->wake_timer);
        t->wake_timer = 0;
    }
    t->dec_node = NULL;
    if (t->dec_opened) {
        intercom_rx_player_close(0);
        t->dec_opened = 0;
    }
    if (t->audio_file) {
        fclose(t->audio_file);
        t->audio_file = NULL;
    }
    if (g_dec_ctx == t) {
        g_dec_ctx = NULL;
    }
}

/* intercom_rx_player 单例: 开新解码流前先关掉旧的 (对讲/文件互斥) */
static int kid_audio_dec_open_internal(struct kid_audio_test *t)
{
    struct intercom_rx_player_param param = {0};
    int err;

    kid_audio_dec_close_internal(kid_aud);
    kid_audio_dec_close_internal(kid_audio_play);

    g_dec_ctx = t;
    t->dec_opened = 1; /* 即便 open 失败, close 也安全 */

    param.coding_type  = KID_AUDIO_CODING_TYPE;
    param.channel_mode = AUDIO_CH_MIX;
    param.sample_rate  = 16000;
    param.bit_rate     = 16000;
    param.type         = INTERCOM_SERVICE_VOICE;
    param.frame_dms    = 200;                  /* 20ms */
    param.format_mode  = 0;                    /* 百度无头, 与编码 format=0 一致 */
    param.packet_len   = KID_AUDIO_FRAME_BYTE; /* CBR 解码帧长 40B */

    err = intercom_rx_player_open(NULL, 0, &param, kid_audio_rx_input);
    if (err) {
        log_error("player open fail %d\n", err);
        t->dec_opened = 0;
        g_dec_ctx = NULL;
        return err;
    }

    if (!t->wake_timer) {
        t->wake_timer = sys_hi_timer_add(NULL, kid_audio_wake_timer, KID_AUDIO_WAKE_TIMER_MS);
    }
    return 0;
}

/* ====== 对外 API (签名与 br28 一致) ====== */
void audio_app_open(void)
{
    if (kid_aud) {
        return;
    }
    log_info("%s \n", __func__);

    kid_aud = zalloc(sizeof(struct kid_audio_test));
    ASSERT(kid_aud);
    cbuf_init(&kid_aud->cbuf, kid_aud->data, KID_AUDIO_BUF_LEN);
#if KID_AUDIO_SEND_CBUF_EN
    cbuf_init(&kid_aud->send_cbuf, kid_aud->send_data, KID_AUDIO_SEND_BUF_LEN);
#endif

    if (kid_audio_enc_open_internal()) {
        log_error("enc open fail\n");
        free(kid_aud);
        kid_aud = NULL;
        return;
    }

    vstream_sender_audio_init();

    os_time_dly(10); /* 等编码流水线启动 */

    kid_audio_dec_open_internal(kid_aud);

    audio_app_volume_set(APP_AUDIO_STATE_MUSIC, 100, 1);
}

void audio_app_stop(void)
{
    if (!kid_aud) {
        return;
    }
    log_info("%s \n", __func__);

    kid_audio_enc_close_internal();
    kid_audio_dec_close_internal(kid_aud);

    vstream_sender_audio_deinit();

    free(kid_aud);
    kid_aud = NULL;
}

void audio_app_rec_init(void)
{
    if (kid_aud) {
        return;
    }
    log_info("%s \n", __func__);

    kid_aud = zalloc(sizeof(struct kid_audio_test));
    ASSERT(kid_aud);
#if KID_AUDIO_SEND_CBUF_EN
    cbuf_init(&kid_aud->send_cbuf, kid_aud->send_data, KID_AUDIO_SEND_BUF_LEN);
#endif

    if (kid_audio_enc_open_internal()) {
        log_error("rec_init enc open fail\n");
        free(kid_aud);
        kid_aud = NULL;
        return;
    }

    vstream_sender_audio_init();
}

void audio_app_rec_stop(void)
{
    if (!kid_aud) {
        return;
    }
    log_info("%s \n", __func__);

    kid_audio_enc_close_internal();

    vstream_sender_audio_deinit();

    free(kid_aud);
    kid_aud = NULL;
}

void audio_app_play_init(u8 *path)
{
    if (kid_audio_play) {
        return;
    }

    kid_audio_play = zalloc(sizeof(struct kid_audio_test));
    ASSERT(kid_audio_play);
    kid_audio_play->audio_file = fopen((char *)path, "r");
    if (!kid_audio_play->audio_file) {
        free(kid_audio_play);
        kid_audio_play = NULL;
        return;
    }
    cbuf_init(&kid_audio_play->cbuf, kid_audio_play->data, KID_AUDIO_BUF_LEN);

    kid_audio_dec_open_internal(kid_audio_play);
}

void audio_app_play_stop(void)
{
    if (!kid_audio_play) {
        return;
    }
    log_info("%s \n", __func__);

    kid_audio_dec_close_internal(kid_audio_play);

    free(kid_audio_play);
    kid_audio_play = NULL;
}

int audio_app_play_status()
{
    return !!kid_audio_play;
}

void audio_app_suspend()
{
    if (!kid_aud) {
        return;
    }
    log_info("%s \n", __func__);
    /* BR33 编解码器无 suspend/resume, 用 close 代替; kid_aud 与 send_cbuf 保留 */
    kid_audio_enc_close_internal();
}

void audio_app_resume()
{
    if (!kid_aud) {
        printf("%s kid_aud err\n", __func__);
        return;
    }
    log_info("%s \n", __func__);
    if (kid_audio_enc_open_internal()) {
        log_error("resume enc open fail\n");
    }
}

/* 外部获取编码输出音频数据 */
u16 audio_app_data_get(u8 *buf, u16 len)
{
#if KID_AUDIO_SEND_CBUF_EN
    if (kid_aud) {
        return cbuf_read(&kid_aud->send_cbuf, buf, len);
    }
#endif
    return 0;
}


/* ====== transport weak 桩 (BR33 暂未适配 vstream/SPP) ======
 * 真实实现接入后, 强符号自动覆盖以下弱定义, 无需修改本文件. */


#endif /* TCFG_KID_AUDIO_APP_ENABLE */
