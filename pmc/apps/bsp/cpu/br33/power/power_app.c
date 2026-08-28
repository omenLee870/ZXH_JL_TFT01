#include "power_interface.h"
#include "main.h"

/* ------------------------------------------------------------------------------------*/
/**
 * @brief io_map_to_gpio imap 转成标准io口
 *
 * @param imap
 *
 * @return -1 imap非法
 *			other io口
 */
/* ------------------------------------------------------------------------------------*/
static int io_map_to_gpio(u32 imap)
{
#ifndef CONFIG_BR33_FPGA_VERIFY
#define GROUP	16
    if (imap >= PA0_IN && imap < PB0_IN) {
        return imap + (0 * GROUP - PA0_IN);
    } else if (imap >= PB0_IN && imap < PC0_IN) {//IO_PORTB_XX
        return imap + (1 * GROUP - PB0_IN);
    } else if (imap >= PC0_IN && imap < USBDP_IN) {
        return imap + (2 * GROUP - PC0_IN);
    } else if (imap == USBDP_IN) {
        return 14 * GROUP;
    } else if (imap == USBDM_IN) {
        return 14 * GROUP + 1;
    } else if (imap == PP0_IN) {
        return 13 * GROUP;
    }
    return -1;
#else
    return -1;
#endif
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief io_wkup_callback io唤醒回调(中断)
 *
 * @param imap
 * @param edge
 *
 * @return 	0 唤醒主系统处理
 * 			1 P11处理，不唤醒主系统
 */
/* ------------------------------------------------------------------------------------*/
static u32 io_wkup_callback(u32 imap, P33_IO_WKUP_EDGE edge)
{
    /* printf("imap: %d, %d\n", imap, edge); */
#if CONFIG_SENSOR_SLEEP_ENABLE
    if (imap == PB3_IN) {
        task_post_msg(NULL, 1, MSG_P11_SENSOR_IRQ);
        return 1;
    }
#endif
    return 0;
}

void power_early_flowing()
{
    power_early_init(0);

}

int power_later_flowing()
{
    power_later_init(0);

    return 0;
}
