

#include "system/includes.h"
#include "source_node.h"
#include "app_config.h"
#include "reference_time.h"
#include "video_rx_player.h"
/* #include "rcsp_translator.h" */


#if  TCFG_VIDEO_RX_NODE_ENABLE

static int video_ioc_get_fmt(struct video_rx_file_handle *hdl, struct stream_fmt *fmt)
{
    return hdl->file_ops->get_fmt(hdl->file, fmt);
}


static void video_ioc_set_file(struct video_rx_file_handle *hdl, struct stream_file_info *info)
{
    hdl->file = info->file;
    hdl->file_ops = info->ops;
}


extern u32 bt_audio_conn_clock_time(void *addr);
static enum stream_node_state video_rx_get_frame(void *file, struct stream_frame **pframe)
{
    struct video_rx_file_handle *hdl = (struct video_rx_file_handle *)file;
    struct stream_frame *frame;
    u16 fram_len = 512;

    if (hdl->rx_func) {
        return hdl->rx_func(hdl, pframe);
    }

    frame = jlstream_get_frame(hdl->node->oport, fram_len);
    if (frame == NULL) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }
    /*测试代码。具体数据接入根据实际情况加入*/
    extern int pcm_read_debug(void *data, int len);
    int rlen = pcm_read_debug(frame->data, fram_len);
    frame->len = rlen;
    *pframe = frame;
    return NODE_STA_RUN;

}

static void *video_rx_file_init(void *priv, struct stream_node *node)
{
    struct video_rx_file_handle *hdl = (struct video_rx_file_handle *)zalloc(sizeof(struct video_rx_file_handle));

    hdl->node = node;
    node->type |= NODE_TYPE_FLOW_CTRL;//NODE_TYPE_IRQ;
    return hdl;
}




static int video_rx_file_set_bt_addr(struct video_rx_file_handle *hdl, void *bt_addr)
{
    hdl->bt_addr = (void *)bt_addr;

    return 0;
}
static void video_rx_tick_handler(void *priv, u8 source)
{
    struct video_rx_file_handle *hdl = (struct video_rx_file_handle *)priv;

    if (hdl->start && (hdl->node->state & NODE_STA_SOURCE_NO_DATA)) {
        jlstream_wakeup_thread(NULL, hdl->node, NULL);
    }
}

static int video_rx_file_get_fmt(struct video_rx_file_handle *hdl, struct stream_fmt *fmt)
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
    printf("%s  0x%x, %d, %d, %d, %d\n", __FUNCTION__, fmt->coding_type, fmt->sample_rate, fmt->channel_mode, fmt->frame_dms, fmt->bit_rate);
#endif
    return 0;
}

static int video_rx_file_start(struct video_rx_file_handle *hdl)
{
    if (hdl->start) {
        return 0;
    }

    hdl->start = 1;
    return 0;
}

static int video_rx_file_stop(struct video_rx_file_handle *hdl)
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

static int video_rx_file_ioctl(void *file, int cmd, int arg)
{
    int err = 0;
    struct video_rx_file_handle *hdl = (struct video_rx_file_handle *)file;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        video_rx_file_set_bt_addr(hdl, (void *)arg);
        break;
    case NODE_IOC_SET_FMT:
        memcpy(&hdl->param, (struct video_rx_player_param *)arg, sizeof(struct video_rx_player_param));
        break;
    case NODE_IOC_GET_FMT:
        video_rx_file_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_FILE:
        video_ioc_set_file(hdl, (struct stream_file_info *)arg);
        break;
    case NODE_IOC_START:
        video_rx_file_start(hdl);
        break;
    case NODE_IOC_SET_PARAM:
        hdl->source = (u8)arg;
        break;
    case NODE_IOC_SET_PRIV_FMT:
        hdl->rx_func = (enum stream_node_state(*)(void *, struct stream_frame **))arg;
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        video_rx_file_stop(hdl);
        break;
    }

    return 0;
}

static void video_rx_file_release(void *file)
{
    struct video_rx_file_handle *hdl = (struct video_rx_file_handle *)file;

    free(hdl);
}

REGISTER_SOURCE_NODE_PLUG(VIDEO_RX_file_plug) = {
    .uuid       = NODE_UUID_VIDEO_RX,
    .init       = video_rx_file_init,
    .get_frame  = video_rx_get_frame,
    .ioctl      = video_rx_file_ioctl,
    .release    = video_rx_file_release,
};


#endif /*TCFG_VIDEO_RX_NODE_ENABLE*/
