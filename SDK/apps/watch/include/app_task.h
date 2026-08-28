#ifndef APP_TASK_H
#define APP_TASK_H
#include "typedef.h"
#include "app_mode_manager/app_mode_manager.h"
#include "os/os_type.h"

#define NULL_VALUE      0

enum app_mode_t {
    APP_MODE_IDLE     = 1,
    APP_MODE_UPDATE   = 2,
    APP_MODE_POWERON  = 3,
    APP_MODE_POWEROFF = 3,
    APP_MODE_BT       = 4,
    APP_MODE_MUSIC    = 5,
    APP_MODE_FM       = 6,
    APP_MODE_RECORD   = 7,
    APP_MODE_LINEIN   = 8,
    APP_MODE_RTC      = 9,
    APP_MODE_PC       = 10,
    APP_MODE_SPDIF    = 11,
    APP_MODE_RCSP     = 12,
    APP_MODE_SMARTBOX = 13,
    APP_MODE_VIDEO    = 14,
    APP_MODE_PHOTO   = 15,
    APP_MODE_VIDEO_DEC = 16,
    APP_MODE_VIDEO_REC = 17,

};



//调整以下索引顺序就可以调整模式顺序
enum app_mode_index {
    APP_MODE_BT_INDEX,
    APP_MODE_MUSIC_INDEX,
    APP_MODE_FM_INDEX,
    APP_MODE_RECORD_INDEX,
    APP_MODE_LINEIN_INDEX,
    APP_MODE_RTC_INDEX,
    APP_MODE_PC_INDEX,
    APP_MODE_SPDIF_INDEX,
    APP_MODE_VIDEO_REC_INDEX,
    APP_MODE_PHOTO_INDEX,
    APP_MODE_VIDEO_DEC_INDEX,
};



//这里是为了临时兼容一些旧版sdk 名称
#define APP_POWERON_TASK             APP_MODE_POWERON
#define APP_POWEROFF_TASK            APP_MODE_POWEROFF
#define APP_BT_TASK                  APP_MODE_BT
#define APP_MUSIC_TASK               APP_MODE_MUSIC
#define APP_FM_TASK                  APP_MODE_FM
#define APP_RECORD_TASK              APP_MODE_RECORD
#define APP_LINEIN_TASK              APP_MODE_LINEIN
#define APP_RTC_TASK                 APP_MODE_RTC
#define APP_PC_TASK                  APP_MODE_PC
#define APP_SPDIF_TASK               APP_MODE_SPDIF
#define APP_IDLE_TASK                APP_MODE_IDLE
#define APP_WATCH_UPDATE_TASK        APP_MODE_UPDATE
#define APP_RCSP_ACTION_TASK         APP_MODE_RCSP
#define APP_SMARTBOX_ACTION_TASK     APP_MODE_SMARTBOX

enum {
    APP_MSG_SYS_EVENT = Q_EVENT,

    /* 用户自定义消息 */
    APP_MSG_SWITCH_TASK = Q_USER + 1,
    APP_MSG_USER        = Q_USER + 2,

};


extern int app_task_switch_to(u8 app_task, int priv);




extern int app_task_switch_back();

#define app_get_curr_task  app_get_current_mode_name


#endif
