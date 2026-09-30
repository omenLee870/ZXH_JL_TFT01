/**@file        descriptor.h
  * @brief      各种描述符头文件
  * @details    结构体声明，功能函数声明
  * @author     jieli
  * @date       2021-9-1
  * @version    V1.0
  * @copyright  Copyright(c)2010-2021   珠海市杰理科技股份有限公司
  *********************************************************
  * @attention
  * 硬件平台：AC632N
  * SDK版本：AC632N_V1.0.0_SDK
  * @修改日志：
  * <table>
  * <tr><th>Date        <th>Version     <th>Author      <th>Description
  * <tr><td>2021-9-1    <td>1.0         <td>jieli       <td>创建初始版本
  * </table>
  *
  *********************************************************
  */
#ifndef  __DESCRIPTOR_H__
#define  __DESCRIPTOR_H__

#include "usb/usb.h"

/**@brief USB获取设备描述符
  * @param[in]  *ptr  存放设备描述符的地址
  * @return     无
  * @par    示例：
  * @code
  * get_device_descriptor(ptr);
  * @endcode
  */
void get_device_descriptor(u8 *ptr);

/**@brief 获取设备描述符使用的 VID
  * @return     VID（编译期由 TCFG_USB_VID 决定，未配置时取默认值）
  */
u16 usb_get_vid(void);

/**@brief 获取从机模式编译期使能的 class 位图
  * @return     class 位图，与 usb_start() 下发给 usb_device_mode() 的一致
  * @par    示例：
  * @code
  * u16 class = usb_slave_class_config();
  * @endcode
  */
u16 usb_slave_class_config(void);

/**@brief 按 class 位图推算 PID
  * @param[in]  class_config  class 位图，低 8 位有效
  * @return     PID，未置起任何 class 位时为基准 PID
  * @par    示例：
  * @code
  * u16 pid = usb_pid_from_class(usb_slave_class_config());
  * @endcode
  */
u16 usb_pid_from_class(u16 class_config);


/**@brief USB获取***字符串描述符
  * @param[in]  *ptr  存放***字符串描述符的地址
  * @return     无
  * @par    示例：
  * @code
  * get_string_ee(ptr);
  * @endcode
  */
void get_string_ee(u8 *ptr);

/**@brief USB设置描述符
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  class_config  类配置
  * @param[in]  *p  存放描述符的地址
  * @param[in]  max_len  最大长度
  * @return
  * @par    示例：
  * @code
  * set_descriptor(usb_id,class_config,p,max_len);
  * @endcode
  */
u32 set_descriptor(const usb_dev usb_id, u32 class_config, u8 *p, u32 max_len);

u32 descriptor_string_cb(u32 index, u16 langid, u8 *payload);

#endif  /*DESCRIPTOR_H*/
