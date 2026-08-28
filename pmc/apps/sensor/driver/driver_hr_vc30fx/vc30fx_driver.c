#include "sensor_driver.h"
#include "vc30fx_driver.h"
#include "vc30fx_config.h"
#include "vcare_device_com.h"
#include "vc30fx_user_app.h"
#include "vc30fx_sample_result.h"

#if TCFG_VC30fx_ENABLE
// 设备信息
static sensor_info_t vc30fx_info = {
    .type = SENSOR_DRV_HR,
    .state = SENSOR_STATE_OFFLINE,
    .name = "vc30fx",
    .odr = {25, 50, 100, 0},      // 支持的采样率(Hz)
    .range = {1, 2, 4, 0},        // 支持的量程
    .cbuffer = NULL,
};

// 设备状态
static u8 vc30fx_dev_opened = 0;
static vc30fx_config_t vc30fx_cfg = {0};
static vc30fx_raw_data_t vc30fx_raw_data = {0};

// 获取设备信息
static sensor_info_t *vc30fx_driver_info(void)
{
    return &vc30fx_info;
}

// 打开设备
static s8 vc30fx_driver_open(u16 range, u16 odr)
{
    if (vc30fx_dev_opened) {
        return RET_OK;  // 已打开
    }

    // 初始化配置参数
    vc30fx_cfg.work_mode = WORK_MODE_HR;
    vc30fx_cfg.ppg_freq = 20000;    // 20KHz
    vc30fx_cfg.ps_freq_div = 4;     // PS频率 = PPG频率/4
    vc30fx_cfg.fifo_freq_div = 1;   // FIFO频率 = PS频率

    // 根据odr调整分频系数
    if (odr == 25) {
        vc30fx_cfg.fifo_freq_div = 1;
    } else if (odr == 50) {
        vc30fx_cfg.fifo_freq_div = 2;
    } else if (odr == 100) {
        vc30fx_cfg.fifo_freq_div = 4;
    }

    // 设置默认电流和电阻参数
    vc30fx_cfg.slot0_current = 0x40;
    vc30fx_cfg.slot0_resistance = 9;
    vc30fx_cfg.slot1_current = 0x40;
    vc30fx_cfg.slot1_resistance = 9;
    vc30fx_cfg.slot2_current = 0x40;
    vc30fx_cfg.slot2_resistance = 9;

    // 调用原有初始化接口
    InitParamTypeDef init_param = {
        .sampling_gap_time = 1000 / odr,
        .work_mode = WORK_MODE_HR,
    };
    vc30fx_usr_device_init(&init_param);

    // 启动工作
    vc30fx_usr_start_work();

    vc30fx_dev_opened = 1;
    vc30fx_info.state = SENSOR_STATE_OPEN;

    return RET_OK;
}

// 关闭设备
static s8 vc30fx_driver_close(void)
{
    if (!vc30fx_dev_opened) {
        return RET_OK;  // 已关闭
    }

    // 调用原有停止接口
    vc30fx_usr_stop_work();

    vc30fx_dev_opened = 0;
    vc30fx_info.state = SENSOR_STATE_CLOSE;

    return RET_OK;
}

// 运行/采集数据
static s8 vc30fx_driver_run(void *arg, u16 *len)
{
    if (!vc30fx_dev_opened) {
        return RET_ERR;
    }

    // 获取原始数据
    u16 data_len = vc30fx_driver_get_raw_data(&vc30fx_raw_data);
    if (data_len > 0 && arg != NULL) {
        // 拷贝数据到输出缓冲区
        vc30fx_raw_data_t *output = (vc30fx_raw_data_t *)arg;
        *output = vc30fx_raw_data;
        *len = data_len;
        return RET_OK;
    }

    return RET_ERR;
}

// 设备在线检测
static s8 vc30fx_driver_online(void)
{
    // 尝试读取设备ID
    extern u8 vc30fx_online(void);
    u8 online =   vc30fx_online();
    if (online != 0) {
        vc30fx_info.state = SENSOR_STATE_ONLINE;
        return RET_OK;
    }

    vc30fx_info.state = SENSOR_STATE_OFFLINE;
    return RET_ERR;
}

// 设备休眠
static s8 vc30fx_driver_sleep(void)
{
    if (!vc30fx_dev_opened) {
        return RET_OK;
    }

    // 调用原有停止接口
    vc30fx_usr_stop_work();

    return RET_OK;
}

// 获取原始数据
u16 vc30fx_driver_get_raw_data(vc30fx_raw_data_t *data)
{
#if !PMC_DEVELOP_ENABLE
    if (data == NULL) {
        return 0;
    }

    // 获取设备对象
    vcare_ppg_device_t *pdev = get_vc30fx_device();
    if (pdev == NULL || pdev->result == NULL) {
        return 0;
    }

    // 从设备对象中获取采样结果
    vc30fx_sample_info_t *sample_info = (vc30fx_sample_info_t *)pdev->result;

    // 填充PPG数据
    for (int slot = 0; slot < 3; slot++) {
        if (sample_info->slot_result[slot].slot_enwork) {
            data->ppg_num = sample_info->slot_result[slot].ppg_num;
            for (int i = 0; i < data->ppg_num && i < 64; i++) {
                data->ppg_data[slot][i] = sample_info->slot_result[slot].ppg_buffer[i];
            }
            data->ps_data[slot] = sample_info->slot_result[slot].u.ps_data;
        }
    }

    // 填充BIO数据
    data->bio_data[0] = sample_info->extra_result.bioinn_data;
    data->bio_data[1] = sample_info->extra_result.bioext_data;

    // 填充温度数据
    data->temp_data[0] = sample_info->extra_result.tempinn_data;
    data->temp_data[1] = sample_info->extra_result.tempext_data;
    data->temp_data[2] = sample_info->extra_result.tempref_data;

    // 填充佩戴状态
    data->wear_status = vc30fx_driver_get_wear_status();
#endif
    return sizeof(vc30fx_raw_data_t);
}

// 获取佩戴状态
u8 vc30fx_driver_get_wear_status(void)
{
    return vc30fx_usr_get_wear_status();
}

// 中断处理函数
void vc30fx_driver_irq_handler(void)
{
    if (!vc30fx_dev_opened) {
        return;
    }

    // 调用原有的中断处理函数
    vc30fx_usr_device_handler(0, 1);
}

// 注册VC30FX驱动
REGISTER_SENSOR(vc30fx_sensor) = {
    .info = &vc30fx_info,
    .open = vc30fx_driver_open,
    .close = vc30fx_driver_close,
    .run = vc30fx_driver_run,
    .online = vc30fx_driver_online,
    .sleep = vc30fx_driver_sleep,
};



#endif//TCFG_VC30fx_ENABLE
