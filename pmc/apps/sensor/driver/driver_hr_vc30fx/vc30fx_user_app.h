
/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2023-02-15 10:55:30
 * @LastEditTime: 2023-08-21 17:24:07
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
#ifndef __VC30Fx_USER_APP_H__
#define __VC30Fx_USER_APP_H__

#include "vcare_device_com.h"

/* vc931x设备函数接口 */
void vc30fx_usr_device_init(InitParamTypeDef *pinitconfig);
void vc30fx_usr_get_dev_info(unsigned char *pchip_id, unsigned char *pvers_id, char **drv_name, char **dev_name);
void vc30fx_usr_start_work(void);
void vc30fx_usr_stop_work(void);
void vc30fx_usr_soft_reset(void);
void vc30fx_usr_reset_bio_param(void);
int vc30fx_usr_get_wear_status(void);
void vc30fx_usr_device_handler(unsigned char heart_algo_mode, unsigned char spo2_algo_mode);
void vc30fx_usr_check_temperature_abnormal(void);

/* 获取vc931x设备/如果动态内存则会申请驱动内存，以下为获取的设备API，设备对象中会记录一部分计算参数和结果，用户需要直接使用即可，具体参考头文件"vcare_device_com.h"：
{
    .dev_init = vc30fx_usr_device_init,
    .get_dev_info = vc30fx_usr_get_dev_info,
    .start_work = vc30fx_usr_start_work,
    .stop_work = vc30fx_usr_stop_work,
    .soft_reset = vc30fx_usr_soft_reset,
    .get_events = NULL,
    .set_slot_enable = NULL,
    .reset_bio_param = vc30fx_usr_reset_bio_param,
    .get_wear_status = vc30fx_usr_get_wear_status,
    .dev_irq_handler = vc30fx_usr_device_handler,
};
*/
vcare_ppg_device_t *get_vc30fx_device(void);

#endif // !
