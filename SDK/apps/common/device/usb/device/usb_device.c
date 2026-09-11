#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb/device/usb_stack.h"
#include "usb/device/descriptor.h"
#include "usb_config.h"
#include "irq.h"
#include "init.h"
#include "gpio.h"
#include "app_config.h"
#include "clock_manager/clock_manager.h"
#include "clock.h"
#include "usb_multiplex.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_DEV]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/* Linker-section-based USB class driver table */
extern const usb_class_item_t __usb_class_start[];
extern const usb_class_item_t __usb_class_end[];

/* Bubble-sort section items by interface_index */
static void usb_class_sort(const usb_class_item_t **items, u8 count)
{
    for (u8 i = 0; i + 1 < count; i++) {
        for (u8 j = 0; j + 1 + i < count; j++) {
            if (items[j]->interface_index > items[j + 1]->interface_index) {
                const usb_class_item_t *tmp = items[j];
                items[j] = items[j + 1];
                items[j + 1] = tmp;
            }
        }
    }
}

/* Build sorted pointer array, returns count */
static u8 usb_class_collect(const usb_class_item_t **out, u8 max)
{
    u8 count = 0;
    for (const usb_class_item_t *p = __usb_class_start;
         p < __usb_class_end && count < max; p++) {
        out[count++] = p;
    }
    usb_class_sort(out, count);
    return count;
}

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


int usb_device_mode(const usb_dev usb_id, const u32 class)
{
    /* Collect and sort all registered USB class drivers */
    const usb_class_item_t *items[MAX_INTERFACE_NUM + 1];
    u8 item_count = usb_class_collect(items, MAX_INTERFACE_NUM + 1);

    if (class == 0) {
        if (usb_otg_online(usb_id) == OTG_STATE_SLAVE_MODE) {
            usb_otg_suspend(usb_id,  OTG_KEEP_STATE);
        }
        //先关闭设备，避免后面release的时候usb还在dma改写内存
        usb_g_hold(usb_id);

        /* Release: traverse in reverse index order */
        for (s8 i = item_count - 1; i >= 0; i--) {
            items[i]->release_fn((usb_dev)usb_id);
        }
        usb_auto_trim_deinit();
        usb_release(usb_id);

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

#if TCFG_USB_TEST_ENABLE
    u32 _class = TEST_CLASS;
#else
    u32 _class = class;
#endif

    usb_add_desc_config(usb_id, MAX_INTERFACE_NUM, NULL);
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    /* Set wDeviceClass early so register_fn can read it (Audio dispatch needs this) */
    usb_device_set_class(usb_device, _class);

    u8 class_index = 0;

    /* Register: traverse in index order, filter by class_mask */
    for (u8 i = 0; i < item_count; i++) {
        const usb_class_item_t *item = items[i];

        /* Match mode: 0 = exact mask match, 1 = any bit overlap match */
        if (item->match_mode == 0) {
            if ((_class & item->class_mask) != item->class_mask) {
                continue;
            }
        } else {
            if ((_class & item->class_mask) == 0) {
                continue;
            }
        }

        usb_add_desc_config(usb_id, class_index++, item->desc);
        item->register_fn((usb_dev)usb_id);
    }

    usb_device_init(usb_id);

    set_descriptor(usb_id,
                   _class,
                   usb_device->setup_buffer,
                   USB_SETUP_SIZE);


    user_setup_filter_install(usb_device);
    usb_otg_resume(usb_id);
    usb_auto_trim_init(1);
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
