/**@file        xbox360.h
  * @brief      Xbox360 手柄（XInput）从机驱动头文件
  * @details    把设备枚举成 Xbox360 兼容手柄：IF0 为 XID 厂商接口（FF/5D/01），
  *             IF1/IF3 为厂商自定义 HID 接口（耳机 / 聊天板），并配合 MS OS
  *             描述符里的 CompatibleID "XUSB10" 让 Windows 加载 xusb22.sys。
  * @note       本驱动独占 EP1~EP4，不能与 UAC/MSD/HID 等从机 class 同时使能。
  */
#ifndef __USB_DEVICE_XBOX360_H__
#define __USB_DEVICE_XBOX360_H__

#include "typedef.h"
#include "usb/usb.h"
#include "usb/device/usb_stack.h"

/* 输入报文（EP0x81 IN）固定 20 字节，标准 XInput gamepad 布局 */
#define XBOX360_INPUT_REPORT_LEN    20
/* 输出报文（EP0x01 OUT）3 字节：buf[0] 为命令码 */
#define XBOX360_OUTPUT_REPORT_LEN   3

/* 输出报文命令码 */
#define XBOX360_OUT_LED             0x01
#define XBOX360_OUT_RUMBLE          0x02

/**@brief   XInput 输入报文里的摇杆/扳机/按键字段
  * @note    buttons 位定义未做封装，直接按 XInput 原始位图填入即可
  */
struct xbox360_gamepad {
    u16 buttons;    /* 报文 [2] | [3] << 8 */
    u8  lt;         /* 左扳机 0~255 */
    u8  rt;         /* 右扳机 0~255 */
    s16 lx;         /* 左摇杆 X，-32768~32767 */
    s16 ly;
    s16 rx;         /* 右摇杆 X */
    s16 ry;
};

/**@brief   3 字节输出报文的原始字段
  * @note    字段语义尚未完全确定（抓包样本不足），故只透传原始字节：
  *          LED 命令（type=0x01）的 b2 是 LED 状态；
  *          震动命令（type=0x02）的 b1/b2 分别是左右马达强度。
  */
typedef struct {
    u8 type;    /* 命令码：XBOX360_OUT_LED / XBOX360_OUT_RUMBLE */
    u8 b1;
    u8 b2;
} xbox360_output_t;

/**@brief   输出报文（LED / 震动）回调，在 USB 中断上下文中执行
  * @param[in]  priv  注册时传入的私有指针
  * @param[in]  if_idx 接口序号（0 = XID 接口）
  * @param[in]  buf  原始报文
  * @param[in]  len  报文长度
  */
typedef void (*xbox360_output_cb_t)(void *priv, u8 if_idx, const u8 *buf, u32 len);

/**@brief   每帧采样回调，在 EP1 的 TX 完成中断里、发包之前调用，
  *          用来把最新的按键/摇杆状态填进本帧报文。
  * @note    回调执行在 USB 中断上下文：只做取数与换算，不要阻塞、
  *          不要调用 CPU_CRITICAL_ENTER/EXIT（br33 上是裸的
  *          local_irq_enable，在中断里跑会提前开中断）。
  *          pad 已由驱动清零，未填的字段即中立值。
  * @param[in]  priv  注册时传入的私有指针
  * @param[out] pad 本帧要发送的手柄字段
  */
typedef void (*xbox360_sample_cb_t)(void *priv, struct xbox360_gamepad *pad);

/**@brief   切换到 Xbox360 手柄模式（会先停掉当前 USB 设备模式）
  * @param[in]  usb_id usb 的 id 号
  * @return 无
  */
void usb_xbox360_start(const usb_dev usb_id);

/**@brief   退出 Xbox360 手柄模式
  * @param[in]  usb_id usb 的 id 号
  * @return 无
  */
void usb_xbox360_stop(const usb_dev usb_id);

/**@brief   查询主机是否已开始轮询手柄端点（首包 IN 传输完成）
  * @return 1 = 已就绪，0 = 未就绪
  */
u8 xbox360_is_ready(void);

/**@brief   上报一包原始输入报文
  * @note    IF0 的输入端点是"自维持上报流"：驱动在端点 TX 完成中断里持续补发
  *          当前报文，主机每次轮询都能拿到数据（与真实手柄的总线行为一致）。
  *          本函数只更新要发的内容，不直接操作端点，通常返回 len。
  *          从任务或定时器里调用是安全的，可在中断里调用。
  * @param[in]  usb_id usb 的 id 号
  * @param[in]  *report 报文内容，长度应为 XBOX360_INPUT_REPORT_LEN
  * @param[in]  len 报文长度，超过端点包长会被截断
  * @return 已接受的长度，0 表示未就绪
  */
u32 xbox360_report_input(const usb_dev usb_id, const u8 *report, u32 len);

/**@brief   按字段构造并上报一包 XInput 输入报文
  * @param[in]  usb_id usb 的 id 号
  * @param[in]  *pad 手柄字段
  * @return 已接受的长度，0 表示未就绪
  */
u32 xbox360_report_gamepad(const usb_dev usb_id, const struct xbox360_gamepad *pad);

/**@brief   往指定接口的 IN 端点上报数据
  * @note    if_idx = 0（XID 接口）与 xbox360_report_input 等价；
  *          if_idx = 1/2（两个厂商 HID 接口）为按需直接发送，不自维持。
  * @param[in]  usb_id usb 的 id 号
  * @param[in]  if_idx 接口序号（0/1/2）
  * @param[in]  *buf 数据
  * @param[in]  len 数据长度
  * @return 已接受/写入的长度，0 表示参数非法或未就绪
  */
u32 xbox360_report_input_if(const usb_dev usb_id, u8 if_idx, const u8 *buf, u32 len);

/**@brief   注册输出报文（LED / 震动）回调
  * @param[in]  priv 私有指针，回调时原样回传
  * @param[in]  cb 回调，传 NULL 取消注册
  * @return 无
  */
void xbox360_set_output_cb(void *priv, xbox360_output_cb_t cb);

/**@brief   注册每帧采样回调（用于接真实按键/摇杆）
  * @note    驱动每发一包前都会调用它取最新状态；不注册则沿用
  *          xbox360_report_input() 最近一次写入的内容。
  * @param[in]  priv 私有指针，回调时原样回传
  * @param[in]  cb 回调，传 NULL 取消注册
  * @return 无
  */
void xbox360_set_sample_cb(void *priv, xbox360_sample_cb_t cb);

/**@brief   解析 3 字节输出报文
  * @param[in]  *buf 原始报文
  * @param[in]  len 报文长度
  * @param[out] *out 解析结果
  * @return 0 = 成功，非 0 = 报文非法
  */
u32 xbox360_parse_output(const u8 *buf, u32 len, xbox360_output_t *out);

/**@brief   设备收件人厂商请求钩子，供 user_setup.c 的 setup_device() 调用
  * @param[in]  *usb_device usb_device_t 结构体指针
  * @param[in]  *req 控制请求
  * @return 非 0 表示本驱动已接管该请求
  */
u32 xbox360_setup_device_hook(struct usb_device_t *usb_device, struct usb_ctrlrequest *req);

/**@brief   当前是否处于 Xbox360 手柄模式（描述符已构建、句柄已分配）
  * @note    usb_device_t.wDeviceClass 是 u8，装不下 bit12 的 XBOX360_CLASS，
  *          需要按类位做判断的地方不能查那个字段，改用本函数。
  * @return 1 = 处于手柄模式，0 = 否
  */
u8 xbox360_usb_mode_active(void);

/* 以下三个供 USB_CLASS_REGISTER 引用 */
u32  xbox360_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num);
u32  xbox360_register(const usb_dev usb_id);
void xbox360_release(const usb_dev usb_id);

#endif  /* __USB_DEVICE_XBOX360_H__ */
