
#include "product_test.h"
#include "pt_gpio.h"
#include "gpio.h"
#include "asm/gpio_hw.h"
#include "ui/ui_api.h"
#define LOG_TAG_CONST     		PRODUCT_TEST
#define LOG_TAG     		"[PRODUCT_TEST]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if PT_GPIO_ENABLE
#define GPIO_CHECK_DELAY_US			10

// 记录PCBA中没有外接设备的GPIO。
// 相邻的写在一起，和其他的块之间以IO_PORT_MAX间隔
// 此处以7070为例，PCBA不接屏，不做屏幕测试
// 使用其他芯片时请按实际io和硬件外设配置，删掉没有邦的io口
static const u8 idle_gpio_list[] = {

#if PT_GPIO_CHECK_LCD_TP
    //TP
    /*    rst		int			sda				scl	*/
    IO_PORTA_05, IO_PORTA_06, IO_PORTA_07, IO_PORTA_08, IO_PORT_MAX,
    //LCD
    /* 	 en			rst			te				cs			clk			d0			d1				d2			d3			*/
    IO_PORTB_02, IO_PORTB_03, IO_PORTB_04, IO_PORTB_05, IO_PORTB_06, IO_PORTB_07, IO_PORTB_08, IO_PORTB_09, IO_PORTB_10, IO_PORT_MAX,
#endif /* #if PT_GPIO_CHECK_LCD_TP */
    /*mic0										mic1		*/
    /* IO_PORTA_00, IO_PORTA_01, IO_PORTA_02, IO_PORTA_03, IO_PORTA_04, IO_PORT_MAX, */

    IO_PORTA_09, IO_PORTA_10, IO_PORTA_11, IO_PORTA_12, IO_PORTA_13, IO_PORT_MAX,
    IO_PORTB_00, IO_PORTB_01, IO_PORT_MAX,
    /* SD(PC0~PC5) NULL(PC6~PC8)*/
    /* IO_PORTC_00, IO_PORTC_01, IO_PORTC_02, IO_PORTC_03, IO_PORTC_04, IO_PORTC_05, IO_PORTC_06, IO_PORTC_07, IO_PORTC_08, IO_PORT_MAX, */
    /*sfc1(PC9~PC15)
     	d0			clk			d3			cs0				d1			d2				cs1
     */
    /* IO_PORTC_09, IO_PORTC_10, IO_PORTC_11, IO_PORTC_12, IO_PORTC_13, IO_PORTC_14, IO_PORTC_15, IO_PORT_MAX, */
    /*NULL(PD0~PD11),可能用于PMC，如果PMC使用了，大核拿不到权限就会报错*/																						/*		SENSOR					*/
    /*打印*/
    /*IO_PORTD_00, IO_PORTD_01,*/ IO_PORTD_02, IO_PORTD_03, IO_PORTD_04, IO_PORTD_05, IO_PORTD_06, IO_PORTD_07, /*IO_PORTD_08, IO_PORTD_09, IO_PORTD_10,*/ IO_PORTD_11, IO_PORT_MAX,
    /* NULL			RDEC0		RDEC1			ADKEY		IOKEY*/
    IO_PORTE_00, IO_PORTE_01, IO_PORTE_02, IO_PORTE_03, IO_PORTE_04, IO_PORT_MAX,
    /*NULL(PG0~PG3)*/
    IO_PORTG_00, IO_PORTG_01, IO_PORTG_02, IO_PORTG_03, IO_PORT_MAX,
    /*camera*/																					/*mute*/
    IO_PORTH_00, IO_PORTH_01, IO_PORTH_02, IO_PORTH_03, IO_PORTH_04, IO_PORTH_05, IO_PORTH_06, IO_PORTH_07, IO_PORTH_08, IO_PORT_MAX,
};
static const u32 gpio_regs[] = {
    (u32) JL_PORTA,
    (u32) JL_PORTB,
    (u32) JL_PORTC,
    (u32) JL_PORTD,
    (u32) JL_PORTE,
    (u32) - 1, //F 占位
    (u32) JL_PORTG,
    (u32) JL_PORTH,
};
struct pt_gpio {
    volatile unsigned int in;
    volatile unsigned int out;
    volatile unsigned int dir;
    volatile unsigned int die;
    volatile unsigned int dieh;
    volatile unsigned int pu;
    volatile unsigned int pd;
    volatile unsigned int hd;
    volatile unsigned int spl;
    volatile unsigned int con;
    volatile unsigned int con1;
    volatile unsigned int sel;
};
struct pt_gpio pt_gpio1;
struct pt_gpio pt_gpio2;
struct port_reg *pt_gpio_status;
void gpio_status_config(u32 gpio, u32 mode)
{
    pt_gpio_status = (struct port_reg *)gpio_regs[gpio / IO_GROUP_NUM];
    u32 pt_pin = gpio % 16;
    switch (mode & 0xf) {
    case 0://保存gpio1的状态
        pt_gpio1.out = pt_gpio_status->out & BIT(pt_pin);
        pt_gpio1.dir = pt_gpio_status->dir & BIT(pt_pin);
        pt_gpio1.pu = pt_gpio_status->pu & (BIT((pt_pin << 1)) | BIT(((pt_pin << 1) + 1)));
        pt_gpio1.pd = pt_gpio_status->pd & (BIT((pt_pin << 1)) | BIT(((pt_pin << 1) + 1)));
        break;
    case 1://恢复gpio1的状态
        pt_gpio_status->out |= pt_gpio1.out;
        pt_gpio_status->dir |= pt_gpio1.dir;
        pt_gpio_status->pu |= pt_gpio1.pu;
        pt_gpio_status->pd |= pt_gpio1.pd;
        break;
    case 2://保存gpio2的状态
        pt_gpio2.out = pt_gpio_status->out & BIT(pt_pin);
        pt_gpio2.dir = pt_gpio_status->dir & BIT(pt_pin);
        pt_gpio2.pu = pt_gpio_status->pu & (BIT((pt_pin << 1)) | BIT(((pt_pin << 1) + 1)));
        pt_gpio2.pd = pt_gpio_status->pd & (BIT((pt_pin << 1)) | BIT(((pt_pin << 1) + 1)));
        break;
    case 3://恢复gpio2的状态
        pt_gpio_status->out |= pt_gpio2.out;
        pt_gpio_status->dir |= pt_gpio2.dir;
        pt_gpio_status->pu |= pt_gpio1.pu;
        pt_gpio_status->pd |= pt_gpio1.pd;
        break;
    }
}

static u8 pt_gpio_busy = 0; // 忙碌标记
static u8 pt_gpio_res = PT_E_SYS_UNREADY;      		// 测试结果

static int idle_gpio_test(void)
{
    int ret = true;
    for (int i = 0; i < sizeof(idle_gpio_list) - 1; i++) {
        if (idle_gpio_list[i] == IO_PORT_MAX) {
            continue;
        }
        local_irq_disable();
        // 保存idle_gpio_list[i]状态
        u32 mode = 0;
        gpio_status_config(idle_gpio_list[i], mode);
        if (idle_gpio_list[i + 1] != IO_PORT_MAX) {
            // 保存idle_gpio_list[i+1]状态
            mode = 2;
            gpio_status_config(idle_gpio_list[i + 1], mode);

            // idle_gpio_list[i+1]输出高电平
            // idle_gpio_list[i]下拉输入
            // delay
            // 检测idle_gpio_list[i]，高电平标记出错并退出
            gpio_set_mode(IO_PORT_SPILT(idle_gpio_list[i + 1]), PORT_OUTPUT_HIGH);
            gpio_set_mode(IO_PORT_SPILT(idle_gpio_list[i ]), PORT_INPUT_PULLDOWN_10K);
            udelay(100);
            if (gpio_read(idle_gpio_list[i])) {
                local_irq_enable();
                log_error("%s,%d, i:%d, io:%d \n", __func__, __LINE__, i, idle_gpio_list[i]);
                return false;
            }

            // idle_gpio_list[i+1]输出低电平
            // idle_gpio_list[i]上拉输入
            // delay
            // 检测idle_gpio_list[i]，低电平标记出错并退出
            gpio_set_mode(IO_PORT_SPILT(idle_gpio_list[i + 1]), PORT_OUTPUT_LOW);
            gpio_set_mode(IO_PORT_SPILT(idle_gpio_list[i ]), PORT_INPUT_PULLUP_10K);
            udelay(100);
            if (gpio_read(idle_gpio_list[i]) == 0) {
                local_irq_enable();
                log_error("%s,%d, i:%d, io:%d \n", __func__, __LINE__, i, idle_gpio_list[i]);
                return false;
            }

            // 恢复idle_gpio_list[i+1]状态
            mode = 3;
            gpio_status_config(idle_gpio_list[i + 1], mode);
        } else {
            // idle_gpio_list[i]下拉输入
            // delay
            // 检测idle_gpio_list[i]，高电平标记出错并退出
            gpio_set_mode(IO_PORT_SPILT(idle_gpio_list[i ]), PORT_INPUT_PULLDOWN_10K);
            udelay(100);
            if (gpio_read(idle_gpio_list[i])) {
                local_irq_enable();
                log_error("%s,%d, i:%d, io:%d \n", __func__, __LINE__, i, idle_gpio_list[i]);
                return false;
            }

            // idle_gpio_list[i]上拉输入
            // delay
            // 检测idle_gpio_list[i]，低电平标记出错并退出
            gpio_set_mode(IO_PORT_SPILT(idle_gpio_list[i ]), PORT_INPUT_PULLUP_10K);
            udelay(100);
            if (gpio_read(idle_gpio_list[i]) == 0) {
                local_irq_enable();
                log_error("%s,%d, i:%d, io:%d \n", __func__, __LINE__, i, idle_gpio_list[i]);
                return false;
            }

        }

        // 恢复idle_gpio_list[i]状态
        mode = 1;
        gpio_status_config(idle_gpio_list[i], mode);

        local_irq_enable();
    }
    return true;
}

static int pt_gpio_test(int priv)
{
    u32 result = PT_E_OK;
    if (idle_gpio_test() == false) {
        result = PT_E_MOD_ERROR;
    }
    pt_gpio_busy = 0;

    pt_gpio_res = result;
    /* product_test_push_data(PT_ORDER_M_SET(PT_M_GPIO) | PT_ORDER_C_SET(PT_N_C_PUSH_RESULT), 4, &result); */
    /* product_test_push_data(PT_ORDER_M_SET(PT_M_SYSTEM) | PT_ORDER_C_SET(PT_S_C_HOLD), 4, &result); */

    return 0;
}

int pt_gpio_start(void)
{
    if (pt_gpio_busy) {
        return PT_E_MOD_RUN;
    }
#if PT_GPIO_CHECK_LCD_TP
    ui_set_shut_down_time(10);
    ui_auto_shut_down_modify();
    u8 cnt = 100;
    while (!lcd_sleep_status()) {
        os_time_dly(1);
        if (cnt == 0) {
            break;
        }
        cnt --;
    }
    log_debug("lcd_sleep_status:%d \n", lcd_sleep_status());
#endif /* #if PT_GPIO_CHECK_LCD_TP */

    pt_gpio_res = PT_E_MOD_RUN;
    int msg[3] = {0};
    msg[0] = (int)pt_gpio_test;
    msg[1] = 1;
    msg[2] = (int)0;
    do {
        int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        if (os_err == OS_ERR_NONE) {
            break;
        }
        if (os_err != OS_Q_FULL) {
            pt_gpio_res = PT_E_SYS_ERROR;
            return PT_E_SYS_ERROR;
        }
        os_time_dly(1);
    } while (1);
    /* pt_gpio_busy = 1; */

    return 0;
}

int pt_gpio_stop(void)
{
    if (pt_gpio_busy) {
        return PT_E_MOD_CANT_STOP;
    }
    if (pt_gpio_res == PT_E_MOD_RUN) {
        pt_gpio_res = PT_E_MOD_STOP_NO_END;
    }
    return 0;
}

int pt_gpio_ioctrl(u32 order, int len, void *param)
{
    u32 result = 0;
    switch (PT_ORDER_C_GET(order)) {
    case PT_N_C_START:
        result = pt_gpio_start();
        break;
    case PT_N_C_STOP:
        result = pt_gpio_stop();
        break;
    case PT_N_C_GET_RESULT:
        result = pt_gpio_res;
        break;
    default:
        result = PT_E_PARAM;
        break;
    }
    product_test_push_data(order, 4, (u8 *)&result);
    return result;
}

REGISTER_PT_MODULE(gpio) = {
    .module = PT_M_GPIO,
    .attr	= PT_ATTR_SELF,
    .init	= NULL,
    .ioctrl	= pt_gpio_ioctrl,
};

int pt_gpio_simulation_test()
{
    log_debug("%s %d", __func__, __LINE__);
    int ret = pt_gpio_test(0);
    ASSERT(!pt_gpio_res, "%s result:%d", __func__, pt_gpio_res);
    return ret;
}

#endif /* #if PT_GPIO_ENABLE */

