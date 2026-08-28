#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_alink_demo.data.bss")
#pragma data_seg(".audio_alink_demo.data")
#pragma const_seg(".audio_alink_demo.text.const")
#pragma code_seg(".audio_alink_demo.text")
#endif
/*
 ****************************************************************************
 *							Audio ALINK Demo
 *
 *Description	: Audio ALINK使用范例，AudioLink是一个通用的双声道音频接口，
 *				  连接信号有MCLK,SCLK,LRCK,DATA,支持IIS模式。
 *Notes			: 本demo为开发测试范例，请不要修改该demo， 如有需求，请自行
 *				  复制再修改
 ****************************************************************************
 */
#include "asm/audio_link.h"
#include "media/includes.h"
#include "media/audio_iis.h"
#include "audio_config.h"
#include "audio_general_config.h"

#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
/*
 * IIS与DAC的硬件直通功能
 * 建议固定全局采样率 TCFG_AUDIO_GLOBAL_SAMPLE_RATE
 */
extern struct audio_dac_hdl dac_hdl;

//IIS_RX->DAC
void alink_rx_2_dac_open()
{
    struct alink2da_param param = {0};
    param.sr          = TCFG_AUDIO_GLOBAL_SAMPLE_RATE ;
    param.dma_time_ms = TCFG_AUDIO_DAC_BUFFER_TIME_MS;
    param.module_idx  = 0;
    param.bit_width   = audio_general_in_dev_bit_width();
    param.irq_points  = AUDIO_IIS_IRQ_POINTS;
    param.clk_close   = TCFG_AUDIO_IIS_CLOCK_CLOSE;
    param.ch_num     = IIS_CH_NUM;
    //cppcheck-suppress knownConditionTrueFalse
    param.clk_mode   = param.module_idx ? ALINK1_CLK_MODE : ALINK0_CLK_MODE;
    param.alink_work_mode = IIS_BASE;
    param.merge_param.ch0_data0 = IIS_L_DATA;
    param.merge_param.ch1_data0 = IIS_R_DATA;
    audio_alink2dac_open(&param);
}
void alink_rx_2_dac_close()
{
    audio_alink2dac_close(0);
}

//DAC->IIS_TX
void dac_2_alink_tx_open()
{
    struct da2alink_param param = {0};
    param.module_idx = 0;
    param.bit_width = audio_general_in_dev_bit_width();
    param.clk_close = TCFG_AUDIO_IIS_CLOCK_CLOSE;
    param.irq_points = AUDIO_IIS_IRQ_POINTS;
    param.sr = TCFG_AUDIO_GLOBAL_SAMPLE_RATE;
    param.dma_time_ms = TCFG_AUDIO_DAC_BUFFER_TIME_MS;
    param.ch_num     = IIS_CH_NUM;
    //cppcheck-suppress knownConditionTrueFalse
    param.clk_mode   = param.module_idx ? ALINK1_CLK_MODE : ALINK0_CLK_MODE;
    param.alink_work_mode = IIS_BASE;
    audio_dac2alink_open(&param);
}
void dac_2_alink_tx_close()
{
    audio_dac2alink_close(0);
}

/*
 * IIS_RX->DAC->IIS_TX
 * dac_dma_en: DAC是否跑数据流写DMA
 */
void alink_rx_2_alink_tx_open(u8 dac_dma_en)
{
    if (!dac_dma_en) {
        //不跑数据流，DAC没使能的情况下，需要手动开启DAC
        audio_dac_set_sample_rate(&dac_hdl, TCFG_AUDIO_GLOBAL_SAMPLE_RATE);
        audio_dac_start(&dac_hdl);
        audio_dac_channel_start(NULL);
    }
    alink_rx_2_dac_open();
    dac_2_alink_tx_open();
}

void alink_rx_2_alink_tx_close(u8 dac_dma_en)
{
    alink_rx_2_dac_close();
    dac_2_alink_tx_close();
    if (!dac_dma_en) {
        audio_dac_stop(&dac_hdl);
        audio_dac_channel_close(NULL);
    }
}

#endif

#if TCFG_IIS_NODE_ENABLE || TCFG_IIS_RX_NODE_ENABLE

//模块总句柄
static void *alink0_hdl = NULL;
//每个通道的操作句柄
static void *alink0_ch0 = NULL;
static void *alink0_ch1 = NULL;
static void *alink0_ch2 = NULL;
static void *alink0_ch3 = NULL;


/*
 *每个通道的中断回调函数，可在函数内获取输入输出buf的数据,地址buf, 长度len(byte)
 * */
static void ch0_tx_handle(void *priv, void *buf, int len);
static void ch1_rx_handle(void *priv, void *buf, int len);
static void ch2_rx_handle(void *priv, void *buf, int len);
static void ch3_tx_handle(void *priv, void *buf, int len);


/*
 *总模块共用的信息配置
 * */
#if !ALINK_MODE_SEL
//适用cpu:BR29 BR27 BR28 BR36
ALINK_PARM	alink0_demo = {
    //模块参数配置
#if defined(CONFIG_CPU_BR29)
    .module = 0,//port_a, br29 alink端口io只有固定的两组
    .mclk_io = IO_PORTA_02,
    .sclk_io = IO_PORTA_03,
    .lrclk_io = IO_PORTA_04,
#else
    .module = ALINK0,
    .mclk_io = IO_PORTC_00,
    .sclk_io = IO_PORTC_01,
    .lrclk_io = IO_PORTC_02,
#endif
    .mode = ALINK_MD_IIS,
    .role = ALINK_ROLE_MASTER,
    .clk_mode = ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE,
    .bitwide = ALINK_LEN_16BIT,
    .sclk_per_frame = ALINK_FRAME_32SCLK,
    .dma_len = 256 * 2 * 2 * 2,
    .sample_rate = ALINK_SR_48000,
    .buf_mode = ALINK_BUF_DUAL,
};

#else

ALINK_PARM	alink0_demo = {
    //模块参数配置
    .module = ALINK0,
    .mclk_io = IO_PORTC_00,
    .sclk_io = IO_PORTC_01,
    .lrclk_io = IO_PORTC_02,
    .mode = ALINK_MD_BASIC,
    .role = ALINK_ROLE_MASTER,
    .clk_mode = ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE,
    .bitwide = ALINK_BW_16BIT,
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
#else
    .dma_len = 256 * 2 * 2 * 2,
#endif
    .sample_rate = ALINK_SR_48000,
    .buf_mode = ALINK_BUF_DUAL,
    .slot_num = 2,
};

#endif


/*
 *通道信息配置
 * */
struct alnk_hw_ch hw_ch0_tx_cfg = {
    .cfg = {
        .ch_idx = 0,
        .module = ALINK0,
#if defined(CONFIG_CPU_BR29)
        .data_io = IO_PORTA_00,
#else
        .data_io = IO_PORTC_03,
#endif
        .ch_ie = 1,
        .dir = ALINK_DIR_TX,
        .isr_cb = ch0_tx_handle,
        .priv = NULL,
#if ALINK_MODE_SEL
        .ch_mode = ALINK_CH_MD_BASIC_IIS,
#endif
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
        .adv_cfg = {
            .dma_en = 1,
        },
#endif
    },
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
    .dma_len = 256 * 2 * 2 * 2,
#endif
};

struct alnk_hw_ch hw_ch1_rx_cfg = {

    .cfg = {
        .ch_idx = 1,
        .module = ALINK0,
#if defined(CONFIG_CPU_BR29)
        .data_io = IO_PORTA_01,
#else
        .data_io = IO_PORTC_04,
#endif
        .ch_ie = 1,
        .dir = ALINK_DIR_RX,
        .isr_cb = ch1_rx_handle,
        .priv = NULL,
#if ALINK_MODE_SEL
        .ch_mode = ALINK_CH_MD_BASIC_IIS,
#endif
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
        .adv_cfg = {
            .dma_en = 1,
        },
#endif

    },
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
    .dma_len = 256 * 2 * 2 * 2,
#endif
};

struct alnk_hw_ch hw_ch2_rx_cfg = {

    .cfg = {
        .ch_idx = 2,
        .module = ALINK0,

#if defined(CONFIG_CPU_BR29)
        .data_io = IO_PORTA_05,
#else
        .data_io = IO_PORTA_04,
#endif
        .ch_ie = 1,
        .dir = ALINK_DIR_RX,
        .isr_cb = ch2_rx_handle,
        .priv = NULL,
#if ALINK_MODE_SEL
        .ch_mode = ALINK_CH_MD_BASIC_IIS,
#endif
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
        .adv_cfg = {
            .dma_en = 1,
        },
#endif

    },
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
    .dma_len = 256 * 2 * 2 * 2,
#endif
};

struct alnk_hw_ch hw_ch3_tx_cfg = {

    .cfg = {
        .ch_idx = 3,
        .module = ALINK0,

#if defined(CONFIG_CPU_BR29)
        .data_io = IO_PORTA_06,
#else
        .data_io = IO_PORTA_05,
#endif
        .ch_ie = 1,
        .dir = ALINK_DIR_TX,
        .isr_cb = ch3_tx_handle,
        .priv = NULL,
#if ALINK_MODE_SEL
        .ch_mode = ALINK_CH_MD_BASIC_IIS,
#endif
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
        .adv_cfg = {
            .dma_en = 1,
        },
#endif

    },
#if defined(ALINK_VERSION_01) && ALINK_VERSION_01
    .dma_len = 256 * 2 * 2 * 2,
#endif
};


static void ch0_tx_handle(void *priv, void *buf, int len)
{
    putchar('0');
    //往buf填数据,填len的长度
}
static void ch1_rx_handle(void *priv, void *buf, int len)
{
    putchar('1');
    //从buf读数据,读len的长度
}
static void ch2_rx_handle(void *priv, void *buf, int len)
{
    putchar('2');
    //从buf读数据,读len的长度
}
static void ch3_tx_handle(void *priv, void *buf, int len)
{
    putchar('3');
    //往buf填数据,填len的长度
}


void alink_channel_dump(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    r_printf("hw_channel_parm->data_io = %d\n", hw_channel_parm->cfg.data_io);
    r_printf("hw_channel_parm->ch_ie = %d\n", hw_channel_parm->cfg.ch_ie);
    r_printf("hw_channel_parm->dir = %d\n", hw_channel_parm->cfg.dir);
    r_printf("hw_channel_parm->isr_cb = 0x%x\n", hw_channel_parm->cfg.isr_cb);
    r_printf("hw_channel_parm->buf = 0x%x\n", hw_channel_parm->buf);
    r_printf("hw_channel_parm->module = %d\n", hw_channel_parm->cfg.module);
    r_printf("hw_channel_parm->ch_idx = %d\n", hw_channel_parm->cfg.ch_idx);
    r_printf("hw_channel_parm->ch_en = %d\n", hw_channel_parm->ch_en);
}


/*
 *模块打开
 * */
void audio_link_open_demo(void)
{

    alink0_hdl = alink_init(&alink0_demo);				//模块初始化

    alink0_ch0 = alink_channel_init(alink0_hdl, &hw_ch0_tx_cfg);	//通道0初始化 tx
    alink0_ch1 = alink_channel_init(alink0_hdl, &hw_ch1_rx_cfg);	//通道1初始化 rx
    alink0_ch2 = alink_channel_init(alink0_hdl, &hw_ch2_rx_cfg);	//通道2初始化 rx
    alink0_ch3 = alink_channel_init(alink0_hdl, &hw_ch3_tx_cfg);	//通道3初始化 tx

    alink_start(alink0_hdl);							//模块打开

    alink_channel_dump(alink0_ch0);
    alink_channel_dump(alink0_ch1);
    alink_channel_dump(alink0_ch2);
    alink_channel_dump(alink0_ch3);

    void alink0_info_dump();
    alink0_info_dump();
}

/*
 *模块关闭
 * */
void audio_link_close_demo(void)
{
    alink_channel_close(alink0_hdl, alink0_ch0);		//通道0关闭
    alink_channel_close(alink0_hdl, alink0_ch1);		//通道1关闭
    alink_channel_close(alink0_hdl, alink0_ch2);		//通道2关闭
    alink_channel_close(alink0_hdl, alink0_ch3);		//通道3关闭

    alink_uninit(alink0_hdl, NULL);	//模块关闭
    os_time_dly(100);

    void alink0_info_dump();
    alink0_info_dump();
}

#endif

