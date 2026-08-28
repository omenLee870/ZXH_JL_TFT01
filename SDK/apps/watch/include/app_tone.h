#ifndef APP_TONE_H
#define APP_TONE_H

#include "tone_player.h"
#include "ring_player.h"
#include "tws_tone_player.h"

enum tone_language {
    TONE_ENGLISH,
    TONE_CHINESE,
};



struct tone_files {
    const char *num[10];
    const char *power_on;
    const char *power_off;
    const char *bt_mode;
    const char *bt_connect;
    const char *bt_disconnect;
    const char *phone_in;
    const char *phone_out;
    const char *charge_start;
    const char *charge_full;
    const char *low_power;
    const char *max_vol;
    const char *normal;
    const char *rtc_mode;
    const char *pc_mode;			// pc模式
    const char *key_tone;
    const char *ring_volume;        // 音量调节声
    const char *bt_discon[3];   	// 蓝牙断连
    const char *bt_scan;     		// 蓝牙扫描
    const char *bt_conn;  			// 蓝牙连接
    const char *factory_reset;      // 恢复出厂设置
    const char *reset_complete;     // 重置成功
    const char *ota_start;          // OTA开始
    const char *ota_reminder;       // OTA提醒
    const char *low_battery;        // 低电量
    const char *auto_power_off;     // 自动关机
    const char *charging_on;        // 充电开启
    const char *charging_off;       // 充电断开
    const char *touch_tone;         // 触摸音
    const char *camera_shutter;     // 拍照快门
    const char *video_recording;    // 录像提示
    const char *music_mode;    		// 音乐模式
    const char *battery_status;     // 电池状态"当前电量"
    const char *battery_lvl[13];    // 电量等级"百分之x"
};


void tone_language_set(enum tone_language lang);

enum tone_language tone_language_get();

const struct tone_files *get_tone_files();


#endif



