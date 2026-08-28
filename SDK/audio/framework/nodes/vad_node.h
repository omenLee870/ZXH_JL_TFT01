#ifndef __VAD_NODE__H
#define __VAD_NODE__H

#include "jlstream_node_cfg.h"
#include "crc.h"
#include "fs/resfile.h"

#define NN_VAD BIT(0)
#define AUDIO_NN_VAD_PARAM_FILE 	(FLASH_RES_PATH"vad.bin")

struct vad_param {
    u8 vad_type;
};

struct packet_ctrl {
    void *node_hdl;
    s16 *remain_buf;
    void (*vad_run)(void *, s16 *, s16 *, u32);
    u32 sample_rate;    //采样率
    u16 remain_len;     //记录算法消耗的剩余的数据长度
    u16 frame_len;
    u8 in_ch_num;       //输入声道数,单声道:1，立体声:2, 四声道:4
    u8 out_ch_num;      //输出声道数,单声道:1，立体声:2, 四声道:4
    u8 bit_width;       //位宽, 0:16bit  1:32bit
    u8 qval;            //q, 15:16bit, 23:24bit
};

struct vad_node_hdl {
    char name[16];
#if TCFG_AUDIO_VAD_TYPE & NN_VAD
    int out_flag;
    void *share_buffer;
    void *heap_buffer;
    void *model;
#endif
    void *vad;
    struct vad_param cfg;
    struct packet_ctrl dev;
};

#endif
