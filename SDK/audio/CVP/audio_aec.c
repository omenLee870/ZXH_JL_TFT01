#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_cvp.data.bss")
#pragma data_seg(".audio_cvp.data")
#pragma const_seg(".audio_cvp.text.const")
#pragma code_seg(".audio_cvp.text")
#endif
/*
 ****************************************************************
 *							AUDIO AEC
 * File  : audio_aec.c
 * By    :
 * Notes : 仅AEC回音消除
 *
 ****************************************************************
 */
#include "audio_aec.h"
#include "system/includes.h"
#include "media/includes.h"
#include "effects/eq_config.h"
#include "effects/audio_pitch.h"
#include "circular_buf.h"
#include "overlay_code.h"
#include "audio_cvp_online.h"
#include "audio_config.h"
#include "cvp_node.h"
#include "sdk_config.h"
#include "audio_dc_offset_remove.h"
#include "adc_file.h"
#include "audio_cvp_def.h"
#include "effects/audio_gain_process.h"



#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/* TCFG_USER_TWS_ENABLE */

#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice/smart_voice.h"
#endif

#if TCFG_AUDIO_AVC_NODE_ENABLE
#include "audio_avc.h"
#endif


#define LOG_TAG_CONST       AEC_USER
#define LOG_TAG             "[AEC_USER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
//#include "audio_cvp_debug.c"


/*CVP_TOGGLE:CVP模块(包括AEC、NLP、NS等)总开关，Disable则数据完全不经过处理，释放资源*/
#define CVP_TOGGLE				TCFG_AEC_ENABLE	/*CVP模块总开关*/


/*使用输入立体声参考数据的TDE回音消除算法*/
extern const u8 CONST_SMS_TDE_STEREO_REF_ENABLE;
//const u8 CONST_SMS_TDE_STEREO_REF_ENABLE = 0;


/*使能即可跟踪通话过程的内存情况*/
#define CVP_MEM_TRACE_ENABLE	0


/*
 *AEC复杂等级，等级越高，ram和mips越大，适应性越好
 *回音路径不定/回音失真等情况才需要比较高的等级
 *音箱建议使用等级:5
 *耳机建议使用等级:2
 */
#define AEC_TAIL_LENGTH			5 /*range:2~10,default:2*/

/*单工连续清0的帧数*/
#define AEC_SIMPLEX_TAIL 	15
/**远端数据大于CONST_AEC_SIMPLEX_THR,即清零近端数据
 *越小，回音限制得越好，同时也就越容易卡*/
#define AEC_SIMPLEX_THR		100000	/*default:260000*/

/*数据输出开头丢掉的数据包数*/
#define AEC_OUT_DUMP_PACKET		15
/*数据输出开头丢掉的数据包数*/
#define AEC_IN_DUMP_PACKET		1

extern void aec_code_movable_load(void);
extern void aec_code_movable_unload(void);
extern int db2mag(int db, int dbQ, int magDQ);//10^db/20

extern int esco_player_runing();


struct echo_cancel_hdl {
    u8 start;				//aec模块状态
    u8 inbuf_clear_cnt;		//aec输入数据丢掉
    u8 output_fade_in;		//aec输出淡入使能
    u8 output_fade_in_gain;	//aec输出淡入增益
    u8 EnableBit;			//aec使能模块
    u8 input_clear;			//清0输入数据标志
    u16 dump_packet;		//前面如果有杂音，丢掉几包

#if ((TCFG_SUPPORT_MIC_CAPLESS)&&(AUDIO_MIC_CAPLESS_VERSION < MIC_CAPLESS_VER3))
    void *dcc_hdl;
#endif
    struct aec_s_attr attr;
    struct audio_cvp_pre_param_t pre;	//预处理配置
};
struct echo_cancel_hdl *aec_hdl = NULL;
static u8 global_output_way = 0;


/*
*********************************************************************
*                  Audio AEC Process_Probe
* Description: AEC模块数据前处理回调
* Arguments  : data 数据地址
*			   len	数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : 在源数据经过AEC模块前，可以增加自定义处理
*********************************************************************
*/
static int acoustic_echo_cancel_probe(short *talk_mic, short *talk_ref_mic, short *mic3, short *ref, u16 len)
{
    if (aec_hdl->pre.pre_gain_en) {
        GainProcess_16Bit(talk_mic, talk_mic, aec_hdl->pre.talk_mic_gain, 1, 1, 1, len >> 1);
    }

#if ((TCFG_SUPPORT_MIC_CAPLESS)&&(AUDIO_MIC_CAPLESS_VERSION < MIC_CAPLESS_VER3))
    if (aec_hdl->dcc_hdl) {
        audio_dc_offset_remove_run(aec_hdl->dcc_hdl, (void *)talk_mic, len);
    }
#endif
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Process_Post
* Description: AEC模块数据后处理回调
* Arguments  : data 数据地址
*			   len	数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : 在数据处理完毕，可以增加自定义后处理
*********************************************************************
*/
static int acoustic_echo_cancel_post(s16 *data, u16 len)
{
    return 0;
}

static int audio_aec_update(u8 EnableBit)
{
    printf("aec_update,wideband:%d,EnableBit:%x", aec_hdl->attr.wideband, EnableBit);
    return 0;
}

/*跟踪系统内存使用情况:physics memory size xxxx bytes*/
static void sys_memory_trace(void)
{
    static int cnt = 0;
    if (cnt++ > 200) {
        cnt = 0;
        mem_stats();
    }
}

/*通话上行同步输出回调*/
/* int audio_aec_sync_buffer_set(s16 *data, int len)
{
    return cvp_node_output_handle(data, len);
} */
/*
*********************************************************************
*                  Audio AEC Output Handle
* Description: AEC模块数据输出回调
* Arguments  : data 输出数据地址
*			   len	输出数据长度
* Return	 : 数据输出消耗长度
* Note(s)    : None.
*********************************************************************
*/
static int acoustic_echo_cancel_output(s16 *data, u16 len)
{
#if (((defined TCFG_KWS_VOICE_RECOGNITION_ENABLE) && TCFG_KWS_VOICE_RECOGNITION_ENABLE) || \
     ((defined TCFG_CALL_KWS_SWITCH_ENABLE) && TCFG_CALL_KWS_SWITCH_ENABLE))
    //Voice Recognition get mic data here
    if (esco_player_runing()) {
        extern void kws_aec_data_output(void *priv, s16 * data, int len);
        kws_aec_data_output(NULL, data, len);
    }
#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

#if CVP_MEM_TRACE_ENABLE
    sys_memory_trace();
#endif/*CVP_MEM_TRACE_ENABLE*/

    if (aec_hdl->dump_packet) {
        aec_hdl->dump_packet--;
        memset(data, 0, len);
    } else  {
        if (aec_hdl->output_fade_in) {
            s32 tmp_data;
            //printf("fade:%d\n",aec_hdl->output_fade_in_gain);
            for (int i = 0; i < len / 2; i++) {
                tmp_data = data[i];
                data[i] = tmp_data * aec_hdl->output_fade_in_gain >> 7;
            }
            aec_hdl->output_fade_in_gain += 12;
            if (aec_hdl->output_fade_in_gain >= 128) {
                aec_hdl->output_fade_in = 0;
            }
        }
    }
    return len;
}

/*
*********************************************************************
*                  Audio AEC Parameters
* Description: AEC模块配置参数
* Arguments  : p	参数指针
* Return	 : None.
* Note(s)    : 读取配置文件成功，则使用配置文件的参数配置，否则使用默
*			   认参数配置
*********************************************************************
*/
static void audio_aec_param_init(struct aec_s_attr *p)
{
    printf("use default param");
    p->EnableBit = AEC_EN;
    p->toggle = 1;
    /*AEC*/
    p->AEC_DT_AggressiveFactor = 1.f;	/*范围：1~5，越大追踪越好，但会不稳定,如破音*/
    p->AEC_RefEngThr = -70.f;
    /*ES*/
    p->ES_AggressFactor = -3.0f;
    p->ES_MinSuppress = 4.f;
    p->ES_Unconverge_OverDrive = p->ES_MinSuppress;
    p->wn_gain = 331;
    p->aec_tail_length = AEC_TAIL_LENGTH;
    p->ES_OverSuppressThr = 0.02f;
    p->ES_OverSuppress = 2.f;
    p->TDE_EngThr = -80.f;

    /* aec_param_dump(p); */
}

/*
*********************************************************************
*                  Audio AEC Open
* Description: 初始化AEC模块
* Arguments  : init_param 	sr 			采样率(8000/16000)
*              				ref_sr  	参考采样率
*			   enablebit	使能模块(AEC/NLP/AGC/ANS...)
*			   out_hdl		自定义回调函数，NULL则用默认的回调
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口是对audio_aec_init的扩展，支持自定义使能模块以及
*			   数据输出回调函数
*********************************************************************
*/
int acoustic_echo_cancel_init(struct audio_aec_init_param_t *init_param, s16 enablebit,
                              int (*out_hdl)(s16 *data, u16 len), struct aec_param *algo_param)
{
    s16 sample_rate = init_param->sample_rate;
    u32 ref_sr = init_param->ref_sr;
    u8 ref_channel = init_param->ref_channel;
    struct aec_s_attr *aec_param;
    printf("audio_aec_open\n");
    mem_stats();
    aec_hdl = zalloc(sizeof(struct echo_cancel_hdl));
    if (aec_hdl == NULL) {
        log_error("aec_hdl malloc failed");
        return -ENOMEM;
    }

    overlay_load_code(OVERLAY_AEC);
    aec_code_movable_load();

    aec_hdl->dump_packet = AEC_OUT_DUMP_PACKET;
    aec_hdl->inbuf_clear_cnt = AEC_IN_DUMP_PACKET;
    aec_hdl->output_fade_in = 1;
    aec_hdl->output_fade_in_gain = 0;
    aec_param = &aec_hdl->attr;
    aec_param->aec_probe = acoustic_echo_cancel_probe;
    aec_param->aec_post = acoustic_echo_cancel_post;
    aec_param->output_handle = acoustic_echo_cancel_output;
    aec_param->far_noise_gate = 10;
    if (ref_sr) {
        aec_param->ref_sr  = ref_sr;
    } else {
        //aec_param->ref_sr  = usb_mic_is_running();
    }
    if (aec_param->ref_sr == 0) {
        if (TCFG_ESCO_DL_CVSD_SR_USE_16K && (sample_rate == 8000)) {
            aec_param->ref_sr = 16000;	//CVSD 下行为16K
        } else {
            aec_param->ref_sr = sample_rate;
        }
    }
    if (ref_channel != 2) {
        ref_channel = 1;
    }
    aec_param->ref_channel = ref_channel;

    audio_aec_param_init(aec_param);
    if (enablebit >= 0) {
        aec_param->EnableBit = enablebit;
    }
    if (out_hdl) {
        aec_param->output_handle = out_hdl;
    }

    aec_param->output_way = global_output_way;

    if (aec_param->output_way == 0 && aec_param->adc_ref_en == 0) {
        /*内部读取DAC数据做参考数据才需要做24bit转16bit*/
        extern struct dac_platform_data dac_data;
        aec_param->ref_bit_width = dac_data.bit_width;
    } else {
        aec_param->ref_bit_width = DATA_BIT_WIDE_16BIT;
    }

#if ((TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC) || (TCFG_AUDIO_AVC_NODE_ENABLE && AVC_USE_AEC))
    printf("esco_player_runing() : %d", esco_player_runing());
    if (!esco_player_runing()) {
        /* aec_param->ref_sr = TCFG_AUDIO_GLOBAL_SAMPLE_RATE; */

        aec_param->output_way = 1;
        aec_param->fm_tx_start = 1;
        /*在语音识别做24bit转16bit*/
        aec_param->ref_bit_width = DATA_BIT_WIDE_16BIT;

        /*AGC*/
        aec_param->agc_en = 0;

        /*AEC*/
        aec_param->AEC_DT_AggressiveFactor = algo_param->aec_dt_aggress;	/*范围：1~5，越大追踪越好，但会不稳定,如破音*/
        aec_param->AEC_RefEngThr = algo_param->aec_refengthr;

        /*NLP*/
        aec_param->ES_AggressFactor = algo_param->es_aggress_factor; /*-5~ -1 越小越强*/
        aec_param->ES_MinSuppress = algo_param->es_min_suppress;/*0`10 越大越强*/
    }
#endif // TCFG_SMART_VOICE_USE_AEC

#if TCFG_AEC_SIMPLEX
    aec_param->wn_en = 1;
    aec_param.EnableBit = AEC_MODE_SIMPLEX;
    if (sr == 8000) {
        aec_param.SimplexTail = aec_param.SimplexTail / 2;
    }
#else
    aec_param->wn_en = 0;
#endif/*TCFG_AEC_SIMPLEX*/

    /*根据清晰语音处理模块配置，配置相应的系统时钟*/
    if (sample_rate == 16000) {
        /*宽带wide-band*/
        aec_param->wideband = 1;
        aec_param->hw_delay_offset = 60;
    } else {
        /*窄带narrow-band*/
        aec_param->wideband = 0;
        aec_param->hw_delay_offset = 55;
    }

    /* clk_set("sys", 0); */

#if ((TCFG_SUPPORT_MIC_CAPLESS)&&(AUDIO_MIC_CAPLESS_VERSION < MIC_CAPLESS_VER3))
    if (audio_adc_file_get_mic_mode(0) == AUDIO_MIC_CAPLESS_MODE) {
        aec_hdl->dcc_hdl = audio_dc_offset_remove_open(sample_rate, 1);
    }
#endif

    //aec_param_dump(aec_param);
    aec_hdl->EnableBit = aec_param->EnableBit;
#if (((defined TCFG_KWS_VOICE_RECOGNITION_ENABLE) && TCFG_KWS_VOICE_RECOGNITION_ENABLE) || \
     ((defined TCFG_CALL_KWS_SWITCH_ENABLE) && TCFG_CALL_KWS_SWITCH_ENABLE))
    extern u8 kws_get_state(void);
    if (esco_player_runing()) {
        if (kws_get_state()) {
            aec_param->EnableBit = AEC_EN;
            aec_param->agc_en = 0;
            printf("kws open,aec_enablebit=%x", aec_param->EnableBit);
            //临时关闭aec, 对比测试
            //aec_param->toggle = 0;
        }
    }
#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

#if CVP_TOGGLE
    if (aec_param->output_way && (aec_param->adc_ref_en || aec_param->ref_bit_width)) {
        ASSERT(0, "output_way and adc_ref_en can not open at same time, or output_way nonsupport 24bit Width");
    }
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
    if (CONST_SMS_TDE_STEREO_REF_ENABLE && aec_param->ref_channel != 2) {
        ASSERT(0, "ref_channel need set 2 ch when CONST_SMS_TDE_STEREO_REF_ENABLE is 1");
    }
#endif
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
    int ret = sms_tde_init(aec_param);
#else
    int ret = aec_init(aec_param);
#endif
    ASSERT(ret == 0, "aec_open err %d!!", ret);
#endif/*CVP_TOGGLE*/
    aec_hdl->start = 1;
    mem_stats();
    printf("audio_aec_open succ\n");
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Reboot
* Description: AEC模块复位接口
* Arguments  : reduce 复位/恢复标志
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void acoustic_echo_cancel_reboot(u8 reduce)
{
    if (aec_hdl) {
        printf("audio_aec_reboot:%x,%x,start:%d", aec_hdl->EnableBit, aec_hdl->attr.EnableBit, aec_hdl->start);
        if (aec_hdl->start) {
            if (reduce) {
                aec_hdl->attr.EnableBit = AEC_EN;
                aec_hdl->attr.agc_en = 0;
            } else {
                if (aec_hdl->EnableBit != aec_hdl->attr.EnableBit) {
                    aec_hdl->attr.EnableBit = aec_hdl->EnableBit;
                }
                aec_hdl->attr.agc_en = 1;
            }
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
            sms_tde_reboot(aec_hdl->attr.EnableBit);
#else
            aec_reboot(aec_hdl->attr.EnableBit);
#endif
        }
    } else {
        printf("audio_aec close now\n");
    }
}

/*
*********************************************************************
*                  Audio AEC Close
* Description: 关闭AEC模块
* Arguments  : None.
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void acoustic_echo_cancel_close(void)
{
    printf("audio_aec_close:%x", (u32)aec_hdl);
    if (aec_hdl) {
        aec_hdl->start = 0;

#if CVP_TOGGLE
        if (TCFG_AUDIO_SMS_SEL == SMS_TDE) {
            sms_tde_exit();
        } else {
            aec_exit();
        }
#endif

#if ((TCFG_SUPPORT_MIC_CAPLESS)&&(AUDIO_MIC_CAPLESS_VERSION < MIC_CAPLESS_VER3))
        if (aec_hdl->dcc_hdl) {
            audio_dc_offset_remove_close(aec_hdl->dcc_hdl);
            aec_hdl->dcc_hdl = NULL;
        }
#endif

        local_irq_disable();
        free(aec_hdl);
        aec_hdl = NULL;
        local_irq_enable();

        aec_code_movable_unload();
    }
}

/*
*********************************************************************
*                  Audio AEC Status
* Description: AEC模块当前状态
* Arguments  : None.
* Return	 : 0 关闭 其他 打开
* Note(s)    : None.
*********************************************************************
*/
u8 acoustic_echo_cancel_status_query(void)
{
    if (aec_hdl) {
        return aec_hdl->start;
    }
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Input
* Description: AEC源数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 输入一帧数据，唤醒一次运行任务处理数据，默认帧长256点
*********************************************************************
*/
void acoustic_echo_cancel_inbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        if (aec_hdl->input_clear) {
            memset(buf, 0, len);
        }
#if CVP_TOGGLE
        if (aec_hdl->inbuf_clear_cnt) {
            aec_hdl->inbuf_clear_cnt--;
            memset(buf, 0, len);
        }
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
        int ret = sms_tde_talk_data_push(buf, len);
#else
        int ret = sms_talk_data_push(buf, len);
#endif
        if (ret == -1) {
        } else if (ret == -2) {
            log_error("aec inbuf full\n");
#if TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC
            audio_smart_voice_aec_cbuf_data_clear();
#endif
        }
#else	/*不经算法，直通到输出*/
        aec_hdl->attr.output_handle(buf, len);
#endif/*CVP_TOGGLE*/
    }
}


/*
*********************************************************************
*                  Audio AEC Reference
* Description: AEC模块参考数据输入
* Arguments  : buf	输入参考数据地址
*			   len	输入参考数据长度
* Return	 : None.
* Note(s)    : 声卡设备是DAC，默认不用外部提供参考数据
*********************************************************************
*/
void acoustic_echo_cancel_refbuf(s16 *data0, s16 *data1, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
#if CVP_TOGGLE
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
        sms_tde_spk_data_push(data0, data1, len);
#else
        sms_spk_data_push(data0, data1, len);
#endif
#endif/*CVP_TOGGLE*/
    }
}

/*
*********************************************************************
*                  			Audio CVP IOCTL
* Description: CVP功能配置
* Arguments  : cmd 		操作命令
*		       value 	操作数
*		       priv 	操作内存地址
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)比如动态开关降噪NS模块:
*				audio_cvp_ioctl(CVP_NS_SWITCH,1,NULL);	//降噪关
*				audio_cvp_ioctl(CVP_NS_SWITCH,0,NULL);  //降噪开
*********************************************************************
*/
int acoustic_echo_cancel_ioctl(int cmd, int value, void *priv)
{
    if (!aec_hdl) {
        return -1;
    }
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
    return sms_tde_ioctl(cmd, value, priv);
#else
    return aec_ioctl(cmd, value, priv);
#endif
}

/*
*********************************************************************
*                  Audio CVP Toggle Set
* Description: CVP模块算法开关使能
* Arguments  : toggle 0 关闭算法 1 打开算法
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int acoustic_echo_cancel_toggle_set(u8 toggle)
{
    if (aec_hdl) {
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
        sms_tde_toggle(toggle);
#else
        aec_toggle(toggle);
#endif
    }
    return 0;
}



