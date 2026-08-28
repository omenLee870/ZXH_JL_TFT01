#include "jlui_demo_cfg.h"

#if (defined JLUI_DEMO_LAYOUT_ROTATE && JLUI_DEMO_LAYOUT_ROTATE)

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "typedef.h"
#include "app_task.h"


#define JLUI_DEMO_PAGE_ID		PAGE_2
#define DEMO_LAYOUT_ID			0x40C00
#define DEMO_LAYOUT_MIN_ANGLE	0.0f
#define DEMO_LAYOUT_MAX_ANGLE	360.0f

static float curr_angle = DEMO_LAYOUT_MIN_ANGLE;

int jlui_layout_rotate()
{
    JLUI_DEMO_SWITCH(JLUI_DEMO_PAGE_ID);
    struct element *layout = ui_core_get_element_by_id(DEMO_LAYOUT_ID);
    if (layout) {
        struct rect lay_rect;
        ui_core_get_element_abs_rect(layout, &lay_rect);
        ui_core_set_element_rotate(layout, rect_width(&lay_rect) / 2, rect_height(&lay_rect) / 2, 160, 193, curr_angle, true);
    }

    curr_angle += 1.5f;
    printf("curr_angle: %f\n", curr_angle);

    ui_core_redraw(ui_core_get_element_by_id(ui_get_current_window_id()));

    if (curr_angle >= DEMO_LAYOUT_MAX_ANGLE) {
        curr_angle = DEMO_LAYOUT_MIN_ANGLE;
        return 0;
    } else {
        return JLUI_DEMO_DEFAULT_TIME;
    }
}


#else

int jlui_layout_rotate()
{
    return 0;
}

#endif

