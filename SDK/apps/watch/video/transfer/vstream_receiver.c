#include "app_config.h"
#include "os/os_api.h"
#include "task.h"
#include "lbuf.h"
#include "vstream_common.h"
#include "crc.h"
#include "kid_wt_app.h"


#if 1
#define LOG_TAG_CONST      	VSTREAM_RECEIVER
//#define LOG_TAG             "[KWT_VSTREAM_RECV]"
#define LOG_ERROR_ENABLE
//#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#else
#define log_info(...)
#define log_debug(...)
#define log_error(...)
#endif
#include "kid_mem_dbg.h"

#if TCFG_VSTREAM_TRANSFER

#define STREAM_RECV_TASK_NAME       "vstream_recv_task"

#define offsetof(type, memb) \
    ((unsigned long)(&((type *)0)->memb))

typedef enum  {
    VIDEO_TYPE_PACKET = 10,
    AUDIO_TYPE_PACKET,
    CMD_TYPE_PACKET,
    FILE_TYPE_PACKET,
    CONTINUE_PARSE_TYPE_PACKET,
    UNKNOW_TYPE_PACKET,
} PACKET_TYPE;

struct lbuf_data_head {
    int len;
    u8 data[0];
};

struct parse_info {
    u8 *data_buf;        //解析出来的数据缓存
    int data_len;
    PACKET_TYPE packet_type;     //解析出的数据类型(视频/音频)
    u32 old_frame_seq;  //媒体包序列
    u32 total_payload_len;
};
struct lbuf_manager {
    OS_SEM sem;
    struct lbuff_head *hd;
    u8 received_frame;
    u8 *ptr;
};
struct stream_dev {
    struct list_head entry;
    struct parse_info parse_info;
    u32 ip_addr;
    struct lbuf_manager video;
    struct lbuf_manager audio;
    struct lbuf_manager cmd;
    struct lbuf_manager file;

};
static void stream_recv_task(void *priv);
static int parse_data_enqueue(struct vstream_recv_hdl *recv_hdl, u8 *buf, int len, u32 ip_addr);
static int parse_recv_data(u8 *recv_buf, int recv_len, struct parse_info *parse_info);
static struct stream_dev *find_stream_dev_by_ip(struct vstream_recv_hdl *recv_hdl, u32 ip_addr);
static struct stream_dev *stream_dev_create(u32 ip_addr);
static int stream_dev_packet_enqueue(struct stream_dev *dev, u8 *buf, int len, PACKET_TYPE type);
static int stream_dev_queue_deinit(struct stream_dev *dev, int type);
static void stream_dev_destroy(struct stream_dev *dev);
static int vstream_dev_queue_push(struct stream_dev *dev, int type, u8 *buf, int len);
static int  vstream_dev_queue_pop(struct stream_dev *dev, int type, u8 **buf, int *len, int timeout_ms);
static void vstream_recv_cmd_deal_task_register(u8 *task);

static struct vstream_recv_hdl *g_recv_hdl = NULL;
static FRAME_TYPE packe_type_to_frame_type(PACKET_TYPE packet_type)
{
    int frame = packet_type - VIDEO_TYPE_PACKET + VIDEO_FRAME;
    log_debug("%s %d to %d \n", __func__, packet_type, frame);
    return frame;
}
static struct lbuf_manager *stream_dev_get_hd_by_type(struct stream_dev *dev, int type)
{
    struct lbuf_manager *lm = NULL;
    switch (type) {
    case VIDEO_FRAME:
        lm = &dev->video;
        break;
    case AUDIO_FRAME:
        lm = &dev->audio;
        break;
    case CMD_FRAME:
        lm = &dev->cmd;
        break;
    case FILE_FRAME:
        lm = &dev->file;
        break;
    default:
        break;
    }
    if (!lm) {
        log_error("%s tydpe:%d \n", __func__, type);
    }
    return lm;
}
static int stream_dev_type_get_size(int type)
{
    int size = 0;
    switch (type) {
    case VIDEO_FRAME:
        size = STREAM_PACKET_VIDEO_QUEUE_SIZE;
        break;
    case AUDIO_FRAME:
        size = STREAM_PACKET_AUDIO_QUEUE_SIZE;
        break;
    case CMD_FRAME:
        size = STREAM_PACKET_CMD_QUEUE_SIZE;
        break;
    case FILE_FRAME:
        size = STREAM_PACKET_FILE_QUEUE_SIZE;
        break;
    default:
        break;
    }
    if (!size) {
        log_error("%s tydpe:%d \n", __func__, type);
    }
    return size;
}
static int stream_dev_queue_is_init(struct lbuf_manager *lm)
{
    if (lm && lm->ptr) {
        return true;
    }
    return false;
}

static int stream_dev_queue_init(struct stream_dev *dev, int type, int size)
{
    struct lbuf_manager *lm = stream_dev_get_hd_by_type(dev, type);
    lm->ptr = malloc(size);   /* malloc redirects to PSRAM (H264 frames are large) */
    if (lm->ptr && type == VIDEO_FRAME) {
        memset(lm->ptr, 0, size);   /* malloc is not zeroed */
    }
    if (!lm->ptr) {
        log_error(" stream dev lbuf ptr alloc err \n");
        KWT_MEM_STATS("recv.queue_init malloc fail");
        return -1;
    }
    os_sem_create(&lm->sem, 0);
    lm->received_frame = 1;
    lm->hd = lbuf_init(lm->ptr, size, 4, sizeof(struct lbuf_data_head));
    if (!lm->hd) {
        log_error(" stream dev lbuf handle init err \n");
        free(lm->ptr);
        lm->ptr = NULL;
        return -1;
    }
    log_info("%s type:%d succ\n", __func__, type);
    if (type == VIDEO_FRAME) {
        KWT_MEM_STATS("recv.queue_init video done");
    }
    return 0;
}

static int stream_dev_queue_deinit(struct stream_dev *dev, int type)
{
    struct lbuf_manager *lm = stream_dev_get_hd_by_type(dev, type);
    if (!stream_dev_queue_is_init(lm)) {
        log_error("stream dev lbuf handle not init\n");
        return -1;
    }
    if (lm->ptr) {
        free(lm->ptr);
        lm->ptr = NULL;
        lm->hd = NULL;            /* head 由 lbuf_init 落在被释放的 buffer 内, 一并置空避免悬垂 */
        lm->received_frame = 0;   /* 复位"已收帧"标志, 供 vstream_dev_queue_pop 判断 */
    }
    log_info("%s type:%d succ\n", __func__, type);
    return 0;
}

static int vstream_recv_create(struct vstream_recv_hdl **hdl)
{
    struct vstream_recv_hdl *recv_hdl;
    recv_hdl = zalloc(sizeof(struct vstream_recv_hdl));
    if (!recv_hdl) {
        log_error(" stream recv hdl alloc err \n");
        KWT_MEM_STATS("recv.hdl zalloc fail");
        return -1;
    }

    INIT_LIST_HEAD(&recv_hdl->dev_list);

    int recv_buf_len = (STREAM_RECV_BUF_SIZE  + 48) * STREAM_RECV_BUF_NUM;
    recv_hdl->recv_lbuf_ptr = zalloc(recv_buf_len);
    recv_hdl->recv_lbuf_handle = lbuf_init(recv_hdl->recv_lbuf_ptr, recv_buf_len, 4, sizeof(struct lbuf_data_head));
    *hdl = recv_hdl;
    return 0;
}

int vstream_receiver_open(void)
{
    int ret = 0;

    ret = vstream_recv_create(&g_recv_hdl);
    if (ret) {
        log_error(" video stream recv hdl create err :%d \n", ret);
        return ret;
    }
    vstream_recv_cmd_deal_task_register((u8 *)"ui");
    g_recv_hdl->stream_recv_task_exit = 0;
    ret = task_create(stream_recv_task, g_recv_hdl, STREAM_RECV_TASK_NAME);
    if (ret) {
        log_error(" video stream recv task create err :%d \n", ret);
        return -1;
    }
    KWT_MEM_STATS("receiver.open done");

    return 0;
}

int vstream_recv_destroy(struct vstream_recv_hdl **hdl)
{
    struct vstream_recv_hdl *recv_hdl = *hdl;
    struct stream_dev *dev, *n;
    if (!recv_hdl) {
        return -1;
    }

    if (recv_hdl->recv_lbuf_ptr) {
        free(recv_hdl->recv_lbuf_ptr);
        recv_hdl->recv_lbuf_ptr = NULL;
    }

    list_for_each_entry_safe(dev, n, &recv_hdl->dev_list, entry) {
        stream_dev_destroy(dev);
    }

    free(recv_hdl);
    *hdl = NULL;
    KWT_MEM_STATS("recv.destroy done");

    return 0;
}
int vstream_receiver_close(void)
{
    if (!g_recv_hdl) {
        log_error("%s already closed\n", __func__);
        return 0;
    }
    while (OS_TASK_NOT_EXIST != os_task_del_req(STREAM_RECV_TASK_NAME)) {
        os_taskq_post(STREAM_RECV_TASK_NAME, 1, 0);
        os_time_dly(1);
    }
    g_recv_hdl->stream_recv_task_exit = 1;

    vstream_recv_destroy(&g_recv_hdl);

    return 0;
}

int vstream_recv_acquire_frame(u32 ip_addr, u8 **frame_buf, int *frame_len, FRAME_TYPE frame_type)
{
    if (!g_recv_hdl || !frame_buf || !frame_len) {
        return -1;
    }
    struct stream_dev *dev;

    dev = find_stream_dev_by_ip(g_recv_hdl, ip_addr);
    if (!dev) {
        log_error(" stream dev not found for ip:%d \n", ip_addr);
        return -1;
    }
    struct lbuf_manager *lm = stream_dev_get_hd_by_type(dev, frame_type);
    if (!lm) {
        log_error("%s frame_type:%d err\n", __func__, frame_type);
        return -1;
    }
    struct lbuf_data_head *head = lbuf_pop(lm->hd, BIT(0));
    if (!head) {
        /* log_error(" stream frame lbuf get null :%d \n", frame_type); */
        return -1;
    }
    *frame_buf = head->data;
    *frame_len = head->len;
    log_debug("%s type:%d succ\n", __func__, frame_type);
    if (frame_type == VIDEO_FRAME) {
#if VSTREAM_DATA_CRC_CHECK_EN
        int crc16 = CRC16(head->data, head->len);
        log_debug("%s head:0x%x crc16:0x%x", __func__, (u32)head, crc16);
#endif
    }
    return 0;
}

int vstream_recv_acquire_frame_block(u32 ip_addr, u8 **frame_buf, int *frame_len, FRAME_TYPE frame_type, int timeout_ms)
{
    int ret;
    struct stream_dev *dev;
    if (!g_recv_hdl || !frame_buf || !frame_len) {
        return -1;
    }

    *frame_buf = NULL;
    *frame_len = 0;

    dev = find_stream_dev_by_ip(g_recv_hdl, ip_addr);
    if (!dev) {
        log_error(" stream dev not found for ip:%d \n", ip_addr);
        return -1;
    }

    ret = vstream_dev_queue_pop(dev, frame_type, frame_buf, frame_len, timeout_ms);

    log_debug("%s type:%d succ\n", __func__, frame_type);
    return ret;
}

void video_stream_recv_release_frame(u8 *frame_buf)
{
    if (!frame_buf) {
        return;
    }

    //偏移得到lbuf头
    struct lbuf_data_head *head = (struct lbuf_data_head *)(frame_buf - offsetof(struct lbuf_data_head, data));
    lbuf_free(head);
}


static void stream_recv_task(void *priv)
{
    struct vstream_recv_hdl *recv_hdl = (struct vstream_recv_hdl *)priv;
    int ret;
    int msg[32];

    while (!recv_hdl->stream_recv_task_exit) {
        /* log_info("%s %d", __func__, __LINE__); */
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        /* log_info("%s %d", __func__, __LINE__); */
        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            log_info("%s %d del!", __func__, __LINE__);
            os_task_del_res(OS_TASK_SELF);
        }

        if (ret != OS_TASKQ) {
            continue;
        }
        /* log_info("%s msg[0]:%d", __func__, msg[0]); */
        switch (msg[0]) {
        case VSTREAM_MSG_RECV:
            /* log_debug("%s %d",__func__,__LINE__);  */
            struct lbuf_data_head *head = lbuf_pop(recv_hdl->recv_lbuf_handle, BIT(0));
            if (!head) {
                os_time_dly(1);
                continue;
            }
            parse_data_enqueue(recv_hdl, head->data, head->len, 0);
            lbuf_free(head);
            break;
        default:
            break;
        }


        // pend -> 接收完成 post
    }
}

void vstream_spp_rx_handler(u8 *buf, u16 len)
{
    /* SPP 收包统计: 1s 打印一次, 用于区分"对端没发/传输断"与"本地解析卡住" */
    extern u32 timer_get_ms(void);
    static u32 rx_pkt_cnt = 0;
    static u32 rx_byte_cnt = 0;
    static u32 rx_dump_ms = 0;
    u32 now = timer_get_ms();

    if (!g_recv_hdl) {
        log_error("%s g_recv_hdl is NULL\n", __func__);
        return;
    }
    ASSERT(len <= STREAM_RECV_BUF_SIZE);

    rx_pkt_cnt++;
    rx_byte_cnt += len;
    if (now - rx_dump_ms >= 1000) {
        log_info("%s rx: %u pkts/s, %u B/s\n", __func__, rx_pkt_cnt, rx_byte_cnt);
        rx_pkt_cnt = 0;
        rx_byte_cnt = 0;
        rx_dump_ms = now;
    }

    struct lbuf_data_head *head = NULL;
    int retry = 0;
    //do {
    head = lbuf_alloc(g_recv_hdl->recv_lbuf_handle, STREAM_RECV_BUF_SIZE);
    //    if (head) {
    //        break;
    //    }
    //    os_time_dly(1);
    //} while (retry-- > 0);

    if (head) {
        head->len = len;
        memcpy(head->data, buf, len);
        lbuf_push(head, BIT(0));
        log_debug("%s succ(len:%d)\n", __func__, len);
    } else {
        int max = lbuf_free_space(g_recv_hdl->recv_lbuf_handle);
        int total = lbuf_remain_space(g_recv_hdl->recv_lbuf_handle);
        log_error("%s get buf error(len:%d max:%d total:%d) \n", __func__, len, max, total);
    }
    os_taskq_post_type(STREAM_RECV_TASK_NAME, VSTREAM_MSG_RECV, 0, NULL);
}

void vstream_recv_deal_cb(int type)
{
    int ret;
    u8 *data;
    int data_len;
    if (type == CMD_FRAME) {
        ret = vstream_recv_acquire_frame(0, &data, &data_len, CMD_FRAME);
        if (ret < 0 || !data) {
            log_error("%s acquire cmd frame failed:%d\n", __func__, ret);
            return;
        }
        extern int vstream_cmd_recv(u8 * data, int len);
        vstream_cmd_recv(data, data_len);
        video_stream_recv_release_frame(data);
    } else if (type == FILE_FRAME) {
        ret = vstream_recv_acquire_frame(0, &data, &data_len,  FILE_FRAME);
        if (ret < 0 || !data) {
            log_error("%s acquire file frame failed:%d\n", __func__, ret);
            return;
        }
        int vstream_file_save(u8 * data, int len);
        vstream_file_save(data, data_len);
        video_stream_recv_release_frame(data);
    } else {
        log_error("%s type:%d \n", __func__, type);
    }
}
void vstream_recv_cmd_deal_task_register(u8 *task)
{
    if (g_recv_hdl) {
        g_recv_hdl->recv_cmd_deal_task = task;
    }
}
void vstream_recv_deal(int type)
{
    u8 *deal_task = (g_recv_hdl->recv_cmd_deal_task) ? g_recv_hdl->recv_cmd_deal_task : (u8 *)"app_core";
    int msg[3] = {0};
    msg[0] = (int)vstream_recv_deal_cb;
    msg[1] = 1;
    msg[2] = type;
    int cnt = 3;
__retry:
    int ret = os_taskq_post_type((const char *)deal_task,  Q_CALLBACK, 3, msg);
    if (ret) {
        if (cpu_in_irq() || cpu_irq_disabled()) {

        } else {
            os_time_dly(1);
        }
        cnt --;
        if (cnt) {
            goto __retry;
        }
    }
    if (ret && (!cnt)) {
        log_error("%s msg post error\n", __func__);
    }
}



static int parse_data_enqueue(struct vstream_recv_hdl *recv_hdl,
                              u8 *buf, int len, u32 ip_addr)
{
    struct stream_dev *dev;
    dev = find_stream_dev_by_ip(recv_hdl, ip_addr);
    if (!dev) {
        /* log_debug("%s %d ip_addr:%x", __func__, __LINE__, ip_addr); */
        dev = stream_dev_create(ip_addr);
        if (!dev) {
            log_error("%s %d ip_addr:%x", __func__, __LINE__, ip_addr);
            log_error(" stream dev create err \n");
            return -1;
        }
        list_add_tail(&dev->entry, &recv_hdl->dev_list);
    }

    int ret = parse_recv_data(buf, len, &dev->parse_info);
    if (ret) {
        log_error(" parse recv packet err :%d \n", ret);
        return -1;
    }
    PACKET_TYPE type = dev->parse_info.packet_type;

    if (type == VIDEO_TYPE_PACKET || type == AUDIO_TYPE_PACKET || type == CMD_TYPE_PACKET || type == FILE_TYPE_PACKET) {
        ret = stream_dev_packet_enqueue(dev, dev->parse_info.data_buf, dev->parse_info.data_len, type);
        if (ret) {
            log_error("stream_dev_packet_enqueue:%d", ret);
            return -1;
        }

        log_debug("%s succ!", __func__);
    } else if (type == CONTINUE_PARSE_TYPE_PACKET) {
        //do nothing
    } else {
        log_error(" stream packet enter dev queue unknow type :%d \n", type);
        return -1;
    }
    log_debug("%s succ!", __func__);
    return 0;
}


static int stream_dev_packet_enqueue(struct stream_dev *dev, u8 *buf, int len, PACKET_TYPE type)
{
    int frame_type = packe_type_to_frame_type(type);
    if (!stream_dev_queue_is_init(stream_dev_get_hd_by_type(dev, frame_type))) {
        if (stream_dev_queue_init(dev, frame_type, stream_dev_type_get_size(frame_type)) != 0) {
            return -1;
        }
        log_info("%s type:%d queue init\n", __func__, type);
    }
    int ret = vstream_dev_queue_push(dev, frame_type, buf, len);
    if (ret) {
        log_error("%s ret:%d\n", __func__);
        return ret;
    }
    if (type == AUDIO_TYPE_PACKET) {
        void kid_audio_dec_resume(void);
        kid_audio_dec_resume(); // 收数后激活解码
    } else if (type == CMD_TYPE_PACKET || type == FILE_TYPE_PACKET) {
        vstream_recv_deal(frame_type);
    }
    return 0;
}

static struct stream_dev *stream_dev_create(u32 ip_addr)
{
    struct stream_dev *dev;
    dev = zalloc(sizeof(struct stream_dev));
    if (!dev) {
        log_error(" stream dev alloc err \n");
        return NULL;
    }

    dev->parse_info.data_buf = malloc(STREAM_PARSE_BUF_SIZE);   /* malloc redirects to PSRAM: reassembly buffer */
    if (!dev->parse_info.data_buf) {
        log_error(" stream dev parse buf alloc err \n");
        KWT_MEM_STATS("recv.parse_buf malloc fail");
        free(dev);
        return NULL;
    }
    memset(dev->parse_info.data_buf, 0, STREAM_PARSE_BUF_SIZE);   /* malloc is not zeroed */

    dev->ip_addr = ip_addr;
    INIT_LIST_HEAD(&dev->entry);
    return dev;
}

static void stream_dev_destroy(struct stream_dev *dev)
{
    if (!dev) {
        return;
    }

    list_del(&dev->entry);
    stream_dev_queue_deinit(dev, CMD_FRAME);
    stream_dev_queue_deinit(dev, VIDEO_FRAME);
    stream_dev_queue_deinit(dev, AUDIO_FRAME);
    stream_dev_queue_deinit(dev, FILE_FRAME);
    if (dev->parse_info.data_buf) {
        free(dev->parse_info.data_buf);
        dev->parse_info.data_buf = NULL;
    }

    free(dev);
}
int stream_dev_create_video()
{
    int frame_type = VIDEO_FRAME;
    struct stream_dev *dev;
    dev = find_stream_dev_by_ip(g_recv_hdl, 0);
    if (!dev) {
        return -1;
    }
    if (!stream_dev_queue_is_init(stream_dev_get_hd_by_type(dev, frame_type))) {
        if (stream_dev_queue_init(dev, frame_type, stream_dev_type_get_size(frame_type)) != 0) {
            return -1;
        }
    }
    return 0;
}
int stream_dev_create_audio()
{
    int frame_type = AUDIO_FRAME;
    struct stream_dev *dev;
    dev = find_stream_dev_by_ip(g_recv_hdl, 0);
    if (!dev) {
        return -1;
    }
    if (!stream_dev_queue_is_init(stream_dev_get_hd_by_type(dev, frame_type))) {
        if (stream_dev_queue_init(dev, frame_type, stream_dev_type_get_size(frame_type)) != 0) {
            return -1;
        }
    }
    return 0;
}
int stream_dev_destroy_video()
{
    if (!g_recv_hdl) {
        return -1;
    }
    struct stream_dev *dev = find_stream_dev_by_ip(g_recv_hdl, 0);
    if (!dev) {
        return -1;
    }
    stream_dev_queue_deinit(dev, VIDEO_FRAME);
    return 0;
}
int stream_dev_destroy_audio()
{
    if (!g_recv_hdl) {
        return -1;
    }
    struct stream_dev *dev = find_stream_dev_by_ip(g_recv_hdl, 0);
    if (!dev) {
        return -1;
    }
    stream_dev_queue_deinit(dev, AUDIO_FRAME);
    return 0;

}
int stream_dev_destroy_file()
{
    if (!g_recv_hdl) {
        return -1;
    }
    struct stream_dev *dev = find_stream_dev_by_ip(g_recv_hdl, 0);
    if (!dev) {
        return -1;
    }
    stream_dev_queue_deinit(dev, FILE_FRAME);
    return 0;
}

/* 断线时排空接收端视频/音频 lbuf 队列并复位解析状态, 避免上一会话的残帧
 * 与半帧解析状态残留在重连后被 VDEC 误解码(远端画面卡住)。
 *
 * 注意: 只排空(drain: pop+free 帧节点), 不释放 lbuf 内存(deinit)。因为
 * 生产者任务 stream_recv_task 在断线后仍存活(仅 protocol deinit 时才停止),
 * 若此处 free(lm->ptr) 会与其 push 路径形成 use-after-free / lbuf_alloc(NULL)
 * 竞态。排空只回收帧节点, lbuf 内存本身保持有效, 与既有 consumer
 * (kwt_video_recv_thread 的 lbuf_pop) 对同一 lbuf 的无锁访问属同类, 不引入
 * 新的竞态类别。 */
int stream_dev_media_clear(void)
{
    if (!g_recv_hdl) {
        return -1;
    }
    struct stream_dev *dev = find_stream_dev_by_ip(g_recv_hdl, 0);
    if (!dev) {
        return -1;
    }
    if (stream_dev_queue_is_init(&dev->video)) {
        struct lbuf_data_head *h;
        while ((h = lbuf_pop(dev->video.hd, BIT(0))) != NULL) {
            lbuf_free(h);
        }
    }
    if (stream_dev_queue_is_init(&dev->audio)) {
        struct lbuf_data_head *h;
        while ((h = lbuf_pop(dev->audio.hd, BIT(0))) != NULL) {
            lbuf_free(h);
        }
    }
    /* data_buf 指向的组包缓冲保留复用, 仅清组包/序号状态, 避免半帧残留 */
    dev->parse_info.old_frame_seq = 0;
    dev->parse_info.total_payload_len = 0;
    dev->parse_info.data_len = 0;
    return 0;
}

int vstream_recv_audio_ready(void)
{
    if (!g_recv_hdl) {
        return 0;
    }
    struct stream_dev *dev = find_stream_dev_by_ip(g_recv_hdl, 0);
    if (!dev) {
        return 0;
    }
    return stream_dev_queue_is_init(&dev->audio);
}

static int vstream_dev_queue_push(struct stream_dev *dev, int type, u8 *buf, int len)
{
    struct lbuf_data_head *head = NULL;
    struct lbuf_manager *lm =  stream_dev_get_hd_by_type(dev, type);
    int retry = 5;
    do {
        head = lbuf_alloc(lm->hd, len);
        if (!head) {
#if 0
            //decrease sem count
            ret = os_sem_pend(&lm->sem, 10);
            if (ret != OS_NO_ERR) {
                log_error("type:%d queue pend err %d\n", type, ret);
                return -1;
            }
#endif
            if ((type == VIDEO_FRAME) || (type == AUDIO_FRAME)) {
                log_debug(" stream  lbuf full :%d, drop frame \n", type);
                //drop frame
                struct lbuf_data_head *temp_head = lbuf_pop(lm->hd, BIT(0));
                if (temp_head) {
                    lbuf_free(temp_head);
                }
            } else {
                if (retry) {
                    os_time_dly(5);
                    retry--;
                } else {
                    log_error("%s head alloc err(type:%d)\n", __func__, type);
                    return -1;
                }
            }
        }
    } while (!head);

    if (!head) {
        return -1;
    }
    head->len = len;
    memcpy(head->data, buf, len);
#if VSTREAM_DATA_CRC_CHECK_EN
    int crc16 = CRC16(head->data, head->len);
    log_debug("%s head:0x%x crc16:0x%x", __func__, (u32)head, crc16);
#endif
    //printf("%s type:%d", __func__, type);
    lbuf_push(head, BIT(0));
#if 0
    os_sem_post(&lm->sem);
#endif
#if KID_WT_FPS_TEST
    if (type == VIDEO_FRAME) {
        extern int fps_win_rx_counter_update(u32 now_ms);
        fps_win_rx_counter_update(jiffies_msec());
    } else if (type == AUDIO_FRAME) {
        extern int fps_audio_win_rx_counter_update(void);
        fps_audio_win_rx_counter_update();
    }
#endif
    return 0;
}

static int  vstream_dev_queue_pop(struct stream_dev *dev, int type, u8 **buf, int *len, int timeout_ms)
{
    ASSERT(0);//暂不支持
    int ret = 0;
    int timeout_tick = timeout_ms / 10;
    struct lbuf_manager *lm =  stream_dev_get_hd_by_type(dev, type);
    if (!lm) {
        log_error("%s type:%d err\n", __func__, type);
        return -1;
    }
    if (lm->received_frame == 0) {
        log_debug(" no frame received yet !\n");
        os_time_dly(timeout_tick);          //TODO: no block ?
        return -1;
    }

    ret = os_sem_pend(&lm->sem, timeout_tick);
    if (ret != OS_NO_ERR) {
        log_debug(" queue pop timeout %d ms \n", timeout_ms);
        return -1;
    }

    struct lbuf_data_head *head = lbuf_pop(lm->hd, BIT(0));
    if (!head) {
        log_error(" stream frame lbuf get null \n");
        return -1;
    }
#if VSTREAM_DATA_CRC_CHECK_EN
    int crc16 = CRC16(head->data, head->len);
    log_debug("%s head:0x%x crc16:0x%x", __func__, (u32)head, crc16);
#endif
    *buf = head->data;
    *len = head->len;
    return 0;
}


static struct stream_dev *find_stream_dev_by_ip(struct vstream_recv_hdl *recv_hdl, u32 ip_addr)
{
    struct stream_dev *dev;
    list_for_each_entry(dev, &recv_hdl->dev_list, entry) {
        if (dev->ip_addr == ip_addr) {
            return dev;
        }
    }
    return NULL;
}

static int parse_recv_data(u8 *recv_buf, int recv_len, struct parse_info *parse_info)
{
    u32 position = 0;
    struct frm_head  *head_info;
    u32 frame_head_size = sizeof(struct frm_head);

    if (recv_len < frame_head_size) {
        log_error(" recv_recv_len err\n");
        return -1;
    }
    int block_idx = 0;
    do {
        head_info = (struct frm_head *)(recv_buf + position);
#if VSTREAM_DATA_CRC_CHECK_EN
        // vsteam_frame_head_dump(head_info, __func__, __LINE__);
#endif
        //vsteam_frame_head_dump(head_info, __func__, __LINE__);
        u8 frame_type = head_info->type;
        frame_type &= ~VSTREAM_FAST_PLAY_MAKER;
        frame_type &= ~VSTREAM_LAST_FREG_MAKER;
#if KWT_USE_H264
        if ((frame_type == VSTREAM_JPEG_TYPE_VIDEO) || \
            (frame_type == VSTREAM_H264_TYPE_VIDEO) || \
            (frame_type == VSTREAM_PCM_TYPE_AUDIO) || \
            (frame_type == VSTREAM_TYPE_CMD) || \
            (frame_type == VSTREAM_TYPE_FILE)\
           ) {
#else
        if ((frame_type == VSTREAM_JPEG_TYPE_VIDEO) || \
            (frame_type == VSTREAM_PCM_TYPE_AUDIO) || \
            (frame_type == VSTREAM_TYPE_CMD) || \
            (frame_type == VSTREAM_TYPE_FILE)\
           ) {
#endif
            if (head_info->frm_sz > STREAM_PARSE_BUF_SIZE) {
                log_error("jpeg frame size too large :%d \n", head_info->frm_sz);
                return -1;
            }
            recv_len = recv_len - (frame_head_size + head_info->payload_size);
            if (recv_len < 0) {
                log_error("remain recv_len err:%d \n", recv_len);
                return -1;
            }

            //如果当前的seq小于旧的seq,说明是旧的数据包,跳过不处理
            // if (head_info->seq < parse_info->old_frame_seq) {
            //     log_error("old frame \n");
            //     goto continue_deal;
            // }

            //如果当前seq大于旧的seq,认为是新的数据包,组包重新初始 or 当前数据偏移为0当作新的一包
            if (head_info->seq > parse_info->old_frame_seq || head_info->offset == 0) {
                parse_info->old_frame_seq = head_info->seq;
                parse_info->total_payload_len = 0;
            }

            /* head_info->offset 是线上原始 u32, 未校验过; 越界(或 32 位回绕)会让
             * data_buf + offset 覆盖组包缓冲之外的内存。用减法形式比较, 避免
             * offset + payload_size 自身溢出后绕回小值绕过检查。 */
            if ((u32)head_info->offset >= STREAM_PARSE_BUF_SIZE ||
                head_info->payload_size > STREAM_PARSE_BUF_SIZE - (u32)head_info->offset) {
                log_error("reassembly offset out of bounds: offset=%u payload=%u\n",
                          head_info->offset, head_info->payload_size);
                return -1;
            }

            memcpy(parse_info->data_buf + head_info->offset,
                   (recv_buf + position) + frame_head_size,
                   head_info->payload_size);
#if VSTREAM_DATA_CRC_CHECK_EN
            u16 block_crc = CRC16((recv_buf + position) + frame_head_size,
                                  head_info->payload_size);
            if (block_crc != head_info->block_crc) {
                log_error("%s:%d crc:0x%x != 0x%x", __func__, __LINE__, block_crc, head_info->block_crc);
                return -1;
            }
#endif
            parse_info->total_payload_len += head_info->payload_size;
            block_idx  ++;
            //接收到完整一帧
            if (parse_info->total_payload_len == head_info->frm_sz) {
                log_info("%s frame complete: seq=%u frm_sz=%u type=%d\n",
                         __func__, head_info->seq, head_info->frm_sz, frame_type);
                parse_info->data_len = parse_info->total_payload_len;
                parse_info->total_payload_len = 0;
#if VSTREAM_DATA_CRC_CHECK_EN
                int crc16 = CRC16(parse_info->data_buf, parse_info->data_len);
                if (crc16 != head_info->crc16) {
                    log_error("%s:%d crc:0x%x != 0x%x", __func__, __LINE__, crc16, head_info->crc16);
                    vsteam_frame_head_dump(head_info, __func__, __LINE__);
                    return -1;
                }
                // ASSERT(crc16 == head_info->crc16, "%x :%x", crc16, head_info->crc16);
#endif
                if (frame_type == VSTREAM_JPEG_TYPE_VIDEO
#if KWT_USE_H264
                    || frame_type == VSTREAM_H264_TYPE_VIDEO
#endif
                   ) {
                    parse_info->packet_type =  VIDEO_TYPE_PACKET;
                } else if (frame_type == VSTREAM_PCM_TYPE_AUDIO) {
                    parse_info->packet_type =  AUDIO_TYPE_PACKET;
                } else if (frame_type == VSTREAM_TYPE_CMD) {
                    parse_info->packet_type =  CMD_TYPE_PACKET;
                } else if (frame_type == VSTREAM_TYPE_FILE) {
                    parse_info->packet_type =  FILE_TYPE_PACKET;
                }
                return 0;
            }
        } else {
            log_error("recv type err:%d \n", head_info->type);
            parse_info->packet_type = UNKNOW_TYPE_PACKET;
            return 0;
        }

        position += (frame_head_size + head_info->payload_size);
    }
    while (recv_len > 0);

    parse_info->packet_type = CONTINUE_PARSE_TYPE_PACKET;
    return 0;
}

#endif /*#if TCFG_VSTREAM_TRANSFER*/

