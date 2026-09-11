#ifndef __RCSP_H__
#define __RCSP_H__

#include "typedef.h"
#include "app_config.h"

#include "rcsp_config.h"
#include "rcsp_device_status.h"
#include "rcsp_browser.h"
#include "rcsp_cmd_user.h"
#include "rcsp_task.h"

void rcsp_init(void);
void rcsp_exit(void);

void rcsp_timer_contrl(u8 status);

#if RCSP_FILE_TRANS_SPP_WAKEUP_FLOW_CTRL
void rcsp_spp_tx_wait_prepare(void);
int rcsp_spp_tx_wait(void);
void rcsp_spp_tx_wait_wakeup(void);
int rcsp_spp_tx_flow_ctrl_ready(void);
u32 rcsp_spp_tx_wakeup_count_get(void);
#endif

#endif // __RCSP_H__
