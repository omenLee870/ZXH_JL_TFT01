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
#include "codec/MIDI_CTRL_API.h"
#include "fs/resfile.h"
#include "media/audio_def.h"
#include "file_player.h"
#include "codec/audio_midi.h"
#include "audio_general_config.h"
#include "audio_config_def.h"


#if defined(MIDI_CTRL_DEC_ENABLE) && MIDI_CTRL_DEC_ENABLE

//midi琴音色路径(用户可修改路径)
#define MIDI_CTRL_TIMBRE_FILE_PATH "mnt/sdfile/res/align_dir/timbre.mdb"

/*
 *如遇到多按琴键同时按下或重复快速按某琴键出现卡顿的情况
 *处理方法：1、减少同时发声MIDI_KEY_NUM 个数
 *          2、提高外挂flash的 波特率（60M、90M）,调整spi读线宽的为2线模式或4线
 *          3、提高系统时钟
 * */
#if MIDI_CTRL_OPORT_NUM == 1
#define MIDI_KEY_NUM  (10)//(支持多少个key同时发声， 越大，需要的时钟越大)
#else
#define MIDI_KEY_NUM  (22)//(支持多少个key同时发声， 越大，需要的时钟越大)
#endif

void midi_event_protect() {}
void midi_event_disprotect() {}

static u8 solo_chnl[] = {0, 1, 2};
static u8 prog_path[64];
static u32 prog_sclust;
int midi_ctrl_param_init(midi_ctrl_open_parm *parm)
{
    void *cache_addr;
    if (midi_get_cfg_addr((u8 *)MIDI_CTRL_TIMBRE_FILE_PATH, &cache_addr)) {
        return -1;
    }
    memcpy(prog_path, MIDI_CTRL_TIMBRE_FILE_PATH, strlen(MIDI_CTRL_TIMBRE_FILE_PATH));
    prog_sclust = (u32)cache_addr;
    u8 bit_width = audio_general_in_dev_bit_width();
    u8 channel_mode   = AUDIO_CH_LR;
    parm->ctrl_parm.track_num = 1;//支持音轨的最大个数0~15
    parm->ctrl_parm.bitwidth  = bit_width ? 24 : 16;
    parm->ctrl_parm.tempo     = 1024;//解码会改变播放速度,1024是正常值

    parm->cfg_parm.spi_pos     = (unsigned short *)cache_addr;
    parm->cfg_parm.sample_rate = get_sr_tab_num(MIDI_DEC_SR);
    parm->cfg_parm.bitwidth    = bit_width ? 24 : 16; ; //输出pcm数据位宽  16 或者24
    parm->cfg_parm.OutdataBit  = bit_width;//输出数据位宽  0->16bit  1->32bit
    parm->cfg_parm.out_channel = AUDIO_CH_NUM(channel_mode);		//输出通道
    parm->cfg_parm.player_t    = MIDI_KEY_NUM; //(支持多少个key同时发声,用户可修改)
    parm->cfg_parm.solo_chnl   = (unsigned char *)solo_chnl;
    parm->cfg_parm.solo_chnl_num = MIDI_CTRL_OPORT_NUM - 1;

    parm->protect_trigger.midi_event_protect  = midi_event_protect;
    parm->protect_trigger.midi_event_disprotect  = midi_event_disprotect;


    /* if (parm->cfg_parm.solo_chnl_num != ARRAY_SIZE(solo_chnl)) { */
    /* log_e("solo_chnl error %d %d\n", parm->cfg_parm.solo_chnl_num, (int)ARRAY_SIZE(solo_chnl)); */
    /* } */
    /* printf("midi_ctrl_init :%d,%x\n", __LINE__, (int)parm->cfg_parm.spi_pos); //算法要求spi_pos要求两字节对齐 */

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   乐器更新
   @param   prog:乐器号
   @param   trk_num :音轨 (0~15)
   @param   timbre_file_path:文件路径"mnt/sdfile/res/align_dir/xxx.mdb",配NULL就使用init时配置的音色
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_set_porg(struct jlstream *stream, u8 prog, u8 trk_num, u8 *timbre_file_path)
{
    void *cache_addr;
    if (timbre_file_path) {
        if (!strcmp((const char *)timbre_file_path, (const char *)prog_path)) {
            cache_addr = (void *)prog_sclust;
        } else {
            midi_get_cfg_addr(timbre_file_path, &cache_addr);
            if (!cache_addr) {
                log_e("timbre_file open error %s\n", timbre_file_path);
                return ;
            }
            memcpy(prog_path, timbre_file_path, (int)strlen((const char *)timbre_file_path));
        }
    } else {
        cache_addr = (void *)prog_sclust;
    }

    struct set_prog_parm parm = {0};
    parm.sclust = (u32)cache_addr;
    parm.prog = prog;
    parm.trk_num = trk_num;
    midi_ctrl_ioctrl(stream, MIDI_CTRL_SET_PROG, &parm);
}

/*----------------------------------------------------------------------------*/
/**@brief   按键按下
   @param   nkey:按键序号（0~127）
   @param   nvel:按键力度（0~127）
   @param   chn :通道(0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_note_on(struct jlstream *stream, u8 nkey, u8 nvel, u8 chn)
{
    struct note_on_parm parm = {0};
    parm.nkey = nkey;
    parm.nvel = nvel;
    parm.chn = chn;
    midi_ctrl_ioctrl(stream, MIDI_CTRL_NOTE_ON, &parm);
}
/*----------------------------------------------------------------------------*/
/**@brief   按键松开
   @param   nkey:按键序号（0~127）
   @param   chn :通道(0~15)
   @param   time ：time为衰减时间ms，若为0则使用音色中的衰减
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_note_off(struct jlstream *stream, u8 nkey, u8 chn, u16 time)
{
    struct note_off_parm parm = {0};
    parm.nkey = nkey;
    parm.chn = chn;
    parm.time = time;
    midi_ctrl_ioctrl(stream, MIDI_CTRL_NOTE_OFF, &parm);
}
/*----------------------------------------------------------------------------*/
/**@brief  midi 配置接口
   @param   cmd:命令
   @param   priv:对应cmd的结构体
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_confing(struct jlstream *stream, u32 cmd, void *priv)
{
    midi_ctrl_ioctrl(stream, cmd, priv);
}

/*----------------------------------------------------------------------------*/
/**@brief   midi keyboard 设置按键按下音符发声的衰减系数
   @param   val:((衰减系数&0x7fff) | (延时系数&0x1f <<11))
   @return
   @note    低11bit为调节衰减系数，值越小，衰减越快， 1024为默认值， 范围：0~1024
            大于11bit为延时系数，节尾音长度的，值越大拉德越长，0为默认值,31延长1s,范围：0~31(延时系数无效)
   @example midi_ctrl_confing_set_melody_decay((衰减系数&0x7fff) | (延时系数&0x1f <<11));
			u16 val = ((1024&0x7ff) | (31&0x1f << 11));
			midi_ctrl_confing_set_melody_decay(val);
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_confing_set_melody_decay(struct jlstream *stream, u16 val)
{
    u32 cmd = CMD_MIDI_CTRL_TEMPO;
    MIDI_PLAY_CTRL_TEMPO tempo = {0};
    for (int i = 0; i < 16; i++) {
        tempo.decay_val[i] = val;
    }
    tempo.tempo_val = 1024;//设置为固定1024即可
    midi_ctrl_confing(stream, cmd, (void *)&tempo);
}
/*----------------------------------------------------------------------------*/
/**@brief   midi keyboard 设置每个通道按下音符发声的衰减系数
   @param   val:16个值的数组，每个值组成组成结构((衰减系数&0x7fff) | (延时系数&0x1f <<11))
   @return
   @note    低11bit为调节衰减系数，值越小，衰减越快， 1024为默认值， 范围：0~1024
            大于11bit为延时系数，节尾音长度的，值越大拉德越长，0为默认值,31延长1s,范围：0~31(延时系数无效)
   @example u16 val[16];
   			for (int i = 0; i< 16; i++){
			    val[i] = ((1024&0x7ff) | (31&0x1f << 11));
			}
   			midi_ctrl_confing_set_melody_decay(val);
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_confing_set_melody_decay_each_chn(struct jlstream *stream, u16 *val)
{
    u32 cmd = CMD_MIDI_CTRL_TEMPO;
    MIDI_PLAY_CTRL_TEMPO tempo = {0};
    if (val) {
        memcpy(tempo.decay_val, val, sizeof(tempo.decay_val));
    }
    tempo.tempo_val = 1024;//设置为固定1024即可
    midi_ctrl_confing(stream, cmd, (void *)&tempo);
}

/*----------------------------------------------------------------------------*/
/**@brief  弯音轮配置
   @param   pitch_val:弯音轮值,1 - 65535 ；256是正常值,对音高有作用
   @param   chn :通道(0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_pitch_bend(struct jlstream *stream, u16 pitch_val, u8 chn)
{
    struct pitch_bend_parm parm = {0};
    parm.pitch_val = pitch_val;
    parm.chn = chn;
    midi_ctrl_ioctrl(stream, MIDI_CTRL_PITCH_BEND, &parm);
}

/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl控制函数
	@param   cmd:
			 MIDI_CTRL_NOTE_ON,     //按键按下，参数结构对应struct note_on_parm
			 MIDI_CTRL_NOTE_OFF,    //按键松开，参数结构对应struct note_off_parm
			 MIDI_CTRL_SET_PROG,    //更改乐器，参数结构对应struct set_prog_parm
			 MIDI_CTRL_PITCH_BEND,  //弯音轮，参数结构对应struct pitch_bend_parm
		     CMD_MIDI_CTRL_TEMPO,    //改变节奏,参数结构对应 MIDI_PLAY_CTRL_TEMPO
			 MIDI_CTRL_VEL_VIBRATE, //抖动幅度,参数结构对应 struct vel_vibrate_parm
			 MIDI_CTRL_QUERY_PLAY_KEY,//查询指定通道的key播放,参数结构对应struct query_play_key_parm
   @param    priv:对应cmd的参数地址
   @return   0
   @note    midi解码控制例程
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_ioctrl(void *stream,  u32 cmd, void *priv)
{
    cmd = cmd + MIDI_CTRL_CMD_OFFSET;
    if (stream) {
#if MIDI_CTRL_OPORT_NUM == 1
        jlstream_node_ioctl(stream, NODE_UUID_DECODER, cmd, (int)priv);
#else
        jlstream_node_ioctl(stream, NODE_UUID_SOURCE, cmd, (int)priv);
#endif

    }

}
static struct midi_player *g_midi_player;
extern struct midi_player *midi_ctrl_player_open();
extern void midi_ctrl_player_close(struct midi_player *player);
#if 1
#define MIDI_SWITCH_KEY 0
#define MIDI_SET_KEY 1
#define MIDI_PLAY_KEY 2

/*----------------------------------------------------------------------------*/
/**@brief   midi key 测试样例
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_paly_test(u32 key)
{
    static u8 open_close = 0;
    static u8 change_prog = 0;
    static u8 note_on_off = 0;


    switch (key) {
    case MIDI_SWITCH_KEY:
        if (!open_close) {
            puts("============open====\n");
            g_midi_player = midi_ctrl_player_open();
        } else {
            if (g_midi_player) {
                puts("============close====\n");
                midi_ctrl_player_close(g_midi_player);
                g_midi_player = NULL;
            }
        }
        open_close = !open_close;
        break;
    case MIDI_SET_KEY:

        if (!g_midi_player) {
            break;
        }

        if (!change_prog) {
            puts("============set 0====\n");
            midi_ctrl_set_porg(g_midi_player->stream, 0, 0, (u8 *)MIDI_CTRL_TIMBRE_FILE_PATH); //设置0号乐器，音轨0
        } else {
            puts("============set 2====\n");
            midi_ctrl_set_porg(g_midi_player->stream, 2, 0, (u8 *)MIDI_CTRL_TIMBRE_FILE_PATH); //设置1号乐器，音轨0
        }
        puts("---set prog end\n");

        change_prog = !change_prog;
        break;
    case MIDI_PLAY_KEY:
        if (!g_midi_player) {
            break;
        }
        if (!note_on_off) {
            puts("============on====\n");
            //模拟按键57、58、59、60、61、62,以力度127，通道0，按下测试
            //JL_PORTA->DIR &= ~BIT(7);
            //JL_PORTA->OUT |= BIT(7);
            for (int i = 0; i < MIDI_KEY_NUM; i++) {
                midi_ctrl_note_on(g_midi_player->stream, 57 + i, 127, 0);
            }
        } else {
            puts("============off====\n");
            //模拟按键57、58、59、60、61、62松开测试
            //JL_PORTA->OUT &= ~BIT(7);
            for (int i = 0; i < MIDI_KEY_NUM; i++) {
                midi_ctrl_note_off(g_midi_player->stream, 57 + i,  0, 0);
            }
        }
        note_on_off = !note_on_off;
        break;
    default:
        break;
    }
}


void midi_test_demo(void *p)
{

#if 0
    void midi_file_player_test();
    midi_file_player_test();
#else
    static u32  cnt = 0;
    if (cnt == 1) {
        midi_paly_test(MIDI_SWITCH_KEY);
    } else if (cnt == 2) {
        midi_paly_test(MIDI_SET_KEY);
    } else if (cnt == 3) {
        midi_paly_test(MIDI_PLAY_KEY);
    } else if (cnt == 6) {
        /* midi_paly_test(MIDI_SWITCH_KEY); */
    }
    cnt++;
    if (cnt >= 7) {
        cnt = 2;
    }
#endif
}

#endif
#endif
