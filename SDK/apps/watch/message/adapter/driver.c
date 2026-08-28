#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".driver.data.bss")
#pragma data_seg(".driver.data")
#pragma const_seg(".driver.text.const")
#pragma code_seg(".driver.text")
#endif
#include "app_msg.h"
#include "system/event.h"
#include "usb/usb_task.h"
#include "app_config.h"
#include "utils/resource_multiplex.h"
#if (TCFG_APP_VIDEO_REC_EN || TCFG_APP_VIDEO_DEC_EN || TCFG_APP_PHOTO_EN)
static volatile u8 g_sdx_online = 1;

u8 sd1_driver_online(void)
{
    return g_sdx_online;
}

u8 sd0_driver_online(void)
{
    return g_sdx_online;
}
#endif /* TCFG_APP_VIDEO_REC_EN || TCFG_APP_VIDEO_DEC_EN || TCFG_APP_PHOTO_EN */

void sdx_dev_event_to_user(u32 arg, u8 sdx_status, u8 sdx_index)
{

    int msg[3];

#if (TCFG_APP_VIDEO_REC_EN || TCFG_APP_VIDEO_DEC_EN || TCFG_APP_PHOTO_EN)
    if (arg == (u32)DRIVER_EVENT_FROM_SD0 || arg == (u32)DRIVER_EVENT_FROM_SD1) {
        g_sdx_online = (sdx_status == DEVICE_EVENT_IN);
    }
#endif /* TCFG_APP_VIDEO_REC_EN || TCFG_APP_VIDEO_DEC_EN || TCFG_APP_PHOTO_EN */

    msg[0] = arg;
    msg[1] = sdx_status;

    if (arg == (u32)DRIVER_EVENT_FROM_SD0) {
        msg[2] = (int)"sd0";
    } else {
        msg[2] = (int)"sd1";
    }

    printf("sd dev msg %x, %x, %x\n", msg[0], msg[1], msg[2]);

    /* os_taskq_post_msg("test_gpu_task", 2, 0xa5a5, msg); */
    /* return ; */

#if (TCFG_DEV_MANAGER_ENABLE)
    os_taskq_post_type("dev_mg", MSG_FROM_DEVICE, 3, msg);
#else
    os_taskq_post_type("app_core", MSG_FROM_DEVICE, 3, msg);
#endif

}

void usb_driver_event_to_user(u32 from, u32 event, void *arg)
{
    int msg[3] = {0};

    msg[0] = from;
    msg[1] = event;
    msg[2] = (int)arg;
#if (TCFG_DEV_MANAGER_ENABLE)
    os_taskq_post_type("dev_mg", MSG_FROM_DEVICE, 3, msg);
#else
    os_taskq_post_type("app_core", MSG_FROM_DEVICE, 3, msg);
#endif
}

void usb_driver_event_from_otg(u32 from, u32 event, void *arg)
{
    if (DEVICE_EVENT_IN == event) {
        printf("[usb_msg]otg_post event:IN\n");
    } else if (DEVICE_EVENT_OUT  == event) {
        printf("[usb_msg]otg_post event:OUT\n");
    } else {
        printf("[usb_msg]otg_post event:%d\n", event);
    }
    os_taskq_post_msg("usb_stack", 4, USBSTACK_OTG_MSG, from, event, arg);
}

void usb_driver_event_from_host(u32 from, u32 event, void *arg)
{
    if (resource_multiplex_is_registered(IO_PORT_DM)) {
        usb_driver_event_to_user(from, event, arg);
    } else {
        os_taskq_post_msg(USB_TASK_NAME, 3, USBSTACK_HOST_MSG, event, arg);
    }
}
