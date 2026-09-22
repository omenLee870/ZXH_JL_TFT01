/**
 * @file task_pc.c
 * @brief 从机模式
 * @author chenrixin@zh-jieli.com
 * @version 1.0.0
 * @date 2020-02-29
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "os/os_api.h"
#include "asm/sdmmc.h"

#include "app_charge.h"
#include "asm/charge.h"
#include "app_main.h"

#if TCFG_USB_SLAVE_ENABLE
#if  USB_PC_NO_APP_MODE == 0
#include "app_task.h"
#endif
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "usb/usb_task.h"
#include "utils/resource_multiplex.h"
#include "usb_multiplex.h"

#if TCFG_USB_SLAVE_HID_ENABLE
#include "usb/device/hid.h"
#endif

#if TCFG_USB_SLAVE_MTP_ENABLE
#include "usb/device/mtp.h"
#endif

#if TCFG_USB_SLAVE_MSD_ENABLE
#include "usb/device/msd.h"
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
#include "usb/device/cdc.h"
#endif

#if TCFG_USB_APPLE_DOCK_EN
#include "usb/device/iap.h"
#endif

#if TCFG_CFG_TOOL_ENABLE
#include "cfg_tool_cdc.h"
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
#include "usb/device/custom_hid.h"
#endif

#if TCFG_USB_SLAVE_MIDI_ENABLE
#include "usb/device/midi.h"
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
#include "usb/device/printer.h"
#endif

#if ((TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE) \
     && TCFG_CHARGESTORE_PORT == IO_PORT_DP)
#include "chargestore/chargestore.h"
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


extern u8 msd_in_task;
extern u8 msd_run_reset;


#if TCFG_USB_SLAVE_MSD_ENABLE
static void usb_msd_wakeup(struct usb_device_t *usb_device)
{
    os_taskq_post_msg(USB_TASK_NAME, 2, USBSTACK_MSD_RUN, usb_device);
}
static void usb_msd_reset_wakeup(struct usb_device_t *usb_device, u32 itf_num)
{
    if (msd_in_task) {
        msd_run_reset = 1;
    } else {
        msd_reset(usb_device, 0);
    }
}
#endif

#if TCFG_USB_APPLE_DOCK_EN
static void usb_iap_wakeup(struct usb_device_t *usb_device)
{
    os_taskq_post_msg(USB_TASK_NAME, 2, USBSTACK_IAP_RUN, usb_device);
}
#endif

#if TCFG_USB_SLAVE_MTP_ENABLE
static void usb_mtp_wakeup(struct usb_device_t *usb_device)
{
    os_taskq_post_msg(USB_TASK_NAME, 2, USBSTACK_MTP_RUN, usb_device);
}
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
static void usb_cdc_wakeup(struct usb_device_t *usb_device, u32 num)
{
    //回调函数在中断里，正式使用不要在这里加太多东西阻塞中断，
    //或者先post到任务，由任务调用cdc_read_data()读取再执行后续工作
    // int msg[2];
    // msg[0] = usb_device2id(usb_device);;
    // msg[1] = num;
    // usb_message_to_stack(USBSTACK_CDC_DATA, msg, 0);

#if TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_USB_COMM)
    int msg[2] = {0};
    msg[0] = (u32)usb_device;
    msg[1] = num;
    if (OS_NO_ERR != os_taskq_post_type("app_core", MSG_FROM_CDC_DATA, 2, msg)) {
        log_warn("cdc_rx post error\n");
    }
#endif

}
static int cdc_rx_data(int *msg)
{
    /* log_debug("msg[0]:0x%x\n", (u32)msg[0]); */
    const struct usb_device_t *usb_device = (struct usb_device_t *)msg[0];
    const usb_dev usb_id = usb_device2id(usb_device);
    const u32 num = (u32)msg[1];
    u8 buf[64] = {0};
    u32 rlen;

    rlen = cdc_read_data(usb_id, buf, 64, num);
    /* log_debug("cdc rx data"); */
    /* put_buf(buf, rlen);//固件三部测试使用 */
    /* cdc_write_data(usb_id, buf, rlen, num);//固件三部测试使用 */

#if TCFG_CFG_TOOL_ENABLE
    cfg_tool_data_from_cdc(buf, rlen, num);
#endif
    return rlen;
}

APP_MSG_HANDLER(cdc_data_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_CDC_DATA,
    .handler    = cdc_rx_data,
};
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
static void custom_hid_rx_handler(void *priv, u8 *buf, u32 len)
{
    printf("custom_hid_rx_handler,%d,\n", __LINE__);
    put_buf(buf, len);
    //custom_hid_tx_data(0, buf, len);
}
#endif /* #if TCFG_USB_CUSTOM_HID_ENABLE */

#if TCFG_USB_SLAVE_MIDI_ENABLE
int midi_rx_buffer_read(const usb_dev usb_id)
{
    u8 buf[64];
    u32 rlen;

    rlen = midi_rx_data(usb_id, buf);
    log_debug("midi_rx_buffer_read() %d\n", usb_id);
    log_debug_hexdump((u8 *)buf, rlen);

    /* echo received data back to host */
    if (rlen > 0) {
        midi_tx_data(usb_id, buf, rlen);
    }

    return 0;
}
void midi_rx_handler(const usb_dev usb_id)
{
    //回调函数在中断中被调用，因此最好post到线程里面再读数据
    int msg[4];
    msg[0] = (int)midi_rx_buffer_read;
    msg[1] = 1;
    msg[2] = (int)usb_id;
    os_taskq_post_type(USB_TASK_NAME, Q_CALLBACK, 3, msg);
}
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
static int printer_rx_read(const usb_dev usb_id)
{
    u8 buf[64] = {0};
    u32 rlen;

    rlen = usb_g_bulk_read(usb_id, PTR_BULK_EP_OUT, buf, sizeof(buf), 0);
    log_debug("printer_rx_read(), recv_len = %d\n", rlen);
    log_debug_hexdump((u8 *)buf, rlen);

    //usb_g_bulk_write(usb_id, PTR_BULK_EP_IN, buf, rlen);
    return 0;
}
static void printer_rx_handler(struct usb_device_t *usb_device)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    int msg[4];

    msg[0] = (int)printer_rx_read;
    msg[1] = 1;
    msg[2] = (int)usb_id;
    os_taskq_post_type(USB_TASK_NAME, Q_CALLBACK, 3, msg);
}
#endif

void usb_start(const usb_dev usbfd)
{
    log_info("enter usb slave mode");
    u32 class = 0;

#if TCFG_USB_SLAVE_MSD_ENABLE
    class |= MASSSTORAGE_CLASS;
#endif
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    class |= SPEAKER_CLASS;
#endif
#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    class |= MIC_CLASS;
#endif
#if TCFG_USB_SLAVE_HID_ENABLE
    class |= HID_CLASS;
#endif
#if TCFG_USB_SLAVE_CDC_ENABLE
    class |= CDC_CLASS;
#endif
#if TCFG_USB_CUSTOM_HID_ENABLE
    class |= CUSTOM_HID_CLASS;
#endif
#if TCFG_USB_SLAVE_MTP_ENABLE
    class |= MTP_CLASS;
#endif
#if TCFG_USB_SLAVE_MIDI_ENABLE
    class |= MIDI_CLASS;
#endif
#if TCFG_USB_SLAVE_PRINTER_ENABLE
    class |= PRINTER_CLASS;
#endif
#if TCFG_USB_APPLE_DOCK_EN
    class |= IAP_CLASS;
#endif
#if TCFG_USB_WEBUSB_ENABLE
    class |= WEBUSB_CLASS;
#endif
#if TCFG_USB_SLAVE_HCI_ENABLE
    class |= HCI_CLASS;
#endif
    /* Xbox360 手柄（XInput）与其配套的标准键鼠 HID。
     * 注意：xbox360 独占 EP1~EP4、hid_kbmouse 占 EP3，与 MSD(EP1)/HID(EP2)/UAC(EP3)
     * 的端点硬冲突，必须互斥使能；同时开启会导致端点被覆盖、描述符与端点不符。 */
#if TCFG_USB_SLAVE_XBOX360_ENABLE
    class |= XBOX360_CLASS;
#endif
#if TCFG_USB_SLAVE_HID_KBMOUSE_ENABLE
    class |= HID_KBMOUSE_CLASS;
#endif
    g_printf("USB_DEVICE_CLASS_CONFIG:%x", class);
    usb_device_mode(usbfd, class);


#if TCFG_USB_SLAVE_MSD_ENABLE
#if TCFG_SD0_ENABLE
    //复用DM的时候不注册sd卡
    if (resource_multiplex_is_registered(IO_PORT_DM) == 0) {
        msd_register_disk("sd0", NULL);
    }
#endif

#if TCFG_SD1_ENABLE
    msd_register_disk("sd1", NULL);
#endif

#if TCFG_NOR_FAT
    msd_register_disk("fat_nor", NULL);
#endif

#if TCFG_NANDFLASH_DEV_ENABLE
    msd_register_disk("nandflash_ftl", NULL);
#endif

#if TCFG_VIR_UDISK_ENABLE
    msd_register_disk("vir_udisk0", NULL);
#endif

    msd_set_wakeup_handle(usb_msd_wakeup);
    msd_set_reset_wakeup_handle(usb_msd_reset_wakeup);
#endif

#if TCFG_USB_APPLE_DOCK_EN
    iap_set_wakeup_handler(usb_iap_wakeup);
#endif

#if TCFG_USB_SLAVE_MTP_ENABLE
#if (!TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_SD0_ENABLE)\
     ||(TCFG_SD0_ENABLE && TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_DM_MULTIPLEX_WITH_SD_PORT != 0)
    mtp_register_disk("sd0", NULL);
#endif
    mtp_set_wakeup_handle(usb_mtp_wakeup);
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
    cdc_set_wakeup_handler(0, usb_cdc_wakeup);
    cdc_set_wakeup_handler(1, usb_cdc_wakeup);
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
    custom_hid_set_rx_hook(NULL, custom_hid_rx_handler);
#endif

#if TCFG_USB_SLAVE_MIDI_ENABLE
    midi_register_rx_notify(midi_rx_handler);
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
    printer_set_wakeup_handle(printer_rx_handler);
#endif
}

void usb_pause(const usb_dev usbfd)
{
    log_info("usb pause");

    usb_sie_disable(usbfd);

#if TCFG_USB_SLAVE_MSD_ENABLE
    if (msd_set_wakeup_handle(NULL)) {
        msd_unregister_all();
    }
#endif
#if TCFG_USB_SLAVE_MTP_ENABLE
    if (mtp_set_wakeup_handle(NULL)) {
        mtp_unregister_all();
    }
#endif


    usb_device_mode(usbfd, 0);
}

void usb_stop(const usb_dev usbfd)
{

    log_info("usb stop");

    usb_pause(usbfd);

    usb_sie_close(usbfd);

    //拔出的时候才释放，pause不释放
    pc_multiplex_suspend();
    pc_multiplex_open_other_device();
}

int usb_standby(const usb_dev usbfd)
{
    usb_pause(usbfd);
    return 1;
}

#if TCFG_USB_CDC_BACKGROUND_RUN
void usb_cdc_background_run(const usb_dev usbfd)
{
    g_printf("CDC is running in the background");
    usb_device_mode(usbfd, CDC_CLASS);
    cdc_set_wakeup_handler(0, usb_cdc_wakeup);
    cdc_set_wakeup_handler(1, usb_cdc_wakeup);
}

int usb_cdc_background_standby(const usb_dev usbfd)
{
    int ret = 0;
#if TCFG_PC_ENABLE
#if TWFG_APP_POWERON_IGNORE_DEV
    if ((jiffies_to_msecs(jiffies) < TWFG_APP_POWERON_IGNORE_DEV)\
        && (!app_in_mode(APP_MODE_IDLE))) {
        usb_cdc_background_run(usbfd);
    } else
#endif
    {
        usb_pause(usbfd);
        ret = 1;
    }
#else
    usb_cdc_background_run(usbfd);
#endif
    return ret;
}
#endif

static int handle_usb_in(int usb_id)
{
    int switch_app_case = 0;

    log_info("usb %d online", usb_id);

    if (!resource_multiplex_is_registered(IO_PORT_DM)) {
        return 1;
    }

    usb_otg_suspend(usb_id, OTG_KEEP_STATE);

#if TCFG_USB_CDC_BACKGROUND_RUN && TCFG_PC_ENABLE

#if TWFG_APP_POWERON_IGNORE_DEV
    if (jiffies_to_msecs(jiffies) < TWFG_APP_POWERON_IGNORE_DEV) {
        usb_message_to_stack(USBSTACK_CDC_BACKGROUND, (void *)usb_id, 1);
    } else
#endif
    {
        switch_app_case = 1;
    }

#elif TCFG_USB_CDC_BACKGROUND_RUN

    usb_message_to_stack(USBSTACK_CDC_BACKGROUND, (void *)usb_id, 1);

#elif TCFG_PC_ENABLE

    switch_app_case = 1;

#endif

    return switch_app_case;
}


static int handle_usb_out(int usb_id)
{
    int switch_app_case = 0;

    log_info("usb %d offline", usb_id);

    if (app_in_mode(APP_MODE_PC)) {
        switch_app_case = 2;
    } else {
        usb_message_to_stack(USBSTACK_STOP, 0, 1);
    }

    return switch_app_case;
}


int pc_device_event_handler(int *msg)
{
    if (msg[0] != DEVICE_EVENT_FROM_OTG) {
        return false;
    }

    const char *usb_msg = (const char *)msg[2];

    if (usb_msg[0] != 's') {
        return false;
    }

    int usb_id = usb_msg[2] - '0';

    log_info("usb event : %d DEVICE_EVENT_FROM_OTG %s", msg[1], usb_msg);

    switch (msg[1]) {

    case DEVICE_EVENT_IN:
        return handle_usb_in(usb_id);

    case DEVICE_EVENT_OUT:
        return handle_usb_out(usb_id);

    default:
        break;
    }

    return 0;
}

#if ((TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE) \
     && TCFG_CHARGESTORE_PORT == IO_PORT_DP)
static void otg_sof_check_before_hook()
{
    log_debug("sof_check_before");
    p33_io_wakeup_enable(TCFG_CHARGESTORE_PORT, 0);
    chargestore_api_stop();
}

static void otg_sof_check_after_hook()
{
    log_debug("sof_check_after");
    chargestore_api_restart();
    p33_io_wakeup_enable(TCFG_CHARGESTORE_PORT, 1);
}

static int otg_sof_check_hook_register()
{
    //需要在otg定时器启动之前（即devices_init()之前）注册钩子函数
    usb_otg_sof_check_register_hooks(otg_sof_check_before_hook,
                                     otg_sof_check_after_hook);
    return 0;
}
early_initcall(otg_sof_check_hook_register);
#endif

#endif
