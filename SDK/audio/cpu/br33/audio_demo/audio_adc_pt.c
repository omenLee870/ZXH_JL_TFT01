/*
 ****************************************************************************
 *							Audio ADC Demo
 *
 *Description	: Audio ADC使用范例
 *Notes			: (1)本demo为开发测试范例，请不要修改该demo， 如有需求，请自行
 *				  复制再修改
 *				  (2)mic工作模式说明
 *				A.单端隔直(cap_mode)
 *				  可以选择mic供电方式：外部供电还是内部供电(mic_bias_inside = 1)
 *				B.单端省电容(capless_mode)
 *				C.差分模式(cap_diff_mode)
 ****************************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_main.h"
#include "audio_config.h"
#include "audio_demo.h"
#include "audio_adc.h"

/* #define LOG_TAG_CONST       AUDIO_APP */
#define LOG_TAG     		"[AUD APP adc]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if 1
#define ADC_DEMO_LOG	log_info//printf
#else
#define ADC_DEMO_LOG(...)
#endif

extern struct audio_dac_hdl dac_hdl;
extern struct audio_adc_hdl adc_hdl;

#define ADC_DEMO_CH_NUM        	2	/*支持的最大采样通道(max = 2)*/
#define ADC_DEMO_BUF_NUM        2	/*采样buf数*/
#define ADC_DEMO_IRQ_POINTS     256	/*采样中断点数*/
#define ADC_DEMO_BUFS_SIZE      (ADC_DEMO_CH_NUM * ADC_DEMO_BUF_NUM * ADC_DEMO_IRQ_POINTS*2)

#define ADC_DEMO_TASK_NAME "aud_adc_demo"

struct adc_demo {
    u8 adc_2_dac;
    u8 mic_idx;
    u8 linein_idx;
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    struct adc_linein_ch linein_ch;
    s16 *adc_buf;
    u8 linein_ch_num;
    u8 linein_ch_sel;
    OS_SEM sem;
    cbuffer_t cbuf;
    u8 c_buf[ADC_DEMO_BUFS_SIZE << 2];
    s16 runbuf[ADC_DEMO_IRQ_POINTS << 2];
};
static struct adc_demo *mic_demo = NULL;
static struct adc_demo *linein_demo = NULL;
extern const struct adc_platform_cfg adc_platform_cfg_table[AUDIO_ADC_MAX_NUM];

/*
*********************************************************************
*                  AUDIO MIC OUTPUT
* Description: mic采样数据回调
* Arguments  : pric 私有参数
*			   data	mic数据
*			   len	数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : 单声道数据格式
*			   L0 L1 L2 L3 L4 ...
*				双声道数据格式()
*			   L0 R0 L1 R1 L2 R2...
*********************************************************************
*/

#define ADC_BITWIDTH 1
#define ADC_DEBUG_BUF       (0)
#if ADC_BITWIDTH
#define ADC_DEBUF_SIZE      (64000)
#else
#define ADC_DEBUF_SIZE      (32000)
#endif


#if ADC_DEBUG_BUF
u32 pt_d_skip_cnt = 1000;
#if ADC_BITWIDTH
s32 pt_d_buf[ADC_DEBUF_SIZE / 2] = {0};
#else
s16 pt_d_buf[ADC_DEBUF_SIZE / 2] = {0};
#endif
u8 *d_ptr = (u8 *)pt_d_buf;
u32 d_buf_cnt = 0;
#endif
u8 pt_mic_num = 0;
s16 *pt_ptr16 = NULL;
s32 *pt_ptr32 = NULL;
s16 *pt_temp16 = NULL;
s32 *pt_temp32 = NULL;
static void mic_demo_task_data_write(void *data, int len);

static void adc_mic_demo_output(void *priv, s16 *data, int len)
{
    if (adc_hdl.bit_width) {
        pt_ptr32 = (s32 *) data;
    } else {
        pt_ptr16 =  data;
    }
    int i;
    if (mic_demo && mic_demo->adc_2_dac) {
        if (pt_mic_num == 1) {
            mic_demo_task_data_write(data, len * 1);
        } else {
            mic_demo_task_data_write(data, len * 2);
        }

        return;
        if (adc_hdl.channel_num >= 2) {
#if 0//(TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
            /*输出两个mic的数据到DAC两个通道,默认输出第一个和第二个采样通道的数据*/
            log_char('a');
            if (adc_hdl.bit_width) {
                for (i = 0; i < len / 4; i++) {
                    pt_temp32[2 * i] = pt_ptr32[adc_hdl.channel_num * i];
                    pt_temp32[2 * i + 1] = pt_ptr32[adc_hdl.channel_num * i + 1];
                }
                mic_demo_task_data_write(pt_temp32, len * 2);
            } else {
                for (i = 0; i < len / 2; i++) {
                    pt_temp16[2 * i] = pt_ptr16[adc_hdl.channel_num * i];
                    pt_temp16[2 * i + 1] = pt_ptr16[adc_hdl.channel_num * i + 1];
                }
                mic_demo_task_data_write(pt_temp16, len * 2);
            }
#else
            /*输出其中一个mic的数据到DAC一个通道*/
            log_char('b');
            u8 mic_ch_sel = 0;
            if (adc_hdl.bit_width) {
                for (i = 0; i < (len / 4); i++) {
                    switch (mic_ch_sel) {
                    case 0:
                        pt_temp32[i] = pt_ptr32[i * adc_hdl.channel_num];			/*第一路 mic采样的数据*/
                        break;
                    case 1:
                        pt_temp32[i] = pt_ptr32[i * adc_hdl.channel_num + 1];	     /*第一路 mic采样的数据*/
                        break;
                    case 2:
                        pt_temp32[i] = pt_ptr32[i * adc_hdl.channel_num + 2];		 /*第三路 mic采样的数据*/
                        break;
                    case 3:
                        pt_temp32[i] = pt_ptr32[i * adc_hdl.channel_num + 3];	     /*第四路 mic采样的数据*/
                        break;
                    default:
                        break;
                    }
                }
                mic_demo_task_data_write(pt_temp32, len);
            } else {
                for (i = 0; i < (len / 2); i++) {
                    switch (mic_ch_sel) {
                    case 0:
                        pt_temp16[i] = pt_ptr16[i * adc_hdl.channel_num];			/*第一路 mic采样的数据*/
                        break;
                    case 1:
                        pt_temp16[i] = pt_ptr16[i * adc_hdl.channel_num + 1];	     /*第一路 mic采样的数据*/
                        break;
                    case 2:
                        pt_temp16[i] = pt_ptr16[i * adc_hdl.channel_num + 2];		 /*第三路 mic采样的数据*/
                        break;
                    case 3:
                        pt_temp16[i] = pt_ptr16[i * adc_hdl.channel_num + 3];	     /*第四路 mic采样的数据*/
                        break;
                    default:
                        break;
                    }
                }
                mic_demo_task_data_write(pt_temp16, len);
            }
#endif/*TCFG_AUDIO_DAC_CONNECT_MODE*/
        } else {
#if 0//(TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
            /*输出一个mic的数据到DAC两个通道*/
            log_char('c');
            if (adc_hdl.bit_width) {
                for (i = 0; i < len / 2; i++) {
                    pt_temp32[i] = pt_ptr32[i / 2];
                }
                mic_demo_task_data_write(pt_temp32, len * 2);
            } else {
                for (i = 0; i < len; i++) {
                    pt_temp16[i] = pt_ptr16[i / 2];
                }
                mic_demo_task_data_write(pt_temp16, len * 2);
            }
#else
            /*输出一个mic的数据到DAC一个通道*/
            /* log_char('d'); */
            mic_demo_task_data_write(data, len);
#endif
        }
    }

#if ADC_DEBUG_BUF
    wdt_close();
    if (pt_d_skip_cnt) {
        pt_d_skip_cnt--;
        return;
    }

    d_buf_cnt += len * ADC_DEMO_CH_NUM;
    if (d_buf_cnt >= ADC_DEBUF_SIZE) {
        wdt_close();
        local_irq_disable();
        for (i = 0; i < ((d_buf_cnt - len) >> (1 + adc_hdl.bit_width)); i++) {
            printf("%d\n", pt_d_buf[i]);
        }
        while (1);
        pt_d_skip_cnt = 1000;
        d_ptr = (u8 *)pt_d_buf;
        d_buf_cnt = 0;
    }
    memcpy(d_ptr, data, len * ADC_DEMO_CH_NUM);
    d_ptr += len * ADC_DEMO_CH_NUM;
#endif
}

static void mic_demo_task_data_write(void *data, int len)
{
    void pcm_single_to_dual(void *out, void *in, u16 len);
    /* u8 *audio_buf=zalloc(1024*2); */
    /* pcm_single_to_dual(audio_buf,data,len); */
    int wlen = cbuf_write(&mic_demo->cbuf, data, len);
    if (wlen == len) {
        os_sem_post(&mic_demo->sem);
    } else {
        printf("mic_demo cbuf write  err %d, %d", wlen, len);
    }
    /* free(audio_buf); */
}

static void mic_demo_task(void *priv)
{
    int rlen;
    int wlen = 0;
    extern struct audio_dac_hdl dac_hdl;
    while (1) {
        os_sem_pend(&mic_demo->sem, 0);
        if (pt_mic_num == 2) {
            rlen = cbuf_read(&mic_demo->cbuf, mic_demo->runbuf, ADC_DEMO_IRQ_POINTS << 2);
            if (rlen == (ADC_DEMO_IRQ_POINTS << 2)) {
                putchar('w');
                wlen = audio_dac_channel_write(NULL, (void *)mic_demo->runbuf, rlen);
                // printf("output write err86 %d  %d", wlen, rlen);
                if (wlen != rlen) {
                    printf("output write err1 %d  %d", wlen, rlen);
                }

            } else {
                printf("mic demo cbuf read fail");
            }
        }


        else if (pt_mic_num == 1 && dac_hdl.channel == 1) {
            rlen = cbuf_read(&mic_demo->cbuf, mic_demo->runbuf, ADC_DEMO_IRQ_POINTS << 1);
            if (rlen == (ADC_DEMO_IRQ_POINTS << 1)) {
                putchar('w');
                wlen = audio_dac_channel_write(NULL, (void *)mic_demo->runbuf, rlen);
                if (wlen != rlen) {
                    printf("output write err2 %d  %d", wlen, rlen);
                }

            } else {
                printf("mic demo cbuf read fail");
            }
        } else if (pt_mic_num == 1 && dac_hdl.channel == 2) {


            u8 *dual_buf = zalloc(ADC_DEMO_IRQ_POINTS << 2);
            rlen = cbuf_read(&mic_demo->cbuf, mic_demo->runbuf, ADC_DEMO_IRQ_POINTS << 1);
            void pcm_single_to_dual(void *out, void *in, u16 len);
            pcm_single_to_dual(dual_buf, mic_demo->runbuf, ADC_DEMO_IRQ_POINTS << 1);
            if (rlen == (ADC_DEMO_IRQ_POINTS << 1)) {
                putchar('w');
                wlen = audio_dac_channel_write(NULL, (void *)dual_buf, rlen << 1);
                // printf("output write err89 %d  %d", wlen, rlen);
                free(dual_buf);
                if (wlen != (rlen << 1)) {
                    printf("output write err3 %d  %d", wlen, rlen);
                }

            } else {
                printf("mic demo cbuf read fail");
            }
        }

    }
}

/*
*********************************************************************
*                  AUDIO ADC MIC OPEN
* Description: 打开mic通道
* Arguments  : mic_idx 		mic通道
*			   gain			mic增益
*			   sr			mic采样率
*			   mic_2_dac 	mic数据（通过DAC）监听
* Return	 : None.
* Note(s)    : (1)打开一个mic通道示例：
*				audio_adc_mic_demo_open(AUDIO_ADC_MIC_0,10,16000,1);
*				或者
*				audio_adc_mic_demo_open(AUDIO_ADC_MIC_1,10,16000,1);
*			   (2)打开两个mic通道示例：
*				audio_adc_mic_demo_open(AUDIO_ADC_MIC_1|AUDIO_ADC_MIC_0,10,16000,1);
*********************************************************************
*/

void pt_audio_adc_mic_demo_open(u8 mic_idx, u8 gain, int sr, u8 mic_2_dac)
{

    gpio_set_mode(IO_PORT_SPILT(IO_PORTH_08), PORT_OUTPUT_HIGH);
    ADC_DEMO_LOG("audio_adc_mic_demo open:%d,gain:%d,sr:%d,mic_2_dac:%d\n", mic_idx, gain, sr, mic_2_dac);
    mic_demo = zalloc(sizeof(struct adc_demo));
    ASSERT(mic_demo);
    mic_demo->adc_buf = zalloc(ADC_DEMO_BUFS_SIZE << (2 + adc_hdl.bit_width));
    ASSERT(mic_demo->adc_buf);

    /*监听配置（可选）*/
    mic_demo->adc_2_dac = mic_2_dac;
    mic_demo->mic_idx = mic_idx;

    if (mic_2_dac) {
        app_audio_state_switch(APP_AUDIO_STATE_MUSIC, app_audio_volume_max_query(AppVol_BT_MUSIC), NULL);	// 音量状态设置
        audio_dac_set_volume(&dac_hdl, app_audio_volume_max_query(AppVol_BT_MUSIC));					// dac 音量设置
        audio_dac_set_sample_rate(&dac_hdl, sr);							// 采样率设置
        audio_dac_channel_start(NULL);
        /* JL_AUDDAC->DAC_VL0 = 0x40004000; */
        s16 *zero_data = zalloc(60);
        for (int i = 0; i < 100; i++) {
            audio_dac_channel_write(NULL, zero_data, 60);
        }
        free(zero_data);
    }
    if (adc_hdl.bit_width) {
        pt_temp32 = zalloc(ADC_DEMO_IRQ_POINTS * 4 * 2);
    } else {
        pt_temp16 = zalloc(ADC_DEMO_IRQ_POINTS * 2 * 2);
    }

    os_sem_create(&mic_demo->sem, 0);
    cbuf_init(&mic_demo->cbuf, mic_demo->c_buf, sizeof(mic_demo->c_buf));
    task_create(mic_demo_task, mic_demo, ADC_DEMO_TASK_NAME);

    //step0:打开mic通道，并设置增益
    //step1:设置mic通道采样率,LPADC会根据采样率分频，需要在mic_open前配置采样率
    audio_adc_mic_set_sample_rate(&mic_demo->mic_ch, sr);

#if 0   // use config file
    extern void audio_adc_file_init(void);
    extern int adc_file_mic_open(struct adc_mic_ch * mic, int ch);
    audio_adc_file_init();
    adc_file_mic_open(&mic_demo->mic_ch, AUDIO_ADC_MIC_0);
#else
    struct mic_open_param mic_param[3] = {0};
    struct adc_platform_cfg *platform_cfg = (struct adc_platform_cfg *)adc_platform_cfg_table;
    pt_mic_num = 0;
    if (mic_idx & AUDIO_ADC_MIC_0) {
        audio_adc_param_fill(&mic_param[0], &platform_cfg[0]);
        mic_param[0].pga_gain = gain;
        audio_adc_mic_open(&mic_demo->mic_ch, AUDIO_ADC_MIC_0, &adc_hdl, &mic_param[0]);
        pt_mic_num++;
    }
    if (mic_idx & AUDIO_ADC_MIC_1) {
        audio_adc_param_fill(&mic_param[1], &platform_cfg[1]);
        mic_param[1].pga_gain = gain;
        audio_adc_mic_open(&mic_demo->mic_ch, AUDIO_ADC_MIC_1, &adc_hdl, &mic_param[1]);
        pt_mic_num++;
    }
    if (mic_idx & AUDIO_ADC_MIC_2) {
        audio_adc_param_fill(&mic_param[2], &platform_cfg[2]);
        mic_param[2].pga_gain = gain;
        audio_adc_mic_open(&mic_demo->mic_ch, AUDIO_ADC_MIC_2, &adc_hdl, &mic_param[2]);
        pt_mic_num++;
    }
    if (mic_idx & AUDIO_ADC_MIC_3) {
        audio_adc_param_fill(&mic_param[3], &platform_cfg[3]);
        mic_param[3].pga_gain = gain;
        audio_adc_mic_open(&mic_demo->mic_ch, AUDIO_ADC_MIC_3, &adc_hdl, &mic_param[3]);
        pt_mic_num++;
    }

#endif
    if (pt_mic_num > 2) {
        ASSERT("not support over 2 mic");
    }
    //step2:设置mic采样buf
    audio_adc_mic_set_buffs(&mic_demo->mic_ch, mic_demo->adc_buf, ADC_DEMO_IRQ_POINTS * 2, ADC_DEMO_BUF_NUM);
    //step3:设置mic采样输出回调函数
    mic_demo->adc_output.handler = adc_mic_demo_output;
    audio_adc_add_output_handler(&adc_hdl, &mic_demo->adc_output);
    //step4:启动mic通道采样
    audio_adc_mic_start(&mic_demo->mic_ch);
    void audio_adda_dump(void); //打印所有的dac,adc寄存器
    /* audio_adda_dump(); */
    ADC_DEMO_LOG("audio_adc_mic_demo start succ\n");

    //	循环一直往dac写数据
#ifdef BT_DUT_ADC_INTERFERE
    while (0) {
#else
    while (0) {
#endif/*BT_DUT_ADC_INTERFERE*/
        os_time_dly(500);
    }
}

/*
*********************************************************************
*                  AUDIO ADC MIC CLOSE
* Description: 关闭mic采样模块
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void pt_audio_adc_mic_demo_close(void)
{
    gpio_set_mode(IO_PORT_SPILT(IO_PORTH_08), PORT_OUTPUT_LOW);
    ADC_DEMO_LOG("audio_adc_mic_demo_close\n");
    if (mic_demo) {
        audio_adc_del_output_handler(&adc_hdl, &mic_demo->adc_output);
        /*audio_adc_mic_close()里面自动释放adcbuf，不需要外面释放*/
        /* free(mic_demo->adc_buf); */
        audio_adc_mic_close(&mic_demo->mic_ch);
        if (mic_demo->adc_2_dac) {
            audio_dac_channel_close(NULL);
        }
        task_kill(ADC_DEMO_TASK_NAME);
        free(mic_demo);
        mic_demo = NULL;
        free(pt_temp16);
        free(pt_temp32);
    }
}



