#ifndef __PC_RX_FILE_H
#define __PC_RX_FILE_H

#include "cpu.h"

#define PC_SPK_STREAM_PLAY   	   (1UL << 0)
#define PC_SPK_STREAM_PAUSE   	   (1UL << 1)
#define PC_SPK_STREAM_WAIT_PAUSE   (1UL << 2)
#define PC_SPK_STREAM_WAIT_RESUNE  (1UL << 3)

u8 is_pc_spk_file_start(void);
u8 is_pc_spk_file_run(void);
u32 pc_spk_get_fmt_sample_rate(void);
void pc_spk_set_fmt(u8 channel, u8 bit, u32 sample_rate);

void pc_spk_data_isr_cb(void *buf, u32 len);	//spk 中断往数据流推数据函数
int pc_mode_broadcast_deal_by_taskq(int broadcast_music_status);

u8 pc_spk_get_stream_status(void);
void pc_spk_fill_packet_init();//主动补0帧
void pc_spk_fill_packet_uninit();
#endif

