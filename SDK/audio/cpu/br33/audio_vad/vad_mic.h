/*****************************************************************
>file name : vad_mic.h
>author : lichao
>create time : Mon 01 Nov 2021 05:07:50 PM CST
*****************************************************************/
#ifndef _P11_VAD_H_
#define _P11_VAD_H_
#include "includes.h"
#include "asm/power/pmc/lp_ipc.h"
#include "asm/power/pmc/pmc_mmap.h"

//--------------------- p11 VAD参数配置 ---------------------------------//

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
//                      P11_VAD_CONFIG_SIZE                 //
//==========================================================//
#define CONFIG_P2M_VAD_PLATFORM_CONFIG_SIZE (28)
#define CONFIG_P2M_DVAD_ALGO_CONFIG_SIZE    (12)
#define CONFIG_VAD_CONFIG_SIZE              (CONFIG_P2M_VAD_PLATFORM_CONFIG_SIZE + CONFIG_P2M_DVAD_ALGO_CONFIG_SIZE)


#define VAD_CORE_CLK_DIG_SINGLE         	(0)
#define VAD_CORE_CLK_DIF_XOSC           	(1)
#define VAD_CORE_CLK_DIG_XOSC           	(2)

#define	VAD_CORE_PERFORMANCE_MODE0			(0)	// 高性能模式1
#define	VAD_CORE_PERFORMANCE_MODE1			(1)	// 高性能模式2
#define	VAD_CORE_PERFORMANCE_MODE2			(2)	// 高性能模式3
#define	VAD_CORE_LOWPOWER_MODE				(3)	// 低功耗模式

#define VAD_MIC_CORE_CAP_DIFF_MODE      	(0) //差分隔直电容模式
#define VAD_MIC_CORE_CAP_SINGLE_MODE0 		(1) //单端隔直电容模式, AIP(PA0)作为MICIN输入端
#define VAD_MIC_CORE_CAP_SINGLE_MODE1 		(2) //单端隔直电容模式, AIN(PA1)作为MICIN输入端

struct vad_mic_platform_data {
    //digital
    u32 sr;
    s16 vco_dc_sys;         //减去该直流值，13bit
    u16 dig_pvth;           //AVAD数字阈值，16bit
    u16 dig_nvth;           //AVAD数字阈值，16bit
    u8 ana_vth;             //AVAD模拟阈值，4bit
    u8 pfm_mode;            //性能模式，0~4，详见上方宏定义
    u8 mic_dat_sel;		    // MIC数据来源选择，0:ndat	1:pdat	2:pdat-ndat	3:dmic0	4:dmic1
    u8 dcc_en;              //去直流滤波器使能
    u8 dcc_lvl;             //去直流滤波器档位，0~15，15为bypass
    //analog
    u8 mode;                // 模拟通道模式，0~2，详见上方宏定义
    u8 clk_mode;			// 时钟来源选择，0~2，详见上方宏定义
    u8 pga_gain;            // VAD PGA增益，0~15，范围-6~39dB，每档步进 3dB/step
    u8 micldo_pwr;          // MICLDO 输出使能
    u8 micldo_vsel;
    u8 micbias_inside;		// MICBIAS 使能
    u8 micbias_rsel;        // MIC内部偏置电阻大小配置，1~15，范围0.5~10k
    u8 vcm_lvl;             //与主系统vcm_lvl一致
    //clk
    u8 dvad_clk_src;        // DVAD时钟选择，1:std_12m	2:xosc_24m	3:fll_24m 4:sys_clk	5:pat_clk
    //other
    u8 adc_test;            //VAD数据采集测试使能，会持续输出数据到主系统
};

struct dvad_config {
    int smooth;
    int confidence;
    int exit_cnt_limit;
};

#define P11_VAD_IRQ_ENABLE()    \
    PMC_SYSTEM->M2P_INT_IE |= BIT(M2P_VAD_INDEX);

//===========================================================================//
//                          Master to P11 EVENT POST                         //
//===========================================================================//
enum M2P_VAD_CMD_TABLE {
    M2P_VAD_CMD_INIT = 0x55,
    M2P_VAD_CMD_FRAME,
    M2P_VAD_CMD_CLOSE,
    M2P_VAD_CMD_TEST,
};

//===========================================================================//
//                          P11 VAD EVENT CMD                                //
//===========================================================================//
enum P2M_VAD_CMD_TABLE {
    P2M_VAD_TRIGGER_START = 0xAA,
    P2M_VAD_TRIGGER_DMA,
    P2M_VAD_TRIGGER_STOP,
};

static inline void audio_vad_m2p_event_post(enum M2P_VAD_CMD_TABLE cmd)
{
    while (M2P_MESSAGE_VAD_PEND); //PMC收到后清零，防止地址重入修改
    M2P_MESSAGE_VAD_CMD = cmd;

    P11_M2P_INT_SET = BIT(M2P_VAD_INDEX);
    M2P_MESSAGE_VAD_PEND = 1;
}

#define AUDIO_VAD_DMA_READ_UPDATE(n) \
    M2P_MESSAGE_VAD_CBUF_RPTR = (n); \
    audio_vad_m2p_event_post(M2P_VAD_CMD_FRAME);

extern struct vad_mic_platform_data vad_data;
extern const int config_lp_vad_enable;

int lp_vad_mic_data_init(struct vad_mic_platform_data *mic_data);

void *lp_vad_mic_open(void *priv, int (*dma_output)(void *priv, s16 *data, int len));

void lp_vad_mic_close(void *vad);

void lp_vad_mic_test(void);

void lp_vad_mic_disable(void);
#endif
