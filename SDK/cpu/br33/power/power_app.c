#include "asm/power_interface.h"
#include "power/p11_app_msg.h"
#include "init.h"
#include "app_config.h"
#include "cpu/includes.h"
#include "gpio_config.h"
#include "rtc/rtc_dev.h"

//-------------------------------------------------------------------
/*config
 */
#define CONFIG_UART_DEBUG_ENABLE	(CONFIG_DEBUG_ENABLE||CONFIG_DEBUG_LITE_ENABLE)
#ifdef TCFG_DEBUG_UART_TX_PIN
#define CONFIG_UART_DEBUG_PORT		TCFG_DEBUG_UART_TX_PIN
#else
#define CONFIG_UART_DEBUG_PORT		-1
#endif

#define DO_PLATFORM_UNINITCALL()	do_platform_uninitcall()


/*-----------------------------------------------------------------------
 *进入、退出低功耗函数回调状态，函数单核操作、关中断，请勿做耗时操作
 *
 */
__attribute__((weak))
void board_sleep_enter_callback()
{
    //注册在板卡
}

__attribute__((weak))
void board_sleep_exit_callback()
{
    //注册在板卡
}

__attribute__((weak))
void board_poweroff_uninit()
{
    //注册在板卡
}

static u32 pmc_pd_sel;
static u32 pmc_pe_sel;
static u32 pmc_ph_sel;
static u32 pmc_pp_sel;

/**
 * @brief 将 msys 侧 IO 配置按 mask 迁移到 PMC 侧
 * @param pmc_port PMC 侧端口寄存器
 * @param sys_port msys 侧端口寄存器
 * @param mask     需要迁移的引脚掩码
 * @note SEL 最后置位，完成控制权交接
 */
static void pmc_io_port_migrate(JL_PORT_TypeDef *pmc_port, JL_PORT_TypeDef *sys_port, u32 mask)
{
    pmc_port->DIR  = (pmc_port->DIR  & ~mask) | (sys_port->DIR  & mask);
    pmc_port->DIE  = (pmc_port->DIE  & ~mask) | (sys_port->DIE  & mask);
    pmc_port->DIEH = (pmc_port->DIEH & ~mask) | (sys_port->DIEH & mask);
    pmc_port->PU   = (pmc_port->PU   & ~mask) | (sys_port->PU   & mask);
    pmc_port->PD   = (pmc_port->PD   & ~mask) | (sys_port->PD   & mask);
    pmc_port->HD   = (pmc_port->HD   & ~mask) | (sys_port->HD   & mask);
    pmc_port->OUT  = (pmc_port->OUT  & ~mask) | (sys_port->OUT  & mask);
    pmc_port->SPL  = (pmc_port->SPL  & ~mask) | (sys_port->SPL  & mask);
    pmc_port->SEL |= mask;
    asm volatile("csync");
}

/**
 * @brief 退出低功耗时归还 IO 控制权
 * @param pmc_port PMC 侧端口寄存器
 * @param mask     需要归还的引脚掩码
 * @note msys 侧配置在进入时只读未写，无需回写
 */
static void pmc_io_port_restore(JL_PORT_TypeDef *pmc_port, u32 mask)
{
    pmc_port->SEL &= ~mask;
}

/**
 * @brief PMC 与 msys 之间切换 PORTD/E/H 的 IO 控制权
 * @param pmc_ctl 非 0：msys → PMC；0：PMC → msys
 */
void pmc_io_control_sw(u32 pmc_ctl)
{
    ipc_spin_lock(IPC_SPIN_LOCK_EVENT_P11_GPIO);

    if (pmc_ctl) {
        pmc_pd_sel = PMC_PORTD->SEL;
        pmc_pe_sel = PMC_PORTE->SEL;
        pmc_ph_sel = PMC_PORTH->SEL;
        pmc_pp_sel = PMC_PORTP->SEL;

        //进入低功耗，主系统控制的 IO 需要由 PMC 来控制
        pmc_io_port_migrate(PMC_PORTD, JL_PORTD, ~pmc_pd_sel & IO_PORTD_MASK);
        pmc_io_port_migrate(PMC_PORTE, JL_PORTE, ~pmc_pe_sel & IO_PORTE_MASK);
        pmc_io_port_migrate(PMC_PORTH, JL_PORTH, ~pmc_ph_sel & IO_PORTH_MASK);
        pmc_io_port_migrate(PMC_PORTP, JL_PORTP, ~pmc_pp_sel & IO_PORTP_MASK);
    } else {
        pmc_io_port_restore(PMC_PORTD, ~pmc_pd_sel & IO_PORTD_MASK);
        pmc_io_port_restore(PMC_PORTE, ~pmc_pe_sel & IO_PORTE_MASK);
        pmc_io_port_restore(PMC_PORTH, ~pmc_ph_sel & IO_PORTH_MASK);
        pmc_io_port_restore(PMC_PORTP, ~pmc_pp_sel & IO_PORTP_MASK);
    }

    ipc_spin_unlock(IPC_SPIN_LOCK_EVENT_P11_GPIO);
}

void sleep_enter_callback()
{
    pmc_io_control_sw(1);
    board_sleep_enter_callback();
}

void sleep_exit_callback()
{
    pmc_io_control_sw(0);
    board_sleep_exit_callback();
}

//maskrom 使用到的io
static void __mask_io_cfg()
{
    struct app_soft_flag_t app_soft_flag = {0};

    mask_softflag_config(&app_soft_flag);
}

void msys_soff_to_p11_msg(void)
{
    u8 msg[3];
    msg[0] = MSG_P11_SOFF_EVENT;
    m2p_post_msg(MSG_APP, 1, (u8 *)msg, sizeof(msg));
}

u8 power_soff_callback()
{
#if TCFG_APP_RTC_EN
    rtc_1hz_wkup_switch(0);

#if RTC_TRIM_WKUP_TIME
    rtc_lptmr_wakeup_enable(RTC_TRIM_WKUP_TIME);
#endif

#endif

    DO_PLATFORM_UNINITCALL();

    board_poweroff_uninit();

    msys_soff_to_p11_msg();

    return 0;
}

void gpio_config_soft_poweroff(void);
void board_set_soft_poweroff()
{
    __mask_io_cfg();

    gpio_config_soft_poweroff();

    gpio_config_uninit();

}

void power_early_flowing()
{
    /*默认把低功耗部分代码加载到power overlay段*/
    lowpower_init();

    PORT_TABLE(g);

#if defined(CONFIG_UPDATE_JUMP_TO_MASK) && CONFIG_UPDATE_JUMP_TO_MASK
    u16 update_result = update_flag_get();
    if (update_result != UPDATA_NON && update_result != 0) {
        latch_io_set_protect(gpio_config, STRCHANGE(CONFIG_SD_LATCH_IO));
    }
#endif

    /*init_boot_rom();*/

    printf("get_boot_rom(): %d", get_boot_rom());

    // 默认关闭长按复位0，由key_driver配置
    gpio_longpress_pin0_reset_config(PINR_DEFAULT_IO, 0, 0, 1, 1, 0);
    //长按复位1默认配置2s，写保护
#if TCFG_CHARGE_ENABLE
    gpio_longpress_pin1_reset_config(IO_LDOIN_DET, 1, 2, 1);
#endif

#if CONFIG_UART_DEBUG_ENABLE
    PORT_PROTECT(CONFIG_UART_DEBUG_PORT);
#endif

    power_early_init((u32)gpio_config);
}

//early_initcall(_power_early_init);

static int power_later_flowing()
{
    pmu_trim(0, 0);

    power_later_init(0);

#if TCFG_APP_RTC_EN
    /* if (rtc_is_alarm_wkup() == RTC_OVERFLOW_WKUP) { */
    /* rtc_debug_dump(); */
    /* power_set_soft_poweroff(); */
    /* } */
#endif

    return 0;
}

late_initcall(power_later_flowing);
