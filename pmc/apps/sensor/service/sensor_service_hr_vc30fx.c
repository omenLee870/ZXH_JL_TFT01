#include "health_manager.h"
#include "sensor_hub.h"
#include "driver/driver_hr_vc30fx/vcare_device_com.h"


#if TCFG_VC30fx_ENABLE
int sensor_servvice_hr_enable(int enable)
{
    extern void vc30fxStart(u8 work_mode);
    extern void vc30fx_usr_stop_work(void);
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
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        int *hr = priv;
        vc30fx_irq_handler();
        *hr = (int)get_hrs_dev_hr_valus();
        break;
    default:
        break;
    }
    return 0;
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
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        int *hr = priv;
        vc30fx_irq_handler();
        *hr = (int)get_hrs_dev_spo2_valus();
        break;
    default:
        break;
    }
    return 0;
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
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        int *hr = priv;
        vc30fx_irq_handler();
        *hr = (int)get_hrs_dev_wear_valus();
        break;
    default:
        break;
    }
    return 0;
}
REGISTER_SENSOR_SERVICE(wear_service)
{
    .mod = SHM_MOD_WEAR_DETECTION,
     .init = NULL,
      .enable =  sensor_servvice_spo2_enable,
       .get_result = sensor_service_spo2_get,
        .set_config = NULL,
};





#endif
