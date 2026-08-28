#include "sensor_hub.h"
#include "health_manager.h"
#include "ui/ui_api.h"
#if TCFG_SPORT_HEALTH_ENABLE &&TCFG_SENSOR_HUB	&& (TCFG_ACCELER_MASTER_ENABLE || TCFG_ACCELER_P11_ENABLE)

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_SRV_WRIST]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"



#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sensor_hub.data.bss")
#pragma data_seg(".sensor_hub.data")
#pragma const_seg(".sensor_hub.text.const")
#pragma code_seg(".sensor_hub.text")
#endif

extern void ui_screen_recover(u8 recover_cur_page);
extern void ui_screen_saver(void *p);
void wrist_tilt_event_handler(int wrist_tilt_event)
{
    log_info("event=%d", wrist_tilt_event);
    /*床头时钟不响应*/
    if (ui_get_current_window_id() == ID_WINDOW_BEDSIDE_WATCH) {
        log_info("%s curr_win is bedside_watch", __func__);
        return;
    }
    switch (wrist_tilt_event) {
    case ALGO_WRIST_UP:
        ui_screen_recover(1);//亮屏
        break;

    case ALGO_WRIST_DOWN:

        ui_screen_saver(NULL);//灭屏
        break;

    default:
        break;
    }
    wrist_tilt_event = ALGO_NOTHING;
}

static int wrist_tilt_io_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_INIT:
    case SHM_CMD_ENABLE:
        ret = sensor_hub_enable(SENSOR_ALGO_WRIST_TILT, 1);
        break;
    case SHM_CMD_DISBALE:
        ret = sensor_hub_enable(SENSOR_ALGO_WRIST_TILT, 0);
        break;
    case SHM_CMD_UPDATE:
        wrist_tilt_event_handler((int)priv);
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }

    log_debug("cmd=%d ret:%d", cmd, ret);
    return ret;
}


REGISTER_SPORT_HEALTH_MODULE(wrist_tilt)
{
    .module = SHM_MOD_DET_WRIST,
     .io_ctrl = wrist_tilt_io_crtl,
};

#endif //TCFG_SPORT_HEALTH_ENABLE & (TCFG_ACCELER_MASTER_ENABLE | TCFG_ACCELER_P11_ENABLE)
