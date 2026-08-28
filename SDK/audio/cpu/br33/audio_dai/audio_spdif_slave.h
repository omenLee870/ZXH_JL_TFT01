#ifndef _AUDIO_SPDIF_SLAVE_H_
#define _AUDIO_SPDIF_SLAVE_H_

#include "generic/typedef.h"
#include "generic/list.h"


#define SS_RESET()		do {JL_SPDIF->SS_CON = 0; JL_SPDIF->SS_IO_CON = 0; JL_SPDIF->SS_DMA_CON = 0;} while(0)

#define SS_EN(x)         SFR(JL_SPDIF->SS_CON, 0, 1, x)
#define SS_START(x)      SFR(JL_SPDIF->SS_CON, 1, 1, x)
#define SS_DET_MODE(x)      SFR(JL_SPDIF->SS_CON, 2, 1, x)

#define DATA_PND_CLR()	    (JL_SPDIF->SS_CON |= BIT(3))
#define DATA_PND()         (JL_SPDIF->SS_CON &  BIT(4))

#define INFO_PND_CLR()     (JL_SPDIF->SS_CON |= BIT(5))
#define INFO_PND()         (JL_SPDIF->SS_CON &  BIT(6))

#define ERR_ALL_CLR()   (JL_SPDIF->SS_CON |= BIT(7))
#define F_ERR()         (JL_SPDIF->SS_CON &  BIT(8))
#define B_ERR()         (JL_SPDIF->SS_CON &  BIT(9))
#define D_ERR()         (JL_SPDIF->SS_CON &  BIT(10))
#define RL_ERR()        (JL_SPDIF->SS_CON &  BIT(11))
#define ERROR_VALUE()     (F_ERR() || B_ERR() || D_ERR() || RL_ERR())

#define USING_BUF_INDEX       (JL_SPDIF->SS_CON & BIT(12))
#define USING_INF_BUF_INDEX   (JL_SPDIF->SS_CON & BIT(13))
#define RLE_CNT_SET(x)			SFR(JL_SPDIF->SS_CON, 16, 16, x)

#define IO_IN_SEL(x)     SFR(JL_SPDIF->SS_IO_CON, 0, 2, x)
#define IO_IN_EN(x)      SFR(JL_SPDIF->SS_IO_CON, 2, 1, x)

#define IO_IS_DEN(x, en)     SFR(JL_SPDIF->SS_IO_CON, 4 + x, 1, en)

#define IO_GET_INSERT()     ((JL_SPDIF->SS_IO_CON >> 8) & 0x7)
#define IS_A_ONLINE()    (JL_SPDIF->SS_IO_CON & BIT(12))
#define IS_B_ONLINE()    (JL_SPDIF->SS_IO_CON & BIT(13))
#define IS_C_ONLINE()    (JL_SPDIF->SS_IO_CON & BIT(14))
#define IS_D_ONLINE()    (JL_SPDIF->SS_IO_CON & BIT(15))


#define IS_PND_CLR()    (JL_SPDIF->SS_IO_CON |= BIT(16))
#define IS_PND()        (JL_SPDIF->SS_IO_CON & BIT(17))

#define ET_PND_CLR()    (JL_SPDIF->SS_IO_CON |= BIT(18))
#define ET_PND()        (JL_SPDIF->SS_IO_CON & BIT(19))

#define IO_ET_DEN(x, en)     SFR(JL_SPDIF->SS_IO_CON, 20 + x, 1, en)

#define IO_IN_TO_OUT_SEL(x)     SFR(JL_SPDIF->SS_IO_CON, 24, 2, x)
#define IO_IN_TO_OUT_EN(x)      SFR(JL_SPDIF->SS_IO_CON, 26, 1, x)

#define IO_OUTPUT_SEL(x)       SFR(JL_SPDIF->SS_IO_CON, 27, 1, x)

#define IO_IN_ANA_MUX(x)      SFR(JL_SPDIF->SS_IO_CON, 28, 1, x)
#define IO_IN_ANA_EN(x)       SFR(JL_SPDIF->SS_IO_CON, 29, 1, x)
#define IO_IN_ANA_SEL(x)      SFR(JL_SPDIF->SS_IO_CON, 30, 2, x)


#define DATA_DMA_EN(x)         SFR(JL_SPDIF->SS_DMA_CON, 0, 1, x)
#define DATA_DMA_MD(x)         SFR(JL_SPDIF->SS_DMA_CON, 1, 1, x)

#define INFO_DMA_EN(x)         SFR(JL_SPDIF->SS_DMA_CON, 2, 1, x)

#define RLE_DECT_EN(x)        SFR(JL_SPDIF->SS_DMA_CON, 3, 1, x)

#define SR_CHECK_UPGRADE(x)     SFR(JL_SPDIF->SS_DMA_CON, 4, 1, x)
#define OUT_ORIGINAL_INF(x)     SFR(JL_SPDIF->SS_DMA_CON, 5, 1, x)

#define CSBR_CNT(x)           SFR(JL_SPDIF->SS_DMA_CON, 8, 5, x)
#define CSBR_PND_CLR()    	  (JL_SPDIF->SS_DMA_CON |= BIT(13))
#define CSBR_PND()            (JL_SPDIF->SS_DMA_CON & BIT(14))

#define CSBR_PND_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 15, 1, x)
#define DATA_PND_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 16, 1, x)
#define INFO_PND_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 17, 1, x)
#define IS_PND_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 18, 1, x)
#define ET_PND_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 19, 1, x)
#define F_ERR_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 20, 1, x)
#define B_ERR_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 21, 1, x)
#define D_ERR_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 22, 1, x)
#define SE_ERR_IE(x)           SFR(JL_SPDIF->SS_DMA_CON, 23, 1, x)

#define FTL_TRACE(x)           SFR(JL_SPDIF->SS_DMA_CON, 24, 3, x)
#define FTL_SEL(x)           SFR(JL_SPDIF->SS_DMA_CON, 27, 2, x)


#define DATA_DMA_LEN(x)           SFR(JL_SPDIF->SS_DMA_LEN, 0, 16, x)
#define INFO_DMA_LEN(x)           SFR(JL_SPDIF->SS_DMA_LEN, 16, 16, x)


typedef enum {
    SPDIF_IN_DIGITAL_PORT_A	= 0u,
    SPDIF_IN_DIGITAL_PORT_B,
    SPDIF_IN_DIGITAL_PORT_C,
    SPDIF_IN_DIGITAL_PORT_D,

    SPDIF_IN_ANALOG_PORT_A,
    SPDIF_IN_ANALOG_PORT_B,
    SPDIF_IN_ANALOG_PORT_C,
    SPDIF_IN_ANALOG_PORT_D,
} SPDIF_SLAVE_CH;


struct spdif_s_ch_cfg {
    SPDIF_SLAVE_CH port_sel;	//
    u8 data_io;					//data IO配置
    u8 et_den;					//
    u8 is_den;					//
};

typedef enum {
    SPDIF_S_DAT_24BIT	= 0u,
    SPDIF_S_DAT_16BIT,
} SPDIF_DAT_MD;

typedef enum {
    SPDIF_S_DAT_UNKNOWN		= 0u,
    SPDIF_S_DAT_PCM,
    SPDIF_S_DAT_NOT_PCM,
} SPDIF_DAT_FORMAT;

typedef struct _SPDIF_SLAVE_PARM {
    u32 rx_clk;
    struct spdif_s_ch_cfg ch_cfg[4];		//通道内部配置
    SPDIF_DAT_MD data_mode;
    u32 data_dma_len;				//24bit: x*4*2*2	16bit: x*2*2*2	x为192的整数倍
    u32 inf_dma_len;				//x*2*2/2		x应和data_dma_len的一样
    u32 *data_buf;					//dma buf地址
    u32 *test_buf;					//dma buf地址
    u32 *inf_buf;					//dma buf地址
    void (*data_isr_cb)(void *buf, u32 len);
    void (*inf_isr_cb)(void *buf, u32 len);
} SPDIF_SLAVE_PARM;


u32 spdif_slave_get_sr();		//ss获取采样率
SPDIF_DAT_FORMAT spdif_slave_get_data_format();	//ss获取是否线性PCM格式

void *spdif_slave_init(void *hw_spdif_slave);	//ss模块初始化
int spdif_slave_start(void *hw_spdif_slave);	//ss模块启动
int spdif_slave_stop(void *hw_spdif_slave);		//ss模块停止
int spdif_slave_uninit(void *hw_spdif_slave);	//ss模块反初始化



void *spdif_slave_channel_init(void *hw_spdif_slave, u8 ch_idx);	//ss通道初始化
int spdif_slave_channel_open(void *hw_spdif_slave_channel);			//ss通道启动
int spdif_slave_channel_close(void *hw_spdif_slave_channel);		//ss通道关闭


#endif/*_AUDIO_SPDIF_SLAVE_H_*/
