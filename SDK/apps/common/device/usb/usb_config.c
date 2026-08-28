#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb_config.h"
#include "usb/scsi.h"
#include "irq.h"
#include "init.h"
#include "gpio.h"
#include "timer.h"
#include "app_config.h"
#include "lbuf.h"
#include "usb/device/usb_suspend_resume.h"
#include "usb_task.h"

#ifdef CONFIG_ADAPTER_ENABLE
#include "adapter_usb_hid.h"
#endif//CONFIG_ADAPTER_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_CFG]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define     SET_INTERRUPT   ___interrupt

#ifdef USB_HW_20
#define     MAX_EP_TX   7
#define     MAX_EP_RX   7
#else
#define     MAX_EP_TX   5
#define     MAX_EP_RX   5
#endif
static usb_interrupt usb_interrupt_tx[USB_MAX_HW_NUM][MAX_EP_TX];
static usb_interrupt usb_interrupt_rx[USB_MAX_HW_NUM][MAX_EP_RX];
static usb_sof_ep_hander usb_sof_ep_tx[USB_MAX_HW_NUM][MAX_EP_TX];
static usb_sof_ep_hander usb_sof_ep_rx[USB_MAX_HW_NUM][MAX_EP_RX];

struct usb_config_var_t {
    u8 usb_setup_buffer[USB_SETUP_SIZE];
    struct usb_ep_addr_t usb_ep_addr;
    struct usb_setup_t usb_setup;
};

static struct usb_config_var_t *usb_config_var = {NULL};



static void usb_slave_reset(const usb_dev usb_id)
{
    usb_write_intr_usbe(usb_id, INTRUSB_RESET_BABBLE | INTRUSB_SUSPEND | INTRUSB_RESUME);
    u32 reg = usb_read_power(usb_id);
    usb_write_power(usb_id, (reg | BIT(0)));
}

static volatile u32 g_intr_usb;
static volatile u32 g_intr_tx;
static volatile u32 g_intr_rx;
static volatile u32 g_error_tx;
static volatile u32 g_error_rx;

void usb_irq_handle()
{
    const usb_dev usb_id = 0;
    bit_clr_ie(IRQ_USB_CTRL_IDX);
    u32 intr_usb = g_intr_usb;
    u32 intr_tx = g_intr_tx;
    u32 intr_rx = g_intr_rx;
    g_intr_usb = 0;
    g_intr_rx = 0;
    g_intr_tx = 0;
    g_error_rx = 0;
    g_error_tx = 0;

    bit_set_ie(IRQ_USB_CTRL_IDX);

    //  printf("2-intr_usb %x intr_rx %x intr_tx %x", intr_usb, intr_rx, intr_tx);

    struct usb_device_t *usb_device = usb_id2device(usb_id);

    if (intr_usb & INTRUSB_SUSPEND) {
        log_info("usb suspend");
        os_taskq_post_msg(USB_TASK_NAME, 2, USBSTACK_SLAVE_SUSPEND, usb_id);
    }

    if (intr_usb & INTRUSB_RESET_BABBLE) {
        log_info("usb reset");
        usb_write_intr_usbe(usb_id, INTRUSB_RESET_BABBLE | INTRUSB_SUSPEND);

        usb_reset_host_type(usb_id);
        usb_reset_interface(usb_device);
        usb_slave_reset(usb_id);
    }


    if (intr_tx & BIT(0)) {
        if (usb_interrupt_rx[usb_id][0]) {
            usb_interrupt_rx[usb_id][0](usb_device, 0);
        } else {
            usb_control_transfer(usb_device);
        }
    }

    for (int i = 1; i < MAX_EP_TX; i++) {
        if (intr_tx & BIT(i)) {
            if (usb_interrupt_tx[usb_id][i]) {
                usb_interrupt_tx[usb_id][i](usb_device, i);
            }
        }
    }

    for (int i = 1; i < MAX_EP_RX; i++) {
        if (intr_rx & BIT(i)) {
            if (usb_interrupt_rx[usb_id][i]) {

                usb_interrupt_rx[usb_id][i](usb_device, i);
            }
        }
    }

}
#ifdef USB_SOFT_IRQ
SET_INTERRUPT
void usb_soft_irq()
{
    q32DSP(core_num())->ILAT_CLR = BIT(USB_SOFT_IRQ - IRQ_SOFT0_IDX);
    usb_irq_handle();
}
#endif
USB_RAM_CODE
void usb_isr(const usb_dev usb_id)
{
    u32 intr_usb, intr_usbe;
    u32 intr_tx, intr_txe;
    u32 intr_rx, intr_rxe;

    usb_read_intr(usb_id, &intr_usb, &intr_tx, &intr_rx);
    usb_read_intre(usb_id, &intr_usbe, &intr_txe, &intr_rxe);

    intr_usb &= intr_usbe;
    intr_tx &= intr_txe;
    intr_rx &= intr_rxe;

    //  printf("0-intr_usb %x intr_rx %x intr_tx %x", g_intr_usb, g_intr_rx, g_intr_tx);
//   printf("1-intr_usb %x intr_rx %x intr_tx %x", intr_usb, intr_rx, intr_tx);
    g_intr_usb |= intr_usb;
    g_intr_rx |= intr_rx;
    g_intr_tx |= intr_tx;
    u32 error_rx = 0;
    u32 error_tx = 0;
    for (int i = 1; i < MAX_EP_TX; i++) {
        if (intr_tx & BIT(i)) {
            if (usb_ep_tx_is_unmask_irq(usb_id, i)) {
                int r = usb_g_ep_write(usb_id, i, NULL, -1);
                if (r < 0) {
                    error_tx |= BIT(i);
                }
            }
        }
    }

    for (int i = 1; i < MAX_EP_RX; i++) {
        if (intr_rx & BIT(i)) {
            if (usb_ep_rx_is_unmask_irq(usb_id, i)) {
                int r = usb_g_ep_read(usb_id, i, NULL, -1, 0);
                if (r < 0) {
                    error_rx |= BIT(i);
                }
            }
        }
    }

    g_error_rx |= error_rx;
    g_error_tx |= error_tx;
#ifdef USB_SOFT_IRQ
    /* 两级中断: 硬中断只做数据搬运, 软中断处理协议事件 */
    q32DSP(core_num())->ILAT_SET = BIT(USB_SOFT_IRQ - IRQ_SOFT0_IDX);
#else
    /* 单级中断: 硬中断直接处理全部事件 */
    usb_irq_handle();
#endif
}

void usb_sof_isr(const usb_dev usb_id)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    u32 frame = usb_read_sofframe(usb_id);
    usb_sof_clr_pnd(usb_id);
    usb_slave_status_set(usb_id, USB_SLAVE_SOF_READY);

    for (int i = 0; i < MAX_EP_TX; i++) {
        if (usb_sof_ep_tx[usb_id][i]) {
            usb_sof_ep_tx[usb_id][i](usb_device, i, frame);
        }
    }
    for (int i = 0; i < MAX_EP_RX; i++) {
        if (usb_sof_ep_rx[usb_id][i]) {
            usb_sof_ep_rx[usb_id][i](usb_device, i, frame);
        }
    }
}

SET_INTERRUPT
void usb0_g_isr()
{
//    JL_PORTC->OUT_BSR = (BIT(5));
//    JL_PORTC->DIR_BSR = (BIT(5+16));

    usb_isr(0);
}
SET_INTERRUPT
void usb0_sof_isr()
{
    usb_sof_isr(0);
}

#if USB_MAX_HW_NUM == 2
SET_INTERRUPT
void usb1_g_isr()
{
    usb_isr(1);
}
SET_INTERRUPT
void usb1_sof_isr()
{
    usb_sof_isr(1);
}
#endif
__attribute__((always_inline_when_const_args))
u32 usb_g_set_intr_hander(const usb_dev usb_id, u32 ep, usb_interrupt hander)
{
    if (ep & USB_DIR_IN) {
        usb_interrupt_tx[usb_id][ep & 0xf] = hander;
    } else {
        usb_interrupt_rx[usb_id][ep] = hander;
    }
    return 0;
}
__attribute__((always_inline_when_const_args))
u32 usb_g_set_sof_hander(const usb_dev usb_id, u32 ep, usb_sof_ep_hander hander)
{
    if (ep & USB_DIR_IN) {
        usb_sof_ep_tx[usb_id][ep & 0xf] = hander;
    } else {
        usb_sof_ep_rx[usb_id][ep] = hander;
    }
    return 0;
}
void usb_g_isr_reg(const usb_dev usb_id, u8 priority, u8 cpu_id)
{
    if (usb_id == 0) {
#if defined(USB_SOFT_IRQ)
        request_irq(IRQ_USB_CTRL_IDX, 7, usb0_g_isr, cpu_id);
        irq_unmask_set(IRQ_USB_CTRL_IDX, 7, cpu_id);
        request_irq(USB_SOFT_IRQ, priority, usb_soft_irq, cpu_id);

#else
        request_irq(IRQ_USB_CTRL_IDX, priority, usb0_g_isr, cpu_id);
#endif
    } else {
#if USB_MAX_HW_NUM == 2
        request_irq(IRQ_USB1_CTRL_IDX, priority, usb1_g_isr, cpu_id);
#endif
    }
}
void usb_sof_isr_reg(const usb_dev usb_id, u8 priority, u8 cpu_id)
{
    if (usb_id == 0) {
        request_irq(IRQ_USB_SOF_IDX, priority, usb0_sof_isr, cpu_id);
    } else {
#if USB_MAX_HW_NUM == 2
        request_irq(IRQ_USB1_SOF_IDX, priority, usb1_sof_isr, cpu_id);
#endif
    }
    usb_sofie_enable(usb_id);
}
u32 usb_config(const usb_dev usb_id)
{
    memset(usb_interrupt_rx[usb_id], 0, sizeof(usb_interrupt_rx[usb_id]));
    memset(usb_interrupt_tx[usb_id], 0, sizeof(usb_interrupt_tx[usb_id]));
    memset(usb_sof_ep_rx[usb_id], 0, sizeof(usb_sof_ep_rx[usb_id]));
    memset(usb_sof_ep_tx[usb_id], 0, sizeof(usb_sof_ep_tx[usb_id]));

    assert_d(usb_config_var == NULL);
    usb_config_var = (struct usb_config_var_t *)zalloc(sizeof(struct usb_config_var_t));
    assert_d(usb_config_var != NULL);

    log_debug("zalloc: usb_config_var = %x\n", usb_config_var);
    usb_var_init(usb_id, &(usb_config_var->usb_ep_addr));
    usb_setup_init(usb_id, &(usb_config_var->usb_setup), usb_config_var->usb_setup_buffer);
    usb_slave_suspend_resume_init(usb_id);
    return 0;
}

u32 usb_release(const usb_dev usb_id)
{
    log_debug("free: usb_config_var = %x\n", usb_id, usb_config_var);
    usb_var_release(usb_id);
    usb_setup_release(usb_id);
    usb_slave_suspend_resume_deinit(usb_id);

    if (usb_config_var) {
        free(usb_config_var);
    }

    usb_config_var = NULL;

    return 0;
}
