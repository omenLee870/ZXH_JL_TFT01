#ifndef __NET_SCR_API_H__
#define __NET_SCR_API_H__

#include "generic/typedef.h"
#include "sock_api/sock_api.h"
#include "server/rt_stream_pkg.h"

typedef void (*net_scr_ack_cb_t)(int seq);

typedef enum {
    MAP_VIDEO_SOURCE_NONE = 0,
    MAP_VIDEO_SOURCE_WIFI_JPEG,
    MAP_VIDEO_SOURCE_WIFI_JLJPEG,
    MAP_VIDEO_SOURCE_BT_H264,
} map_video_source_t;

struct __JPG_INFO {
    u32 src_w;
    u32 src_h;
    u32 buf_len;
    u8  buf[];
};

struct __NET_SCR_CFG {
    struct sockaddr_in cli_addr;
    int fps;
    int src_w;
    int src_h;
    int prot;               /* 0: tcp, 1: udp */
    int ack;                /* 0: fps paced, 1: ack paced */
    int frame_type;         /*2: JPEG, 10: JLJPEG*/
    net_scr_ack_cb_t ack_cb;
};

u8 get_net_scr_status(void);
int net_scr_init(struct __NET_SCR_CFG *cfg);
int net_scr_uninit(struct __NET_SCR_CFG *cfg);
void map_video_enter(void);
void map_video_leave(void);

int map_video_source_acquire(map_video_source_t source);
void map_video_source_release(map_video_source_t source);
int map_video_source_is(map_video_source_t source);
int map_video_page_active(void);


#endif

