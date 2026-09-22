#ifndef _PC_H_
#define _PC_H_

#include "system/event.h"
#include "music/music_player.h"


struct app_mode *app_enter_pc_mode(int arg);
int pc_app_msg_handler(int *msg);
//pc从机在线状态: 1 在线, 0 下线
u8 pc_get_online_status(void);


#endif
