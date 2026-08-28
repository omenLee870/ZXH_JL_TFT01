#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb/device/usb_stack.h"
#include "usb_config.h"
#include "usb/device/msd.h"
#include "usb/device/mtp.h"
#include "usb/scsi.h"
#include "usb/device/hid.h"
#include "usb/device/custom_hid.h"
#include "usb/device/uac_audio.h"
#include "usb/device/cdc.h"
#include "usb/device/iap.h"
#include "usb/device/midi.h"
#include "usb/device/printer.h"
#include "usb/device/webusb.h"
#include "usb/test_usb.h"
#include "usb/device/usb_test_device.h"
#include "irq.h"
#include "init.h"
#include "gpio.h"
#include "app_config.h"
#include "clock_manager/clock_manager.h"
#include "usb_multiplex.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE

#include "debug.h"

#if TCFG_USB_SLAVE_ENABLE
static void usb_device_init(const usb_dev usb_id)
{
    u32 speed = USB_SPEED_FULL;
#if defined(FUSB_MODE) && FUSB_MODE
    speed = USB_SPEED_FULL;
#elif defined(FUSB_MODE) && (FUSB_MODE == 0)
    speed = USB_SPEED_HIGH;
#endif
    usb_g_sie_init(usb_id);
    usb_slave_init(usb_id, speed);
    u8 *ep0_dma_buffer = usb_alloc_ep_dmabuffer(usb_id, 0, 64);

    usb_set_dma_raddr(usb_id, 0, ep0_dma_buffer);
    usb_set_dma_raddr(usb_id, 1, ep0_dma_buffer);
    usb_set_dma_raddr(usb_id, 2, ep0_dma_buffer);
    usb_set_dma_raddr(usb_id, 3, ep0_dma_buffer);
    usb_set_dma_raddr(usb_id, 4, ep0_dma_buffer);

    usb_write_intr_usbe(usb_id, INTRUSB_RESET_BABBLE);
    usb_clr_intr_txe(usb_id, -1);
    usb_clr_intr_rxe(usb_id, -1);
    usb_set_intr_txe(usb_id, 0);
    usb_set_intr_rxe(usb_id, 0);
    const u32 usb_irq_priority = 1;
    usb_g_isr_reg(usb_id, usb_irq_priority, 0);
    usb_sof_isr_reg(usb_id, usb_irq_priority, 0);
    /* usb_sofie_enable(usb_id); */
    /* USB_DEBUG_PRINTF("ep0 addr %x %x\n", usb_get_dma_taddr(0), ep0_dma_buffer); */
}
static void usb_device_hold(const usb_dev usb_id)
{
    if (usb_otg_online(usb_id) == OTG_STATE_SLAVE_MODE) {
        usb_otg_suspend(usb_id,  OTG_KEEP_STATE);
    }

    usb_g_hold(usb_id);
    usb_release(usb_id);
}


int usb_device_mode(const usb_dev usb_id, const u32 class)
{

    //cppcheck-suppress unreadVariable
    u8 class_index = 0;
    if (class == 0) {
        usb_device_hold(usb_id);

#if TCFG_USB_SLAVE_MSD_ENABLE
        msd_release(usb_id);
#endif

#if TCFG_USB_APPLE_DOCK_EN
        iap_release(usb_id);
#endif

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        uac_spk_release(usb_id);
#endif

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
        uac_mic_release(usb_id);
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
        cdc_release(usb_id);
#endif

#if TCFG_USB_SLAVE_HID_ENABLE
        hid_release(usb_id);
#endif

#if TCFG_USB_SLAVE_MTP_ENABLE
        mtp_release(usb_id);
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
        custom_hid_release(usb_id);
#endif

#if TCFG_USB_SLAVE_MIDI_ENABLE
        midi_release(usb_id);
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
        printer_release(usb_id);
#endif
#if TCFG_USB_WEBUSB_ENABLE
        webusb_release(usb_id);
#endif

#if TCFG_USB_TEST_ENABLE
        test_release(usb_id);
#endif

#ifdef USB_HW_20
        clock_unlock("usb_device");
#endif
        return 0;
    }

    usb_config(usb_id);

#ifdef USB_HW_20
    clock_lock("usb_device", clk_get_max_frequency());
#endif

    pc_multiplex_close_other_device();
    pc_multiplex_resume();

    u32 _class = class;
#if TCFG_USB_TEST_ENABLE
    _class = TEST_CLASS;
#endif

    usb_add_desc_config(usb_id, MAX_INTERFACE_NUM, NULL);

#if TCFG_USB_SLAVE_MSD_ENABLE
    if ((_class & MASSSTORAGE_CLASS) == MASSSTORAGE_CLASS) {
        log_info("add desc msd");
        usb_add_desc_config(usb_id, class_index++, msd_desc_config);
        msd_register(usb_id);
    }
#endif

#if TCFG_USB_APPLE_DOCK_EN
    if ((_class & IAP_CLASS) == IAP_CLASS) {
        log_info("add desc iap");
        usb_add_desc_config(usb_id, class_index++, iap_desc_config);
        iap_register(usb_id);
    }
#endif

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE || TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    if ((_class & AUDIO_CLASS) == AUDIO_CLASS) {
        log_info("add audio desc");
        usb_add_desc_config(usb_id, class_index++, uac_audio_desc_config);
        uac_spk_register(usb_id);
        uac_mic_register(usb_id);
    } else if ((_class & SPEAKER_CLASS) == SPEAKER_CLASS) {
        log_info("add desc speaker");
        usb_add_desc_config(usb_id, class_index++, uac_spk_desc_config);
        uac_spk_register(usb_id);
    } else if ((_class & MIC_CLASS) == MIC_CLASS) {
        log_info("add desc mic");
        usb_add_desc_config(usb_id, class_index++, uac_mic_desc_config);
        uac_mic_register(usb_id);
    }
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
    if ((_class & CUSTOM_HID_CLASS) == CUSTOM_HID_CLASS) {
        log_info("add desc std custom_hid");
        custom_hid_register(usb_id);
        usb_add_desc_config(usb_id, class_index++, custom_hid_desc_config);
    }
#endif

#if TCFG_USB_SLAVE_HID_ENABLE
    if ((_class & HID_CLASS) == HID_CLASS) {
        log_info("add desc std hid");
        usb_add_desc_config(usb_id, class_index++, hid_desc_config);
        hid_register(usb_id);
        //dongle 1t2设置两条usb通路
#if CONFIG_APP_DONGLE && (CONFIG_BT_GATT_CLIENT_NUM == 2)
        hid_register_second(usb_id);
        usb_add_desc_config(usb_id, class_index++, hid_second_desc_config);

#endif
    }
#endif

#if TCFG_USB_SLAVE_MTP_ENABLE
    if ((_class & MTP_CLASS) == MTP_CLASS) {
        log_info("add desc std mtp");
        usb_add_desc_config(usb_id, class_index++, mtp_desc_config);
        mtp_register(usb_id);
    }
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
    if ((_class & CDC_CLASS) == CDC_CLASS) {
        log_info("add desc cdc");
        usb_add_desc_config(usb_id, class_index++, cdc_desc_config);
        cdc_register(usb_id);
    }
#endif

#if TCFG_USB_SLAVE_MIDI_ENABLE
    if (_class & MIDI_CLASS) {
        log_info("add desc midi");
        usb_add_desc_config(usb_id, class_index++, midi_desc_config);
        midi_register(usb_id);
    }
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
    if ((_class & PRINTER_CLASS) == PRINTER_CLASS) {
        log_info("add desc printer");
        usb_add_desc_config(usb_id, class_index++, printer_desc_config);
        printer_register(usb_id);
    }
#endif
#if TCFG_USB_WEBUSB_ENABLE
    if ((_class & WEBUSB_CLASS) == WEBUSB_CLASS) {
        log_info("add desc webusb");
        usb_add_desc_config(usb_id, class_index++, webusb_desc_config);
        webusb_register(usb_id);
    }
#endif

#if TCFG_USB_TEST_ENABLE
    if ((_class & TEST_CLASS) == TEST_CLASS) {
        log_info("add desc test");
        usb_add_desc_config(usb_id, class_index++, test_desc_config);
        test_register(usb_id);
    }
#endif

    usb_device_init(usb_id);
    usb_device_set_class(usb_id2device(usb_id), _class);
    user_setup_filter_install(usb_id2device(usb_id));

    usb_otg_resume(usb_id); //从机初始化完成，进入sof检查模式
    return 0;
}

#endif

/*
 * @brief otg检测中sof初始化，不要放在TCFG_USB_SLAVE_ENABLE里
 * @parm id usb设备号
 * @return 0 ,忽略sof检查，1 等待sof信号
 */
u32 usb_otg_sof_check_init(const usb_dev id)
{
#ifdef USBCV_TEST
    void otg_usbif_test_mode(const usb_dev usb_id);
    otg_usbif_test_mode(id);
    return 0;
#endif
    u8 *ep0_dma_buffer = usb_alloc_ep_dmabuffer(id, 0, 64);

    usb_g_sie_init(id);

#ifdef USB_HW_20
    //husb调用完usb_g_sie_init()，或者收到usb reset中断，intrusbe，intrtxe，
    //intrrxe会复位成默认值，不手动清零以及关中断，会收到usb reset中断，由于
    //空指针问题导致异常
    usb_write_intr_usbe(id, 0);
    usb_clr_intr_txe(id, -1);
    usb_clr_intr_rxe(id, -1);
    if (id == 0) {
        unrequest_irq(IRQ_USB_CTRL_IDX);
#if USB_MAX_HW_NUM > 1
    } else {
        unrequest_irq(IRQ_USB1_CTRL_IDX);
#endif
    }
#endif

#if defined(FUSB_MODE) && FUSB_MODE
    usb_write_power(id, 0x40);
#elif defined(FUSB_MODE) && (FUSB_MODE == 0)
    usb_write_power(id, 0x60);
#endif

    usb_set_dma_raddr(id, 0, ep0_dma_buffer);

    for (int ep = 1; ep < USB_MAX_HW_EPNUM; ep++) {
        usb_disable_ep(id, ep);
    }
    usb_sof_clr_pnd(id);
    return 1;
}
