#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".voice_mic_data.data.bss")
#pragma data_seg(".voice_mic_data.data")
#pragma const_seg(".voice_mic_data.text.const")
#pragma code_seg(".voice_mic_data.text")
#endif
/*****************************************************************
>file name : voice_mic_data.c
>author : lichao
>create time : Mon 01 Nov 2021 11:33:32 AM CST
*****************************************************************/
#include "app_config.h"
#include "voice_mic_data.h"
#include "smart_voice.h"
#include "app_main.h"
#include "adc_file.h"
#include "aec_uart_debug.h"
#include "effects/convert_data.h"
#include "circular_buf.h"
#include "media/audio_general.h"
#include "audio_splicing.h"
#include "media/media_memory.h"
#include "clock_manager/clock_manager.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif /*TCFG_AUDIO_ANC_ENABLE*/

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
#include "audio_vad/vad_mic.h"
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/

#if TCFG_SMART_VOICE_USE_AEC
#include "cvp/aec_ref_dac_ch_data.h"
#include "audio_cvp.h"
#include "overlay_code.h"
#include "audio_dac.h"
static DEFINE_SPINLOCK(voice_aec_lock);

#if CONFIG_VAD_AEC_REF_SUPPORT_IIS
#include "audio_iis.h"
#endif
#endif

//使用外部存储buf(aec_ref_dac_ch)的读回采方式
#define CONFIG_VOICE_MIC_DAC_REF_EXT        CONFIG_VAD_PLATFORM_SUPPORT_EN

//在aec_ref_dac_ch_fill中就做SRC(运行在jlstream线程)节省搬运buf，若阻塞影响主通路播歌效率，需要关闭该选项
#define CONFIG_VOICE_MIC_DAC_REF_SRC_PRE    1

#if !CONFIG_VOICE_MIC_DAC_REF_EXT
#undef CONFIG_VOICE_MIC_DAC_REF_SRC_PRE
#define CONFIG_VOICE_MIC_DAC_REF_SRC_PRE    0
#endif

#if ((defined TCFG_SMART_VOICE_ENABLE) && TCFG_SMART_VOICE_ENABLE)
extern struct audio_adc_hdl adc_hdl;

#define CONFIG_VOICE_MIC_DATA_DUMP          0

//使能AEC时导出三路数据，不使能AEC时导出一路数据
#define CONFIG_VOICE_MIC_DATA_EXPORT        0

#if (CONFIG_VOICE_MIC_DATA_EXPORT && (TCFG_AUDIO_DATA_EXPORT_DEFINE != AUDIO_DATA_EXPORT_VIA_UART))
#error "please open AUDIO_DATA_EXPORT_VIA_UART"
#endif

#define MAIN_ADC_GAIN               8
#define MAIN_ADC_BUF_NUM            3

struct main_adc_context {
    struct audio_adc_output_hdl dma_output;
    struct adc_mic_ch mic_ch;
    s16 *dma_buf;
    s16 *mic_sample_data;
    u8 adc_ch_num;  //打开的adc ch数量
    u8 adc_seq;     //记录当前用的那个mic
    u8 bit_width;
};
/*
 * Mic 数据接收buffer(循环buffer，动态大小)
 */
struct voice_mic_data {
    u8 open;
    u8 source;
    struct main_adc_context *main_adc;
    void *vad_mic;
    struct list_head head;
    cbuffer_t cbuf;
    u8 buf[0];
};

struct voice_mic_capture_channel {
    void *priv;
    void (*output)(void *priv, s16 *data, int len);
    struct list_head entry;
};

static struct voice_mic_data *voice_handle = NULL;
extern const u8 const_adc_async_en;
#define __this      (voice_handle)

#if CONFIG_VOICE_MIC_DATA_DUMP
static u8 mic_data_dump = 0;
#endif

#if TCFG_SMART_VOICE_USE_AEC
#include "Resample_api.h"
#include "voice_mic_aec.h"
#define AEC_RUN_FRAME_LEN          512  //AEC处理每帧长度(byte)
#define AEC_REF_READLOCK_POINTS    320  //死区点数
#define AEC_REF_READ_GOBACK_POINTS 45   //读指针回退点数

struct voice_aec_hdl_t {
    cbuffer_t ref_cbuf;     //存储参考数据的cbuf
    int ref_buf_len;        //存储参考数据的cbuf的长度
    u8 *ref_buf;            //buf地址
    cbuffer_t aec_cbuf;     //存储AEC输出buf
    int aec_buf_len;        //存储2帧，512*2 byte
    u8 *aec_buf;            //buf地址
    int dac_ch_buf_len_ext; //vad使用，额外存储多帧数据
    int read_lock;          //dac_ref_buf预留多一部分读死区，用于回退读指针
#if !CONFIG_VOICE_MIC_DAC_REF_SRC_PRE
    RS_STUCT_API *sw_src_api; //src
    u32 *sw_src_buf;          //src
#endif
    u8 bit_width;           //参考数据位宽
    u8 ref_channel;         //参考数据通道数
    u8 multiple_cnt;        //mic sr和ref sr的倍数
    volatile u8 ref_align;  //置1后下次读参考数据前需要对齐
};
static struct voice_aec_hdl_t *voice_aec_hdl = NULL;

void audio_smart_voice_aec_ref_align_set()
{
    struct voice_aec_hdl_t *hdl = voice_aec_hdl;
    if (!hdl || !__this) {
        return;
    }
    hdl->ref_align = 1;
    cbuf_clear(&hdl->ref_cbuf);
    cbuf_clear(&hdl->aec_cbuf);
    cbuf_clear(&__this->cbuf);
#if !CONFIG_VOICE_MIC_DAC_REF_EXT
    audio_dac_read_reset();
#endif
}

void audio_smart_voice_aec_ref_dac_ch_data_read(struct voice_aec_hdl_t *hdl, void *data, int len)
{
    if (!hdl) {
        return;
    }
    if (hdl->ref_align) {
        //DAC已开始写非静音数据，可对齐
        if (get_aec_ref_dac_ch_write_flag()) {
            //获取的MIC数据帧数
            int frame_num = len / (VOICE_MIC_DATA_PERIOD_FRAMES * (hdl->bit_width ? 4 : 2) * hdl->multiple_cnt);
            printf("align_frame_num %d\n", frame_num);

            int ext_len = 0;
            if (frame_num > 1) {
                //大于1帧：VAD初次启动，需要对齐缓存数据并推出
                //等于1帧：VAD已启动再开AEC
#if CONFIG_VOICE_MIC_DAC_REF_SRC_PRE
                ext_len = hdl->dac_ch_buf_len_ext * (audio_general_get_global_sample_rate() / VOICE_ADC_SAMPLE_RATE);
#else
                ext_len = hdl->dac_ch_buf_len_ext;
#endif
            }

            //取同一时刻的长度用于对齐
            int aec_ref_len = aec_ref_dac_ch_data_size_get();
#if CONFIG_VAD_AEC_REF_SUPPORT_IIS
            int align_len = audio_iis_get_buffered_frames(iis_hdl[CONFIG_VAD_AEC_REF_SUPPORT_IIS_MODULE], CONFIG_VAD_AEC_REF_SUPPORT_IIS_CH) *
                            (hdl->bit_width ? 4 : 2) * hdl->ref_channel + ext_len;
#else
            int align_len = (JL_AUDDAC->DAC_LEN - JL_AUDDAC->DAC_SWN - 1) *
                            (hdl->bit_width ? 4 : 2) * hdl->ref_channel + ext_len;
#endif

            //对齐操作，判断是否成功对齐
            if (aec_ref_dac_ch_data_align_same_time(align_len, aec_ref_len)) {

#if AEC_REF_READ_GOBACK_POINTS
                //读指针回退，控制MIC数据与参考数据延时差
                int read_goback = AEC_REF_READ_GOBACK_POINTS * hdl->ref_channel *
                                  (audio_general_get_global_sample_rate() / VOICE_ADC_SAMPLE_RATE) * (hdl->bit_width ? 4 : 2);
                aec_ref_dac_ch_data_read_goback(read_goback);
#endif

                //VAD初次启动，需要推出缓存数据
                if (frame_num > 1) {
                    aec_ref_dac_ch_data_read((hdl->ref_channel == 2) ? 1 : 0, data, len * hdl->ref_channel);
                }

                hdl->ref_align = 0;
            }
        }
        return;
    }

    aec_ref_dac_ch_data_read((hdl->ref_channel == 2) ? 1 : 0, data, len * hdl->ref_channel);
}

//len 单声道，16bit，16k采样率，长度与VAD中断输出相同
void audio_smart_voice_aec_ref_data_fill(int len)
{
    if (!voice_aec_hdl) {
        return;
    }
    struct voice_aec_hdl_t *hdl = voice_aec_hdl;

    len *= hdl->multiple_cnt; //按倍数确认实际读取长度

#if CONFIG_VOICE_MIC_DAC_REF_EXT
    s16 *ref_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC_TMP, len * (hdl->bit_width ? 2 : 1));
    audio_smart_voice_aec_ref_dac_ch_data_read(hdl, ref_buf, len * (hdl->bit_width ? 2 : 1));
#else
    s16 *ref_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC_TMP, len * (hdl->bit_width ? 2 : 1) * hdl->ref_channel);
    audio_dac_read(0, ref_buf, len / hdl->ref_channel, hdl->ref_channel);
    if (hdl->ref_channel == 2) {
        if (hdl->bit_width) {
            pcm_dual_to_single_32bit(ref_buf, ref_buf, len * hdl->ref_channel * 2);
        } else {
            pcm_dual_to_single(ref_buf, ref_buf, len * hdl->ref_channel);
        }
    }
#endif
    if (hdl->bit_width) { //若dac数据是32bit(qval 23)，需要转换为16bit
        audio_convert_data_process(ref_buf, ref_buf, len * 2, 1, 0, AUDIO_QVAL_24BIT);
    }
    int ret = cbuf_write(&hdl->ref_cbuf, ref_buf, len);
    if (ret != len) {
        printf("%s write err %d %d\n", __func__, ret, len);
    }
    media_free(ref_buf);
}

int audio_smart_voice_aec_open(void)
{
    if (voice_aec_hdl) {
        printf("voice_aec_hdl is alreadly open !!!");
        return -1;
    }
    struct voice_aec_hdl_t *hdl = media_malloc(AUD_MODULE_SMART_VOICE_AEC, sizeof(struct voice_aec_hdl_t));
    if (hdl == NULL) {
        printf("hdl malloc fail !!!");
        return -1;
    }

#if CONFIG_VAD_PLATFORM_SUPPORT_EN && CONFIG_VAD_AEC_REF_SUPPORT_IIS
    hdl->ref_channel = 2;
#else
    hdl->ref_channel = AUDIO_CH_NUM(TCFG_AUDIO_DAC_CONNECT_MODE);
    ASSERT((hdl->ref_channel == 2) || (hdl->ref_channel == 1));
#endif
#if CONFIG_VOICE_MIC_DAC_REF_SRC_PRE
    hdl->multiple_cnt = 1;
#else
    hdl->multiple_cnt = audio_general_get_global_sample_rate() / VOICE_ADC_SAMPLE_RATE;
#endif
    hdl->bit_width = audio_general_out_dev_bit_width();
    printf("smart_voice_aec_ref_data_bit_width %d", hdl->bit_width);

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    hdl->ref_buf_len = (VAD_CBUF_FRAME_CNT_TOTAL * VAD_CBUF_FRAME_SIZE) * hdl->multiple_cnt;
    hdl->ref_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC, hdl->ref_buf_len);
#else
    hdl->ref_buf_len = (VOICE_MIC_DATA_PERIOD_FRAMES * 2) * hdl->multiple_cnt * 4; //够存4帧数据
    hdl->ref_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC, hdl->ref_buf_len);
#endif
    cbuf_init(&hdl->ref_cbuf, hdl->ref_buf, hdl->ref_buf_len);

    hdl->aec_buf_len = AEC_RUN_FRAME_LEN * 2; //2帧AEC数据
    hdl->aec_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC, hdl->aec_buf_len);
    cbuf_init(&hdl->aec_cbuf, hdl->aec_buf, hdl->aec_buf_len);

#if !CONFIG_VOICE_MIC_DAC_REF_SRC_PRE
    //SRC
    hdl->sw_src_api = get_rs16_context();
    u32 sw_src_need_buf = hdl->sw_src_api->need_buf();
    hdl->sw_src_buf = media_malloc(AUD_MODULE_SMART_VOICE_AEC, sw_src_need_buf);
    RS_PARA_STRUCT rs_para_obj;
    rs_para_obj.nch = hdl->ref_channel;
    rs_para_obj.new_insample = audio_general_get_global_sample_rate();
    rs_para_obj.new_outsample = VOICE_ADC_SAMPLE_RATE;
    rs_para_obj.dataTypeobj.IndataBit = hdl->bit_width;
    rs_para_obj.dataTypeobj.OutdataBit = hdl->bit_width;
    rs_para_obj.dataTypeobj.IndataInc = hdl->ref_channel;
    rs_para_obj.dataTypeobj.OutdataInc = hdl->ref_channel;
    rs_para_obj.dataTypeobj.Qval = hdl->bit_width ? 23 : 15;
    hdl->sw_src_api->open(hdl->sw_src_buf, &rs_para_obj);
#endif

    //AEC
    struct voice_mic_aec_open_param param = {0};
    param.enablebit = (AEC_EN | NLP_EN);
    //aec_param
    param.M = 128;
    param.B = 2;
    param.ConverThr = 20.0f;
    param.RefEngThr = -70.0f;
    param.DT_AggressiveFactor = 5.0f;
    //nlp_param
    param.AggressFactor = -3.0f;
    param.minSuppress = 4.0f;
    param.OverSuppress = 2.0f;
    param.OverSuppressThr = 0.02f;
    voice_mic_aec_open(&param);

#if CONFIG_VOICE_MIC_DAC_REF_EXT
    //REF
#if CONFIG_VAD_AEC_REF_SUPPORT_IIS
    set_aec_ref_iis_ch_name(SMART_VOICE_AEC_REF_NODE_NAME);
#else
    set_aec_ref_dac_ch_name(SMART_VOICE_AEC_REF_NODE_NAME);
#endif
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    hdl->dac_ch_buf_len_ext = (VAD_CBUF_FRAME_CNT_TOTAL * VAD_POINT_PER_FRAME * hdl->ref_channel * hdl->multiple_cnt) * (hdl->bit_width ? 4 : 2);
#else
    hdl->dac_ch_buf_len_ext = 0;
#endif
    hdl->read_lock = AEC_REF_READLOCK_POINTS * hdl->ref_channel * hdl->multiple_cnt * (hdl->bit_width ? 4 : 2);

#if CONFIG_VAD_AEC_REF_SUPPORT_IIS
    int hw_dma_len = audio_iis_get_buffered_len(iis_hdl[CONFIG_VAD_AEC_REF_SUPPORT_IIS_MODULE],
                     CONFIG_VAD_AEC_REF_SUPPORT_IIS_CH);
#else
    int hw_dma_len = dac_hdl.output_buf_len;
#endif
    printf("hw_dma_len %d, dac_ch_buf_len_ext %d, read_lock %d\n", hw_dma_len, hdl->dac_ch_buf_len_ext, hdl->read_lock);
    struct aec_ref_open_param aec_ref_param = {0};
    aec_ref_param.hw_dma_len = hw_dma_len;
    aec_ref_param.bit_width = audio_general_out_dev_bit_width();
    aec_ref_dac_ch_data_read_init(&aec_ref_param);
    aec_ref_dac_ch_data_buf_len_set(hdl->dac_ch_buf_len_ext + hw_dma_len + hdl->read_lock);

#if CONFIG_VOICE_MIC_DAC_REF_SRC_PRE
    RS_PARA_STRUCT rs_para_obj;
    rs_para_obj.nch = AUDIO_CH_NUM(TCFG_AUDIO_DAC_CONNECT_MODE);
    rs_para_obj.new_insample = audio_general_get_global_sample_rate();
    rs_para_obj.new_outsample = VOICE_ADC_SAMPLE_RATE;
    rs_para_obj.dataTypeobj.IndataBit = hdl->bit_width;
    rs_para_obj.dataTypeobj.OutdataBit = hdl->bit_width;
    rs_para_obj.dataTypeobj.IndataInc = AUDIO_CH_NUM(TCFG_AUDIO_DAC_CONNECT_MODE);
    rs_para_obj.dataTypeobj.OutdataInc = AUDIO_CH_NUM(TCFG_AUDIO_DAC_CONNECT_MODE);
    rs_para_obj.dataTypeobj.Qval = hdl->bit_width ? 23 : 15;
    aec_ref_dac_ch_data_src_init(&rs_para_obj);
#endif
#endif

    voice_aec_hdl = hdl;

    audio_smart_voice_aec_ref_align_set();
    return 0;
}

void audio_smart_voice_aec_close(void)
{
    spin_lock(&voice_aec_lock);
    if (voice_aec_hdl) {
#if CONFIG_VOICE_MIC_DAC_REF_EXT
        aec_ref_dac_ch_data_read_exit();
#endif
        if (voice_aec_hdl->ref_buf) {
            media_free(voice_aec_hdl->ref_buf);
            voice_aec_hdl->ref_buf = NULL;
        }
        if (voice_aec_hdl->aec_buf) {
            media_free(voice_aec_hdl->aec_buf);
            voice_aec_hdl->aec_buf = NULL;
        }
#if !CONFIG_VOICE_MIC_DAC_REF_SRC_PRE
        if (voice_aec_hdl->sw_src_api) {
            voice_aec_hdl->sw_src_api = NULL;
        }
        if (voice_aec_hdl->sw_src_buf) {
            media_free(voice_aec_hdl->sw_src_buf);
            voice_aec_hdl->sw_src_buf = NULL;
        }
#endif
        media_free(voice_aec_hdl);
        voice_aec_hdl = NULL;
    }
    spin_unlock(&voice_aec_lock);
    voice_mic_aec_close();
}

//统一在语音识别线程开关，防止临界区问题
void audio_smart_voice_aec_open_by_kws_task()
{
    if (!__this) {
        return;
    }
    int argv[2] = {0};
    argv[0] = (int)audio_smart_voice_aec_open;
    argv[1] = 0;
    int ret = os_taskq_post_type(ASR_CORE, Q_CALLBACK, sizeof(argv) / sizeof(int), argv);
    if (ret) {
        printf("audio_smart_voice_aec_open_by_kws_task error\n");
    }
}

void audio_smart_voice_aec_close_by_kws_task()
{
    if (!__this) {
        return;
    }
    int argv[2] = {0};
    argv[0] = (int)audio_smart_voice_aec_close;
    argv[1] = 0;
    int ret = os_taskq_post_type(ASR_CORE, Q_CALLBACK, sizeof(argv) / sizeof(int), argv);
    if (ret) {
        printf("audio_smart_voice_aec_close_by_kws_task error\n");
    }
}

#endif // TCFG_SMART_VOICE_USE_AEC

static int voice_mic_data_output(void *priv, s16 *data, int len)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)priv;
    int wlen = 0;
    struct voice_mic_capture_channel *ch;
    list_for_each_entry(ch, &voice->head, entry) {
        if (ch->output) {
            ch->output(ch->priv, data, len);
        }
    }
#if TCFG_SMART_VOICE_USE_AEC
    spin_lock(&voice_aec_lock);
    audio_smart_voice_aec_ref_data_fill(len);
    spin_unlock(&voice_aec_lock);
#endif
    wlen = cbuf_write(&voice->cbuf, data, len);
    if (wlen < len) {
        printf("%s cbuf_write err %d, %d\n", __func__, wlen, len);
    }
    return wlen;
}

static void audio_main_adc_dma_data_handler(void *priv, s16 *data, int len)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)priv;
    if (!voice || voice->source != VOICE_MCU_MIC) {
        return;
    }

    s16 *pcm_data = NULL;
    if (voice->main_adc->bit_width != ADC_BIT_WIDTH_16) {
        s32 *s32_src = (s32 *)data;
        s32 *s32_dst = (s32 *)voice->main_adc->mic_sample_data;
        if (voice->main_adc->adc_ch_num > 1) {
            for (int i = 0; i < len / 4; i++) {
                s32_dst[i] = s32_src[i * voice->main_adc->adc_ch_num + voice->main_adc->adc_seq];
            }
            audio_convert_data_32bit_to_16bit_round(s32_dst, (s16 *)s32_dst, len / 4);
        } else {
            audio_convert_data_32bit_to_16bit_round(s32_src, (s16 *)s32_dst, len / 4);
        }
        pcm_data = (s16 *)s32_dst;
        len >>= 1;
    } else {
        s16 *s16_src = (s16 *)data;
        s16 *s16_dst = (s16 *)voice->main_adc->mic_sample_data;
        if (voice->main_adc->adc_ch_num > 1) {
            for (int i = 0; i < len / 2; i++) {
                s16_dst[i] = s16_src[i * voice->main_adc->adc_ch_num + voice->main_adc->adc_seq];
            }
            pcm_data = (s16 *)s16_dst;

        } else {
            pcm_data = (s16 *)data;
        }
    }
    /* putchar('M'); */
    voice_mic_data_output(voice, pcm_data, len);
    smart_voice_core_post_msg(1, SMART_VOICE_MSG_DMA);
}

#if TCFG_CALL_KWS_SWITCH_ENABLE
static void audio_main_adc_mic_close(struct voice_mic_data *voice, u8 all_channel);
static void audio_main_adc_suspend_handler(int all_channel, int arg)
{
    OS_SEM *sem = (OS_SEM *)arg;
    if (__this) {
        audio_main_adc_mic_close(__this, all_channel);
    }
    os_sem_post(sem);
}

void audio_main_adc_suspend_in_core_task(u8 all_channel)
{
    if (!__this || !__this->main_adc) {
        return;
    }
    int argv[5];
    OS_SEM *sem = media_malloc(AUD_MODULE_SMART_VOICE, sizeof(OS_SEM));
    os_sem_create(sem, 0);

    argv[0] = (int)audio_main_adc_suspend_handler;
    argv[1] = 2;
    argv[2] = all_channel;
    argv[3] = (int)sem;

    do {
        int err = os_taskq_post_type(ASR_CORE, Q_CALLBACK, 4, argv);
        if (err == OS_ERR_NONE) {
            break;
        }

        if (err != OS_Q_FULL) {
            audio_main_adc_suspend_handler(all_channel, (int)sem);
            goto exit;
        }
        os_time_dly(2);
    } while (1);

    os_sem_pend(sem, 100);
exit:
    media_free(sem);
}

void kws_aec_data_output(void *priv, s16 *data, int len)
{
    if (!__this || __this->source != VOICE_MCU_MIC) {
        return;
    }

    audio_main_adc_suspend_in_core_task(0);

    voice_mic_data_output(__this, data, len);
    smart_voice_core_post_msg(1, SMART_VOICE_MSG_DMA);
}

u8 kws_get_state(void)
{
    if (!__this || __this->source != VOICE_MCU_MIC) {
        return 0;
    }
    return 1;
}

void smart_voice_mcu_mic_suspend(void)
{
    if (!__this || __this->source != VOICE_MCU_MIC) {
        return;
    }

    //打开adc前, 如果kws本身打开了mic，需要close
    audio_main_adc_suspend_in_core_task(1);
}
#endif


#define audio_main_adc_mic_ch_setup(ch, mic_ch, ch_map, adc_handle) \
    if (ch_map & BIT(ch)) { \
        audio_adc_mic##ch##_open(mic_ch, ch_map, adc_handle); \
    }

static int audio_main_adc_mic_open(struct voice_mic_data *voice)
{
    int ret = 0;
    if (voice->main_adc || adc_file_is_runing()) {
        printf("audio main adc mic already  open:-%p -%d !!!", voice->main_adc, adc_file_is_runing());
        return 0;
    }
    voice->main_adc = media_malloc(AUD_MODULE_SMART_VOICE, sizeof(struct main_adc_context));

    if (!voice->main_adc) {
        return -ENOMEM;
    }

#if TCFG_AUDIO_ANC_ENABLE && (!TCFG_AUDIO_DYNAMIC_ADC_GAIN)
    /* MAIN_ADC_GAIN = anc_mic_gain_get(); */
#elif TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_DYNAMIC_ADC_GAIN
    anc_dynamic_micgain_start(MAIN_ADC_GAIN);
#endif/*TCFG_AUDIO_ANC_ENABLE && (!TCFG_AUDIO_DYNAMIC_ADC_GAIN)*/

    if (const_adc_async_en) {
        /*是否4个adc通道都打开，音箱使用*/
        voice->main_adc->adc_ch_num = AUDIO_ADC_MAX_NUM;
    } else {
        /*默认打开一个adc通道做语音识别，耳机使用*/
        voice->main_adc->adc_ch_num = 1;
    }

    voice->main_adc->bit_width = adc_hdl.bit_width;
    /*设置adc buf大小*/
    int dma_len = 0;
    if (voice->main_adc->bit_width == ADC_BIT_WIDTH_16) {
        dma_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(short) * MAIN_ADC_BUF_NUM * voice->main_adc->adc_ch_num;
    } else {
        dma_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(int) * MAIN_ADC_BUF_NUM * voice->main_adc->adc_ch_num;
    }
    voice->main_adc->dma_buf = media_malloc(AUD_MODULE_SMART_VOICE, dma_len);
    printf("adc dma_len %d ", dma_len);

    /*设置缓存buf大小*/
    int buf_len = 0;
    if (voice->main_adc->bit_width == ADC_BIT_WIDTH_16) {
        if (voice->main_adc->adc_ch_num > 1) {
            buf_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(short);
        }
    } else {
        if (voice->main_adc->adc_ch_num > 1) {
            buf_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(int);
        } else {
            buf_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(short);
        }
    }
    printf("mic sample data len %d", buf_len);
    if (buf_len) {
        voice->main_adc->mic_sample_data = media_malloc(AUD_MODULE_SMART_VOICE, buf_len);
    }

    /*配置使用哪个mic做语音识别*/
#ifdef TCFG_SMART_VOICE_MIC_CH_SEL
    u8 ch = TCFG_SMART_VOICE_MIC_CH_SEL;
#else
    u8 ch = AUDIO_ADC_MIC_0;
#endif
    printf("%s:%d", __func__, ch);
    audio_adc_file_init();

    for (int i = 0; i < AUDIO_ADC_MAX_NUM; i++) {
        if (ch & AUDIO_ADC_MIC(i)) {
            adc_file_mic_open(&voice->main_adc->mic_ch, AUDIO_ADC_MIC(i));
            audio_adc_mic_set_gain(&voice->main_adc->mic_ch, AUDIO_ADC_MIC(i), MAIN_ADC_GAIN);
        }
    }

    audio_adc_mic_set_sample_rate(&voice->main_adc->mic_ch, VOICE_ADC_SAMPLE_RATE);
    ret =  audio_adc_mic_set_buffs(&voice->main_adc->mic_ch, voice->main_adc->dma_buf,
                                   VOICE_MIC_DATA_PERIOD_FRAMES * 2, MAIN_ADC_BUF_NUM);
    if (ret && voice->main_adc->dma_buf) {
        /*已经设置过buf了，并不需要在set buf*/
        media_free(voice->main_adc->dma_buf);
        voice->main_adc->dma_buf = NULL;
    }
    voice->main_adc->dma_output.priv = voice;
    voice->main_adc->dma_output.handler = audio_main_adc_dma_data_handler;
    audio_adc_add_output_handler(&adc_hdl, &voice->main_adc->dma_output);
    audio_adc_mic_start(&voice->main_adc->mic_ch);
    voice->main_adc->adc_seq = get_adc_seq(&adc_hdl, ch); //查询模拟mic对应的ADC通道,打开4个adc通道时使用
    printf("adc_seq %d", voice->main_adc->adc_seq);

    return 0;
}

static void audio_main_adc_mic_close(struct voice_mic_data *voice, u8 all_channel)
{
    if (voice->main_adc) {
#if TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_DYNAMIC_ADC_GAIN
        anc_dynamic_micgain_stop();
#endif/*TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_DYNAMIC_ADC_GAIN*/
        if (all_channel) {
            audio_adc_mic_close(&voice->main_adc->mic_ch);
        }
        audio_adc_del_output_handler(&adc_hdl, &voice->main_adc->dma_output);
        if (voice->main_adc->dma_buf) {
            /*mic close时会自动释放内存*/
            /* media_free(voice->main_adc->dma_buf); */
            voice->main_adc->dma_buf = NULL;
        }
        if (voice->main_adc->mic_sample_data) {
            media_free(voice->main_adc->mic_sample_data);
            voice->main_adc->mic_sample_data = NULL;
        }
        media_free(voice->main_adc);
        voice->main_adc = NULL;
    }
}


void *voice_mic_data_open(u8 source, int buffer_size, int sample_rate)
{
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    buffer_size = (VAD_CBUF_FRAME_CNT_TOTAL * VAD_CBUF_FRAME_SIZE);
#endif
    if (!__this) {
        __this = media_malloc(AUD_MODULE_SMART_VOICE, sizeof(struct voice_mic_data) + buffer_size);
    }

    if (!__this) {
        return NULL;
    }

    if (__this->open) {
        return __this;
    }
    cbuf_init(&__this->cbuf, __this->buf, buffer_size);
    __this->source = source;
    INIT_LIST_HEAD(&__this->head);

    clock_alloc("ASR", ASR_CLK);

#if CONFIG_VOICE_MIC_DATA_EXPORT
#if TCFG_SMART_VOICE_USE_AEC
    aec_uart_open_v2(3, buffer_size * 3);
#else
    aec_uart_open_v2(1, buffer_size);
#endif
#endif

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    if (source == VOICE_VAD_MIC) {
        __this->vad_mic = lp_vad_mic_open((void *)__this, voice_mic_data_output);
    } else if (source == VOICE_MCU_MIC)
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    {
        audio_main_adc_mic_open(__this);
        smart_voice_core_post_msg(1, SMART_VOICE_MSG_WAKE);
    }
    __this->open = 1;
    return __this;
}

void voice_mic_data_close(void *mic)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    if (voice->source == VOICE_VAD_MIC) {
        lp_vad_mic_close(voice->vad_mic);
        voice->vad_mic = NULL;
    } else if (voice->source == VOICE_MCU_MIC)
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    {
        audio_main_adc_mic_close(voice, 1);
        smart_voice_core_post_msg(1, SMART_VOICE_MSG_STANDBY);
    }

    clock_free("ASR");

#if CONFIG_VOICE_MIC_DATA_EXPORT
    aec_uart_close_v2();
#endif
    if (voice) {
        media_free(voice);
    }
    __this = NULL;
}

void voice_mic_data_switch_source(void *mic, u8 source, int buffer_size, int sample_rate)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;

    voice->source = source;

    if (voice->source == VOICE_VAD_MIC) {
        audio_main_adc_mic_close(voice, 1);
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
        if (!voice->vad_mic) {
            voice->vad_mic = lp_vad_mic_open(voice, voice_mic_data_output);
        }
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    } else if (voice->source == VOICE_MCU_MIC) {
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
        if (voice->vad_mic) {
            lp_vad_mic_close(voice->vad_mic);
            voice->vad_mic = NULL;
        }
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
        audio_main_adc_mic_open(voice);
        smart_voice_core_post_msg(1, SMART_VOICE_MSG_WAKE);
    }
}

void *voice_mic_data_capture(int sample_rate, void *priv, void (*output)(void *priv, s16 *data, int len))
{
    struct voice_mic_capture_channel *ch = (struct voice_mic_capture_channel *)media_malloc(AUD_MODULE_SMART_VOICE, sizeof(struct voice_mic_capture_channel));

    if (!ch) {
        return NULL;
    }
    voice_mic_data_open(VOICE_VAD_MIC, 2048, sample_rate);
    if (!__this) {
        media_free(ch);
        return NULL;
    }
    ch->priv = priv;
    ch->output = output;
    list_add(&ch->entry, &__this->head);

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    lp_vad_mic_test();
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    return ch;
}

void voice_mic_data_stop_capture(void *mic)
{
    struct voice_mic_capture_channel *ch = (struct voice_mic_capture_channel *)mic;

    if (ch) {
        list_del(&ch->entry);
        media_free(ch);
    }

    if (list_empty(&__this->head)) {
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
        lp_vad_mic_test();
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    }
}

#if TCFG_SMART_VOICE_USE_AEC
//读取参考数据和MIC数据并做AEC处理
int voice_mic_data_read_with_aec(void *mic, void *data, int len)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;
    struct voice_aec_hdl_t *hdl = voice_aec_hdl;
    if (!voice || !hdl) {
        return 0;
    }

    //AEC处理后的数据是否够一帧语音识别
    if (cbuf_get_data_len(&hdl->aec_cbuf) > len) {
        int ret = cbuf_read(&hdl->aec_cbuf, data, len);
        return ret;
    }
    //MIC数据是否够一帧AEC处理
    if ((cbuf_get_data_len(&voice->cbuf) > AEC_RUN_FRAME_LEN) && \
        (cbuf_get_data_len(&hdl->ref_cbuf) > AEC_RUN_FRAME_LEN * hdl->multiple_cnt)) {
        s16 *mic_data = media_malloc(AUD_MODULE_SMART_VOICE_AEC_TMP, AEC_RUN_FRAME_LEN);
        s16 *ref_data = media_malloc(AUD_MODULE_SMART_VOICE_AEC_TMP, AEC_RUN_FRAME_LEN * hdl->multiple_cnt);
        s16 *out_data = media_malloc(AUD_MODULE_SMART_VOICE_AEC_TMP, AEC_RUN_FRAME_LEN);
        cbuf_read(&voice->cbuf, mic_data, AEC_RUN_FRAME_LEN);
        cbuf_read(&hdl->ref_cbuf, ref_data, AEC_RUN_FRAME_LEN * hdl->multiple_cnt);
#if !CONFIG_VOICE_MIC_DAC_REF_SRC_PRE
        //SRC RUN
        int out_len = hdl->sw_src_api->run(hdl->sw_src_buf, ref_data, (AEC_RUN_FRAME_LEN * hdl->multiple_cnt) >> 1, ref_data);
        out_len <<= 1;
        ASSERT(out_len == AEC_RUN_FRAME_LEN);
#endif
#if CONFIG_VOICE_MIC_DATA_EXPORT
        aec_uart_fill_v2(0, mic_data, AEC_RUN_FRAME_LEN);
        aec_uart_write_v2();
        aec_uart_fill_v2(1, ref_data, AEC_RUN_FRAME_LEN);
        aec_uart_write_v2();
#endif

        voice_mic_aec_run(mic_data, out_data, ref_data, AEC_RUN_FRAME_LEN);

#if CONFIG_VOICE_MIC_DATA_EXPORT
        aec_uart_fill_v2(2, out_data, AEC_RUN_FRAME_LEN);
        aec_uart_write_v2();
#endif
        int ret = cbuf_write(&hdl->aec_cbuf, out_data, AEC_RUN_FRAME_LEN);
        if (ret != AEC_RUN_FRAME_LEN) {
            printf("voice_mic_data_read_with_aec cbuf_write err %d %d\n", ret, AEC_RUN_FRAME_LEN);
        }
        media_free(mic_data);
        media_free(ref_data);
        media_free(out_data);
        if (cbuf_get_data_len(&hdl->aec_cbuf) > len) {
            return cbuf_read(&hdl->aec_cbuf, data, len);
        }
    }
    return 0;
}
#endif

//直接读取MIC数据
int voice_mic_data_read_base(void *mic, void *data, int len)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;
    if (cbuf_get_data_len(&voice->cbuf) < len) {
        return 0;
    } else {
        int wlen = cbuf_read(&voice->cbuf, data, len);


#if (CONFIG_VOICE_MIC_DATA_EXPORT && !TCFG_SMART_VOICE_USE_AEC)
        aec_uart_fill_v2(0, data, len);
        aec_uart_write_v2();
#endif
        return wlen;
    }
}

int voice_mic_data_read(void *mic, void *data, int len)
{
#if TCFG_SMART_VOICE_USE_AEC
    if (voice_aec_hdl) {
        return voice_mic_data_read_with_aec(mic, data, len);
    } else {
        return voice_mic_data_read_base(mic, data, len);
    }
#else
    return voice_mic_data_read_base(mic, data, len);
#endif
}

int voice_mic_data_buffered_samples(void *mic)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;

    return cbuf_get_data_len(&voice->cbuf) >> 1;
}

void voice_mic_data_clear(void *mic)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;

    cbuf_clear(&voice->cbuf);
}

void voice_mic_data_dump(void *mic)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;

    if (voice == NULL) {
        return;
    }
#if CONFIG_VOICE_MIC_DATA_DUMP
    mic_data_dump = 1;
    int len = 0;
    int i = 0;
    s16 *data = (s16 *)cbuf_read_alloc(&voice->cbuf, &len);

    len >>= 1;
    if (data) {
#if 0
        for (i = 0; i < len; i++) {
            if ((i % 3000) == 0) {
                wdt_clear();
            }
            printf("%d\n", data[i]);
        }
#else
        put_buf(data, len << 1);
#endif
    }
    cbuf_read_updata(&voice->cbuf, len << 1);
    mic_data_dump = 0;
#endif
}

#endif
