#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iis_file.data.bss")
#pragma data_seg(".iis_file.data")
#pragma const_seg(".iis_file.text.const")
#pragma code_seg(".iis_file.text")
#endif
#include "source_node.h"
#include "media/audio_splicing.h"
#include "audio_config.h"
#include "jlstream.h"
#include "iis_file.h"
#include "app_config.h"
#include "media/audio_iis.h"
#include "sync/audio_clk_sync.h"
#include "gpio.h"
#include "audio_config.h"
#include "audio_cvp.h"
#include "effects/effects_adj.h"
#include "media/audio_general.h"
#include "circular_buf.h"
#include "audio_iis_lrclk_capture.h"
#include "effects/audio_howling_ahs.h"
#include "common/audio_general_config.h"

/************************ ********************* ************************/
/************************ 该文件是 IIS 输入用的 ************************/
/************************ ********************* ************************/

#if TCFG_IIS_RX_NODE_ENABLE || TCFG_TDM_RX_NODE_ENABLE

#define IIS_LOG_ENABLE          1
#if IIS_LOG_ENABLE
#define LOG_TAG_CONST    ALINK
#define LOG_TAG     "[IIS-FILE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif


#define MODULE_IDX_SEL (((hdl->plug_uuid == NODE_UUID_IIS1_RX) || (hdl->plug_uuid == NODE_UUID_TDM1_RX)) ? 1 : 0)
extern const float const_out_dev_pns_time_ms;

struct iis_file_hdl {
    void *source_node;
    struct stream_node *node;
    struct stream_snode *snode;
    enum stream_scene scene;
    enum audio_iis_state_enum state;
    struct audio_iis_rx_output_hdl output;
    cbuffer_t cache_cbuffer;
    u8 *cache_buf;
    u32 buf_len;
    int value;
    int force_dump;
    u32 sample_rate;
    u16 irq_points;
    u16 ch_idx;
    u16 plug_uuid;
    u8 start;
    u8 dump_cnt;
    u8 received_data;
    u8 channel_mode;
    u8 bit_width;
    u8 module_idx;
    u8 hw_ch_num;
    u8 out_ch_num;

};

/*
 * 24bit转32bit，处理符号位
 * bit23如果是1，高8位补1;如果是0，高8位补0*/
__NODE_CACHE_CODE(iis)
static void audio_data_24bit_to_32bit(void *buf, int npoint)
{
    s32 *data = (s32 *)buf;
    for (int i = 0; i < npoint; i++) {
        if (data[i] & 0x00800000) {
            data[i] = data[i] | 0xff000000;
        } else {
            data[i] = data[i] & 0x00ffffff;
        }
    }
}

__NODE_CACHE_CODE(iis)
static void iis_file_fade_in(struct iis_file_hdl *hdl, void *buf, int len)
{
    if (hdl->value < FADE_GAIN_MAX) {
        int fade_ms = 100;//ms
        int fade_step = FADE_GAIN_MAX / (fade_ms * hdl->sample_rate / 1000);
        if (fade_step < 1) {
            fade_step = 1;
        }
        if (hdl->bit_width == DATA_BIT_WIDE_16BIT) {
            hdl->value = jlstream_fade_in(hdl->value, fade_step, buf, len, hdl->out_ch_num);
        } else {
            hdl->value = jlstream_fade_in_32bit(hdl->value, fade_step, buf, len, hdl->out_ch_num);
        }
    }
}

/* IIS 接收中断测试 */
__NODE_CACHE_CODE(iis)
static void iis_rx_handle(void *priv, void *buf, int len)
{
    struct iis_file_hdl *hdl = priv;
    struct stream_frame *frame = NULL;
    if (!hdl) {
        return;
    }
    if (hdl->received_data == 0) {
        hdl->received_data = 1;
    }

    if (len == 0) {
        log_error("iis rx length error");
        return;
    }
    if (hdl->start == 0) {
        return;
    }

    if (hdl->bit_width) {
        audio_data_24bit_to_32bit(buf, len >> 2);
    }

    if (hdl->hw_ch_num == 2) {
        if (hdl->out_ch_num == 1) {
            if (hdl->bit_width) {
                pcm_dual_to_single_32bit(buf, buf, len);
            } else {
                pcm_dual_to_single(buf, buf, len);
            }
            len >>= 1;
        }
    }

#if defined(IIS_USE_DOUBLE_BUF_MODE_EN) && IIS_USE_DOUBLE_BUF_MODE_EN
    if (hdl->dump_cnt < 10) {
        hdl->dump_cnt++;
        return;
    }
    if (hdl->force_dump) {
        hdl->value = 0;
        return;
    }

    frame = source_plug_get_output_frame(hdl->source_node, len);
    if (!frame) {
        return;
    }
    frame->len          = len;
    frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
    frame->timestamp    = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
    memcpy(frame->data, buf, frame->len);

    iis_file_fade_in(hdl, frame->data, frame->len);//淡入处理
    source_plug_put_output_frame(hdl->source_node, frame);
#else
    if (!hdl->cache_buf) {
        //申请cbuffer
        hdl->buf_len = hdl->irq_points * (hdl->bit_width ? 4 : 2) * hdl->out_ch_num;
        int buf_len = hdl->irq_points * (hdl->bit_width ? 4 : 2) * hdl->out_ch_num * 3;
        hdl->cache_buf = malloc(buf_len);
        if (hdl->cache_buf) {
            cbuf_init(&hdl->cache_cbuffer, hdl->cache_buf, buf_len);
        }
    }
    if (hdl->cache_buf) {
        int wlen = cbuf_write(&hdl->cache_cbuffer, buf, len);
        if (wlen != len) {
            log_error("iis rx wlen %d != len %d\n", wlen, len);
        }
    }

    u32 cache_len = cbuf_get_data_len(&hdl->cache_cbuffer);
    if (cache_len >= hdl->buf_len) {
        if (hdl->dump_cnt < 10) {
            hdl->dump_cnt++;
            cbuf_read_updata(&hdl->cache_cbuffer, hdl->buf_len);
            return;
        }
        if (hdl->force_dump) {
            cbuf_read_updata(&hdl->cache_cbuffer, hdl->buf_len);
            hdl->value = 0;
            return;
        }

        frame = source_plug_get_output_frame(hdl->source_node, hdl->buf_len);
        if (!frame) {
            return;
        }
        frame->len          = hdl->buf_len;
#if 1
        frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
        frame->timestamp    = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
#else
        frame->flags        = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
        frame->timestamp    = audio_jiffies_usec();
#endif
        cbuf_read(&hdl->cache_cbuffer, frame->data, frame->len);

        iis_file_fade_in(hdl, frame->data, frame->len);//淡入处理

        source_plug_put_output_frame(hdl->source_node, frame);
    }
#endif

    if (hdl->scene == STREAM_SCENE_ESCO || hdl->scene == STREAM_SCENE_PC_MIC) {
#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && (defined TCFG_IIS_NODE_ENABLE)
        //scene1: 音频流iis_rx输入iis_tx输出
        /*对齐iis外部参考数据延时*/
        if (!get_audio_aec_rebooting()) {
            audio_cvp_ref_data_align();
        }
#else
        //scene2: 音频流iis_rx输入dac输出
        audio_cvp_phase_align();
#endif
    }
#if (defined(TCFG_HOWLING_AHS_NODE_ENABLE) && TCFG_HOWLING_AHS_NODE_ENABLE)
    if (hdl->scene == STREAM_SCENE_MIC_EFFECT || hdl->scene == STREAM_SCENE_LOUDSPEAKER_IIS) {
        if (audio_ahs_status()) {
            howling_ahs_read_ref_data();
        }
    }
#endif
}

void iis_rx_start(struct iis_file_hdl *hdl)
{
    if (hdl && hdl->state == AUDIO_IIS_STATE_INIT) {
        hdl->value = 0;
        audio_iis_open(iis_hdl[hdl->module_idx], hdl->ch_idx);
        audio_iis_set_rx_irq_points(iis_hdl[hdl->module_idx], hdl->ch_idx, hdl->irq_points);
        hdl->output.ch_idx = hdl->ch_idx;
        hdl->output.priv = hdl;
        hdl->output.handler = (void (*)(void *, void *, int))iis_rx_handle;
        audio_iis_add_rx_output_handler(iis_hdl[hdl->module_idx], &hdl->output);
        audio_iis_start(iis_hdl[hdl->module_idx]);
        hdl->state = AUDIO_IIS_STATE_START;
    }
}

/*
 * @description: 初始化iis rx
 * @return：iis_file_hdl 结构体指针
 * @note: 无
 */
void iis_rx_init(struct iis_file_hdl *hdl)
{
    if (hdl == NULL) {
        return;
    }
    log_debug(" --- iis_init ---\n");

#if 0 //使用wm8978模块作IIS输入
    u8 WM8978_Init(u8 dacen, u8 adcen);
    WM8978_Init(0, 1);
#endif

    // 初始化iis rx
    jlstream_read_node_data_by_cfg_index(hdl->plug_uuid, hdl->node->subid, 0, (void *)&hdl->ch_idx, NULL);
    if (!iis_hdl[hdl->module_idx]) {
        struct alink_param params = {0};
        params.module_idx = hdl->module_idx;
        for (int i = 0; i < ALINK_CH_NUM_MAX; i++) {
            params.dma_size[i]   = audio_iis_fix_dma_len(hdl->module_idx, TCFG_AUDIO_DAC_BUFFER_TIME_MS, hdl->irq_points, hdl->bit_width, hdl->hw_ch_num, hdl->sample_rate, i);
        }

        params.sr         = hdl->sample_rate;
        params.bit_width  = hdl->bit_width;
        params.fixed_pns  = const_out_dev_pns_time_ms;

        params.ch_num = MODULE_IDX_SEL ? ALINK1_CH_NUM : ALINK0_CH_NUM;
        params.alink_work_mode = MODULE_IDX_SEL ? ALINK1_WORK_MODE : ALINK0_WORK_MODE;
        params.clk_mode = MODULE_IDX_SEL ? ALINK1_CLK_MODE : ALINK0_CLK_MODE;
        params.io_drive_strength_mode = MODULE_IDX_SEL ? ALINK1_IO_DRIVE_STRENGTH_MODE : ALINK0_IO_DRIVE_STRENGTH_MODE;
        params.clk_close = TCFG_AUDIO_IIS_CLOCK_CLOSE;
        iis_hdl[hdl->module_idx] = audio_iis_init(params);
    }
    if (!iis_hdl[hdl->module_idx]) {
        log_debug("iis module_idx %d rx init err\n", hdl->module_idx);
        return;
    }
    audio_iis_check_hw_cfg_status(hdl->module_idx, hdl->ch_idx, ALINK_DIR_RX);

    hdl->state = AUDIO_IIS_STATE_INIT;
}


/*
 * @description: iis file 初始化
 * @return: iis_file_hdl 结构体类型的指针
 * @note: 无
 */
static void *iis_file_init(void *source_node, struct stream_node *node)
{
    struct iis_file_hdl *hdl = zalloc(sizeof(*hdl));
    log_debug("--- iis_file_init ---\n");



    hdl->source_node = source_node;
    hdl->node = node;
    hdl->snode = (struct stream_snode *)node;
    node->type |= NODE_TYPE_FLOW_CTRL;
    int bit_width = MODULE_IDX_SEL ? ALINK1_BIT_WIDTH : ALINK0_BIT_WIDTH;
    if (bit_width < 0) {
        hdl->bit_width = audio_general_in_dev_bit_width();
    } else {
        hdl->bit_width = bit_width;
    }
    hdl->plug_uuid = get_source_node_plug_uuid(source_node);
    hdl->module_idx = MODULE_IDX_SEL;

    hdl->hw_ch_num = MODULE_IDX_SEL ? ALINK1_CH_NUM : ALINK0_CH_NUM;
    return hdl;
}

static void iis_ioc_get_fmt(struct iis_file_hdl *hdl, struct stream_fmt *fmt)
{
    log_debug("==========  iis_ioc_get_fmt  ========== \n");
    fmt->coding_type = AUDIO_CODING_PCM;	//默认PCM
    struct audio_general_params *general_params = audio_general_get_param();
    if (!hdl->sample_rate) {
#if defined(AUDIO_IIS_LRCLK_CAPTURE_EN) && AUDIO_IIS_LRCLK_CAPTURE_EN
        hdl->sample_rate = audio_iis_get_lrclk_sample_rate(hdl->module_idx) ? audio_iis_get_lrclk_sample_rate(hdl->module_idx) : general_params->sample_rate;	//默认采样率值
#else
        hdl->sample_rate = general_params->sample_rate;	//默认采样率值
#endif
    }
    switch (hdl->scene) {
    case STREAM_SCENE_ESCO:
        fmt->sample_rate = 16000;
#if TCFG_IIS_NODE_ENABLE
        u32 sample_rate = hdl->sample_rate % fmt->sample_rate;
        if (sample_rate) {
            ASSERT(sample_rate == 0, "iis rx sr[%u] not divisible by fmt 16000", hdl->sample_rate);
        }
#else
        if (fmt->sample_rate != hdl->sample_rate) {
            hdl->sample_rate = fmt->sample_rate;
        }
#endif
        hdl->channel_mode   = AUDIO_CH_MIX;
        break;
    case STREAM_SCENE_LOUDSPEAKER_IIS:
    case STREAM_SCENE_WIRELESS_MIC:
    case STREAM_SCENE_MIC_EFFECT:
#if (defined(TCFG_HOWLING_AHS_NODE_ENABLE) && TCFG_HOWLING_AHS_NODE_ENABLE)
        hdl->sample_rate = 16000;
#endif
        hdl->channel_mode   = AUDIO_CH_MIX;
        break;
    case STREAM_SCENE_PC_MIC:
        hdl->channel_mode   = AUDIO_CH_MIX;
        break;
    default:
        hdl->channel_mode   = AUDIO_CH_LR;
        break;
    }

#if ((ALINK0_WORK_MODE >= DSP0_SHORT) || (ALINK1_WORK_MODE >= DSP0_SHORT))
    if (hdl->hw_ch_num == 8) {
        hdl->channel_mode   = AUDIO_CH_EIGHT;
    } else if (hdl->hw_ch_num == 7) {
        hdl->channel_mode   = AUDIO_CH_SEVEN;
    } else if (hdl->hw_ch_num == 6) {
        hdl->channel_mode   = AUDIO_CH_SIX;
    }   else if (hdl->hw_ch_num == 5) {
        hdl->channel_mode   = AUDIO_CH_FIVE;
    } else if (hdl->hw_ch_num == 4) {
        hdl->channel_mode   = AUDIO_CH_QUAD;
    } else if (hdl->hw_ch_num == 3) {
        hdl->channel_mode   = AUDIO_CH_TRIPLE;
    }  else if (hdl->hw_ch_num == 2) {
        hdl->channel_mode   = AUDIO_CH_LR;
    } else {
        hdl->channel_mode   = AUDIO_CH_MIX;
    }
#endif
    int alink_sr = MODULE_IDX_SEL ? ALINK1_SAMPLE_RATE : ALINK0_SAMPLE_RATE;
    if (alink_sr) {
        hdl->sample_rate = alink_sr;
        int ahs_en = 0;
#if (defined(TCFG_HOWLING_AHS_NODE_ENABLE) && TCFG_HOWLING_AHS_NODE_ENABLE)
        ahs_en = 1;
#endif
        //cppcheck-suppress knownConditionTrueFalse
        if (hdl->scene == STREAM_SCENE_ESCO || ahs_en) {
            ASSERT(16000 == hdl->sample_rate, "iis rx sr[%d] is not 16000 in esco", hdl->sample_rate);
        }
    }
    hdl->out_ch_num = AUDIO_CH_NUM(hdl->channel_mode);
    fmt->channel_mode = hdl->channel_mode;
    fmt->sample_rate = hdl->sample_rate;
    fmt->bit_wide = hdl->bit_width;
    log_debug("iis/tdm hw_ch_num: %d, out_ch:%d, sr:%d %d\n", hdl->hw_ch_num, hdl->out_ch_num, hdl->sample_rate, alink_sr);
}

static int iis_ioc_set_fmt(struct iis_file_hdl *hdl, struct stream_fmt *fmt)
{
    log_debug("==========  iis_ioc_set_fmt  ==========\n");
    hdl->sample_rate = fmt->sample_rate;
    return 0;
}

static void iis_file_start(struct iis_file_hdl *hdl)
{
    if (hdl->start == 0) {
        hdl->start = 1;
        iis_rx_init(hdl);
        iis_rx_start(hdl);
        hdl->dump_cnt = 0;
    }
}

static void iis_file_stop(struct iis_file_hdl *hdl)
{
    if (hdl->start) {
        hdl->start = 0;
        //停止 IIS 接收
        int ret = 0;
        hdl->output.ch_idx = hdl->ch_idx;
        audio_iis_del_rx_output_handler(iis_hdl[hdl->module_idx], &hdl->output);
        audio_iis_stop(iis_hdl[hdl->module_idx], hdl->ch_idx);
        // 释放IIS RX
        audio_iis_close(iis_hdl[hdl->module_idx]);
        ret = audio_iis_uninit(iis_hdl[hdl->module_idx]);
        if (ret) {
            iis_hdl[hdl->module_idx] = NULL;
        }
        if (hdl->cache_buf) {
            free(hdl->cache_buf);
            hdl->cache_buf = NULL;
        }
        hdl->state = AUDIO_IIS_STATE_CLOSE;
    }
}

static int iis_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct iis_file_hdl *hdl = (struct iis_file_hdl *)_hdl;
    switch (cmd) {
    case NODE_IOC_GET_FMT:
        iis_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_FMT:
        ret = iis_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_SET_PRIV_FMT:
#if TCFG_IIS_NODE_ENABLE
    {
        struct audio_general_params *general_params = audio_general_get_param();
        u32 ratio = general_params->sample_rate / ALINK_SR_16000;
        if (general_params->sample_rate % ALINK_SR_16000 == 0) {
            hdl->irq_points = arg * ratio;
        } else {
            hdl->irq_points = arg;
        }
    }
#else
    hdl->irq_points = arg;
#endif
        /*iis rx 是固定双声道数据输入*/
    log_debug("set iis rx irq points %d\n", hdl->irq_points);
    break;
    case NODE_IOC_START:
        iis_file_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        iis_file_stop(hdl);
        break;
    case NODE_IOC_FORCE_DUMP_PACKET:
        hdl->force_dump = arg;
        break;
    }
    return ret;
}

/*
 * @description: 释放掉 iis 申请的内存
 * @param: 参数传入 iis_file_hdl 结构体指针类型的参数
 * @return：无
 * @note: 无
 */
void iis_release(void *_hdl)
{
    struct iis_file_hdl *hdl = (struct iis_file_hdl *)_hdl;
    log_debug(" --- iis_release ---\n");

    free(hdl);
    hdl = NULL;

    log_debug(">>[%s] : iis rx release succ\n", __func__);
}

REGISTER_SOURCE_NODE_PLUG(iis_file_plug) = {
    .uuid       = NODE_UUID_IIS0_RX,
    .init       = iis_file_init,
    .ioctl      = iis_ioctl,
    .release    = iis_release,
};
REGISTER_SOURCE_NODE_PLUG(iis1_file_plug) = {
    .uuid       = NODE_UUID_IIS1_RX,
    .init       = iis_file_init,
    .ioctl      = iis_ioctl,
    .release    = iis_release,
};

#endif

#if TCFG_TDM_RX_NODE_ENABLE
REGISTER_SOURCE_NODE_PLUG(tdm_file_plug) = {
    .uuid       = NODE_UUID_TDM_RX,
    .init       = iis_file_init,
    .ioctl      = iis_ioctl,
    .release    = iis_release,
};
REGISTER_SOURCE_NODE_PLUG(tdm1_file_plug) = {
    .uuid       = NODE_UUID_TDM1_RX,
    .init       = iis_file_init,
    .ioctl      = iis_ioctl,
    .release    = iis_release,
};
#endif
