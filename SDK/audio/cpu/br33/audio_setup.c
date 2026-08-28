#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_setup.data.bss")
#pragma data_seg(".audio_setup.data")
#pragma const_seg(".audio_setup.text.const")
#pragma code_seg(".audio_setup.text")
#endif
/*
 ******************************************************************
 *					      Audio Setup
 *
 * Discription: 音频模块初始化，配置，调试等
 *
 * Notes:
 ******************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "sdk_config.h"
#include "audio_adc.h"
#include "media/audio_energy_detect.h"
#include "adc_file.h"
#include "linein_file.h"
#include "asm/audio_common.h"
#include "gpio_config.h"
#include "audio_demo/audio_demo.h"
#include "gpadc.h"
#include "update.h"
#include "media/audio_general.h"
#include "effects/audio_eq.h"
#include "effects/audio_fir.h"
#include "audio_spdif_master.h"
#include "media/audio_iis.h"
#include "reference_time.h"
#include "media_config.h"
#include "audio_volume_mixer.h"
#include "audio_general_config.h"

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#include "audio_dvol.h"
#endif

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice.h"
#endif /*TCFG_SMART_VOICE_ENABLE*/

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
#include "audio_vad/vad_mic.h"
#endif

struct audio_dac_hdl dac_hdl;
struct audio_adc_hdl adc_hdl;

typedef struct {
    u8 audio_inited;
    atomic_t ref;
} audio_setup_t;
audio_setup_t audio_setup = {0};
#define __this      (&audio_setup)

#if TCFG_MC_BIAS_AUTO_ADJUST
u8 mic_bias_rsel_use_save[AUDIO_ADC_MIC_MAX_NUM] = {0};
u8 save_mic_bias_rsel[AUDIO_ADC_MIC_MAX_NUM]     = {0};
u8 mic_ldo_vsel_use_save = 0;
u8 save_mic_ldo_vsel     = 0;
#endif // #if TCFG_MC_BIAS_AUTO_ADJUST

struct dac_platform_data dac_data;

int get_dac_channel_num(void)
{
    return dac_hdl.channel;
}

static u8 audio_dac_hpvdd_check()
{
    u8 res = 0;
#if 0//临时屏蔽,待整理
    u16 hpvdd = adc_get_voltage_blocking(AD_CH_AUDIO_HPVDD);
    /* printf("HPVDD: %d\n", hpvdd); */
    if (hpvdd > 1500) {
        res = DAC_HPVDD_18V;
        puts("DAC_HPVDD: 1.8V");
    } else {
        res = DAC_HPVDD_12V;
        puts("DAC_HPVDD: 1.2V");
    }
    SFR(JL_ADDA->ADDA_CON0,  0,  1,  0);					// AUDIO到SARADC的总测试通道使能
    SFR(JL_ADDA->ADDA_CON0,  3,  1,  0);					// DAC测试通道总使能
    SFR(JL_ADDA->ADDA_CON0,  4,  3,  0);					// DAC待测试信号选择位
#endif
    return res;
}

/*
 * DAC开关状态回调，需要的时候，"#if 0" 改成 "#if 1"
 */
#if 1
void audio_dac_power_state(u8 state)
{
    switch (state) {
    case DAC_ANALOG_OPEN_PREPARE:
        //DAC打开前，即准备打开
#if (TCFG_AUDIO_DAC_PA_PORT != NO_CONFIG_PORT)
#if TCFG_AUDIO_DAC_PA_MUTE_LEVEL
        gpio_set_mode(IO_PORT_SPILT(TCFG_AUDIO_DAC_PA_PORT), PORT_OUTPUT_LOW);
#else
        gpio_set_mode(IO_PORT_SPILT(TCFG_AUDIO_DAC_PA_PORT), PORT_OUTPUT_HIGH);
#endif
#endif
        printf("DAC power_on start\n");
        break;
    case DAC_ANALOG_OPEN_FINISH:
        //DAC打开后，即打开完成
        printf("DAC power_on end\n");
        break;
    case DAC_ANALOG_CLOSE_PREPARE:
        //DAC关闭前，即准备关闭
        printf("DAC power_off start\n");
        break;
    case DAC_ANALOG_CLOSE_FINISH:
        //DAC关闭后，即关闭完成
#if (TCFG_AUDIO_DAC_PA_PORT != NO_CONFIG_PORT)
#if TCFG_AUDIO_DAC_PA_MUTE_LEVEL
        gpio_set_mode(IO_PORT_SPILT(TCFG_AUDIO_DAC_PA_PORT), PORT_OUTPUT_HIGH);
#else
        gpio_set_mode(IO_PORT_SPILT(TCFG_AUDIO_DAC_PA_PORT), PORT_OUTPUT_LOW);
#endif
#endif
        printf("DAC power_off end\n");
        break;
    }
}
#endif

static void audio_common_initcall()
{
    audio_common_param_t common_param = {0};
    common_param.vbg_i_trim_value = 8;
    common_param.pmu_vbg_value = 3;
    common_param.aud_vbg_value = 3;

#if TCFG_APP_FM_EN
    // FM 模式切频点需要切 DAC 时钟频率，所以 DAC 要选数字时钟
    common_param.clock_mode = AUDIO_COMMON_CLK_DIG_SINGLE;
#else
    common_param.clock_mode = AUDIO_COMMON_CLK_DIF_XOSC;
#endif
    common_param.dsm_clks = (dac_data.dsm_clks > 3) ? (3) : (dac_data.dsm_clks);

    common_param.vcm_cap_en = TCFG_AUDIO_VCM_CAP_EN;
#if (TCFG_AUDIO_DAC_MODE == DAC_MODE_VCMO)
    common_param.power_level = 1;
#else
    common_param.power_level = TCFG_DAC_POWER_LEVEL;
#endif
    audio_common_init(&common_param);
    audio_common_clock_open();
    audio_common_param_t *common = audio_common_get_param();
    int ret;
    audio_ibias_trim_t ibias_trim = {0};
    int vbg_i_info = syscfg_read(CFG_VBG_I_TRIM, (void *)&ibias_trim.ibias_i_trim_value, sizeof(ibias_trim.ibias_i_trim_value));
    if (vbg_i_info != sizeof(ibias_trim.ibias_i_trim_value)) {
        // 校准时需要保持IOVDD = 3.0V
        vddiom_vol_sel(VDDIOM_VOL_3P0V);
        ret = audio_power_ibias_trim(&ibias_trim);
        // 校准结束还原IOVDD
        vddiom_vol_sel(TCFG_LOWPOWER_VDDIOM_LEVEL);
        if (ret == 0) {
            syscfg_write(CFG_VBG_I_TRIM, (void *)&ibias_trim.ibias_i_trim_value, sizeof(ibias_trim.ibias_i_trim_value));
        }
    }
    common->vbg_i_trim_value = ibias_trim.ibias_i_trim_value;
    printf(">>IBIAS: %d\n", common->vbg_i_trim_value);

    if (!efuse_get_cp_pass()) {
        common->pmu_vbg_value = efuse_get_audio_bg();
    } else {
        common->pmu_vbg_value = 3;
        printf("[Warning]audio vbg trim value invalid,default=%d\n", common->pmu_vbg_value);
    }
    if (!common->vcm_cap_en) {
        audio_vbg_trim_t vbg_trim = {0};
        int vbg_info = syscfg_read(CFG_VBG_TRIM, (void *)&vbg_trim, sizeof(audio_vbg_trim_t));
        if (vbg_info != sizeof(audio_vbg_trim_t)) {
            ret = audio_power_vbg_trim(&vbg_trim);
            if (ret == 0) {
                syscfg_write(CFG_VBG_TRIM, (void *)&vbg_trim, sizeof(audio_vbg_trim_t));
            }
        }
        common->aud_vbg_value = vbg_trim.aud_vbg_value;
    } else {
        common->aud_vbg_value = 0;
    }
    printf(">>VBG_TRIM 0: %d, 1: %d\n", common->pmu_vbg_value, common->aud_vbg_value);
}
void audio_dac_alink_dma_shared_init_base(u8 module_idx)
{
    if (!audio_da2alink_en) {
        return;
    }
    struct iis_file_cfg cfg = {0};
    int rlen = audio_iis_read_cfg(module_idx, &cfg);
    if (rlen == sizeof(cfg)) {
        if (cfg.hwcfg[1].en && cfg.hwcfg_adv[1].dac_2_alink_en) {

            s8 alink_bit_width = module_idx ? ALINK1_BIT_WIDTH : ALINK0_BIT_WIDTH; //
            alink_bit_width = (alink_bit_width < 0) ? audio_general_in_dev_bit_width() : alink_bit_width;
            alink_bit_width = alink_bit_width ? 1 : 0;
            //仅ch1支持
            dac_data.dac_alink[module_idx].module_idx = module_idx;
            dac_data.dac_alink[module_idx].bit_width = alink_bit_width;
            dac_data.dac_alink[module_idx].clk_close = 1;
            dac_data.dac_alink[module_idx].ch_num = 2;
            dac_data.dac_alink[module_idx].alink_work_mode = IIS_BASE;
            dac_data.dac_alink[module_idx].clk_mode = module_idx ? ALINK1_CLK_MODE : ALINK0_CLK_MODE;

            dac_data.dac_alink[module_idx].irq_points = AUDIO_IIS_IRQ_POINTS;
            dac_data.dac_alink[module_idx].sr = 0; //每次启动时会读取dac当前采样率，此处不配置
            dac_data.dac_alink[module_idx].dma_time_ms = TCFG_AUDIO_DAC_BUFFER_TIME_MS;

            u8 cnt = 0;
            for (u8 i = 0; i < 4; i++) {
                //统计alink使能通道数
                if (cfg.hwcfg[i].en) {
                    cnt++;
                }
            }
            if (cnt > 1) {
                /*
                 * 若使能了2个以上的通道，不支持Alink固定输出所有来自DAC的数据
                 * 请自行调用audio_alink_demo.c中的接口使用
                 */
                printf("dac_alink_dma_shared_en fail, alink_ch_num %d > 1\n", cnt);
            } else {
                dac_data.dac_alink_dma_shared_en |= BIT(module_idx);
            }
        }
    }
    printf("dac_alink_dma_shared: %d\n", dac_data.dac_alink_dma_shared_en);
}
void audio_dac_alink_dma_shared_init()
{
    audio_dac_alink_dma_shared_init_base(0);
    audio_dac_alink_dma_shared_init_base(1);
}

void audio_dac_initcall(void)
{
    printf("audio_dac_initcall\n");

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif
    audio_dac_alink_dma_shared_init();

    dac_data.max_sample_rate    = AUDIO_DAC_MAX_SAMPLE_RATE;
    dac_data.bit_width = audio_general_out_dev_bit_width();
    audio_dac_init(&dac_hdl, &dac_data);

#if defined(CONFIG_SOUNDBOX_CASE_ENABLE)
    dac_hdl.ng.threshold = 3;
#endif

    audio_common_audio_init((void *)(adc_hdl.mic_param), (void *)(dac_hdl.pd));

#if defined(TCFG_AUDIO_DAC_24BIT_MODE) && TCFG_AUDIO_DAC_24BIT_MODE
    audio_dac_set_bit_mode(&dac_hdl, 1);
#endif

    audio_dac_set_analog_vol(&dac_hdl, 0);

#if AUD_DAC_TRIM_ENABLE
    struct audio_dac_trim dac_trim = {0};
    int len = syscfg_read(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    if (len != sizeof(struct audio_dac_trim)) {
        if (config_audio_dac_output_mode == DAC_MODE_VCMO) {
            int trim_limit = 200;
            struct trim_init_param_t trim_init = {0};
            trim_init.precision = 1; //DAC trim的收敛精度(-precision, +precision)
            trim_init.trim_speed = 0.5f; //DAC trim的收敛速度(不建议修改)
            int ret = audio_dac_do_trim(&dac_hdl, &dac_trim, &trim_init);
            if ((ret == 0) && (__builtin_abs(dac_trim.left) < trim_limit) && (__builtin_abs(dac_trim.right) < trim_limit)) {
                puts("DAC_trim_verify:Succ");
                syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(struct audio_dac_trim));
            } else {
                puts("[Error]DAC_trim_verify:Error!!!");
                dac_trim.left = 0;
                dac_trim.right = 0;
            }
            audio_dac_close(&dac_hdl);
        } else {
            dac_trim.left = 0;
            dac_trim.right = 0;
        }
    }
    audio_dac_set_trim_value(&dac_hdl, &dac_trim);
    printf(">>DAC_TRIM L: %d, R: %d\n", dac_trim.left, dac_trim.right);
#endif
    audio_dac_set_fade_handler(&dac_hdl, NULL, audio_fade_in_fade_out);

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    /* audio_src_base_filt_init(audio_src_hw_filt, sizeof(audio_src_hw_filt)); */
}

static u8 audio_init_complete()
{
    if (!__this->audio_inited) {
        return 0;
    }
    return 1;
}

REGISTER_LP_TARGET(audio_init_lp_target) = {
    .name = "audio_init",
    .is_idle = audio_init_complete,
};

struct audio_adc_private_param adc_private_param = {
    .performance_mode = TCFG_ADC_PERFORMANCE_MODE,
    .mic_ldo_vsel   = TCFG_AUDIO_MIC_LDO_VSEL,
    .filter_type = AUD_ADC_FILTER_TYPE,
};


#if TCFG_AUDIO_ADC_ENABLE
struct adc_platform_cfg adc_platform_cfg_table[AUDIO_ADC_MAX_NUM] = {
#if TCFG_ADC0_ENABLE
    [0] = {
        .mic_mode           = TCFG_ADC0_MODE,
        .mic_ain_sel        = TCFG_ADC0_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC0_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC0_BIAS_RSEL,
        .power_io           = TCFG_ADC0_POWER_IO,
        .mic_dcc_en         = TCFG_ADC0_DCC_EN,
        .mic_dcc            = TCFG_ADC0_DCC_LEVEL,
        .cclk_en            = TCFG_ADC0_USE_CASE,
    },
#endif
#if TCFG_ADC1_ENABLE
    [1] = {
        .mic_mode           = TCFG_ADC1_MODE,
        .mic_ain_sel        = TCFG_ADC1_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC1_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC1_BIAS_RSEL,
        .power_io           = TCFG_ADC1_POWER_IO,
        .mic_dcc_en         = TCFG_ADC1_DCC_EN,
        .mic_dcc            = TCFG_ADC1_DCC_LEVEL,
        .cclk_en            = TCFG_ADC1_USE_CASE,
    },
#endif
#if TCFG_ADC2_ENABLE
    [2] = {
        .mic_mode           = TCFG_ADC2_MODE,
        .mic_ain_sel        = TCFG_ADC2_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC2_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC2_BIAS_RSEL,
        .power_io           = TCFG_ADC2_POWER_IO,
        .mic_dcc_en         = TCFG_ADC2_DCC_EN,
        .mic_dcc            = TCFG_ADC2_DCC_LEVEL,
        .cclk_en            = TCFG_ADC2_USE_CASE,
    },
#endif
#if TCFG_ADC3_ENABLE
    [3] = {
        .mic_mode           = TCFG_ADC3_MODE,
        .mic_ain_sel        = TCFG_ADC3_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC3_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC3_BIAS_RSEL,
        .power_io           = TCFG_ADC3_POWER_IO,
        .mic_dcc_en         = TCFG_ADC3_DCC_EN,
        .mic_dcc            = TCFG_ADC3_DCC_LEVEL,
        .cclk_en            = TCFG_ADC3_USE_CASE,
    },
#endif
};
struct adc_platform_cfg linein_platform_cfg_table[AUDIO_ADC_MAX_NUM] = {
#if TCFG_LINEIN0_ENABLE
    [0] = {
        .mic_mode           = TCFG_LINEIN0_MODE,
        .mic_ain_sel        = TCFG_LINEIN0_AIN_SEL,
        .mic_dcc_en         = TCFG_LINEIN0_DCC_EN,
        .mic_dcc            = TCFG_LINEIN0_DCC_LEVEL,
        .cclk_en            = 0,
    },
#endif
#if TCFG_LINEIN1_ENABLE
    [1] = {
        .mic_mode           = TCFG_LINEIN1_MODE,
        .mic_ain_sel        = TCFG_LINEIN1_AIN_SEL,
        .mic_dcc_en         = TCFG_LINEIN1_DCC_EN,
        .mic_dcc            = TCFG_LINEIN1_DCC_LEVEL,
        .cclk_en            = 0,
    },
#endif
#if TCFG_LINEIN2_ENABLE
    [2] = {
        .mic_mode           = TCFG_LINEIN2_MODE,
        .mic_ain_sel        = TCFG_LINEIN2_AIN_SEL,
        .mic_dcc_en         = TCFG_LINEIN2_DCC_EN,
        .mic_dcc            = TCFG_LINEIN2_DCC_LEVEL,
        .cclk_en            = 0,
    },
#endif
#if TCFG_LINEIN3_ENABLE
    [3] = {
        .mic_mode           = TCFG_LINEIN3_MODE,
        .mic_ain_sel        = TCFG_LINEIN3_AIN_SEL,
        .mic_dcc_en         = TCFG_LINEIN3_DCC_EN,
        .mic_dcc            = TCFG_LINEIN3_DCC_LEVEL,
        .cclk_en            = 0,
    },
#endif
};
#endif

void audio_input_initcall(void)
{
    printf("audio_input_initcall\n");

    audio_adc_init(&adc_hdl, &adc_private_param);
    adc_hdl.bit_width = audio_general_in_dev_bit_width();
    audio_adc_file_init();

#if TCFG_SUPPORT_MIC_CAPLESS
    adc_hdl.capless_trim.bias_rsel0 = TCFG_ADC0_BIAS_RSEL;
    adc_hdl.capless_trim.bias_rsel1 = TCFG_ADC1_BIAS_RSEL;
    adc_hdl.capless_trim.bias_rsel2 = TCFG_ADC2_BIAS_RSEL;
    adc_hdl.capless_trim.bias_rsel3 = TCFG_ADC3_BIAS_RSEL;
    adc_hdl.capless_param.trigger_threshold = 50; //电压差校准触发阈值(单位:mV)
    adc_hdl.capless_param.mic_online_detect = 1;   //是否使能省电容mic的检测在线功能
    adc_hdl.capless_param.mic_online_threshold = 10; //检测省电容mic两次校准电压小于多少mv就认为是不在线
    /*
     * 以下两个delay需要根据
     * const_mic_capless_open_delay_debug、const_mic_capless_trim_delay_debug 的结果配置
     */
    adc_hdl.capless_param.open_delay_ms = 200; //adc上电等待稳定延时
    adc_hdl.capless_param.trim_delay_ms = 100; //偏置调整等待稳定延时
    int ret = 0;
#if (TCFG_ADC0_ENABLE && (TCFG_ADC0_MODE == AUDIO_MIC_CAPLESS_MODE))
    adc_hdl.capless_param.mic_trim_ch |= AUDIO_ADC_MIC_0;
    adc_hdl.capless_param.mic_trim_ain[0] = TCFG_ADC0_AIN_SEL;
    adc_hdl.capless_param.mic_trim_bias[0] = TCFG_ADC0_BIAS_SEL;
#endif
#if (TCFG_ADC1_ENABLE && (TCFG_ADC1_MODE == AUDIO_MIC_CAPLESS_MODE))
    adc_hdl.capless_param.mic_trim_ch |= AUDIO_ADC_MIC_1;
    adc_hdl.capless_param.mic_trim_ain[1] = TCFG_ADC1_AIN_SEL;
    adc_hdl.capless_param.mic_trim_bias[1] = TCFG_ADC1_BIAS_SEL;
#endif
#if (TCFG_ADC2_ENABLE && (TCFG_ADC2_MODE == AUDIO_MIC_CAPLESS_MODE))
    adc_hdl.capless_param.mic_trim_ch |= AUDIO_ADC_MIC_2;
    adc_hdl.capless_param.mic_trim_ain[2] = TCFG_ADC2_AIN_SEL;
    adc_hdl.capless_param.mic_trim_bias[2] = TCFG_ADC2_BIAS_SEL;
#endif
#if (TCFG_ADC3_ENABLE && (TCFG_ADC3_MODE == AUDIO_MIC_CAPLESS_MODE))
    adc_hdl.capless_param.mic_trim_ch |= AUDIO_ADC_MIC_3;
    adc_hdl.capless_param.mic_trim_ain[3] = TCFG_ADC3_AIN_SEL;
    adc_hdl.capless_param.mic_trim_bias[3] = TCFG_ADC3_BIAS_SEL;
#endif

#if (TCFG_MC_BIAS_AUTO_ADJUST == MC_BIAS_ADJUST_ONE)
    int len = syscfg_read(CFG_MC_BIAS, &adc_hdl.capless_trim, sizeof(struct mic_capless_trim_result));
    if (len != sizeof(struct mic_capless_trim_result)) {
        ret = audio_mic_bias_adjust(&adc_hdl.capless_trim, &adc_hdl.capless_param);
        if (ret == 0) {
            syscfg_write(CFG_MC_BIAS, &adc_hdl.capless_trim, sizeof(struct mic_capless_trim_result));
        }
    }
#elif (TCFG_MC_BIAS_AUTO_ADJUST == MC_BIAS_ADJUST_ALWAYS)
    ret = audio_mic_bias_adjust(&adc_hdl.capless_trim, &adc_hdl.capless_param);
    if (ret == 0) {
        syscfg_write(CFG_MC_BIAS, &adc_hdl.capless_trim, sizeof(struct mic_capless_trim_result));
    }
#endif
    adc_platform_cfg_table[0].mic_bias_rsel = adc_hdl.capless_trim.bias_rsel0;
    adc_platform_cfg_table[1].mic_bias_rsel = adc_hdl.capless_trim.bias_rsel1;
    adc_platform_cfg_table[2].mic_bias_rsel = adc_hdl.capless_trim.bias_rsel2;
    adc_platform_cfg_table[3].mic_bias_rsel = adc_hdl.capless_trim.bias_rsel3;
    printf("BIAS_RSEL: %d, %d, %d, %d\n", adc_hdl.capless_trim.bias_rsel0, adc_hdl.capless_trim.bias_rsel1, adc_hdl.capless_trim.bias_rsel2, adc_hdl.capless_trim.bias_rsel3);
#endif

#if TCFG_AUDIO_DUT_ENABLE
    audio_dut_init();
#endif /*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_AUDIO_LINEIN_ENABLE
    audio_linein_file_init();
#endif/*TCFG_AUDIO_LINEIN_ENABLE*/

    if (const_adc_async_en) {
        //ICSD ADT启动时不可注册所有ADC_CH
#if !((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN)
        audio_all_adc_file_init();
#endif
    }

    audio_timer_init();
}

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
struct vad_mic_platform_data vad_data = {
    .sr = 16000,
    .pfm_mode = 0,
    .mic_dat_sel = 2,
    .dcc_lvl = 1,
    .mode = 1,
    .clk_mode = 1,
    .pga_gain = 10,
    .micldo_pwr = 1,
    .micldo_vsel = 0,
    .micbias_inside = 0,
    .micbias_rsel = 5,
    .dcc_en = 1,
    .vco_dc_sys = 0,
    .dvad_clk_src = 2,
    .adc_test = 0,
    .dig_pvth = 15,
    .dig_nvth = 15,
    .ana_vth = 4,
#if (TCFG_AUDIO_DAC_MODE == DAC_MODE_VCMO)
    .vcm_lvl = 1,
#else
    .vcm_lvl = TCFG_DAC_POWER_LEVEL,
#endif
};
#endif

struct dac_platform_data dac_data = {
    .performance_mode   = TCFG_DAC_PERFORMANCE_MODE,
    .power_mode         = 0,//TCFG_DAC_POWER_MODE,
    .dma_buf_time_ms    = TCFG_AUDIO_DAC_BUFFER_TIME_MS,
    .dcc_level          = 10,
    .bit_width          = DAC_BIT_WIDTH_16,
    .l_ana_gain         = TCFG_AUDIO_L_CHANNEL_GAIN,
    .r_ana_gain         = TCFG_AUDIO_R_CHANNEL_GAIN,
    .rl_ana_gain        = TCFG_AUDIO_RL_CHANNEL_GAIN,
#if (TCFG_AUDIO_DAC_MODE == DAC_MODE_VCMO)
    .dacldo_vcmsel      = 1,
    .dsm_clks           = 1,
#else
    .dacldo_vcmsel      = TCFG_DAC_POWER_LEVEL,
    .dsm_clks           = 3,
#endif
    .fifo_clks          = 0,
    .fade_en            = 0,
    .fade_points        = 0,
    .fade_volume        = 0,
    .mute_delay_isel    = 3,
    .mute_delay_time    = 50,
    .filter_type        = AUD_DAC_FILTER_TYPE,
    .src_type           = AUD_DAC_SRC_FILTER_TYPE,
    .fast_on            = 1,
#if (TCFG_DAC_PERFORMANCE_MODE == DAC_MODE_HIGH_PERFORMANCE)
    .pa_isel0           = TCFG_AUDIO_DAC_HP_PA_ISEL0,
    .pa_isel1           = TCFG_AUDIO_DAC_HP_PA_ISEL1,
#else
    .pa_isel0           = TCFG_AUDIO_DAC_LP_PA_ISEL0,
    .pa_isel1           = TCFG_AUDIO_DAC_LP_PA_ISEL1,
#endif
};

static void wl_audio_clk_on(void)
{
    /* JL_WL_AUD->CON0 = 1; */
    /*JL_ASS->CLK_CON |= BIT(0);//audio时钟*/
}

#if defined(TCFG_AUDIO_DAC_IO_ENABLE) && TCFG_AUDIO_DAC_IO_ENABLE
static void audio_dac_io_initcall()
{
    struct audio_dac_io_param param = {0};
    param.state[0] = 1;                 //左声道初始电平状态
    param.state[1] = 1;                 //右声道初始电平状态
    param.state[2] = 1;                 //RL声道初始电平状态
    param.irq_points = 256;
    param.channel = BIT(0) | BIT(1) | BIT(2);  //使能左右声道
    param.digital_gain = 32767;//16384;         //数字音量：-16384~16384
    param.ldo_volt = dac_data.dacldo_vsel;                 //电压档位：0~7
    audio_dac_io_init(&param);
}
#endif

static int audio_init()
{
    wl_audio_clk_on();

    struct volume_mixer vol_mixer = {
        .hw_dvol_max = dac_dvol_max_query,
    };
    audio_volume_mixer_init(&vol_mixer);

    audio_general_init();

#if (TCFG_DAC_NODE_ENABLE || TCFG_AUDIO_ADC_ENABLE)
    audio_common_initcall();
#endif
#if (defined(TCFG_AUDIO_DAC_IO_ENABLE) && TCFG_AUDIO_DAC_IO_ENABLE)
    audio_dac_io_initcall();
#endif
    audio_input_initcall();

#if TCFG_AUDIO_ANC_ENABLE
    anc_init();
#endif

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif


    //AC700N DAC初始化在ANC之后，因为需要读取ANC 采样率以及DAC DRC配置
#if TCFG_DAC_NODE_ENABLE
    audio_dac_initcall();
#endif

#if TCFG_EQ_ENABLE
    audio_eq_lib_init(AUDIO_EQ_MAX_SECTION);
#endif

#if TCFG_FIR_NODE_ENABLE
    fir_hardware_init(0);
#endif

    if (config_sync_msg_latch_en) {
        sync_msg_init(SYNC_MSG_NETWORK_EDR);
    }


#if TCFG_SMART_VOICE_ENABLE
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    audio_smart_voice_detect_init((void *)&vad_data);
#else
    audio_smart_voice_detect_init(NULL);
#endif
#endif /* #if TCFG_SMART_VOICE_ENABLE */
    __this->audio_inited = 1;
    return 0;
}
platform_initcall(audio_init);

static void audio_uninit()
{
#if TCFG_DAC_NODE_ENABLE
    audio_dac_close(&dac_hdl);
#endif
}
platform_uninitcall(audio_uninit);

/*关闭audio相关模块使能*/
static void audio_disable_all(void)
{
    printf("audio_disable_all\n");
#if TCFG_DAC_NODE_ENABLE
    audio_dac_close(&dac_hdl);
#endif
    //DAC:DACEN
    JL_AUD->AUD_CON0 &= ~BIT(2);
    //ADC:ADCEN
    JL_AUD->AUD_CON0 &= ~(BIT(1) | BIT(4) | BIT(9));
    //EQ:
    JL_EQ0->CON0 &= ~BIT(1);
    JL_EQ0->CON1 &= ~BIT(1);
    //FFT:
    //JL_FFT->CON = BIT(1);//置1强制关闭模块，不管是否已经运算完成
    //SRC:
    JL_SRC0->CON1 |= BIT(22);
    JL_SRC1->CON0 |= BIT(10);

    //ANC:anc_en anc_start
#if 0//build
    JL_ANC->CON0 &= ~(BIT(1) | BIT(29));
#endif

}

REGISTER_UPDATE_TARGET(audio_update_target) = {
    .name = "audio",
    .driver_close = audio_disable_all,
};

static u8 audio_common_enter_deepsleep(void)
{
    if (audio_common_clock_get_status()) {
        audio_common_clock_close();
    }
    return 0;
}

AT_VOLATILE_CACHE_CODE_LOWPOWER
static u8 audio_common_exit_deepsleep(void)
{
    return 0;
}

DEEPSLEEP_TARGET_REGISTER(audio_common) = {
    .name   = "audio_common",
    .enter  = audio_common_enter_deepsleep,
    .exit   = audio_common_exit_deepsleep,
};
