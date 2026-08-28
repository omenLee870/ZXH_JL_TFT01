#ifndef VSTREAM_CMD_H
#define VSTREAM_CMD_H


typedef enum {
    VSTREAM_CMD_HEARTBEAT_PARCKET	= 0X01,

    VSTREAM_CMD_VIDEO_START			= 0x20,
    VSTREAM_CMD_VIDEO_STOP,
    VSTREAM_CMD_VOICE_START,
    VSTREAM_CMD_VOICE_STOP,
    VSTREAM_CMD_VIDEO_RECONN,

    VSTREAM_CMD_CHAT_START			= 0x40,
    VSTREAM_CMD_CHAT_STOP,

    VSTREAM_CMD_CHAT_MEME,
    VSTREAM_CMD_CHAT_TEXT,
    VSTREAM_CMD_CHAT_MUSIC,
    VSTREAM_CMD_CHAT_VOICE,
    VSTREAM_CMD_CHAT_PHOTO,

    VSTREAM_CMD_USER_CMD			= 0XFF,
} VSTREAM_CMD;

enum {
    VSTREAM_CMD_TYPE_SEND_WITH_ASK = 0XC0,
    VSTREAM_CMD_TYPE_SEND_WITOUT_ASK = 0X80,
    VSTREAM_CMD_TYPE_RECV = 0X00,
};
enum {
    VSTREAM_CMD_DISCONENT,
    VSTREAM_CMD_CONNENT,
};
struct packet_head {
    u32 magic_a;

    u8 type;
    u8 cmd;
    u8 cmd_sn;
    u8 version;

    u16 data_crc;
    u16 data_len;

    u8 data[0];
};

#pragma pack(1)//不对齐编译
struct packet_end {
    u8 magic_b;
};
#pragma pack(0)//对齐编译

#define VSTREAM_MAGIC_A 0XFEDCBA
#define VSTREAM_MAGIC_B 0xEF

typedef enum {
    VSTREAM_CMD_NO_ERR,
    VSTREAM_CMD_PACKET_ERR,
    VSTREAM_CMD_TYPE_ERR,
    VSTREAM_CMD_SEND_ERR,
} VSTREAM_CMD_ERR;

int vstream_cmd_send_recv(u8 cmd, u8 cmd_sn, u8 *data, u8 data_len);
int vstream_cmd_send_with_adk_sn(u8 cmd, u8 *cmd_sn, u8 *data, int data_len);
int vstream_cmd_send_with_adk(u8 cmd, u8 *data, int data_len);
int vstream_cmd_recv(u8 *data, int len);
//对讲设置
int vstream_cmd_send_video_start();
int vstream_cmd_send_video_stop();
int vstream_cmd_video_status_get();
int vstream_cmd_send_video_reconnect();
int vstream_cmd_answer_video_ctrl(u8 ctrl);
//语音设置
int vstream_cmd_send_voice_start();
int vstream_cmd_send_voice_stop();
int vstream_cmd_voice_status_get();

int vstream_cmd_video_status_set(u8 status);
int vstream_cmd_voice_status_set(u8 status);

int vstream_cmd_video_break_set(u8 status);
int vstream_cmd_video_break_get();
#endif /* VSTREAM_CMD_H */
