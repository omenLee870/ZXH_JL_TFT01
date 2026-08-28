
/* #include "vc30fx_hal.h" */
#include "sensor_driver.h"
#include "sensor_hub.h"

#if TCFG_VC30fx_ENABLE
// HAL层状态
static u8 vc30fx_hal_inited = 0;
static u8 vc30fx_hal_enabled = 0;

// 获取设备信息
sensor_info_t *vc30fx_hal_info(void)
{
    return sensor_driver_info_get(SENSOR_DRV_HR);
}

// 初始化设备
s8 vc30fx_hal_init(u8 enable)
{
    if (enable) {
        if (!vc30fx_hal_inited) {
            // 初始化Driver层
            sensor_driver_init(SENSOR_DRV_HR, 1, 1, 25);
            vc30fx_hal_inited = 1;
        }
        vc30fx_hal_enabled = 1;
    } else {
        if (vc30fx_hal_inited) {
            // 关闭Driver层
            sensor_driver_init(SENSOR_DRV_HR, 0, 0, 0);
            vc30fx_hal_inited = 0;
        }
        vc30fx_hal_enabled = 0;
    }

    return RET_OK;
}

// 缓冲区管理
s8 vc30fx_hal_cbuf(u8 index, u8 enable)
{
    // 调用Driver层的缓冲区管理
    // 这里可以根据需要实现多缓冲区管理
    return sensor_hub_cbuf_enable(SENSOR_DRV_HR, index, enable);
}

// 获取数据
u16 vc30fx_hal_get(u8 index, void *buff, u16 buff_len)
{
    if (!vc30fx_hal_enabled || buff == NULL || buff_len == 0) {
        return 0;
    }

    // 从Driver层获取数据
    return sensor_driver_data_get(SENSOR_DRV_HR, buff, buff_len);
}

// HAL层运行(中断中调用)
void vc30fx_hal_run(void)
{
    if (!vc30fx_hal_enabled) {
        return;
    }
#if !PMC_DEVELOP_ENABLE
    // 调用Driver层中断处理
    vc30fx_driver_irq_handler();
#endif
}

// 获取设备状态
sensor_state_t vc30fx_hal_get_state(u8 type)
{
    return sensor_hub_get_sensor_state(type);
}

// 注册VC30FX HAL
REGISTER_SENSOR_HAL(vc30fx_hal) = {
    .dev_type = SENSOR_DRV_HR,
    .dev_info = vc30fx_hal_info,
    .dev_init = vc30fx_hal_init,
    .dev_cbuf = vc30fx_hal_cbuf,
    .dev_get = vc30fx_hal_get,
};


#endif//TCFG_VC30fx_ENABLE
