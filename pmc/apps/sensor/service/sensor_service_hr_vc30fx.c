#include "health_manager.h"
#include "sensor_hub.h"
#include "driver/driver_hr_vc30fx/vcare_device_com.h"

#define LOG_TAG_CONST       SENSOR_SERVICE
#define LOG_TAG     		"[vc30fx_svc]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define __DEBUG
#include "log.h"

#if TCFG_SENSOR_HR_CODE_IN_RAM
#pragma bss_seg(".sensor_bss.L1")
#pragma data_seg(".sensor_data.L1")
#pragma code_seg(".sensor_code.L1")
#pragma const_seg(".sensor_const")
#else
#pragma bss_seg(".sensor_bss.L2")
#pragma data_seg(".sensor_data.L2")
#pragma code_seg(".sensor_code.L2")
#pragma const_seg(".sensor_const")
#endif

#if TCFG_VC30fx_ENABLE
int sensor_servvice_hr_enable(int enable)
{
    extern void vc30fxStart(u8 work_mode);
    extern void vc30fx_usr_stop_work(void);
    log_info("%s enable=%d", __func__, enable);
    if (enable) {
        vc30fxStart(WORK_MODE_HR);
    } else {
        vc30fx_usr_stop_work();
    }
    return 0;
}
int sensor_service_hr_get(int type, void *priv)
{
    extern u8 get_hrs_dev_hr_valus(void);
    extern u8 vc30fx_irq_handler();
    int ret = 0;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        vc30fx_irq_handler();
        /* 消费方按 4 字节读取，必须写满 */
        {
            struct heart_data *hr = (struct heart_data *)priv;
            hr->real = get_hrs_dev_hr_valus();
            hr->max  = 0;
            hr->min  = 0;
            hr->avg  = 0;
        }
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}
REGISTER_SENSOR_SERVICE(hr_service)
{
    .mod = SHM_MOD_HEART_RATE_ALGO,
    .init = NULL,
    .enable =  sensor_servvice_hr_enable,
    .get_result = sensor_service_hr_get,
    .set_config = NULL,
};
int sensor_servvice_spo2_enable(int enable)
{
    extern void vc30fxStart(u8 work_mode);
    extern void vc30fx_usr_stop_work(void);
    log_info("%s enable=%d", __func__, enable);
    if (enable) {
        vc30fxStart(WORK_MODE_SPO2);
    } else {
        vc30fx_usr_stop_work();
    }
    return 0;
}
int sensor_service_spo2_get(int type, void *priv)
{
    extern u8 get_hrs_dev_spo2_valus(void);
    extern u8 vc30fx_irq_handler();
    int ret = 0;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        vc30fx_irq_handler();
        /* 同 hr，必须写满 4 字节 */
        {
            struct spo2_data *spo2 = (struct spo2_data *)priv;
            spo2->real = get_hrs_dev_spo2_valus();
            spo2->max  = 0;
            spo2->min  = 0;
            spo2->avg  = 0;
        }
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}
REGISTER_SENSOR_SERVICE(spo2_service)
{
    .mod = SHM_MOD_SPO2_ALGO,
    .init = NULL,
    .enable =  sensor_servvice_spo2_enable,
    .get_result = sensor_service_spo2_get,
    .set_config = NULL,
};
int sensor_servvice_wear_enable(int enable)
{
    extern void vc30fxStart(u8 work_mode);
    extern void vc30fx_usr_stop_work(void);
    if (enable) {
        vc30fxStart(WORK_MODE_WEAR);
    } else {
        vc30fx_usr_stop_work();
    }
    return 0;
}
int sensor_service_wear_get(int type, void *priv)
{
    extern u8 get_hrs_dev_wear_valus(void);
    extern u8 vc30fx_irq_handler();
    int ret = 0;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        vc30fx_irq_handler();
        /* wear 无专用结构体，消费方按 4 字节 int 读取，必须写满 */
        *(int *)priv = (int)get_hrs_dev_wear_valus();
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}
REGISTER_SENSOR_SERVICE(wear_service)
{
    .mod = SHM_MOD_WEAR_DETECTION,
    .init = NULL,
    .enable =  sensor_servvice_wear_enable,
    .get_result = sensor_service_wear_get,
    .set_config = NULL,
};





#endif
