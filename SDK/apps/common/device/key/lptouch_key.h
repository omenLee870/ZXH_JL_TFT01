#ifndef _LPTOUCH_KEY_H_
#define _LPTOUCH_KEY_H_

#include "typedef.h"
#include "key_driver.h"


/* 事件类型 */
#define TOUCH_EVENT_PRESS       1
#define TOUCH_EVENT_RELEASE     0

#define TOUCH_CH_MAX			8

typedef struct {
    u8 channel;     /* 通道号 0~7 */
    u8 event;       /* TOUCH_EVENT_PRESS / TOUCH_EVENT_RELEASE */
    u16 value;      /* 当前采样值(可选, 方便上层做手势识别) */
} touch_msg_t;

/* 按键驱动扫描参数 (外部可引用, 允许用户覆盖时间参数) */
extern struct key_driver_para lptouch_key_scan_para;

/* 获取当前按压通道的键值 (由 key_driver_scan 定时调用) */
u8 lptouch_key_get_value(void);

/* 硬件初始化 (由 key_driver_init 调用) */
int lptouch_key_init(void);

/* 时间基准 (用于滑动时序判断) */
extern unsigned long jiffies_msec(void);

#endif /* _LPTOUCH_KEY_H_ */
