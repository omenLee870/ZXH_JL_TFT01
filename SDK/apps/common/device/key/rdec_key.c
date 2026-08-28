#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rdec_key.data.bss")
#pragma data_seg(".rdec_key.data")
#pragma const_seg(".rdec_key.text.const")
#pragma code_seg(".rdec_key.text")
#endif
#include "app_config.h"
#include "gpio.h"
#include "rdec.h"
#include "system/timer.h"
#include "key_driver.h"
#include "rdec_key.h"

#if TCFG_RDEC_KEY_ENABLE

extern const struct rdec_platform_data rdec_key_data;
static const struct rdec_platform_data *__this = NULL;

static u32 rdec_id;
void rdec_get_key_value(void *p)
{
    int i;
    s8 rdec_data;
    struct key_event key = {0};

    if (!__this->enable) {
        return;
    }

    for (i = 0; i < __this->num; i++) {
        rdec_data = rdec_get_value(rdec_id);
        if (rdec_data != 0) {
            key.init = 1;
            key.type = KEY_DRIVER_TYPE_RDEC;
            key.event = KEY_ACTION_RDEC_ROTATE;
            key.value = rdec_data;
            key.tmr = sys_timer_get_ms();
            key_event_handler(&key);
        }
    }
}

__attribute__((weak))
const struct rdec_platform_data  *get_rdeckey_platform_data()
{
    return NULL;
}

int rdec_key_init(void)
{
    /* __this = &rdec_key_data; */
    __this = get_rdeckey_platform_data();
    if (!__this) {
        return -EINVAL;
    }
    if (!__this->enable) {
        return KEY_NOT_SUPPORT;
    }

    for (int i = 0; i < __this->num; i++) {
        const struct rdec_config user_cfg = {
            .rdec_a = __this->rdec[i].sin_portA,
            .rdec_b = __this->rdec[i].sin_portB,
            .phase_mode = __this->rdec[i].phase_mode,
            .work_mode = RDEC_WORK_AUTO,
            .rdec_x = RDEC_0,
        };

        gpio_set_mode(IO_PORT_SPILT(__this->rdec[i].sin_portA), __this->rdec[i].sin_portA_io_mode);
        gpio_set_mode(IO_PORT_SPILT(__this->rdec[i].sin_portB), __this->rdec[i].sin_portB_io_mode);
        rdec_id = rdec_init(&user_cfg);
        rdec_start(rdec_id);
        sys_s_hi_timer_add(NULL, rdec_get_key_value, __this->rdec[i].scan_time);
    }

    printf("[rdec] rdec_key_init ");

    return rdec_id;
}


#endif  /* #if TCFG_RDEC_KEY_ENABLE */

