#include "app_config.h"

#if TCFG_NETWORK_ENABLE
#include "system/includes.h"
#include "screen_mirror.h"
#include "screen_mirror_bt_h264.h"
#include "wifi_connect.h"
#include "lwip.h"
#include "playback/app_playback_mpi.h"
#include "camera/app_camera_mpi.h"
#include "lcd_conf.h"
#include "fs/fs.h"
#include "lv_port_video_disp.h"
#include "lvgl_video_init.h"

#define LOG_TAG_CONST       SCREEN_MIRROR
#define LOG_TAG             "[SCREEN_MIRROR]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE

#include "debug.h"

#if TCFG_NET_SCR_ENABLE

#define SCR_THREAD_TASK_NAME    "scr_recv"
#define JPG_BUF_MAX_SIZE        (120 * 1024)
#define JPG_BUF_NUM             3   //JPEG buf缓存数量
#define UDP_MAX_RECV            1540
#define CHECK_CODE              0x88
#define CHECK_CODE_NUM          32
#define SERVER_TCP_PORT         8888

struct net_scr_jpg_ctx {
    u32 frame_seq;
    u32 total_payload_len;
    u8 frame_active;
};

struct net_scr_runtime {
    volatile u8 state;
    int socket_type;
    int fps;
    int src_width;
    int src_height;
    int ack;
    net_scr_ack_cb_t ack_cb;
    u32 last_seq;
    struct sockaddr_in cli_addr;
    void *sock_hdl;
    void *cli_sock_hdl;
    int recv_task_pid;
    u8 sock_fps;
    u8 disp_fps;

    OS_MUTEX jpeg_buf_mutex;
    OS_SEM jpeg_buf_sem;
    u8 jpeg_buf_sync_inited;
    u8 jpeg_buf_busy[JPG_BUF_NUM];
    u8 jpeg_recv_idx;
    struct __JPG_INFO *jpeg_frame[JPG_BUF_NUM];

};

static struct net_scr_runtime g_net_scr;
static u8 g_udp_recv_buf[UDP_MAX_RECV];
static app_playback_handle_t scr_hdl;
#define __ALIGN_16(a)   (((a) + 15) / 16 * 16)
#define __this (&g_net_scr)


int screen_mpi_open(void);
app_playback_ret_t screen_mpi_close(void);
int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);

__attribute__((weak)) void screen_map_post_msg_to_lvgl_ui(const char *msg, ...)
{
    (void)msg;
}

__attribute__((weak)) void screen_map_request_display(int active)
{
    (void)active;
}

static u32 min_u32(u32 a, u32 b)
{
    return a < b ? a : b;
}

static int buffer_all_equal(const u8 *buf, u8 value, u32 len)
{
    while (len--) {
        if (*buf++ != value) {
            return 0;
        }
    }

    return 1;
}

static void close_sock(void **sock_hdl)
{
    if (*sock_hdl) {
        sock_unreg(*sock_hdl);
        *sock_hdl = NULL;
    }
}

static void request_sock_quit(void *sock_hdl)
{
    if (sock_hdl) {
        sock_set_quit(sock_hdl);
    }
}

static int same_cli(const struct sockaddr_in *lhs, const struct sockaddr_in *rhs)
{
    if (!lhs || !rhs) {
        return 0;
    }

    /* The stream socket can use a different source port than the CTP socket. */
    return lhs->sin_addr.s_addr == rhs->sin_addr.s_addr;
}

static int net_scr_sock_init(struct net_scr_runtime *hdl)
{
    int ret;
    struct sockaddr_in server_addr = {0};

    hdl->sock_hdl = sock_reg(AF_INET, hdl->socket_type, 0, NULL, NULL);
    if (!hdl->sock_hdl) {
        log_error("socket reg failed");
        return -1;
    }

    if (sock_set_reuseaddr(hdl->sock_hdl)) {
        log_error("socket reuseaddr failed");
        close_sock(&hdl->sock_hdl);
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(NET_SCR_PORT);

    ret = sock_bind(hdl->sock_hdl, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    if (ret) {
        log_error("socket bind failed");
        close_sock(&hdl->sock_hdl);
        return -1;
    }

    if (hdl->socket_type == SOCK_STREAM) {
        ret = sock_listen(hdl->sock_hdl, 0xFF);
        if (ret) {
            log_error("socket listen failed");
            close_sock(&hdl->sock_hdl);
            return -1;
        }
    }

    return 0;
}

static void net_scr_jpeg_buf_deinit(struct net_scr_runtime *hdl)
{
    for (int i = 0; i < JPG_BUF_NUM; i++) {
        if (hdl->jpeg_frame[i]) {
            free_psram(hdl->jpeg_frame[i]);
            hdl->jpeg_frame[i] = NULL;
        }
        hdl->jpeg_buf_busy[i] = 0;
    }
}

static void net_scr_jpeg_buf_wait_idle(struct net_scr_runtime *hdl)
{
    for (int retry = 0; retry < 50; retry++) {
        u8 busy = 0;

        os_mutex_pend(&hdl->jpeg_buf_mutex, 0);
        for (int i = 0; i < JPG_BUF_NUM; i++) {
            if (hdl->jpeg_buf_busy[i]) {
                busy = 1;
                break;
            }
        }
        os_mutex_post(&hdl->jpeg_buf_mutex);

        if (!busy) {
            return;
        }

        os_sem_pend(&hdl->jpeg_buf_sem, 10);
    }
}


static int net_scr_jpeg_buf_init(struct net_scr_runtime *hdl)
{
    if (!hdl->jpeg_buf_sync_inited) {
        os_mutex_create(&hdl->jpeg_buf_mutex);
        os_sem_create(&hdl->jpeg_buf_sem, 0);
        hdl->jpeg_buf_sync_inited = 1;
    }

    for (int i = 0; i < JPG_BUF_NUM; i++) {
        hdl->jpeg_frame[i] = malloc_psram(sizeof(struct __JPG_INFO) + JPG_BUF_MAX_SIZE);
        if (!hdl->jpeg_frame[i]) {
            log_error("jpeg frame buf alloc failed: %d", i);
            net_scr_jpeg_buf_deinit(hdl);
            return -1;
        }
        memset(hdl->jpeg_frame[i], 0, sizeof(struct __JPG_INFO) + JPG_BUF_MAX_SIZE);
        hdl->jpeg_frame[i]->src_w = hdl->src_width;
        hdl->jpeg_frame[i]->src_h = hdl->src_height;
    }

    hdl->jpeg_recv_idx = 0;
    return 0;
}

static struct __JPG_INFO *net_scr_get_recv_jpg(struct net_scr_runtime *hdl)
{
    while (hdl->state) {
        os_mutex_pend(&hdl->jpeg_buf_mutex, 0);
        for (int i = 0; i < JPG_BUF_NUM; i++) {
            u8 idx = (hdl->jpeg_recv_idx + i) % JPG_BUF_NUM;

            if (!hdl->jpeg_buf_busy[idx]) {
                os_mutex_post(&hdl->jpeg_buf_mutex);
                return hdl->jpeg_frame[idx];
            }
        }
        os_mutex_post(&hdl->jpeg_buf_mutex);

        os_sem_pend(&hdl->jpeg_buf_sem, 10);
    }

    return NULL;
}

static void net_scr_jpeg_buf_mark_busy(struct net_scr_runtime *hdl,
                                       struct __JPG_INFO *jpg)
{
    os_mutex_pend(&hdl->jpeg_buf_mutex, 0);
    for (int i = 0; i < JPG_BUF_NUM; i++) {
        if (hdl->jpeg_frame[i] == jpg) {
            hdl->jpeg_buf_busy[i] = 1;
            hdl->jpeg_recv_idx = (i + 1) % JPG_BUF_NUM;
            break;
        }
    }
    os_mutex_post(&hdl->jpeg_buf_mutex);
}

void screen_map_jpeg_buf_release(void *buf)
{
    struct net_scr_runtime *hdl = __this;

    if (!buf || !hdl->jpeg_buf_sync_inited) {
        return;
    }

    os_mutex_pend(&hdl->jpeg_buf_mutex, 0);
    for (int i = 0; i < JPG_BUF_NUM; i++) {
        if (hdl->jpeg_frame[i] && hdl->jpeg_frame[i]->buf == buf) {
            if (hdl->jpeg_buf_busy[i]) {
                hdl->jpeg_buf_busy[i] = 0;
                os_sem_post(&hdl->jpeg_buf_sem);
            }
            break;
        }
    }
    os_mutex_post(&hdl->jpeg_buf_mutex);
}


static inline int net_scr_is_video_frame_type(u32 frame_type)
{
    return frame_type == JPEG_TYPE_VIDEO || frame_type == JLJPEG_TYPE_VIDEO;;
}

static int net_scr_read_tcp_frame(struct net_scr_runtime *hdl, struct __JPG_INFO *jpg)
{
    int recv_len;
    struct frm_head head_info = {0};
    u32 frame_type;

    recv_len = sock_recv(hdl->cli_sock_hdl, &head_info, sizeof(head_info), MSG_WAITALL);
    log_info("recv len : %d \n", recv_len);
    if (recv_len <= 0) {
        return -1;
    }

    if (recv_len != sizeof(head_info)) {
        log_error("tcp recv head len err: %d", recv_len);
        return -1;
    }

    if (head_info.frm_sz > JPG_BUF_MAX_SIZE) {
        log_error("tcp frame too large: %u", head_info.frm_sz);
        return -1;
    }

    hdl->last_seq = head_info.seq;
    if (hdl->ack_cb && hdl->state) {
        hdl->ack_cb(hdl->last_seq);
    }

    recv_len = sock_recv(hdl->cli_sock_hdl, jpg->buf, head_info.frm_sz, MSG_WAITALL);
    if (recv_len <= 0 || recv_len != head_info.frm_sz) {
        log_error("tcp recv payload len err: %d/%u", recv_len, head_info.frm_sz);
        return -1;
    }

    frame_type = head_info.type & 0x7F;
    if (!net_scr_is_video_frame_type(frame_type)) {
        log_error("frame type is unexpected: %u", frame_type);
        return 1;
    }

    jpg->buf_len = recv_len;
    return 0;
}

static int net_scr_build_udp_frame(struct net_scr_runtime *hdl,
                                   struct net_scr_jpg_ctx *ctx,
                                   const u8 *buf,
                                   int len,
                                   struct __JPG_INFO *jpg)
{
    u32 position = 0;
    u32 frame_head_size = sizeof(struct frm_head);

    if (len < frame_head_size) {
        log_error("udp recv len err: %d", len);
        return -1;
    }

    while (len > 0) {
        if ((u32)len < frame_head_size) {
            log_error("udp frame head err");
            return -1;
        }

        const struct frm_head *head_info = (const struct frm_head *)(buf + position);
        u32 frame_type = head_info->type & 0x7F;
        u32 cur_frame_seq = head_info->seq;
        u32 frame_offset = head_info->offset;
        u32 slice_data_len = head_info->payload_size;
        u32 frame_size = head_info->frm_sz;
        u32 probe_len;

        /* printf("[UDP_SLICE] seq=%u off=%u payload=%u frm_sz=%u type=%u \n", cur_frame_seq, frame_offset, slice_data_len, frame_size, head_info->type); */

        if ((u32)len < frame_head_size + slice_data_len) {
            log_error("udp remain len err");
            return -1;
        }
        len -= frame_head_size + slice_data_len;

        if (!net_scr_is_video_frame_type(frame_type)) {
            log_error("frame type is unexpected: %u", frame_type);
            position += frame_head_size + slice_data_len;
            continue;
        }

        if (frame_size > JPG_BUF_MAX_SIZE) {
            log_error("udp frame too large: %u", frame_size);
            return -1;
        }

        if (frame_offset >= frame_size || frame_offset + slice_data_len > frame_size) {
            log_error("udp frame offset err: off=%u payload=%u size=%u", frame_offset, slice_data_len, frame_size);
            return -1;
        }

        if (!ctx->frame_active || cur_frame_seq > ctx->frame_seq) {
            ctx->frame_active = 1;
            ctx->frame_seq = cur_frame_seq;
            ctx->total_payload_len = 0;
            memset(jpg->buf, CHECK_CODE, JPG_BUF_MAX_SIZE);
        } else if (cur_frame_seq < ctx->frame_seq) {
            position += frame_head_size + slice_data_len;
            continue;
        }

        probe_len = min_u32(CHECK_CODE_NUM, slice_data_len);
        if (!buffer_all_equal(jpg->buf + frame_offset, CHECK_CODE, probe_len)) {
            position += frame_head_size + slice_data_len;
            continue;
        }


        memcpy(jpg->buf + frame_offset, buf + position + frame_head_size, slice_data_len);
        ctx->total_payload_len += slice_data_len;

        if (ctx->total_payload_len == frame_size) {
            jpg->buf_len = frame_size;
            hdl->last_seq = cur_frame_seq;
            return 0;
        }

        position += frame_head_size + slice_data_len;
    }

    return -1;
}

static int net_scr_accept_client(struct net_scr_runtime *hdl)
{
    struct sockaddr_in cli_addr = {0};
    socklen_t addrlen = sizeof(cli_addr);

    while (hdl->state) {
        void *cli_sock = sock_accept(hdl->sock_hdl, (struct sockaddr *)&cli_addr, &addrlen, NULL, NULL);
        if (!cli_sock) {
            return -1;
        }

        if (same_cli(&hdl->cli_addr, &cli_addr)) {
            hdl->cli_sock_hdl = cli_sock;
            return 0;
        }

        log_error("reject tcp client ip: 0x%x", cli_addr.sin_addr.s_addr);
        sock_unreg(cli_sock);
        addrlen = sizeof(cli_addr);
    }

    return -1;
}

static void statistic_socket_fps(void *priv)
{
    struct net_scr_runtime *hdl = (struct net_scr_runtime *)priv;

    log_info("sock fps: %d, disp fps: %d\n", hdl->sock_fps, hdl->disp_fps);
    hdl->sock_fps = 0;
    hdl->disp_fps = 0;
}

uint32_t timer_get_ms(void);
extern int get_g_ui_in_map_flag(void);

__attribute__((weak))
int get_g_ui_in_map_flag(void)
{
    return 0;
}

typedef enum {
    MAP_VIDEO_CLOSED = 0,
    MAP_VIDEO_OPENED,
} map_video_state_t;

static volatile int g_map_video_active = 0;  //是否在map界面
static map_video_state_t g_map_video_state = MAP_VIDEO_CLOSED; //解码器状态
static map_video_source_t g_map_video_source = MAP_VIDEO_SOURCE_NONE; //视频源
static OS_MUTEX g_map_video_mutex;

static int g_map_video_mutex_inited;
lv_video_disp_handle_t *map_disp_handle = NULL;    //视频显示句柄
void lvgl_map_exit(void);

static void map_video_ctrl_init_once(void)
{
    if (g_map_video_mutex_inited) {
        return;
    }

    os_mutex_create(&g_map_video_mutex);
    g_map_video_mutex_inited = 1;
}
/* 指定视频源 */
int map_video_source_acquire(map_video_source_t source)
{
    int ret = 0;

    if (source == MAP_VIDEO_SOURCE_NONE) {
        return -1;
    }

    map_video_ctrl_init_once();
    os_mutex_pend(&g_map_video_mutex, 0);
    if (g_map_video_source != MAP_VIDEO_SOURCE_NONE &&
        g_map_video_source != source) {
        log_error("map video source busy: %d", g_map_video_source);
        ret = -1;
    } else {
        g_map_video_source = source;
    }
    os_mutex_post(&g_map_video_mutex);
    return ret;
}

void map_video_source_release(map_video_source_t source)
{
    map_video_ctrl_init_once();
    os_mutex_pend(&g_map_video_mutex, 0);
    if (g_map_video_source == source) {
        g_map_video_source = MAP_VIDEO_SOURCE_NONE;
    }
    os_mutex_post(&g_map_video_mutex);
}

int map_video_source_is(map_video_source_t source)
{
    int is_source;

    map_video_ctrl_init_once();
    os_mutex_pend(&g_map_video_mutex, 0);
    is_source = (g_map_video_source == source);
    os_mutex_post(&g_map_video_mutex);
    return is_source;
}

int map_video_page_active(void)
{
    int active;

    map_video_ctrl_init_once();
    os_mutex_pend(&g_map_video_mutex, 0);
    active = g_map_video_active;
    os_mutex_post(&g_map_video_mutex);
    return active;
}

void map_video_enter(void)
{
    map_video_ctrl_init_once();
    os_mutex_pend(&g_map_video_mutex, 0);

    g_map_video_active = 1;
    os_mutex_post(&g_map_video_mutex);
    screen_mirror_bt_h264_map_enter();
}

static void map_video_close_locked(void)
{
    app_playback_ret_t ret;

    if (g_map_video_state != MAP_VIDEO_OPENED) {
        return;
    }

    ret = screen_mpi_close();
    if (ret != APP_PLAYBACK_OK) {
        printf("map video close failed, ret=%d\n", ret);
        return;
    }

    g_map_video_state = MAP_VIDEO_CLOSED;
}

static void map_video_stream_exit(void)
{
    map_video_ctrl_init_once();

    os_mutex_pend(&g_map_video_mutex, 0);
#if (SCR_FRAME_TYPE == JPEG_TYPE_VIDEO)
    map_video_close_locked();
#endif

#if (SCR_FRAME_TYPE == JLJPEG_TYPE_VIDEO)
    screen_map_post_msg_to_lvgl_ui("screen_map_jljpg_clear");
#endif
    if (g_map_video_source == MAP_VIDEO_SOURCE_WIFI_JPEG ||
        g_map_video_source == MAP_VIDEO_SOURCE_WIFI_JLJPEG) {

        g_map_video_source = MAP_VIDEO_SOURCE_NONE;
    }

    os_mutex_post(&g_map_video_mutex);
}

void map_video_leave(void)
{
    map_video_ctrl_init_once();

    os_mutex_pend(&g_map_video_mutex, 0);
    g_map_video_active = 0;

    map_video_close_locked();
    os_mutex_post(&g_map_video_mutex);
    screen_mirror_bt_h264_map_leave();
}

int map_video_input_frame(u8 *buf, int len)
{
    int ret = 0;

    map_video_ctrl_init_once();

    os_mutex_pend(&g_map_video_mutex, 0);

    if (!g_map_video_active) {
        goto __exit;
    }

#if (SCR_FRAME_TYPE == JPEG_TYPE_VIDEO) 	//jpeg解码
    if (g_map_video_source == MAP_VIDEO_SOURCE_NONE) {
        g_map_video_source = MAP_VIDEO_SOURCE_WIFI_JPEG;
    }
    if (g_map_video_source != MAP_VIDEO_SOURCE_WIFI_JPEG) {
        goto __exit;
    }

    if (g_map_video_state == MAP_VIDEO_CLOSED) {
        if (screen_mpi_open() != APP_PLAYBACK_OK) {
            goto __exit;
        }

        g_map_video_state = MAP_VIDEO_OPENED;
    }

    // UI为MAP界面且MPI解码器已打开
    if (g_map_video_state == MAP_VIDEO_OPENED) {
        app_playback_one_image_data(scr_hdl, buf, len);
        screen_map_jpeg_buf_release(buf);
        ret = 1;
    }

#elif (SCR_FRAME_TYPE == JLJPEG_TYPE_VIDEO) //jljpeg解码

    if (g_map_video_source == MAP_VIDEO_SOURCE_NONE) {
        g_map_video_source = MAP_VIDEO_SOURCE_WIFI_JLJPEG;
    }
    if (g_map_video_source != MAP_VIDEO_SOURCE_WIFI_JLJPEG) {
        goto __exit;
    }

    // UI为MAP
    if (g_map_video_active) {
        screen_map_post_msg_to_lvgl_ui("screen_map_jljpg_update", buf, len);
        ret = 1;
    }
#endif

__exit:
    os_mutex_post(&g_map_video_mutex);
    return ret;
}

static void scr_recv_task(void *priv)
{
    struct net_scr_runtime *hdl = (struct net_scr_runtime *)priv;
    struct __JPG_INFO *jpg;
    struct net_scr_jpg_ctx jpg_ctx = {0};
    u32 fps_time = 0;

    if (net_scr_jpeg_buf_init(hdl)) {
        hdl->recv_task_pid = 0;
        return;
    }

    if (hdl->socket_type == SOCK_STREAM && net_scr_accept_client(hdl)) {
        net_scr_jpeg_buf_deinit(hdl);
        hdl->recv_task_pid = 0;
        return;
    }


    while (hdl->state) {
        int ret;
        jpg = net_scr_get_recv_jpg(hdl);
        if (!jpg) {
            break;
        }

        if (hdl->socket_type == SOCK_STREAM) {
            ret = net_scr_read_tcp_frame(hdl, jpg);
            if (ret < 0) {
                break;
            }
            if (ret > 0) {
                continue;
            }
        } else {
            struct sockaddr_in cli_addr = {0};
            socklen_t addrlen = sizeof(cli_addr);

            ret = sock_recvfrom(hdl->sock_hdl, g_udp_recv_buf, sizeof(g_udp_recv_buf), 0, (struct sockaddr *)&cli_addr, &addrlen);
            if (ret < 0) {
                break;
            }
            if (!same_cli(&hdl->cli_addr, &cli_addr)) {
                continue;
            }

            ret = net_scr_build_udp_frame(hdl, &jpg_ctx, g_udp_recv_buf, ret, jpg);
            if (ret) {
                continue;
            }

            if (hdl->state && hdl->ack_cb) {
                hdl->ack_cb(hdl->last_seq);
            }
        }

        if (ret == 0) {
            log_d("recv a image, len: %d", jpg->buf_len);
            hdl->sock_fps++;
            net_scr_jpeg_buf_mark_busy(hdl, jpg);
            if (!map_video_input_frame(jpg->buf, jpg->buf_len)) {
                screen_map_jpeg_buf_release(jpg->buf);
            }

        }
        if (!fps_time) {
            fps_time = timer_get_ms();
        } else if (timer_get_ms() - fps_time >= 1000) {
            log_info("sock fps: %d, disp fps: %d\n",
                     hdl->sock_fps,
                     hdl->disp_fps);
            hdl->sock_fps = 0;
            hdl->disp_fps = 0;
            fps_time = timer_get_ms();
        }

    }
    map_video_stream_exit();

    net_scr_jpeg_buf_wait_idle(hdl);
    net_scr_jpeg_buf_deinit(hdl);
    hdl->recv_task_pid = 0;

}


int net_scr_init(struct __NET_SCR_CFG *cfg)
{
    int ret;

    if (!cfg) {
        return -1;
    }

    if (__this->state) {
        log_error("%s multiple init", __func__);
        return 0;
    }

    memset(__this, 0, sizeof(*__this));
    memcpy(&__this->cli_addr, &cfg->cli_addr, sizeof(struct sockaddr_in));
    __this->socket_type = (cfg->prot == 0) ? SOCK_STREAM : SOCK_DGRAM;
    __this->fps = cfg->fps ? cfg->fps : 25;
    __this->src_width = cfg->src_w;
    __this->src_height = cfg->src_h;
    __this->ack = cfg->ack;
    __this->ack_cb = cfg->ack_cb;

    log_info("scr size: %d x %d", __this->src_width, __this->src_height);
    log_info("scr fps: %d, prot: %s, ack: %d",
             __this->fps,
             (__this->socket_type == SOCK_STREAM) ? "tcp" : "udp",
             __this->ack);

    ret = net_scr_sock_init(__this);
    if (ret) {
        memset(__this, 0, sizeof(*__this));
        return ret;
    }

    __this->state = 1;
    ret = thread_fork(SCR_THREAD_TASK_NAME, 22, 1024, 0, &__this->recv_task_pid, scr_recv_task, __this);

    if (ret != OS_NO_ERR) {
        log_error("scr recv thread fork fail: %d", ret);
        __this->state = 0;
        close_sock(&__this->cli_sock_hdl);
        close_sock(&__this->sock_hdl);
        memset(__this, 0, sizeof(*__this));
        return -1;
    }
    return 0;
}


int net_scr_uninit(struct __NET_SCR_CFG *cfg)
{
    if (!cfg) {
        return -1;
    }

    if (!__this->state) {
        return -1;
    }

    if (!same_cli(&__this->cli_addr, &cfg->cli_addr)) {
        log_error("cli addr not match");
        return -1;
    }

    __this->state = 0;

    request_sock_quit(__this->cli_sock_hdl);
    request_sock_quit(__this->sock_hdl);

    thread_kill(&__this->recv_task_pid, KILL_WAIT);

    close_sock(&__this->cli_sock_hdl);
    close_sock(&__this->sock_hdl);

    memset(__this, 0, sizeof(*__this));
    return 0;
}

void lvgl_create_map_disp(int32_t width, int32_t height, int32_t x, int32_t y)
{
#if (defined LV_USE_VIDEO_DISP && LV_USE_VIDEO_DISP)
    if (map_disp_handle == NULL) {
        map_disp_handle = lv_port_video_disp_create(lv_screen_active(), width, height, x, y, LV_COLOR_FORMAT_NV12, "fb5");
    }
    if (!map_disp_handle) {
        printf("lv_port_video_disp_create fail \n");
        return;
    }
    lv_obj_move_background(lv_port_video_disp_get_img_obj(map_disp_handle));  //MAP img置于UI底层

#else
    printf("error LV_USE_VIDEO_DISP \n");
    return;
#endif
}

void lvgl_destroy_map_disp(void *disp_handle)
{
#if (defined LV_USE_VIDEO_DISP && LV_USE_VIDEO_DISP)
    if (disp_handle) {
        lv_port_video_disp_del((lv_video_disp_handle_t *)disp_handle);
    }
#else
    printf("error LV_USE_VIDEO_DISP \n");
#endif
}

void lvgl_map_init(int32_t width, int32_t height, int32_t x, int32_t y)
{
    log_info("lvgl_map_init");
    lvgl_rpc_post_func((void *)lvgl_create_map_disp, 4, width, height, x, y);
}

void lvgl_map_exit(void)
{
    if (map_disp_handle) {
        /* 同步退出: 删视频 img + 排空双链表(LIST==2)异步渲染, 返回后再由
         * screen_mpi_close() 释放 playback buffer 才安全。 */
        lvgl_video_exit_drain(map_disp_handle);
        map_disp_handle = NULL;
    }
}

int screen_mpi_open(void)
{
    printf("screen_mpi_open\n");

    app_playback_ret_t ret;
    app_playback_display_attr_t display_attr = {
        .fb_name = "fb5",
        .fb_num = 2,
        .width = __ALIGN_16(LCD_WIDTH),
        .height = __ALIGN_16(LCD_HEIGHT),
    };
    printf("w: %d, h: %d , left: %d, top: %d\n", display_attr.width, display_attr.height, display_attr.left, display_attr.top);
    ret = app_playback_open(&scr_hdl, &display_attr);
    if (ret != APP_PLAYBACK_OK) {
        printf("failed to open playback handle, ret=%d\n", ret);
        return ret;
    } else {
        lvgl_map_init(display_attr.width, display_attr.height, display_attr.left, display_attr.top);
    }
    return ret;

}

app_playback_ret_t screen_mpi_close(void)
{
    lvgl_map_exit();
    if (scr_hdl == NULL) {
        return APP_PLAYBACK_OK;
    }
    return app_playback_close(&scr_hdl);
}

u8 get_net_scr_status(void)
{
    return __this->state;
}

static int tcp_client_init(const char *server_ip, int server_port)
{
    void *sock = NULL;
    struct sockaddr_in dest = {0};

    sock = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
    if (!sock) {
        log_error("sock_reg fail");
        return -1;
    }

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(server_ip);
    dest.sin_port = htons(server_port);
    if (sock_connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in))) {
        log_error("sock_connect fail");
        sock_unreg(sock);
        return -1;
    }

    sock_unreg(sock);
    return 0;
}

static void tcp_client_connect_task(void *priv)
{
    char gateway[16] = {0};

    (void)priv;
    get_gateway(1, gateway);
    log_info("tcp connect : server ip[%s], port[%d]", gateway, SERVER_TCP_PORT);
    tcp_client_init(gateway, SERVER_TCP_PORT);
}

void connect_to_server_port_8888(void)
{
    //if (thread_fork("tcp_client_connect_task", 10, 512, 0, NULL, tcp_client_connect_task, NULL) != OS_NO_ERR) {
    if (os_task_create(tcp_client_connect_task, NULL, 10, 512, 0, "tcp_client_connect_task") != OS_NO_ERR) {

        log_error("thread fork fail");
    }
}

#endif


#endif /* 网络功能宏: 投屏依赖网络, kid camera 不用 */
