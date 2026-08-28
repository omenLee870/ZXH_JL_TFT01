#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_tx_node.data.bss")
#pragma data_seg(".video_tx_node.data")
#pragma const_seg(".video_tx_node.text.const")
#pragma code_seg(".video_tx_node.text")
#endif

#include "jlstream.h"
/* #include "classic/hci_lmp.h" */
#include "media/audio_base.h"
#include "app_config.h"
#if TCFG_VIDEO_TX_NODE_ENABLE

struct video_tx_hdl {
    u8 start;
    struct stream_fmt fmt;
    int (*tx_func)(u8 *buf, u32 len);
};


static void video_tx_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct video_tx_hdl *hdl = (struct video_tx_hdl *)iport->node->private_data;
    struct stream_frame *frame;

    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
        if (hdl->tx_func) {
            hdl->tx_func(frame->data, frame->len);
        }
        jlstream_free_frame(frame);
    }
}

static int video_tx_bind(struct stream_node *node, u16 uuid)
{
    struct video_tx_hdl *hdl = (struct video_tx_hdl *)node->private_data;
    hdl->fmt.coding_type = AUDIO_CODING_OPUS;
    return 0;
}

static void video_tx_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = video_tx_handle_frame;
}

static int video_tx_ioc_fmt_nego(struct video_tx_hdl *hdl, struct stream_iport *iport)
{
    struct stream_fmt *in_fmt = &iport->prev->fmt;
#if 0
    in_fmt->coding_type = hdl->fmt.coding_type;
    in_fmt->sample_rate = 16000;
    in_fmt->channel_mode = AUDIO_CH_MIX;
#else
    if (in_fmt->coding_type != AUDIO_CODING_UNKNOW) {
        hdl->fmt.coding_type = in_fmt->coding_type;
    } else {
        in_fmt->coding_type = hdl->fmt.coding_type;
    }
    hdl->fmt.sample_rate = in_fmt->sample_rate;
    hdl->fmt.frame_dms = in_fmt->frame_dms;
    hdl->fmt.channel_mode = in_fmt->channel_mode;
    hdl->fmt.bit_rate = in_fmt->bit_rate;

    /* printf("video_tx: coding_type: %x", in_fmt->coding_type); */
    /* printf("video_tx: sample_rate: %d", in_fmt->sample_rate); */
    /* printf("video_tx: frame_dms: %d", in_fmt->frame_dms); */
    /* printf("video_tx: channel_mode: %d", hdl->fmt.channel_mode); */
    /* printf("video_tx: bit_rate: %d", in_fmt->bit_rate); */

    if (in_fmt->coding_type == AUDIO_CODING_UNKNOW) {
        g_printf("video_tx coding_type nego fail\n");
        return NEGO_STA_CONTINUE;
    }
    if (in_fmt->sample_rate == 0) {
        return  NEGO_STA_CONTINUE;
    }

#endif

    return NEGO_STA_ACCPTED;
}

static int video_tx_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct video_tx_hdl *hdl = (struct video_tx_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        video_tx_open_iport(iport);
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_SET_FMT:
        struct stream_fmt *fmt = (struct stream_fmt *)arg;
        hdl->fmt.coding_type = fmt->coding_type;
        break;
    case NODE_IOC_SET_PRIV_FMT:
        hdl->tx_func = (int (*)(u8 *, u32))arg;
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= video_tx_ioc_fmt_nego(hdl, iport);
        break;
    case NODE_IOC_GET_DELAY:
        break;
        /* return lmp_private_get_ai_tx_packet_num() * 75; */
    }

    return 0;
}

static void video_tx_release(struct stream_node *node)
{

}


REGISTER_STREAM_NODE_ADAPTER(video_tx_adapter) = {
    .name       = "video_tx",
    .uuid       = NODE_UUID_VIDEO_TX,
    .bind       = video_tx_bind,
    .ioctl      = video_tx_ioctl,
    .release    = video_tx_release,
    .hdl_size   = sizeof(struct video_tx_hdl),
};

#endif


