#ifndef _AUDIO_LINK_H_
#define _AUDIO_LINK_H_

#include "generic/typedef.h"
#include "asm/dac.h"

#define ALINK_MODULE_NUM_MAX    2//iis模块数量
#define ALINK_CH_NUM_MAX        4//iis通道数量

typedef struct {
    volatile unsigned short CON0;
    volatile unsigned short CON1;
    volatile unsigned short CON2;
    volatile unsigned int   CON3;
    volatile unsigned int   CON4;
    volatile unsigned int   CON5;
    volatile unsigned int   ADR0;
    volatile unsigned int   ADR1;
    volatile unsigned int   ADR2;
    volatile unsigned int   ADR3;
    volatile unsigned int   LEN[2];
    volatile unsigned int   PNS[4];
    volatile unsigned int   HWPTR[4];
    volatile unsigned int   SWPTR[4];
    volatile unsigned int   SHN[4];
    volatile unsigned int   BLOCK[4];
} JL_ALINK;

#define IIS_BASE            1
#define IIS_LEFT            2
#define IIS_RIGHT           3
#define DSP0_SHORT          4
#define DSP1_SHORT          5
#define DSP0_LONG           6
#define DSP1_LONG           7
#define ALINK_WORK_MODE IIS_BASE


#define IIS_CH_NUM  2  //测试通道数 1：mono  2:stereo 4:quad
#define ALINK_SR_NUM 17
#define IIS_SW_MUTE_EN 0 //br56 alink 支持hw_mute/hw_unmute
#define IIS_USE_DOUBLE_BUF_MODE_EN 0
#define ALINK_FRAME_MODE_EN  0
#define ALINK_SLOT_NUM_EN 1
#define ALINK_MODE_SEL    1 //have ch_mode
extern u32 alink_sr_tab[ALINK_SR_NUM];


#define HW_MUTE_AND_UNMUTE_TEST  0

#define  ALINK_VERSION_01 1

#define ALINK_SEL(module, reg)             (((JL_ALNK_TypeDef    *)(((unsigned int)JL_ALNK0) + module*((unsigned int)JL_ALNK1 - (unsigned int)JL_ALNK0)))->reg)

#define ALINK_CON_RESET(module)	do {ALINK_SEL(module, CON0) = 0; ALINK_SEL(module, CON1) = 0; ALINK_SEL(module, CON2) = 0; ALINK_SEL(module, CON3) = 0;SFR(ALINK_SEL(module, CON4), 13, 4, 0x0f);SFR(ALINK_SEL(module, CON4), 17, 4, 0x0);SFR(ALINK_SEL(module, CON4), 28, 4, 0x0);ALINK_SEL(module, CON5) = 0xffffffff;} while(0)
#define ALINK_HWPTR_RESET(module)	do {ALINK_SEL(module, HWPTR0) = 0; ALINK_SEL(module, HWPTR1) = 0; ALINK_SEL(module, HWPTR2) = 0; ALINK_SEL(module, HWPTR3) = 0;} while(0)
#define ALINK_SWPTR_RESET(module)	do {ALINK_SEL(module, SWPTR0) = 0; ALINK_SEL(module, SWPTR1) = 0; ALINK_SEL(module, SWPTR2) = 0; ALINK_SEL(module, SWPTR3) = 0;} while(0)
#define ALINK_SHN_RESET(module)	do {ALINK_SEL(module, SHN0) = 0; ALINK_SEL(module, SHN1) = 0; ALINK_SEL(module, SHN2) = 0; ALINK_SEL(module, SHN3) = 0;} while(0)
#define ALINK_ADR_RESET(module)	do {ALINK_SEL(module, ADR0) = 0; ALINK_SEL(module, ADR1) = 0; ALINK_SEL(module, ADR2) = 0; ALINK_SEL(module, ADR3) = 0;} while(0)
#define ALINK_PNS_RESET(module)	do {ALINK_SEL(module, PNS0) = 0;ALINK_SEL(module, PNS1) = 0;ALINK_SEL(module, PNS2) = 0;ALINK_SEL(module, PNS3) = 0;} while(0)
#define ALINK_BLOCK_RESET(module)	do {ALINK_SEL(module, BLOCK0) = 0;ALINK_SEL(module, BLOCK1) = 0;ALINK_SEL(module, BLOCK2) = 0;ALINK_SEL(module, BLOCK3) = 0;} while(0)

//CON0
#define	ALINK_DA2BTSRC_SEL(module, x)	SFR(ALINK_SEL(module, CON0), 0, 2, x)
#define	ALINK_DMA_MODE_SEL(module, x)	SFR(ALINK_SEL(module, CON0), 2, 1, x)
#define ALINK_DSP_MODE(module, x)		SFR(ALINK_SEL(module, CON0), 5, 2, x)
#define ALINK_SOE(module, x) 			SFR(ALINK_SEL(module, CON0), 7, 1, x)		//unless for crossbar
#define ALINK_MOE(module, x) 			SFR(ALINK_SEL(module, CON0), 8, 1, x)		//unless for crossbar
#define ALINK_SCLKINV(module, x)   		SFR(ALINK_SEL(module, CON0),10, 1, x)
#define ALINK_EN(module, x)  		    SFR(ALINK_SEL(module, CON0),11, 1, x)
#define ALINK_EN_GET(module)            (ALINK_SEL(module, CON0) & BIT(11)) ? 1 : 0


//CON1
#define ALINK_BWLEN_MODE(module, x)			 SFR(ALINK_SEL(module, CON1),12, 2, x)
#define ALINK_CHx_DIR_MODE(module, ch, x)  	 SFR(ALINK_SEL(module, CON1), 2 + 3 * ch, 1, x)
#define ALINK_CHx_MODE_SEL(module, ch, x)  	 SFR(ALINK_SEL(module, CON1), 3 * ch, 2, x)
#define ALINK_CHx_CLOSE(module, ch)  	 	 SFR(ALINK_SEL(module, CON1), 3 * ch, 2, 0)

//CON2
#define ALINK_CHx_PND(module, ch)			(ALINK_SEL(module, CON2) & BIT(ch+4))
#define ALINK_CLR_ALL_PND(module)			(ALINK_SEL(module, CON2) |= BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define ALINK_CLR_CHx_PND(module, ch)		(ALINK_SEL(module, CON2) |= BIT(ch))
#define ALINK_CHx_IE(module, ch, x)			SFR(ALINK_SEL(module, CON2), ch + 12, 1, x)
#define ALINK_CHx_IE_GET(module, ch)			(ALINK_SEL(module, CON2) & BIT(ch + 12))?1:0

//CON3
#define ALINK_MSRC(module, x)			SFR(ALINK_SEL(module, CON3), 0, 2, x)
#define ALINK_SDIV(module, x)			SFR(ALINK_SEL(module, CON3), 2, 5, x)	//0~31 for 1~32
#define ALINK_SCLK_SEL(module, x)		SFR(ALINK_SEL(module, CON3), 7, 1, x)
#define ALINK_LRCK_PRD_SET(module, x)	SFR(ALINK_SEL(module, CON3), 8, 9, x)	//LRCK=SCLK/(x+1)	x = bit_wide * slot
#define ALINK_LRCK_DUTY_SET(module, x)	SFR(ALINK_SEL(module, CON3),17, 6, x)
#define ALINK_SLOT_SET(module, x)		SFR(ALINK_SEL(module, CON3),23, 3, x)
#define ALINK_SW_MUTE_EN(module, ch, x)    SFR(ALINK_SEL(module, CON3), ch + 26, 1, x)

//CON4
#define ALINK_SW_MUTE_WAIT_ACK(module, ch)  while(!(ALINK_SEL(module, CON4) & BIT(ch + 0)))
#define ALINK_SW_MUTE_STATUS(module, ch)    (ALINK_SEL(module, CON4) & BIT(ch + 0))?1:0

#define ALINK_CH_DMA_REG_EN(module, ch, x)    SFR(ALINK_SEL(module, CON4),ch+13, 1, x)
#define ALINK_DAC2LINK_EN(module, ch, x)      SFR(ALINK_SEL(module, CON4),ch+17, 1, x)
#define ALINK_MDIV(module, x)                 SFR(ALINK_SEL(module, CON4),21, 7, x)

//CON4
#define ALINK_SLOT_GATE_EN(module, ch, x)     SFR(ALINK_SEL(module, CON4),ch+28, 1, x)
//CON5
#define ALINK_SLOT_EN(module, ch, x)      SFR(ALINK_SEL(module, CON5),8*ch, 8, x)

/* #define ALINK_OPNS_SET(module, ch, x)	SFR(*ALNK0_PNS[(ch)], 16, 16, x) */
// #define ALINK_IPNS_SET(module, ch, x)		SFR(*ALNK0_PNS[(ch)], 0, 16, x)
/* #define ALINK_IPNS_GET(module, ch)      (*ALNK0_PNS[(ch)] & 0xffff) */
// #define ALINK_LEN_SET(module, x)		(ALINK_SEL(module, LEN) = x)
// #define ALINK_FIFO_LEN(module)          (ALINK_SEL(module, LEN))

// #define ALINK_BLOCK_SET(module, ch, x)   (*ALNK0_BLOCK[ch] = x)


// ALINK_BLOCK_SET(module, x)



#define ALINK_CLK_OUPUT_DISABLE 	0xFF

typedef enum {
    ALINK0			= 0u,	//BR50只有1个ALNK模块
    ALINK1,						//BR27有两个ALNK模块
} ALINK_PORT;

//ch_num
typedef enum {
    ALINK_CH0		= 0u,
    ALINK_CH1,
    ALINK_CH2,
    ALINK_CH3,
} ALINK_CH;

//ch_dir
typedef enum {
    ALINK_DIR_TX	= 0u,
    ALINK_DIR_RX		,
} ALINK_DIR;

typedef enum {
    ALINK_BW_16BIT = 16,
    ALINK_BW_20BIT = 20,	//only for DSP mode
    ALINK_BW_24BIT = 24,
    ALINK_BW_32BIT	= 32,
} ALINK_DATA_BW;



typedef enum {
    ALINK_SLOT_NUM1 = 1,
    ALINK_SLOT_NUM2 	,	//basic mode must be
    ALINK_SLOT_NUM3 	,
    ALINK_SLOT_NUM4		,
    ALINK_SLOT_NUM5		,
    ALINK_SLOT_NUM6		,
    ALINK_SLOT_NUM7		,
    ALINK_SLOT_NUM8		,
} ALINK_SLOT_NUM;

//mode
typedef enum {
    ALINK_MD_BASIC	= 0u,
    ALINK_MD_DSP_SHORT	,
    ALINK_MD_DSP_LONG	,
} ALINK_MODE;

//ch_mode
typedef enum {
    ALINK_CH_MD_NONE	= 0u,
    ALINK_CH_MD_BASIC_IIS = 1,
    ALINK_CH_MD_BASIC_LALIGN = 2,
    ALINK_CH_MD_BASIC_RALIGN = 3,
    ALINK_CH_MD_DSP0 = 1,
    ALINK_CH_MD_DSP1 = 2,
} ALINK_CH_MODE;

typedef enum {
    ALINK_ROLE_MASTER, //主机
    ALINK_ROLE_SLAVE,  //从机
} ALINK_ROLE;

typedef enum {
    ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE, //下降沿更新数据, 上升沿采样数据
    ALINK_CLK_RAISE_UPDATE_FALL_SAMPLE, //上降沿更新数据, 下升沿采样数据
} ALINK_CLK_MODE;


typedef enum {
    ALINK_SR_384000 = 384000,
    ALINK_SR_352800 = 352800,
    ALINK_SR_192000 = 192000,
    ALINK_SR_176400 = 176400,
    ALINK_SR_128000 = 128000,
    ALINK_SR_96000 = 96000,
    ALINK_SR_88200 = 88200,
    ALINK_SR_64000 = 64000,
    ALINK_SR_48000 = 48000,
    ALINK_SR_44100 = 44100,
    ALINK_SR_32000 = 32000,
    ALINK_SR_24000 = 24000,
    ALINK_SR_22050 = 22050,
    ALINK_SR_16000 = 16000,
    ALINK_SR_12000 = 12000,
    ALINK_SR_11025 = 11025,
    ALINK_SR_8000  = 8000,
} ALINK_SR;

struct alink2da_param {
    u8 module_idx;
    u8 bit_width;
    u8 clk_close;
    u8 clk_mode;          //更新与采样边沿
    u8 alink_work_mode;   //alink模式配置
    u8 ch_num;
    u16 irq_points;
    u32 sr;
    u32 dma_time_ms;
    struct audio_dac_merge_param merge_param;
};
typedef enum {
    ALINK_BUF_DUAL, 	//乒乓BUF
    ALINK_BUF_CIRCLE,	//循环BUF
} ALINK_BUF_MODE;

struct ch_adv_cfg {
    u8 slot_gate_en;
    u8 slot_en;
    u8 dma_en; //CH0接收k模式下 0:rx不起中断 1:正常起中断
    u8 dac_2_alink_en; //CH1发送模式下 0:正常获取dma数据, 1:获取来自dac0和dac1得数据
};

struct alink_hw_cfg {
    void (*isr_cb)(void *priv, void *addr, int len);	//中断回调
    void *priv;			//音频私有数据
    struct ch_adv_cfg adv_cfg;
    ALINK_DIR dir; 				//通道传输数据方向: Tx, Rx
    ALINK_PORT	module;
    ALINK_CH	ch_idx;
    ALINK_CH_MODE ch_mode;		//iis left right DSP0 DSP1
    u8 bit_width;
    u8 data_io;					//data IO配置
    u8 ch_ie;					//中断使能
    u8 rx_dma_ctrl;
};

struct alnk_hw_ch {
    void *hw_alink;
    void *buf;					//dma buf地址
    struct alink_hw_cfg cfg;
    u32 timestamp;
    u32 timestamp_gap;
    u32 timestamp_cnt;
    u32 dma_len; 						//buf长度: byte
    u8 sw_mute_status;
    u8 chx_sw;
    u8 ch_en;					//使用标记
};

//===================================//
//多个通道使用需要注意:
//1.数据位宽需要保持一致
//2.buf长度相同
//===================================//
typedef struct _ALINK_PARM {
    JL_ALINK *regs;
    ALINK_PORT	module;
    u8 mclk_io; 				//mclk IO输出配置: ALINK_CLK_OUPUT_DISABLE不输出该时钟
    u8 sclk_io;					//sclk IO输出配置: ALINK_CLK_OUPUT_DISABLE不输出该时钟
    u8 lrclk_io;				//lrclk IO输出配置: ALINK_CLK_OUPUT_DISABLE不输出该时钟
    struct alnk_hw_ch *hw_ch[ALINK_CH_NUM_MAX];		//通道内部配置
    ALINK_MODE mode; 					//IIS, left, right, dsp
    ALINK_ROLE role; 			//主机/从机
    ALINK_CLK_MODE clk_mode; 			//更新和采样边沿
    ALINK_DATA_BW  bitwide;   //数据位宽16/32bit
    ALINK_SLOT_NUM  slot_num;   //slot num
    ALINK_SR sample_rate;					//采样
    ALINK_BUF_MODE 	buf_mode;  	//乒乓buf or 循环buf率
    u32 init_cnt; 						//buf长度: byte
    u16 rx_pns;
    u16 tx_pns;
    u8 da2sync_ch;             //与蓝牙同步关联的目标通道
    u8 tdm_32bit_en;
    u8 io_drive_strength_mode;//io驱动强度设置 0/1/2/3 四档,查看enum gpio_drive_strength
} ALINK_PARM;

//iis 模块相关
void *alink_init(void *hw_alink);  //iis 初始化
int alink_start(void *hw_alink);             //iis 开启
int alink_set_sr(void *hw_alink, u32 sr); 			//iis 设置采样率
int alink_get_sr(void *hw_alink); 				//iis 获取采样率
u32 alink_get_dma_len(void *hw_alink, u32 ch); 				//iis 获取DMA_LEN
void alink_set_rx_pns(void *hw_alink, u32 ch, u32 len);		//iis 设置接收PNS
void alink_set_tx_pns(void *hw_alink, u32 ch, u32 len);		//iis 设置发送PNS
int alink_uninit_base(void *hw_alink, void *hw_channel);
int alink_uninit(void *hw_alink, void *hw_channel);
void alink_uninit_force(void *hw_alink);
void alink_sr(void *hw_alink, u32 rate);
/* void ALINK_OPNS_SET(u32 module, u32 ch, u32 x); */
// void ALINK_IPNS_SET(u32 module, u32 ch, u32 x);
// void ALINK_LEN_SET(u32 module, u32 ch, u32 x);
// u32 ALINK_FIFO_LEN(u32 module, u32 ch);




//iis 通道相关
void alink_set_irq_handler(void *hw_alink, void *hw_channel,  void *priv, void (*handle)(void *priv, void *addr, int len));
void *alink_channel_init(void *hw_alink, void *hw_channel);
void alink_channel_io_init(void *hw_alink, void *hw_channel);
void alink_channel_close(void *hw_alink, void *hw_channel);
s32 *alink_get_addr(void *hw_channel);			//iis获取通道DMA地址
u32 alink_get_shn(void *hw_channel);			//iis获取通道SHN
u32 alink_get_shn_lock(void *hw_channel);
u32 alink_get_len(void *hw_channel);			//iis 获取长度LEN
void alink_set_shn(void *hw_channel, u32 len);	//iis设置通道SHN
void alink_set_shn_with_no_sync(void *hw_channel, u32 len);
u32 alink_get_swptr(void *hw_channel);			//iis获取通道swptr
void alink_set_swptr(void *hw_channel, u32 value);		//iis设置通道swptr
u32 alink_get_hwptr(void *hw_channel);			//iis获取通道hwptr
void alink_set_hwptr(void *hw_channel, u32 value);	//iis设置通道hwptr
void alink_set_ch_ie(void *hw_channel, u32 value);	//iis设置通道ie
void alink_clr_ch_pnd(void *hw_channel);			//iis清除通道pnd
int alink_get_ch_ie(void *hw_channel);
void alink_set_da2sync_ch(void *hw_alink);
void audio_alink_lock(u8 module_idx);
void audio_alink_unlock(u8 module_idx);
u32 alink_timestamp(u32 module_idx, u8 ch_idx);
void alink_sw_mute(void *hw_channel, u8 mute);

void alink0_info_dump();
void alink1_info_dump();
void alink_set_rx_mode(void *hw_alink, void *hw_channel);
void alink_release_rx_mode(void *hw_alink, void *hw_channel);
void alink_dma_free(void *hw_alink, void *hw_channel);
void alink_set_mrx_mode(void *hw_alink, void *hw_channel, u8 miis);
void alink_release_mrx_mode(void *hw_alink, u8 miis);
u32 alink_ch_mode(u32 alink_work_mode);
u32 alink_work_mode(u32 alink_work_mode);

//只支持iis base ch0直通dac,可配置界面选配dma是否使能
int audio_alink2dac_open(struct alink2da_param *param);
int audio_alink2dac_close(u8 module_idx);

//只支持dac直通iis base ch1，配置界面使能"DAC Alink DMA Shared En"
int audio_dac2alink_open(struct da2alink_param *param);
int audio_dac2alink_close(u8 module_idx);
#define AUDIO_IIS_AT_RAM	__attribute__((section(".audio_iis_ram_code.text.cache.L1")))

#endif/*_AUDIO_LINK_H_*/
