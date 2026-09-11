#include "app_config.h"
#include "clock.h"
#include "usb/device/usb_suspend_resume.h"
#include "usb/device/usb_stack.h"
#include "usb/otg.h"
#include "gpio.h"
#include "usb_task.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_S&R]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if USB_SUSPEND_RESUME
static u8 usb_status = USB_SLAVE_SUSPEND;
static usb_status_hander status_hander;
void usb_slave_set_status_hander(usb_status_hander hander)
{
    status_hander = hander;
}

void usb_slave_suspend_resume_init(const usb_dev usb_id)
{
    usb_slave_dp_wakeup_init(usb_id);
    usb_slave_dp_wakeup_disable(usb_id);
}
void usb_slave_suspend_resume_deinit(const usb_dev usb_id)
{
    usb_slave_dp_wakeup_disable(usb_id);
    usb_slave_dp_wakeup_deinit(usb_id);
}
u8 usb_slave_status_get()
{
    return usb_status == USB_SLAVE_SUSPEND;
}
void usb_slave_status_set(const usb_dev usb_id, usb_slave_status status)
{
    if (usb_status != status) {
        u32 rets;
        asm volatile("%0 = rets": "=r"(rets));
        log_info("usb_slave_status_set() r:%x %d->%d\n", rets, usb_status, status);

        if (usb_status == USB_SLAVE_SUSPEND &&
            (status == USB_SLAVE_RESUME || status == USB_SLAVE_SOF_READY)) {
            // struct usb_device_t *usb_device = usb_id2device(usb_id);
            // usb_message_to_stack(USBSTACK_SLAVE_RESUME, usb_device, 0);
        }

        usb_status = status;

        if (status_hander) {
            status_hander(usb_id, status);
        }
    }
}

void usb_slave_suspend(const usb_dev usb_id)
{
    if (usb_status == USB_SLAVE_SUSPEND) {
        log_warn("usb_slave_suspend already suspend");
        return;
    }
    if (usb_status != USB_SLAVE_SOF_READY) {
        log_debug("usb_slave_suspend not in SOF_READY status");
        return;
    }

    usb_otg_suspend(usb_id, OTG_SUSPEND);
    usb_slave_status_set(usb_id, USB_SLAVE_SUSPEND);
    usb_phy_suspend(usb_id);
    usb_slave_dp_wakeup_enable(usb_id);
}
void usb_slave_resume(const usb_dev usb_id)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    if (usb_device == NULL) {
        return;
    }

    if (usb_status == USB_SLAVE_RESUME) {
        log_warn("usb_slave_resume already resume");
        return;
    }

    usb_otg_resume(usb_id);
    usb_slave_status_set(usb_id, USB_SLAVE_RESUME);
    usb_slave_dp_wakeup_disable(usb_id);
    usb_phy_resume(usb_id);
}

static void usb_resume_sign(void *priv)
{
    usb_dev usb_id = usb_device2id(priv);
    u32 reg = usb_read_power(usb_id);
    usb_write_power(usb_id, reg | BIT(2));//send resume
    mdelay(8); //1ms~15ms
    usb_write_power(usb_id, reg);//clean resume
}

void usb_remote_wakeup(const usb_dev usb_id)
{
#if USB_REMOTE_WAKEUP_EN

    if (usb_slave_status_get() == USB_SLAVE_SUSPEND) {
        struct usb_device_t *usb_device = usb_id2device(usb_id);

        if (usb_device == NULL) {
            return;
        }

        usb_slave_resume(usb_id);

        if (usb_device->bRemoteWakup) {
            sys_timeout_add(usb_device, usb_resume_sign, 0);
        }
    }

#endif
}
void usb_phy_resume(const usb_dev usb_id)
{
    /* usb_reg_dump(); */
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    if (usb_device == NULL) {
        return;
    }

    usb_write_faddr(usb_id, usb_device->baddr);

    if (usb_device->baddr == 0) {
        usb_device->bDeviceStates = USB_DEFAULT;
    } else {
        usb_device->bDeviceStates = USB_CONFIGURED;
    }
}

void usb_phy_suspend(const usb_dev usb_id)
{
    /* usb_reg_dump(); */
}

void usb_slave_dp_wakeup_enable(const usb_dev usb_id)
{
    log_info("usb_slave_dp_wakeup_enable");
    p33_io_wakeup_enable(IO_PORT_DP, 1);
}
void usb_slave_dp_wakeup_disable(const usb_dev usb_id)
{
    log_info(" usb_slave_dp_wakeup_disable");
    p33_io_wakeup_enable(IO_PORT_DP, 0);
}
static void usb_dp_wkup_cb(P33_IO_WKUP_EDGE edge)
{
    log_info("usb_dp_wkup_cb");
    struct usb_device_t *usb_device = usb_id2device(0);

    if (usb_device == NULL) {
        return;
    }

    usb_message_to_stack(USBSTACK_SLAVE_RESUME, usb_device, 0);
}
static const struct _p33_io_wakeup_config port_dp = {
    .gpio = IO_PORT_DP,
    .pullup_down_mode = PORT_KEEP_STATE,
    .filter = PORT_FLT_DISABLE,
    .edge = FALLING_EDGE,
    .callback = usb_dp_wkup_cb,
};
void usb_slave_dp_wakeup_init(const usb_dev usb_id)
{
    log_info("usb_slave_dp_wakeup_init");
    p33_io_wakeup_port_init(&port_dp);
}
void usb_slave_dp_wakeup_deinit(const usb_dev usb_id)
{
    log_info("usb_slave_dp_wakeup_deinit");
    p33_io_wakeup_port_uninit(IO_PORT_DP);
}

#else

void usb_slave_suspend_resume_init(const usb_dev usb_id)
{

}
void usb_slave_suspend_resume_deinit(const usb_dev usb_id)
{

}
void usb_slave_suspend(const usb_dev usb_id)
{

}
void usb_slave_resume(const usb_dev usb_id)
{

}
void usb_slave_status_set(const usb_dev usb_id, usb_slave_status status)
{

}

#endif
