

#ifndef _INTERCOM_RX_PLAYER_H_
#define _INTERCOM_RX_PLAYER_H_

#include "jlstream.h"

enum INTERCOM_SERVICE {
    INTERCOM_SERVICE_VOICE
};

struct intercom_rx_player_param {
    u8 type;
    u8 channel_mode;
    u8 format_mode;
    u16 packet_len;
    u16 frame_dms;		//帧长时间，单位 deci-ms (ms/10)
    u32 coding_type;
    u32 sample_rate;
    u32 bit_rate;
};
struct intercom_rx_file_handle {
    void *file;
    void *bt_addr;
    u8 start;
    u8 source;
    u8 reference;
    struct stream_node *node;
    u32 play_latency; //us
    enum stream_node_state(*rx_func)(void *buf, struct stream_frame **pframe);
    struct intercom_rx_player_param param;
    const struct stream_file_ops *file_ops;
};


enum  opus_dec_frame_len {
    FRAME_LEN_40 = 0,
    FRAME_LEN_80,
    FRAME_LEN_160,
};
int intercom_rx_player_open(u8 *bt_addr, u8 source, struct intercom_rx_player_param *param, enum stream_node_state(*rx_func)(void *file, struct stream_frame **pframe));


void intercom_rx_player_close(u8 source);

bool intercom_rx_player_runing(u8 source);

#endif
