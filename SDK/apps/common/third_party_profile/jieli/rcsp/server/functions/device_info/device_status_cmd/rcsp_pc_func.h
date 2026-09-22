#ifndef __RCSP_PC_FUNC_H__
#define __RCSP_PC_FUNC_H__
#include "typedef.h"
#include "app_config.h"

/* 3.3.1.8 PC从机模式 属性定义
 * Bit0 播放状态   (0:pause, 1:play)      允许设置，获取和推送
 * Bit1 切换上下曲 (0:上一曲, 1:下一曲)   允许设置和推送
 * Bit2 在线状态   (0:下线, 1:在线)       允许获取和推送，不允许设置 */
#define PC_INFO_ATTR_PLAY_STATUS		(0)
#define PC_INFO_ATTR_PREV_NEXT			(1)
#define PC_INFO_ATTR_ONLINE_STATUS		(2)

#endif
