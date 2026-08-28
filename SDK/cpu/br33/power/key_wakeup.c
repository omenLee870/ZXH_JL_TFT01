#include "asm/power_interface.h"
#include "app_config.h"
#include "gpio.h"
#include "iokey.h"
#include "adkey.h"

void key_active_set(u8 port);
static void key_wakeup_callback(P33_IO_WKUP_EDGE edge)
{
    key_active_set(0);
}

//只读配置，用户后续新增io唤醒、ad唤醒注意使用const修饰，节约静态ram
static const struct _p33_io_wakeup_config port0 = {
    .pullup_down_mode  = PORT_INPUT_PULLUP_10K,
    .filter      		= PORT_FLT_DISABLE,
    .edge               = FALLING_EDGE,
    .gpio              = IO_PORTE_03,
    .callback          = key_wakeup_callback,
};

void key_wakeup_init()
{
#if 0//(!TCFG_LP_TOUCH_KEY_ENABLE)
    p33_io_wakeup_port_init(&port0);
    p33_io_wakeup_enable(IO_PORTE_03, 1);
#endif

#if TCFG_IOKEY_ENABLE
    const struct iokey_platform_data *iokey_info = get_iokey_platform_data();
    if (iokey_info) {
        struct _p33_io_wakeup_config iokey_port = {
            .pullup_down_mode  	= PORT_INPUT_PULLUP_10K,
            .filter      		= PORT_FLT_DISABLE,
            .edge           	= FALLING_EDGE,
            /* .gpio           	= IO_PORTE_04, */
            .callback       	= key_wakeup_callback,
        };
        for (int i = 0; i < iokey_info->num; i++) {
            int gpio = iokey_info->port[i].key_type.one_io.port;
            iokey_port.gpio = gpio;
            if (iokey_info->press_wakeup_enable[i]) {
                p33_io_wakeup_port_init(&iokey_port);
                p33_io_wakeup_enable(gpio, 1);
            }
        }
    }
#endif

#if TCFG_ADKEY_ENABLE
    const struct adkey_platform_data *adkey_info = get_adkey_platform_data();
    if (adkey_info) {
        struct _p33_io_wakeup_config adkey_port = {
            /* .pullup_down_mode  	= PORT_INPUT_PULLUP_10K, */
            .filter      		= PORT_FLT_DISABLE,
            .edge           	= FALLING_EDGE,
            /* .gpio           	= IO_PORTE_03, */
            .callback       	= key_wakeup_callback,
        };
        int gpio = adkey_info->adkey_pin;
        adkey_port.gpio = gpio;
        if (adkey_info->extern_up_en) {
            adkey_port.pullup_down_mode = PORT_INPUT_FLOATING;
        } else {
            adkey_port.pullup_down_mode = PORT_INPUT_PULLUP_100K;
        }
        p33_io_wakeup_port_init(&adkey_port);
        p33_io_wakeup_enable(gpio, 1);
    }
#endif

}
