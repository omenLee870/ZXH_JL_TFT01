#include "jlui_demo_cfg.h"

#if (defined JLUI_DEMO_LAYOUT_SCALE && JLUI_DEMO_LAYOUT_SCALE)

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "typedef.h"
#include "app_task.h"

#define JLUI_DEMO_PAGE_ID		PAGE_2
#define DEMO_LAYOUT_ID			0x40C00
#define DEMO_LAYOUT_MIN_RATIO	0.0f
#define DEMO_LAYOUT_MAX_RATIO	2.0f

static float curr_ratio = DEMO_LAYOUT_MIN_RATIO;

int jlui_layout_scale()
{
    JLUI_DEMO_SWITCH(JLUI_DEMO_PAGE_ID);
    struct element *layout = ui_core_get_element_by_id(DEMO_LAYOUT_ID);
    if (layout) {
        ui_core_set_element_ratio(layout, curr_ratio, curr_ratio, true);
    }
    curr_ratio += 0.2f;
    printf("curr_ratio: %f\n", curr_ratio);

    ui_core_redraw(ui_core_get_element_by_id(ui_get_current_window_id()));

    if (curr_ratio >= DEMO_LAYOUT_MAX_RATIO) {
        curr_ratio = DEMO_LAYOUT_MIN_RATIO;
        return 0;
    } else {
        return JLUI_DEMO_DEFAULT_TIME;
    }
}


#else

int jlui_layout_scale()
{
    return 0;
}

#endif



