#ifndef __RCSP_TRACE_H__
#define __RCSP_TRACE_H__

#include "typedef.h"

/*
 * RCSP 交互指令抓包打印(调试辅助)
 * ============================================================
 * 作用:
 *   把手机APP <-> 设备(智能眼镜/手表)之间的 RCSP 命令帧, 按命令级打印出来,
 *   方便在串口上观察整个交互过程。典型形态:
 *
 *   [RCSP][<--] RX CMD                 code=0x07(SYS_INFO_GET   ) sn=3   len=5 data: ff 00 00 00 02
 *   [RCSP][-->] TX SYSINFO_GET_RESP    code=0x07(SYS_INFO_GET   ) sn=3   status=0 len=5 data: ff 04 01 05 05
 *   [RCSP][-->] TX SYSINFO_PUSH        code=0x09(SYS_INFO_AUTO_UPDATE) len=5 data: ff 04 01 05 05
 *
 * 开关: 改 RCSP_CMD_TRACE_EN 为 0 关闭 (编译期剔除, 无运行开销)。
 *
 * 说明: 帧的底层收/发(SPP/BLE 字节流、组帧)在 RCSP 协议库里(.a 预编译),
 *       不开源; 本抓包挂在协议库回调到 SDK 的命令入口/出口上, 属于"命令级"
 *       抓包 —— 对一个"公有模式信息(0x07/0x08/0x09)"的完整交互已足够观察。
 *
 * 实现: rcsp_functions/../server/rcsp_cmd_recieve.c 提供 rcsp_trace_frame()
 *       (与各调用点同在一个 rcsp 库内链接, 无需新增编译单元)。
 */
#ifndef RCSP_CMD_TRACE_EN
#define RCSP_CMD_TRACE_EN       0
#endif

#if RCSP_CMD_TRACE_EN
void rcsp_trace_frame(const char *dir, const char *name, u8 opcode, u8 sn, u8 status, const u8 *data, u16 len);
#else
#define rcsp_trace_frame(...)   do {} while (0)
#endif

#endif//__RCSP_TRACE_H__
