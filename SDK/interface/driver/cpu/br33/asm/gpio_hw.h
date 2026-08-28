#ifndef  __GPIO_HW_H__
#define  __GPIO_HW_H__

#include "typedef.h"


#define IO_PORTA_00 				(GPIOA + 0)
#define IO_PORTA_01 				(GPIOA + 1)
#define IO_PORTA_02 				(GPIOA + 2)
#define IO_PORTA_03 				(GPIOA + 3)
#define IO_PORTA_04 				(GPIOA + 4)
#define IO_PORTA_05 				(GPIOA + 5)
#define IO_PORTA_06 				(GPIOA + 6)
#define IO_PORTA_07 				(GPIOA + 7)
#define IO_PORTA_08 				(GPIOA + 8)
#define IO_PORTA_09 				(GPIOA + 9)
#define IO_PORTA_10 				(GPIOA + 10)
#define IO_PORTA_11 				(GPIOA + 11)
#define IO_PORTA_12 				(GPIOA + 12)
#define IO_PORTA_13 				(GPIOA + 13)
#define IO_PORTA_MASK              0x3fff

#define IO_PORTB_00 				(GPIOB + 0)
#define IO_PORTB_01 				(GPIOB + 1)
#define IO_PORTB_02 				(GPIOB + 2)
#define IO_PORTB_03 				(GPIOB + 3)
#define IO_PORTB_04 				(GPIOB + 4)
#define IO_PORTB_05 				(GPIOB + 5)
#define IO_PORTB_06 				(GPIOB + 6)
#define IO_PORTB_07 				(GPIOB + 7)
#define IO_PORTB_08 				(GPIOB + 8)
#define IO_PORTB_09 				(GPIOB + 9)
#define IO_PORTB_10 				(GPIOB + 10)
#define IO_PORTB_11 				(GPIOB + 11)
#define IO_PORTB_12 				(GPIOB + 12) // 没有PB12
#define IO_PORTB_MASK              0x0fff

#define IO_PORTC_00 				(GPIOC + 0)
#define IO_PORTC_01 				(GPIOC + 1)
#define IO_PORTC_02 				(GPIOC + 2)
#define IO_PORTC_03 				(GPIOC + 3)
#define IO_PORTC_04 				(GPIOC + 4)
#define IO_PORTC_05 				(GPIOC + 5)
#define IO_PORTC_06 				(GPIOC + 6)
#define IO_PORTC_07 				(GPIOC + 7)
#define IO_PORTC_08 				(GPIOC + 8)
#define IO_PORTC_09 				(GPIOC + 9)
#define IO_PORTC_10 				(GPIOC + 10)
#define IO_PORTC_11 				(GPIOC + 11)
#define IO_PORTC_12 				(GPIOC + 12)
#define IO_PORTC_13 				(GPIOC + 13)
#define IO_PORTC_14 				(GPIOC + 14)
#define IO_PORTC_15 				(GPIOC + 15)
#define IO_PORTC_MASK              0xffff

#define IO_PORTD_00 				(GPIOD + 0)
#define IO_PORTD_01 				(GPIOD + 1)
#define IO_PORTD_02 				(GPIOD + 2)
#define IO_PORTD_03 				(GPIOD + 3)
#define IO_PORTD_04 				(GPIOD + 4)
#define IO_PORTD_05 				(GPIOD + 5)
#define IO_PORTD_06 				(GPIOD + 6)
#define IO_PORTD_07 				(GPIOD + 7)
#define IO_PORTD_08 				(GPIOD + 8)
#define IO_PORTD_09 				(GPIOD + 9)
#define IO_PORTD_10 				(GPIOD + 10)
#define IO_PORTD_11 				(GPIOD + 11)
#define IO_PORTD_12 				(GPIOD + 12)
#define IO_PORTD_MASK              0x1fff

#define IO_PORTE_00 				(GPIOE + 0)
#define IO_PORTE_01 				(GPIOE + 1)
#define IO_PORTE_02 				(GPIOE + 2)
#define IO_PORTE_03 				(GPIOE + 3)
#define IO_PORTE_04 				(GPIOE + 4)
#define IO_PORTE_MASK              0x001f

//pmc io:pf
#define IO_PORTF_00 				(IO_GROUP_NUM * GPIOF_INDEX + 0) // pmc io
#define IO_PORTF_01 				(IO_GROUP_NUM * GPIOF_INDEX + 1) // pmc io
#define IO_PORTF_02 				(IO_GROUP_NUM * GPIOF_INDEX + 2) // pmc io
#define IO_PORTF_03 				(IO_GROUP_NUM * GPIOF_INDEX + 3) // pmc io
#define IO_PORTF_04 				(IO_GROUP_NUM * GPIOF_INDEX + 4) // pmc io
#define IO_PORTF_05 				(IO_GROUP_NUM * GPIOF_INDEX + 5) // pmc io
// #define IO_PORTF_MASK              0x003f

#define IO_PORTG_00 				(GPIOG + 0)
#define IO_PORTG_01 				(GPIOG + 1)
#define IO_PORTG_02 				(GPIOG + 2)
#define IO_PORTG_03 				(GPIOG + 3)
#define IO_PORTG_MASK              0x000f

#define IO_PORTH_00 				(GPIOH + 0)
#define IO_PORTH_01 				(GPIOH + 1)
#define IO_PORTH_02 				(GPIOH + 2)
#define IO_PORTH_03 				(GPIOH + 3)
#define IO_PORTH_04 				(GPIOH + 4)
#define IO_PORTH_05 				(GPIOH + 5)
#define IO_PORTH_06 				(GPIOH + 6)
#define IO_PORTH_07 				(GPIOH + 7)
#define IO_PORTH_08 				(GPIOH + 8)
#define IO_PORTH_MASK              0x01ff

//pmc io:pp
#define IO_PORTP_00 				(IO_GROUP_NUM * GPIOP_INDEX + 0) // pmc io:pp
#define IO_PORTP_01 				(IO_GROUP_NUM * GPIOP_INDEX + 1) // pmc io:pp
#define IO_PORTP_02 				(IO_GROUP_NUM * GPIOP_INDEX + 2) // pmc io:pp
#define IO_PORTP_MASK              0x0005 // pmc io:pp

// #define IO_PORT_LDOIN   IO_PORTP_01
#define IO_MAX_NUM 					(IO_PORTP_02 + 1)

#define IO_PORT_DP                  (GPIOUSB + 0)
#define IO_PORT_DM                  (GPIOUSB + 1)
#define IO_PORT_USB_MASK            0x03
#define IS_PORT_USB(x)              (x <= IO_PORT_DM)//无usb赋0

//无pr
// #define IO_PORT_PR_00               (GPIOR + 0)//pr固定15
// #define IO_PORT_PR_01               (GPIOR + 1)
// #define IO_PORTR_MASK              0x03

#define IO_PORT_MAX					(IO_PORT_DM + 1)

#define P33_IO_OFFSET               0
#define IO_CHGFL_DET                (IO_PORT_MAX + P33_IO_OFFSET + 0)
#define IO_VBGOK_DET                (IO_PORT_MAX + P33_IO_OFFSET + 1)
#define IO_VBTCH_DET                (IO_PORT_MAX + P33_IO_OFFSET + 2)
#define IO_LDOIN_DET                (IO_PORT_MAX + P33_IO_OFFSET + 3)
#define IO_VBATDT_DET               (IO_PORT_MAX + P33_IO_OFFSET + 4)
#define IO_SPDIF_RX_DET             (IO_PORT_MAX + P33_IO_OFFSET + 5)

#define PG_IO_OFFSET                5
#define IO_FS_PG2                    (IO_PORT_MAX + PG_IO_OFFSET + 2)

#define SDC_IO_OFFSET               8
#define IO_PORT_SDC                 (IO_PORT_MAX + SDC_IO_OFFSET + 0)

#define LDO_IO_OFFSET               9
#define IO_PORT_LDO28               (IO_PORT_MAX + LDO_IO_OFFSET + 0)

#define DAC_IO_OFFSET               10
#define IO_PORT_DACFL               (IO_PORT_MAX + DAC_IO_OFFSET + 0)
#define IO_PORT_DACFR               (IO_PORT_MAX + DAC_IO_OFFSET + 1)
#define IO_PORT_DACRL               (IO_PORT_MAX + DAC_IO_OFFSET + 2)

#define GPIOP33                     (IO_PORT_MAX + P33_IO_OFFSET)



#define GPIO_NEW_PUPDHD_EN 1 //new pu pd hd(需usb,pr,px都支持新上下拉,sh59)

#define PORT_INPUT_PULLUP_5K   PORT_INPUT_PULLUP_LEVEL1
#define PORT_INPUT_PULLUP_10K  PORT_INPUT_PULLUP_LEVEL2
#define PORT_INPUT_PULLUP_100K PORT_INPUT_PULLUP_10K
#define PORT_INPUT_PULLUP_200K PORT_INPUT_PULLUP_LEVEL3
#define PORT_INPUT_PULLUP_1M   PORT_INPUT_PULLUP_200K

#define PORT_INPUT_PULLDOWN_5K   PORT_INPUT_PULLDOWN_LEVEL1
#define PORT_INPUT_PULLDOWN_10K  PORT_INPUT_PULLDOWN_LEVEL2
#define PORT_INPUT_PULLDOWN_100K PORT_INPUT_PULLDOWN_10K
#define PORT_INPUT_PULLDOWN_200K PORT_INPUT_PULLDOWN_LEVEL3
#define PORT_INPUT_PULLDOWN_1M   PORT_INPUT_PULLDOWN_200K

//===================================================//
// Crossbar API
//===================================================//

#define  INPUT_GP_ICH_MAX  8
#define  INPUT_GP_ICH_BIT_WIDTH 5
#define  OUTPUT_GP_OCH_MAX 8
#define  OUTPUT_GP_OCH_BIT_WIDTH 8
#define  OUTPUT_GP_OCH_REG_ADDR ((u32 *)(&(JL_IOMC->OCH_IOMC0)))
#define  INPUT_GP_ICH_REG_ADDR  ((u32 *)(&(JL_IOMC->ICH_IOMC0)))
#define  INPUT_GP_ICH_SIGNAL_NUM (32/INPUT_GP_ICH_BIT_WIDTH)
#define  OUTPUT_GP_OCH_SIGNAL_NUM (32/OUTPUT_GP_OCH_BIT_WIDTH)

enum OUTPUT_CH_SIGNAL {
    OUTPUT_CH_SIGNAL_TIMER0_PWM,//8
    OUTPUT_CH_SIGNAL_TIMER1_PWM,
    OUTPUT_CH_SIGNAL_TIMER2_PWM,
    OUTPUT_CH_SIGNAL_TIMER3_PWM,
    OUTPUT_CH_SIGNAL_TIMER4_PWM,
    OUTPUT_CH_SIGNAL_TIMER5_PWM,
    OUTPUT_CH_SIGNAL_UART0_RTS,
    OUTPUT_CH_SIGNAL_CLOCK_OUT0,
    OUTPUT_CH_SIGNAL_CLOCK_OUT1,
    OUTPUT_CH_SIGNAL_CLOCK_OUT2,
    OUTPUT_CH_SIGNAL_GP_ICH0,
    OUTPUT_CH_SIGNAL_GP_ICH1,
    OUTPUT_CH_SIGNAL_NFC_TDAT,
    // OUTPUT_CH_SIGNAL_NULL0,
    // OUTPUT_CH_SIGNAL_NULL1,
    // OUTPUT_CH_SIGNAL_NULL2,
    // OUTPUT_CH_SIGNAL_NULL3,
    OUTPUT_CH_SIGNAL_USB_DBG_OUT,
    OUTPUT_CH_SIGNAL_LSPI0_CSO,
    OUTPUT_CH_SIGNAL_LSPI1_CSO,
    OUTPUT_CH_SIGNAL_PMC_DBG_OUT,
    OUTPUT_CH_SIGNAL_P33_SIG_DBG0,
    OUTPUT_CH_SIGNAL_P33_SIG_DBG1,
    // OUTPUT_CH_SIGNAL_NULL4,
    // OUTPUT_CH_SIGNAL_NULL5,
    // OUTPUT_CH_SIGNAL_NULL6,
    // OUTPUT_CH_SIGNAL_NULL7,
    // OUTPUT_CH_SIGNAL_NULL8,
    // OUTPUT_CH_SIGNAL_NULL9,
    // OUTPUT_CH_SIGNAL_NULL10,
    OUTPUT_CH_SIGNAL_SPDIF_DO,
    OUTPUT_CH_SIGNAL_ASS_DBG_CLKO,
    OUTPUT_CH_SIGNAL_ASS_DBG_DATO0,
    OUTPUT_CH_SIGNAL_ASS_DBG_DATO1,
    OUTPUT_CH_SIGNAL_ASS_DBG_DATO2,
    OUTPUT_CH_SIGNAL_ASS_DBG_DATO3,
    OUTPUT_CH_SIGNAL_ASS_DBG_DATO4,
    OUTPUT_CH_SIGNAL_ASS_DMIC_CLK,
    OUTPUT_CH_SIGNAL_DT_TX_P_OCH,
    OUTPUT_CH_SIGNAL_DT_TX_N_OCH,
    // OUTPUT_CH_SIGNAL_NULL11,

    // OUTPUT_CH_SIGNAL_WL_AMPE,
    // OUTPUT_CH_SIGNAL_WL_LNAE,
    // OUTPUT_CH_SIGNAL_WLC_INT_ACTIVE,
    // OUTPUT_CH_SIGNAL_WLC_INT_STATUS,
    // OUTPUT_CH_SIGNAL_WLC_INT_FREQ,
    // OUTPUT_CH_SIGNAL_ADC_DMIC_CLK,
    // OUTPUT_CH_SIGNAL_PLNK_CLK,
    // OUTPUT_CH_SIGNAL_P33_CLK_DBG,
    // OUTPUT_CH_SIGNAL_P11_DBG_OUT,
    // OUTPUT_CH_SIGNAL_WL_DBG_PORTx,//fix wl0~7对应och0~7

    // OUTPUT_CH_SIGNAL_MCPWM0_H,
    // OUTPUT_CH_SIGNAL_MCPWM0_L,
    // OUTPUT_CH_SIGNAL_MCPWM1_H,
    // OUTPUT_CH_SIGNAL_MCPWM1_L,
    // OUTPUT_CH_SIGNAL_LEDC0_OUT,
    // OUTPUT_CH_SIGNAL_LEDC1_OUT,
};

enum INPUT_CH_TYPE {
    INPUT_CH_TYPE_GP_ICH = 0,
    INPUT_CH_TYPE_TIME2_PWM = 8,
    INPUT_CH_TYPE_TIME3_PWM,
    // INPUT_CH_TYPE_WL_AMPE,
    // INPUT_CH_TYPE_WL_LNAE,
    INPUT_CH_TYPE_MAX,
};

enum INPUT_CH_SIGNAL {
    //ICH_CON0
    INPUT_CH_SIGNAL_TIMER0_CIN = 0,//5
    INPUT_CH_SIGNAL_TIMER1_CIN,
    INPUT_CH_SIGNAL_TIMER2_CIN,
    INPUT_CH_SIGNAL_TIMER3_CIN,
    INPUT_CH_SIGNAL_TIMER4_CIN,
    INPUT_CH_SIGNAL_TIMER5_CIN,
    //ICH_CON1
    INPUT_CH_SIGNAL_TIMER0_CAPTURE,
    INPUT_CH_SIGNAL_TIMER1_CAPTURE,
    INPUT_CH_SIGNAL_TIMER2_CAPTURE,
    INPUT_CH_SIGNAL_TIMER3_CAPTURE,
    INPUT_CH_SIGNAL_TIMER4_CAPTURE,
    INPUT_CH_SIGNAL_TIMER5_CAPTURE,
    //ICH_CON2
    ICH_ADV_TMR0_IC1_IN,
    ICH_ADV_TMR0_IC2_IN,
    ICH_ADV_TMR0_IC3_IN,
    ICH_ADV_TMR0_IC4_IN,
    ICH_ADV_TMR0_ETR_IN,
    ICH_ADV_TMR0_BRK_IN1,

    //ICH_CON3
    ICH_ADV_TMR0_BRK_IN2,
    INPUT_CH_SIGNAL_UART0_CTS,
    INPUT_CH_SIGNAL_CAP,//CAP_MUX_OUT
    INPUT_CH_SIGNAL_CLK_PIN,  //CLK_MUX_IN
    INPUT_CH_SIGNAL_EXT_CLK,  //EXT_CLK_P
    INPUT_CH_SIGNAL_LSPI0_CS,

    //ICH_CON4
    INPUT_CH_SIGNAL_LSPI1_CS,
    INPUT_CH_SIGNAL_ASS_DMIC_IDAT0,
    INPUT_CH_SIGNAL_ASS_DMIC_IDAT1,
    INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIA,
    INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIB,
    INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DIC,
    //ICH_CON5
    INPUT_CH_SIGNAL_ASS_DMIC_SPDIF_DID,
    INPUT_CH_SIGNAL_RESERVE0,
    INPUT_CH_SIGNAL_RESERVE1,
    INPUT_CH_SIGNAL_RESERVE2,
    INPUT_CH_SIGNAL_RESERVE3,
    INPUT_CH_SIGNAL_RESERVE4,
    INPUT_CH_SIGNAL_RESERVE5,
    //ICH_CON6
    INPUT_CH_SIGNAL_RESERVE6,

    // INPUT_CH_SIGNAL_MCPWM0_CK,
    // INPUT_CH_SIGNAL_MCPWM1_CK,
    // INPUT_CH_SIGNAL_MCPWM0_FP,
    // INPUT_CH_SIGNAL_MCPWM1_FP,
    // INPUT_CH_SIGNAL_WLC_EXT_ACT,
    // INPUT_CH_SIGNAL_SPI1_CS,
    // INPUT_CH_SIGNAL_IRFLT,
    // INPUT_CH_SIGNAL_SPI2_CS,
    // INPUT_CH_SIGNAL_IMD_TE,
    // INPUT_CH_SIGNAL_AUD_DBG_DATI,
    // INPUT_CH_SIGNAL_PLNK_IDAT0,
    // INPUT_CH_SIGNAL_PLNK_IDAT1,
    // INPUT_CH_SIGNAL_QDEC_SIN0,
    // INPUT_CH_SIGNAL_QDEC_SIN1,
};
enum gpio_function {
    PORT_FUNC_NULL,    //null
//uart
    PORT_FUNC_UART0_TX, //out
    PORT_FUNC_UART0_RX,//in
    PORT_FUNC_UART1_TX, //out
    PORT_FUNC_UART1_RX,//in
    PORT_FUNC_UART2_TX, //out
    PORT_FUNC_UART2_RX,//in
    PORT_FUNC_UART3_TX, //out
    PORT_FUNC_UART3_RX,//in
    PORT_FUNC_UART0_RTS,//out
    PORT_FUNC_UART0_CTS,//in

//spi
    PORT_FUNC_SPI0_CS,//och:master; ich slave
    PORT_FUNC_SPI0_CLK,
    PORT_FUNC_SPI0_DA0,
    PORT_FUNC_SPI0_DA1,
    PORT_FUNC_SPI0_DA2,
    PORT_FUNC_SPI0_DA3,
    PORT_FUNC_SPI1_CS,//och:master; ich slave
    PORT_FUNC_SPI1_CLK,
    PORT_FUNC_SPI1_DA0,
    PORT_FUNC_SPI1_DA1,
    PORT_FUNC_SPI1_DA2,
    PORT_FUNC_SPI1_DA3,
    // PORT_FUNC_SPI2_CS,//ich slave
    // PORT_FUNC_SPI2_CLK,
    // PORT_FUNC_SPI2_DA0,
    // PORT_FUNC_SPI2_DA1,
    // PORT_FUNC_SPI2_DA2,
    // PORT_FUNC_SPI2_DA3,

//iic
    PORT_FUNC_IIC0_SCL,
    PORT_FUNC_IIC0_SDA,
    PORT_FUNC_IIC1_SCL,
    PORT_FUNC_IIC1_SDA,
    PORT_FUNC_IIC2_SCL,
    PORT_FUNC_IIC2_SDA,
    PORT_FUNC_IIC3_SCL,
    PORT_FUNC_IIC3_SDA,
    PORT_FUNC_IIC4_SCL,
    PORT_FUNC_IIC4_SDA,
    PORT_FUNC_IIC5_SCL,
    PORT_FUNC_IIC5_SDA,

//CAN
    PORT_FUNC_CAN_TX,
    PORT_FUNC_CAN_RX,

//sd
    PORT_FUNC_SD0_CLK,//out
    PORT_FUNC_SD0_CMD,
    PORT_FUNC_SD0_DA0,
    PORT_FUNC_SD0_DA1,
    PORT_FUNC_SD0_DA2,
    PORT_FUNC_SD0_DA3,

    // PORT_FUNC_SD1_CLK,//out
    // PORT_FUNC_SD1_CMD,
    // PORT_FUNC_SD1_DA0,
    // PORT_FUNC_SD1_DA1,
    // PORT_FUNC_SD1_DA2,
    // PORT_FUNC_SD1_DA3,

//timer
    PORT_FUNC_TIMER0_PWM,
    PORT_FUNC_TIMER1_PWM,
    PORT_FUNC_TIMER2_PWM,
    PORT_FUNC_TIMER3_PWM,
    PORT_FUNC_TIMER4_PWM,
    PORT_FUNC_TIMER5_PWM,
    PORT_FUNC_TIMER0_CAPTURE,
    PORT_FUNC_TIMER1_CAPTURE,
    PORT_FUNC_TIMER2_CAPTURE,
    PORT_FUNC_TIMER3_CAPTURE,
    PORT_FUNC_TIMER4_CAPTURE,
    PORT_FUNC_TIMER5_CAPTURE,
    PORT_FUNC_TIMER0_CIN,
    PORT_FUNC_TIMER1_CIN,
    PORT_FUNC_TIMER2_CIN,
    PORT_FUNC_TIMER3_CIN,
    PORT_FUNC_TIMER4_CIN,
    PORT_FUNC_TIMER5_CIN,

//advtimer:
    PORT_FUNC_TMR0_OC1P,
    PORT_FUNC_TMR0_OC2P,
    PORT_FUNC_TMR0_OC3P,
    PORT_FUNC_TMR0_OC4P,
    PORT_FUNC_TMR0_OC1N,
    PORT_FUNC_TMR0_OC2N,
    PORT_FUNC_TMR0_OC3N,
    PORT_FUNC_TMR0_OC4N,
    PORT_FUNC_TMR0_IC1,
    PORT_FUNC_TMR0_IC2,
    PORT_FUNC_TMR0_IC3,
    PORT_FUNC_TMR0_IC4,
    PORT_FUNC_TMR0_ETR_IN,
    PORT_FUNC_TMR0_BRK_IN1,
    PORT_FUNC_TMR0_BRK_IN2,

//mcpwm
    PORT_FUNC_MCPWM0_H,
    PORT_FUNC_MCPWM0_L,
    PORT_FUNC_MCPWM1_H,
    PORT_FUNC_MCPWM1_L,
    PORT_FUNC_MCPWM0_FP,
    PORT_FUNC_MCPWM1_FP,
    PORT_FUNC_MCPWM0_CK,
    PORT_FUNC_MCPWM1_CK,

//clk_out
    PORT_FUNC_OCH_CLOCK_OUT0,
    PORT_FUNC_OCH_CLOCK_OUT1,//PORT_FUNC_OCH_RESERVED0,//不连续
    PORT_FUNC_OCH_CLOCK_OUT2,
    // PORT_FUNC_OCH_CLOCK_OUT3,
//other
    PORT_FUNC_IRFLT_0, //实际只有1个IRFLT
    // PORT_FUNC_IRFLT_1,
    // PORT_FUNC_IRFLT_2,
    // PORT_FUNC_IRFLT_3,

    PORT_FUNC_CLK_PIN,//CLK_MUX_IN
    // PORT_FUNC_PORT_WKUP,
    PORT_FUNC_GPADC,    //in
    PORT_FUNC_PWM_LED,
    PORT_FUNC_PWM_LED_OE0,
    PORT_FUNC_PWM_LED_OE1,

//plnk
    PORT_FUNC_PLNK_SCLK,//out
    PORT_FUNC_PLNK_DAT0,//in
    PORT_FUNC_PLNK_DAT1,//in

//ledc
    // PORT_FUNC_LEDC0_OUT,
    // PORT_FUNC_LEDC1_OUT,

//rdec
    // PORT_FUNC_RDEC0_PORT0,
    // PORT_FUNC_RDEC0_PORT1,
    PORT_FUNC_RDEC0_PORTA,
    PORT_FUNC_RDEC0_PORTB,

//qdec
    PORT_FUNC_QDEC0_SIN0,
    PORT_FUNC_QDEC0_SIN1,
    PORT_FUNC_QDEC1_SIN0,
    PORT_FUNC_QDEC1_SIN1,
    PORT_FUNC_QDEC2_SIN0,
    PORT_FUNC_QDEC2_SIN1,
//chain
};
/**************************************************/
#define __struct(x) (x##_TypeDef *)
#define _struct(x) __struct(x)

#ifdef GPIOR
#define JL_PORTPR ((struct port_pr_reg *)R3_PR_IO_P)
#endif
#ifdef GPIOUSB
// #define JL_PORTUSB (JL_PORTUSB)
#endif
#define __toggle_port(x,y) JL_PORT##x->OUT ^= y;
#define _toggle_port(port,pin) __toggle_port(port,pin)

//log:
#define __portx(x,y) __PORT##x->y
#define _portx(x,y)  __portx(x,y)
#define GPIO_NO_SUPPORT_FUN "------"
#if defined(GPIO_NEW_PUPDHD_EN) && GPIO_NEW_PUPDHD_EN
#define GPIO_LOG_ITEM "port   :in      out     dir     die     dieh    pu          pd          hd          spl     sel"
#define GPIO_LOG_FORMAT "0x%04x  0x%04x  0x%04x  0x%04x  0x%04x  0x%08x  0x%08x  0x%08x  0x%04x  0x%04x"
#define GPIO_LOG_PORT(x,y) JL_PORT##x->IN&y,JL_PORT##x->OUT&y,JL_PORT##x->DIR&y,JL_PORT##x->DIE&y,JL_PORT##x->DIEH&y,JL_PORT##x->PU&0xffffffff,JL_PORT##x->PD&0xffffffff,JL_PORT##x->HD&0xffffffff,JL_PORT##x->SPL&y,JL_PORT##x->SEL&y
#else
#define GPIO_LOG_ITEM "port   :in      out     dir     die     dieh    pu0   ,pu1     pd0   ,pd1     hd0   ,hd1     spl"
#define GPIO_LOG_FORMAT "0x%04x  0x%04x  0x%04x  0x%04x  0x%04x  0x%04x,0x%04x  0x%04x,0x%04x  0x%04x,0x%04x  0x%04x"
#define GPIO_LOG_PORT(x,y) JL_PORT##x->IN&y,JL_PORT##x->OUT&y,JL_PORT##x->DIR&y,JL_PORT##x->DIE&y,JL_PORT##x->DIEH&y,JL_PORT##x->PU0&y,JL_PORT##x->PU1&y,JL_PORT##x->PD0&y,JL_PORT##x->PD1&y,JL_PORT##x->HD0&y,JL_PORT##x->HD1&y,JL_PORT##x->SPL&y
#endif
#ifdef GPIOR
#define GPIO_LOG_FORMAT_R "0x%04x  0x%04x  0x%04x  0x%04x  %s  0x%04x,0x%04x  0x%04x,0x%04x  0x%04x,0x%04x  %s"
#define GPIO_LOG_PORTR R3_PR_IN,R3_PR_OUT,R3_PR_DIR,R3_PR_DIE,GPIO_NO_SUPPORT_FUN,R3_PR_PU0,R3_PR_PU1,R3_PR_PD0,R3_PR_PD1,R3_PR_HD0,R3_PR_HD1,GPIO_NO_SUPPORT_FUN
#endif
#ifdef GPIOUSB
#if defined(GPIO_NEW_PUPDHD_EN) && GPIO_NEW_PUPDHD_EN
#else
#define GPIO_LOG_FORMAT_U "0x%04x  0x%04x  0x%04x  0x%04x  0x%04x  0x%04x,%s  0x%04x,%s  %s,%s  0x%04x"
#define GPIO_LOG_PORTU _portx(PU,in),_portx(PU,out),_portx(PU,dir),_portx(PU,die),_portx(PU,dieh),_portx(PU,pu0),GPIO_NO_SUPPORT_FUN,_portx(PU,pd0),GPIO_NO_SUPPORT_FUN,GPIO_NO_SUPPORT_FUN,GPIO_NO_SUPPORT_FUN,_portx(PU,spl)
#endif
#endif

/*************************function*************************/
void usb_iomode(const u32 enable);


/**************************************************************/
/*********************multi pin interface***************************/
//多io同一模式
int gpio_hw_set_direction(const enum gpio_port port, u32 pin, const u32 value);
int gpio_hw_direction_input(const enum gpio_port port, u32 pin);
int gpio_hw_direction_output(const enum gpio_port port, u32 pin, const int value);/////////
int gpio_hw_write_port(const enum gpio_port port, u32 pin, const u32 value);
int gpio_hw_set_output_value(const enum gpio_port port, u32 pin, const u32 value);
int gpio_hw_set_pull_up(const enum gpio_port port, u32 pin, const enum gpio_pullup_mode value);
int gpio_hw_set_pull_down(const enum gpio_port port, u32 pin, const enum gpio_pulldown_mode value);//portabcdpr:pd0,pd1,usb:pd0
int gpio_hw_set_drive_strength(const enum gpio_port port, u32 pin, const enum gpio_drive_strength value);
int gpio_hw_set_die(const enum gpio_port port, u32 pin, const int value);
int gpio_hw_set_dieh(const enum gpio_port port, u32 pin, const u32 value);

//开漏输出
int gpio_hw_set_spl(const enum gpio_port port, u32 pin, const u32 value);
//hvt必须开下拉100k
int gpio_hw_set_hvt(const enum gpio_port port, u32 pin, const u32 value);//输入io1/2分压到adc(仅HVT类型io有效,其他io无效,影响功耗)
int gpio_hw_set_ddr(const enum gpio_port port, u32 pin, const u32 value);//VD18电源域使能(仅DDR类型io有效,其他io无效,影响功耗)
int gpio_hw_set_pmc(const enum gpio_port port, u32 pin, const u32 value);//MSYS控制pmc-io(仅PMC类型io有效,其他io无效)

int gpio_hw_read_port(const enum gpio_port port, u32 pin);
int gpio_hw_read_out_level(const enum gpio_port port, u32 pin);
u32 gpio_hw_read_drive_strength(const enum gpio_port port, u32 pin);


//将某个输入io的信号转到其他io输出:(两个io需要支持crossbar)
//需要输入通道0或1，不能被占用; 占用1个输出通道ochx输出到其他io
void gpio_set_inputio2other_io_out(u32 input_io, u32 out_io);

u32 get_sfc_port(void);

#endif  /*GPIO_H*/
