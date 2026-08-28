#ifndef VSTREAM_COMMON_H
#define VSTREAM_COMMON_H

#include "generic/list.h"

#define VSTREAM_PCM_TYPE_AUDIO      1
#define VSTREAM_JPEG_TYPE_VIDEO     2
#define VSTREAM_H264_TYPE_VIDEO     3
#define VSTREAM_PREVIEW_TYPE        4

/* ============================================================================
 * KWT video codec select: 1 = H264 (SOFT_264 VENC / soft-H264 VDEC), 0 (default)
 * = JPEG.  Compile-time symmetry: sender AND receiver must build with the same
 * value.  Defined here (not kid_wt_video.h) because vstream_sender.c and
 * vstream_receiver.c also include this header and need the same switch for
 * buffer sizing + type routing.  Set to 1 (or pass -DKWT_USE_H264=1) to enable.
 * ============================================================================ */
#ifndef KWT_USE_H264
#define KWT_USE_H264            0
#endif

#define VSTREAM_DATE_TIME_TYPE      5
#define VSTREAM_MEDIA_INFO_TYPE  	6
#define VSTREAM_PLAY_OVER_TYPE      7
#define VSTREAM_GPS_INFO_TYPE	    8
#define VSTREAM_NO_GPS_DATA_TYPE	9
#define VSTREAM_G729_TYPE_AUDIO    10

#define VSTREAM_TYPE_FILE			0X3e//file
#define VSTREAM_TYPE_CMD			0x3f//max


#define VSTREAM_FAST_PLAY_MAKER (1<<6)
#define VSTREAM_LAST_FREG_MAKER (1<<7)
#define VSTREAM_DATA_CRC_CHECK_EN	1
struct frm_head {
    u8 type;
    u8 res;
#if 0
    u8 sample_seq;
#endif
    u16 payload_size;
    u32 seq;
    u32 frm_sz;
    u32 offset;
#if VSTREAM_DATA_CRC_CHECK_EN
    u16 block_crc;
    u16 crc16;//total
#else
    u32 timestamp;
#endif
} __attribute__((packed));

void vsteam_frame_head_dump(struct frm_head *head, const char *func, int line);



/*
 *********************************************************************************************************
 *                                             发送
 *********************************************************************************************************
 */
//COMMON
#define FILE_PACKET_SIZE			512
#define UDP_SEND_BUF_SIZE  (650)
#define MAX_PAYLOAD (650-sizeof(struct frm_head))   //650 spp最大有效
//CMD
//音频1帧512
//图片1帧不超过6k
//这里预留3*6+2
#define VIDEO_STREAM_PACKET_NUM 	8
#define AUDIO_STREAM_PACKET_NUM		8
#define CMD_STREAM_PACKET_NUM		(4)
#define FILE_STREAM_PACKET_NUM		(6)
#if KWT_USE_H264
#define VIDEO_STREAM_BUF_ALIGN		(64 * 1024)   /* H264 I-frame upper bound (320x240) */
#else
#define VIDEO_STREAM_BUF_ALIGN		(TCFG_JPEG_MAX_SIZE)
#endif
#define AUDIO_STREAM_BUF_ALIGN		(600)
#define CMD_STREAM_BUF_ALIGN		(64)
#define FILE_STREAM_BUF_ALIGN		(512)

#define VSTREAM_SPP_TX_CTRL_INTERVAL    10  //ms


struct vstream_info {
    int sample_rate;
    int abr_kbps;
    int fps;
};


int vstream_sender_open(struct vstream_info *s_info);
int vstream_sender_close(void);
void vstream_sender_reset(void);
int vstream_send_frame(u8 *buffer, size_t len, u8 type);


int vstream_sender_video_init();
int vstream_sender_video_deinit();
int vstream_sender_audio_init();
int vstream_sender_audio_deinit();
int vstream_sender_file_init();
int vstream_sender_file_deinit();
/*
 *********************************************************************************************************
 *                                            接收
 *********************************************************************************************************
 */

#define VSTREAM_MSG_RECV                    1

#define STREAM_RECV_BUF_SIZE               650
#define STREAM_RECV_BUF_NUM				   10		//more than 6k
#if KWT_USE_H264
#define STREAM_PARSE_BUF_SIZE               (64 * 1024)            /* H264 reassembly buffer */
#else
#define STREAM_PARSE_BUF_SIZE               TCFG_JPEG_MAX_SIZE//max(video,audio,cmd)
#endif
#define STREAM_PACKET_VIDEO_QUEUE_SIZE     (VIDEO_STREAM_PACKET_NUM *VIDEO_STREAM_BUF_ALIGN	 + 256)
#define STREAM_PACKET_AUDIO_QUEUE_SIZE     (AUDIO_STREAM_PACKET_NUM	*AUDIO_STREAM_BUF_ALIGN	+ 256)
#define STREAM_PACKET_CMD_QUEUE_SIZE       (CMD_STREAM_PACKET_NUM * CMD_STREAM_BUF_ALIGN+256)
#define STREAM_PACKET_FILE_QUEUE_SIZE		(FILE_STREAM_PACKET_NUM *FILE_STREAM_BUF_ALIGN+256)

typedef enum {
    VIDEO_FRAME = 0x01,
    AUDIO_FRAME = 0x02,
    CMD_FRAME = 0x03,
    FILE_FRAME = 0x04,
} FRAME_TYPE;


struct vstream_recv_hdl {
    u8 stream_recv_task_exit;
    int recv_len;
    struct list_head dev_list;
    u8 *recv_cmd_deal_task;
    struct lbuff_head *recv_lbuf_handle;
    u8 *recv_lbuf_ptr;
};




int vstream_receiver_open(void);
int vstream_receiver_close(void);
void vstream_spp_rx_handler(u8 *buf, u16 len);


int vstream_recv_acquire_frame(u32 ip_addr, u8 **frame_buf, int *frame_len, FRAME_TYPE frame_type);
int vstream_recv_acquire_frame_block(u32 ip_addr, u8 **frame_buf, int *frame_len, FRAME_TYPE frame_type, int timeout_ms);
void video_stream_recv_release_frame(u8 *frame_buf);
int stream_dev_create_video();
int stream_dev_create_audio();
int stream_dev_destroy_video();
int stream_dev_destroy_audio();
int stream_dev_destroy_file();
int stream_dev_media_clear(void);
int vstream_recv_audio_ready(void);
enum {
    CHAT_TYPE_MEME,		//表情包
    CHAT_TYPE_TEXT,		//文本
    CHAT_TYPE_MUSIC,	//音乐

    CHAT_TYPE_VOICE,	//语音
    CHAT_TYPE_PIC,		//图片

    CHAT_TYPE_MAX,
};
enum {
    //发送
    FILE_STATUS_SEND = 0,
    //接收
    FILE_STATUS_RECV = 1,
    //中间态
    FILE_STATUS_LOADING = 0xFE,
    FILE_STATUS_ERROR = 0XFF,
};
#pragma pack(1)//不对齐编译
struct chat_data_storage {
    u8 type;
    u8 recv;//收到的数据
    u8 data[0];
};
struct chat_data_mult_packet {
    int offset;		//数据偏移
    int total_len;	//总长度
    u16 data_len;
    u8 data[0];
};
#pragma pack(0)//对齐编译


void kid_wt_chat_save_dump(struct chat_data_storage *save);
int kid_wt_char_get_rec_data_total();
int kid_wt_chat_get_rec_data_by_index(int index, struct chat_data_storage *save);
int vstream_cmd_ask_chat(u8 cmd, u8 cmd_sn, u8 *data, int len);
int vstream_cmd_recv_chat(u8 cmd, u8 cmd_sn, u8 *data, int len);
int vstream_cmd_send_chat(u8 type, u8 *data, int data_len);
int vstream_cmd_send_chat_save(u8 cmd, u8 *data, int data_len, u8 status);
#endif

/* ============================================================================
 * UI / File system constants — 源 701n demo 中定义，目标 SDK 需对接
 * ============================================================================ */

/* File type IDs (chat record) are now defined in data_storage/data_storage.h;
   do NOT redefine F_TYPE_CHAT_REC here to avoid clashing with the SDK type system. */

/* ============================================================================
 * Standard file I/O compatibility — 源 SDK fwrite(fp,buf,len) → 标准 fwrite(buf,1,len,fp)
 * ============================================================================ */
#ifndef fwrite3
#define fwrite3(fp, buf, len)   fwrite(buf, 1, len, fp)
#endif
