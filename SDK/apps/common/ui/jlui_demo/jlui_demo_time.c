#include "jlui_demo_cfg.h"

#if (defined JLUI_DEMO_TIME_ENABLE && JLUI_DEMO_TIME_ENABLE)

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "typedef.h"
#include "app_task.h"

#define JLUI_DEMO_PAGE_ID	PAGE_4
#define JLUI_DEMO_TIME_ID	0x82800

static struct utime demo_time[] = {
    {2026, 1, 5, 10, 24, 13},
    {2026, 1, 5, 20, 33, 10},
    {2026, 1, 5,  3, 56,  0},
};
static int time_index = 0;

int jlui_demo_time()
{
    JLUI_DEMO_SWITCH(JLUI_DEMO_PAGE_ID);
    struct ui_time *time = (struct ui_time *)ui_core_get_element_by_id(JLUI_DEMO_TIME_ID);
    if (time) {
        ui_time_update(time, &demo_time[time_index]);
    }
    time_index ++;

    if (time_index > ARRAY_SIZE(demo_time)) {
        time_index = 0;
        return 0;
    } else {
        return 3 * 1000;
    }
}

#else

int jlui_demo_time()
{
    return 0;
}

#endif

