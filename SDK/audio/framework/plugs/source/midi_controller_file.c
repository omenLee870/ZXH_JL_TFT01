#include "source_node.h"
#include "media/audio_splicing.h"
#include "audio_config.h"
#include "jlstream.h"
#include "app_config.h"
#include "sync/audio_clk_sync.h"
#include "gpio.h"
#include "audio_config.h"
#include "audio_cvp.h"
#include "effects/effects_adj.h"
#include "media/audio_general.h"
#include "circular_buf.h"
#include "decoder_node.h"
#include "codec/audio_midi.h"
#include "framework/include/decoder_node.h"
#include "audio_general_config.h"
#include "audio_config_def.h"


#if TCFG_MIDI_CONTROLLER_NODE_ENABLE

#define MIDI_CTRL_LOG_ENABLE          1
#if MIDI_CTRL_LOG_ENABLE
#define LOG_TAG     "[Midi-ctrl]"
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

#pragma   code_seg(".midi.text.cache.L2.code")
#pragma  const_seg(".midi.text.cache.L2.const")

#define MIDI_OUTPUT_POINTS_PER_CH 92//缓冲输出

struct midi_ctrl_hdl {
    void *decoder;
    int (*run)(void *);
    void *source_node;
    struct stream_node *node;
    struct decoder_dec_task *task;
    const struct decoder_plug_ops *plug;
    cbuffer_t cbuf[MIDI_CTRL_OPORT_NUM];
    void *buf[MIDI_CTRL_OPORT_NUM];
    midi_ctrl_open_parm *param;
    OS_MUTEX mutex;
    u32 coding_type;
    u32 sample_rate;
    u32 timestamp;
    u32 next_timestamp;
    u16 plug_uuid;
    u16 timer;
    u16 frame_len;
    u8 start;
    u8 channel_mode;
    u8 bit_width;
    u8 scene;
    u8 dec_end;
    u8 output_frame_cnt;
    u8 no_sync;
};


#define MIDI_STREAM_ACTIVE_CH 0
int midi_flow_ctrl(struct midi_ctrl_hdl *hdl)
{
    int ret = 0;
    int frame_num = jlstream_for_node(hdl->node)->thread_num == 1 ? 1 : 2;
    struct stream_oport *oport = source_plug_get_oport_by_id(hdl->source_node, MIDI_STREAM_ACTIVE_CH);
    if (oport) {
        ret = jlstream_flow_ctrl_check(oport->next, frame_num);
    }
    return ret;
}
//multi_out
int midi_ctrl_decoder_plug_output_data(void *_hdl, void *data, int len)
{
#if 0
    //直接输出
    struct midi_ctrl_hdl *hdl = (struct midi_ctrl_hdl *)_hdl;

    int frame_len = len / MIDI_CTRL_OPORT_NUM;
    for (int i = MIDI_CTRL_OPORT_NUM - 1; i >= 0 ; i--) {
        struct stream_frame *frame = source_plug_get_output_frame_by_id(hdl->source_node, i, frame_len);
        if (!frame) {
            continue;
        }

        frame->len = frame_len;
        memcpy(frame->data, (void *)((u32)data + frame_len * i), frame_len);
        source_plug_put_output_frame_by_id(hdl->source_node, i, frame);
        if (i == MIDI_STREAM_ACTIVE_CH) {
            hdl->output_frame_cnt += 1;
            jlstream_wakeup_thread(NULL, hdl->node, NULL);
        }

    }
    return len;
#else
    //缓存组帧长
    if (!_hdl) {
        return 0;
    }
    struct midi_ctrl_hdl *hdl = (struct midi_ctrl_hdl *)_hdl;
    if (!hdl->node || !hdl->source_node || hdl->frame_len <= 0) {
        return 0;
    }
    struct stream_node *node = hdl->node;

    if (hdl->output_frame_cnt) {
        return 0;
    }

    int frame_len = len / MIDI_CTRL_OPORT_NUM;
    int offset[MIDI_CTRL_OPORT_NUM] = {0};
    char *data_ptr = (char *)data;

    if (hdl->timestamp == 0) {
        hdl->timestamp = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
        hdl->next_timestamp = hdl->timestamp;
    }
    for (int i = 0; i < MIDI_CTRL_OPORT_NUM; i++) {
        int remain = hdl->frame_len - cbuf_get_data_len(&hdl->cbuf[i]);
        int wlen = (frame_len > remain) ? remain : frame_len;
        if (wlen <= 0) {
            continue;
        }
        offset[i] = cbuf_write(&hdl->cbuf[i], data_ptr + frame_len * i, wlen);
    }
    for (int i = MIDI_CTRL_OPORT_NUM - 1; i >= 0 ; i--) {
        if (cbuf_get_data_len(&hdl->cbuf[i]) == hdl->frame_len) {
            struct stream_frame *frame = source_plug_get_output_frame_by_id(hdl->source_node, i, hdl->frame_len);
            if (!frame) {
                continue;
            }
            if (!hdl->no_sync) {
                frame->timestamp =  hdl->next_timestamp;
                frame->flags = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
            }
            frame->len = hdl->frame_len;
            cbuf_read(&hdl->cbuf[i], frame->data, hdl->frame_len);
            if (!hdl->no_sync) {
                if (i == (MIDI_CTRL_OPORT_NUM - 1)) {
                    hdl->next_timestamp = frame->timestamp + PCM_SAMPLE_TO_TIMESTAMP(hdl->frame_len / (hdl->bit_width ? 4 : 2) / AUDIO_CH_NUM(hdl->channel_mode), (int)hdl->sample_rate);
                }
            }

            source_plug_put_output_frame_by_id(hdl->source_node, i, frame);

            if (i == MIDI_STREAM_ACTIVE_CH) {
                hdl->output_frame_cnt += 1;
                jlstream_wakeup_thread(NULL, hdl->node, NULL);
            }

        }
    }
    // 6. 处理剩余数据
    for (int i = 0; i < MIDI_CTRL_OPORT_NUM; i++) {
        if (offset[i] > 0 && offset[i] < frame_len) {
            int remain_write = frame_len - offset[i];
            int cbuf_remain = hdl->frame_len - cbuf_get_data_len(&hdl->cbuf[i]);
            if (remain_write > 0 && remain_write <= cbuf_remain) {
                cbuf_write(&hdl->cbuf[i], data_ptr + frame_len * i + offset[i], remain_write);
            }
        }
    }
    return len;
#endif
}
/*
 * @description: iis file 初始化
 * @return: midi_ctrl_hdl 结构体类型的指针
 * @note: 无
 */
static void *midi_ctrl_init(void *source_node, struct stream_node *node)
{
    struct midi_ctrl_hdl *hdl = zalloc(sizeof(*hdl));
    log_debug("--- midi_ctrl_init ---\n");


    hdl->source_node = source_node;
    hdl->node = node;
    hdl->bit_width = audio_general_in_dev_bit_width();
    hdl->plug_uuid = get_source_node_plug_uuid(source_node);

    return hdl;
}

static void midi_ctrl_ioc_get_fmt(struct midi_ctrl_hdl *hdl, struct stream_fmt *fmt)
{
    log_debug("==========  midi_ctrl_ioc_get_fmt  ========== \n");
    fmt->coding_type = AUDIO_CODING_PCM;	//output pcm
    switch (hdl->scene) {
    default:
        hdl->channel_mode   = AUDIO_CH_LR;
        break;
    }
    fmt->channel_mode = hdl->channel_mode;
    if (!hdl->sample_rate) {
        hdl->sample_rate = MIDI_DEC_SR;
    }

    fmt->sample_rate = hdl->sample_rate;
    fmt->bit_wide = hdl->bit_width;
}

static int midi_ctrl_ioc_set_fmt(struct midi_ctrl_hdl *hdl, struct stream_fmt *fmt)
{
    log_debug("==========  midi_ctrl_ioc_set_fmt  ==========\n");
    hdl->sample_rate = fmt->sample_rate;
    return 0;
}

static void midi_decoder_run(struct midi_ctrl_hdl *hdl)
{
    struct stream_node *node = hdl->node;

    if (!hdl->decoder) {
        return;
    }
    if (node->state & NODE_STA_DEC_END) {
        return;
    }

    if (hdl->dec_end == 0) {
        hdl->output_frame_cnt = 0;
        int err = os_mutex_pend(&hdl->mutex, 2);
        if (err) {
            return;
        }
        for (int i = 0; i < 20; i++) {
            err = hdl->run(hdl->decoder);
            if (err || hdl->output_frame_cnt) {
                break;
            }
        }
        os_mutex_post(&hdl->mutex);

        if (err) {
            log_debug("midi_ctrl dec_err: %x\n", err);
            hdl->dec_end = 1;
        }
    }

    if (hdl->dec_end) {
        hdl->node->state = NODE_STA_DEC_END;
    }
}


static void decoder_task(void *_hdl)
{
    struct midi_ctrl_hdl *hdl = (struct midi_ctrl_hdl *)_hdl;

    while (1) {
        os_sem_pend(&hdl->task->dec_sem, 0);

        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            os_task_del_res(OS_TASK_SELF);
        }

        if (midi_flow_ctrl(hdl)) {
            continue;
        }

        midi_decoder_run(hdl);

    }
}


static int midi_ctrl_decoder_init_plug(struct midi_ctrl_hdl *hdl, int coding_type, u8 bit_width,
                                       struct stream_fmt *fmt)
{
    const struct decoder_plug_ops *plug;

    plug = get_decoder(coding_type);
    if (!plug) {
        return -ENOENT;
    }

    struct stream_decoder_info info = {
        .scene          = hdl->scene,
        .coding_type    = coding_type,
        .task_name      = NULL,
    };
    jlstream_event_notify(STREAM_EVENT_LOAD_DECODER, (int)&info);
    void *decoder = plug->init((struct decoder_hdl *)hdl);
    if (!decoder) {
        jlstream_event_notify(STREAM_EVENT_UNLOAD_DECODER, (int)&info);
        return -ENOMEM;
    }
    if (hdl->param) {
        u8 bitwidth = hdl->bit_width ? 24 : 16;
        hdl->param->ctrl_parm.priv = hdl;
        hdl->param->ctrl_parm.output = midi_ctrl_decoder_plug_output_data;
        if (hdl->param->ctrl_parm.bitwidth != bitwidth) {
            hdl->param->ctrl_parm.bitwidth = bitwidth;
            hdl->param->cfg_parm.bitwidth = bitwidth;
            hdl->param->cfg_parm.OutdataBit = hdl->bit_width;
        }
        if (AUDIO_CH_NUM(hdl->channel_mode) != hdl->param->cfg_parm.out_channel) {
            hdl->param->cfg_parm.out_channel = AUDIO_CH_NUM(hdl->channel_mode);
        }
        plug->ioctl(decoder, NODE_IOC_SET_PRIV_INFO, (int)hdl->param);
        free(hdl->param);
        hdl->param = NULL;
    } else {
        log_error("midi ctrl param NULL\n")	;
        jlstream_event_notify(STREAM_EVENT_UNLOAD_DECODER, (int)&info);
        return -ENOMEM;

    }

    if (fmt) {
        int err = plug->ioctl(decoder, NODE_IOC_GET_FMT, (int)fmt);
        if (err) {
            plug->release(decoder);
            jlstream_event_notify(STREAM_EVENT_UNLOAD_DECODER, (int)&info);
            return err;
        }
        fmt->coding_type = coding_type;
    }

    hdl->plug           = plug;
    hdl->decoder        = decoder;
    hdl->run            = plug->run;
    hdl->coding_type    = coding_type;
    hdl->task = zalloc(sizeof(struct decoder_dec_task));
    if (hdl->task) {
        hdl->task->task_name = "midi_ctrl";
        os_sem_create(&hdl->task->dec_sem, 0);
        os_mutex_create(&hdl->mutex);
    }

    return 0;
}

static void midi_ctrl_decoder_ioc_set_timestamp(struct midi_ctrl_hdl *hdl)
{
    int err = stream_node_ioctl(hdl->node, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SYNCTS, 0);
    if (err) {
        for (int i = 0; i < 2; i++) {
            err = stream_node_ioctl(hdl->node, (i == 1) ? NODE_UUID_CAPTURE_SYNC : NODE_UUID_PLAY_SYNC, NODE_IOC_SYNCTS, 0);
            if (!err) {
                return;
            } else {
                continue;
            }
        }
        if (err) {
            hdl->no_sync = 1;
            log_debug("===midi ctrl set local timestamp err, need play sync node===\n");
        }
        return;
    }
}
void midi_decoder_wakeup(void *p)
{
    struct midi_ctrl_hdl *hdl = (struct midi_ctrl_hdl *)p;
    if (!hdl) {
        return;
    }
    if (!midi_flow_ctrl(hdl)) {
        os_sem_set(&hdl->task->dec_sem, 0);
        os_sem_post(&hdl->task->dec_sem);
    }

}

static void midi_ctrl_start(struct midi_ctrl_hdl *hdl)
{
    hdl->dec_end = 0;
    midi_ctrl_decoder_init_plug(hdl, AUDIO_CODING_MIDI_CTRL, hdl->bit_width, NULL);
    if (hdl->task) {
        for (int i = 0; i < MIDI_CTRL_OPORT_NUM; i++) {
            int len = MIDI_OUTPUT_POINTS_PER_CH * AUDIO_CH_NUM(hdl->channel_mode) * (hdl->bit_width ? 4 : 2);
            hdl->frame_len = len;
            hdl->buf[i] = zalloc(len);
            if (hdl->buf[i]) {
                cbuf_init(&hdl->cbuf[i], hdl->buf[i], len);
            }
        }

        task_create(decoder_task, hdl, hdl->task->task_name);
        if (!hdl->timer) {
            u32 time = MIDI_OUTPUT_POINTS_PER_CH * 1000 / hdl->sample_rate;
            time = (time < 1) ? 1 : time;
            midi_decoder_wakeup(hdl);
            hdl->timer = sys_hi_timer_add(hdl, midi_decoder_wakeup, time);
        }
    }

}

static void midi_ctrl_stop(struct midi_ctrl_hdl *hdl)
{
    if (hdl->task) {
        if (hdl->timer) {
            sys_hi_timer_del(hdl->timer);
            hdl->timer = 0;
        }
        task_delete(hdl->task->task_name, &hdl->task->dec_sem);
        free(hdl->task);
        hdl->task = NULL;
    }
    if (hdl->plug) {
        jlstream_event_notify(STREAM_EVENT_UNLOAD_DECODER, hdl->coding_type);
        hdl->plug->ioctl(hdl->decoder, NODE_IOC_STOP, 0);
        hdl->plug->release(hdl->decoder);
        hdl->plug = NULL;
        hdl->decoder = NULL;
    }
    for (int i = 0; i < MIDI_CTRL_OPORT_NUM; i++) {
        if (hdl->buf[i]) {
            free(hdl->buf[i]);
            hdl->buf[i] = NULL;
        }
    }
    if (hdl->param) {
        free(hdl->param);
        hdl->param = NULL;
    }
}

static int midi_ctrl_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct midi_ctrl_hdl *hdl = (struct midi_ctrl_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_GET_FMT:
        midi_ctrl_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        midi_ctrl_decoder_ioc_set_timestamp(hdl);
        break;
    case NODE_IOC_SET_FMT:
        ret = midi_ctrl_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_SET_PRIV_FMT:
        break;
    case NODE_IOC_START:
        midi_ctrl_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        midi_ctrl_stop(hdl);
        break;
    case NODE_IOC_SET_PRIV_INFO:
        if (!hdl->param) {
            hdl->param = zalloc(sizeof(midi_ctrl_open_parm));
        }
        if (hdl->param) {
            memcpy(hdl->param, (void *)arg, sizeof(midi_ctrl_open_parm));
        } else {
            log_error("midi ctrl param NULL\n")	;
        }
        break;

    default:
        if (hdl->task) {
            os_mutex_pend(&hdl->mutex, 0);
            if (hdl->plug) {
                ret = hdl->plug->ioctl(hdl->decoder, cmd, arg);
                if (cmd == NODE_IOC_GET_PRIV_FMT) {
                    struct stream_fmt *priv_fmt	= (struct stream_fmt *)arg;
                    priv_fmt->coding_type = hdl->coding_type;
                }
            }
            os_mutex_post(&hdl->mutex);
        }
        break;
    }


    return ret;
}

/*
 * @description: 释放掉 iis 申请的内存
 * @param: 参数传入 midi_ctrl_hdl 结构体指针类型的参数
 * @return：无
 * @note: 无
 */
static void midi_ctrl_release(void *_hdl)
{
    struct midi_ctrl_hdl *hdl = (struct midi_ctrl_hdl *)_hdl;

    free(hdl);
    hdl = NULL;
    log_debug(">>[%s] : midi_crtl release succ\n", __func__);
}

REGISTER_SOURCE_NODE_PLUG(midi_ctrl_plugs) = {
    .uuid       = NODE_UUID_MIDI_CONTROLLER,
    .init       = midi_ctrl_init,
    .ioctl      = midi_ctrl_ioctl,
    .release    = midi_ctrl_release,
};
#endif


