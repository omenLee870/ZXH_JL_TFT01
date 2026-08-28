#ifndef __MAIN_H__
#define __MAIN_H__

enum {
    MSG_APP_CALLBACK         =    1,
    MSG_P11_SYS_RAM_INIT     =    2,
    MSG_P11_SYS_KICK         =    3,
    MSG_P11_SYS_TO_SELF      =    4,
    MSG_P11_SENSOR_INFO      =    5,
    MSG_P11_SENSOR_INIT      =    6,
    MSG_P11_SENSOR_EVENT     =    7,
    MSG_P11_SENSOR_TIMER     =    8,
    MSG_P11_ALGORITHM_EVENT  =    9,
    MSG_P11_SOFF_EVENT       =    10,
    MSG_P11_SENSOR_SLEEP     =    11,
    MSG_P11_SENSOR_IRQ       =    12,
    MSG_P11_POWER_RECOVER    =    13,
    MSG_P11_HEALTH_MANAGER_LOOP = 	 14,
    MSG_P11_HEALTH_MANAGER_DATA_M2P  =  15,
    MSG_P11_HEALTH_MANAGER_DATA_P2M = 16,
    MSG_P11_HEALTH_MANAGER_DATA_P2P = 17,
    MSG_P11_SYS_WAKE         =    0xff,
};

void app_handler_add(void (*callback)(u32 *priv), u32 *priv);

extern int task_post_msg(char *name, int argc, ...);




#endif
