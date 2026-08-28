#include "jlui_demo_cfg.h"

#if (defined JLUI_PAGE_SWITCH_ENABLE && JLUI_PAGE_SWITCH_ENABLE)

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "typedef.h"
#include "app_task.h"


#define PAGE_SWITCH_TIME	2 * 1000 /*! 页面切换时间间隔 ms */

static u32 page_list[] = {
    PAGE_1,
    PAGE_2,
    PAGE_3,
    PAGE_4,
    PAGE_5,
    PAGE_6,
    PAGE_7,
    PAGE_8,
    PAGE_9,
    PAGE_10,
    PAGE_11,
    PAGE_12,
    PAGE_13,
    PAGE_14,
    PAGE_15,
    /* PAGE_16,  */
    PAGE_17,
    /* PAGE_18,  */
    /* PAGE_19, */
}; /*! 需切换的页面列表 */


struct jlui_demo_page_switch_prev {
    u16 page_index;
};
static struct jlui_demo_page_switch_prev page_prev = {0};
#define __this (&page_prev)


int jlui_page_auto_switch()
{
    JLUI_DEMO_SWITCH(page_list[__this->page_index]);

    __this->page_index ++;
    if (__this->page_index >= ARRAY_SIZE(page_list)) {
        __this->page_index = 0;
        return 0; /*! 索引结束 */
    }

    return PAGE_SWITCH_TIME;
}

#else

int jlui_page_auto_switch()
{
    return 0;
}

#endif

