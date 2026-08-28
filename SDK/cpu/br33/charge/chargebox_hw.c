#include "board_config.h"
#include "includes.h"
#include "app_config.h"
#include "chargebox.h"
#include "clock.h"
#include "uart.h"
#include "asm/gpio_hw.h"

#if (TCFG_CHARGE_BOX_ENABLE)

#define BUF_LEN     (64)

struct chargebox_handle {
    const struct chargebox_platform_data *data;
    s32 udev;
    volatile u8 lr_flag;
    volatile u8 rx_index;
};

static u8 chg_uart_buf[BUF_LEN] __attribute__((aligned(4)));
static u8 chg_uart_dma_buf[BUF_LEN] __attribute__((aligned(4)));

#define __this  (&hdl)
static struct chargebox_handle hdl;

extern void chargebox_data_deal(u8 cmd, u8 l_r, u8 *data, u8 length);
void __attribute__((weak)) chargebox_data_deal(u8 cmd, u8 l_r, u8 *data, u8 len)
{

}

static void chargebox_uart_isr_cb(uart_dev uart_num, enum uart_event event)
{
    if (event == UART_EVENT_TX_DONE) {
        chargebox_data_deal(CMD_COMPLETE, __this->lr_flag, NULL, 0);
    } else if (event == UART_EVENT_RX_DATA) {
        uart_recv_bytes(__this->udev, &chg_uart_buf[__this->rx_index], 1);
        chargebox_data_deal(CMD_RECVBYTE, __this->lr_flag, NULL, 0);
        if (chg_uart_buf[0] == 0x55) {
            __this->rx_index++;
        } else {
            return;
        }
        //耳机发来数据长度为head 2byte + (data len +1) + 校验码 1byte
        if ((__this->rx_index > 3) && (__this->rx_index == (chg_uart_buf[2] + 4))) {
            chargebox_data_deal(CMD_RECVDATA, __this->lr_flag, chg_uart_buf, __this->rx_index);
            __this->rx_index = 0;
        }
    }
}

u8 chargebox_write(u8 l_r, u8 *data, u8 len)
{
    if ((len == 0) || (len > BUF_LEN) || (__this->udev < 0)) {
        return 0;
    }
    __this->lr_flag = l_r;

    if (((u32)data) % 4) {//4byte对齐
        ASSERT(0, "%s: unaligned accesses!", __func__);
    }
    memcpy(chg_uart_buf, data, len);
    uart_send_bytes(__this->udev, chg_uart_buf, (u32)len);
    return len;
}

void chargebox_open(u8 l_r, u8 mode)
{
    u32 uart_timeout;
    u32 io_port;
    s32 ret;
    struct uart_config config = {0};
    struct uart_dma_config dma = {0};

    if (mode == MODE_RECVDATA) {
        __this->rx_index = 0;
    }

    if (__this->udev >= 0) {
        return;
    }

    if (l_r ==  EAR_L) {
        io_port = __this->data->L_port;
    } else {
        io_port = __this->data->R_port;
    }
    __this->lr_flag = l_r;

    config.baud_rate = __this->data->baudrate;
    config.tx_pin = io_port;
    config.rx_pin = io_port;
    config.parity = UART_PARITY_DISABLE;
    config.tx_wait_mutex = 0;
    __this->udev = uart_init(-1, &config);
    if (__this->udev < 0) {
        goto __err_exit;
    }

    //确保2byte的时间
    uart_timeout = 25 * 1000000 / __this->data->baudrate;//us
    dma.rx_timeout_thresh = uart_timeout;
    dma.frame_size = 1;
    dma.event_mask = UART_EVENT_RX_DATA | UART_EVENT_RX_TIMEOUT | UART_EVENT_TX_DONE;
    dma.irq_priority = 2;
    dma.irq_callback = chargebox_uart_isr_cb;
    dma.rx_cbuffer = chg_uart_dma_buf;
    dma.rx_cbuffer_size = BUF_LEN;
    ret = uart_dma_init(__this->udev, &dma);
    if (ret < 0) {
        goto __err_exit;
    }
    return;

__err_exit:
    ASSERT(0, "chargebox open err!\n");
    if (__this->udev >= 0) {
        uart_deinit(__this->udev);
    }
    __this->udev = -1;
}

void chargebox_close(u8 l_r)
{
    if (__this->udev < 0) {
        return;
    }
    uart_deinit(__this->udev);
    __this->udev = -1;

    if (l_r == EAR_L) {
        gpio_hw_set_pull_up(IO_PORT_SPILT(__this->data->L_port), GPIO_PULLUP_DISABLE);
        gpio_hw_set_pull_down(IO_PORT_SPILT(__this->data->L_port), GPIO_PULLDOWN_DISABLE);
        gpio_hw_set_die(IO_PORT_SPILT(__this->data->L_port), 1);
        gpio_hw_set_dieh(IO_PORT_SPILT(__this->data->L_port), 1);
        gpio_set_mode(IO_PORT_SPILT(__this->data->L_port), PORT_OUTPUT_HIGH);
    } else {
        gpio_hw_set_pull_up(IO_PORT_SPILT(__this->data->R_port), GPIO_PULLUP_DISABLE);
        gpio_hw_set_pull_down(IO_PORT_SPILT(__this->data->R_port), GPIO_PULLDOWN_DISABLE);
        gpio_hw_set_die(IO_PORT_SPILT(__this->data->R_port), 1);
        gpio_hw_set_dieh(IO_PORT_SPILT(__this->data->R_port), 1);
        gpio_set_mode(IO_PORT_SPILT(__this->data->R_port), PORT_OUTPUT_HIGH);
    }
}

void chargebox_set_baud(u32 baudrate)
{
    u32 uart_timeout;
    if (__this->udev < 0) {
        return;
    }
    //确保2byte的时间
    uart_timeout = 25 * 1000000 / baudrate;
    uart_set_baudrate(__this->udev, baudrate);
    uart_set_rx_timeout_thresh(__this->udev, uart_timeout);
    //重新初始化DMA,防止高波特率时候出现分包行为
    uart_dma_rx_reset(__this->udev);
}

void chargebox_init(const struct chargebox_platform_data *data)
{
    __this->data = (struct chargebox_platform_data *)data;
    ASSERT(data);
    __this->udev = -1;
}

#endif

