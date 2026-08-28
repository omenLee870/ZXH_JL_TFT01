#include "includes.h"
#include "sdk_config.h"
#include "bsp.h"
#include "init.h"

#define _IIC_USE_HW
#include "iic_api.h"
#include "lpctmu.h"

const struct lpctmu_platform_data lpctmu_data = {
    .clk_sel = LPCTMU_STRC_32K,
    .sample_window_time = 2,
    .sample_scan_time = 20,
    .aim_vol_delta = 4,//800
    .aim_charge_khz = 2500,
    .ch_en.cfg.ch0 = LPTOUCH_CH0_ENABLE,
    .ch_en.cfg.ch1 = LPTOUCH_CH1_ENABLE,
    .ch_en.cfg.ch2 = LPTOUCH_CH2_ENABLE,
    .ch_en.cfg.ch3 = LPTOUCH_CH3_ENABLE,
    .ch_en.cfg.ch4 = LPTOUCH_CH4_ENABLE,
    .ch_en.cfg.ch5 = LPTOUCH_CH5_ENABLE,
    .ch_en.cfg.ch6 = LPTOUCH_CH6_ENABLE,
    .ch_en.cfg.ch7 = LPTOUCH_CH7_ENABLE,
    .wkp_en = 0,
    .wkp_ch = 0,
    .sf_keep = 0,
};


const struct iic_master_config hw_iic_cfg_const[MAX_HW_IIC_NUM] = {
    {
        .role = IIC_MASTER,
        .scl_io = PMC_HW_IIC_SCL,
        .sda_io = PMC_HW_IIC_SDA,
        .io_mode = PMC_HW_IIC_PU_EN	,//1:上拉或0:浮空
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,   //enum GPIO_HDRIVE 0:2.4MA, 1:8MA, 2:26.4MA, 3:40MA
        .master_frequency = PMC_HW_IIC_FREQ	, //软件iic频率不准(hz)
        .io_filter = 1,  //软件无效
        .ie_en = 0,//1:注册中断
        .irq_priority = 3,//优先级
    },//iic0
};



void bsp_init()
{
#if PMC_HW_IIC_EN
    /* system_iic_init(HW_IIC_0,get_hw_iic_config(HW_IIC_0)); */
    extern void hw_iic_master_module_init();
    hw_iic_master_module_init();
    /* iic_api_init(); */
#endif//P11_HW_IIC_EN
}


#if TCFG_LPTOUCH_ENABLE
int lptouch_init(void)
{
    lpctmu_init(&lpctmu_data);
    return 0;
}
module_initcall(lptouch_init);
#endif

