#include "jlui_demo_cfg.h"

#if (defined JLUI_DEMO_WATCH_ENABLE && JLUI_DEMO_WATCH_ENABLE)

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "typedef.h"
#include "app_task.h"

#define JLUI_DEMO_PAGE_ID	PAGE_16
#define JLUI_DEMO_WATCH_ID	0x206000
#define JLUI_DEFAULT_MSEC	345678
#define JLUI_REFRESH_COUNT	60

static u32 curr_msec = JLUI_DEFAULT_MSEC;
static u32 refresh_count = 0;

int jlui_demo_watch()
{
    JLUI_DEMO_SWITCH(JLUI_DEMO_PAGE_ID);

    struct ui_watch *watch = (struct ui_watch *)ui_core_get_element_by_id(JLUI_DEMO_WATCH_ID);
    if (watch) {
        int msec = curr_msec % 1000;
        int sec = (curr_msec / 1000) % 60;
        int min = ((curr_msec / 1000) / 60) % 60;
        int hour = (((curr_msec / 1000) / 60) / 60) % 12;
        printf("set watch time: %d:%d:%d %d\n", hour, min, sec, msec);
        ui_watch_set_time(watch, hour, min, sec, msec);
    }
    JLUI_DEMO_REFRESH();
    curr_msec += JLUI_DEMO_DEFAULT_TIME;
    printf("curr_msec: %d\n", curr_msec);

    if (++refresh_count > JLUI_REFRESH_COUNT) {
        refresh_count = 0;
        curr_msec = JLUI_DEFAULT_MSEC;
        return 0;
    } else {
        return JLUI_DEMO_DEFAULT_TIME;
    }
}

#else

int jlui_demo_watch()
{
    return 0;
}

#endif

