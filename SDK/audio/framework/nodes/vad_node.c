#include "jlstream.h"
#include "media/audio_base.h"
#include "app_config.h"
#include "vad_node.h"
#include "aec_uart_debug.h"
#include "media/tech_lib/jlsp_vad.h"

#if TCFG_AUDIO_VAD_NODE_ENABLE

#define LOG_TAG_CONST VAD
#define LOG_TAG     "[VAD-NODE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma   code_seg(".vad.text")
#pragma   data_seg(".vad.data")
#pragma  const_seg(".vad.text.const")
#endif

/* NN_VAD算法处理帧长
 * 非0 : 按照帧长输入数据到算法处理接口
 */
#define NN_VAD_FRAME_POINTS  (160)
#define UART_DEBUG_ENABLE    (0)

/*
 *声道转换类型选配
 * */
#define CHANNEL_ADAPTER_AUTO   0 //自动协商,通常用于无声道数转换的场景,结果随数据流配置自动适配
#define CHANNEL_ADAPTER_2TO4   1 //立体声转4声道协商使能,支持2to4,结果随数据流配置自动适配
#define CHANNEL_ADAPTER_1TO2   2 //单声道转立体声协商使能,支持1to2,结果随数据流配置自动适配
#define CHANNEL_ADAPTER_2TO6   3 //立体声转6声道协商使能,支持2to6,结果随数据流配置自动适配
#define CHANNEL_ADAPTER_2TO8   4 //立体声转8声道协商使能,支持2to8,结果随数据流配置自动适配
#define CHANNEL_ADAPTER_TYPE   CHANNEL_ADAPTER_AUTO //默认无声道转换

static void audio_vad_run(struct vad_node_hdl *hdl, s16 *indata, s16 *outdata, u32 indata_len);

void vad_dev_init(struct packet_ctrl *hdl, u32 process_points_per_ch)
{
    if (process_points_per_ch) {
        hdl->frame_len = process_points_per_ch * hdl->in_ch_num * (2 << hdl->bit_width);
        hdl->remain_buf = zalloc(hdl->frame_len);
    }
}

void vad_dev_process(struct packet_ctrl *hdl, struct stream_iport *iport,  struct stream_note *note)
{
    struct stream_frame *frame;
    struct stream_frame *out_frame = NULL;
    struct stream_node *node = iport->node;

    if (hdl->remain_buf) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            return;
        }
        /*算法出来一帧的数据长度，byte*/
        int total_len = hdl->remain_len + frame->len;  //记录目前还有多少数据
        u8 pack_frame_num = total_len / hdl->frame_len;//每次数据需要跑多少帧
        u16 pack_frame_len = pack_frame_num * hdl->frame_len;       //记录本次需要跑多少数据
        u16 out_frame_len = hdl->out_ch_num * pack_frame_len / hdl->in_ch_num;

        if (pack_frame_num) {
            if (!out_frame) {
                out_frame = jlstream_get_frame(node->oport, out_frame_len);
                if (!out_frame) {
                    return;
                }
            }
            int out_offset = 0;
            if (hdl->remain_len) {
                int need_size = hdl->frame_len - hdl->remain_len;
                /*拷贝上一次剩余的数据*/
                memcpy((u8 *)hdl->remain_buf + hdl->remain_len, frame->data, need_size);
                if (hdl->out_ch_num == hdl->in_ch_num) {
                    memcpy(out_frame->data, hdl->remain_buf, hdl->frame_len);
                }
                hdl->vad_run(hdl->node_hdl, (s16 *)hdl->remain_buf, (s16 *)out_frame->data,
                             hdl->frame_len);
                hdl->remain_len = 0;
                frame->offset += need_size;
                out_offset = hdl->frame_len * hdl->out_ch_num / hdl->in_ch_num;
                pack_frame_num--;
            }
            while (pack_frame_num--) {
                if (hdl->out_ch_num == hdl->in_ch_num) {
                    memcpy(out_frame->data + out_offset, frame->data + frame->offset,
                           hdl->frame_len);
                }
                hdl->vad_run(hdl->node_hdl, (s16 *)(frame->data + frame->offset),
                             (s16 *)(out_frame->data + out_offset), hdl->frame_len);
                frame->offset += hdl->frame_len;
                out_offset += hdl->frame_len * hdl->out_ch_num  / hdl->in_ch_num;
            }
            if (frame->offset < frame->len) {
                memcpy(hdl->remain_buf, frame->data + frame->offset, frame->len - frame->offset);
                hdl->remain_len = frame->len - frame->offset;
            }
            out_frame->len = out_frame_len;
            jlstream_push_frame(node->oport, out_frame);	//将数据推到oport
            jlstream_free_frame(frame);	//释放iport资源
        } else {
            /*当前数据不够跑一帧算法时*/
            memcpy((void *)((u8 *)hdl->remain_buf + hdl->remain_len), frame->data, frame->len);
            hdl->remain_len += frame->len;
            jlstream_free_frame(frame);	//释放iport资源
        }
    } else {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            return;
        }
        if (hdl->out_ch_num != hdl->in_ch_num) {
            int out_len = hdl->out_ch_num * frame->len / hdl->in_ch_num;
            out_frame = jlstream_get_frame(node->oport, out_len);
            if (!out_frame) {
                return;
            }
            hdl->vad_run(hdl->node_hdl, (s16 *)frame->data, (s16 *)out_frame->data, frame->len);
            out_frame->len = out_len;
            jlstream_push_frame(node->oport, out_frame);	//将数据推到oport
            jlstream_free_frame(frame);	//释放iport资源
        } else {
            hdl->vad_run(hdl->node_hdl, (s16 *)frame->data, (s16 *)frame->data, frame->len);
            jlstream_push_frame(node->oport, frame);	//将数据推到oport
        }
    }
}
void vad_dev_close(struct packet_ctrl *hdl)
{
    hdl->remain_len = 0;
    if (hdl->remain_buf) {
        free(hdl->remain_buf);
        hdl->remain_buf = NULL;
    }
}

/*
 * hdl->dev.sample_rate:采样率
 * hdl->dev.in_ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * hdl->dev.out_ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * hdl->dev.bit_width:位宽 0，16bit  1，32bit
 **/
static void audio_vad_init(struct vad_node_hdl *hdl)
{
    hdl->dev.node_hdl = hdl;
    hdl->dev.vad_run = (void (*)(void *, s16 *, s16 *, u32))audio_vad_run;
    vad_dev_init(&hdl->dev, NN_VAD_FRAME_POINTS);

    RESFILE *vad_fp = NULL;
    int model_size = 0;
    int param_len = 0;
    int rlen = 0;
    int heap_size = 0;
    int share_size = 1024 * 10;

    vad_fp = resfile_open(AUDIO_NN_VAD_PARAM_FILE);
    if (!vad_fp) {
        log_error("[err] open vad.bin fail !!!\n");
        goto error;
    }

    param_len = resfile_get_len(vad_fp);
    log_info("param_len %d\n", param_len);
    if (param_len <= 0) {
        log_error("[error] invalid param_len %d\n", param_len);
        goto error;
    }

    hdl->model = (char *)zalloc(param_len);
    if (!hdl->model) {
        log_error("[error] zalloc model failed !!!\n");
        goto error;
    }
    rlen = resfile_read(vad_fp, hdl->model, param_len);
    if (rlen != param_len) {
        log_error("[error] read vad.bin err !!! %d != %d\n", rlen, param_len);
        goto error;
    }
    resfile_close(vad_fp);
    vad_fp = NULL;

    heap_size = JLSP_vad_get_heap_size(hdl->model, &model_size);
    log_info("lib_heap_size %d model_size %d\n", heap_size, model_size);
    if (heap_size <= 0) {
        log_error("[error] invalid heap_size %d\n", heap_size);
        goto error;
    }

    hdl->heap_buffer = zalloc(heap_size);
    if (!hdl->heap_buffer) {
        log_error("[error] hdl->heap_buffer zalloc failed\n");
        goto error;
    }

    hdl->share_buffer = zalloc(share_size);
    if (!hdl->share_buffer) {
        log_error("[error] hdl->share_buffer zalloc failed\n");
        goto error;
    }

    hdl->vad = JLSP_vad_init(hdl->heap_buffer,
                             heap_size,
                             hdl->share_buffer,
                             share_size,
                             hdl->model,
                             model_size);
    if (!hdl->vad) {
        log_error("[error] JLSP_vad_init failed\n");
        goto error;
    }

    JLSP_vad_reset(hdl->vad);
#if UART_DEBUG_ENABLE
#if TCFG_AUDIO_VAD_TYPE & NN_VAD
    aec_uart_open(1, 320);
#endif
#endif
    log_info("VAD init success\n");
    return;

error:
    if (vad_fp) {
        resfile_close(vad_fp);
    }
    if (hdl->model) {
        free(hdl->model);
        hdl->model = NULL;
    }
    if (hdl->heap_buffer) {
        free(hdl->heap_buffer);
        hdl->heap_buffer = NULL;
    }
    if (hdl->share_buffer) {
        free(hdl->share_buffer);
        hdl->share_buffer = NULL;
    }
    if (hdl->vad) {
        free(hdl->vad);
        hdl->vad = NULL;
    }
    log_info("VAD init failed, resources cleaned\n");
}

/*
 * hdl->dev.sample_rate:采样率
 * hdl->dev.in_ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * hdl->dev.out_ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * hdl->dev.bit_width:位宽 0，16bit  1，32bit
 * *indata:输入数据地址
 * *outdata:输出数据地址
 * indata_len :输入数据长度,byte
 * */
static void audio_vad_run(struct vad_node_hdl *hdl, s16 *indata, s16 *outdata, u32 indata_len)
{
    if (hdl->vad) {
#if UART_DEBUG_ENABLE
#if TCFG_AUDIO_VAD_TYPE & NN_VAD
        aec_uart_fill(0, indata, indata_len);
        aec_uart_write();
#endif
#endif
        JLSP_vad_process(hdl->vad, indata, &hdl->out_flag); /*0是非语音，1是语音*/
    }
    log_debug(">>>>> hdl->out_flag %d\n", hdl->out_flag);
}

static void audio_vad_exit(struct vad_node_hdl *hdl)
{
    if (hdl->vad) {
        JLSP_vad_free(hdl->vad);
        hdl->vad = NULL;
    }
    if (hdl->model) {
        free(hdl->model);
        hdl->model = NULL;
    }
    if (hdl->heap_buffer) {
        free(hdl->heap_buffer);
        hdl->heap_buffer = NULL;
    }
    if (hdl->share_buffer) {
        free(hdl->share_buffer);
        hdl->share_buffer = NULL;
    }
    log_info("VAD deinit success\n");
#if UART_DEBUG_ENABLE
#if TCFG_AUDIO_VAD_TYPE & NN_VAD
    aec_uart_close();
#endif
#endif
}

/*节点输出回调处理，可处理数据或post信号量*/
static void vad_handle_frame(struct stream_iport *iport, struct stream_note *note)
{

    struct vad_node_hdl *hdl = (struct vad_node_hdl *)iport->node->private_data;
    vad_dev_process(&hdl->dev, iport,  note);
}

/*节点预处理-在ioctl之前*/
static int vad_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct vad_node_hdl *hdl = (struct vad_node_hdl *)node->private_data;
    memset(hdl, 0, sizeof(*hdl));
    return 0;
}

/*打开改节点输入接口*/
static void vad_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = vad_handle_frame;				//注册输出回调
}

/*节点参数协商*/
static int vad_ioc_negotiate(struct stream_iport *iport)
{
    int ret = NEGO_STA_ACCPTED;
    struct stream_oport *oport = iport->node->oport;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    struct vad_node_hdl *hdl = (struct vad_node_hdl *)iport->node->private_data;

    if (oport->fmt.channel_mode == 0xff) {
        return 0;
    }

    hdl->dev.out_ch_num = AUDIO_CH_NUM(oport->fmt.channel_mode);
    hdl->dev.in_ch_num = AUDIO_CH_NUM(in_fmt->channel_mode);
#if (CHANNEL_ADAPTER_TYPE == CHANNEL_ADAPTER_2TO4)
    if (hdl->dev.out_ch_num == 4) {
        if (hdl->dev.in_ch_num != 2) {
            in_fmt->channel_mode = AUDIO_CH_LR;
            ret = NEGO_STA_CONTINUE;
        }
    }
#elif (CHANNEL_ADAPTER_TYPE == CHANNEL_ADAPTER_1TO2)
    if (hdl->dev.out_ch_num == 2) {
        if (hdl->dev.in_ch_num != 1) {
            in_fmt->channel_mode = AUDIO_CH_MIX;
            ret = NEGO_STA_CONTINUE;
        }
    }
#elif (CHANNEL_ADAPTER_TYPE == CHANNEL_ADAPTER_2TO6)
    if (hdl->dev.out_ch_num == 6) {
        if (hdl->dev.in_ch_num != 2) {
            in_fmt->channel_mode = AUDIO_CH_LR;
            ret = NEGO_STA_CONTINUE;
        }
    }
#elif (CHANNEL_ADAPTER_TYPE == CHANNEL_ADAPTER_2TO8)
    if (hdl->dev.out_ch_num == 8) {
        if (hdl->dev.in_ch_num != 2) {
            in_fmt->channel_mode = AUDIO_CH_LR;
            ret = NEGO_STA_CONTINUE;
        }
    }
#endif

    log_debug(" vad_dev in_ch_num %d, out_ch_num %d\n", hdl->dev.in_ch_num, hdl->dev.out_ch_num);


    return ret;
}

/*节点start函数*/
static void vad_ioc_start(struct vad_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt;
    /*
     *获取配置文件内的参数,及名字
     * */
    jlstream_read_node_data(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, (u8 *)hdl->name);

    hdl->dev.sample_rate = fmt->sample_rate;
    hdl->dev.bit_width = hdl_node(hdl)->iport->prev->fmt.bit_wide;
    hdl->dev.qval = hdl_node(hdl)->iport->prev->fmt.Qval;
    log_info("vad_ioc_start, sr: %d, in_ch: %d, out_ch: %d, bitw: %d, %d", hdl->dev.sample_rate, hdl->dev.in_ch_num, hdl->dev.out_ch_num, hdl->dev.bit_width, hdl->dev.qval);


    hdl->dev.node_hdl = hdl;
    hdl->dev.vad_run = (void (*)(void *, s16 *, s16 *, u32))audio_vad_run;
    vad_dev_init(&hdl->dev, NN_VAD_FRAME_POINTS);
    audio_vad_init(hdl);
}


/*节点stop函数*/
static void vad_ioc_stop(struct vad_node_hdl *hdl)
{
    audio_vad_exit(hdl);
    vad_dev_close(&hdl->dev);
}

/*节点ioctl函数*/
static int vad_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct vad_node_hdl *hdl = (struct vad_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        vad_ioc_open_iport(iport);
        break;
    case NODE_IOC_OPEN_OPORT:
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_SET_SCENE:
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= vad_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        vad_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        vad_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void vad_adapter_release(struct stream_node *node)
{
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(vad_node_adapter) = {
    .name       = "vad",
    .uuid       = NODE_UUID_VAD,
    .bind       = vad_adapter_bind,
    .ioctl      = vad_adapter_ioctl,
    .release    = vad_adapter_release,
    .hdl_size   = sizeof(struct vad_node_hdl),
#if (CHANNEL_ADAPTER_TYPE != CHANNEL_ADAPTER_AUTO)
    .ability_channel_out = 0x80 | 1 | 2 | 4 | 6 | 8,
    .ability_channel_convert = 1,
#endif
};

#endif


