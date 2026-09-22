#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "system/event.h"
#include "os/os_api.h"
#include "app_main.h"

#include "usb/usb_common_def.h"
#include "usb/usb_config.h"
#include "usb/usb_task.h"
#include "usb/device/usb_stack.h"
#include "usb/host/usb_host.h"
#include "usb/otg.h"
#include "usb_multiplex.h"
#include "utils/resource_multiplex.h"


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_MULT]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

typedef enum {
    USB_MULT_NONE,
    USB_MULT_STOR,
    USB_MULT_PC,
} mult_state_t;

struct usb_multiplex_info {
    mult_state_t state;
    u8 mount;
    struct pc_multiplex_callback pc_cb;
};
static struct usb_multiplex_info mult;

#define USB_RESET_DELAY     20


#if TCFG_USB_HOST_ENABLE
int usb_storage_multiplex_resume(const struct usb_host_device *host_dev)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("usb_storage_multiplex_resume, rets=0x%x\n", rets_addr);

    int err = 0;
    usb_dev usb_id = host_device2id(host_dev);
    u32 requester = (u32)host_dev;

    if (resource_multiplex_is_registered(IO_PORT_DM) == 0) {
        return 0;
    }
    if (mult.state == USB_MULT_NONE) {
        resource_multiplex_check_request(IO_PORT_DM,
                                         requester,
                                         WAIT_RESOURCE_FREE,
                                         NULL);

        usb_otg_io_resume(usb_id);
        /*
         * 有一部分U盘在DM和SD DAT0复用时，插入U盘或者模式切到播U盘，
         * 需要把DPDM拉低一段时间，让U盘自己认为插入了主机
         */
        usb_otg_suspend(usb_id, 0);
        os_time_dly(USB_RESET_DELAY);
        usb_otg_resume(usb_id);

        if (!mult.mount) {
            err = usb_host_mount(usb_id,
                                 0,
                                 TCFG_USB_HOST_MOUNT_RETRY,
                                 TCFG_USB_HOST_MOUNT_RESET,
                                 TCFG_USB_HOST_MOUNT_TIMEOUT);
            if (err) {
                goto __exit;
            }
            mult.mount = 1;
        } else {
            err = usb_host_remount(usb_id,
                                   0,
                                   TCFG_USB_HOST_MOUNT_RETRY,
                                   TCFG_USB_HOST_MOUNT_RESET,
                                   TCFG_USB_HOST_MOUNT_TIMEOUT,
                                   0);
            if (err) {
                goto __exit;
            }
        }
        mult.state = USB_MULT_STOR;
    }
__exit:
    if (err) {
        usb_otg_io_suspend(usb_id);
        resource_multiplex_free(IO_PORT_DM, requester);
    }
    return err;
}

int usb_storage_multiplex_suspend(const struct usb_host_device *host_dev)
{

    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("usb_storage_multiplex_suspend, rets=0x%x\n", rets_addr);

    u32 requester = (u32)host_dev;
    usb_dev usb_id = host_device2id(host_dev);
    int err = 0;

    if (resource_multiplex_is_registered(IO_PORT_DM) == 0) {
        return 0;
    }
    if (mult.state == USB_MULT_STOR) {
        if (usb_otg_online(usb_id) == OTG_STATE_HOST_MODE) {
            usb_otg_io_suspend(usb_id);
        }
        if (mult.mount) {
            usb_host_unmount(usb_id, 0);
            mult.mount = 0;
        }
        resource_multiplex_free(IO_PORT_DM, requester);
        mult.state = USB_MULT_NONE;
    }
    return 0;
}

/*
 * 只把multiplex记账拉回USB_MULT_NONE,不做suspend/unmount动作
 *
 * host被别处直接usb_host_unmount()关掉时(usb_task.c/hub.c/idle.c),
 * usb_stor_release()必须调它,否则mult.state仍是USB_MULT_STOR,
 * 下一次usb_storage_multiplex_resume()会跳过mount直接返回0,
 * 导致usb_stor_init()报offline,重新插入的U盘收不到DEVICE_EVENT_IN
 *
 * 这里不碰OTG/PHY: release跑在_usb_host_unmount()的接口循环里,
 * 此时usb_sie_close()还没执行,OTG的关断由发起unmount的一方负责
 */
void usb_storage_multiplex_host_released(const struct usb_host_device *host_dev)
{
    u32 requester = (u32)host_dev;

    if (resource_multiplex_is_registered(IO_PORT_DM) == 0) {
        return;
    }
    if (mult.state == USB_MULT_STOR) {
        mult.mount = 0;
        mult.state = USB_MULT_NONE;
        resource_multiplex_free(IO_PORT_DM, requester);
    }
}
#else

int usb_storage_multiplex_resume(const struct usb_host_device *host_dev)
{
    return 0;
}


int usb_storage_multiplex_suspend(const struct usb_host_device *host_dev)
{
    return 0;
}

void usb_storage_multiplex_host_released(const struct usb_host_device *host_dev)
{
}

#endif

#if TCFG_PC_ENABLE
int pc_multiplex_resume()
{

    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("pc_multiplex_resume, rets=0x%x\n", rets_addr);
    int err = 0;
    u32 requester = (u32)usb_device_mode;

    if (resource_multiplex_is_registered(IO_PORT_DM) == 0) {
        return 0;
    }
    if (mult.state == USB_MULT_NONE) {
        resource_multiplex_check_request(IO_PORT_DM,
                                         requester,
                                         WAIT_RESOURCE_FREE,
                                         NULL);
        gpio_set_mode(IO_PORT_SPILT(IO_PORT_DM), PORT_OUTPUT_LOW);
        os_time_dly(2);
        gpio_set_mode(IO_PORT_SPILT(IO_PORT_DM), PORT_HIGHZ);
        usb_otg_resume(0);
        mult.state = USB_MULT_PC;
    }
    return err;
}

int pc_multiplex_suspend()
{

    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("pc_multiplex_suspend, rets=0x%x\n", rets_addr);
    u32 requester = (u32)usb_device_mode;

    if (resource_multiplex_is_registered(IO_PORT_DM) == 0) {
        return 0;
    }
    if (mult.state == USB_MULT_PC) {
        //usb_otg_resume(0);
        resource_multiplex_free(IO_PORT_DM, requester);
        mult.state = USB_MULT_NONE;
    }
    return 0;
}

void pc_multiplex_register_callback(struct pc_multiplex_callback *callback)
{
    memcpy(&mult.pc_cb, callback, sizeof(struct pc_multiplex_callback));
}

void pc_multiplex_close_other_device()
{
    if (mult.pc_cb.close_other_device) {
        mult.pc_cb.close_other_device(mult.pc_cb.pc_mult_priv);
    }
}

void pc_multiplex_open_other_device()
{
    if (mult.pc_cb.open_other_device) {
        mult.pc_cb.open_other_device(mult.pc_cb.pc_mult_priv);
    }
}
#else
int pc_multiplex_resume()
{
    return 0;
}

int pc_multiplex_suspend()
{
    return 0;
}

void pc_multiplex_register_callback(struct pc_multiplex_callback *callback)
{

}

void pc_multiplex_close_other_device()
{

}

void pc_multiplex_open_other_device()
{

}

#endif
