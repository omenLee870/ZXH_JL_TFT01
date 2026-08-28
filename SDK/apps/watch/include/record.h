#ifndef _RECORD_H_
#define _RECORD_H_

#include "system/event.h"

//录音去头时间,单位ms
#define CUT_HEAD_TIME   (0)
//录音去尾时间,单位ms
#define CUT_TAIL_TIME   (0)
//wav 头的大小
#define WAV_HEAD_SIZE   (13 * 4)

struct app_mode *app_enter_record_mode(int arg);
int record_app_msg_handler(int *msg);
int record_device_msg_handler(int *msg);
void record_ui_del_mutex(void);		//仅供record_ui使用


//用来设置录音文件的后缀名称，例如为 .mp3 或者 .wav等等
void local_set_record_file_suffix(const char *_suffix);
//开始播放录音文件
void app_recorder_file_play_start(void);
//停止播放录音文件，参数决定是否释放盘符
void app_recorder_file_play_stop(void);
//创建录音的互斥量，可供外部调用
void app_local_record_mutex_create(void);
//播放录音文件夹
void app_recorder_play_record_folder(void);
//播放上一首录音文件
void app_recorder_file_play_prev(void);
//播放下一首录音文件
void app_recorder_file_play_next(void);
//是否录音文件正在播放
bool is_recorder_file_play_runing(void);
//删除正在播放的录音文件
void app_recorder_del_cur_play_file(void);
//是否正在录音
u8 is_recorder_runing(void);

//以下两个函数均是给UI调用的
//获取正在解码的录音文件时间
int app_recorder_get_play_file_time(void);
//获取正在编码的录音文件时间
int app_recorder_get_enc_time(void);

#endif
