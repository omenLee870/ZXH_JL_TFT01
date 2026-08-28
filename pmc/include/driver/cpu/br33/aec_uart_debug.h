#ifndef _AEC_UART_DEBUG_H_
#define _AEC_UART_DEBUG_H_

#include "generic/typedef.h"

/*
*********************************************************
*                  aec_uart_open_v2
* Description: 打开数据写卡接口
* Arguments  : nch 允许发送的最大通道数，pcm_packet_size 单次发送允许的最大包长
* Return     : 0 成功 其他 失败
* Note(s)    : 裸机版本，使用静态内存分配
*********************************************************
*/
int aec_uart_open_v2(u8 nch, u16 pcm_packet_size);

/*
*********************************************************
*                  aec_uart_init_v2
* Description: 数据写卡初始化（使用默认参数）
* Arguments  : None.
* Return     : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************
*/
int aec_uart_init_v2(void);

/*
*********************************************************
*                  aec_uart_fill_v2
* Description: 填写对应通道的数据
* Arguments  : ch 通道号，buf 数据地址，size 数据大小
* Return     : 实际填写的字节数
* Note(s)    : None.
*********************************************************
*/
int aec_uart_fill_v2(u8 ch, void *buf, u16 size);

/*
*********************************************************
*                  aec_uart_write_v2
* Description: 通知有数据需要发送（由fill之后调用）
* Arguments  : None.
* Return     : None.
* Note(s)    : 裸机版本中只设置标志位，实际发送在aec_uart_send_loop中完成
*********************************************************
*/
void aec_uart_write_v2(void);

/*
*********************************************************
*                  aec_uart_send_loop
* Description: 裸机主循环中调用，每次发送最多512字节数据
* Arguments  : None.
* Return     : None.
* Note(s)    : 需要在主循环中反复调用，每次发送一个512byte数据块+CRC
*********************************************************
*/
void aec_uart_send_loop(void);

/*
*********************************************************
*                  aec_uart_close_v2
* Description: 关闭数据写卡
* Arguments  : None.
* Return     : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************
*/
int aec_uart_close_v2(void);

#endif/*_AEC_UART_DEBUG_H_*/
