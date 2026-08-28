
#include "app_config.h"
#include "includes.h"

#include "dbi.h"

#include "lcd_task.h"

struct lcd_task_private_def {
    u8 has_init;
};

static struct lcd_task_private_def lcd_task_priv ALIGNED(4) = {0};
#define __this	(&lcd_task_priv)




void jllcd_task(void *p)
{
    int ret;
    int msg[32] = {0};

    while (1) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        /* put_buf((u8 *)msg, 32); */
        /* printf("%s(), msg: %d, flush: %d, area[%d, %d, %d, %d], addr: 0x%x, hdl: 0x%x\n", \ */
        /* __func__, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]); */

        switch (msg[0]) {
        case LCD_MSG_FLUSH:
            // 消息格式：刷屏动作，刷屏标志，刷屏区域，刷屏buf，buf句柄
            switch (msg[1]) {
            case LCD_FLUSH_SET_AREA:
                lcd_set_draw_area(msg[2], msg[3], msg[4], msg[5]);
                break;
            case LCD_FLUSH_KISTART:
                extern void lcd_wait_te_sync();
                lcd_wait_te_sync();		// 等待TE同步
                lcd_set_draw_area(msg[2], msg[3], msg[4], msg[5]); // 设置绘制区域
                lcd_draw_kistart((u8 *)msg[6], msg[2], msg[3], msg[4], msg[5]); // 开始绘制
                break;
            case LCD_FLUSH_CONTINUE:
                lcd_draw_continue((u8 *)msg[6], msg[2], msg[3], msg[4], msg[5]);
                break;
            default:
                break;
            }
            break;
        case LCD_MSG_OTHER:
            break;
        default:
            break;
        }
    }
}


