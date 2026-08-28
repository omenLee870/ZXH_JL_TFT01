#ifndef _VIDEO_RX_PLAYER_H_
#define _VIDEO_RX_PLAYER_H_

#include "jlstream.h"

enum VIDEO_SERVICE {
    VIDEO_SERVICE_MEDIA,
    VIDEO_SERVICE_CALL_DOWNSTREAM,
    VIDEO_SERVICE_CALL_UPSTREAM,
    VIDEO_SERVICE_VOICE
};

struct video_rx_player_param {
    u8 type;
    u8 channel_mode;
    u8 format_mode;
    u16 packet_len;
    u16 frame_dms;		//帧长时间，单位 deci-ms (ms/10)
    u32 coding_type;
    u32 sample_rate;
    u32 bit_rate;
};


struct video_rx_file_handle {
    void *file;
    void *bt_addr;
    u8 start;
    u8 source;
    u8 reference;
    struct stream_node *node;
    u32 play_latency; //us
    enum stream_node_state(*rx_func)(void *buf, struct stream_frame **pframe);
    struct video_rx_player_param param;
    const struct stream_file_ops *file_ops;
};


int video_rx_player_open(u8 source, struct video_rx_player_param *param,
                         enum stream_node_state(*rx_func)(void *file, struct stream_frame **pframe));

void video_rx_player_close(u8 source);

bool video_rx_player_runing(u8 source);

#endif
