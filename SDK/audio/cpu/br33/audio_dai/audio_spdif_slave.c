#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_spdif_slave.data.bss")
#pragma data_seg(".audio_spdif_slave.data")
#pragma const_seg(".audio_spdif_slave.text.const")
#pragma code_seg(".audio_spdif_slave.text")
#endif
#include "includes.h"
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_spdif_slave.h"

#define SPDIF_DEBUG_INFO
#ifdef SPDIF_DEBUG_INFO
#define spdif_printf  printf
#else
#define spdif_printf(...)
#endif

#define SPDIF_SLAVE_TEST		0
static SPDIF_SLAVE_PARM *spdif_slave_parm = NULL;

static u32 PFI_SPDIF_DAT[] = {
    INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIA,
    INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIB,
    INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIC,
    INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DID,
};
static u8 spdif_analog_port[] = {
    IO_PORTB_02,
    IO_PORTB_02,
    IO_PORTB_02,
    IO_PORTB_02,
};

static void spdif_slave_info_dump()
{
    spdif_printf("JL_SPDIF->SS_CON = 0x%x\n", JL_SPDIF->SS_CON);
    spdif_printf("JL_SPDIF->SS_IO_CON = 0x%x\n", JL_SPDIF->SS_IO_CON);
    spdif_printf("JL_SPDIF->SS_DMA_CON = 0x%x\n", JL_SPDIF->SS_DMA_CON);
    spdif_printf("JL_SPDIF->SS_DMA_LEN = 0x%x\n", JL_SPDIF->SS_DMA_LEN);
    spdif_printf("JL_SPDIF->SS_DAT_ADR = 0x%x\n", JL_SPDIF->SS_DAT_ADR);
    spdif_printf("JL_SPDIF->SS_INF_ADR = 0x%x\n", JL_SPDIF->SS_INF_ADR);
    /* spdif_printf("JL_CLOCK->CLK_CON5[10:8] = 0x%x\n", (JL_CLOCK->CLK_CON5 >> 8) & 0b111); */
    /* spdif_printf("JL_IMAP->FI_SPDIF_DIA = 0x%x\n", JL_IMAP->FI_SPDIF_DIA); */
}

sec(.volatile_ram_code)
static void *spdif_slave_data_addr()
{
    u8 buf_index = 0;
    u8 *buf_addr = NULL;

    buf_addr = (u8 *)(spdif_slave_parm->data_buf);
    buf_index = (JL_SPDIF->SS_CON & BIT(12)) ? 0 : 1;
    buf_addr = buf_addr + ((spdif_slave_parm->data_dma_len / 2) * buf_index);

    return buf_addr;
}

sec(.volatile_ram_code)
static void *spdif_slave_inf_addr()
{
    u8 buf_index = 0;
    u8 *buf_addr = NULL;

    buf_addr = (u8 *)(spdif_slave_parm->inf_buf);
    buf_index = (JL_SPDIF->SS_CON & BIT(13)) ? 0 : 1;
    buf_addr = buf_addr + ((spdif_slave_parm->inf_dma_len / 2) * buf_index);

    return buf_addr;
}

u32 spdif_slave_get_sr()
{
    u32 lsb_clk;
    lsb_clk = clk_get("lsb");

    u32 spdif_sr_cnt;
    spdif_sr_cnt = JL_SPDIF->SS_SR_CNT;

    if (!spdif_sr_cnt) {
        return 0;
    }

    u64 spdif_slave_sr;
    if (spdif_slave_parm->data_mode) {
        spdif_slave_sr = (u64)lsb_clk * (spdif_slave_parm->data_dma_len / 8) / spdif_sr_cnt;
    } else {
        spdif_slave_sr = (u64)lsb_clk * (spdif_slave_parm->data_dma_len / 16) / spdif_sr_cnt;
    }

    /* spdif_printf("spdif_slave_sr = %d\n", spdif_slave_sr); */
    return (u32)spdif_slave_sr;
}

___interrupt
static void spdif_slave_isr()
{
    /* putchar('k'); */
    if (ERROR_VALUE()) {
        u8 is_d_err = 0;
        if (F_ERR()) {
            /* putchar('F'); */
        }
        if (B_ERR()) {
            /* putchar('B'); */
        }
        if (D_ERR()) {
            is_d_err = 1;
            /* putchar('D'); */
        }
        if (RL_ERR()) {
            /* putchar('R'); */
        }

        ERR_ALL_CLR();
        if (!is_d_err) {
            /* SS_START(1); */
        }
    }

    if (INFO_PND()) {
        INFO_PND_CLR();
        s8 *inf_buf_addr = NULL;
        inf_buf_addr = spdif_slave_inf_addr();
        if (spdif_slave_parm->inf_isr_cb) {
            spdif_slave_parm->inf_isr_cb(inf_buf_addr, spdif_slave_parm->inf_dma_len / 2);
        }
    }

    if (DATA_PND()) {
        DATA_PND_CLR();
        /* putchar('a'); */
        s8 *data_buf_addr = NULL;
        data_buf_addr = spdif_slave_data_addr();
        if (spdif_slave_parm->data_isr_cb) {
            spdif_slave_parm->data_isr_cb(data_buf_addr, spdif_slave_parm->data_dma_len / 2);
        }
    }


    if (CSBR_PND()) {
        CSBR_PND_CLR();
    }

    if (IS_PND()) {
        /* putchar('i'); */
        IS_PND_CLR();
    }

    if (ET_PND()) {
        /* putchar('e'); */
        ET_PND_CLR();
    }
}

static void spdif_slave_digital_io_init(u8 gpio)
{
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_INPUT_FLOATING);
}

static void spdif_slave_analog_io_init(u8 port)
{
    gpio_set_mode(IO_PORT_SPILT(port), PORT_HIGHZ);
}

void *spdif_slave_channel_init(void *hw_spdif_slave, u8 ch_idx)
{
    if (hw_spdif_slave == NULL) {
        return NULL;
    }


    SPDIF_SLAVE_PARM *spdif_slave_hdl = (SPDIF_SLAVE_PARM *)hw_spdif_slave;
    IS_PND_CLR();
    ET_PND_CLR();
    IS_PND_IE(0);
    ET_PND_IE(0);
    IO_IN_EN(0);

    ch_idx = 0;
    u8 port_sel = spdif_slave_hdl->ch_cfg[ch_idx].port_sel = SPDIF_IN_DIGITAL_PORT_A;

    if (port_sel <= SPDIF_IN_DIGITAL_PORT_D) {
        IO_IS_DEN(ch_idx, spdif_slave_hdl->ch_cfg[ch_idx].is_den);
        IO_ET_DEN(ch_idx, spdif_slave_hdl->ch_cfg[ch_idx].et_den);

        IO_IN_SEL(ch_idx);
        spdif_slave_hdl->ch_cfg[ch_idx].data_io = IO_PORTB_02;
        spdif_slave_digital_io_init(spdif_slave_hdl->ch_cfg[ch_idx].data_io);
        /* gpio_set_fun_input_port(spdif_slave_hdl->ch_cfg[ch_idx].data_io, PFI_SPDIF_DAT[ch_idx]); */
        gpio_ich_sel_input_signal(spdif_slave_hdl->ch_cfg[ch_idx].data_io, PFI_SPDIF_DAT[ch_idx], INPUT_CH_TYPE_GP_ICH);

    } else if (port_sel >= SPDIF_IN_ANALOG_PORT_A && port_sel <= SPDIF_IN_ANALOG_PORT_D) {
        port_sel = port_sel - SPDIF_IN_ANALOG_PORT_A;
        spdif_slave_analog_io_init(spdif_analog_port[port_sel]);

        IO_IS_DEN(0, spdif_slave_hdl->ch_cfg[ch_idx].is_den);
        IO_ET_DEN(0, spdif_slave_hdl->ch_cfg[ch_idx].et_den);

        IO_IN_ANA_SEL(port_sel);
        IO_IN_ANA_EN(1);
        IO_IN_ANA_MUX(1);

        IO_IN_SEL(0);
    }

    return &spdif_slave_hdl->ch_cfg[ch_idx];
}

int spdif_slave_channel_open(void *hw_spdif_slave_channel)
{
    if (hw_spdif_slave_channel == NULL) {
        return -1;
    }
    struct spdif_s_ch_cfg *spdif_slave_channel_hdl = (struct spdif_s_ch_cfg *)hw_spdif_slave_channel;

    IS_PND_CLR();
    ET_PND_CLR();
    IS_PND_IE(spdif_slave_channel_hdl->is_den);
    ET_PND_IE(spdif_slave_channel_hdl->et_den);
    IO_IN_EN(1);
    puts("============spdif_slave_channel_open\n");
    return 0;
}

int spdif_slave_channel_close(void *hw_spdif_slave_channel)
{
    if (hw_spdif_slave_channel == NULL) {
        return -1;
    }

    IS_PND_CLR();
    ET_PND_CLR();
    IS_PND_IE(0);
    ET_PND_IE(0);
    IO_IN_EN(0);
    return 0;
}


void *spdif_slave_init(void *hw_spdif_slave)
{
    if (hw_spdif_slave == NULL) {
        return NULL;
    }
    spdif_slave_parm = hw_spdif_slave;
    puts(">>>>>>>>>>>>>>>>>>>spdif_slave_init\n");

    SFR(JL_LSBCLK->PRP_CON1, 24, 3, 1);// d1po 192M
    SFR(JL_LSBCLK->PRP_CON1, 27, 1, 0);//
    SFR(JL_LSBCLK->PRP_CON1, 28, 2, 0);//
    SFR(JL_LSBCLK->PRP_CON1, 30, 2, 0);//


    SS_RESET();													//reset all con

    /* DATA_DMA_LEN(spdif_slave_parm->data_dma_len / 8); */
    INFO_DMA_LEN(spdif_slave_parm->inf_dma_len / 8);

    if (spdif_slave_parm->data_mode == SPDIF_S_DAT_24BIT) {
        DATA_DMA_LEN(spdif_slave_parm->data_dma_len / 16);
        spdif_slave_parm->data_buf = dma_malloc(spdif_slave_parm->data_dma_len);
    } else if (spdif_slave_parm->data_mode == SPDIF_S_DAT_16BIT) {
        DATA_DMA_LEN(spdif_slave_parm->data_dma_len / 8);
        spdif_slave_parm->data_buf = dma_malloc(spdif_slave_parm->data_dma_len);
    }
    memset(spdif_slave_parm->data_buf, 0, spdif_slave_parm->data_dma_len);
    DATA_DMA_MD(spdif_slave_parm->data_mode);

    spdif_slave_parm->inf_buf = dma_malloc(spdif_slave_parm->inf_dma_len);
    memset(spdif_slave_parm->inf_buf, 0, spdif_slave_parm->inf_dma_len);

    spdif_printf("data_buf = 0x%p, inf_buf = 0x%p\n", spdif_slave_parm->data_buf, spdif_slave_parm->inf_buf);

    if (spdif_slave_parm->data_buf) {
        JL_SPDIF->SS_DAT_ADR = (volatile unsigned int)spdif_slave_parm->data_buf;
    }

    if (spdif_slave_parm->inf_buf) {
        JL_SPDIF->SS_INF_ADR = (volatile unsigned int)spdif_slave_parm->inf_buf;
    }

    /* clk_set_api("spdif_slave", spdif_slave_parm->rx_clk); */
    SR_CHECK_UPGRADE(1);
    INFO_PND_IE(1);
    DATA_PND_IE(1);
    CSBR_PND_IE(1);

    F_ERR_IE(1);
    B_ERR_IE(1);
    D_ERR_IE(0);
    SE_ERR_IE(1);
    RLE_DECT_EN(1);

    DATA_PND_CLR();
    INFO_PND_CLR();
    ERR_ALL_CLR();
    IS_PND_CLR();
    ET_PND_CLR();
    CSBR_PND_CLR();

    RLE_CNT_SET(0xffff);
    SS_DET_MODE(1);
    FTL_TRACE(3);
    FTL_SEL(1);

    request_irq(IRQ_SPDIF_S_IDX, 3, spdif_slave_isr, 0);

    return spdif_slave_parm;
}

int spdif_slave_start(void *hw_spdif_slave)
{
    if (hw_spdif_slave == NULL) {
        return 0;
    }
    SPDIF_SLAVE_PARM *spdif_slave_hdl = (SPDIF_SLAVE_PARM *)hw_spdif_slave;
    if (spdif_slave_hdl->data_buf) {
        DATA_DMA_EN(1);
    }
    if (spdif_slave_hdl->inf_buf) {
        INFO_DMA_EN(1);
    }
    SS_EN(1);
    SS_START(1);
    spdif_printf("spdif_start\n");
    spdif_slave_info_dump();
    return 1;
}

int spdif_slave_stop(void *hw_spdif_slave)
{
    if (hw_spdif_slave == NULL) {
        return -1;
    }
    DATA_PND_CLR();
    INFO_PND_CLR();
    ERR_ALL_CLR();
    IS_PND_CLR();
    ET_PND_CLR();
    CSBR_PND_CLR();
    SS_EN(0);
    return 0;
}


int spdif_slave_uninit(void *hw_spdif_slave)
{
    if (hw_spdif_slave == NULL) {
        return -1;
    }
    SPDIF_SLAVE_PARM *spdif_slave_hdl = (SPDIF_SLAVE_PARM *)hw_spdif_slave;
    spdif_slave_stop(spdif_slave_hdl);

    INFO_PND_IE(0);
    DATA_PND_IE(0);
    CSBR_PND_IE(0);
    F_ERR_IE(0);
    B_ERR_IE(0);
    D_ERR_IE(0);
    SE_ERR_IE(0);
    RLE_DECT_EN(0);

    DATA_PND_CLR();
    INFO_PND_CLR();
    ERR_ALL_CLR();
    IS_PND_CLR();
    ET_PND_CLR();
    CSBR_PND_CLR();

    if (spdif_slave_hdl->data_buf) {
        dma_free(spdif_slave_hdl->data_buf);
        spdif_slave_hdl->data_buf = NULL;
    }

    if (spdif_slave_hdl->inf_buf) {
        dma_free(spdif_slave_hdl->inf_buf);
        spdif_slave_hdl->inf_buf = NULL;
    }
    spdif_slave_hdl = NULL;
    spdif_slave_parm = NULL;
    spdif_printf("spdif_slave_exit OK\n");
    return 0;
}

SPDIF_DAT_FORMAT spdif_slave_get_data_format()
{
    u8 ss_csb0 = (JL_SPDIF->SS_CSB0 & 0x3f);
    if (ss_csb0 & 0x2) {
        return SPDIF_S_DAT_NOT_PCM;
    } else {
        return SPDIF_S_DAT_PCM;
    }
    return SPDIF_S_DAT_UNKNOWN;
}


#define SPDIF_SLAVE_FPGA_DEBUG
#ifdef  SPDIF_SLAVE_FPGA_DEBUG
#define ENABLE_ORIGINAL_DATA_OUT 0 //24bit 时才能使用
// slaver buffer length
#define     INFO_LEN    48
/* #define     DATA_LEN    120 */
#define     DATA_LEN    192
volatile long info_buffer32[INFO_LEN * 2];
#define     RX_BIT16   0

#if RX_BIT16
#define BUF_TYPE   short
#define LEN_DOUBLE 2
volatile long data_buffer32[DATA_LEN * 2];
#else
#define BUF_TYPE   long
#define LEN_DOUBLE 1
/* volatile long data_buffer32[DATA_LEN*2]; */
volatile long data_buffer32[DATA_LEN * 2 * 2];
#endif

/* #define JL_TIMER5 JL_TIMER3  */
void spdif_dat_readout(short buf_len, BUF_TYPE *buffer)
{
    // chn=0 is left signal; chn=1 is right signal;
    long dump_dat;
    JL_TIMER5->CNT = (long) buffer;
    for (int chn = 0; chn < 2; chn ++) {
        JL_TIMER5->PWM = chn;
        for (int i = chn; i < buf_len; i = i + 2) {
            JL_TIMER5->PRD = (long) buffer[i];
        }
    }

}
static void delay(u32 cnt)
{
    while (cnt) {
        cnt--;
    }
}
___interrupt
void spdif_isr_s(void)
{
    long *infobuf_ptr;
    long *databuf_ptr;
    int  i, j, k, l;
    char f_err, b_err, d_err;

    /* putchar('i'); */
    if ((JL_SPDIF->SS_IO_CON & BIT(17)) && (JL_SPDIF->SS_DMA_CON & BIT(18))) {  // is_pnd & is_pns_ie
        SFR(JL_SPDIF->SS_CON,  1,  1,  0);                // ss_str
        SFR(JL_SPDIF->SS_IO_CON, 16,  1,  1);             // is_pnd_clr

        /* SFR(JL_SPDIF->SS_DMA_CON, 18,  1,  0);                // is interrupt enable,   0: disable  1: enable */
        delay(10000);
        putchar('I');
        SFR(JL_SPDIF->SS_CON,  1,  1,  1);                // ss_str
        /* SFR(JL_SPDIF->SS_DMA_CON, 19,  1,  1);                // et interrupt enable,   0: disable  1: enable */
    }


    if ((JL_SPDIF->SS_IO_CON & BIT(19)) && (JL_SPDIF->SS_DMA_CON & BIT(19))) {  // et_pnd & et_pns_ie
        SFR(JL_SPDIF->SS_CON,  1,  1,  0);                // ss_str

        SFR(JL_SPDIF->SS_IO_CON, 18,  1,  1);             // et_pnd_clr
        /* SFR(JL_SPDIF->SS_DMA_CON, 19,  1,  0);                // et interrupt enable,   0: disable  1: enable */
        delay(10000);
        putchar('O');
        SFR(JL_SPDIF->SS_CON,  1,  1,  1);                // ss_str
        /* SFR(JL_SPDIF->SS_DMA_CON, 18,  1,  1);                // is interrupt enable,   0: disable  1: enable */
    }


    if ((JL_SPDIF->SS_CON & BIT(11)) && (JL_SPDIF->SS_DMA_CON & BIT(23))) {                  // occurs hot-pull-out or self-excitation
        SFR(JL_SPDIF->SS_CON,  0,  1,  0);                // spdif module enable, 0: disable      1: enable
        SFR(JL_SPDIF->SS_CON,  7,  1,  1);                // clear error, include f_err, b_err, d_err, rl_err

        putchar('e');
        /* delay(100); */
        SFR(JL_SPDIF->SS_CON,  0,  1,  1);                // spdif en,            0: disable      1: enable
        SFR(JL_SPDIF->SS_CON,  1,  1,  1);                // spdif start,         0: disable      1: enable
    }


    f_err = (JL_SPDIF->SS_CON & BIT(8)) && (JL_SPDIF->SS_DMA_CON & BIT(20));
    b_err = (JL_SPDIF->SS_CON & BIT(9)) && (JL_SPDIF->SS_DMA_CON & BIT(21));
    d_err = (JL_SPDIF->SS_CON & BIT(10));
    if (f_err || b_err || d_err) {                  // f_err, b_err, d_err
        putchar('E');
        SFR(JL_SPDIF->SS_CON,  7,  1,  1);                // clear error, include f_err, b_err, d_err, rl_err

        for (i = 0; i < INFO_LEN * 2; i++) {
            info_buffer32[i] = 0;                   // clear info_buffer32
        }

        for (j = 0; j < DATA_LEN * 2; j++) {
            data_buffer32[j] = 0;                   // clear data_buffer32
        }
    }


    if ((JL_SPDIF->SS_CON & BIT(6)) && (JL_SPDIF->SS_DMA_CON & BIT(17))) {  // i_pnd & i_pnd_ie
        putchar('I');
        SFR(JL_SPDIF->SS_CON,  5,  1,  1);                            // clear information pnding

        if (!(JL_SPDIF->SS_CON & BIT(13))) {                          // information dual-buf using flag
            infobuf_ptr = (long *)info_buffer32 + INFO_LEN;     // using buf0, buf1 can be read/write
        } else {
            infobuf_ptr = (long *)info_buffer32;                // using buf1, buf0 can be read/write
        }

        char *info_buffer8  = (char *)infobuf_ptr;
        char L_C;
        char L_U;
        char R_C;
        char R_U;
        for (l = 0; l < INFO_LEN * 4; l = l + 8) {
            L_U = (((info_buffer8[l + 0] & 0x02) >> 1) << 0) + (((info_buffer8[l + 1] & 0x02) >> 1) << 1) + (((info_buffer8[l + 2] & 0x02) >> 1) << 2) + (((info_buffer8[l + 3] & 0x02) >> 1) << 3) +
                  (((info_buffer8[l + 4] & 0x02) >> 1) << 4) + (((info_buffer8[l + 5] & 0x02) >> 1) << 5) + (((info_buffer8[l + 6] & 0x02) >> 1) << 6) + (((info_buffer8[l + 7] & 0x02) >> 1) << 7);

            L_C = (((info_buffer8[l + 0] & 0x04) >> 2) << 0) + (((info_buffer8[l + 1] & 0x04) >> 2) << 1) + (((info_buffer8[l + 2] & 0x04) >> 2) << 2) + (((info_buffer8[l + 3] & 0x04) >> 2) << 3) +
                  (((info_buffer8[l + 4] & 0x04) >> 2) << 4) + (((info_buffer8[l + 5] & 0x04) >> 2) << 5) + (((info_buffer8[l + 6] & 0x04) >> 2) << 6) + (((info_buffer8[l + 7] & 0x04) >> 2) << 7);

            R_U = (((info_buffer8[l + 0] & 0x20) >> 5) << 0) + (((info_buffer8[l + 1] & 0x20) >> 5) << 1) + (((info_buffer8[l + 2] & 0x20) >> 5) << 2) + (((info_buffer8[l + 3] & 0x20) >> 5) << 3) +
                  (((info_buffer8[l + 4] & 0x20) >> 5) << 4) + (((info_buffer8[l + 5] & 0x20) >> 5) << 5) + (((info_buffer8[l + 6] & 0x20) >> 5) << 6) + (((info_buffer8[l + 7] & 0x20) >> 5) << 7);

            R_C = (((info_buffer8[l + 0] & 0x40) >> 6) << 0) + (((info_buffer8[l + 1] & 0x40) >> 6) << 1) + (((info_buffer8[l + 2] & 0x40) >> 6) << 2) + (((info_buffer8[l + 3] & 0x40) >> 6) << 3) +
                  (((info_buffer8[l + 4] & 0x40) >> 6) << 4) + (((info_buffer8[l + 5] & 0x40) >> 6) << 5) + (((info_buffer8[l + 6] & 0x40) >> 6) << 6) + (((info_buffer8[l + 7] & 0x40) >> 6) << 7);


            /* JL_TIMER3->CNT = L_U; */
            /* JL_TIMER3->PRD = L_C; */
            /* JL_TIMER4->CNT = R_U; */
            /* JL_TIMER4->PRD = R_C; */

        }
        //for(l=0; l<INFO_LEN; l++)
        //{
        //     JL_TIMER1->PRD = *(infobuf_ptr + l);
        //}
    }


    if ((JL_SPDIF->SS_CON & BIT(4)) && (JL_SPDIF->SS_DMA_CON & BIT(16))) {  // d_pnd & d_pnd_ie
        putchar('D');
        SFR(JL_SPDIF->SS_CON,  3,  1,  1);                            // clear data pnding
#if RX_BIT16
        if (!(JL_SPDIF->SS_CON & BIT(12))) {                          // data dual-buf using flag
            databuf_ptr = (long *)data_buffer32 + DATA_LEN;     // using buf0, buf1 can be read/write
        } else {
            databuf_ptr = (long *)data_buffer32;                // using buf1, buf0 can be read/write
        }
#else
        if (!(JL_SPDIF->SS_CON & BIT(12))) {                          // data dual-buf using flag
            databuf_ptr = (long *)data_buffer32 + DATA_LEN * 2;   // using buf0, buf1 can be read/write
            /* databuf_ptr = (long *)data_buffer32 + DATA_LEN;     // using buf0, buf1 can be read/write */
        } else {
            databuf_ptr = (long *)data_buffer32;                // using buf1, buf0 can be read/write
        }
#if (ENABLE_ORIGINAL_DATA_OUT)
        u32 pre_code;
        for (int i = 0; i < DATA_LEN * 2; i++) {
            /* for(int i =0;i<DATA_LEN;i++){ */
            pre_code = databuf_ptr[i] & 0x0F;
            if (pre_code != 2 && pre_code != 4) {
                printf("start code%d ,%d", i, pre_code);
            }
        }
        /* u32 pre_code = databuf_ptr[0]&0x0F;  */
        /* u32 pre_code2 = databuf_ptr[1]&0x0F;  */
        /* u32 pre_code3 = databuf_ptr[2]&0x0F;  */
        /* u32 pre_code4 = databuf_ptr[3]&0x0F;  */
        /* printf("p:%d p2:%d p3:%d p4:%d\n",pre_code,pre_code2,pre_code3,pre_code4);	 */
#endif

#endif


        /* spdif_dat_readout(DATA_LEN * LEN_DOUBLE, (BUF_TYPE *)databuf_ptr); */
        //for(k=0; k<DATA_LEN; k++)
        //{
        //     // JL_TIMER1->CNT ponit to left  channel
        //     // JL_TIMER1->PWM ponit to right channel
        //     #if RX_BIT16
        //        JL_TIMER1->CNT = (short)((*(databuf_ptr + k)>> 0) & 0xffff);
        //        JL_TIMER1->PWM = (short)((*(databuf_ptr + k)>>16) & 0xffff);
        //     #else
        //        if(k%2 == 0)
        //            JL_TIMER1->CNT = *(databuf_ptr + k);
        //        else
        //            JL_TIMER1->PWM = *(databuf_ptr + k);
        //     #endif
        //}
        //ss_data_flag = 1;

    }


    if ((JL_SPDIF->SS_DMA_CON & BIT(14)) && (JL_SPDIF->SS_DMA_CON & BIT(15))) { // c_pnd(csbr_pnd) & c_pnd_ie
        SFR(JL_SPDIF->SS_DMA_CON, 13, 1, 1);

        // ....
    }
}

void spdif_slave_test(void)
{
    printf("\n--func=%s\n", __FUNCTION__);
    SFR(JL_LSBCLK->PRP_CON1, 24, 3, 1);// d1po 192M
    SFR(JL_LSBCLK->PRP_CON1, 27, 1, 0);//
    SFR(JL_LSBCLK->PRP_CON1, 28, 2, 0);//
    SFR(JL_LSBCLK->PRP_CON1, 30, 2, 0);//

    request_irq(IRQ_SPDIF_S_IDX, 3, spdif_isr_s, 0);

    // ......   IO setting  ......
    // ..............................................
#if 0
    SFR(JL_PORTA->DIR,  9,  1,  1);                 // spdif_in a(CH0)
    SFR(JL_PORTA->DIR, 10,  1,  1);                 // spdif_in b(CH1)
    SFR(JL_PORTB->DIR,  0,  1,  1);                 // spdif_in c(CH2)
    SFR(JL_PORTB->DIR,  1,  1,  1);                 // spdif_in d(CH3)
    SFR(JL_PORTB->DIR, 11,  1,  0);                 // spdif_out

    JL_IMAP->FI_SPDIF_DIA = PB2_IN;
    JL_IMAP->FI_SPDIF_DIB = PB3_IN;
    JL_IMAP->FI_SPDIF_DIC = PB0_IN;
    JL_IMAP->FI_SPDIF_DID = PB1_IN;

    SFR(JL_PORTA->DIE,  6,  1,  1);
    SFR(JL_PORTA->DIE,  8,  1,  1);
    SFR(JL_PORTC->DIE,  0,  1,  1);
    SFR(JL_PORTC->DIE,  2,  1,  1);
    SFR(JL_PORTA->DIR,  6,  1,  1);
    SFR(JL_PORTA->DIR,  8,  1,  1);
    SFR(JL_PORTC->DIR,  0,  1,  1);
    SFR(JL_PORTC->DIR,  2,  1,  1);
    /* #else */
    SFR(JL_PORTB->DIR,  0,  1,  1);                 // spdif_in a(CH0)
    SFR(JL_PORTB->DIR,  2,  1,  1);                 // spdif_in b(CH1)
    SFR(JL_PORTB->DIR,  3,  1,  1);                 // spdif_in c(CH2)
    SFR(JL_PORTB->DIR,  4,  1,  1);                 // spdif_in d(CH3)
    /* SFR(JL_PORTB->DIR, 11,  1,  0);                 // spdif_out */

    JL_IMAP->FI_SPDIF_DIA = PB0_IN;
    JL_IMAP->FI_SPDIF_DIB = PB2_IN;
    JL_IMAP->FI_SPDIF_DIC = PB3_IN;
    JL_IMAP->FI_SPDIF_DID = PB4_IN;

    SFR(JL_PORTC->DIE,  1,  1,  1);
    SFR(JL_PORTC->DIE,  2,  1,  1);
    SFR(JL_PORTC->DIE,  3,  1,  1);
    SFR(JL_PORTC->DIE,  4,  1,  1);
    SFR(JL_PORTC->DIR,  1,  1,  1);
    SFR(JL_PORTC->DIR,  2,  1,  1);
    SFR(JL_PORTC->DIR,  3,  1,  1);
    SFR(JL_PORTC->DIR,  4,  1,  1);
#endif

    /* SFR(JL_PORTB->DIR,  5,  1,  1);                 // spdif_in d(CH3) */

    /* JL_IMAP->FI_SPDIF_DIA = PB5_IN; */
    /* JL_IMAP->FI_SPDIF_DID = PB3_IN; */

    /* gpio_set_mode(IO_PORT_SPILT(IO_PORTB_08), PORT_INPUT_FLOATING);	 */
    /* gpio_set_fun_input_port(IO_PORTB_08, PFI_SPDIF_DIA); */
    gpio_set_mode(IO_PORT_SPILT(IO_PORTB_02), PORT_INPUT_FLOATING);
    /* gpio_ich_sel_input_signal(IO_PORTB_02, INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIA, INPUT_CH_TYPE_GP_ICH); */
    /* gpio_ich_sel_input_signal(IO_PORTB_02, INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIB, INPUT_CH_TYPE_GP_ICH); */
    /* gpio_ich_sel_input_signal(IO_PORTB_02, INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIC, INPUT_CH_TYPE_GP_ICH); */
    gpio_ich_sel_input_signal(IO_PORTB_02, INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DID, INPUT_CH_TYPE_GP_ICH);


    //analog
    /* u8 port_sel = 3;//0-3 	 */
    /* gpio_set_mode(IO_PORT_SPILT(spdif_analog_port[port_sel]), PORT_INPUT_FLOATING);	 */
    //digital

    /* SFR(JL_SPDIF->SS_IO_CON,  0,  3,  0b111);             // spdif_in  io select, 0~3: no input   4: a    5: b    6: c    7: d */
    SFR(JL_SPDIF->SS_IO_CON,  0,  3,  0b100);             // spdif_in  io select, 0~3: no input   4: a    5: b    6: c    7: d

    SFR(JL_SPDIF->SS_IO_CON, 24,  3,  5);                 // spdif_out io select, 0~3: no output  4: a    5: b    6: c    7: d

    SFR(JL_SPDIF->SS_IO_CON, 27,  1,  0);                 // spdif io oe,     0: disable      1: enable

    SFR(JL_SPDIF->SS_IO_CON, 28,  1,  0);                 // di_a from; 0:digital  1:analog
    /* SFR(JL_SPDIF->SS_IO_CON, 28,  1,  1);                 // di_a from; 0:digital  1:analog */


    SFR(JL_SPDIF->SS_ANA_CON, 0, 1, 0); // amp en
    SFR(JL_SPDIF->SS_ANA_CON, 1, 2, 0); // vin sel 0:VIN0--3:VIN3
    // ......   Address & Length setting  ......
    // ..............................................
    JL_SPDIF->SS_INF_ADR = (u32)info_buffer32;            // information dma buf base address
    SFR(JL_SPDIF->SS_DMA_LEN, 16, 16, INFO_LEN);          // information dma length
    if (RX_BIT16) {
        JL_SPDIF->SS_DAT_ADR = (u32)data_buffer32;            // data dma buf base address
        SFR(JL_SPDIF->SS_DMA_LEN,  0, 16, DATA_LEN);          // data dma length
    } else {
        JL_SPDIF->SS_DAT_ADR = (u32)data_buffer32;            // data dma buf base address
        SFR(JL_SPDIF->SS_DMA_LEN,  0, 16, DATA_LEN);          // data dma length
        /* SFR(JL_SPDIF->SS_DMA_LEN,  0, 16, DATA_LEN/2);          // data dma length */

    }


    // ......   initial  ......
    // ..............................................
    SFR(JL_SPDIF->SS_IO_CON,  4,  4,  1);
    SFR(JL_SPDIF->SS_IO_CON, 12,  4,  1);
    SFR(JL_SPDIF->SS_IO_CON, 20,  4,  1);


    SFR(JL_SPDIF->SS_DMA_CON,  1,  1,  RX_BIT16);         // data dma mode,       1: 16bit        0: 24bit
    SFR(JL_SPDIF->SS_DMA_CON,  2,  1,  1);                // information dma en,  0: disable      1: enable
    SFR(JL_SPDIF->SS_DMA_CON,  3,  1,  1);                // receive level error detect enable,   0: disable      1: enable
    /* SFR(JL_SPDIF->SS_DMA_CON,  8,  5,  8);                // ???, channel status byte receive counter */
    SFR(JL_SPDIF->SS_DMA_CON, 13,  1,  1);                // clear channel status byte receive counter pnding
    SFR(JL_SPDIF->SS_DMA_CON, 16,  1,  1);                // data interrupt enable,   0: disable  1: enable
    SFR(JL_SPDIF->SS_DMA_CON, 17,  1,  1);                // information interrupt enable,   0: disable  1: enable
    /* SFR(JL_SPDIF->SS_DMA_CON, 18,  1,  1);                // is interrupt enable,   0: disable  1: enable */
    /* SFR(JL_SPDIF->SS_DMA_CON, 19,  1,  1);                // et interrupt enable,   0: disable  1: enable */
    SFR(JL_SPDIF->SS_DMA_CON,  0,  1,  1);                // data  dma en,        0: disable      1: enable
    SFR(JL_SPDIF->SS_DMA_CON,  4,  1,  1);                // 采样率跟踪优化 new add
#if (ENABLE_ORIGINAL_DATA_OUT)
    SFR(JL_SPDIF->SS_DMA_CON,  5,  1,  1);                // 24bit模式下，可以使用32bit模式 new add
#endif
    SFR(JL_SPDIF->SS_CON, 16, 16, 280);                    // ???, spdif receive level count value, 0~65535
    /* SFR(JL_SPDIF->SS_CON, 16, 16, 0xffff);                    // ???, spdif receive level count value, 0~65535 */
    SFR(JL_SPDIF->SS_CON, 14,  2,  0);                    // spdif sample clock select,   0: pll_192m(default)    1: pll_96m      2: pll_48m      3: 1
    SFR(JL_SPDIF->SS_CON,  7,  1,  1);                    // clear error, include f_err, b_err, d_err, rl_err
    SFR(JL_SPDIF->SS_CON,  5,  1,  1);                    // clear information pnding
    SFR(JL_SPDIF->SS_CON,  3,  1,  1);                    // clear danta pnding
    SFR(JL_SPDIF->SS_CON,  2,  1,  1);                    //

    SFR(JL_SPDIF->SS_CON,  0,  1,  1);                    // spdif en,            0: disable      1: enable
    SFR(JL_SPDIF->SS_CON,  1,  1,  1);                    // spdif start,         0: disable      1: enable
    printf("\n--func=%s\n", __FUNCTION__);
    /* giemask_test(); */
    /* SFR(JL_SPDIF->SS_CON,  0,  1,  0);       // slaver enable,        0: disable      1: enable */
    /* SFR(JL_SPDIF->SS_CON,  0,  1,  1);      // slaver enable,        0: disable      1: enable */
    while (1) {
        asm("idle");
    }
}

#endif
