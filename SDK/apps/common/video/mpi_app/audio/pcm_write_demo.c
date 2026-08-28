
//////////////////////////////////////////////////////////////
//
//	video_voice_recoder demo
//  编码数据写入SD卡文件 pcm.bin
//
//////////////////////////////////////////////////////////////
#include "fs/fs.h"
#include "jlstream.h"
/* #include "system/malloc.h" */

extern int video_voice_recoder_open(u32 code_type, u8 ai_type, u32 sample_rate, u32 bit_rate);
extern void video_voice_recoder_close();
extern void video_voice_recoder_set_video_tx_node_func(int (*func)(u8 *, u32));

#if 1

#define SD0_PCM_FILE_PATH    "storage/sd0/C/pcm.pcm"

struct video_voice_recoder_demo {
    FILE *file;
    u8 started;
};

static struct video_voice_recoder_demo *g_demo = NULL;

static int video_voice_recoder_demo_tx_cb(u8 *buf, u32 len)
{
    struct video_voice_recoder_demo *demo = g_demo;

    if (!demo || !demo->file || !buf || !len) {
        return 0;
    }

    int wlen = fwrite(buf, len, 1, demo->file);
    if (wlen != len) {
        printf("video_voice_recoder_demo: write file err, wlen=%d, len=%d\n", wlen, len);
    }

    return 0;
}

int video_voice_recoder_demo_start(void)
{
    if (g_demo) {
        printf("video_voice_recoder_demo already started\n");
        return -EBUSY;
    }

    g_demo = zalloc(sizeof(*g_demo));
    if (!g_demo) {
        return -ENOMEM;
    }

    /* 打开文件，sd0 挂载在 application 之前需要确保已经挂载 */
    g_demo->file = fopen(SD0_PCM_FILE_PATH, "w+");
    if (!g_demo->file) {
        printf("video_voice_recoder_demo: open file %s failed\n", SD0_PCM_FILE_PATH);
        free(g_demo);
        g_demo = NULL;
        return -EFAULT;
    }

    /* 设置编码数据输出回调，写入文件 */

    /* 打开编码器，AUDIO_CODING_OPUS / AUDIO_CODING_SPEEX */
    int err = video_voice_recoder_open(AUDIO_CODING_PCM, 0, 16000, 0);
    video_voice_recoder_set_video_tx_node_func(video_voice_recoder_demo_tx_cb);
    if (err) {
        printf("video_voice_recoder_demo: open failed, err=%d\n", err);
        fclose(g_demo->file);
        free(g_demo);
        g_demo = NULL;
        return err;
    }

    g_demo->started = 1;
    printf("video_voice_recoder_demo started, file: %s\n", SD0_PCM_FILE_PATH);

    return 0;
}

void video_voice_recoder_demo_stop(void)
{
    if (!g_demo) {
        return;
    }

    video_voice_recoder_close();

    if (g_demo->file) {
        fclose(g_demo->file);
        g_demo->file = NULL;
    }

    g_demo->started = 0;
    free(g_demo);
    g_demo = NULL;

    printf("video_voice_recoder_demo stopped\n");
}

#endif /* TCFG_VIDEO_VOICE_RECODER_DEMO_ENABLE */