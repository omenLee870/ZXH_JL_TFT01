#include "chargeIc_manage.h"
#include "device/device.h"
#include "app_config.h"
#include "app_main.h"
#include "user_cfg.h"
#include "chgbox_det.h"
#include "chgbox_ctrl.h"
#include "chgbox_wireless.h"

#if (defined TCFG_CHARGE_IC_SY7609 &&TCFG_CHARGE_IC_SY7609 )

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHG_IC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


static int charge_ic_init()
{
    gpio_set_mode(IO_PORT_SPILT(TCFG_BOOST_CTRL_IO), PORT_OUTPUT_LOW);
#if (TCFG_PWR_CTRL_TYPE == PWR_CTRL_TYPE_PU_PD)
    gpio_set_mode(IO_PORT_SPILT(TCFG_PWR_CTRL_IO), PORT_INPUT_PULLDOWN_10K);
#else
    power_gate_open_drain_output(TCFG_PWR_CTRL_IO, PORT_OUTPUT_HIGH);
#endif
    gpio_set_mode(IO_PORT_SPILT(TCFG_STOP_CHARGE_IO), PORT_INPUT_FLOATING);
    return 0;
}
static int charge_ic_uninit()
{
    return 0;
}
static int charge_ic_charge_start()
{
    gpio_set_mode(IO_PORT_SPILT(TCFG_STOP_CHARGE_IO), PORT_INPUT_FLOATING);
    usb_charge_full_wakeup_deal();//外挂充电,开充电时主动去查询一次是否充满

    power_set_mode(PWR_LDO15);
    return 0;
}
static int charge_ic_charge_stop()
{
    gpio_set_mode(IO_PORT_SPILT(TCFG_STOP_CHARGE_IO), PORT_OUTPUT_HIGH);
    return 0;
}
static int charge_ic_boost_ctrl(u32 en)
{
    gpio_set_mode(IO_PORT_SPILT(TCFG_BOOST_CTRL_IO), PORT_OUTPUT_HIGH);
    os_time_dly(1);
    gpio_set_mode(IO_PORT_SPILT(TCFG_BOOST_CTRL_IO), PORT_OUTPUT_LOW);
    return 0;
}
static int charge_ic_pwr_crtl(u32 en)
{
#if (TCFG_PWR_CTRL_TYPE == PWR_CTRL_TYPE_PU_PD)
    if (en == 0) {
        gpio_set_pull_up(TCFG_PWR_CTRL_IO, 0);
        gpio_set_pull_down(TCFG_PWR_CTRL_IO, 1);
    } else {
        gpio_set_pull_down(TCFG_PWR_CTRL_IO, 0);
        gpio_set_pull_up(TCFG_PWR_CTRL_IO, 1);
    }
#elif (TCFG_PWR_CTRL_TYPE == PWR_CTRL_TYPE_OUTPUT_0)
    if (en == 0) {
        // gpio_direction_input(TCFG_PWR_CTRL_IO);
        // gpio_set_mode(IO_PORT_SPILT(TCFG_PWR_CTRL_IO),PORT_INPUT_FLOATING);
        power_gate_open_drain_output(TCFG_PWR_CTRL_IO, PORT_OUTPUT_HIGH);
    } else {
        // gpio_direction_output(TCFG_PWR_CTRL_IO, 0);
        // gpio_set_mode(IO_PORT_SPILT(TCFG_PWR_CTRL_IO),PORT_OUTPUT_LOW);
        // gpio_write_port(IO_PORT_SPILT(TCFG_PWR_CTRL_IO), PORT_OUTPUT_LOW);
        power_gate_open_drain_output(TCFG_PWR_CTRL_IO, PORT_OUTPUT_LOW);
    }
#elif (TCFG_PWR_CTRL_TYPE == PWR_CTRL_TYPE_OUTPUT_1)
    if (en == 0) {
        gpio_set_mode(IO_PORT_SPILT(TCFG_STOP_CHARGE_IO), PORT_INPUT_FLOATING);
    } else {
        gpio_set_mode(IO_PORT_SPILT(TCFG_PWR_CTRL_IO), PORT_OUTPUT_HIGH);
    }
#endif
    return 0;
}


static int charge_ic_ioctrl(u32 cmd, u32 arg)
{
    int ret = 0;
    switch (cmd) {
    case CHARGE_IC_CMD_INIT:
        if (arg) {
            ret = charge_ic_init();
        } else {
            ret = charge_ic_uninit();
        }
        break;
    case CHARGE_IC_CMD_CHARGE:
        if (arg) {
            ret = charge_ic_charge_start();
        } else {
            ret = charge_ic_charge_stop();
        }
        break;
    case CHARGE_IC_CMD_BOOST:
        ret = charge_ic_boost_ctrl(arg);
        break;
    case CHARGE_IC_CMD_PWR:
        ret = charge_ic_pwr_crtl(arg);
        break;
    default:
        break;
    }
    return ret;
}

REGISTER_CHAREG_IC_MODULE(sy7609)
{
    .io_ctrl = charge_ic_ioctrl,
};

#endif//





