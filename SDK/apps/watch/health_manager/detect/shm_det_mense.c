
#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"


#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-DET-MENSE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if (TCFG_SPORT_HEALTH_ENABLE&&TCFG_SPORT_HEALTH_DET_MENSE)
int mense_resp_check();
static int shm_det_sec_deal()
{
#ifdef CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE
    mense_resp_check();
#endif
    return true;
}
static int shm_det_mense_io_ctrl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_INIT:
        break;
    case SHM_CMD_RELEASE:
        break;
    case SHM_CMD_UPDATE_SEC:
        shm_det_sec_deal();
        break;
    case SHM_CMD_TARGET_SET:
        break;
    case  SHM_CMD_CLEAR_VALUE:
        break;
    case SHM_CMD_SAVE_SINGLE:
    case SHM_CMD_SAVE_CONTINUE:
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }
    return ret;
}


REGISTER_SPORT_HEALTH_MODULE(det_mense)
{
    .module =  SHM_MOD_DET_MENSE,
     .io_ctrl =  shm_det_mense_io_ctrl,
      .get_value =  NULL,
};

#endif



