#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".record.data.bss")
#pragma data_seg(".record.data")
#pragma const_seg(".record.text.const")
#pragma code_seg(".record.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "app_main.h"
#include "default_event_handler.h"
#include "bt.h"
#include "tone_player.h"
#include "app_tone.h"
#include "dev_manager.h"
#include "record.h"
#include "file_recorder.h"
#include "audio_config.h"


#if TCFG_APP_RECORD_EN

#define PIPELINE_RECODER    0x49EC

static OS_MUTEX record_mutex;

struct app_recode_handle {
    void *recorder;
    FILE *file;
    char logo[20];
};

struct app_recode_file_play_hdl {
    FILE *file;
    u8 need_update_fsn_flag;		//是否需要重新扫描盘符
    char path[64];
    char folder_path[64];
    char suffix[5];
    int  pause_rec_flag;			//暂停录音标志
    u32 file_index;		//用来指定播放的是哪一首
    struct file_player *player;
    struct vfscan *fsn;
    FILE *latest_play_file;			//记录下最近的文件, 用于回播下删除最近的文件
};
static struct app_recode_file_play_hdl g_rec_play_hdl = {0};

static u8 os_mutex_create_flag = 0;
static void get_latest_rec_file_name(void);
static void app_recorder_stop();
static int recorde_file_play_callback(void *_priv, int parm, enum stream_event event);
static void app_recorder_pause(void);
static void app_recorder_file_play_next(void);
static void app_recorder_file_play_prev(void);
static void app_recorder_file_play_vol_down(void);
static void app_recorder_file_play_vol_up(void);
extern void audio_app_volume_up(u8 value);
extern void audio_app_volume_down(u8 value);

void app_local_record_mutex_create(void);

static struct app_recode_handle g_rec_hdl;
static u8 record_idle_flag = 1;

//*----------------------------------------------------------------------------*/
/**@brief    录音模式提示音结束处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int record_tone_play_end_callback(void *priv, enum stream_event event)
{
    return 0;

}
//*----------------------------------------------------------------------------*/
/**@brief    record 模式初始化
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int app_record_init()
{
    printf("\n--------record  start-----------\n");
    app_local_record_mutex_create();
    record_idle_flag = 0;
    tone_player_stop();
    play_tone_file_callback(get_tone_files()->record_mode, NULL,
                            record_tone_play_end_callback);
    g_rec_play_hdl.file_index = 1;		//默认初始化为1
    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_RECORD);
    return 0;
}
//*----------------------------------------------------------------------------*/
/**@brief    record 退出
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

void app_record_exit()
{
    //停止播放模式提示音
    //如果有暂停录音，需要先恢复录音
    if (g_rec_play_hdl.pause_rec_flag == STREAM_STA_PAUSE) {
        //恢复录音
        app_recorder_pause();
    }
    ///停止mic录音
    app_recorder_stop();
#if TCFG_RECORD_AUDIO_REPLAY_EN
    ///停止回放
    app_recorder_file_play_stop();
    //释放盘符
    if (g_rec_play_hdl.fsn) {
        fscan_release(g_rec_play_hdl.fsn);
        g_rec_play_hdl.fsn = NULL;
        g_rec_play_hdl.file_index = 1;			//下标重新设置为1
        g_rec_play_hdl.player = NULL;		//播放结束，加上这句防止ui继续访问异常
    }
    g_rec_play_hdl.latest_play_file = NULL;
#endif
    record_idle_flag = 1;
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_RECORD);
}

void record_ui_del_mutex()
{
    os_mutex_del(&record_mutex, 0);
    os_mutex_create_flag = 0;
}


static void app_recorder_callback(void *priv, enum stream_state state)
{

}

void record_cut_head_timeout(void *priv)
{
    struct file_recorder *recoder = (struct file_recorder *) priv;
    if (recoder) {
        jlstream_node_ioctl(recoder->stream, NODE_UUID_SOURCE, NODE_IOC_FORCE_DUMP_PACKET, 0);
    }
}

static void app_recorder_start()
{
    int err;
    struct file_recorder *recorder;

    recorder = file_recorder_open(PIPELINE_RECODER, NODE_UUID_ADC);
    /* recorder = file_recorder_open(PIPELINE_RECODER, NODE_UUID_ZERO_ACTIVE); */
    if (!recorder) {
        puts("file_recorder_open_faild\n");
        return;
    }

    struct stream_enc_fmt fmt;
    err = file_recorder_get_fmt(recorder, &fmt);
    if (err) {
        file_recorder_close(recorder, 0);
        return;
    }
    printf("recorder_fmt: ch %d, sample rate %d\n", fmt.channel, fmt.sample_rate);

#if CUT_HEAD_TIME
    if (CUT_HEAD_TIME) {
        jlstream_node_ioctl(recorder->stream, NODE_UUID_SOURCE, NODE_IOC_FORCE_DUMP_PACKET, 1);
        recorder->cut_head_timer = sys_timeout_add(recorder, record_cut_head_timeout, CUT_HEAD_TIME);
    }
#endif
#if CUT_TAIL_TIME
    if (fmt.coding_type == AUDIO_CODING_PCM) {
        recorder->cut_tail_size = (2 * fmt.sample_rate * fmt.channel) * CUT_TAIL_TIME / 1000 ;
        recorder->cut_tail_size = ((recorder->cut_tail_size + fmt.bit_rate - 1) / (fmt.bit_rate) * fmt.bit_rate);
    } else if (fmt.coding_type != AUDIO_CODING_WAV) { //adpcm需要从库里面更新头,且编码数据是成块的，不支持做去尾功能
        recorder->cut_tail_size = fmt.bit_rate / 1000  * CUT_TAIL_TIME / 8;
    }
    jlstream_node_ioctl(recorder->stream, NODE_UUID_FILE_PACKAGE, NODE_IOC_SET_PRIV_FMT, recorder->cut_tail_size);
#endif
    const char *suffix;
    switch (fmt.coding_type) {
    case AUDIO_CODING_PCM:
        suffix = "pcm";
        break;
    case AUDIO_CODING_WAV:
        suffix = "wav";
        recorder->head_size = WAV_HEAD_SIZE; //pcm 编码的头文件大小
        break;
    case AUDIO_CODING_MP3:
        suffix = "mp3";
        break;
    case AUDIO_CODING_AMR:
        suffix = "amr";
        break;
    default:
        suffix = "bin";
        break;
    }
    char folder[] = {TCFG_REC_FOLDER_NAME};         //录音文件夹名称
    char filename[] = {TCFG_REC_FILE_NAME};     //录音文件名，不需要加后缀

#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};		//外挂flash录音
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};		//内置flash录音
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));//普通设备录音，获取最后活动设备
    /* char logo[] = {RECODER_DEVICE_LOGO};		//sd卡录音 */
#endif

    char *root_path = dev_manager_get_root_path_by_logo(logo);
    char path[64] = {};

    snprintf(path, sizeof(path), "%s%s/%s.%s", root_path, folder, filename, suffix);
    FILE *file = file_recorder_open_file(recorder, path);
    if (!file) {
        file_recorder_close(recorder, 0);
        return;
    }
    file_recorder_set_callback(recorder, NULL, app_recorder_callback);

    err = file_recorder_start(recorder);
    if (err) {
        printf("file_recorder_err: %d\n", err);
        file_recorder_close(recorder, 1);
        return;
    }

    snprintf(g_rec_hdl.logo, sizeof(g_rec_hdl.logo), "%s", logo);
    g_rec_hdl.file      = file;
    g_rec_hdl.recorder   = recorder;

    //记录播放录音需要的信息
    memcpy(g_rec_play_hdl.path, path, sizeof(path));
    memcpy(g_rec_play_hdl.suffix, suffix, strlen(suffix));
}

static void app_recorder_stop()
{
    os_mutex_pend(&record_mutex, 0);
    file_recorder_close(g_rec_hdl.recorder, 1);
    g_rec_hdl.recorder = NULL;
    os_mutex_post(&record_mutex);
}

//获取录音编码时间
int app_recorder_get_enc_time(void)
{
    os_mutex_pend(&record_mutex, 0);
    struct file_recorder *recorder = g_rec_hdl.recorder;
    int time  = 0;
    if (recorder && recorder->stream) {
        //录音时候才能获取时间
        jlstream_ioctl(recorder->stream, NODE_IOC_GET_ENC_TIME, (int)(&time));
    }
    os_mutex_post(&record_mutex);
    return time;
}

//获取播放录音文件的时间
int app_recorder_get_play_file_time(void)
{
    int time = -1;
    os_mutex_pend(&record_mutex, 0);
    if (g_rec_play_hdl.player && g_rec_play_hdl.player->stream) {
        time = jlstream_node_ioctl(g_rec_play_hdl.player->stream, NODE_UUID_DECODER, NODE_IOC_GET_CUR_TIME, 0);
    }
    os_mutex_post(&record_mutex);
    return time;
}



//控制录音暂停
static void app_recorder_pause(void)
{
    struct file_recorder *recorder = g_rec_hdl.recorder;
    if (recorder && recorder->stream) {
        g_rec_play_hdl.pause_rec_flag = jlstream_pp_toggle(recorder->stream, 50);
        if (g_rec_play_hdl.pause_rec_flag == STREAM_STA_PAUSE) {
            y_printf(">>> REC PAUSE!\n");
        } else if (g_rec_play_hdl.pause_rec_flag == STREAM_STA_PLAY) {
            y_printf(">>> REC Continue!\n");
        }
    }
}

//扫描录音文件夹获取盘符
static void app_recorder_fsn_scan(void)
{
    if (g_rec_play_hdl.need_update_fsn_flag) {
        g_rec_play_hdl.need_update_fsn_flag = 0;
        if (g_rec_play_hdl.fsn) {
            fscan_release(g_rec_play_hdl.fsn);
            g_rec_play_hdl.file_index = 1;			//重新update 盘符，file_index 重新赋值
            g_rec_play_hdl.fsn = NULL;
        }
    }

    if (g_rec_play_hdl.fsn == NULL) {
        char folder[] = {TCFG_REC_FOLDER_NAME};
#if (TCFG_NOR_REC)
        char logo[] = {"rec_nor"};
#elif (FLASH_INSIDE_REC_ENABLE)
        char logo[] = {"rec_sdfile"};
#else
        char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));
        /* char logo[] = {RECODER_DEVICE_LOGO};		//sd卡录音 */
#endif
        char *root_path = dev_manager_get_root_path_by_logo(logo);
        sprintf(g_rec_play_hdl.folder_path, "%s%s%s", root_path, folder, "/");
        /* printf("REC folder_path : %s\n", g_rec_play_hdl.folder_path); */
        //扫盘获取文件夹
        const u8 fscan_file_param[] = "-t"
                                      "ALL"
                                      " -sn -r";
        struct vfscan *fsn = fscan(g_rec_play_hdl.folder_path, (char *)fscan_file_param, 1);
        if (fsn == NULL) {
            printf("error, %s %d, fsn is NULL!\n", __func__, __LINE__);
            g_rec_play_hdl.fsn = NULL;
            g_rec_play_hdl.file_index = 1;			//扫盘失败
            return;
        }
        g_rec_play_hdl.fsn = fsn;
        g_printf(">>>>> Scan fsn Success!\n");
    }
}



//删除最新的录音文件, 从最后那个文件开始删起
static void app_recorder_del_latest_file(void)
{
    if (g_rec_play_hdl.file) {
        fclose(g_rec_play_hdl.file);
        g_rec_play_hdl.file = NULL;
        g_rec_play_hdl.latest_play_file = NULL;
    }

    app_recorder_fsn_scan();
    FILE *file = NULL;
    if (g_rec_play_hdl.fsn && g_rec_play_hdl.fsn->file_number) {
        y_printf(">>>>>>>>>>>>>>>>>>>>>> g_rec_play_hdl.fsn->file_num:%d", g_rec_play_hdl.fsn->file_number);
        file = fselect(g_rec_play_hdl.fsn, FSEL_BY_NUMBER, g_rec_play_hdl.fsn->file_number);
    }
    if (file) {
        int err = fdelete(file);
        if (err) {
            r_printf("[%s, %d] Recorder file delete failed!\n", __func__, __LINE__);
        }
    }
    //删完后释放盘符
    if (g_rec_play_hdl.fsn) {
        fscan_release(g_rec_play_hdl.fsn);
        g_rec_play_hdl.fsn = NULL;
    }
}

/* 获取最新的录音文件名称 */
static void get_latest_rec_file_name(void)
{
    char folder[] = {TCFG_REC_FOLDER_NAME};         //录音文件夹名称
#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};		//外挂flash录音
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};		//内置flash录音
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));//普通设备录音，获取最后活动设备
    /* char logo[] = {RECODER_DEVICE_LOGO};		//sd卡录音 */
#endif
    char *root_path = dev_manager_get_root_path_by_logo(logo);
    int index = get_last_num();
    char index_str[5] = {0};
    if (index != (u32) - 1) {
        index_str[0] = index / 1000 + '0';
        index_str[1] = index % 1000 / 100 + '0';
        index_str[2] = index % 100 / 10 + '0';
        index_str[3] = index % 10 + '0';
        sprintf(g_rec_play_hdl.path, "%s%s%s%s%s%s", root_path, folder, "/"TCFG_REC_FILE_NAME_PREFIX, index_str, ".", g_rec_play_hdl.suffix);
    } else {
        r_printf("-- Error, func %s, index is err!! index:%d\n", index);
    }
    /* printf("%s, %s, %s, %s\n", root_path, folder, TCFG_REC_FILE_NAME_PREFIX, g_rec_play_hdl.suffix); */
    printf("Record file path :%s, index:%d\n", g_rec_play_hdl.path, index);
}

static void recorder_device_offline_check(const char *logo)
{
    os_mutex_pend(&record_mutex, 0);
    if (g_rec_hdl.recorder) {
        if (!strcmp(g_rec_hdl.logo, logo)) {
            ///当前录音正在使用的设备掉线， 应该停掉录音
            printf("is the recording dev = %s\n", logo);
            /* app_recorder_stop(); */
            file_recorder_close(g_rec_hdl.recorder, 0);
            g_rec_hdl.recorder = NULL;
        }
    }
    os_mutex_post(&record_mutex);
}

//record录音 设备事件响应接口，return 1代表事件已经处理, 0表示还未处理
int record_device_msg_handler(int *msg)
{
    const char *logo = NULL;
    int err = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        logo = (char *)msg[2];
    case DEVICE_EVENT_FROM_USB_HOST:
        if (!strncmp((char *)msg[2], "udisk", 5)) {
            logo = (char *)msg[2];
        }

        if (msg[1] == DEVICE_EVENT_IN) {
        } else if (msg[1] == DEVICE_EVENT_OUT) {
            r_printf("Event Record Device Out!!\n");
            recorder_device_offline_check(logo);
            g_rec_play_hdl.need_update_fsn_flag = 1;
            app_recorder_fsn_scan();
            if (g_rec_play_hdl.fsn == NULL) {
                r_printf(">>> Record Device Out!!\n");
                app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);	//推出record 模式
            }
        }
        break;
    default:
        break;
    }
    return false;
}

int record_app_msg_handler(int *msg)
{
    printf("msg == %d", msg[0]);
    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("app msg key change mode\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
#if TCFG_RECORD_AUDIO_REPLAY_EN
    case APP_MSG_MUSIC_PP:
        printf("app msg record pp\n");
        break;
    case APP_MSG_MUSIC_NEXT:
        printf("app msg record next\n");
        //如果在录音，则需要先停止录音
        if (g_rec_hdl.recorder) {
            r_printf("recording..., need stop record at first!\n");
            break;
        }
        app_recorder_file_play_next();
        break;
    case APP_MSG_MUSIC_PREV:
        printf("app msg record prev\n");
        if (g_rec_hdl.recorder) {
            r_printf("recording..., need stop record at first!\n");
            break;
        }
        app_recorder_file_play_prev();
        break;
    case APP_MSG_VOL_UP:
        printf(">>>>> APP MSG Vol Up!!\n");
        app_recorder_file_play_vol_up();
        break;
    case APP_MSG_VOL_DOWN:
        printf(">>>>> APP MSG Vol Down!!\n");
        app_recorder_file_play_vol_down();
        break;
#endif
    case APP_MSG_REC_PP:
        printf("APP_MSG_REC_PP %d", (int)g_rec_hdl.recorder);
        if (!g_rec_hdl.recorder) {
            //没有录音，先停止播放
#if TCFG_RECORD_AUDIO_REPLAY_EN
            app_recorder_file_play_stop();
            if (g_rec_play_hdl.fsn) {
                fscan_release(g_rec_play_hdl.fsn);
                g_rec_play_hdl.fsn = NULL;
                g_rec_play_hdl.file_index = 1;			//下标重新设置为1
                g_rec_play_hdl.player = NULL;		//播放结束，加上这句防止ui继续访问异常
            }
#endif
            app_recorder_start();
        } else {
            if (g_rec_play_hdl.pause_rec_flag == STREAM_STA_PAUSE) {
                //恢复录音
                app_recorder_pause();
            }
            app_recorder_stop();
            g_rec_play_hdl.need_update_fsn_flag = 1;		//录制完文件，要重新扫描盘符
#if TCFG_RECORD_AUDIO_REPLAY_EN
            app_recorder_file_play_start();		//播录音文件
#endif
        }
        break;
    case APP_MSG_REC_DEL_CUR_FILE:
        printf("APP_MSG_REC_DEL_CUR_FILE!!\n");
        if (!g_rec_hdl.recorder) {
            //不在录音，直接删
#if TCFG_RECORD_AUDIO_REPLAY_EN
            //回播时，删除最近播放的文件
            if (music_player_runing()) {
                os_mutex_pend(&record_mutex, 0);	//加pend防止已经释放数据流中app_recorder_get_play_file_time 函数触发的数据流访问异常
                music_file_player_stop();
                g_rec_play_hdl.player = NULL;
                os_mutex_post(&record_mutex);
                app_recorder_del_cur_play_file();
                app_recorder_play_record_folder();
            } else {
                app_recorder_file_play_stop();	//如果在播歌的话先停止播歌
                app_recorder_del_latest_file();
            }
#endif
        } else {
            //在录音，先关闭录音，再删
            if (g_rec_play_hdl.pause_rec_flag == STREAM_STA_PAUSE) {
                //恢复录音
                app_recorder_pause();
            }
            app_recorder_stop();
            //录音时，直接删除录音文件
            app_recorder_del_latest_file();	//删除录音文件
        }
        break;
    case APP_MSG_REC_PAUSE:
        y_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> case APP_MSG_REC_PAUSE!!\n");
        if (g_rec_hdl.recorder) {
            //只有在录音时才能控制录音的暂停和恢复
            app_recorder_pause();
        } else {
#if TCFG_RECORD_AUDIO_REPLAY_EN
            //如果不是在录音中，如果使能了回播功能，则此按键功能为暂停播放的功能
            //恢复播放则会从下一个录音文件开始播起
            if (music_player_runing()) {
                app_recorder_file_play_stop();
            } else {
                app_recorder_play_record_folder();
            }
#endif
        }
        break;
    default:
        printf("default");
        app_common_key_msg_handler(msg);
        break;
    }

    return 0;
}


//创建互斥量，互斥量给UI使用的，保护UI线程访问临界段代码
void app_local_record_mutex_create(void)
{
    if (os_mutex_create_flag == 0) {
        os_mutex_create(&record_mutex);
        os_mutex_create_flag = 1;
    }
}


//是否正在录音
u8 is_recorder_runing(void)
{
    app_local_record_mutex_create();
    os_mutex_pend(&record_mutex, 0);
    struct file_recorder *recorder = g_rec_hdl.recorder;
    if (recorder && recorder->stream) {
        os_mutex_post(&record_mutex);
        return 1;
    }
    os_mutex_post(&record_mutex);
    return 0;
}

//用来设置录音文件的后缀名称，例如为 .mp3 或者 .wav等等
void local_set_record_file_suffix(const char *_suffix)
{
    if (_suffix != NULL) {
        memcpy(g_rec_play_hdl.suffix, _suffix, strlen(_suffix));
        y_printf(">>Func:%s, g_rec_play_hdl.suffix:%s\n", __func__, g_rec_play_hdl.suffix);
    }
}

#if TCFG_RECORD_AUDIO_REPLAY_EN

static void app_recorder_file_play_vol_up()
{
    if (music_player_runing()) {
        u8 state = app_audio_get_state();
        s16 vol = app_audio_get_volume(state);
        printf(">>>>>>>>> state:%d, vol:%d\n", state, vol);
        audio_app_volume_up(1);
    }
}

static void app_recorder_file_play_vol_down()
{
    if (music_player_runing()) {
        u8 state = app_audio_get_state();
        s16 vol = app_audio_get_volume(state);
        printf(">>>>>>>>> state:%d, vol:%d\n", state, vol);
        audio_app_volume_down(1);
    }
}

void app_recorder_file_play_start(void)
{
    struct file_player *player = NULL;
    if (g_rec_play_hdl.file) {
        fclose(g_rec_play_hdl.file);
        g_rec_play_hdl.file = NULL;
        g_rec_play_hdl.latest_play_file = NULL;
    }
    //获取最新的录音文件路径
    get_latest_rec_file_name();
    g_rec_play_hdl.file = fopen(g_rec_play_hdl.path, "r");		//不能用wr+或者r+参数，这样会导致fclose时截断文件数据

    if (g_rec_play_hdl.file) {
        /* g_printf("g_rec_play_hdl.file is exist!\n"); */
        g_rec_play_hdl.latest_play_file = g_rec_play_hdl.file;
        g_rec_play_hdl.file_index = 2;	//回播模式从下一个文件开始播
        player = music_file_play_callback(g_rec_play_hdl.file, NULL, recorde_file_play_callback, NULL);
    }

    g_rec_play_hdl.player = player;
}

void app_recorder_file_play_stop(void)
{
    printf("music_player_runing: %d\n", music_player_runing());
    app_local_record_mutex_create();
    os_mutex_pend(&record_mutex, 0);
    if (music_player_runing()) {
        music_file_player_stop();
        g_rec_play_hdl.player = NULL;
    }
    if (g_rec_play_hdl.file) {
        fclose(g_rec_play_hdl.file);
        g_rec_play_hdl.file = NULL;
    }

    os_mutex_post(&record_mutex);
}



//开始播放文件
static struct file_player *recorder_file_play_start(void)
{
    struct file_player *player = NULL;
    if (g_rec_play_hdl.file) {
        fclose(g_rec_play_hdl.file);
        g_rec_play_hdl.file = NULL;
    }
    //获取最新的录音文件路径
    get_latest_rec_file_name();
    g_rec_play_hdl.file = fopen(g_rec_play_hdl.path, "r");		//不能用wr+或者r+参数，这样会导致fclose时截断文件数据

    if (g_rec_play_hdl.file) {
        g_printf("g_rec_play_hdl.file is exist!\n");
        player = music_file_play_callback(g_rec_play_hdl.file, NULL, recorde_file_play_callback, NULL);
    }
    return player;
}

//是否正在播放录音文件
bool is_recorder_file_play_runing(void)
{
    return ((g_rec_play_hdl.player == NULL) ? 0 : 1);
}

static int recorde_file_play_callback(void *_priv, int parm, enum stream_event event)
{
    /* printf(">>>>> recorde_file_play_callback: %x, parm:%d\n!!\n", event, parm); */
    if (event == STREAM_EVENT_STOP) {
        /* y_printf(">> STREAM_EVENT_STOP!\n"); */
        app_recorder_file_play_stop();
#if TCFG_RECORD_AUDIO_REPLAY_EN
        app_recorder_play_record_folder();
#endif
    } else if (event == STREAM_EVENT_START) {

    }
    return 0;
}

//需要播放的录音文件夹
void app_recorder_play_record_folder(void)
{
    app_local_record_mutex_create();
    app_recorder_fsn_scan();
    /* g_rec_play_hdl.file_index = 1;			//下标重新设置为1 */
    FILE *file = NULL;
    if (g_rec_play_hdl.fsn && g_rec_play_hdl.file_index <= g_rec_play_hdl.fsn->file_number) {
        g_printf(">>>>> g_rec_play_hdl.file_index:%d, g_rec_play_hdl.fsn->file_number:%d!\n", g_rec_play_hdl.file_index, g_rec_play_hdl.fsn->file_number);
        file = fselect(g_rec_play_hdl.fsn, FSEL_BY_NUMBER, g_rec_play_hdl.file_index++);
    } else {
        r_printf(">>>>> Record Play Ended!!  g_rec_play_hdl.file_index:%d, g_rec_play_hdl.fsn->file_number:%d!\n", g_rec_play_hdl.fsn->file_number, g_rec_play_hdl.fsn->file_number);
#if 0
        //录音文件一直循环播放
        g_rec_play_hdl.file_index = 1;
        if (g_rec_play_hdl.file_index <= g_rec_play_hdl.fsn->file_number) {
            file = fselect(g_rec_play_hdl.fsn, FSEL_BY_NUMBER, g_rec_play_hdl.file_index++);
        }
#endif
    }
    if (file) {
        g_rec_play_hdl.file = file;
        g_rec_play_hdl.latest_play_file = file;
        g_rec_play_hdl.player = music_file_play_callback(g_rec_play_hdl.file, NULL, recorde_file_play_callback, NULL);
    } else {
        r_printf(">> file is NULL!, release fsn!!\n");
        g_rec_play_hdl.player = NULL;		//播放结束，加上这句防止ui继续访问异常
        if (g_rec_play_hdl.fsn) {
            fscan_release(g_rec_play_hdl.fsn);
            g_rec_play_hdl.fsn = NULL;
            g_rec_play_hdl.file_index = 1;			//已经循环播放了一轮, 下标重新设置为1
        }
        //如果当前模式是音乐模式，则继续播放其它音乐
        if (app_get_current_mode()->name == APP_MODE_MUSIC) {
            r_printf("#################### play record file ended! will play other music!!\n");
            app_send_message(APP_MSG_MUSIC_PLAYE_NEXT_FOLDER, 0);
        }
    }
}

// 播放下一首录音文件
void app_recorder_file_play_next(void)
{
    if (is_recorder_file_play_runing()) {
        y_printf(">>> recorder file play next!!\n");
        app_recorder_file_play_stop();
        /* g_rec_play_hdl.file_index++; */
        app_recorder_play_record_folder();
    }
}

// 播放上一首录音文件
void app_recorder_file_play_prev(void)
{
    if (is_recorder_file_play_runing()) {
        y_printf(">>> recorder file play prev!!\n");
        app_recorder_file_play_stop();
        if (g_rec_play_hdl.file_index - 2 >= 1) {
            g_rec_play_hdl.file_index = g_rec_play_hdl.file_index - 2;
        } else {
            app_recorder_fsn_scan();
            if (g_rec_play_hdl.fsn) {
                g_rec_play_hdl.file_index = g_rec_play_hdl.fsn->file_number;
            }
        }
        app_recorder_play_record_folder();
    }
}

//录音模式删除最近播放的文件
void app_recorder_del_cur_play_file(void)
{
    if (music_player_runing()) {
        music_file_player_stop();
        g_rec_play_hdl.player = NULL;
    }

    if (g_rec_play_hdl.fsn) {
        fscan_release(g_rec_play_hdl.fsn);
        g_rec_play_hdl.fsn = NULL;
        g_rec_play_hdl.player = NULL;		//播放结束，加上这句防止ui继续访问异常
    }

    if (g_rec_play_hdl.latest_play_file) {
        int err = fdelete(g_rec_play_hdl.latest_play_file);
        if (err) {
            //删除失败
            r_printf("[%s, %d] Recorder file delete failed!\n", __func__, __LINE__);
            fclose(g_rec_play_hdl.latest_play_file);
            if (g_rec_play_hdl.latest_play_file == g_rec_play_hdl.file) {
                g_rec_play_hdl.file = NULL;
                g_rec_play_hdl.latest_play_file = NULL;
            } else {
                g_rec_play_hdl.latest_play_file = NULL;
            }
        } else {
            printf("%s Delete file success!\n", __func__);
            if (g_rec_play_hdl.latest_play_file == g_rec_play_hdl.file) {
                g_rec_play_hdl.file = NULL;
                g_rec_play_hdl.latest_play_file = NULL;
            } else {
                g_rec_play_hdl.latest_play_file = NULL;
            }
        }
    } else {
        r_printf("%s, %d, Delete file Failed!!\n", __func__, __LINE__);
    }
}
#endif


struct app_mode *app_enter_record_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    y_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Enter Record Mode!!\n");
    app_record_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), record_mode_key_table)) {
            printf("app_get_message");
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            printf("next mode %d", next_mode->name);
            break;
        }

        printf("msg %x", msg[0]);
        switch (msg[0]) {
        case MSG_FROM_APP:
            record_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_record_exit();

    return next_mode;
}

static int record_mode_try_enter()
{
    if (dev_manager_get_total(0)) {
        return 0;
    }
    return -1;
}

static int record_mode_try_exit()
{
    return 0;
}

static const struct app_mode_ops record_mode_ops = {
    .try_enter      = record_mode_try_enter,
    .try_exit       = record_mode_try_exit,
};

/*
 * 注册record模式
 */
REGISTER_APP_MODE(record_mode) = {
    .name 	= APP_MODE_RECORD,
    .index  = APP_MODE_RECORD_INDEX,
    .ops 	= &record_mode_ops,
};


static u8 record_idle_query(void)
{
    return record_idle_flag;
}

REGISTER_LP_TARGET(record_lp_target) = {
    .name = "record",
    .is_idle = record_idle_query,
};

#endif

