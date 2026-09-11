#ifndef __RCSP_MIX_RECORD_H__
#define __RCSP_MIX_RECORD_H__

#include "app_config.h"

#if RCSP_MIX_RECORD_ENABLE
int rcsp_mix_record_device_msg_handler(int *msg);
int rcsp_mix_recorder_get_status(void);
void rcsp_mix_recorder_stop(void);
#endif

#endif /* __RCSP_MIX_RECORD_H__ */
