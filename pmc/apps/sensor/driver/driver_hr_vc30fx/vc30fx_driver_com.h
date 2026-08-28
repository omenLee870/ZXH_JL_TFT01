
/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2023-08-03 16:09:24
 * @LastEditTime: 2023-09-18 11:22:33
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/

#ifndef __VC30Fx_DRIVER_COM_H__
#define __VC30Fx_DRIVER_COM_H__

#include "vcare_device_com.h"
/*********************************************************************************************************
 * DRV_API
 ********************************************************************************************************/
/* 注意：
VC30FS - fifo长度最长只有128字节，在多通道时，一次fifo的缓存不能超过128字节
VC30FC - fifo长度最长只有 96字节，在多通道时，一次fifo的缓存不能超过 96字节

typical:
ppg_div=800, ps_div=5,  fifo_div=4  (ppg-25hz, psbio-5hz, fifo-1.25hz)    800ms (30FS_max=20*4+4*6=104, 30FC_max=20*4+4*2=88)
ppg_div=400, ps_div=10, fifo_div=2  (ppg-50hz, psbio-5hz, fifo-2.5hz)     400ms (30FS_max=20*4+2*6=92,  30FC_max=20*4+2*2=84)
ppg_div=200, ps_div=20, fifo_div=1  (ppg-100hz,psbio-5hz, fifo-5hz)       200ms (30FS_max=20*4+1*6=88,  30FC_max=20*4+1*2=82)
--------------------------------------------------------------------
ppg_div=800, ps_div=2, fifo_div=5   (ppg-25hz, psbio-12.5hz,  fifo-2.5hz) 400ms (30FS_max=10*4+5*6=70, 30FC_max=10*4+5*2=50)
ppg_div=400, ps_div=4, fifo_div=2   (ppg-50hz, psbio-12.5hz, fifo-6.25hz) 160ms (30FS_max=8*4+2*6=44, 30FC_max=8*4+2*2=36)
ppg_div=200, ps_div=8, fifo_div=2   (ppg-100hz,psbio-12.5hz, fifo-6.25hz) 160ms (30FS_max=16*4+2*6=76, 30FC_max=16*4+2*2=68)
*/
/****************************************************************************
 * @description: 初始化并且开始进行采样
 * @param {void} *pdevice
 * @param {int} workmode
 * @param {unsigned short int} ppgdiv
 * @param {unsigned char} psdiv
 * @param {unsigned char} fifodiv
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_init_start_work(void *pdevice, int workmode, unsigned short int ppgdiv, unsigned char psdiv, unsigned char fifodiv);
/****************************************************************************
 * @description: 获取驱动版本，IC的主ID，次ID
 * @param {char} *
 * @param {unsigned char} *pid1
 * @param {unsigned char} *pid2
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_get_hwsw_infomation(char **str_drv_ver, unsigned char *pid1, unsigned char *pid2);
/****************************************************************************
 * @description: 软复位IC并且退出工作（如需重新工作需要重新调用初始化）
 * @param {void} *device
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_exit_stop_work(void *device);
/****************************************************************************
 * @description: 软复位IC（如需重新工作需要重新调用初始化）
 * @param {void} *device
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_software_reset(void *device);
/****************************************************************************
 * @description: 唤醒工作（可以唤醒调用sleep停止工作）
 * @param {void} *device
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_wakeup_work(void *device);
/****************************************************************************
 * @description: 休眠工作（休眠后可以用wakeup唤醒工作）
 * @param {void} *device
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_sleep_work(void *device);
/****************************************************************************
 * @description: 心率算法扩展的摘下状态设置
 * @param {void} *device
 * @param {int} status
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_set_algo_wear_status(void *device, int status);
/****************************************************************************
 * @description: 运动量判断佩戴的状态设置
 * @param {void} *device
 * @param {int} status
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_set_acce_wear_status(void *device, int status);
/****************************************************************************
 * @description: 获取参数并且进行佩戴状态判断
 * @param {void} *device
 * @return {*}
 ****************************************************************************/
wear_status vc30fx_drv_get_wear_status(void *device);
/****************************************************************************
 * @description: 获取具体的佩戴状态各子状态
 * @param {unsigned char} *bio_ret
 * @param {unsigned char} *algo_ret
 * @param {unsigned char} *acce_ret
 * @return {*}
 ****************************************************************************/
void vc30fx_drv_get_wear_detail_result(unsigned char *bio_ret, unsigned char *algo_ret, unsigned char *acce_ret);
/****************************************************************************
 * @description: 获取采样数据,包含PPG,PS,BIO，TMEP
 * @param {void} *device
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_get_result_handler(void *device);
/****************************************************************************
 * @description: 是否为fifo中断事件
 * @param {void} *pdevice
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_is_fifo_event(void *pdevice);
/****************************************************************************
 * @description: 是否为PS中断事件
 * @param {void} *pdevice
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_is_ps_event(void *pdevice);
/****************************************************************************
 * @description: 是否为PPG中断事件
 * @param {void} *pdevice
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_is_ppg_event(void *pdevice);
/****************************************************************************
 * @description: 配平校准活体
 * @param {void} *pdevice
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_clear_reset_bio_param(void *pdevice);
/****************************************************************************
 * @description: 同步驱动状态，清理状态数据
 * @param {void} *pdevice
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_finished_sync(void *pdevice);
/****************************************************************************
 * @description: 提前读取中断信息，避免任务延时响应导致信息被刷掉
 * @param  {*}
 * @return {*}
 ****************************************************************************/
void vc30fx_drv_read_event_infomation_byIRQ(void);
/****************************************************************************
 * @description: 软件定时器读取RO信息，用于定时温度检测
 * @param  {*}
 * @return {*}
 ****************************************************************************/
int vc30fx_drv_read_check_temperature_abnormal(void *pdevice, unsigned char *p_freq_ret, unsigned char *p_temp_ret, unsigned short *pdev_cnt1, unsigned short *pdev_cnt2);

int vc30fx_drv_memroy_alloc(void);
int vc30fx_drv_memroy_free(void);

#endif /* __DRV_DEV_MANAGER_H__ */
