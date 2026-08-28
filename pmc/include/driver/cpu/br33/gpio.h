/*********************************************************************************************
    *   Filename        : gpio.h

    *   Description     : 本文件存放p11 gpio的接口函数和宏定义

    *   Author          : MoZhiYe

    *   Email           : mozhiye@zh-jieli.com

    *   Last modifiled  : 2021-05-17 09:00

    *   Copyright:(c)JIELI  2021-2029  @ , All Rights Reserved.
*********************************************************************************************/

#ifndef __GPIO_H__
#define __GPIO_H__

#define IO_GROUP_NUM 				16


#define GPIOA_INDEX     0
#define GPIOB_INDEX     1
#define GPIOC_INDEX     2
#define GPIOD_INDEX     3
#define GPIOE_INDEX     4
#define GPIOF_INDEX     5
#define GPIOG_INDEX     6
#define GPIOH_INDEX     7
#define GPIOP_INDEX     13
#define GPIOUSB_INDEX   14
#define GPIOR_INDEX     15

#define GPIOD                       (IO_GROUP_NUM * GPIOD_INDEX)//br35无PDx_OUT/IN
#define GPIOE                       (IO_GROUP_NUM * GPIOE_INDEX)//
#define GPIOF                       (IO_GROUP_NUM * GPIOF_INDEX) //pmc io
// #define GPIOG                       (IO_GROUP_NUM * 6)
#define GPIOH                       (IO_GROUP_NUM * GPIOH_INDEX)
#define GPIOP                       (IO_GROUP_NUM * GPIOP_INDEX) //pmc io

//PB0 ~ PB13
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

#define IO_PORTE_00 				(GPIOE + 0)
#define IO_PORTE_01 				(GPIOE + 1)
#define IO_PORTE_02 				(GPIOE + 2)
#define IO_PORTE_03 				(GPIOE + 3)
#define IO_PORTE_04 				(GPIOE + 4)

#define IO_PORTF_00 				(GPIOF + 0)
#define IO_PORTF_01 				(GPIOF + 1)
#define IO_PORTF_02 				(GPIOF + 2)
#define IO_PORTF_03 				(GPIOF + 3)
#define IO_PORTF_04 				(GPIOF + 4)
#define IO_PORTF_05 				(GPIOF + 5)

#define IO_PORTH_00 				(GPIOH + 0)
#define IO_PORTH_01 				(GPIOH + 1)
#define IO_PORTH_02 				(GPIOH + 2)
#define IO_PORTH_03 				(GPIOH + 3)
#define IO_PORTH_04 				(GPIOH + 4)
#define IO_PORTH_05 				(GPIOH + 5)
#define IO_PORTH_06 				(GPIOH + 6)
#define IO_PORTH_07 				(GPIOH + 7)
#define IO_PORTH_08 				(GPIOH + 8)

#define IO_PORTP_00 				(GPIOP + 0)
#define IO_PORTP_01 				(GPIOP + 1)
#define IO_PORTP_02 				(GPIOP + 2)

#define IO_MAX_NUM 					(IO_PORTP_02 + 1)

#ifndef EINVAL
#define EINVAL      22  /* Invalid argument */
#endif /* #ifndef EINVAL */

#ifndef EFAULT
#define	EFAULT		14	/* Bad address */
#endif /* #ifndef EFAULT */

enum gpio_drive_strength {
    PORT_DRIVE_STRENGT_2p4mA,		///< 最大驱动电流  2.4mA
    PORT_DRIVE_STRENGT_8p0mA,		///< 最大驱动电流  8.0mA
    PORT_DRIVE_STRENGT_24p0mA,		///< 最大驱动电流 24.0mA
    PORT_DRIVE_STRENGT_64p0mA,		///< 最大驱动电流 64.0mA
};

//===================================================//
// BR27 P11 Crossbar API
//===================================================//
enum PFI_TABLE {
    PFI_GP_ICH0 = ((u32)(&(PMC_IMAP->PMC_FI_GP_ICH0))),
    PFI_GP_ICH1 = ((u32)(&(PMC_IMAP->PMC_FI_GP_ICH1))),
    PFI_GP_ICH2 = ((u32)(&(PMC_IMAP->PMC_FI_GP_ICH2))),
    PFI_GP_ICH3 = ((u32)(&(PMC_IMAP->PMC_FI_GP_ICH3))),
    PFI_GP_ICH4 = ((u32)(&(PMC_IMAP->PMC_FI_GP_ICH4))),
    PFI_GP_ICH5 = ((u32)(&(PMC_IMAP->PMC_FI_GP_ICH5))),
    PFI_UART0_RX = ((u32)(&(PMC_IMAP->PMC_FI_UART0_RX))),
    PFI_UART1_RX = ((u32)(&(PMC_IMAP->PMC_FI_UART1_RX))),
    PFI_SPI0_CLK = ((u32)(&(PMC_IMAP->PMC_FI_SPI0_CLK))),
    PFI_SPI0_DA0 = ((u32)(&(PMC_IMAP->PMC_FI_SPI0_DA0))),
    PFI_SPI0_DA1 = ((u32)(&(PMC_IMAP->PMC_FI_SPI0_DA1))),
    PFI_SPI0_DA2 = ((u32)(&(PMC_IMAP->PMC_FI_SPI0_DA2))),
    PFI_SPI0_DA3 = ((u32)(&(PMC_IMAP->PMC_FI_SPI0_DA3))),
    PFI_IIC0_SCL = ((u32)(&(PMC_IMAP->PMC_FI_IIC0_SCL))),
    PFI_IIC0_SDA = ((u32)(&(PMC_IMAP->PMC_FI_IIC0_SDA))),
    PFI_IIC1_SCL = ((u32)(&(PMC_IMAP->PMC_FI_IIC1_SCL))),
    PFI_IIC1_SDA = ((u32)(&(PMC_IMAP->PMC_FI_IIC1_SDA))),
    PFI_DMIC_DAT = ((u32)(&(PMC_IMAP->PMC_FI_DMIC_DAT))),
};

#define  INPUT_GP_ICH_MAX  6
#define  INPUT_GP_ICH_BIT_WIDTH 5
#define  OUTPUT_GP_OCH_MAX 8
#define  OUTPUT_GP_OCH_BIT_WIDTH 8
#define  INPUT_GP_ICH_SIGNAL_NUM (32/INPUT_GP_ICH_BIT_WIDTH)
#define  OUTPUT_GP_OCH_SIGNAL_NUM (32/OUTPUT_GP_OCH_BIT_WIDTH)

enum OUTPUT_CH_SIGNAL {
    OCH_CLOCK_OUT0,
    OCH_CLOCK_OUT1,
    OCH_TIMER0_PWM,
    OCH_TIMER1_PWM,
    // OCH_TIMER2_PWM,
    // OCH_TIMER3_PWM,
    // OCH_TIMER4_PWM,
    // OCH_TIMER5_PWM,
    OCH_LPCTM0_ANA_CLK,
    OCH_PMC_SIG_DBG,
    OCH_P33_SIG_DBG0,
    OCH_P33_SIG_DBG1,
    OCH_WL_OCH_OPORT0,
    OCH_WL_OCH_OPORT1,
    OCH_WL_OCH_OPORT2,
    OCH_WL_OCH_OPORT3,
    OCH_WL_OCH_OPORT4,
    OCH_WL_OCH_OPORT5,
    OCH_WL_OCH_OPORT6,
    OCH_WL_OCH_OPORT7,
    OCH_WL_OCH_OPORT8,
    OCH_WL_OCH_OPORT9,
    OCH_WL_OCH_OPORT10,
    OCH_WL_OCH_OPORT11,
    OCH_WL_OCH_OPORT12,
    OCH_LPSPI0_CSO,
};

enum INPUT_CH_TYPE {
    ICH_TYPE_GP_ICH = 0,
    ICH_TYPE_P33_PORT_IN0 = 6,
    ICH_TYPE_P33_PORT_IN1,
    ICH_TYPE_TIME0_PWM,
    ICH_TYPE_TIME1_PWM,
    ICH_TYPE_MAX,
};

enum INPUT_CH_SIGNAL {
    //ICH_CON0
    ICH_TIMER0_CIN = 0,//5
    ICH_TIMER0_CAPTURE,
    ICH_TIMER1_CIN,
    ICH_TIMER1_CAPTURE,
    ICH_EXT_CLK,
    ICH_CAP,

    //ICH_CON1
    ICH_CLK_MUX,
    ICH_WL_ICH_IPORT,
    ICH_LPSPI0_CS,
    ICH_RESERVE0,
    ICH_RESERVE1,

    //ICH_CON2

    //ICH_CON3
    //ICH_CON4

    // ICH_RESERVE2,
    // ICH_RESERVE3,
};

int gpio_set_direction(u32 gpio, u32 dir);

int gpio_set_output_value(u32 gpio, u32 value);

int gpio_set_pull_up(u32 gpio, u32 value);

int gpio_set_pull_down(u32 gpio, int value);

int gpio_set_hd(u32 gpio, int value);

int gpio_set_die(u32 gpio, int value);

int gpio_set_dieh(u32 gpio, int value);

int gpio_set_spl(u32 gpio, int value);

int gpio_read(u32 gpio);

const char *gpio_get_name(u32 gpio);

int gpio_set_fun_output_port(u32 gpio, u32 fun_index, u8 dir_ctl, u8 data_ctl);
int gpio_disable_fun_output_port(u32 gpio);

int gpio_set_fun_input_port(u32 gpio, enum PFI_TABLE pfun);
int gpio_disable_fun_input_port(enum PFI_TABLE pfun);

int gpio_och_sel_output_signal(u32 gpio, enum OUTPUT_CH_SIGNAL signal);
int gpio_och_disable_output_signal(u32 gpio, enum OUTPUT_CH_SIGNAL signal);
int gpio_ich_sel_input_signal(u32 gpio, enum INPUT_CH_SIGNAL signal, enum INPUT_CH_TYPE type);
int gpio_ich_disable_input_signal(u32 gpio, enum INPUT_CH_SIGNAL signal, enum INPUT_CH_TYPE type);







#define IO_PORT_SPLIT(io) ((u32)(io)) / 16, BIT(((u32)(io)) %16)

// PORTÒý½ÅÊäÈëÊä³öÄ£Ê½
enum gpio_mode {
    PORT_OUTPUT_LOW = 0,
    PORT_OUTPUT_HIGH = 1,
    PORT_HIGHZ = 2,               //¸ß×èÄ£Ê½

    PORT_INPUT_FLOATING = 0x10,   //¸¡¿ÕÊäÈë
    PORT_INPUT_PULLUP_10K = 0x11,
    PORT_INPUT_PULLUP_100K,
    PORT_INPUT_PULLUP_1M,

    PORT_INPUT_PULLDOWN_10K = 0x21,
    PORT_INPUT_PULLDOWN_100K,
    PORT_INPUT_PULLDOWN_1M,

    PORT_KEEP_STATE = 0x30,
};
//ÉÏÀ­µç×èÓÐ4µµ,¾ßÌå×èÖµÇë²éÔÄÐ¾Æ¬¹æ¸ñÊé²é¿´Ð¾Æ¬¹æ¸ñÊé
enum gpio_pullup_mode {
    GPIO_PULLUP_DISABLE,
    GPIO_PULLUP_10K,
    GPIO_PULLUP_100K,
    GPIO_PULLUP_1M,
};

//ÏÂÀ­µç×èÓÐ4µµ,¾ßÌå×èÖµÇë²éÔÄÐ¾Æ¬¹æ¸ñÊé
enum gpio_pulldown_mode {
    GPIO_PULLDOWN_DISABLE,
    GPIO_PULLDOWN_10K,
    GPIO_PULLDOWN_100K,
    GPIO_PULLDOWN_1M,
};

//return:<0:error
int gpio_set_mode(u32 port, u32 pin, enum gpio_mode mode);

#endif /* #ifndef __GPIO_H__ */
