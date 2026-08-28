#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".uart.bss")
#pragma data_seg(".uart.data")
#pragma const_seg(".uart.const")
#pragma code_seg(".uart.text")
#pragma str_literal_override(".uart.text")
#endif /* #ifdef SUPPORT_MS_EXTENSIONS */

#include "includes.h"
#include "uart.h"
#include "gpio.h"
#include "ipc_spin_lock.h"

#define AT_RAM      AT(.uart.text.cache.L1)

//uart时钟选择:
/* #define UART_CLK_SEL 				UART_CLK_RC16M   //由主系统gpcnt模块算出rc16m频率，rc16m频率跟pvdd有关，注意配置被覆盖。 */
#define UART_CLK_SEL 				UART_CLK_STD_12M //低功耗不能使用该时钟源

//uart波特率选择:
#define UART_TARGET_BAUD    		CONFIG_DEBUG_UART_BAUD
//#define UART_TARGET_BAUD    		1000000L

//调试串口选择: PMC_UART0/P11_UART1
#define DEBUG_UART_SELECT 			PMC_UART0

//输入选择. 只有UART0有输入功能
#define DEBUG_UART_RX_ENABLE 		0
#if DEBUG_UART_RX_ENABLE
#define DEBUG_UART_RX_IO 			IO_PORTB_05
#define DEBUG_UART_RX_OVERTIME 		100 //ms
#endif /* #if DEBUG_UART_RX_ENABLE */

static u8 init = 0;

#define P11_UART_CLK_SEL(x) 		SFR(PMC_CLOCK->PRP_CON0, 0, 3, x)

___interrupt
static void uart0_isr(void)
{
    u8 rx = 0;

    if (UART_RX_PENDING_IS(PMC_UART0)) {
        rx = UART_BUF_READ(PMC_UART0);
        UART_RX_PENDING_CLEAR(PMC_UART0);
        printf("RX pending, rx: 0x%x\n", rx);
    }

    if (UART_OT_PENDING_IS(PMC_UART0)) {
        rx = UART_BUF_READ(PMC_UART0);
        UART_OT_PENDING_CLEAR(PMC_UART0);
        printf("OT pending, rx: 0x%x\n", rx);
    }
}

static u32 uart_src_clk_get(void)
{
    u32 freq = 0;

    if (UART_CLK_SEL == UART_CLK_STD_12M) {
        freq = 12000000;
    } else if (UART_CLK_SEL == UART_CLK_RC16M) {
        freq = 16000000;//(u32)((u32)m2p_message[M2P_RCH_FEQ_H] << 8 | m2p_message[M2P_RCH_FEQ_L]) * 1000;
    } else if (UART_CLK_SEL == UART_CLK_SYS_CLK) {
        freq = 24000000;
    }

    return freq;
}

void uart_clk_sel(enum UART_CLK_TABLE clk)
{
    u32 freq = 0;

    if (clk == UART_CLK_STD_12M) {
        freq = 12000000;
    } else if (clk == UART_CLK_RC16M) {
        freq = 16000000;
    } else if (clk == UART_CLK_SYS_CLK) {
        freq = 24000000;
    }

    P11_UART_CLK_SEL(clk);

    u16 baud_reg = (freq / UART_TARGET_BAUD) / 4 - 1;

    UART_BAUD_SET(DEBUG_UART_SELECT, baud_reg);
}

void debug_uart_init(u8 tx_port)
{
    uart_clk_sel(UART_CLK_SEL);

    //tx io inita
    gpio_set_output_value(tx_port, 1);
    gpio_set_direction(tx_port, 0);

    if ((void *)DEBUG_UART_SELECT == (void *)PMC_UART0) {
        gpio_set_fun_output_port(tx_port, PMC_FO_UART0_TX, 1, 1);
    } else if ((void *)DEBUG_UART_SELECT == (void *)PMC_UART1) {
        gpio_set_fun_output_port(tx_port, PMC_FO_UART1_TX, 1, 1);
    }

#if DEBUG_UART_RX_ENABLE
    u32 ot_value = 0;
    if (DEBUG_UART_SELECT == PMC_UART0) {
        //rx io init:
        gpio_set_direction(DEBUG_UART_RX_IO, 1);
        gpio_set_pull_up(DEBUG_UART_RX_IO, 1);
        gpio_set_pull_down(DEBUG_UART_RX_IO, 0);
        gpio_set_die(DEBUG_UART_RX_IO, 1);
        //iomc:
        gpio_set_fun_input_port(DEBUG_UART_RX_IO, PFI_UART0_RX);

        //rx config:
        UART_OT_PENDING_CLEAR(PMC_UART0);
        UART_RX_PENDING_CLEAR(PMC_UART0);
        ot_value = (uart_src_clk_get() * DEBUG_UART_RX_OVERTIME) / 1000;
        UART_OTCNT_SET(PMC_UART0, ot_value);
        UART_OT_INT_ENABLE(PMC_UART0);
        UART_RX_INT_ENABLE(PMC_UART0);
        UART_RX_ENABLE(PMC_UART0);
        PMC_CLOCK->WKUP_EN |= BIT(IRQ_UART0_IDX);
        request_irq(IRQ_UART0_IDX, 0, uart0_isr, 0);
    }
#endif /* #if DEBUG_UART_RX_ENABLE */

    UART_TX_ENABLE(DEBUG_UART_SELECT);

    init = 1;
}

AT_RAM
void uart_putbyte(char a)
{
    u32 ot = 400;
    while (!UART_TX_PENDING_IS(DEBUG_UART_SELECT)) {
        if (ot-- == 0) {
            break;
        }
    }
    UART_TX_PENDING_CLEAR(DEBUG_UART_SELECT);
    UART_BUF_WRITE(DEBUG_UART_SELECT, a);
}
__WEAK__
AT_RAM
void putchar(char a)
{
    if (init == 0) {
        return;
    }

    if (a == '\r') {
        return;
    }

    if (a == '\n') {
        uart_putbyte('\r');
    }
    uart_putbyte(a);
}

__WEAK__
AT_RAM
void putbyte(char a)
{
    putchar(a);
}

#include "power_interface.h"

static JL_UART_TypeDef UART_POWEROFF;

static u8 uart_enter_deepsleep(void)
{
    UART_POWEROFF.TX_CON0 =  DEBUG_UART_SELECT->TX_CON0;
    UART_POWEROFF.TX_CON1 =  DEBUG_UART_SELECT->TX_CON1;
    UART_POWEROFF.CON2 =  DEBUG_UART_SELECT->CON2;
    UART_POWEROFF.BAUD =  DEBUG_UART_SELECT->BAUD;
    UART_POWEROFF.OTCNT =  DEBUG_UART_SELECT->OTCNT;

    return 0;
}

static u8 uart_exit_deepsleep(void)
{
    DEBUG_UART_SELECT->TX_CON1	 =  UART_POWEROFF.TX_CON1;
    DEBUG_UART_SELECT->BAUD	 =  UART_POWEROFF.BAUD;
    DEBUG_UART_SELECT->OTCNT =  UART_POWEROFF.OTCNT;
    DEBUG_UART_SELECT->CON2	 =  UART_POWEROFF.CON2;

    DEBUG_UART_SELECT->TX_CON0  =  UART_POWEROFF.TX_CON0;
    DEBUG_UART_SELECT->TX_CON0 |= (BIT(13) | BIT(12) | BIT(10));

    return 0;
}


#if CONFIG_UART_DEBUG_ENABLE
DEEPSLEEP_TARGET_REGISTER(uart) = {
    .name   = "uart",
    .enter  = uart_enter_deepsleep,
    .exit   = uart_exit_deepsleep,
};
#endif


/*-----------------------------------------------------------*/
