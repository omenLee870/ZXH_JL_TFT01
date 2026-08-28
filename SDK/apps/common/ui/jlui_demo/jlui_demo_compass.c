#include "jlui_demo_cfg.h"

#if (defined JLUI_DEMO_COMPASS_ENABLE && JLUI_DEMO_COMPASS_ENABLE)

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "typedef.h"
#include "app_task.h"

#define JLUI_DEMO_PAGE_ID		PAGE_11
#define JLUI_DEMO_COMPASS_ID	0x169800

static int demo_angle[] = {5, 10, 20, 30, 50, 80, 95};

static int bk_angle_index =  0;
static int indicator_angle = 0;

int jlui_demo_compass()
{
    JLUI_DEMO_SWITCH(JLUI_DEMO_PAGE_ID);
    struct ui_compass *compass = (struct ui_compass *)ui_core_get_element_by_id(JLUI_DEMO_COMPASS_ID);
    if (compass) {
        ui_compass_set_angle(compass, demo_angle[bk_angle_index], indicator_angle);
    }
    printf("bk_angle: %d\n", demo_angle[bk_angle_index]);
    JLUI_DEMO_REFRESH();

    bk_angle_index ++;
    if (bk_angle_index >= ARRAY_SIZE(demo_angle)) {
        bk_angle_index = 0;
        return 0;
    } else {
        return 100;
    }
}

#else

int jlui_demo_compass()
{
    return 0;
}

#endif

