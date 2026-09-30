#include "sensor_hub.h"
#include "health_manager.h"
#include "sensor_service.h"
#include "ui_core.h"
#include "ui/ui_api.h"
#include "sensor_driver_p11.h"
#include "shm_info_storage.h"
/* pmc_gsensor: P11 模式保留翻腕事件收发与亮灭屏 */
#if (TCFG_SPORT_HEALTH_ENABLE && TCFG_SENSOR_HUB && \
	(TCFG_ACCELER_MASTER_ENABLE || TCFG_ACCELER_P11_ENABLE))

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
    log_info("[wrist] event:%d cur_win:%d", wrist_tilt_event, ui_get_current_window_id());
    /*床头时钟不响应*/
    if (ui_get_current_window_id() == ID_WINDOW_BEDSIDE_WATCH) {
        log_info("%s curr_win is bedside_watch", __func__);
        return;
    }
    switch (wrist_tilt_event) {
    case ALGO_WRIST_UP:
        log_info("[wrist] WRIST_UP -> screen on");
        ui_screen_recover(1);//亮屏
        break;

    case ALGO_WRIST_DOWN:
        log_info("[wrist] WRIST_DOWN -> screen off");
        ui_screen_saver(NULL);//灭屏
        break;

    default:
        log_info("[wrist] unknown event:%d, ignored", wrist_tilt_event);
        break;
    }
    wrist_tilt_event = ALGO_NOTHING;
}

/* 被其它模块（罗盘）抢占 gsensor 期间为 1：此时开关变化只记意图不下发，
 * 由占用方（ui_action_compass.c）成对置位/清零。 */
static u8 wrist_tilt_suspended;

void wrist_tilt_set_suspended(u8 suspended)
{
    log_debug("[wrist] suspended:%d -> %d", wrist_tilt_suspended, suspended);
    wrist_tilt_suspended = suspended;
}

/* 抬腕检测下发：on=1 请求使能，on=0 请求关闭（跨核 M2P，或 master 直连 sensor_hub） */
static int wrist_tilt_set_p11(int on, const char *from)
{
    log_info("[wrist] %s switch:%d -> p11:%s", from, on, on ? "ENABLE" : "DISABLE");
#if TCFG_ACCELER_MASTER_ENABLE
    return sensor_hub_enable(SENSOR_ALGO_WRIST_TILT, on);
#else
    sensor_driver_p11_ctrl(SHM_MOD_DET_WRIST, on ? SHM_CMD_ENABLE : SHM_CMD_DISABLE);
    return SHM_ERR_OK;
#endif
}

static int wrist_tilt_io_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_INIT:
    case SHM_CMD_ENABLE:
        /* 开关闸门：使能请求来自 INIT / 罗盘退出 / 开关变化三处，
         * 一律以「抬腕亮屏开关」为准，关闭时下发 DISABLE。 */
        if (wrist_tilt_suspended) {
            /* 占用期间只记意图不下发，罗盘退出时会发 ENABLE 恢复 */
            log_debug("[wrist] %s suspended, defer", (cmd == SHM_CMD_INIT) ? "init" : "enable");
            break;
        }
        ret = wrist_tilt_set_p11(sport_info_raise_wrist_enabled(),
                                 (cmd == SHM_CMD_INIT) ? "init" : "enable");
        break;
    case SHM_CMD_DISABLE:
        /* 强制关闭（罗盘界面进入时占用 gsensor），不过开关闸门 */
        ret = wrist_tilt_set_p11(0, "disable");
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


/* Adapter for .sensor_service segment: enable flag -> legacy io_ctrl cmd */
static int wrist_tilt_enable(int enable)
{
    return wrist_tilt_io_crtl(enable ? SHM_CMD_ENABLE : SHM_CMD_DISABLE, NULL);
}

REGISTER_SPORT_HEALTH_MODULE(wrist_tilt)
{
    .module = SHM_MOD_DET_WRIST,
    .io_ctrl = wrist_tilt_io_crtl,
};

/* sensor_service registration (REQ-013): same impl exposed via function pointer table */
REGISTER_SENSOR_SERVICE(wrist_tilt_service)
{
    .mod        = SHM_MOD_DET_WRIST,
    .init       = NULL,
    .enable     = wrist_tilt_enable,
    .get_result = NULL,
    .set_config = NULL,
};

#else
/* 抬腕检测链路未编译时提供空实现，保证罗盘页面的调用点始终可链接 */
void wrist_tilt_set_suspended(u8 suspended)
{
}
#endif //TCFG_SPORT_HEALTH_ENABLE & (TCFG_ACCELER_MASTER_ENABLE | TCFG_ACCELER_P11_ENABLE)
