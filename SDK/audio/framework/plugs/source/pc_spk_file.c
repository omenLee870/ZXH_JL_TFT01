#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc_spk_file.data.bss")
#pragma data_seg(".pc_spk_file.data")
#pragma const_seg(".pc_spk_file.text.const")
#pragma code_seg(".pc_spk_file.text")
#endif
#include "source_node.h"
#include "media/audio_splicing.h"
#include "audio_config.h"
#include "jlstream.h"
#include "pc_spk_file.h"
#include "app_config.h"
#include "effects/effects_adj.h"
#include "gpio_config.h"
#include "sync/audio_clk_sync.h"
#include "clock_manager/clock_manager.h"
#include "spinlock.h"
#include "circular_buf.h"
#include "pc_spk_player.h"
#include "uac_stream.h"
#include "jlstream.h"

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_JL_BIS_TX_EN | LE_AUDIO_JL_BIS_RX_EN))
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
#include "app_le_auracast.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[pcspk]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if  TCFG_USB_SLAVE_AUDIO_SPK_ENABLE

/* #define PC_SPK_CACHE_BUF_LEN   (1024 * 2) */
//足够缓存20ms的数据
#define PC_SPK_CACHE_BUF_LEN   ((SPK_AUDIO_RATE/1000) * 4 * 20)

/* PC SPK 在线检测 */
#define PC_SPK_ONLINE_DET_EN   1
#define PC_SPK_ONLINE_DET_TIME 3		//50->20

struct pc_spk_file_hdl {
    void *source_node;
    struct stream_node *node;
    struct stream_snode *snode;
    cbuffer_t spk_cache_cbuffer;
#if PC_SPK_ONLINE_DET_EN
    u32 irq_cnt;		//进中断++，用来给定时器判断是否中断没有起
    u16 det_timer_id;
#endif
    int sr;
    u8 *cache_buf;
    u8 start;
    u8 data_run;
    u8 lost_cnt;
    u8 rev_cnt; //连续收到数据计数
    u8 stream_status;
    u16 timer;
    u32 timestamp;
    u32 frame_len;
};
static struct pc_spk_file_hdl *pc_spk = NULL;

struct pc_spk_fmt_t {
    u8 init;
    u8 channel;
    u8 bit;
    u32 sample_rate;
    u32 iso_data_len;
};

struct pc_spk_isr_state {
    u8 pc_spk_isr_close;
};

struct pc_spk_isr_state pc_spk_isr = {
    .pc_spk_isr_close = 0,
};

struct pc_spk_fmt_t pc_spk_fmt = {
    .init = 0,
    .channel = SPK_CHANNEL,
    .bit = SPK_AUDIO_RES,
    .sample_rate = SPK_AUDIO_RATE,
    .iso_data_len = SPK_CHANNEL * SPK_AUDIO_RATE * (SPK_AUDIO_RES / 8) / 1000,
};
#if defined(TCFG_VIRTUAL_SURROUND_EFF_MODULE_NODE_ENABLE) && TCFG_VIRTUAL_SURROUND_EFF_MODULE_NODE_ENABLE
#define SPK_PUSH_FRAME_NUM 8 //SPK一次push的帧数，单位：uac rx中断间隔
#else
#define SPK_PUSH_FRAME_NUM 5 //SPK一次push的帧数，单位：uac rx中断间隔
#endif

#define SPK_PUSH_FRAME_WAY 0 // 0代表存够SPK_PUSH_FRAME_NUM包后全部推出，1代表存够SPK_PUSH_FRAME_NUM包作为缓存，没有数据进来时推缓存数据

static DEFINE_SPINLOCK(pc_spk_lock);

static void pc_spk_stream_pp(void *priv)
{
    struct pc_spk_file_hdl *hdl = (struct pc_spk_file_hdl *)priv;
    if (!hdl->timer) {
        return;
    }

    if (hdl->stream_status & PC_SPK_STREAM_PLAY) {
        hdl->stream_status = PC_SPK_STREAM_PAUSE;
        log_debug(">>>>>>>>>>>>>> PCSPK STREAM SUSPEND <<<<<<<<<<<<");
    } else {
        hdl->stream_status = PC_SPK_STREAM_PLAY;
        log_debug(">>>>>>>>>>>>>> PCSPK STREAM RESUME <<<<<<<<<<<<");
    }
    pc_spk_player_pp();

    hdl->timer = 0;
}

static u16 fill_timer = 0;
static void fill_packet(void *p)
{
    pc_spk_data_isr_cb(NULL, 0);
}
void pc_spk_fill_packet_init()
{
    spin_lock(&pc_spk_lock);
    if (!fill_timer) {
        fill_packet(NULL);
        fill_timer = sys_hi_timer_add(NULL, fill_packet, 1);
    }
    spin_unlock(&pc_spk_lock);
}

void pc_spk_fill_packet_uninit()
{
    spin_lock(&pc_spk_lock);
    if (fill_timer) {
        sys_hi_timer_del(fill_timer);
        fill_timer = 0;
    }
    spin_unlock(&pc_spk_lock);
}
void pc_spk_data_isr_cb(void *buf, u32 len)
{
    struct pc_spk_file_hdl *hdl = pc_spk;
    struct stream_frame *frame = NULL;

    spin_lock(&pc_spk_lock);
    int wlen = 0;

    if (pc_spk_isr.pc_spk_isr_close) {
        goto __exit;
    }

    if (!hdl) {
        goto __exit;
    }

    if (len) { //数据流激活流程判断及处理
        if (hdl->timer && (hdl->stream_status & PC_SPK_STREAM_WAIT_PAUSE)) {
            sys_timeout_del(hdl->timer);
            hdl->timer = 0;
        } else {
            if (hdl->stream_status & PC_SPK_STREAM_PAUSE) {
                if (hdl->stream_status & PC_SPK_STREAM_WAIT_RESUNE) {
                    goto __exit;
                }
                hdl->rev_cnt++;
                if (!hdl->timer && (hdl->rev_cnt >= 5)) { //连续收到5包正常数据认为可以恢复播放,简单消抖
                    hdl->timer = sys_timeout_add(hdl, pc_spk_stream_pp, 2);
                    hdl->stream_status |= PC_SPK_STREAM_WAIT_RESUNE;
                    hdl->rev_cnt = 0;
                }
                goto __exit;
            }
        }
    } else {
        hdl->rev_cnt = 0;
        if (hdl->stream_status & PC_SPK_STREAM_WAIT_RESUNE) {
            if (hdl->timer) {
                sys_timeout_del(hdl->timer);
                hdl->timer = 0;
            }
        }
    }


    if (!hdl->start) { //当前使用usb SOF中断推数，会出现len = 0情况，下面处理
        goto __exit;
    }

    if (!hdl->frame_len) {
        if (len) {
            hdl->frame_len = len;
        } else {
            hdl->frame_len = pc_spk_fmt.iso_data_len;// 容错处理，如果前几帧就丢包了，会导致frame_len没有赋值
        }
    }

    if (!hdl->cache_buf) {
        int cache_buf_len = hdl->frame_len * SPK_PUSH_FRAME_NUM * 4; //4块输出buf
        //申请cbuffer
        hdl->cache_buf = zalloc(cache_buf_len);
        if (hdl->cache_buf) {
            cbuf_init(&hdl->spk_cache_cbuffer, hdl->cache_buf, cache_buf_len);
        }
    }

#if SPK_PUSH_FRAME_WAY
    hdl->timestamp = audio_jiffies_usec();
#else
    if (cbuf_get_data_len(&hdl->spk_cache_cbuffer) == 0) {
        hdl->timestamp = audio_jiffies_usec();
    }
#endif

#if PC_SPK_ONLINE_DET_EN
    hdl->irq_cnt++;
#endif
    /* if (hdl->stream_status & PC_SPK_STREAM_PAUSE) { //数据流已经挂起,不写数据，直接返回; */
    /* goto __exit; */
    /* } */

    //1ms 起一次中断，一次长度192, 中断太快,需缓存
    if (len) {
        hdl->lost_cnt = 0;
        wlen = cbuf_write(&hdl->spk_cache_cbuffer, buf, len);
        if (wlen != len) {
            /*putchar('W');*/
        }
    } else {
        len = hdl->frame_len;
        if (hdl->lost_cnt > 0) {
            s16 *tbuf = zalloc(len);
            if (tbuf) {
                cbuf_write(&hdl->spk_cache_cbuffer, tbuf, len);//丢中断没数据填静音数据,避免音频断流
                free(tbuf);
            }
            putchar('L');
        }
        hdl->lost_cnt++;
        //设置超过3帧的检测,可能导致自动切歌时，挂起流程被挡执行的不及时，
        if (hdl->lost_cnt >= 3) { //连续丢数据超过3帧，认为有可能断链了，设置2ms的timer准备挂起数据流,
            if (!hdl->timer && (hdl->stream_status & PC_SPK_STREAM_PLAY)) {
                hdl->timer = sys_timeout_add(hdl, pc_spk_stream_pp, 2); //中断中添加的sys_timer 会在app_core任务执行
                hdl->stream_status |= PC_SPK_STREAM_WAIT_PAUSE;
            }
        }
    }

    u32 cache_len = cbuf_get_data_len(&hdl->spk_cache_cbuffer);
#if SPK_PUSH_FRAME_WAY

    if (cache_len < hdl->frame_len) {//数据长度检查
        goto __exit;
    }

    frame = source_plug_get_output_frame(hdl->source_node, hdl->frame_len);
    if (!frame) {
        goto __exit;
    }
    frame->len    = hdl->frame_len;
#else
    if (cache_len < hdl->frame_len * SPK_PUSH_FRAME_NUM) {
        goto __exit;
    }
    frame = source_plug_get_output_frame(hdl->source_node, cache_len);
    if (!frame) {
        goto __exit;
    }
    frame->len    = cache_len;

#endif
#if 1
    frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
    frame->timestamp    = hdl->timestamp * TIMESTAMP_US_DENOMINATOR;
#else
    frame->flags  = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
    frame->timestamp = hdl->timestamp;
#endif
    cbuf_read(&hdl->spk_cache_cbuffer, frame->data, frame->len);
    source_plug_put_output_frame(hdl->source_node, frame);
    hdl->data_run = 1;
__exit:

    spin_unlock(&pc_spk_lock);
}

#if PC_SPK_ONLINE_DET_EN
/* 定时器检测 pcspk 在线 */
static void pcspk_det_timer_cb(void *priv)
{
    struct pc_spk_file_hdl *hdl = (struct pc_spk_file_hdl *)priv;
    if (hdl) {
        if (hdl->start) {
            if (hdl->irq_cnt) {
                hdl->irq_cnt = 0;
            } else {
                if (hdl->data_run) {
                    //已经往后面推数据突然中断没有起的情况
                    hdl->data_run = 0;
                    log_debug(">>>>>>> PCSPK LOST CONNECT <<<<<<<");
                }
            }
        }
    }
}
#endif // PC_SPK_ONLINE_DET_EN

/*
 * 申请 pc_spk_file 结构体内存空间
 */
static void *pc_spk_file_init(void *source_node, struct stream_node *node)
{
    struct pc_spk_file_hdl *hdl = NULL;
    if (pc_spk != NULL) {
        hdl = pc_spk;
    } else {
        hdl = zalloc(sizeof(*hdl));
    }
    if (!hdl) {
        log_error("%s, %d, alloc memory failed!\n", __func__, __LINE__);
        return NULL;
    }
    pc_spk_isr.pc_spk_isr_close = 0;
    node->type |= NODE_TYPE_FLOW_CTRL;
    hdl->source_node = source_node;
    hdl->node = node;
    pc_spk = hdl;
    return hdl;
}

static int pc_spk_ioc_get_fmt(struct pc_spk_file_hdl *hdl, struct stream_fmt *fmt)
{
    fmt->coding_type = AUDIO_CODING_PCM;
    if (pc_spk_fmt.channel == 2) {
        fmt->channel_mode   = AUDIO_CH_LR;
    } else {
        fmt->channel_mode   = AUDIO_CH_L;
    }
    fmt->sample_rate    = pc_spk_fmt.sample_rate;
    fmt->bit_wide = (pc_spk_fmt.bit == 24) ? 1 : 0;
    //log_debug(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> fmt->bit_wide : %d\n", fmt->bit_wide);
    return 0;
}

static int pc_spk_ioc_get_fmt_ex(struct pc_spk_file_hdl *hdl, struct stream_fmt_ex *fmt)
{
    fmt->pcm_24bit_type = (pc_spk_fmt.bit == 24) ? PCM_24BIT_DATA_3BYTE : PCM_24BIT_DATA_4BYTE;
    return 1;
}

static int pc_spk_ioc_set_fmt(struct pc_spk_file_hdl *hdl, struct stream_fmt *fmt)
{
    return -EINVAL;
}

//打开pcspk 在线检测定时器
static void pcspk_open_det_timer(void)
{
#if PC_SPK_ONLINE_DET_EN
    struct pc_spk_file_hdl *hdl = pc_spk;
    //申请定时器
    if (hdl) {
        if (!hdl->det_timer_id) {
            hdl->det_timer_id = usr_timer_add(hdl, pcspk_det_timer_cb, PC_SPK_ONLINE_DET_TIME, 0);
        }
    }
#endif
}

//关闭 pcspk 在线检测定时器
static void pcspk_close_det_timer(void)
{
#if PC_SPK_ONLINE_DET_EN
    struct pc_spk_file_hdl *hdl = pc_spk;
    //申请定时器
    if (hdl) {
        if (hdl->det_timer_id) {
            usr_timer_del(hdl->det_timer_id);
            hdl->det_timer_id = 0;
        }
    }
#endif
}

#if 1
static int pc_spk_ioctl(void *_hdl, int cmd, int arg)
{
    u32 i = 0;
    int ret = 0;
    struct pc_spk_file_hdl *hdl = (struct pc_spk_file_hdl *)_hdl;
    switch (cmd) {
    case NODE_IOC_GET_FMT:
        pc_spk_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_GET_FMT_EX:
        ret = pc_spk_ioc_get_fmt_ex(hdl, (struct stream_fmt_ex *)arg);
        break;
    case NODE_IOC_SET_FMT:
        ret = pc_spk_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        ret = pc_spk_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;

    case NODE_IOC_START:
        if (hdl->start == 0) {
            pcspk_open_det_timer();
            hdl->data_run = 0;
            hdl->start = 1;
            hdl->stream_status = PC_SPK_STREAM_PLAY;
        }
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        if (hdl->start) {
            hdl->start = 0;
        }
        break;
    }
    return ret;
}
#endif


static void pc_spk_release(void *_hdl)
{
    struct pc_spk_file_hdl *hdl = (struct pc_spk_file_hdl *)_hdl;

    spin_lock(&pc_spk_lock);
    if (!hdl) {
        hdl = pc_spk;
        if (!hdl) {
            spin_unlock(&pc_spk_lock);
            return;
        }
    }
    if (hdl->timer) {
        sys_timeout_del(hdl->timer);
    }
    pc_spk_isr.pc_spk_isr_close = 1;
    pcspk_close_det_timer();
    free(hdl->cache_buf);
    hdl->cache_buf = NULL;
    free(hdl);
    hdl = NULL;
    pc_spk = NULL;
    spin_unlock(&pc_spk_lock);
}

u8 is_pc_spk_file_start(void)
{
    struct pc_spk_file_hdl *hdl = pc_spk;
    if (!hdl) {
        return 0;
    }
    return (hdl->start);
}

// 设置pc mic 的数据格式，传入0不设置
void pc_spk_set_fmt(u8 channel, u8 bit, u32 sample_rate)
{
    log_info("----------- Call %s, bit:%d, sr:%d\n", __func__, bit, sample_rate);
    pc_spk_fmt.init = 1;
    if (channel != 0) {
        pc_spk_fmt.channel = channel;
    }
    if (bit != 0) {
        pc_spk_fmt.bit = bit;
    }
    if (sample_rate != 0) {
        pc_spk_fmt.sample_rate = sample_rate;
    }
    pc_spk_fmt.iso_data_len = channel * sample_rate * (bit / 8) / 1000;
}

u32 pc_spk_get_fmt_sample_rate(void)
{
    return pc_spk_fmt.sample_rate;
}

u8 pc_spk_get_stream_status(void)
{
    if (pc_spk) {
        return pc_spk->stream_status;
    }
    return PC_SPK_STREAM_PAUSE;
}

REGISTER_SOURCE_NODE_PLUG(pc_spk_file_plug) = {
    .uuid       = NODE_UUID_PC_SPK,
    .init       = pc_spk_file_init,
    .ioctl      = pc_spk_ioctl,
    .release    = pc_spk_release,
};

#endif


