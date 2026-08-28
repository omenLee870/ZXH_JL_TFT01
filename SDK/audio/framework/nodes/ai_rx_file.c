
#include "system/includes.h"
#include "source_node.h"
#include "app_config.h"
#include "reference_time.h"
#include "ai_rx_player.h"
/* #include "rcsp_translator.h" */
#include "media_config.h"



#if  TCFG_AI_RX_NODE_ENABLE

extern const int OPUS_SRINDEX; //选择opus解码文件的帧大小，0代表一帧40字节，1代表一帧80字节，2代表一帧160字节
static int ai_fseek(void *_hdl, u32 fpos)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)_hdl;

    if (!hdl->file) {
        return -1;
    }
    return hdl->file_ops->seek(hdl->file, fpos, SEEK_SET);
}

static int ai_fread(void *_hdl, u8 *data, int size)
{
    u32 rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)_hdl;

    if (!hdl->file) {
        return 0;
    }
    return hdl->file_ops->read(hdl->file, data, size);
}
static int ai_ioc_get_fmt(struct ai_rx_file_handle *hdl, struct stream_fmt *fmt)
{
    return hdl->file_ops->get_fmt(hdl->file, fmt);
}


static void ai_ioc_set_file(struct ai_rx_file_handle *hdl, struct stream_file_info *info)
{
    hdl->file = info->file;
    hdl->file_ops = info->ops;
}


extern u32 bt_audio_conn_clock_time(void *addr);
static enum stream_node_state ai_rx_get_frame(void *file, struct stream_frame **pframe)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;
    struct stream_frame *frame;
    u16 fram_len = 0;
    /* 外部注册了 rx_func 则直接替代本函数的取帧流程 */
    if (hdl->rx_func) {
        return hdl->rx_func(hdl, pframe);
    }
    switch (OPUS_SRINDEX) {
    case  FRAME_LEN_40:
        fram_len = 40;
        break;
    case  FRAME_LEN_80:
        fram_len = 80;
        break;
    case  FRAME_LEN_160:
        fram_len = 160;
        break;
    default:
        break;
    }
    frame = jlstream_get_frame(hdl->node->oport, fram_len);
    if (frame == NULL) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }
    int rlen = fread(frame->data, fram_len, 1, hdl->file);
    frame->len = rlen;
    *pframe = frame;
    return NODE_STA_RUN;

}

static void *ai_rx_file_init(void *priv, struct stream_node *node)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)zalloc(sizeof(struct ai_rx_file_handle));

    hdl->node = node;
    node->type |= NODE_TYPE_FLOW_CTRL;//NODE_TYPE_IRQ;
    return hdl;
}




static int ai_rx_file_set_bt_addr(struct ai_rx_file_handle *hdl, void *bt_addr)
{
    hdl->bt_addr = (void *)bt_addr;

    return 0;
}
static void ai_rx_tick_handler(void *priv, u8 source)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)priv;

    if (hdl->start && (hdl->node->state & NODE_STA_SOURCE_NO_DATA)) {
        jlstream_wakeup_thread(NULL, hdl->node, NULL);
    }
}

static int ai_rx_file_get_fmt(struct ai_rx_file_handle *hdl, struct stream_fmt *fmt)
{
#if 0
    fmt->coding_type = AUDIO_CODING_OPUS;
    fmt->sample_rate = 16000;
    fmt->channel_mode = AUDIO_CH_MIX;
#else

    extern const int CONFIG_OGG_OPUS_DEC_SUPPORT;

    fmt->coding_type = hdl->param.coding_type;
    fmt->sample_rate = hdl->param.sample_rate;
    fmt->channel_mode = hdl->param.channel_mode;
    fmt->frame_dms = hdl->param.frame_dms;
    fmt->bit_rate = hdl->param.bit_rate;
    if (CONFIG_OGG_OPUS_DEC_SUPPORT && (hdl->param.coding_type == AUDIO_CODING_OPUS || hdl->param.coding_type == AUDIO_CODING_STENC_OPUS)) {
        fmt->sample_rate = 48000;
    }
    printf("%s  0x%x, %d, %d, %d, %d\n", __FUNCTION__, fmt->coding_type, fmt->sample_rate, fmt->channel_mode, fmt->frame_dms, fmt->bit_rate);
#endif
    return 0;
}


static int ai_rx_file_get_fmt_ex(struct ai_rx_file_handle *hdl, struct stream_fmt_ex *fmt)
{
    if (hdl->param.coding_type == AUDIO_CODING_OPUS || hdl->param.coding_type == AUDIO_CODING_STENC_OPUS) {
#if 0  //通过参数动态配置解码模式
        fmt->format_mode = hdl->param.format_mode;
        fmt->packet_len = hdl->param.packet_len;
#else
        fmt->format_mode = CONFIG_OGG_OPUS_DEC_FORMAT_MODE;
        fmt->packet_len = CONFIG_OGG_OPUS_DEC_SET_CBR_PACKET_LEN;
#endif
        printf("opus format_mode: %d, packet_len %d\n", fmt->format_mode, fmt->packet_len);
        return 1; //需要配置扩展参数，返回1;
    }
    return 0;

}



static int ai_rx_file_start(struct ai_rx_file_handle *hdl)
{
    if (hdl->start) {
        return 0;
    }

    hdl->start = 1;
    return 0;
}

static int ai_rx_file_stop(struct ai_rx_file_handle *hdl)
{
    if (!hdl->start) {
        return 0;
    }
    hdl->start = 0;
    if (hdl->reference) {
        audio_reference_clock_exit(hdl->reference);
    }
    return 0;
}

static int ai_rx_file_ioctl(void *file, int cmd, int arg)
{
    int err = 0;
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        ai_rx_file_set_bt_addr(hdl, (void *)arg);
        break;
    case NODE_IOC_SET_FMT:
        memcpy(&hdl->param, (struct ai_rx_player_param *)arg, sizeof(struct ai_rx_player_param));
        break;
    case NODE_IOC_GET_FMT:
        ai_rx_file_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_FILE:
        ai_ioc_set_file(hdl, (struct stream_file_info *)arg);
    case NODE_IOC_GET_FMT_EX:
        err = ai_rx_file_get_fmt_ex(hdl, (struct stream_fmt_ex *)arg);
        break;
    case NODE_IOC_START:
        ai_rx_file_start(hdl);
        break;
    case NODE_IOC_SET_PARAM:
        hdl->source = (u8)arg;
        break;
    case NODE_IOC_SET_PRIV_FMT:
        hdl->rx_func = (enum stream_node_state(*)(void *, struct stream_frame **))arg;
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        ai_rx_file_stop(hdl);
        break;
    }

    return err;
}

static void ai_rx_file_release(void *file)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;

    free(hdl);
}

REGISTER_SOURCE_NODE_PLUG(AI_RX_file_plug) = {
    .uuid       = NODE_UUID_AI_RX,
    .init       = ai_rx_file_init,
    .get_frame  = ai_rx_get_frame,
    .ioctl      = ai_rx_file_ioctl,
    .release    = ai_rx_file_release,
};


#endif /*TCFG_AI_RX_NODE_ENABLE*/
