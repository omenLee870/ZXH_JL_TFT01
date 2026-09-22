/**@file        gamepad_input.h
  * @brief       手柄输入源汇聚层
  * @details     USB 手柄驱动只认「一个取数回调」，具体数据从哪来由本层决定。
  *              各输入源（本机摇杆 ADC、无线传送等）注册自己的取数函数，
  *              应用层在运行时用 gamepad_input_select() 切换当前生效的那个。
  *
  *              设计要点：
  *              - 排他：任一时刻只有一个源生效，不会出现两源各填一半字段
  *              - 切换由应用层显式决定（RF 连上/断开的事件里调用），
  *                源之间互不知道对方存在
  *              - 不选中本机 ADC 源时，它的取数函数不被调用，采集会自然停下来，
  *                所以本层不需要额外的启停回调
  */
#ifndef __GAMEPAD_INPUT_H__
#define __GAMEPAD_INPUT_H__

#include "typedef.h"
#include "usb/device/xbox360.h"     /* struct xbox360_gamepad / xbox360_sample_cb_t */

/* 输入源。注册与切换都用这组 id */
enum {
    GAMEPAD_SRC_NONE = 0,       /* 不接任何源 → 上报中立值（摇杆回中、无按键） */
    GAMEPAD_SRC_LOCAL,          /* 本机摇杆 ADC */
    GAMEPAD_SRC_RF,             /* 无线传送来的摇杆/按键 */
    GAMEPAD_SRC_NUM,
};

/**@brief   注册一个输入源
  * @note    约定「先 register 再 select」。get 的签名与 xbox360_sample_cb_t
  *          一致，在 USB 中断上下文被调用：只做取数与换算，不阻塞、
  *          不要调用 CPU_CRITICAL_ENTER/EXIT。
  *          未填充的字段应保持中立（pad 进回调前已被清零）。
  * @param[in]  src 源 id
  * @param[in]  priv 私有指针，回调时原样回传
  * @param[in]  get 取数函数，传 NULL 等价于注销
  * @return 0 成功，非 0 参数非法
  */
int gamepad_input_register(u8 src, void *priv, xbox360_sample_cb_t get);

/**@brief   注销一个输入源
  * @note    若当前生效的正是这个源，注销后自动回退到 GAMEPAD_SRC_NONE
  * @param[in]  src 源 id
  * @return 无
  */
void gamepad_input_unregister(u8 src);

/**@brief   切换当前生效的数据源
  * @note    应用层在 RF 连接/断开的事件里调用。传入未注册的源等价于
  *          GAMEPAD_SRC_NONE（上报中立值），不会出错。
  * @param[in]  src 源 id
  * @return 无
  */
void gamepad_input_select(u8 src);

/**@brief   查询当前生效的数据源
  * @return 源 id（GAMEPAD_SRC_xxx）
  */
u8 gamepad_input_get_active(void);

/**@brief   供 USB 手柄驱动每帧调用
  * @note    签名与 xbox360_sample_cb_t 一致，初始化处一行接上：
  *              xbox360_set_sample_cb(NULL, gamepad_input_get);
  *          当前源未注册时什么都不写，pad 保持驱动清零的中立值。
  * @param[in]  priv 未使用（占位以匹配回调签名）
  * @param[out] pad  取到的本帧手柄状态
  * @return 无
  */
void gamepad_input_get(void *priv, struct xbox360_gamepad *pad);

#endif  /* __GAMEPAD_INPUT_H__ */
