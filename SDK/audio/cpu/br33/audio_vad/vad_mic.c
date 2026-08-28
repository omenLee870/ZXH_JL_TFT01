#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".vad_mic.data.bss")
#pragma data_seg(".vad_mic.data")
#pragma const_seg(".vad_mic.text.const")
#pragma code_seg(".vad_mic.text")
#endif
/*****************************************************************
>file name : vad_mic.c
>create time : Fri 15 Apr 2022 10:27:55 AM CST
*****************************************************************/
#include "smart_voice.h"
#include "vad_mic.h"
#include "voice_mic_data.h"
#include "update/update.h"
#include "asm/audio_common.h"
#include "circular_buf.h"
#if TCFG_LOWPOWER_LOWPOWER_SEL
#include "asm/power/power_api.h"
#endif

#if CONFIG_VAD_PLATFORM_SUPPORT_EN

static DEFINE_SPINLOCK(lp_vad_lock);
struct low_power_vad_mic {
    void *priv;
    int (*dma_output)(void *, s16 *, int);
};
struct vad_mic_hdl {
    struct low_power_vad_mic *lp_vad;
    volatile u8 *vad_cbuf_struct_addr;
    volatile u8 *vad_cbuf_buf_addr;
    volatile u8 *vad_platform_data_addr;
    volatile u8 *dvad_config_addr;
};
static struct vad_mic_hdl vad_mic = {0};

static void p11_vad_mic_dma_irq_handler(void)
{
    u32 len;
    cbuffer_t *cbuffer = (cbuffer_t *)vad_mic.vad_cbuf_struct_addr;
    while (cbuf_get_data_len(cbuffer)) {
        void *ptr = cbuf_read_alloc(cbuffer, &len); //由于read_alloc获取的是连续的buf，若cbuf回到地址开头，则需要获取两次
        if (ptr && len) {
            spin_lock(&lp_vad_lock);
            if (vad_mic.lp_vad && vad_mic.lp_vad->dma_output) {
#if TCFG_SMART_VOICE_USE_AEC
                if (len > VAD_CBUF_FRAME_SIZE) {
                    //不止一帧，代表VAD唤醒，若处于播歌状态，需要对齐参考延时
                    audio_smart_voice_aec_ref_align_set();
                }
#endif
                vad_mic.lp_vad->dma_output(vad_mic.lp_vad->priv, (s16 *)ptr, len);
            }
            spin_unlock(&lp_vad_lock);
            cbuf_read_updata(cbuffer, len);  // 更新读指针，此时可能会自动回绕到 begin
        }
    }
}

/*
 * Event from IRQ
 *
 */
void audio_vad_coprocessor_event_handler(int event)
{
    if (!config_lp_vad_enable) {
        return;
    }

    int msg = SMART_VOICE_MSG_STANDBY;
    switch (event) {
    case P2M_VAD_TRIGGER_START:
        //低功耗唤醒时，该消息为第一次发送的消息，可能丢失
        msg = SMART_VOICE_MSG_WAKE;
        p11_vad_mic_dma_irq_handler();
        break;
    case P2M_VAD_TRIGGER_DMA:
        msg = SMART_VOICE_MSG_DMA;
        p11_vad_mic_dma_irq_handler();
        break;
    case P2M_VAD_TRIGGER_STOP:
        msg = SMART_VOICE_MSG_STANDBY;
        break;
    default:
        break;
    }

    smart_voice_core_post_msg(1, msg);
}

/*
 * 由主系统P11响应中断调用
 */
void audio_vad_p2mevent_irq_handler(void)
{
    audio_vad_coprocessor_event_handler(P2M_MESSAGE_VAD_CMD);
}

int lp_vad_mic_data_init(struct vad_mic_platform_data *mic_data)
{
    while (!vad_mic.vad_platform_data_addr); //若PMC初始化buf，主系统还未获取到，pend
    struct vad_mic_platform_data *data = (struct vad_mic_platform_data *)vad_mic.vad_platform_data_addr;

    ASSERT(CONFIG_P2M_VAD_PLATFORM_CONFIG_SIZE >= sizeof(struct vad_mic_platform_data)); //长度检查
    memcpy(data, mic_data, sizeof(struct vad_mic_platform_data));

    //VAD使用的IO由主系统置为高阻态
    if (data->mode == VAD_MIC_CORE_CAP_DIFF_MODE) {
        gpio_set_mode(IO_PORT_SPILT(IO_PORTA_00), PORT_HIGHZ); //P
        gpio_set_mode(IO_PORT_SPILT(IO_PORTA_01), PORT_HIGHZ); //N
        gpio_set_mode(IO_PORT_SPILT(IO_PORTA_02), PORT_HIGHZ); //LDO
    } else if (data->mode == VAD_MIC_CORE_CAP_SINGLE_MODE0) {
        gpio_set_mode(IO_PORT_SPILT(IO_PORTA_00), PORT_HIGHZ);
        if (data->micldo_pwr) {
            gpio_set_mode(IO_PORT_SPILT(IO_PORTA_02), PORT_HIGHZ);
        }
        if (data->micbias_inside) {
            gpio_set_mode(IO_PORT_SPILT(IO_PORTA_01), PORT_HIGHZ);
        }
    } else if (data->mode == VAD_MIC_CORE_CAP_SINGLE_MODE1) {
        gpio_set_mode(IO_PORT_SPILT(IO_PORTA_01), PORT_HIGHZ);
        if (data->micldo_pwr) {
            gpio_set_mode(IO_PORT_SPILT(IO_PORTA_02), PORT_HIGHZ);
        }
        if (data->micbias_inside) {
            gpio_set_mode(IO_PORT_SPILT(IO_PORTA_00), PORT_HIGHZ);
        }
    }
    return 0;
}

void *lp_vad_mic_open(void *priv, int (*dma_output)(void *priv, s16 *data, int len))
{
    if (!config_lp_vad_enable) {
        return NULL;
    }
    if (!vad_mic.lp_vad) {
        vad_mic.lp_vad = zalloc(sizeof(struct low_power_vad_mic));
        if (!vad_mic.lp_vad) {
            return NULL;
        }
    }
    P11_VAD_IRQ_ENABLE();
    /*
     * P11 VAD初始化
     */
    ASSERT(vad_mic.vad_platform_data_addr && vad_mic.dvad_config_addr && \
           vad_mic.vad_cbuf_struct_addr && vad_mic.vad_cbuf_buf_addr);
    struct dvad_config *dvad_cfg = (struct dvad_config *)vad_mic.dvad_config_addr;

    ASSERT(CONFIG_P2M_DVAD_ALGO_CONFIG_SIZE >= sizeof(struct dvad_config)); //长度检查
    //=================================//
    //       DVAD 算法参数配置         //
    //=================================//
    dvad_cfg->smooth = (VAD_CBUF_FRAME_CNT - 1);       //1~100，平滑帧数，一帧10ms
    dvad_cfg->confidence = 5000; //0~32768，越小越灵敏
    dvad_cfg->exit_cnt_limit = 10; //连续x帧检测不到人声，退出DVAD

    vad_mic.lp_vad->priv = priv;
    vad_mic.lp_vad->dma_output = dma_output;
    audio_common_power_open();
#if TCFG_LOWPOWER_LOWPOWER_SEL
    power_control(PCONTROL_LP_VAD_KEEP, 1); //低功耗保持VAD所需电源
#endif
    audio_vad_m2p_event_post(M2P_VAD_CMD_INIT);

    return vad_mic.lp_vad;
}

void lp_vad_mic_disable(void)
{
    audio_vad_m2p_event_post(M2P_VAD_CMD_CLOSE);
    audio_common_power_close();
#if TCFG_LOWPOWER_LOWPOWER_SEL
    power_control(PCONTROL_LP_VAD_KEEP, 0);
#endif
}

void lp_vad_mic_close(void *vad)
{
    if (!config_lp_vad_enable) {
        return;
    }
    if (!vad) {
        return;
    }
    lp_vad_mic_disable();
    spin_lock(&lp_vad_lock);
    if (vad) {
        free(vad);
    }
    vad_mic.lp_vad = NULL;
    spin_unlock(&lp_vad_lock);
}

void lp_vad_mic_test(void)
{
    ASSERT(vad_data.adc_test == 1); //必须使能adc_test才能测试
    audio_vad_m2p_event_post(M2P_VAD_CMD_TEST);
}

u8 vad_disable(void)
{
    lp_vad_mic_disable();
    return 0;
}

static void vad_msg_handler(void *priv, u8 *buf, u32 len)
{
    int msg[2];
    ASSERT(len <= sizeof(msg));
    memcpy(msg, buf, len);
    vad_mic.dvad_config_addr = (u8 *)msg[0];
    vad_mic.vad_platform_data_addr = vad_mic.dvad_config_addr + CONFIG_P2M_DVAD_ALGO_CONFIG_SIZE;
    vad_mic.vad_cbuf_struct_addr = vad_mic.vad_platform_data_addr + CONFIG_P2M_VAD_PLATFORM_CONFIG_SIZE;
    vad_mic.vad_cbuf_buf_addr = vad_mic.vad_cbuf_struct_addr + VAD_CBUF_STRUCT_SIZE;
    ASSERT(msg[1] == (CONFIG_VAD_CONFIG_SIZE + CONFIG_P11_CBUF_SIZE));

    printf("dvad_config 0x%x, vad_mic.vad_platform_data_addr 0x%x, vad_mic.vad_cbuf_struct_addr 0x%x, vad_mic.vad_cbuf_buf_addr 0x%x, buf_len %d\n",
           (u32)vad_mic.dvad_config_addr, (u32)vad_mic.vad_platform_data_addr, (u32)vad_mic.vad_cbuf_struct_addr, (u32)vad_mic.vad_cbuf_buf_addr, msg[1]);
}

REGISTER_P2M_MSG_HANDLER(NULL, MSG_VAD, vad_msg_handler);


REGISTER_UPDATE_TARGET(vad_update_target) = {
    .name = "vad",
    .driver_close = (void *)vad_disable,
};

#endif
