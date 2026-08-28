#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_ui_api.data.bss")
#pragma data_seg(".lcd_ui_api.data")
#pragma const_seg(".lcd_ui_api.text.const")
#pragma code_seg(".lcd_ui_api.text")
#endif


#include "jlui_demo_cfg.h"

#if (defined JLUI_DEMO_ENABLE && JLUI_DEMO_ENABLE)


#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "res/mem_var.h"
#include "typedef.h"
#include "app_task.h"
#include "app_main.h"
#include "res_config.h"


typedef int (*jlui_demo_callback_t)(void);
/* Demo 回调列表 */
static jlui_demo_callback_t demo_list[] = {
    jlui_page_auto_switch,
    jlui_layout_scale,
    jlui_layout_rotate,
    jlui_demo_hslider,
    jlui_demo_vslider,
    jlui_demo_time,
    jlui_demo_number,
    jlui_demo_compass,
    jlui_demo_watch,
};


struct jlui_demo_main_prev {
    u16 jlui_demo_timer;
    u16 jlui_demo_index;
};
static struct jlui_demo_main_prev demo_prev = {0};
#define __this (&demo_prev)

#define JLUI_DEMO_START() \
{ \
	__this->jlui_demo_index = 0; \
	__this->jlui_demo_timer = sys_timeout_add(NULL, jlui_demo_timer, 1 * 1000); \
}


void jlui_demo_timer(void *p)
{
    int start_index = __this->jlui_demo_index;
    int list_size = ARRAY_SIZE(demo_list);

    do {
        if (__this->jlui_demo_index >= list_size) {
            __this->jlui_demo_index = 0; /*! 索引回绕 */
        }

        int next_timeout = demo_list[__this->jlui_demo_index]();
        printf("@@@ Run demo index: %d, next timeout: %d\n", __this->jlui_demo_index, next_timeout);
        if (next_timeout != 0) {
            /* 返回非0，索引保持不变，返回值设置下一次超时 */
            __this->jlui_demo_timer = sys_timeout_add(NULL, jlui_demo_timer, next_timeout);
            return;
        }
        __this->jlui_demo_index ++; /*! 返回0，移动索引，调用下一个demo */

        /* ui_core_redraw(ui_core_get_element_by_id(ui_get_current_window_id())); [>! 主动刷新 <] */

    } while (start_index != __this->jlui_demo_index);

    JLUI_DEMO_START(); /*! 遍历一圈DEMO，重新开始遍历 */
}


int jlui_demo_init(void)
{
    printf("@@@@@ %s() Enter!\n", __func__);
    JLUI_DEMO_START();

    printf("@@@@@ %s() Leave!\n", __func__);
    return 0;
}

int jlui_demo_free(void)
{
    printf("@@@@@ %s() Enter!\n", __func__);
    if (__this->jlui_demo_timer) {
        sys_timer_del(__this->jlui_demo_timer);
        __this->jlui_demo_timer = 0;
    }

    printf("@@@@@ %s() Leave!\n", __func__);
    return 0;
}

#endif


