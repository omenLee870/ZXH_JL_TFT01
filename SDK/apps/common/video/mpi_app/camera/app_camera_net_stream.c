#include "app_config.h"

#if TCFG_NETWORK_ENABLE
#include "app_camera_mpi_priv.h"
#include "rt_stream_pkg.h"
#include "sock_api/sock_api.h"
#include "lwip/sockets.h"
#include "os/os_api.h"
#include "generic/lbuf.h"
#include "system/spinlock.h"
#include "app_video_audio_rec.h"

extern uint32_t timer_get_ms(void);

#define APP_CAMERA_NET_FORMAT              MPI_PIXEL_FMT_YUV422_PACKED
#define APP_CAMERA_NET_YUV_ORDER           MPI_YUV_ORDER_YUYV
#define APP_CAMERA_NET_LINE_NUM            16u
#define APP_CAMERA_NET_DEFAULT_QUALITY     40u
#define APP_CAMERA_NET_DEFAULT_ABR_KBPS    6000u
#define APP_CAMERA_NET_DEFAULT_FPS         25u
#define APP_CAMERA_NET_STREAM_BUF_SIZE     (200u * 1024u)
#define APP_CAMERA_NET_VENC_HW_DEV         0

#define UDP_SEND_BUF_SIZE  (4 * 1460)
#define MAX_PAYLOAD         (1460 - sizeof(struct frm_head))
#define AUDIO_STALE_MS      200  /* drop audio frames older than this */


struct lbuf_data_head {
    int len;
    u8 type;
    u8 data[0];
};

struct net_stream_info {
    void *fd;
    u8 task_exit;
    char task_name[32];
    int task_pid;

    u8 *video_lbuf_ptr;
    struct lbuff_head *video_lbuf_handle;

    /* audio — independent handle with its own frame interval */
    app_video_audio_rec_t audio_rec;
    u8 audio_enable;

    u8 *udp_send_buf;
    u32 seq;

    struct sockaddr_in addr;
};


static uint32_t calc_net_buf_size(uint16_t width, mpi_pixel_format_t format)
{
    if (format == MPI_PIXEL_FMT_YUV422_PACKED) {
        return (uint32_t)width * APP_CAMERA_NET_LINE_NUM * 2u;
    } else if (format == MPI_PIXEL_FMT_YUV420SP) {
        return (uint32_t)width * APP_CAMERA_NET_LINE_NUM * 3u / 2u;
    } else if (format == MPI_PIXEL_FMT_YUV444_PACKED) {
        return (uint32_t)width * APP_CAMERA_NET_LINE_NUM * 3u;
    } else {
        return (uint32_t)width * APP_CAMERA_NET_LINE_NUM;
    }
}

/* ==================== UDP send ==================== */

static int net_write_data(struct net_stream_info *info, u8 *buffer, size_t len, u8 type)
{
    u16 payload_len = 0;
    u32 total_udp_send = 0;
    struct frm_head frame_head = {0};
    int ret;

    frame_head.offset = 0;
    frame_head.frm_sz = len;
    frame_head.type &= ~LAST_FREG_MAKER;
    frame_head.type |= type;
    frame_head.timestamp = 0;
    frame_head.seq = info->seq++;

    u32 remain_len = len;
    while (remain_len) {
        if (remain_len < MAX_PAYLOAD) {
            payload_len = remain_len;
            frame_head.type |= LAST_FREG_MAKER;
        } else {
            payload_len = MAX_PAYLOAD;
        }

        frame_head.payload_size = payload_len;
        memcpy(info->udp_send_buf + total_udp_send, &frame_head, sizeof(struct frm_head));
        total_udp_send += sizeof(struct frm_head);

        memcpy(info->udp_send_buf + total_udp_send, buffer + frame_head.offset, payload_len);
        total_udp_send += payload_len;

        if ((total_udp_send == UDP_SEND_BUF_SIZE) || (payload_len < MAX_PAYLOAD)) {
            ret = sock_sendto(info->fd, info->udp_send_buf, total_udp_send, 0,
                              (struct sockaddr *)&info->addr, sizeof(struct sockaddr_in));
            if (ret != (int)total_udp_send) {
                puts("net_stream_send error!\n");
                return -1;
            }
            total_udp_send = 0;
        }

        remain_len -= payload_len;
        frame_head.offset += payload_len;
    }

    return len;
}

static int net_rt_send_frame(struct net_stream_info *info, u8 *buffer, size_t len, u8 type)
{
    struct lbuf_data_head *lbuf_data = NULL;

    if (!info) {
        return -1;
    }

    if (type != JPEG_TYPE_VIDEO && type != H264_TYPE_VIDEO) {
        return 0;
    }

    //printf("net_stream video frame: %d bytes\n", (int)len);
    lbuf_data = lbuf_alloc(info->video_lbuf_handle, len);
    if (lbuf_data) {
        lbuf_data->len = len;
        lbuf_data->type = type;
        memcpy(lbuf_data->data, buffer, len);
        lbuf_push(lbuf_data, BIT(0));
    } else {
        putchar('d'); /* drop frame */
    }

    return len;
}

static void net_pkg_send_task(void *priv)
{
    struct net_stream_info *info = (struct net_stream_info *)priv;

    while (!info->task_exit) {
        u8 send_something = 0;
        struct lbuf_data_head *lbuf_data;

        /* audio priority — pop via standard get_frame API */
        if (info->audio_rec) {
            app_video_audio_rec_frame_t *aframe = NULL;
            if (app_video_audio_rec_get_frame(info->audio_rec, &aframe) == 0 && aframe) {
                if (aframe->len > 0) {
                    u32 now = timer_get_ms();
                    if (now - aframe->timestamp_ms < AUDIO_STALE_MS) {
                        net_write_data(info, (u8 *)aframe->data, aframe->len, PCM_TYPE_AUDIO);
                    }
                }
                app_video_audio_rec_release_frame(aframe);
                send_something = 1;
                continue;
            }
        }

        lbuf_data = lbuf_pop(info->video_lbuf_handle, BIT(0));
        if (lbuf_data) {
            if (net_write_data(info, lbuf_data->data, lbuf_data->len, lbuf_data->type) != lbuf_data->len) {
                printf("net write data err\n");
            }
            send_something = 1;
            lbuf_free(lbuf_data);
        }

        if (!send_something) {
            os_time_dly(2);
        }
    }
}

static int sock_cb_func(enum sock_api_msg_type type, void *priv)
{
    struct net_stream_info *info = (struct net_stream_info *)priv;

    if (info->task_exit) {
        return -1;
    }

    return 0;
}

static int path_analyze(struct net_stream_info *info, const char *path)
{
    char *tmp = NULL;
    char *tmp2 = NULL;
    char ip[16] = {0};
    u16 port = 0;

    tmp = strstr(path, "udp://");
    if (!tmp) {
        return -1;
    }

    tmp += strlen("udp://");

    tmp2 = strchr(tmp, ':');
    if (!tmp2) {
        return -1;
    }

    strncpy(ip, tmp, tmp2 - tmp);
    ip[sizeof(ip) - 1] = '\0';
    port = (u16)atoi(tmp2 + 1);

    printf("net_stream remote ip:%s port:%d\n", ip, port);

    info->addr.sin_family = AF_INET;
    info->addr.sin_addr.s_addr = inet_addr(ip);
    info->addr.sin_port = htons(port);
    return 0;
}

static struct net_stream_info *net_stream_open(const char *path,
        uint32_t abr_kbps,
        uint32_t fps,
        u8 audio_enable)
{
    struct net_stream_info *info;

    info = (struct net_stream_info *)calloc(1, sizeof(struct net_stream_info));
    if (info == NULL) {
        printf("%s %d -> Error in calloc\n", __func__, __LINE__);
        return NULL;
    }

    if (path_analyze(info, path)) {
        printf("%s %d -> Error in path_analyze\n", __func__, __LINE__);
        goto err;
    }

    info->fd = sock_reg(AF_INET, SOCK_DGRAM, 0, sock_cb_func, (void *)info);
    if (!info->fd) {
        printf("%s %d -> Error in sock_reg\n", __func__, __LINE__);
        goto err;
    }

    /* Allocate lbuf for ~3 frames of video */
    {
        int video_lbuf_size;

        if (abr_kbps == 0) {
            abr_kbps = APP_CAMERA_NET_DEFAULT_ABR_KBPS;
        }
        if (fps == 0) {
            fps = APP_CAMERA_NET_DEFAULT_FPS;
        }
        video_lbuf_size = (int)(abr_kbps * 1024 / fps / 8 * 3);
        if (video_lbuf_size < 64 * 1024) {
            video_lbuf_size = 64 * 1024;
        }

        info->video_lbuf_ptr = (u8 *)malloc(video_lbuf_size);
        if (!info->video_lbuf_ptr) {
            printf("%s %d -> Error in lbuf malloc\n", __func__, __LINE__);
            goto err;
        }

        info->video_lbuf_handle = lbuf_init(info->video_lbuf_ptr, video_lbuf_size,
                                            4, sizeof(struct lbuf_data_head));
    }

    /* Audio: open independent handle with its own frame interval */
    if (audio_enable) {
        app_video_audio_rec_attr_t aattr = {
            .buffer_size = 16 * 1024,       /* ~1s PCM buffer */
            .frame_interval_ms = 50,        /* 50ms per frame, independent of recording */
        };
        if (app_video_audio_rec_open(&info->audio_rec, &aattr) == 0) {
            app_video_audio_rec_start(info->audio_rec);
            info->audio_enable = 1;
            printf("net_stream: audio started\n");
        } else {
            printf("%s %d -> Error in audio open\n", __func__, __LINE__);
        }
    }

    info->udp_send_buf = (u8 *)malloc(UDP_SEND_BUF_SIZE);
    if (info->udp_send_buf == NULL) {
        printf("%s %d -> Error in malloc\n", __func__, __LINE__);
        goto err;
    }

    snprintf(info->task_name, sizeof(info->task_name), "net_pkg_send");

    if (thread_fork(info->task_name, 15, 2048, 2048,
                    &info->task_pid, net_pkg_send_task, (void *)info)) {
        printf("%s %d -> Error in thread_fork\n", __func__, __LINE__);
        goto err;
    }

    return info;

err:
    if (info->fd) {
        sock_unreg(info->fd);
    }
    if (info->video_lbuf_ptr) {
        free(info->video_lbuf_ptr);
    }
    if (info->udp_send_buf) {
        free(info->udp_send_buf);
    }
    free(info);
    return NULL;
}

static void net_stream_close(struct net_stream_info *info)
{
    if (!info) {
        return;
    }

    if (info->task_pid) {
        info->task_exit = 1;
        thread_kill(&info->task_pid, KILL_WAIT);
    }

    if (info->fd) {
        sock_unreg(info->fd);
    }

    if (info->audio_rec) {
        app_video_audio_rec_close(&info->audio_rec);
    }

    if (info->video_lbuf_ptr) {
        free(info->video_lbuf_ptr);
    }
    if (info->udp_send_buf) {
        free(info->udp_send_buf);
    }

    free(info);
}

/* ==================== VPSS + VENC pipeline for net stream ==================== */

static app_camera_ret_t app_camera_vpss_venc_net_open(app_camera_handle_t ctx,
        const app_camera_net_stream_attr_t *attr)
{
    mpi_ret_t ret;
    mpi_vpss_chn_attr_t chn_attr = {0};
    mpi_venc_chn_attr_t venc_attr = {0};
    mpi_chn_t vpss_chn_src;
    mpi_chn_t venc_chn_dst;

    if (ctx->vpss_venc_net_enabled) {
        printf("net stream is already started\n");
        return APP_CAMERA_ERR_STATE;
    }

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &ctx->vpss_chn_net);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS net channel\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    chn_attr.width = attr->width;
    chn_attr.height = attr->height;
    chn_attr.format = APP_CAMERA_NET_FORMAT;
    chn_attr.yuv_order = APP_CAMERA_NET_YUV_ORDER;
    chn_attr.mirror = 0;
    chn_attr.output_mode = MPI_VPSS_OUTPUT_ONLINE;
#if defined CONFIG_CPU_BR33
    chn_attr.hw_id = MPI_VPSS_HW_AUTO;
#else
    chn_attr.hw_id = MPI_VPSS_HW_AUTO;
#endif
    chn_attr.buf_addr = NULL;
    chn_attr.buf_size = calc_net_buf_size(chn_attr.width, chn_attr.format);
    ret = mpi_vpss_create_chn(ctx->vpss_grp, ctx->vpss_chn_net, &chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to set VPSS net channel attributes\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp, ctx->vpss_chn_net);
    if (ret != MPI_SUCCESS) {
        printf("failed to enable VPSS net channel\n");
        goto err_destroy_vpss_chn;
    }

    ctx->venc_dev_net = APP_CAMERA_NET_VENC_HW_DEV;

    ret = mpi_venc_find_free_chn(&ctx->venc_chn_net);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VENC channel for net\n");
        goto err_disable_vpss_chn;
    }

    venc_attr.hw_dev = ctx->venc_dev_net;
    venc_attr.type = MPI_VENC_TYPE_JPEG;
    venc_attr.input_mode = MPI_VENC_INPUT_ONLINE;
    venc_attr.quality = APP_CAMERA_NET_DEFAULT_QUALITY;
    venc_attr.abr_kbps = attr->abr_kbps ? attr->abr_kbps : APP_CAMERA_NET_DEFAULT_ABR_KBPS;
    venc_attr.fps = attr->fps ? attr->fps : APP_CAMERA_NET_DEFAULT_FPS;
    venc_attr.stream_buf_size = attr->stream_buf_size ? attr->stream_buf_size : APP_CAMERA_NET_STREAM_BUF_SIZE;
    venc_attr.in_width = attr->width;
    venc_attr.in_height = attr->height;
    venc_attr.in_format = chn_attr.format;
    venc_attr.in_yuv_order = chn_attr.yuv_order;

    ret = mpi_venc_create_chn(ctx->venc_chn_net, &venc_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VENC channel for net\n");
        goto err_disable_vpss_chn;
    }

    ret = mpi_venc_start_chn(ctx->venc_chn_net);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VENC channel for net\n");
        goto err_destroy_venc_chn;
    }

    vpss_chn_src.mod_id = MPI_MOD_ID_VPSS;
    vpss_chn_src.dev_id = ctx->vpss_grp;
    vpss_chn_src.chn_id = ctx->vpss_chn_net;

    venc_chn_dst.mod_id = MPI_MOD_ID_VENC;
    venc_chn_dst.dev_id = ctx->venc_dev_net;
    venc_chn_dst.chn_id = ctx->venc_chn_net;

    ret = mpi_sys_bind(&vpss_chn_src, &venc_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to bind VPSS net channel and VENC channel\n");
        goto err_stop_venc_chn;
    }

    ctx->vpss_venc_net_enabled = 1;

    return APP_CAMERA_OK;

err_stop_venc_chn:
    mpi_venc_stop_chn(ctx->venc_chn_net);
err_destroy_venc_chn:
    mpi_venc_destroy_chn(ctx->venc_chn_net);
err_disable_vpss_chn:
    mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_net);
err_destroy_vpss_chn:
    mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_net);
    return APP_CAMERA_ERR_IO;
}

static app_camera_ret_t app_camera_vpss_venc_net_close(app_camera_handle_t ctx)
{
    mpi_ret_t ret;
    mpi_chn_t vpss_chn_src = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = ctx->vpss_chn_net};
    mpi_chn_t venc_chn_dst = {.mod_id = MPI_MOD_ID_VENC, .dev_id = ctx->venc_dev_net, .chn_id = ctx->venc_chn_net};

    if (!ctx->vpss_venc_net_enabled) {
        printf("net stream not started\n");
        return APP_CAMERA_ERR_NOT_READY;
    }

    ret = mpi_sys_unbind(&vpss_chn_src, &venc_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to unbind VPSS net and VENC\n");
    }

    ret = mpi_venc_stop_chn(ctx->venc_chn_net);
    if (ret != MPI_SUCCESS) {
        printf("failed to stop VENC net channel\n");
    }

    ret = mpi_venc_destroy_chn(ctx->venc_chn_net);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VENC net channel\n");
    }

    ret = mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_net);
    if (ret != MPI_SUCCESS) {
        printf("failed to disable VPSS net channel\n");
    }
    ret = mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_net);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VPSS net channel\n");
    }

    ctx->vpss_venc_net_enabled = 0;

    return APP_CAMERA_OK;
}

/* ==================== Net stream worker thread ==================== */

static void app_camera_net_stream_thread(void *parm)
{
    app_camera_handle_t ctx = (app_camera_handle_t)parm;

    while (!ctx->net_thread_exit_flag) {
        mpi_stream_t video_stream = {0};
        mpi_ret_t ret;
        int got_video = 0;
        int wrote = 0;

        ret = mpi_venc_get_stream(ctx->venc_chn_net, &video_stream, 0);
        if (ret == MPI_SUCCESS) {
            if (video_stream.addr != NULL && video_stream.len > 0) {
                got_video = 1;
            } else if (mpi_venc_release_stream(ctx->venc_chn_net, &video_stream) != MPI_SUCCESS) {
                printf("failed to release empty VENC net stream\n");
                break;
            }
        } else if (ret != MPI_ERR_EMPTY && ret != MPI_ERR_NOT_READY) {
            printf("failed to get VENC net stream, ret=%d\n", ret);
            break;
        }

        if (got_video) {
            net_rt_send_frame((struct net_stream_info *)ctx->net_stream_priv,
                              (u8 *)video_stream.addr,
                              video_stream.len,
                              JPEG_TYPE_VIDEO);
            ret = mpi_venc_release_stream(ctx->venc_chn_net, &video_stream);
            if (ret != MPI_SUCCESS) {
                printf("failed to release VENC net stream, ret=%d\n", ret);
                break;
            }
            wrote = 1;
        }

        if (!wrote) {
            os_time_dly(1);
        }
    }
}

/* ==================== Public API ==================== */

app_camera_ret_t app_camera_net_stream_start(app_camera_handle_t handle,
        const app_camera_net_stream_attr_t *attr)
{
    app_camera_ret_t app_ret;
    int once_open_src = 0;

    if (handle == NULL || attr == NULL) {
        printf("handle or net stream attr is NULL\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (attr->width == 0 || attr->height == 0 || attr->fps == 0) {
        printf("invalid net stream size or fps\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (handle->vpss_venc_net_enabled) {
        printf("net stream already started\n");
        return APP_CAMERA_ERR_EXIST;
    }

    if (!app_camera_src_is_opened(handle)) {
        printf("camera source is not opened, open it first\n");
        app_ret = app_camera_src_open(handle);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to open camera source\n");
            return app_ret;
        }
        once_open_src = 1;
    }

    /* Open network stream first */
    handle->net_stream_priv = net_stream_open(attr->path,
                              attr->abr_kbps,
                              attr->fps,
                              attr->audio_enable);
    if (handle->net_stream_priv == NULL) {
        printf("failed to open net stream\n");
        goto err_close_src;
    }

    /* Set up VPSS + VENC pipeline */
    app_ret = app_camera_vpss_venc_net_open(handle, attr);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to start net stream pipeline\n");
        goto err_close_net;
    }

    /* Start worker thread */
    handle->net_thread_exit_flag = 0;
    snprintf(handle->net_thread_name, sizeof(handle->net_thread_name),
             "net_strm_%d", handle->camera_id);
    if (thread_fork(handle->net_thread_name,
                    20,
                    2048,
                    0,
                    &handle->net_thread_pid,
                    app_camera_net_stream_thread,
                    (void *)handle) != 0) {
        printf("failed to create net stream thread\n");
        goto err_close_pipeline;
    }

    printf("net stream started: %s\n", attr->path);
    return APP_CAMERA_OK;

err_close_pipeline:
    app_camera_vpss_venc_net_close(handle);
err_close_net:
    net_stream_close((struct net_stream_info *)handle->net_stream_priv);
    handle->net_stream_priv = NULL;
err_close_src:
    if (once_open_src) {
        app_camera_src_close(handle);
    }
    return APP_CAMERA_ERR_IO;
}

app_camera_ret_t app_camera_net_stream_stop(app_camera_handle_t handle)
{
    app_camera_ret_t app_ret = APP_CAMERA_OK;
    app_camera_ret_t src_ret;

    if (handle == NULL) {
        printf("handle is NULL\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    if (!handle->vpss_venc_net_enabled) {
        printf("net stream not started\n");
        return APP_CAMERA_ERR_NOT_READY;
    }

    /* Stop worker thread */
    if (handle->net_thread_pid) {
        handle->net_thread_exit_flag = 1;
        thread_kill(&handle->net_thread_pid, KILL_WAIT);
        handle->net_thread_pid = 0;
    }

    /* Close network stream */
    if (handle->net_stream_priv) {
        net_stream_close((struct net_stream_info *)handle->net_stream_priv);
        handle->net_stream_priv = NULL;
    }

    /* Close VPSS + VENC pipeline */
    src_ret = app_camera_vpss_venc_net_close(handle);
    if (src_ret != APP_CAMERA_OK) {
        printf("failed to stop net stream pipeline\n");
        if (app_ret == APP_CAMERA_OK) {
            app_ret = src_ret;
        }
    }

    if (app_camera_src_need_stop(handle)) {
        printf("camera source is no longer needed, stopping it\n");
        src_ret = app_camera_src_close(handle);
        if (src_ret != APP_CAMERA_OK) {
            printf("failed to close camera source\n");
            if (app_ret == APP_CAMERA_OK) {
                app_ret = src_ret;
            }
        }
    }

    printf("net stream stopped\n");
    return app_ret;
}

int app_camera_net_stream_is_started(app_camera_handle_t handle)
{
    if (handle == NULL) {
        printf("handle is NULL\n");
        return 0;
    }
    return handle->vpss_venc_net_enabled;
}

#endif /* 摄像头网络流依赖网络, 无网络时禁用 */
