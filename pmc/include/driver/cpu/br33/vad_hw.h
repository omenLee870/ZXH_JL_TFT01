#ifndef __VAD_HW_H__
#define __VAD_HW_H__

#include "vad_algo.h"
#include "audio_vad.h"

//==========================================================//
//                      P11_VAD_CBUF_SIZE                   //
//==========================================================//
#define VAD_POINT_PER_FRAME 				(160)
#define VAD_CBUF_FRAME_CNT 					(13)
#define VAD_CBUF_FRAME_CNT_EXT              (1) //低功耗时，由于主系统唤醒需要时间，可能丢失第一次中断响应，CBUF多申请一帧用于缓存
#define VAD_CBUF_FRAME_CNT_TOTAL            (VAD_CBUF_FRAME_CNT + VAD_CBUF_FRAME_CNT_EXT)
#define VAD_CBUF_FRAME_SIZE 				(VAD_POINT_PER_FRAME * 2)
#define VAD_CBUF_STRUCT_SIZE                (32) //cbuffer_t 结构体大小
#define CONFIG_P11_CBUF_SIZE 				(VAD_CBUF_STRUCT_SIZE + VAD_CBUF_FRAME_SIZE * VAD_CBUF_FRAME_CNT_TOTAL)

//==========================================================//
//                      P11_VAD_DMA_BUF_SIZE                //
//==========================================================//
#define VAD_DMA_BUF_SIZE                    (VAD_CBUF_FRAME_SIZE * 2) //乒乓buf

//==========================================================//
//                      P11_VAD_CONFIG_SIZE                 //
//==========================================================//
#define CONFIG_P2M_VAD_PLATFORM_CONFIG_SIZE	(28)
#define CONFIG_P2M_DVAD_ALGO_CONFIG_SIZE    (12)
#define CONFIG_VAD_CONFIG_SIZE              (CONFIG_P2M_VAD_PLATFORM_CONFIG_SIZE + CONFIG_P2M_DVAD_ALGO_CONFIG_SIZE)

void vad_hw_buf_setup();
volatile u8 *vad_dvad_algo_addr_get();
volatile u8 *vad_platform_data_addr_get();
volatile u8 *vad_cbuf_struct_addr_get();
volatile u8 *vad_cbuf_buf_addr_get();

void vad_init(struct vad_mic_platform_data *platform_data, struct dvad_config *d_cfg);

void vad_close(void);

void vad_test(void);
#endif /* #ifndef __VAD_HW_H__ */
