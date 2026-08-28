#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_spdif_master.data.bss")
#pragma data_seg(".audio_spdif_master.data")
#pragma const_seg(".audio_spdif_master.text.const")
#pragma code_seg(".audio_spdif_master.text")
#endif
#include "includes.h"
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_spdif_master.h"
/* #include "audio_spdif_slave.h" */
#include "audio_config.h"
#include "sync/audio_syncts.h"
/* #include    "u_c_dat.h" */
#define SPDIF_DEBUG_INFO
#ifdef SPDIF_DEBUG_INFO
#define spdif_printf  r_printf
#else
#define spdif_printf(...)
#endif

#define SPDIF_STATE_INIT        0
#define SPDIF_STATE_OPEN        1
#define SPDIF_STATE_START       2
#define SPDIF_STATE_PREPARE     3
#define SPDIF_STATE_STOP        4
#define SPDIF_STATE_PAUSE       5
#define SPDIF_STATE_POWER_OFF   6

#define AUDIO_CFIFO_IDLE      0
#define AUDIO_CFIFO_INITED    1
#define AUDIO_CFIFO_START     2
#define AUDIO_CFIFO_CLOSE     3

/* const int config_audio_spdif_mix_enable = 1; */
/* struct audio_spdif_hdl spdif_handler; */

#define IRQ_MIN_POINTS      60
#define  INF_LEN      192/8         // unit: byte
#define  INF_BUF_LEN  INF_LEN*8     // INF_LEN *2(dual buffer) * 4(l_c,l_v,r_c,r_v)



static struct audio_spdif_hdl *xspdif_hdl = NULL;

int iis_adapter_link_to_syncts_check(void *syncts);
int audio_spdif_syncts_enter_update_frame(struct audio_spdif_channel *ch);
int audio_spdif_syncts_exit_update_frame(struct audio_spdif_channel *ch);
int audio_spdif_channel_buffered_frames(struct audio_spdif_channel *ch);
static int audio_spdif_channel_fifo_write(struct audio_spdif_channel *ch, void *data, int len, u8 is_fixed_data);


/* static SPDIF_MASTER_PARM *__this = NULL; */

static int spdif_set_fifo_start_and_delay(struct audio_spdif_hdl *spdif);
int audio_spdif_set_protect_time(struct audio_spdif_hdl *spdif, int time, void *priv, void (*feedback)(void *));
static void audio_spdif_fifo_start(struct audio_spdif_hdl *spdif);

static void spdif_master_info_dump()
{
    spdif_printf("JL_SPDIF->SM_CLK = 0x%x\n", JL_SPDIF->SM_CLK);
    spdif_printf("JL_SPDIF->SM_CON = 0x%x\n", JL_SPDIF->SM_CON);
    spdif_printf("JL_SPDIF->SM_ABADR = 0x%x\n", JL_SPDIF->SM_ABADR);
    spdif_printf("JL_SPDIF->SM_ABUFLEN = x%x\n", JL_SPDIF->SM_ABUFLEN);
    spdif_printf("JL_SPDIF->SM_AHPTR = 0x%x\n", JL_SPDIF->SM_AHPTR);
    spdif_printf("JL_SPDIF->SM_ASPTR = 0x%x\n", JL_SPDIF->SM_ASPTR);
    spdif_printf("JL_SPDIF->SM_ASHN = 0x%x\n", JL_SPDIF->SM_ASHN);
    spdif_printf("JL_SPDIF->SM_APNS = 0x%x\n", JL_SPDIF->SM_APNS);
    spdif_printf("JL_SPDIF->SM_IBADR = 0x%x\n", JL_SPDIF->SM_IBADR);
    spdif_printf("JL_SPDIF->SM_IBUFLEN = x%x\n", JL_SPDIF->SM_IBUFLEN);
    spdif_printf("JL_SPDIF->SM_IHPTR = 0x%x\n", JL_SPDIF->SM_IHPTR);
    /* spdif_printf("JL_CLOCK->CLK_CON5[13:11] = 0x%x\n", (JL_CLOCK->CLK_CON5 >> 11) & 0b111); */
}


___interrupt
static void spdif_master_isr()
{
    if (INF_IS_PND()) {
        INF_PND_CLR();
#if 0
        if (__this->inf_isr_cb) {
            __this->inf_isr_cb(__this->inf_buf, __this->inf_dma_len / 2);
        }
#endif
    }

    if (DAT_IS_PND()) {
#if 0
        s16 *buf_addr = NULL;
        buf_addr = (s16 *)(__this->data_buf + JL_SPDIF->SM_ASPTR);

        if (__this->data_isr_cb) {
            __this->data_isr_cb(buf_addr, __this->data_dma_len / 2);
        }
        JL_SPDIF->SM_ASHN = __this->data_dma_len / 8;
        /* SET_DATABUF_WRITE_BYTE(__this->data_dma_len / 2); */
#else
        putchar('D');
#endif
        DAT_PND_CLR();
    }
}
static void spdif_master_io_init(u8 gpio)
{
    /*     gpio_hw_set_direction(IO_PORT_SPILT(gpio), 0);//0:out, 1:in */
    /* gpio_hw_set_die(IO_PORT_SPILT(gpio), 0); */
    /* gpio_set_fun_output_port(gpio, FO_SPDIF_DO, 1, 1); */
    gpio = IO_PORTB_00;
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_OUTPUT_LOW);
    gpio_och_sel_output_signal(gpio, OUTPUT_CH_SIGNAL_SPDIF_DO);

    // test io 2
    /* gpio_hw_set_direction(IO_PORT_SPILT(IO_PORTB_08), 0);//0:out, 1:in */
    /* gpio_hw_set_die(IO_PORT_SPILT(IO_PORTB_08), 0); */
    /* gpio_set_fun_output_port(IO_PORTB_08, FO_SPDIF_DO, 1, 1); */

    /* OUTPUT_ANA_SEL(0); */
    /* IO_IN_TO_OUT_EN(0);//禁止从机输入映射到输出 */
    /* IO_OUTPUT_SEL(0);//输出IO选择主机信号 */
}

u8 get_sr_info_mark(u32 sr)
{
    u8 ret = 0;
    switch (sr) {
    case 192000:
        ret = 1 ;
        break;
    case 12000:
        ret = 2;
        break;
    case 176400:
        ret = 3;
        break;
    case 96000:
        ret = 5;
        break;
    case 8000:
        ret = 6;
        break;

    case 16000:
        ret = 8;
        break;
    case 24000:
        ret = 9;
        break;
    case 11025:
        ret = 10;
        break;

    case 32000:
        ret = 12;
        break;
    case 48000:
        ret = 13;
        break;
    case 44100:
        ret = 15;
        break;
    case 88200:
        ret = 7;
        break;
    case 22050:
        ret = 11;
        break;
    default:
        printf("[%s], can't identify spdif sampal rate, return default sr:48000!\n", __func__);
        break;
    }
    //
    ret = ret ^ 0xF;
    //                  //    ret = ret<<4;
    printf("\n -------------spdif sr inf %d %d\n", ret, sr);
    return ret;
}
int spdif_master_init(struct audio_spdif_hdl *hd_spdif)
{
    SM_RESET();


    SM_DAT_LEN(hd_spdif->output_buf_len / 4);
    SM_INF_LEN(hd_spdif->output_inf_buf_len / 8);
    SM_SET_APNS(hd_spdif->output_buf_len / 16);


    SM_DATA_IE(1);
    /* SM_INFO_IE(1); */

    SM_AUX_DAT_L(0);
    SM_AUX_DAT_R(0);
    SM_VAL_FLAG_L(0);
    SM_VAL_FLAG_R(0);
    printf("-----hd_spdif->data_mode %d\n", hd_spdif->data_mode);
    SM_DAT_MODE(hd_spdif->data_mode);
    /* SM_AUX_SEL(__this->data_mode); */
    SM_AUX_SEL(hd_spdif->data_mode ? 0 : 1); //1:辅助位使用音频数据地位（对应24bit位宽）;0:发送 Chl_1_aux Chl_r_aux寄存器值

    if (hd_spdif->output_buf) {
        JL_SPDIF->SM_ABADR = (volatile unsigned int)hd_spdif->output_buf;
    }
    u8 sr_mark = get_sr_info_mark(hd_spdif->sample_rate);
    if (hd_spdif->output_inf_buf) {
        hd_spdif->output_inf_buf[3] =  sr_mark;//BUF0 L_C
        hd_spdif->output_inf_buf[3 + 24] =  sr_mark; //BUF1 L_C
        hd_spdif->output_inf_buf[3 + 96] =  sr_mark; //BUF0 R_C
        hd_spdif->output_inf_buf[3 + 24 + 96] =  sr_mark; //BUF1 R_C
        JL_SPDIF->SM_IBADR = (volatile unsigned int)hd_spdif->output_inf_buf;
    }
    SW_VAL_USER(0);
    /* SFR(JL_CLOCK->CLK_CON5, 11, 3, 2); */
    SFR(JL_LSBCLK->PRP_CON1, 16, 3, 1);// d1p0 192M fpga
    int spdif_tx_clk = SPDIF_TX_CLK;
    int integer = spdif_tx_clk / hd_spdif->sample_rate / 128;
    float decimal = (float)spdif_tx_clk / hd_spdif->sample_rate / 128 - (float)(integer);
    int int_decimal;
    if (decimal > 0.5) {
        integer = integer + 1;
        int_decimal = (float)((decimal - 1) * 32768);
    } else {
        int_decimal = (float)(decimal * 32768);
    }
    spdif_printf("spdif_master : clk = %d, integer = %d, decimal= %d\n", spdif_tx_clk, integer, int_decimal);
    SM_CLK_INT_DIV(integer);
    SM_CLK_FRAC_DIV(int_decimal);

    SM_CLK_DIV_MODE(0);
    SM_CLK_DIV_NSEL(0);


    DAT_PND_CLR();
    INF_PND_CLR();
    spdif_master_io_init(hd_spdif->tx_io);
    request_irq(IRQ_SPDIF_M_IDX, 3, spdif_master_isr, 0);

    return 0;
}

int spdif_master_start(struct audio_spdif_hdl *spdif)
{
    memset(spdif->output_buf, 0x0, spdif->output_buf_len);
    os_mutex_pend(&spdif->mutex, 0);
    spdif->state = SPDIF_STATE_PREPARE;
    spdif->prepare_points = 0;
    spdif->protect_pns = 0;
    spdif->unread_samples = 0;


    os_mutex_post(&spdif->mutex);

    spdif_set_fifo_start_and_delay(spdif);
    audio_spdif_set_protect_time(spdif, 1, NULL, NULL);
    spdif->protect_pns = spdif->protect_time ? (spdif->protect_time * spdif->sample_rate / 1000) : 0;

    JL_SPDIF->SM_APNS = spdif->protect_pns ? spdif->protect_pns : IRQ_MIN_POINTS;
    /* JL_SPDIF->SM_AHPTR = 0; */
    /* JL_SPDIF->SM_ASPTR = 0; */
    /* JL_SPDIF->SM_ASHN  = 0; */
    spdif->prepare_points = spdif->start_points;
    /* if (!config_audio_spdif_mix_enable) { */
    /*     puts(" spdif fifo _start \n\n"); */
    /*     audio_spdif_fifo_start(spdif); */
    /* } */

    /* SM_OUT_EN(1); */
    /* SM_EN(1); */
    spdif_printf("spdif_master_start\n");
    spdif_master_info_dump();
    return 0;
}

int spdif_master_stop(struct audio_spdif_hdl *spdif)
{
    if (spdif->state != SPDIF_STATE_PREPARE && spdif->state != SPDIF_STATE_START) {
        return 0;
    }
    SM_OUT_EN(0);
    SM_EN(0);
    SM_RESET();
    spdif->state = SPDIF_STATE_STOP;
    spdif->fifo_state = AUDIO_CFIFO_CLOSE;
    return 0;
}

void spdif_master_uninit()
{
    /* spdif_master_stop(); */

    INF_PND_CLR();
    DAT_PND_CLR();
#if 0
    if (__this->data_buf) {
        free(__this->data_buf);
        __this->data_buf = NULL;
    }

    if (__this->inf_buf) {
        free(__this->inf_buf);
        __this->inf_buf = NULL;
    }
    __this = NULL;
#endif
    spdif_printf("spdif_master_exit OK\n");
}

__attribute__((weak)) u16 spdif_start_points(void)
{
    return 320;
}

static int spdif_set_fifo_start_and_delay(struct audio_spdif_hdl *spdif)
{
    if (spdif->sample_rate) {
        /*这里处理SPDIF设置最大的延时点数*/
        spdif->delay_points = ((spdif->sample_rate / (1000 / spdif->delay_ms)) >> 2) << 2;
        if (spdif->delay_points > JL_SPDIF->SM_ABUFLEN) {
            spdif->delay_points = JL_SPDIF->SM_ABUFLEN;
        }
        spdif->irq_points = ((spdif->sample_rate / 400) >> 2) << 2;
        if (spdif->irq_points > 512) {
            spdif->irq_points = 512;
        }

        spdif->start_points = ((spdif->sample_rate / (1000 / spdif->start_ms)) >> 2) << 2;
        u16 min_once_points = ((spdif->delay_points - spdif->irq_points - 1) >> 2) << 2;
        if (spdif->start_points > min_once_points) {
            spdif->start_points = min_once_points;
        }

        u16 start_points_min = spdif_start_points();
        if ((spdif->start_points < start_points_min) && (spdif->delay_points > start_points_min)) {
            spdif->start_points = start_points_min;
        }
        log_i("delay set : %d, %d, %d\n", spdif->delay_points, spdif->irq_points,
              spdif->start_points);
    }

    return 0;
}
int audio_spdif_set_sample_rate(struct audio_spdif_hdl *spdif, int sample_rate)
{
    /* int reg; */

    os_mutex_pend(&spdif->mutex, 0);

    spdif->sample_rate = sample_rate;
    os_mutex_post(&spdif->mutex);

    /*log_info("sample %d\n", spdif->sample_rate);*/

    return 0;
}

int audio_spdif_get_sample_rate(struct audio_spdif_hdl *spdif)
{
    return spdif->sample_rate;
}

int audio_spdif_set_protect_time(struct audio_spdif_hdl *spdif, int time, void *priv, void (*feedback)(void *))
{
    spin_lock(&spdif->lock);
    spdif->protect_time = time;
    spdif->protect_pns = 0;
    /* spdif->feedback_priv = priv; */
    /* spdif->underrun_feedback = feedback; */
    spin_unlock(&spdif->lock);
    return 0;
}

/* static void audio_spdif_release_fifo(void *_spdif) */
/* { */
/* struct audio_spdif_hdl *spdif = (struct audio_spdif_hdl *)_spdif; */

/* JL_AUDIO->DAC_SWN = spdif->prepare_points; */
/* } */

static void audio_spdif_fifo_start(struct audio_spdif_hdl *spdif)
{
    if (spdif->state != SPDIF_STATE_PREPARE) {
        /* log_e("spdif module on error, state : 0x%x\n", spdif->state); */
        return;
    }
    /*!!!这里0不写入，DAC SWN写0会导致HRP不停向后跑*/
    if (spdif->prepare_points) {
        JL_SPDIF->SM_ASHN = spdif->prepare_points;
        __asm_csync();
    }
    /* printf("\n--func=%s spdif->prepare_points %d \n", __FUNCTION__,spdif->prepare_points); */
    spin_lock(&spdif->lock);
    SM_OUT_EN(1);
    SM_EN(1);
    spdif->state = SPDIF_STATE_START;
    spin_unlock(&spdif->lock);
    /* log_info("audio_spdif_fifo_start : 0x%x, %d, DAC_CON = 0x%x\n", dac->prepare_points, dac->gain, JL_AUDIO->DAC_CON); */
}

u8 audio_spdif_is_working(struct audio_spdif_hdl *spdif)
{
    if (!spdif) {
        return 0;
    }
    spin_lock(&spdif->lock);
    if (spdif->state == SPDIF_STATE_START) {
        spin_unlock(&spdif->lock);
        return 1;
    }
    spin_unlock(&spdif->lock);

    return 0;
}
int audio_spdif_channel_buffered_frames(struct audio_spdif_channel *ch)
{
    struct audio_spdif_hdl *hdl = (struct audio_spdif_hdl *)ch->hd_spdif;
    if (!hdl) {
        return 0;
    }

    int buffered_frames = 0;

    os_mutex_pend(&hdl->mutex, 0);
    if (ch->state != AUDIO_CFIFO_START) {
        goto ret_value;
    }


    buffered_frames = ((JL_SPDIF->SM_ABUFLEN - JL_SPDIF->SM_ASHN - 1) >> ch->fifo.bit_wide) - audio_cfifo_channel_unread_diff_samples(&ch->fifo);
    if (buffered_frames < 0) {
        buffered_frames = 0;
    }

ret_value:
    os_mutex_post(&hdl->mutex);

    return buffered_frames;
}

int audio_spdif_data_len(void *private_data)
{
    struct audio_spdif_channel *ch = (struct audio_spdif_channel *)private_data;
    return audio_spdif_channel_buffered_frames(ch);
}



static int audio_spdif_channel_use_syncts_frames(struct audio_spdif_channel *ch, int frames)
{
    struct audio_spdif_hdl *hdl = (struct audio_spdif_hdl *)ch->hd_spdif;
    if (!hdl) {
        return 0;
    }

    struct audio_spdif_sync_node *node;
    u8 have_syncts = 0;
    u8 point_offset = 1;
    if (hdl->data_mode == 0) {
        point_offset = 2;
    }

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (!node->triggered) {
            continue;
        }
        /*
         * 这里需要将其他通道延时数据大于音频同步计数通道的延迟数据溢出值通知到音频同步
         * 锁存模块，否则会出现硬件锁存不同通道数据的溢出误差
         */
        int diff_samples = audio_cfifo_channel_unread_diff_samples(&((struct audio_spdif_channel *)node->ch)->fifo);
        sound_pcm_update_overflow_frames(node->hdl, diff_samples);
        if ((u32)node->ch != (u32)ch) {
            continue;
        }
        /* if (node->network == AUDIO_NETWORK_LOCAL) { */
        /* sound_pcm_update_frame_num_and_time(node->hdl, frames, audio_jiffies_usec(), audio_spdif_channel_buffered_frames(ch)); */
        /* } else { */
        /* sound_pcm_update_frame_num(node->hdl, frames); */
        /* } */
        sound_pcm_update_frame_num(node->hdl, frames);
        if (node->network == AUDIO_NETWORK_LOCAL && ch->state == AUDIO_CFIFO_START) {
            if (audio_syncts_latch_enable(node->hdl)) {
                spin_lock(&hdl->lock);
                int timeout = (1000000 / hdl->sample_rate) * (clk_get("sys") / 1000000) * 5 / 4;
                u32 swn = JL_SPDIF->SM_ASHN / point_offset;
                while (--timeout > 0 && swn == (JL_SPDIF->SM_ASHN / point_offset));
                u32 time = audio_jiffies_usec();
                swn = JL_SPDIF->SM_ASHN / point_offset;
                spin_unlock(&hdl->lock);
                int buffered_frames = (JL_SPDIF->SM_ABUFLEN / point_offset - swn - 1) - audio_cfifo_channel_unread_diff_samples(&ch->fifo);
                sound_pcm_update_frame_num_and_time(node->hdl, 0, time, buffered_frames);
            }
        }


        have_syncts = 1;
    }

    if (have_syncts) {
        u16 free_points = JL_SPDIF->SM_ASHN / point_offset;
        int timeout = (1000000 / hdl->sample_rate) * (clk_get("sys") / 1000000);
        while (free_points == (JL_SPDIF->SM_ASHN / point_offset) && (--timeout > 0));
    }

    return 0;
}


static void audio_spdif_update_frame_begin(struct audio_spdif_channel *ch, u32 frames)
{
    audio_spdif_syncts_enter_update_frame(ch);
}

static void audio_spdif_update_frame(struct audio_spdif_channel *ch, u32 frames)
{
    audio_spdif_channel_use_syncts_frames(ch, frames);
}

static void audio_spdif_update_frame_end(struct audio_spdif_channel *ch)
{
    audio_spdif_syncts_exit_update_frame(ch);
}

int audio_spdif_fill_slience_frames(void *private_data,  int frames)
{
    struct audio_spdif_channel *ch = (struct audio_spdif_channel *)private_data;
    struct audio_spdif_hdl *hdl = (struct audio_spdif_hdl *)ch->hd_spdif;
    if (!hdl) {
        return 0;
    }

    int wlen = 0;
    u16 point_offset = 1;
    if (hdl->data_mode == 0) {
        point_offset = 2;
    }
    wlen = audio_spdif_channel_fifo_write(ch, (void *)0, frames * hdl->channel << point_offset, 1);

    return (wlen >> point_offset) / hdl->channel;
}


int audio_spdif_syncts_enter_update_frame(struct audio_spdif_channel *ch)
{
    struct audio_spdif_sync_node *node;
    struct audio_spdif_hdl *hdl = (struct audio_spdif_hdl *)ch->hd_spdif;
    if (!hdl) {
        return 0;
    }
    int ret = 0;
    list_for_each_entry(node, &hdl->sync_list, entry) {
        ret = sound_pcm_enter_update_frame(node->hdl);
    }
    return ret;
}

int audio_spdif_syncts_exit_update_frame(struct audio_spdif_channel *ch)
{
    struct audio_spdif_sync_node *node;
    struct audio_spdif_hdl *hdl = (struct audio_spdif_hdl *)ch->hd_spdif;
    if (!hdl) {
        return 0;
    }

    int ret = 0;
    list_for_each_entry(node, &hdl->sync_list, entry) {
        ret = sound_pcm_exit_update_frame(node->hdl);
    }

    return ret;
}

void audio_spdif_force_use_syncts_frames(void *private_data, int frames, u32 timestamp)
{
    struct audio_spdif_sync_node *node;
    struct audio_spdif_channel *ch = (struct audio_spdif_channel *)private_data;
    struct audio_spdif_hdl *hdl = (struct audio_spdif_hdl *)ch->hd_spdif;
    if (!hdl) {
        return ;
    }
    os_mutex_pend(&hdl->mutex, 0);
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->ch != (u32)ch)  {
            continue;
        }
        if (!node->triggered) {
            if (audio_syncts_support_use_trigger_timestamp(node->hdl)) {
                u8 timestamp_enable = audio_syncts_get_trigger_timestamp(node->hdl, &node->timestamp);
                if (!timestamp_enable) {
                    continue;
                }
            }
            int time_diff = node->timestamp - timestamp;
            if (time_diff > 0) {
                os_mutex_post(&hdl->mutex);
                return;
            }
            if (node->hdl) {
                sound_pcm_update_frame_num(node->hdl, frames);
            }
        }
    }
    os_mutex_post(&hdl->mutex);
}



void audio_spdif_syncts_trigger_with_timestamp(void *private_data, u32 timestamp)
{
    struct audio_spdif_sync_node *node;
    struct audio_spdif_channel *ch = (struct audio_spdif_channel *)private_data;
    struct audio_spdif_hdl *hdl = (struct audio_spdif_hdl *)ch->hd_spdif;
    if (!hdl) {
        return ;
    }

    int time_diff = 0;
    os_mutex_pend(&hdl->mutex, 0);
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (node->triggered || ((u32)node->ch != (u32)ch)) {
            continue;
        }
        if (audio_syncts_support_use_trigger_timestamp(node->hdl)) {
            u8 timestamp_enable = audio_syncts_get_trigger_timestamp(node->hdl, &node->timestamp);
            if (!timestamp_enable) {
                continue;
            }
        }

        //int one_sample = PCM_SAMPLE_ONE_SECOND / dac->sample_rate;
        time_diff = timestamp - node->timestamp;
        /*printf("----- %d, %u, %u-----\n", time_diff, node->timestamp, timestamp);*/
        if (time_diff >= 0) {
            /*
            if (node->hdl) {
                sound_pcm_update_frame_num(node->hdl, TIME_TO_PCM_SAMPLES(time_diff, dac->sample_rate));
            }
            */
            sound_pcm_syncts_latch_trigger(node->hdl);
            /*printf("------------>>>> sound pcm syncts latch trigger : %d, %u, %u---\n", time_diff, node->timestamp, timestamp);*/
            node->triggered = 1;
        }
    }
    os_mutex_post(&hdl->mutex);
}

__attribute__((weak))
int dac_adapter_link_to_syncts_check(void *syncts)
{
    return 0;
}

void audio_spdif_add_syncts_with_timestamp(struct audio_spdif_channel *ch, void *syncts, u32 timestamp)
{
    if (!ch) {
        return;
    }
    struct audio_spdif_hdl *hdl = ch->hd_spdif;
    if (!hdl) {
        return;
    }
    struct audio_spdif_sync_node *node = NULL;
    os_mutex_pend(&hdl->mutex, 0);
    if (iis_adapter_link_to_syncts_check(syncts)) {
        printf("syncts has beed link to iis");
        os_mutex_post(&hdl->mutex);
        return;
    }
    if (dac_adapter_link_to_syncts_check(syncts)) {
        printf("syncts has beed link to dac");
        os_mutex_post(&hdl->mutex);
        return;
    }

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->hdl == (u32)syncts) {
            os_mutex_post(&hdl->mutex);
            return;
        }
    }
    os_mutex_post(&hdl->mutex);

    if (sound_pcm_get_syncts_network(syncts) != AUDIO_NETWORK_LOCAL) { //本地时钟不需要关联
        log_e("spdif not support a2dp 2t1 network\n");
        return;
    }
    log_d(">>>>>>>>>>>>add spdif syncts %x\n", syncts);
    node = (struct audio_spdif_sync_node *)zalloc(sizeof(struct audio_spdif_sync_node));
    node->hdl = syncts;
    node->network = sound_pcm_get_syncts_network(syncts);
    node->timestamp = timestamp;
    node->ch = ch;
    /* if (sound_pcm_get_syncts_network(syncts) != AUDIO_NETWORK_LOCAL) { //本地时钟不需要关联 */
    /* sound_pcm_device_register(syncts, PCM_INSIDE_DAC); */
    /* } */
    os_mutex_pend(&hdl->mutex, 0);
    spin_lock(&hdl->lock);
    list_add(&node->entry, &hdl->sync_list);
    spin_unlock(&hdl->lock);
    os_mutex_post(&hdl->mutex);
}
void audio_spdif_remove_syncts_handle(struct audio_spdif_channel *ch, void *syncts)
{
    if (!ch) {
        return;
    }
    struct audio_spdif_hdl *hdl = ch->hd_spdif;
    if (!hdl) {
        return;
    }
    os_mutex_pend(&hdl->mutex, 0);
    struct audio_spdif_sync_node *node;
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->ch != (u32)ch)  {
            continue;
        }
        if (node->hdl == syncts) {
            goto remove_node;
        }
    }
    os_mutex_post(&hdl->mutex);

    return;
remove_node:

    log_d(">>>>>>>>>>>>del spdif syncts %x\n", syncts);

    spin_lock(&hdl->lock);
    list_del(&node->entry);
    free(node);
    spin_unlock(&hdl->lock);
    os_mutex_post(&hdl->mutex);
}

static int audio_spdif_channel_fifo_write(struct audio_spdif_channel *ch, void *data, int len, u8 is_fixed_data)
{
    if (len == 0) {
        return 0;
    }
    struct audio_spdif_hdl *hd_spdif = ch->hd_spdif;
    os_mutex_pend(&hd_spdif->mutex, 0);
    ch->fifo.bit_wide = hd_spdif->data_mode ? 0 : 1;
    int w_len = 0;
    int frames = 0;
    /* int unread_samples = (JL_SPDIF->SM_ABUFLEN - JL_SPDIF->SM_ASHN - 1) / 2; */
    int unread_samples = (JL_SPDIF->SM_ABUFLEN - JL_SPDIF->SM_ASHN) >> (ch->fifo.bit_wide);
    /* printf("unread_samples %d  %d\n",unread_samples,hd_spdif->unread_samples); */
    if (hd_spdif->unread_samples < unread_samples) {
        //reset cbuf
        JL_SPDIF->SM_ASPTR = (audio_cfifo_get_write_offset(&hd_spdif->fifo) << ch->fifo.bit_wide);
        unread_samples = 0;
    }
    audio_cfifo_read_update(&hd_spdif->fifo, hd_spdif->unread_samples - unread_samples);

    hd_spdif->unread_samples = unread_samples;

    if (is_fixed_data) {
        w_len = audio_cfifo_channel_write_fixed_data(&ch->fifo, (s16)data, len);
    } else {
        w_len = audio_cfifo_channel_write(&ch->fifo, data, len);
    }
    int fifo_frames = (w_len >> (hd_spdif->data_mode ? 1 : 2)) / hd_spdif->channel;

    int samples = audio_cfifo_get_unread_samples(&hd_spdif->fifo) - hd_spdif->unread_samples;

    ASSERT(samples >= 0, " samples : %d, %d\n", audio_cfifo_get_unread_samples(&hd_spdif->fifo), hd_spdif->unread_samples);

    hd_spdif->unread_samples += samples;

    frames = samples;


    audio_spdif_update_frame_begin(ch, fifo_frames);
    if (frames) {
        JL_SPDIF->SM_ASHN = frames << ch->fifo.bit_wide;
        __asm_csync();
    }
    /* audio_spdif_fifo_start(hd_spdif); */
    audio_spdif_update_frame(ch, fifo_frames);
    audio_spdif_update_frame_end(ch);

    ASSERT(JL_SPDIF->SM_ASPTR == (audio_cfifo_get_write_offset(&hd_spdif->fifo) << ch->fifo.bit_wide), ", %d %d\n", JL_SPDIF->SM_ASPTR, audio_cfifo_get_write_offset(&hd_spdif->fifo));
    os_mutex_post(&hd_spdif->mutex);

    return w_len;
}

int audio_spdif_channel_write(void *private_data, struct audio_spdif_hdl *spdif, void *buf, int len)
{
    struct audio_spdif_channel *ch = private_data;
    if (!ch) {
        return audio_spdif_channel_fifo_write(&spdif->main_ch, buf, len, 0);
    } else {
        return audio_spdif_channel_fifo_write(ch, buf, len, 0);
    }
}

void audio_spdif_channel_start(void *private_data)
{
    struct audio_spdif_hdl *hd_spdif = xspdif_hdl;
    if (!hd_spdif) {
        log_e("DAC channel start error! dac_hdl is NULL\n");
        return;
    }
    os_mutex_pend(&hd_spdif->mutex, 0);
    if (!audio_spdif_is_working(hd_spdif)) {
        spdif_master_start(hd_spdif);
    }
    /* u16 point_offset = 1; */
    /* if (hd_spdif->data_mode == 0) { */
    /* point_offset = 2; */
    /* } */

    struct audio_spdif_channel *ch = private_data;

    if (hd_spdif->fifo_state != AUDIO_CFIFO_INITED) {
        hd_spdif->fifo.bit_wide = hd_spdif->data_mode ? 0 : 1;
        /* printf("audio fifo init"); */
        printf("hd_spdif->channel==================== %d, %d\n", hd_spdif->channel, hd_spdif->fifo.bit_wide);
        audio_cfifo_init(&hd_spdif->fifo, hd_spdif->output_buf, hd_spdif->output_buf_len, hd_spdif->sample_rate, hd_spdif->channel);
        hd_spdif->fifo_state = AUDIO_CFIFO_INITED;
    }
    if (hd_spdif->state != SPDIF_STATE_START) {

        audio_spdif_fifo_start(hd_spdif);
        if (!ch) {
            JL_SPDIF->SM_ASPTR = 0;
            audio_cfifo_channel_add(&hd_spdif->fifo, &hd_spdif->main_ch.fifo, hd_spdif->prepare_points, 0);
            hd_spdif->prepare_points = 0;
            hd_spdif->main_ch.state = AUDIO_CFIFO_START;
        } else {
            JL_SPDIF->SM_ASPTR = 0;
            audio_cfifo_channel_add(&hd_spdif->fifo, &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);
            hd_spdif->prepare_points = 0;
            ch->state = AUDIO_CFIFO_START;
        }
    } else {
        if (!ch) {
            if (hd_spdif->main_ch.state != AUDIO_CFIFO_START) {
                audio_cfifo_channel_add(&hd_spdif->fifo, &hd_spdif->main_ch.fifo, hd_spdif->start_points, 0);
                hd_spdif->main_ch.state = AUDIO_CFIFO_START;
            }
        } else {
            if (ch->state != AUDIO_CFIFO_START) {
                audio_cfifo_channel_add(&hd_spdif->fifo, &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);
                ch->state = AUDIO_CFIFO_START;
            }
        }
    }
    os_mutex_post(&hd_spdif->mutex);
}

void audio_spdif_channel_close(void *private_data)
{
    struct audio_spdif_hdl *hd_spdif = xspdif_hdl;
    if (!hd_spdif) {
        log_e("SPDIF channel close error! xspdif_hdl is NULL\n");
        return;
    }
    struct audio_spdif_channel *ch = private_data;
    if (ch == NULL) {
        ch = &hd_spdif->main_ch;
    }

    os_mutex_pend(&hd_spdif->mutex, 0);
    if (ch->state != AUDIO_CFIFO_IDLE && ch->state != AUDIO_CFIFO_CLOSE) {
        if (ch->state == AUDIO_CFIFO_START) {
            audio_cfifo_channel_del(&ch->fifo);
            ch->state = AUDIO_CFIFO_CLOSE;
        }
    }
    if (hd_spdif->fifo_state != AUDIO_CFIFO_IDLE) {
        if (audio_cfifo_channel_num(&hd_spdif->fifo) == 0) {
            spdif_master_stop(hd_spdif);
        }
    }
    os_mutex_post(&hd_spdif->mutex);
}


/*
 * Audio SPDIF通道的属性设置（延时/写入模式/...）
 */
int audio_spdif_channel_set_attr(struct audio_spdif_channel *ch, struct audio_spdif_channel_attr *attr)
{
    if (!attr) {
        return -EINVAL;
    }
    memcpy(&ch->attr, attr, sizeof(ch->attr));
    //TODO : 可继续增加其他属性配置
    return 0;
}

/*
 * Audio SPDIF申请加入一个新的SPDIF通道
 */
int audio_spdif_new_channel(struct audio_spdif_hdl *hd_spdif, struct audio_spdif_channel *ch)
{
    if (!hd_spdif || !ch) {
        return -EINVAL;
    }

    spin_lock(&hd_spdif->lock);
    memset(ch, 0x0, sizeof(struct audio_spdif_channel));

    ch->hd_spdif = hd_spdif;
    ch->state = AUDIO_CFIFO_IDLE;

    spin_unlock(&hd_spdif->lock);

    return 0;
}

#define AUDIO_SPDIF_MAX_SAMPLE_RATE      AUDIO_DAC_MAX_SAMPLE_RATE  //     96000
#define TCFG_AUDIO_SPDIF_BUFFER_TIME_MS   50//TCFG_AUDIO_DAC_BUFFER_TIME_MS   //50 //缓冲长度（ms）
int spdif_master_out_init(struct audio_spdif_hdl *spdif_handler, SPDIF_MASTER_PARM *spdif_master_cfg)
{
    puts("spdif master early init \n");
    memset(spdif_handler, 0x0, sizeof(struct audio_spdif_hdl));
    spdif_handler->tx_io = spdif_master_cfg->tx_io;
    /* spdif_handler->data_mode = 	SPDIF_M_DAT_24BIT; */
    spdif_handler->data_mode = spdif_master_cfg->bit_width ? SPDIF_M_DAT_24BIT : SPDIF_M_DAT_16BIT;
    spdif_handler->output_buf_len = TCFG_AUDIO_SPDIF_BUFFER_TIME_MS * ((AUDIO_SPDIF_MAX_SAMPLE_RATE + 999) / 1000) * 2 * (
                                        spdif_handler->data_mode ? 2 : 4);
    printf(">>>> buf time:%dms, buf_len : %d\n", TCFG_AUDIO_SPDIF_BUFFER_TIME_MS, spdif_handler->output_buf_len);
    spdif_handler->output_buf = dma_malloc(spdif_handler->output_buf_len);
    memset(spdif_handler->output_buf, 0x00, spdif_handler->output_buf_len);

    spdif_handler->output_inf_buf_len = INF_BUF_LEN;
    spdif_handler->output_inf_buf = dma_malloc(spdif_handler->output_inf_buf_len);
    memset(spdif_handler->output_inf_buf, 0x0, spdif_handler->output_inf_buf_len);
    ASSERT(spdif_handler->output_buf != NULL);
    spdif_handler->state = SPDIF_STATE_INIT;
    spdif_handler->fifo_state = AUDIO_CFIFO_IDLE;
    spdif_handler->channel = 2;
    spdif_handler->sample_rate = 48000;//44100;
    spdif_handler->start_ms = 10;
    spdif_handler->delay_ms = 20;

    spdif_handler->main_ch.hd_spdif = spdif_handler;
    spdif_handler->main_ch.state = AUDIO_CFIFO_IDLE;
    spdif_handler->main_ch.attr.delay_time = 20;
    spdif_handler->main_ch.attr.protect_time = 8;
    spdif_handler->main_ch.attr.write_mode = 0;//WRITE_MODE_BLOCK;
    os_mutex_create(&spdif_handler->mutex);

    INIT_LIST_HEAD(&(spdif_handler->sync_list));
    spin_lock_init(&spdif_handler->lock);

    xspdif_hdl = spdif_handler;
#if 0
    // for test
    audio_spdif_new_channel(spdif_handler, &spdif_ch);

    struct audio_spdif_channel_attr attr;
    attr.delay_time = 10;
    attr.protect_time = 8;
    attr.write_mode = 0;//WRITE_MODE_BLOCK;
    audio_spdif_channel_set_attr(&spdif_ch, &attr);
    /* spdif_master_init(&test_spdif_master); */
    /* spdif_master_init(spdif_handler,&test_spdif_master); */
    /* int err = task_create(spdif_app_task, NULL, SPDIF_TEST_TASK_NAME); */
#endif
    return 0;
}


//FPGA TEST
//*****************************************************************************
//*                         Spdif master                                      *
//*****************************************************************************
#if 0
void spdif_master(void)
{
    /* JL_TIMER0->PRD = 0x1; */

    /* INTALL_HWI(58, spdif_isr_m, 0); */
    SFR(JL_LSBCLK->PRP_CON1, 16, 3, 1);// d1p0 192M
    /* SFR(JL_LSBCLK->PRP_CON1, 16, 3, 2);// dp1dp5 128M */
    //
    /* bit_set_ie(IRQ_SPDIF_M_IDX); */
    request_irq(IRQ_SPDIF_M_IDX, 5, spdif_isr_m, 0);
    datNum = 0;

    /*  SFR(JL_PORTB->DIR, 5,  1,  0); */
    /* JL_OMAP->PB5_OUT = FO_SPDIF_DO; */

    gpio_set_mode(IO_PORT_SPILT(IO_PORTB_05), PORT_OUTPUT_LOW);
    gpio_och_sel_output_signal(IO_PORTB_05, OUTPUT_CH_SIGNAL_SPDIF_DO);


    JL_PORTB->DIR |= BIT(6);
    JL_PORTB->OUT &= ~BIT(6);
    /* JL_PORTA->OUT |=  BIT(4); */
    // ......   buffer initial  ....
    //     // ..............................................
    int i;
    //for(i=0; i<INF_BUF_LEN; i++)
    for (i = 0; i < INF_LEN; i++) {
        inf_tx_buf[i + 0 * INF_LEN] = 0xFF; // LC BUFFER0
        inf_tx_buf[i + 1 * INF_LEN] = 0xFF; //    BUFFER1
        inf_tx_buf[i + 2 * INF_LEN] = 0x00; // LU
        inf_tx_buf[i + 3 * INF_LEN] = 0x00;
        inf_tx_buf[i + 4 * INF_LEN] = 0x00; // RC
        inf_tx_buf[i + 5 * INF_LEN] = 0x00;
        inf_tx_buf[i + 6 * INF_LEN] = 0xFF; // RU
        inf_tx_buf[i + 7 * INF_LEN] = 0xFF;
    }
    for (i = 0; i < TX_LEN; i++) {
        if (i % 2 == 0)
            /* dat_tx_buf[i] = -1; */
        {
            dat_tx_buf[i] = (i << 16) + i;
        }
        /* dat_tx_buf[i] = i; */
        else
            /* dat_tx_buf[i] = 0; */
            /* dat_tx_buf[i] = i; */
        {
            dat_tx_buf[i] = (i << 16) + i;
        }
    }

    // ......   address initial  ....
    // ..............................................
    JL_SPDIF->SM_ABUFLEN = TX_LEN;
    JL_SPDIF->SM_ABADR   = (long)dat_tx_buf;
    JL_SPDIF->SM_AHPTR   = 0;
    JL_SPDIF->SM_ASPTR   = 0;
    JL_SPDIF->SM_APNS    = TX_LEN / 2;

    JL_SPDIF->SM_IBUFLEN = INF_LEN;
    JL_SPDIF->SM_IBADR   = (long)inf_tx_buf;
    JL_SPDIF->SM_IHPTR   = 0;

#if 1
    /* #define SPDIF_TX_CLK		    192 * 1000000UL */
    u32 sample_rate = 48000;
    int spdif_tx_clk = SPDIF_TX_CLK;
    int integer = spdif_tx_clk / sample_rate / 128;
    float decimal = (float)spdif_tx_clk / sample_rate / 128 - (float)(integer);
    int int_decimal;
    if (decimal > 0.5) {
        integer = integer + 1;
        int_decimal = (float)((decimal - 1) * 32768);
    } else {
        int_decimal = (float)(decimal * 32768);
    }


    SFR(JL_SPDIF->SM_CLK, 7, 9, integer);
    SFR(JL_SPDIF->SM_CLK, 16, 16, int_decimal);
#endif
    SFR(JL_SPDIF->SM_CLK,  3,  2,     0);       // div_mode
    SFR(JL_SPDIF->SM_CLK,  5,  2,     0);       // div_nsel
    /* SFR(JL_SPDIF->SM_CLK,  7,  9,     9);       // int div */
    /* SFR(JL_SPDIF->SM_CLK, 16, 16, 25087);       // frac div,  div 0.5 = 16384 */
    // ......   register initial  ....
    // ..............................................
    SFR(JL_SPDIF->SM_CON, 15,  1,  0);       // sw_v_u,
    // tx_value source, 0:from chl_r_value/chl_l_value, 1:from dual buffer
    // tx_udata source, 0:from dual buffer,             1:form chl_r_value/chl_l_value
    SFR(JL_SPDIF->SM_CON, 14,  1,  1);       // information data interrupt enable
    SFR(JL_SPDIF->SM_CON, 13,  1,  1);       // audio data interrupt enable
    SFR(JL_SPDIF->SM_CON,  9,  4,  0);       // chl_r_aux
    SFR(JL_SPDIF->SM_CON,  5,  4,  0);       // chl_l_aux
    SFR(JL_SPDIF->SM_CON,  4,  1,  1);       // chl_r_value
    SFR(JL_SPDIF->SM_CON,  3,  1,  1);       // chl_l_value
    SFR(JL_SPDIF->SM_CON,  2,  1,  1);       // aux data sel 0: chl_r / chl_l, 1:aud_tx_dat
    /* SFR(JL_SPDIF->SM_CON,  1,  1,  TX_BIT24);// 0: 24bit,  1:16bit  */
    SFR(JL_SPDIF->SM_CON,  1,  1,  TX_BIT16);// 0: 24bit,  1:16bit
    SFR(JL_SPDIF->SM_CON,  0,  1,  1);       // spdif data out enable


    SFR(JL_SPDIF->SM_CLK,  0,  1,  1);       // master enable
    /* JL_TIMER0->PRD = 0x2; */
}
#endif






