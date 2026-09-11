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

#if  USB_PC_NO_APP_MODE == 0
#include "app_task.h"
#endif
#include "usb/usb_config.h"
#include "usb/usb_task.h"
#include "usb/device/usb_stack.h"
#include "usb/host/usb_host.h"
#include "usb/otg.h"
#include "usb/host/uac_host.h"
#include "usb/device/usb_suspend_resume.h"
#include "utils/resource_multiplex.h"

#if TCFG_USB_SLAVE_MSD_ENABLE
#include "usb/device/msd.h"
#endif

#if TCFG_USB_SLAVE_MTP_ENABLE
#include "usb/device/mtp.h"
#endif

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#endif

#if TCFG_HUB_HOST_ENABLE
#include "usb/host/hub.h"
#endif
#if TCFG_HID_HOST_ENABLE
#include "usb/host/hid.h"
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
#include "usb/device/cdc.h"
#endif

#if TCFG_HOST_WIRELESS_ENABLE
#include "usbnet.h"
#endif

#if TCFG_HOST_UVC_ENABLE
#include "usb/host/host_uvc.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_TASK]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE

/************************** otg data****************************/
#if TCFG_OTG_MODE
struct otg_dev_data otg_data = {
    .usb_dev_en = TCFG_OTG_USB_DEV_EN,
    .slave_online_cnt = TCFG_OTG_SLAVE_ONLINE_CNT,
    .slave_offline_cnt = TCFG_OTG_SLAVE_OFFLINE_CNT,
    .host_online_cnt = TCFG_OTG_HOST_ONLINE_CNT,
    .host_offline_cnt = TCFG_OTG_HOST_OFFLINE_CNT,
#if ((TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE) \
     && TCFG_CHARGESTORE_PORT == IO_PORT_DP)
    .detect_mode = TCFG_OTG_MODE | OTG_DET_DM_ONLY,
#else
    .detect_mode = TCFG_OTG_MODE,
#endif
    .detect_time_interval = TCFG_OTG_DET_INTERVAL,
};
#endif

extern void usb_cdc_background_run(const usb_dev usbfd);
extern int usb_cdc_background_standby(const usb_dev usbfd);

u8 msd_in_task;
u8 msd_run_reset;
static OS_SEM msg_sem;

#define USB_TASK_SEM_TIMEOUT 200

static void handle_otg_msg(int *msg, char *otg_msg, u8 *is_cdc_background)
{
    int from = msg[2];
    int event = msg[3];
    char *usb_msg = (char *)msg[4];
    usb_dev usb_id;

    if (from != DEVICE_EVENT_FROM_OTG) {
        return;
    }

    log_info("OTG event: %s %s", event == DEVICE_EVENT_IN ? "IN" : "OUT", usb_msg);

    strcpy(otg_msg, usb_msg);
    usb_id = usb_msg[2] - '0';

    if (usb_msg[0] == 's') {

#if TCFG_USB_SLAVE_ENABLE

        if (event == DEVICE_EVENT_IN) {

#if USB_PC_NO_APP_MODE
            usb_start(usb_id);
#else

#if TCFG_USB_CDC_BACKGROUND_RUN
            int ret = usb_cdc_background_standby(usb_id);

            if (!ret) {
                *is_cdc_background = 1;
            }

#else
            int ret = usb_standby(usb_id);
#endif

            if (ret) {
                //如果需要切入pc模式，将otg事件分发给app mode
                usb_driver_event_to_user(from, event, usb_msg);
            }

#endif

        } else if (event == DEVICE_EVENT_OUT) {

#if USB_PC_NO_APP_MODE
            usb_stop(usb_id);
#else
            usb_driver_event_to_user(from, event, usb_msg);
#endif

        }

#endif

    } else if (usb_msg[0] == 'h') {

#if TCFG_USB_HOST_ENABLE

        if (event == DEVICE_EVENT_IN) {

            usb_host_mount(
                usb_id,
                0,
                TCFG_USB_HOST_MOUNT_RETRY,
                TCFG_USB_HOST_MOUNT_RESET,
                TCFG_USB_HOST_MOUNT_TIMEOUT);

        } else if (event == DEVICE_EVENT_OUT) {

            usb_host_unmount(usb_id, 0);

        }

#endif
    }
}
static void handle_slave_msg(int *msg, u8 *is_cdc_background)
{
#if TCFG_USB_SLAVE_ENABLE

    usb_dev usb_id;

    switch (msg[1]) {

    case USBSTACK_START:

        usb_id = msg[2];

#if TCFG_USB_CDC_BACKGROUND_RUN

        if (*is_cdc_background) {
            usb_pause(usb_id);
            *is_cdc_background = 0;
        }

#endif

        usb_start(usb_id);
        os_sem_post(&msg_sem);
        break;

    case USBSTACK_PAUSE:

        usb_id = msg[2];
        usb_pause(usb_id);

#if TCFG_USB_CDC_BACKGROUND_RUN
        usb_cdc_background_run(usb_id);
        *is_cdc_background = 1;
#endif

        os_sem_post(&msg_sem);
        break;

    case USBSTACK_STOP:

        usb_id = msg[2];
        usb_stop(usb_id);
        os_sem_post(&msg_sem);
        break;

    case USBSTACK_SLAVE_SUSPEND:
        usb_id = msg[2];
        usb_slave_suspend(usb_id);
        break;

    case USBSTACK_SLAVE_RESUME:
        usb_id = msg[2];
        usb_slave_resume(usb_id);
        break;

#if TCFG_USB_SLAVE_MSD_ENABLE

    case USBSTACK_MSD_RUN:
        msd_in_task = 1;
        USB_MassStorage((void *)msg[2]);

        if (msd_run_reset) {
            msd_reset((struct usb_device_t *)msg[2], 0);
            msd_run_reset = 0;
        }

        msd_in_task = 0;
        break;
#endif

#if TCFG_USB_SLAVE_MTP_ENABLE

    case USBSTACK_MTP_RUN:
        usb_mtp_process((void *)msg[2]);
        break;
#endif

#if TCFG_USB_CDC_BACKGROUND_RUN

    case USBSTACK_CDC_BACKGROUND:
        usb_id = msg[2];
        usb_cdc_background_run(usb_id);
        *is_cdc_background = 1;
        os_sem_post(&msg_sem);
        break;
#endif

#if TCFG_USB_APPLE_DOCK_EN

    case USBSTACK_IAP_RUN:
        if (apple_mfi_link((void *)msg[2]) == IAP2_LINK_ERR) {
            os_time_dly(1);
            os_taskq_post_msg(USB_TASK_NAME, 2, USBSTACK_IAP_RUN, msg[2]);
        }

        break;
#endif
#if TCFG_USB_SLAVE_CDC_ENABLE
    case USBSTACK_CDC_DATA: {
        const usb_dev usb_id = msg[2];
        const usb_dev cdc_num = msg[3];
        u8 buf[64];
        u32 rlen = cdc_read_data(usb_id, buf, 64, cdc_num);
        cdc_write_data(usb_id, buf, rlen, cdc_num);
        break;
    }
#endif

    }

#endif
}
static void handle_host_msg(int *msg, char *otg_msg, u8 *disk_msg)
{
#if TCFG_USB_HOST_ENABLE

    int event = msg[2];
    char *usb_msg = (char *)msg[3];
    struct device *dev;

    if (!strncmp(usb_msg, "udisk", 5)) {

        if (event == DEVICE_EVENT_IN) {
            //只有U盘尝试open成功才将usb host事件分发给app mode
            //offline事件同理，尝试打开不成功则不分发事件
            dev = dev_open(usb_msg, NULL);

            if (dev) {
                dev_close(dev);
                *disk_msg = 1;
            } else {
                *disk_msg = 0;
            }
        }

        if (*disk_msg) {
            usb_driver_event_to_user(
                DEVICE_EVENT_FROM_OTG,
                event,
                otg_msg);

            usb_driver_event_to_user(
                DEVICE_EVENT_FROM_USB_HOST,
                event,
                usb_msg);
            // os_taskq_post_msg("usb_stack", 4, USBSTACK_TEST);
        }

    } else if (!strncmp(usb_msg, "audio", 5)) {

#if TCFG_HOST_AUDIO_ENABLE

        if (event == DEVICE_EVENT_IN) {

            log_info("uac_host IN %s", usb_msg);
            uac_host_mount(usb_msg[5] - '0');

        } else if (event == DEVICE_EVENT_OUT) {

            log_info("uac_host OUT %s", usb_msg);
            uac_host_unmount(usb_msg[5] - '0');
        }

#endif

    } else if (!strncmp(usb_msg, "hub", 3)) {

#if TCFG_HUB_HOST_ENABLE

        if (event == DEVICE_EVENT_IN) {

            log_info("usb_hub online %s", usb_msg);
            usb_hub_process(usb_msg[3] - '0');

        } else if (event == DEVICE_EVENT_OUT) {

            log_info("usb_hub offline %s", usb_msg);
        }

#endif
    } else if (!strncmp(usb_msg, "hid", 3)) {

#if TCFG_HID_HOST_ENABLE

        if (event == DEVICE_EVENT_IN) {

            log_info("hid_host IN %s", usb_msg);
            hid_process(usb_msg[3] - '0');

        } else if (event == DEVICE_EVENT_OUT) {

            log_info("hid_host OUT %s", usb_msg);
        }

#endif
    } else if (!strncmp(usb_msg, "uvc", 3)) {
#if TCFG_HOST_UVC_ENABLE
        if (event == DEVICE_EVENT_IN) {
            log_info("[UVC_FLOW] handle_host_msg: uvc IN %s, calling video_init", usb_msg);
            int vret = usb_host_video_init(usb_msg[3] - '0', 0);
            log_info("[UVC_FLOW] video_init ret=%d, notifying app", vret);
            usb_driver_event_to_user(DEVICE_EVENT_FROM_USB_HOST, event, usb_msg);
        } else if (event == DEVICE_EVENT_OUT) {
            log_info("[UVC_FLOW] handle_host_msg: uvc OUT %s", usb_msg);
            usb_driver_event_to_user(DEVICE_EVENT_FROM_USB_HOST, event, usb_msg);
        }
#endif
    }

#if TCFG_HOST_WIRELESS_ENABLE
    else if (!strncmp(usb_msg, "wireless", strlen("wireless"))) {
        if (event == DEVICE_EVENT_IN) {
            log_info("wireless online %s", usb_msg);
        } else if (event == DEVICE_EVENT_OUT) {
            log_info("wireless offline %s", usb_msg);
            usb_net_stop_process(0);
        }
    } else if (!strncmp(usb_msg, "at_port", strlen("at_port"))) {
        if (event == DEVICE_EVENT_IN) {
            log_info("at_port online %s", usb_msg);
        } else if (event == DEVICE_EVENT_OUT) {
            log_info("at_port offline %s", usb_msg);
            usb_net_at_port_stop_process(0);
        }
    }
#endif

#endif
}
static void handle_host_remount(int *msg)
{
#if TCFG_USB_HOST_ENABLE

    usb_dev usb_id = ((int *)msg[2])[0];

    int ret = usb_host_remount(
                  usb_id,
                  0,
                  TCFG_USB_HOST_MOUNT_RETRY,
                  TCFG_USB_HOST_MOUNT_RESET,
                  TCFG_USB_HOST_MOUNT_TIMEOUT,
                  0);

    ((int *)msg[2])[1] = ret;

    os_sem_post(&msg_sem);

#endif
}


u32 work_in_slave()
{
#if TCFG_USB_TEST_ENABLE
    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_02), PORT_INPUT_PULLUP_10K);
    udelay(1000);
    return !!gpio_read_port(IO_PORT_SPILT(IO_PORTA_02));
#else
    return 0;
#endif
}
static void usb_task(void *p)
{
    int ret;
    int msg[16];
    char otg_msg[8] = {0};
    u8 disk_msg = 0;
    u8 is_cdc_background = 0;

#if USB_EPBUF_ALLOC_USE_LBUF
    extern int usb_memory_init(void);
    usb_memory_init();
#endif

#if TCFG_OTG_MODE && !TCFG_VIR_UDISK_ENABLE

    if (resource_multiplex_is_registered(IO_PORT_DM)) {
        otg_data.detect_mode |= OTG_DET_DP_ONLY;
    }


    if (work_in_slave() == 0) {
        usb_otg_init(NULL, &otg_data);
#if ((defined TCFG_PC_MODE_USB_DETECT_DURING_CHARGING)&&(TCFG_PC_MODE_USB_DETECT_DURING_CHARGING))&&(TCFG_APP_PC_EN)
        //插入充电再打开usb检测
        usb_detect_timer_del();
        /* usb_otg_io_suspend(0);//主机生效 */
#endif
    }

#endif

#if TCFG_HOST_AUDIO_ENABLE
    uac_host_init();
#endif

#if TCFG_HOST_UVC_ENABLE
    usb_host_video_init(0, 0);
#endif

#if TCFG_VIR_UDISK_ENABLE
    usb_driver_event_from_otg(DEVICE_EVENT_FROM_OTG, DEVICE_EVENT_IN, "s:0");
#endif

    if (work_in_slave() == 1) {
        os_time_dly(100 * 5);
        usb_driver_event_from_otg(DEVICE_EVENT_FROM_OTG, DEVICE_EVENT_IN, "s:0");
    }

    while (1) {

        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));

        if (ret != OS_TASKQ || msg[0] != Q_MSG) {
            continue;
        }

        switch (msg[1]) {

        case USBSTACK_OTG_MSG:

            handle_otg_msg(msg, otg_msg, &is_cdc_background);
            break;

        case USBSTACK_HOST_MSG:

            handle_host_msg(msg, otg_msg, &disk_msg);
            break;

        case USBSTACK_HOST_REMOUNT:

            handle_host_remount(msg);
            break;

        case USBSTACK_TEST:
            //handle_udisk_test(msg);
            break;

        default:
            handle_slave_msg(msg, &is_cdc_background);
            break;
        }
    }
}
static int usb_stack_init(void)
{
    r_printf("usb_stack_init()");
    int err;
    os_sem_create(&msg_sem, 0);
    err = task_create(usb_task, NULL, USB_TASK_NAME);

    if (err != OS_NO_ERR) {
        r_printf("usb_msd task creat fail %x\n", err);
    }

    return 0;
}
late_initcall(usb_stack_init);

void usb_message_to_stack(int msg, void *arg, u8 sync)
{
    //先将sem清零0，否则当sync为0时不会pend，另一边post会使counter累加
    os_sem_set(&msg_sem, 0);
    os_taskq_post_msg(USB_TASK_NAME, 2, msg, arg);

    if (sync) {
        os_sem_pend(&msg_sem, USB_TASK_SEM_TIMEOUT);
    }
}


#endif
