#include "product_test.h"
#include "pt_speaker_mic.h"
#include "audio_config.h"
#include "asm/audio_adc.h"
#include "tone_player.h"
#include "effects/spectrum/spectrum_fft.h"
#include "clock_manager/clock_manager.h"
#define LOG_TAG_CONST     		PRODUCT_TEST
#define LOG_TAG     		"[PRODUCT_TEST]"
#define log_ERROR_ENABLE
#define log_DEBUG_ENABLE
#define log_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if PT_SPEAKER_MIC_ENABLE
//================================================================================//
//			缺spectrum库，处理中
//================================================================================//
#define PT_SPEAKER_MIC_USE_SPECTRUM			1	// 频谱判断
#define PT_SPEAKER_MIC_SPRCTRUM_DEBUG		0	// 频谱打印
#define ADC_DEMO_CH_NUM        	1	/*支持的最大采样通道(max = 2)*/
#define ADC_DEMO_BUF_NUM        2	/*采样buf数*/
#define ADC_DEMO_IRQ_POINTS     256	/*采样中断点数*/
#define ADC_DEMO_BUFS_SIZE      (ADC_DEMO_CH_NUM * ADC_DEMO_BUF_NUM * ADC_DEMO_IRQ_POINTS)

#if TCFG_TONE_EN_ENABLE
#define FILE_FOLDER	"tone_en/"
#else
#define FILE_FOLDER "tone_zn/"
#endif

extern struct audio_adc_hdl adc_hdl;
extern const struct adc_platform_cfg adc_platform_cfg_table[AUDIO_ADC_MAX_NUM];


struct pt_sin_info {
    const char *file_name;
#if PT_SPEAKER_MIC_USE_SPECTRUM
    u8  spectrum_num;	// 频谱点
    u8  spectrum_num_contrast;	// 对比频谱点
    u8  spectrum_diff_val;	// 频谱差值
#else
#error "需要频谱判断"
#endif /* #if PT_SPEAKER_MIC_USE_SPECTRUM*/
};

/*开频谱打印，
  在db_data_db[i]中取两个相差5以上的点，
  作为频谱点和对比频谱点
  以实际硬件环境测试的打印为准
*/
static struct pt_sin_info pt_sin_table[] = {
#if PT_SPEAKER_MIC_USE_SPECTRUM
    {"sine500.*", 0x04, 0x07, 0x05},
    {"sine1k.*", 0x04, 0x07, 0x05},
    {"sine3k2.*", 0x04, 0x07, 0x05},
#else /* #if PT_SPEAKER_MIC_USE_SPECTRUM*/
    {"sine500.*"},
    {"sine1k.*"},
    {"sine3k2.*"},
#endif /* #if PT_SPEAKER_MIC_USE_SPECTRUM*/
};

struct pt_speaker_mic {
    // 解码
    u16 dec_idx;	// 当前index
    u16 once_ms;	// 执行一次的时间
    u16 to_id;		// 超时

    // 编码
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch    mic_ch;
    /* struct audio_adc_hdl adc_hdl; */
    int mic_sr;
    u8  mic_gain;
    s16 *adc_buf;

    // 频谱
#if PT_SPEAKER_MIC_USE_SPECTRUM
    u8  spectrum_num;	// 频谱点
    u8  spectrum_num_contrast;	// 对比频谱点
    u8  spectrum_diff_val;	// 频谱差值
    u16 spectrum_cnt;
    spectrum_fft_hdl *spec_hdl;
#endif

    u8  status;		// 状态标志。0-idle，1-run，2-wait close
    u8  res;
    u8  mic_status;
};

static struct pt_speaker_mic pt_spk_mic = {
    .once_ms = 3000,
    .mic_gain = 7,
    .mic_sr  = 44100,
    .mic_status = 0,
};


static void pt_speaker_mic_play_next(void *priv);

int pt_speaker_mic_init(void)
{
    return 0;
}

#if PT_SPEAKER_MIC_USE_SPECTRUM
static void spectrum_check(void)
{
    if (pt_spk_mic.spec_hdl) {
        if (pt_spk_mic.spectrum_cnt == 0) {
            return ;
        }
        int check = 0;
        u8 db_num = audio_spectrum_fft_get_num(pt_spk_mic.spec_hdl);//获取频谱个数
        short *db_data = audio_spectrum_fft_get_val(pt_spk_mic.spec_hdl);//获取存储频谱值得地址
        if (!db_data) {
            return;
        }
        local_irq_disable();
        if (pt_spk_mic.spectrum_cnt && (db_num > pt_spk_mic.spectrum_num) && (db_num > pt_spk_mic.spectrum_num_contrast)) {
            s16 diff_val = db_data[pt_spk_mic.spectrum_num] - db_data[pt_spk_mic.spectrum_num_contrast];
            log_debug("[%d %d %d %d]", db_data[pt_spk_mic.spectrum_num], db_data[pt_spk_mic.spectrum_num_contrast], diff_val, pt_spk_mic.spectrum_diff_val);
            if (diff_val > pt_spk_mic.spectrum_diff_val) {

                pt_spk_mic.spectrum_cnt--;
            }
        }
        local_irq_enable();
        log_debug("%s idx:%d res:%d cnt:%d ", __func__, pt_spk_mic.dec_idx, check, pt_spk_mic.spectrum_cnt);
#if PT_SPEAKER_MIC_SPRCTRUM_DEBUG
        {
            static u8 cnt = 0;
            cnt ++;
            if (cnt < 20) {
                return ;
            }
            cnt = 0;
            putchar('\n');
            for (int i = 0; i < db_num; i++) {
                //输出db_num个 db值
                log_debug("db_data db[%d] %d\n", i, db_data[i]);
                /* log_debug_hexdump(db_data[i]); */
            }
            putchar('\n');
        }
#endif
    }
}
#endif/*PT_SPEAKER_MIC_USE_SPECTRUM*/

static void mic_output_data(void *priv, s16 *data, int len)
{
    /* put_buf((u8*)data,32); */
#if PT_SPEAKER_MIC_USE_SPECTRUM
    if (pt_spk_mic.spec_hdl) {
        audio_spectrum_fft_run(pt_spk_mic.spec_hdl, data, len);
        spectrum_check();
    }
#endif/*PT_SPEAKER_MIC_USE_SPECTRUM*/
}

static int pt_speaker_mic_enc_close(void)
{
    if (!pt_spk_mic.mic_status) {
        return -1;
    }
    audio_adc_del_output_handler(&adc_hdl, &pt_spk_mic.adc_output);
    audio_adc_mic_close(&pt_spk_mic.mic_ch);
#if PT_SPEAKER_MIC_USE_SPECTRUM
    if (pt_spk_mic.spec_hdl) {
        audio_spectrum_fft_close(pt_spk_mic.spec_hdl);
        pt_spk_mic.spec_hdl = NULL;
    }
    clock_free("spectrum");
#endif/*PT_SPEAKER_MIC_USE_SPECTRUM*/
    pt_spk_mic.mic_status = 0;
    return 0;
}

static int pt_speaker_mic_enc_open(void)
{
    if (pt_spk_mic.mic_status) {
        return -1;
    }
    pt_speaker_mic_enc_close();


    if (!pt_spk_mic.adc_buf) {
        pt_spk_mic.adc_buf = zalloc(ADC_DEMO_BUFS_SIZE << 2);
    }
    struct mic_open_param mic_param[3] = {0};
    struct adc_platform_cfg *platform_cfg = (struct adc_platform_cfg *)adc_platform_cfg_table;
    //step0:设置mic通道采样率,LPADC会根据采样率分频，需要在mic_open前配置采样率
    audio_adc_mic_set_sample_rate(&pt_spk_mic.mic_ch, pt_spk_mic.mic_sr);
    //step1:打开mic通道，并设置增益
    audio_adc_param_fill(&mic_param[0], &platform_cfg[0]);
    audio_adc_mic_open(&pt_spk_mic.mic_ch, AUDIO_ADC_MIC_0, &adc_hdl, &mic_param[0]);
    audio_adc_mic_set_gain(&pt_spk_mic.mic_ch, AUDIO_ADC_MIC_0, pt_spk_mic.mic_gain);
    //step2:设置mic采样buf
    audio_adc_mic_set_buffs(&pt_spk_mic.mic_ch, pt_spk_mic.adc_buf, ADC_DEMO_IRQ_POINTS * 2, ADC_DEMO_BUF_NUM);
    audio_adc_set_buf_fix(0, &adc_hdl);
    //step3:设置mic采样输出回调函数
    pt_spk_mic.adc_output.handler = mic_output_data;
    pt_spk_mic.adc_output.priv    = &pt_spk_mic;
    audio_adc_add_output_handler(&adc_hdl, &pt_spk_mic.adc_output);
    //step4:启动mic通道采样
    audio_adc_mic_start(&pt_spk_mic.mic_ch);
#if PT_SPEAKER_MIC_USE_SPECTRUM
    spectrum_fft_open_parm parm = {0};
    parm.param.SampleRate = pt_spk_mic.mic_sr;
    parm.param.channel = 1;
    parm.param.attackFactor = 0.9;
    parm.param.releaseFactor = 0.9;
    parm.param.mode = 2;
    pt_spk_mic.spec_hdl = audio_spectrum_fft_open(&parm);

    clock_alloc("spectrum", 8 * MHz);
#endif/* #if PT_SPEAKER_MIC_USE_SPECTRUM*/
    pt_spk_mic.mic_status = 1;
    return false;
}

static int pt_speaker_mic_play_close(void)
{
    if (pt_spk_mic.to_id) {
        sys_timeout_del(pt_spk_mic.to_id);
        pt_spk_mic.to_id = 0;
    }

    tone_player_stop();
    return 0;
}
static int pt_speaker_mic_play_open(void)
{
    int ret;
    pt_speaker_mic_play_close();
    char file_path[32];
    strcpy(file_path, FILE_FOLDER);
    strcpy(file_path + strlen(FILE_FOLDER), pt_sin_table[pt_spk_mic.dec_idx].file_name);

#if PT_SPEAKER_MIC_USE_SPECTRUM
    pt_spk_mic.spectrum_num = pt_sin_table[pt_spk_mic.dec_idx].spectrum_num;
    pt_spk_mic.spectrum_num_contrast = pt_sin_table[pt_spk_mic.dec_idx].spectrum_num_contrast;
    pt_spk_mic.spectrum_diff_val = pt_sin_table[pt_spk_mic.dec_idx].spectrum_diff_val;
    pt_spk_mic.spectrum_cnt = 100;
#endif /* #if PT_SPEAKER_MIC_USE_SPECTRUM*/
    pt_spk_mic.dec_idx++;
    ret = play_tone_file(file_path);
    if (!ret) {
        pt_spk_mic.to_id = sys_timeout_add(NULL, pt_speaker_mic_play_next, pt_spk_mic.once_ms);
        log_info("play idx:%d \n", pt_spk_mic.dec_idx);
        return true;
    }
    log_error("play err");
    return false;
}
static void pt_speaker_mic_play_next(void *priv)
{
    log_info("play next \n");

    u32 result = PT_E_OK;
    if (pt_spk_mic.status != 1) {
        return ;
    }
#if PT_SPEAKER_MIC_USE_SPECTRUM
    if (pt_spk_mic.spectrum_cnt) {
        log_info("spectrum err\n");
        result = PT_E_MOD_TEST_ERROR;
        goto __end;
    }
#endif/*PT_SPEAKER_MIC_USE_SPECTRUM*/
    if (pt_spk_mic.dec_idx >= ARRAY_SIZE(pt_sin_table)) {
        if (tone_player_runing()) {
            // test end
            log_info("play end \n");
            goto __end;
        }
        return ;
    }
    int ret = pt_speaker_mic_play_open();
    if (ret == false) {
        // test err
        result = PT_E_MOD_TEST_ERROR;
        goto __end;
    }
    return ;

__end:
    pt_speaker_mic_play_close();
    pt_speaker_mic_enc_close();
    pt_spk_mic.res = result;
}

static int pt_speaker_mic_test_start(int priv)
{
    log_info("play test \n");
    tone_player_stop();

    pt_spk_mic.dec_idx = 0;

    int ret;
    u32 result = PT_E_OK;
    ret = pt_speaker_mic_enc_open();
    if (ret == false) {
    }
    ret = pt_speaker_mic_play_open();
    if (ret == false) {
        pt_speaker_mic_enc_close();
        pt_speaker_mic_play_close();
        result = PT_E_MOD_TEST_ERROR;
        pt_spk_mic.res = result;
    }
    return 0;
}
static int pt_speaker_mic_test_stop(int priv)
{
    log_info("play test stop \n");
    pt_speaker_mic_play_close();
    pt_speaker_mic_enc_close();
    if (pt_spk_mic.adc_buf) {
        /* free(pt_spk_mic.adc_buf);//由adc_close释放 */
        pt_spk_mic.adc_buf = NULL;
    }
    pt_spk_mic.status = 0;
    return 0;
}

int pt_speaker_mic_start(void)
{
    if (pt_spk_mic.status) {
        return PT_E_MOD_RUN;
    }
    pt_spk_mic.res = PT_E_MOD_RUN;
    int msg[3] = {0};
    msg[0] = (int)pt_speaker_mic_test_start;
    msg[1] = 1;
    msg[2] = (int)0;
    do {
        int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        if (os_err == OS_ERR_NONE) {
            break;
        }
        if (os_err != OS_Q_FULL) {
            pt_spk_mic.res = PT_E_SYS_ERROR;
            return PT_E_SYS_ERROR;
        }
        os_time_dly(1);
    } while (1);

    pt_spk_mic.status = 1;

    return 0;
}

int pt_speaker_mic_stop(void)
{
    if (pt_spk_mic.status) {
        pt_spk_mic.status = 2;
        int msg[3] = {0};
        msg[0] = (int)pt_speaker_mic_test_stop;
        msg[1] = 1;
        msg[2] = (int)0;
        do {
            int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
            if (os_err == OS_ERR_NONE) {
                break;
            }
            if (os_err != OS_Q_FULL) {
                if (pt_spk_mic.res == PT_E_MOD_RUN) {
                    pt_spk_mic.res = PT_E_MOD_STOP_NO_END;
                }
                return PT_E_MOD_CANT_STOP;//PT_E_SYS_ERROR;
            }
            os_time_dly(1);
        } while (1);
    }
    if (pt_spk_mic.res == PT_E_MOD_RUN) {
        pt_spk_mic.res = PT_E_MOD_STOP_NO_END;
    }
    return 0;
}

int pt_speaker_mic_ioctrl(u32 order, int len, void *param)
{
    u32 result = 0;
    switch (PT_ORDER_C_GET(order)) {
    case PT_N_C_START:
        result = pt_speaker_mic_start();
        break;
    case PT_N_C_STOP:
        result = pt_speaker_mic_stop();
        break;
    case PT_N_C_GET_RESULT:
        result = pt_spk_mic.res;
        break;
    default:
        result = PT_E_PARAM;
        break;
    }
    product_test_push_data(order, 4, (u8 *)&result);
    return result;
}


REGISTER_PT_MODULE(speaker_mic) = {
    .module = PT_M_SPEAKER_MIC,
    .attr	= PT_ATTR_SELF | PT_ATTR_ENV_OUT | PT_ATTR_ENV_IN,
    .init	= pt_speaker_mic_init,
    .ioctrl	= pt_speaker_mic_ioctrl,
};


void pt_speaker_mic_test(void)
{
    pt_speaker_mic_init();
    pt_speaker_mic_start();
}
int pt_spk_simulation_test()
{

    printf("%s %d", __func__, __LINE__);
    pt_spk_mic.status = 1;
    int ret = pt_speaker_mic_test_start(0);
    ASSERT(!ret, "%s result:%d", __func__, ret);
    return ret;
}
#endif /* #if PT_SPEAKER_MIC_ENABLE */


