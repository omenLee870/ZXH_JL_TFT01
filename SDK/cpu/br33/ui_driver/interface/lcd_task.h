#ifndef __PUBLIC_LCD_TASK_H__
#define __PUBLIC_LCD_TASK_H__


// LCD 任务名称
#define LCD_TASK_NAME	"lcd"


// LCD 消息类型
typedef enum {
    LCD_MSG_OTHER,	// 默认消息
    LCD_MSG_FLUSH,	// 刷屏消息
} lcd_task_msg_t;


// 刷屏动作标志
typedef enum {
    LCD_FLUSH_SET_AREA,	// 设置刷屏区域
    LCD_FLUSH_KISTART,	// 启动刷屏
    LCD_FLUSH_CONTINUE,	// 续传
} lcd_flush_flag_t;


void jllcd_task(void *p);


#endif


