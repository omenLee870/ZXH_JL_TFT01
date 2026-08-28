#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".vad_api.bss")
#pragma data_seg(".vad_api.data")
#pragma const_seg(".vad_api.const")
#pragma code_seg(".vad_api.text")
#pragma str_literal_override(".vad_api.text")
#endif /* #ifdef SUPPORT_MS_EXTENSIONS */

#include "includes.h"
#include "sdk_config.h"
#include "gpio.h"
#include "vad_hw.h"
#include "init.h"
#if CONFIG_AEC_UART_EXPORT_ENABLE
#include "aec_uart_debug.h"
#endif

#if CONFIG_VAD_ENABLE

#define P11_VAD_POST_TO_MSYS()  (PMC_SYSTEM->P2M_INT_SET |= BIT(P2M_VAD_INDEX))
#define P11_VAD_P2M_ENABLE()  	(PMC_SYSTEM->P2M_INT_IE |= BIT(P2M_VAD_INDEX))
#define P11_VAD_P2M_DISABLE()  	(PMC_SYSTEM->P2M_INT_IE &= ~BIT(P2M_VAD_INDEX))

#if 1
#define vad_api_log	printf
#else
#define vad_api_log(...)
#endif/*log_en*/

enum P11_VAD_CMD_TABLE {
    P11_VAD_CMD_START = 0xAA,
    P11_VAD_CMD_FRAME,
    P11_VAD_CMD_STOP,
};

enum M2P_VAD_CMD_TABLE {
    M2P_VAD_CMD_INIT = 0x55,
    M2P_VAD_CMD_FRAME,
    M2P_VAD_CMD_CLOSE,
    M2P_VAD_CMD_TEST,
};

enum VAD_STATUS_TABLE {
    VAD_STATUS_CLOSE = 0,
    VAD_STATUS_IDLE,
    VAD_STATUS_ACTIVE,
};

struct vad_handle {
    u8 status;
};

static struct vad_handle vad_hdl = {0};
#define __this 			(&vad_hdl)

static void p11_vad_post_to_main_system(enum P11_VAD_CMD_TABLE cmd)
{
    P2M_MESSAGE_VAD_CMD = cmd;

    P11_VAD_POST_TO_MSYS();
}

void p11_vad_wakeup(void)
{
    __power_recover();

    __this->status = VAD_STATUS_ACTIVE;

    vad_api_log("%s, status %d\n", __func__, __this->status);
}

void p11_dvad_start(void)
{

    __power_recover();

    __this->status = VAD_STATUS_ACTIVE;

    vad_api_log("%s, status %d\n", __func__, __this->status);

    p11_vad_post_to_main_system(P11_VAD_CMD_START);
}


void p11_vad_update_frame(void)
{
    p11_vad_post_to_main_system(P11_VAD_CMD_FRAME);
}

void p11_dvad_pause(void)
{
    p11_vad_post_to_main_system(P11_VAD_CMD_STOP);
}

void p11_dvad_stop(void)
{
    __this->status = VAD_STATUS_IDLE;

    vad_api_log("%s, status %d\n", __func__, __this->status);

    p11_vad_post_to_main_system(P11_VAD_CMD_STOP);
}

static void vad_mic_platform_data_vad_api_log(struct vad_mic_platform_data *platform_data)
{
    ASSERT(platform_data);
    vad_api_log("platform_data->sr %d\n", platform_data->sr);
    vad_api_log("platform_data->vco_dc_sys %d\n", platform_data->vco_dc_sys);
    vad_api_log("platform_data->pfm_mode %d\n", platform_data->pfm_mode);
    vad_api_log("platform_data->mic_dat_sel %d\n", platform_data->mic_dat_sel);
    vad_api_log("platform_data->dcc_en %d\n", platform_data->dcc_en);
    vad_api_log("platform_data->dcc_lvl %d\n", platform_data->dcc_lvl);
    vad_api_log("platform_data->mode %d\n", platform_data->mode);
    vad_api_log("platform_data->clk_mode %d\n", platform_data->clk_mode);
    vad_api_log("platform_data->pga_gain %d\n", platform_data->pga_gain);
    vad_api_log("platform_data->micldo_pwr %d\n", platform_data->micldo_pwr);
    vad_api_log("platform_data->micldo_vsel %d\n", platform_data->micldo_vsel);
    vad_api_log("platform_data->micbias_inside %d\n", platform_data->micbias_inside);
    vad_api_log("platform_data->micbias_rsel %d\n", platform_data->micbias_rsel);
    vad_api_log("platform_data->dvad_clk_src %d\n", platform_data->dvad_clk_src);
    vad_api_log("platform_data->adc_test %d\n", platform_data->adc_test);
    vad_api_log("platform_data->dig_pvth %d\n", platform_data->dig_pvth);
    vad_api_log("platform_data->dig_nvth %d\n", platform_data->dig_nvth);
    vad_api_log("platform_data->ana_vth %d\n", platform_data->ana_vth);
    vad_api_log("platform_data->vcm_lvl %d\n", platform_data->vcm_lvl);
}
static void dvad_config_vad_api_log(struct dvad_config *dvad_cfg)
{
    ASSERT(dvad_cfg);
    vad_api_log("dvad_cfg->smooth  %d\n", dvad_cfg->smooth);
    vad_api_log("dvad_cfg->confidence %d\n", dvad_cfg->confidence);
    vad_api_log("dvad_cfg->exit_cnt_limit %d\n", dvad_cfg->exit_cnt_limit);
}

void p11_vad_init(void)
{
    P11_VAD_P2M_ENABLE();

    struct vad_mic_platform_data *platform_data = (struct vad_mic_platform_data *)vad_platform_data_addr_get();
    struct dvad_config *dvad_cfg = (struct dvad_config *)vad_dvad_algo_addr_get();
    vad_mic_platform_data_vad_api_log(platform_data);
    dvad_config_vad_api_log(dvad_cfg);
    vad_init(platform_data, dvad_cfg);

    __this->status = VAD_STATUS_IDLE;

    vad_api_log("%s, status %d\n", __func__, __this->status);
}

void p11_vad_close(void)
{
    __this->status = VAD_STATUS_CLOSE;

    vad_api_log("%s, status %d\n", __func__, __this->status);

    vad_close();
}

void p11_vad_test(void)
{
    __this->status = VAD_STATUS_ACTIVE;

    vad_api_log("%s, status %d\n", __func__, __this->status);

    vad_test();
}


void p11_vad_m2p_handle(void)
{
    u8 cmd = M2P_MESSAGE_VAD_CMD;

    /* vad_api_log("%s, cmd = 0x%x\n", __func__, cmd); */
    switch (cmd) {
    case M2P_VAD_CMD_INIT:
        p11_vad_init();
        break;
    case M2P_VAD_CMD_CLOSE:
        p11_vad_close();
        break;
    case M2P_VAD_CMD_TEST:
        p11_vad_test();
        break;
    default:
        break;
    }
    M2P_MESSAGE_VAD_PEND = 0;
}

int p11_vad_buf_init(void)
{
    vad_hw_buf_setup();
    return 0;
}
__initcall(p11_vad_buf_init);

#if CONFIG_AEC_UART_EXPORT_ENABLE
__initcall(aec_uart_init_v2);
#endif

#endif /* #if CONFIG_VAD_ENABLE */
