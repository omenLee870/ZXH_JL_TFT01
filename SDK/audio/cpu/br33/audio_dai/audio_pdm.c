#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_pdm.data.bss")
#pragma data_seg(".audio_pdm.data")
#pragma const_seg(".audio_pdm.text.const")
#pragma code_seg(".audio_pdm.text")
#endif
#include "audio_pdm.h"
#include "includes.h"
#include "cpu/includes.h"
#include "asm/dac.h"
#include "media/includes.h"
#include "math.h"
#include "gpio.h"
#include "audio_splicing.h"

#define PLNK_DEBUG_ENABLE

#ifdef PLNK_DEBUG_ENABLE
#define plnk_printf 		printf
#else
#define plnk_printf(...)
#endif

static PLNK_PARM *__this = NULL;
#define PLNK_CLK	24000000

/*00:3order,01:2order,1x:1order*/
/* const u8 plnk_cic_order_reg[3] = {2, 1, 0}; */
#define PLNK_CIC_ORDER	3







#define     CIC_ORDER    3      // cic order(stage)
#define     CIC_DDM      1      // cic differential delay factor

static void plnk_set_shift_scale()
{
    volatile unsigned char  cic_m;      // cic differential delay factor
    volatile unsigned char  cic_order;  // cic order(stage)
    u8  delta_bit;
    u16 delta;
    u32 gain;
    u8  gain_bit;
    u8  shdir;
    u8  shift_bit;
    u16 shift_gain;     // 2^shift_bit
    s8  imax;           // input max value(abs)
    s32 gain_value;
    s32 gain_value0;
    float scale_f;
    u8  scale_sel;


    u32 SR = __this->sr / 1000;

    volatile unsigned char  sr_32k_group_en;
    volatile unsigned char  sr_44k_group_en;
    volatile unsigned char  sr_48k_group_en;
    volatile unsigned char  dmic_div = 0;
    volatile unsigned long  dmic_clk_freq = 0;

    sr_32k_group_en = (SR == 8) || (SR == 16) || (SR == 32) || (SR == 64) || (SR == 128); // not support 128k !!!
    sr_44k_group_en = (SR == 11) || (SR == 22) || (SR == 44) || (SR == 88) || (SR == 176);
    sr_48k_group_en = (SR == 12) || (SR == 24) || (SR == 48) || (SR == 96) || (SR == 192);

    if (sr_32k_group_en) {
        dmic_div = 15;    // 24M / 15 = 1.6M
        dmic_clk_freq = 1600000;
    }
    if (sr_44k_group_en) {
        dmic_div =  8;    // 24M /  8 = 3M
        dmic_clk_freq = 3000000;
    }
    if (sr_48k_group_en) {
        dmic_div =  5;    // 24M /  5 = 4.8M
        dmic_clk_freq = 4800000;
    }
    //printf("\n    sr_32k_group_en=%d\t, sr_44k_group_en=%d\t, sr_48k_group_en=%d\t, dmic_div=%d\n", sr_32k_group_en, sr_44k_group_en, sr_48k_group_en, dmic_div);
    //printf("\n    sample_rate=%d kHz\t, dmic_clk=%d Hz\n", SR, (int)dmic_clk_freq);

    u16 cic_r = (unsigned short)(dmic_clk_freq / SR / 1000);
    if (SR == 176)    {
        cic_r =  17;
    } else if (SR == 88)    {
        cic_r =  34;
    } else if (SR == 44)    {
        cic_r =  68;
    } else if (SR == 22)    {
        cic_r = 136;
    } else if (SR == 11)    {
        cic_r = 272;
    }


    // ... adc dmic clk enable ...
    SFR(JL_AUD->AUD_CON0, 24,  1,  1);              // adc_dmic_cken
    SFR(JL_AUD->AUD_CON0, 25,  5, (dmic_div - 1));  // adc_dmic_div, 1~64, 24m/(div+1)

    cic_order = CIC_ORDER;
    cic_m     = CIC_DDM;
    delta_bit = 16 - 2;     // output width - input width

    // ... shift  & scale ...
    // step_1:
    delta     = powf(2, delta_bit);
    gain      = powf(cic_r * cic_m, cic_order);
    gain_bit  = (u8)ceilf((float)log2(gain));
    if (gain < delta) {
        shdir     = 1;
        shift_bit = (delta_bit - gain_bit);
    } else {
        shdir     = 0;
        shift_bit = (gain_bit - delta_bit);
    }

    // step_2:
    imax       = 1;
    shift_gain = pow(2, shift_bit);
    if (shdir == 0) {
        gain_value0 = (s32)round(imax * gain / shift_gain);
    } else {
        gain_value0 = (s32)round(imax * gain * shift_gain);
    }
    gain_value = gain_value0;
    while (1) {
        gain_value = gain_value * 2;
        if (gain_value > 32767) {
            gain_value = gain_value / 2;
            break;
        }
        if (shdir == 0) {
            shift_bit = shift_bit - 1;
        } else {
            shift_bit = shift_bit + 1;
        }
    }

    // step_3:
    scale_f    = ((float)32767 / (float)gain_value) - 1;
    scale_sel  = (u8)floor((float)(scale_f / 0.125));
    //printf("\n    pre: shift_bit=%d\t, gain_value=%d\t, scale_sel=%d\n", shift_bit, gain_value, scale_sel);
    if (gain_value < 32767) {
        while (scale_sel > 7) {
            if (shdir == 0) {
                shift_bit = shift_bit - 1;
                gain_value = gain_value * 2;
            } else {
                shift_bit = shift_bit + 1;
                gain_value = gain_value * 2;
            }
            if (gain_value > 32767) {
                shift_bit = shift_bit + 1;
                gain_value = gain_value / 2;
            }
            scale_sel = scale_sel - 8;
        }
    } else {
        /* shift_bit = shift_bit; */
        /* scale_sel = scale_sel; */
    }
    /* shdir = 1; */
    /* shift_bit = 0;//2; */
    /* scale_sel = 0; */
    PLNK_CH0_SHDIR(shdir);
    PLNK_CH0_DFDLY(cic_m - 1);
    PLNK_CH0_ORDER(3 - cic_order);
    PLNK_CH0_SHIFT(shift_bit);//增益粗调 ,n倍
    PLNK_CH0_SCALE(scale_sel);//增益细调, 2为倍以下用，步进0.125倍

    if (__this->ch_cfg[1].en) {
        PLNK_CH1_SHDIR(shdir);
        PLNK_CH1_DFDLY(cic_m - 1);
        PLNK_CH1_ORDER(3 - cic_order);
        PLNK_CH1_SHIFT(shift_bit);//增益粗调 ,n倍
        PLNK_CH1_SCALE(scale_sel);//增益细调, 2为倍以下用，步进0.125倍
    }

    PLNK_DOR_SET(cic_r - 1);
}


___interrupt
static void plnk_isr(void)
{
    if (!__this) {
        return;
    }
    u32 *buf = (u32 *)__this->buf;
    u32 *ch0_base;
    u32 *ch1_base;
    u32 len = __this->dma_len / 4;
    if (PLNK_PND_IS()) {
        PLNK_PND_CLR();
        u8 buf_flag = PLNK_USING_BUF() ? 0 : 1; // buf_flag = 0代表buffer0可被读写，buf_flag=1代表buffer1可被读写
        ch0_base = buf + buf_flag * len;
        ch1_base = buf + len * 2 + buf_flag * len;
        if (__this->ch_num == 1) {
            if (__this->isr_cb) {
                __this->isr_cb(__this->private_data, __this->ch_cfg[0].en ? ch0_base : ch1_base, __this->dma_len);
            }
        } else {
            if (__this->isr_cb) {
                pcm_single_l_r_2_dual(__this->stereo_buf, ch0_base, ch1_base, __this->dma_len);
                __this->isr_cb(__this->private_data, __this->stereo_buf, __this->dma_len);
            }
        }
    }

}

static void plnk_info_dump()
{
    plnk_printf("PLNK_CON = 0x%x", JL_PLNK->CON);
    plnk_printf("PLNK_CON1 = 0x%x", JL_PLNK->CON1);
    plnk_printf("PLNK_CON2 = 0x%x", JL_PLNK->CON2);
    plnk_printf("PLNK_LEN = 0x%x", JL_PLNK->LEN);
    plnk_printf("PLNK_ADR = 0x%x", JL_PLNK->ADR);
    plnk_printf("PLNK_DOR = 0x%x", JL_PLNK->DOR);
    plnk_printf("JL_AUD->AUD_CON0 = 0x%x", JL_AUD->AUD_CON0);
    /* plnk_printf("CLK_CON4 = 0x%x", JL_CLOCK->CLK_CON4); */
}

#define gpio_test 0
static void plnk_sclk_io_init(u8 port)
{
#if !gpio_test
    /* port = IO_PORTA_02; */
    gpio_och_sel_output_signal(port, OUTPUT_CH_SIGNAL_ASS_DMIC_CLK);
    gpio_set_mode(IO_PORT_SPILT(port), PORT_OUTPUT_LOW);
#else
    /* ----------------------plnk_clk--------------------- */
    SFR(JL_IOMC->OCH_IOMC0, 0, 6, 36);
    JL_OMAP->PA2_OUT = FO_GP_OCH0;
    SFR(JL_PORTA->DIR,  2,  1,  0);
#endif
}

static void plnk_data_io_init(u8 ch_index, u8 port)
{
#if !gpio_test
    /* port = IO_PORTA_03; */
    if (!ch_index) {
        gpio_ich_sel_input_signal(port, INPUT_CH_SIGNAL_ASS_DMIC_IDAT0, INPUT_CH_TYPE_GP_ICH);
    } else {
        gpio_ich_sel_input_signal(port, INPUT_CH_SIGNAL_ASS_DMIC_IDAT1, INPUT_CH_TYPE_GP_ICH);
    }
    gpio_set_mode(IO_PORT_SPILT(port), PORT_INPUT_FLOATING);
#else
    JL_IMAP->FI_GP_ICH0 = PA3_IN;
    SFR(JL_IOMC->ICH_IOMC3, 10, 3, 0);
    SFR(JL_PORTA->DIR,  3,  1,  1);
    SFR(JL_PORTA->DIE,  3,  1,  1);
#endif
}

void *plnk_init(void *hw_plink)
{
    if (hw_plink == NULL) {
        return NULL;
    }
    __this = (PLNK_PARM *)hw_plink;

    __this->ch_num = __this->ch_cfg[0].en + __this->ch_cfg[1].en;

    PLNK_RESET();

    PLNK_DMA_LEN(__this->dma_len / 2);
    __this->buf = dma_malloc(__this->dma_len  * 2 * __this->ch_num);
    if (!__this->buf) {
        return NULL;
    }
    memset(__this->buf, 0x00, __this->dma_len  * 2 * __this->ch_num);
    if (__this->ch_num > 1) {
        __this->stereo_buf = dma_malloc(__this->dma_len  * __this->ch_num);
        if (!__this->stereo_buf) {
            dma_free(__this->buf);
            return NULL;
        }
    }
    PLNK_BUF_SET((u32)__this->buf);

    plnk_set_shift_scale();

    PLNK_CH0_MD(__this->ch_cfg[0].mode);
    PLNK_CH0_MIC_SEL(__this->ch_cfg[0].mic_type);

    if (__this->ch_cfg[1].en) {
        PLNK_CH1_MD(__this->ch_cfg[1].mode);
        PLNK_CH1_MIC_SEL(__this->ch_cfg[1].mic_type);
    }


// .........................PLNK_CON2......................
    PLNK_DCC_BYPASS_EN(1);             // dcc  bypass enable       0: no bypass    1: bypass dcc

    u8 dcc_en = 1;
    u8 dcc_lvl = 14;
    u8 dcc_ctl = 0;
    PLNK_CH0_DCC_EN(dcc_en);             // dcc0 enable
    PLNK_CH0_DCC_LVL(dcc_lvl);             // dcc0 lvl, 15: bypass
    PLNK_CH0_DCC_CTL(dcc_ctl);             // dcc0 soft_ctl

    if (__this->data_cfg[1].en) {
        PLNK_CH1_DCC_EN(dcc_en);             // dcc1 enable
        PLNK_CH1_DCC_LVL(dcc_lvl);             // dcc1 lvl, 15: bypass
        PLNK_CH1_DCC_CTL(dcc_ctl);             // dcc1 soft_ctl
    }

    plnk_sclk_io_init(__this->sclk_io);

    if ((__this->data_cfg[0].en && (__this->ch_cfg[0].mode <= DATA0_SCLK_FALLING_EDGE))
        || (__this->data_cfg[1].en && (__this->ch_cfg[1].mode <= DATA1_SCLK_FALLING_EDGE))) {
        if (__this->data_cfg[0].io != 0xff) {
            plnk_data_io_init(0, __this->data_cfg[0].io);
        }
    }

    if ((__this->data_cfg[0].en && (__this->ch_cfg[0].mode >= DATA0_SCLK_RISING_EDGE))
        || (__this->data_cfg[1].en && (__this->ch_cfg[1].mode >= DATA1_SCLK_RISING_EDGE))) {
        if (__this->data_cfg[1].io != 0xff) {
            plnk_data_io_init(1, __this->data_cfg[1].io);
        }
    }

    PLNK_CLK_EN(1);
    PLNK_PND_CLR();

    request_irq(IRQ_PLNK_IDX, 3, plnk_isr, 0);

    return __this;
}

void plnk_start(void *hw_plink)
{
    if (!hw_plink) {
        return;
    }

    PLNK_CH0_EN(__this->ch_cfg[0].en);
    PLNK_CH1_EN(__this->ch_cfg[1].en);
    PLNK_PND_CLR();
    PLNK_IE(1);
    plnk_info_dump();
}

void plnk_stop(void *hw_plink)
{
    if (!hw_plink) {
        return;
    }

    PLNK_PND_CLR();
    PLNK_IE(0);
    PLNK_CH0_EN(0);
    PLNK_CH1_EN(0);
    plnk_info_dump();
}

void plnk_uninit(void *hw_plink)
{
    if (!hw_plink) {
        return;
    }

    PLNK_PND_CLR();
    PLNK_RESET();
    unrequest_irq(IRQ_PLNK_IDX);

    gpio_och_disable_output_signal(__this->sclk_io, OUTPUT_CH_SIGNAL_ASS_DMIC_CLK);
    if ((__this->data_cfg[0].en && (__this->ch_cfg[0].mode <= DATA0_SCLK_FALLING_EDGE))
        || (__this->data_cfg[1].en && (__this->ch_cfg[1].mode <= DATA1_SCLK_FALLING_EDGE))) {
        if (__this->data_cfg[0].io != 0xff) {
            gpio_ich_disable_input_signal(__this->data_cfg[0].io, INPUT_CH_SIGNAL_ASS_DMIC_IDAT0, INPUT_CH_TYPE_GP_ICH);
        }
    }
    if ((__this->data_cfg[0].en && (__this->ch_cfg[0].mode >= DATA0_SCLK_RISING_EDGE))
        || (__this->data_cfg[1].en && (__this->ch_cfg[1].mode >= DATA1_SCLK_RISING_EDGE))) {

        if (__this->data_cfg[1].io != 0xff) {
            gpio_ich_disable_input_signal(__this->data_cfg[1].io, INPUT_CH_SIGNAL_ASS_DMIC_IDAT1, INPUT_CH_TYPE_GP_ICH);
        }
    }

    if (__this->buf) {
        dma_free(__this->buf);
        __this->buf = NULL;
    }
    if (__this->stereo_buf) {
        dma_free(__this->stereo_buf);
        __this->stereo_buf = NULL;
    }

    __this = NULL;
}



