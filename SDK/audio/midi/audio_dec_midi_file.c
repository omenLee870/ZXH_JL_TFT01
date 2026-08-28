/*
 ****************************************************************
 *File : audio_dec_midi_file.c
 *Note :
 *
 ****************************************************************
 */
//////////////////////////////////////////////////////////////////////////////

#include "sdk_config.h"
#include "media/includes.h"
#include "jlstream.h"
#include "audio_config_def.h"
#include "app_main.h"
#include "app_config.h"
#include "codec/MIDI_DEC_API.h"
#include "fs/resfile.h"
#include "media/audio_def.h"
#include "codec/audio_midi.h"
#include "audio_general_config.h"
#include "audio_config_def.h"

#if defined(MIDI_FILE_DEC_ENABLE) && MIDI_FILE_DEC_ENABLE
//midi文件播放时，对应的音色文件路径(用户可修改路径)
#define MIDI_TIMBRE_FILE_PATH  "mnt/sdfile/res/align_dir/timbre.mdb\0"

//替换主旋律的音色库，由录音得到(用户可修改路径)
#define MIDI_W2S_FILE_PATH "mnt/sdfile/res/W2S/W2S.raw\0"
//替换主旋律音色库对应的参数文件,录音时生成(用户可修改路径)
#define MIDI_W2S_PARM_PATH "mnt/sdfile/res/W2S/parmout.raw\0"

#define MIDI_DEC_MAX_KEY_NUM   18   //(MIDI文件解码支持多少个key同时发声,用户可修改,越多，消耗的时钟越高)

u32 tmark_trigger(void *priv, u8 *val, u8 len)
{
    return 0;
}

u32 melody_trigger(void *priv, u8 key, u8 vel)
{
    return 0;
}

u32 melody_stop_trigger(void *priv, u8 key, u8 chnl)
{
    return 0;
}

u32 timDiv_trigger(void *priv)
{
    return 0;
}

u32 beat_trigger(void *priv, u8 val1/*一节多少拍*/, u8 val2/*每拍多少分音符*/)
{
    return 0;
}


u32 wdt_clear_trigger(void *priv)
{
    wdt_clear();
    return 16;
}
void note_on_trigger(void *priv, u8 trk_num, u8 chnl, u8 *key, u8 *vel, u8 *note_bypass)
{
    return;
}

void note_off_trigger(void *priv, u8 trk_num, u8 chnl, u8 *key)
{
    return;
}

void adsr_trigger(void *priv, u16 *attack, int *decay, int *release, s8 stero_channel)
{
    return;
}

/*----------------------------------------------------------------------------*/
/**@brief    midi初始化函数，由midi_init调用
   @param    midi_init_info_v:midi信息
   @param    addr:音色文件句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void init_midi_info_val(MIDI_INIT_STRUCT  *midi_init_info_v, MIDI_CONFIG_PARM *cfg_parm, void *addr)
{
    int bit_width = audio_general_in_dev_bit_width();
    int channel_mode = AUDIO_CH_LR;
    cfg_parm->spi_pos     = (unsigned short *)addr;
    cfg_parm->sample_rate = get_sr_tab_num(MIDI_DEC_SR);
    cfg_parm->bitwidth    = bit_width ? 24 : 16; ; //输出pcm数据位宽  16 或者24
    cfg_parm->OutdataBit  = bit_width;//输出数据位宽  0->16bit  1->32bit
    cfg_parm->out_channel = AUDIO_CH_NUM(channel_mode);		//输出通道
    cfg_parm->player_t    = MIDI_DEC_MAX_KEY_NUM; //(支持多少个key同时发声,用户可修改)
    cfg_parm->solo_chnl   = NULL;
    cfg_parm->solo_chnl_num = 0;


    //midi的模式初始化
    midi_init_info_v->mode_info.mode = CMD_MIDI_CTRL_MODE_0; //CMD_MIDI_CTRL_MODE_2;

    //midi节奏初始化
    midi_init_info_v->tempo_info.tempo_val = 1042;

    for (u32 tmp_i = 0; tmp_i < 16; tmp_i++) {
        midi_init_info_v->tempo_info.decay_val[tmp_i] = ((u16)31 << 11) | 1024;
    }
    midi_init_info_v->tempo_info.mute_threshold = (u16)1L << 29;

    //midi主轨道初始化
    midi_init_info_v->mainTrack_info.chn = 17; //把哪个轨道当成主轨道 , 17:库内自动分配

    //midi外部音量初始化
    for (u32 tmp_i = 0; tmp_i < 16; tmp_i++) {
        midi_init_info_v->vol_info.cc_vol[tmp_i] = 4096; //4096即原来的音量
    }
    midi_init_info_v->vol_info.ex_vol_use_chn = 1;

    //midi的主轨道乐器设置
    midi_init_info_v->prog_info.prog = 0;
    midi_init_info_v->prog_info.ex_vol = 1024;
    midi_init_info_v->prog_info.replace_mode = 0;

    //OKON 模式设置
    midi_init_info_v->okon_info.Melody_Key_Mode = CMD_MIDI_MELODY_KEY_0;
    midi_init_info_v->okon_info.OKON_Mode = CMD_MIDI_OKON_MODE_0;


    //midi的mark控制初始化
    midi_init_info_v->mark_info.priv = NULL; //&file_mark;
    midi_init_info_v->mark_info.mark_trigger = tmark_trigger;

    //midi的melody控制初始化
    midi_init_info_v->moledy_info.priv = NULL; //&file_melody;
    midi_init_info_v->moledy_info.melody_trigger = melody_trigger;

    //midi的melody stop控制初始化
    midi_init_info_v->moledy_stop_info.priv = NULL;
    midi_init_info_v->moledy_stop_info.main_chn_enable = 1; //1: 是回调主旋律的停止音符 0:是回调的所有通道的停止音符
    midi_init_info_v->moledy_stop_info.melody_stop_trigger = melody_stop_trigger;//主旋律音符停止播放回调

    //midi的小节回调控制初始化
    midi_init_info_v->tmDiv_info.priv = NULL;
    midi_init_info_v->tmDiv_info.timeDiv_trigger = timDiv_trigger;

    //midi的小拍回调控制初始化
    midi_init_info_v->beat_info.priv = NULL;
    midi_init_info_v->beat_info.beat_trigger = beat_trigger;

    midi_init_info_v->wdt_clear.priv = NULL;
    midi_init_info_v->wdt_clear.count = 16;
    midi_init_info_v->wdt_clear.wdt_clear_trigger = wdt_clear_trigger;


    midi_init_info_v->note_on_trigger.Enable = 0;
    midi_init_info_v->note_on_trigger.midi_note_on_trigger = note_on_trigger;

    midi_init_info_v->note_off_trigger.Enable = 0;
    midi_init_info_v->note_off_trigger.midi_note_off_trigger = note_off_trigger;

    midi_init_info_v->adsr_trigger.Enable = 0;
    midi_init_info_v->adsr_trigger.midi_adsr_trigger = adsr_trigger;


    //使能位控制
    midi_init_info_v->switch_info = MELODY_PLAY_ENABLE;// | MELODY_ENABLE | EX_VOL_ENABLE;            //主轨道播放使能

#if defined(MIDI_SUPPORT_W2S) && MIDI_SUPPORT_W2S
    //设置替换主轨道的声音的音源
    void *cache_addr;
    if (midi_w2s_get_cfg_addr(&cache_addr)) {
        log_e("get midi w2s addr err\n");
        return;
    }
    midi_w2s_parm_get(&midi_init_info_v->w2s_info);
    midi_init_info_v->w2s_info.key_diff = -7;////与音高成反比
    midi_init_info_v->w2s_info.rec_data = (u32)cache_addr;
#endif/*MIDI_SUPPORT_W2S*/
    return;
}

int midi_w2s_get_cfg_addr(void **addr)
{
    FILE  *file = fopen(MIDI_W2S_FILE_PATH, "r");
    if (!file) {
        log_e("MIDI.bin open err\n");
        return -1;
    }

    struct vfs_attr attr = {0};
    fget_attrs(file, &attr);
    *addr = (void *)attr.sclust;
    fclose(file);

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    midi初始化函数，该函数在播放器启动前调用
   @param    参数返回地址
   @return   0
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_init(struct midi_init_param  *midi_param)
{
    void *cache_addr;
    if (midi_get_cfg_addr((u8 *)MIDI_TIMBRE_FILE_PATH, &cache_addr)) {
        log_e("get midi addr err\n");
        return ;
    }
    memset(midi_param, 0x0, sizeof(struct midi_init_param));

    //初始化midi参数
    init_midi_info_val(&midi_param->midi_init_info_val, &midi_param->cfg_parm, cache_addr); //需要外部写

#if defined(MIDI_SUPPORT_W2S) && MIDI_SUPPORT_W2S
    midi_w2s_init(&midi_param->mode_parm);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    midi控制函数
	@param   cmd:
		     CMD_MIDI_SET_CHN_PROG,  //更改乐器,参数结构对应 MIDI_PROG_CTRL_STRUCT
		     CMD_MIDI_CTRL_TEMPO,    //改变节奏,参数结构对应 MIDI_PLAY_CTRL_TEMPO
		     CMD_MIDI_GOON,          //one key one note的时候接着播放使用,参数为空
		     CMD_MIDI_CTRL_MODE,     //改变模式,参数结构对应 MIDI_PLAY_CTRL_MO
		     CMD_MIDI_SET_SWITCH,    //配置开关使能，要不要替换乐器，使用外部音量,参数对应 MIDI_SET_SWITCH
		     CMD_MIDI_SET_EX_VOL,    //设置外部声道音量,参数结构对应 EX_CH_VOL_PARM
   @param    priv:对应cmd的参数地址
   @return   0
   @note    midi解码控制例程
*/
/*----------------------------------------------------------------------------*/
void midi_ioctrl(struct jlstream *stream, u32 cmd, void *priv)
{

    struct midi_cmd  parg = {
        .cmd = cmd,
        .priv = priv,
    };
    if (stream) {
        jlstream_node_ioctl(stream, NODE_UUID_DECODER,
                            NODE_IOC_SET_PRIV_FMT, (int)&parg);
    }

}

/*
 *one key one note播放命令,调一下，播一下声音
 * */
void midi_goon(struct jlstream *stream)
{
    midi_ioctrl(stream, CMD_MIDI_GOON, NULL);
}


#if 0
//测试例子
void *ex_vol_test(struct jlstream *stream)
{
    static int val = 4096;
    EX_CH_VOL_PARM ex_vol;
    for (int test_ci = 0; test_ci < CTRL_CHANNEL_NUM; test_ci++) {
        ex_vol.cc_vol[test_ci] = val;
    }
    val -= 64;
    if (val <= 0) {
        val = 4096;
    }
    midi_ioctrl(stream, CMD_MIDI_SET_EX_VOL, &ex_vol);
    return NULL;
}
#endif


#if defined(MIDI_SUPPORT_W2S) && MIDI_SUPPORT_W2S
int midi_w2s_init(void *info)
{
    struct _AUDIO_DECODE_PARA {
        u32 mode;
    };
    struct _AUDIO_DECODE_PARA *mode_parm = (struct _AUDIO_DECODE_PARA *)info;
    mode_parm->mode = CMD_MIDI_CTRL_MODE_W2S;
    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    midi主旋律是否用录音文件替换主旋律
   @param    flag:1(替换主旋律)  0(不替换主旋律)
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_mode_change(struct jlstream *stream, u8 flag)
{
    struct _AUDIO_DECODE_PARA {
        u32 mode;
    };
    struct _AUDIO_DECODE_PARA mode_parm = {0};
    if (flag) {
        mode_parm.mode = CMD_MIDI_CTRL_MODE_W2S;
    } else {
        mode_parm.mode = CMD_MIDI_CTRL_MODE_0;
    }
    midi_ioctrl(stream, CMD_MIDI_CTRL_MODE, &mode_parm);
}

void midi_mode_test(void *p)
{
    static u8 flag = 0;
    midi_mode_change(flag);
    flag = !flag;
}
#endif/*MIDI_SUPPORT_W2S*/

/*----------------------------------------------------------------------------*/
/**@brief    midi预处理的音色，对应的输出的参数
   @param    data:参数地址
   @param    len:参数长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_w2s_parm_set(void *data, u32 len)
{
    /* MIDI_W2S_STRUCT *w2s_parmOut; */
    void *fp = fopen(MIDI_W2S_PARM_PATH, "wb");
    if (fp) {
        fwrite(data, len, 1, fp);
        fclose(fp);
        fp = NULL;
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    获取midi预处理的音色，对应的参数
   @param    parm:参数返回地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_w2s_parm_get(void *parm)
{
    void *fp = fopen(MIDI_W2S_PARM_PATH, "r");
    if (fp) {
        fread(parm, sizeof(MIDI_W2S_STRUCT), 1, fp);
        fclose(fp);
        fp = NULL;
    }
}

#if 1
//midi音乐文件播放测试
#include "file_player.h"
void midi_file_player_test()
{
    static FILE *test_file;
    static u32 cnt = 0;
    if (cnt == 0) {
        if (!midi_flash_sdfile_mnt) {
            midi_flash_sdfile_mnt = mount(NULL, "mnt/sdfile", "sdfile", 0, NULL);
            if (!midi_flash_sdfile_mnt) {
                log_e("=======mnt sdfile error=====\n");
            }
        }

        char *mfa_file_path = (char *)"mnt/sdfile/res/align_dir/testmid.mfa";
        /* char *mfa_file_path = (char *)"storage/sd0/C/testmid.mfa"; */
        test_file = fopen(mfa_file_path, "r");
        printf("midi file test %p\n", test_file);
        music_file_play(test_file, NULL);
    } else if (cnt == 5) {
        music_file_player_stop();
        if (test_file) {
            fclose(test_file);
            test_file = NULL;
        }
    }
    if (++cnt > 5) {
        cnt = 0;
    }
}
#endif



#endif /*MIDI_FILE_DEC_ENABLE*/
