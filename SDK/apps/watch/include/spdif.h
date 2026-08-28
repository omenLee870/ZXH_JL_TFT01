#ifndef _SPDIF_APP_H_
#define _SPDIF_APP_H_

#include "system/event.h"

struct app_mode *app_enter_spdif_mode(int arg);
int spdif_app_msg_handler(int *msg);

#endif


