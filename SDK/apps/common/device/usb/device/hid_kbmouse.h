/**@file        hid_kbmouse.h
  * @brief      标准键鼠 HID 从机驱动头文件
  * @details    一个可参数化的标准 HID 接口实现：报告描述符、子类/协议、端点对、
  *             包长、bInterval、bcdHID 全部由宏配置，便于复用到其他产品。
  *             当前实例化为「Boot Mouse + Report ID 2 键盘」的复合描述符。
  */
#ifndef __USB_DEVICE_HID_KBMOUSE_H__
#define __USB_DEVICE_HID_KBMOUSE_H__

#include "typedef.h"
#include "usb/usb.h"
#include "usb/device/usb_stack.h"

/* 报文长度（含 Report ID）：
 * 鼠标 5 字节 = ID + 按键位 + X + Y + Wheel
 * 键盘 9 字节 = ID + 修饰键 + 保留 1 字节 + 6 个键码 */
#define HID_KBMOUSE_REPORT_LEN      5
#define HID_KBMOUSE_KEYBOARD_LEN    9
#define HID_KBMOUSE_KEYCODE_MAX     6

#define HID_KBMOUSE_REPORT_MOUSE    0x01
#define HID_KBMOUSE_REPORT_KEYBOARD 0x02

/**@brief   上报一包原始报文（首字节为 Report ID）
  * @param[in]  usb_id usb 的 id 号
  * @param[in]  *report 报文内容
  * @param[in]  len 报文长度，超过端点包长会被截断
  * @return 实际写入的长度，0 表示未就绪
  */
u32 hid_kbmouse_report(const usb_dev usb_id, const u8 *report, u32 len);

/**@brief   上报一包鼠标报文
  * @param[in]  usb_id usb 的 id 号
  * @param[in]  buttons 按键位图（bit0 左键、bit1 右键、bit2 中键）
  * @param[in]  x X 位移，-127~127
  * @param[in]  y Y 位移，-127~127
  * @param[in]  wheel 滚轮，-127~127
  * @return 实际写入的长度，0 表示未就绪
  */
u32 hid_kbmouse_report_mouse(const usb_dev usb_id, u8 buttons, s8 x, s8 y, s8 wheel);

/**@brief   上报一包键盘报文
  * @param[in]  usb_id usb 的 id 号
  * @param[in]  modifier 修饰键位图（bit0 LCtrl ... bit7 RGUI）
  * @param[in]  *keycode 最多 HID_KBMOUSE_KEYCODE_MAX 个按键码，不足部分用 0 填充
  * @return 实际写入的长度，0 表示未就绪
  */
u32 hid_kbmouse_report_keyboard(const usb_dev usb_id, u8 modifier, const u8 *keycode);

/**@brief   主机是否已开始轮询本接口
  * @return 1 = 已就绪，0 = 未就绪
  */
u8 hid_kbmouse_is_ready(void);

/* 以下三个供 USB_CLASS_REGISTER 引用 */
u32  hid_kbmouse_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num);
u32  hid_kbmouse_register(const usb_dev usb_id);
void hid_kbmouse_release(const usb_dev usb_id);

#endif  /* __USB_DEVICE_HID_KBMOUSE_H__ */
