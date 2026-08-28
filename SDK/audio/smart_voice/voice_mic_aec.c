#include "voice_mic_aec.h"
#include "app_config.h"
#include "overlay_code.h"
#include "media/media_memory.h"

#if TCFG_SMART_VOICE_USE_AEC

extern void aec_code_movable_load(void);
extern void aec_code_movable_unload(void);

struct voice_mic_aec {
    OS_MUTEX mutex;
    void *run_buf;
    void *tmp_buf;
    void *nlp_run_buf;
    void *nlp_tmp_buf;
    int enablebit;
};

struct voice_mic_aec hdl = {0};

void voice_mic_aec_open(struct voice_mic_aec_open_param *param)
{
    if (!((param->enablebit & AEC_EN) || (param->enablebit & NLP_EN))) {
        return;
    }
    hdl.enablebit = param->enablebit;

    os_mutex_create(&hdl.mutex);

    overlay_load_code(OVERLAY_AEC);
    aec_code_movable_load();
    //AEC
    if (hdl.enablebit & AEC_EN) {
        int buf_size = AptFilt_QueryBufSize(param->M, param->B);
        int tmp_buf_size = AptFilt_QueryTempBufSize(param->M, param->B);
        printf("voice_mic_aec buf_size %d, tmp_buf_size %d\n", buf_size, tmp_buf_size);
        hdl.run_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC, buf_size);
        hdl.tmp_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC, tmp_buf_size);
        AptFilt_Init(hdl.run_buf, param->M, param->B, param->ConverThr, param->RefEngThr, param->DT_AggressiveFactor);
    }

    //NLP
    if (hdl.enablebit & NLP_EN) {
        int buf_size = EchoSuppress_QueryBufSize();
        int tmp_buf_size = EchoSuppress_QueryTempBufSize();
        printf("voice_mic_nlp buf_size %d, tmp_buf_size %d\n", buf_size, tmp_buf_size);
        hdl.nlp_run_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC, buf_size);
        hdl.nlp_tmp_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC, tmp_buf_size);
        EchoSuppress_Init(hdl.nlp_run_buf, param->AggressFactor, param->minSuppress, param->OverSuppressThr, param->OverSuppress);
    }
}

void voice_mic_aec_close()
{
    os_mutex_pend(&hdl.mutex, 0);
    if (hdl.run_buf) {
        media_free(hdl.run_buf);
        hdl.run_buf = NULL;
    }
    if (hdl.tmp_buf) {
        media_free(hdl.tmp_buf);
        hdl.tmp_buf = NULL;
    }
    if (hdl.nlp_run_buf) {
        media_free(hdl.nlp_run_buf);
        hdl.nlp_run_buf = NULL;
    }
    if (hdl.nlp_tmp_buf) {
        media_free(hdl.nlp_tmp_buf);
        hdl.nlp_tmp_buf = NULL;
    }
    aec_code_movable_unload();
    os_mutex_post(&hdl.mutex);
}

//in out不可同地址
void voice_mic_aec_run(void *in, void *out, void *ref, int len)
{
    os_mutex_pend(&hdl.mutex, 0);

    if (!((hdl.enablebit & AEC_EN) || (hdl.enablebit & NLP_EN))) {
        memcpy(out, in, len);
        os_mutex_post(&hdl.mutex);
        return;
    }

    ASSERT(len == 512); //传入数据长度检查

    if (hdl.enablebit & AEC_EN) {
        if (!hdl.run_buf || !hdl.tmp_buf) {
            os_mutex_post(&hdl.mutex);
            return;
        }
        AptFilt_Process(hdl.run_buf, hdl.tmp_buf, ref, in, out, len / 2);
    } else {
        memcpy(out, in, len); //输出拷贝输入数据，NLP的ERROR需要用到
    }

    if (hdl.enablebit & NLP_EN) {
        if (!hdl.nlp_run_buf || !hdl.nlp_tmp_buf) {
            os_mutex_post(&hdl.mutex);
            return;
        }
        EchoSuppress_Process(hdl.nlp_run_buf, hdl.nlp_tmp_buf, ref, in, out, out, NULL, NULL, len / 2);
    }

    os_mutex_post(&hdl.mutex);
}

#endif
