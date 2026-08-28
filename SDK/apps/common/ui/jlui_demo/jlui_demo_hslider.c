#include "jlui_demo_cfg.h"

#if (defined JLUI_DEMO_HSLIDER_ENABLE && JLUI_DEMO_HSLIDER_ENABLE)

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "typedef.h"
#include "app_task.h"

#define JLUI_DEMO_PAGE_ID		PAGE_14
#define JLUI_DEMO_HSLIDER_ID	0x1C7000

static int percent = 0;

int jlui_demo_hslider()
{
    JLUI_DEMO_SWITCH(JLUI_DEMO_PAGE_ID);
    struct ui_slider *slider = (struct ui_slider *)ui_core_get_element_by_id(JLUI_DEMO_HSLIDER_ID);
    if (slider) {
        ui_slider_set_persent(slider, percent);
    }
    percent += 1;
    printf("percent: %d\n", percent);

    ui_core_redraw(ui_core_get_element_by_id(ui_get_current_window_id()));

    if (percent >= 100) {
        percent = 0;
        return 0;
    } else {
        return JLUI_DEMO_DEFAULT_TIME;
    }
}

#else

int jlui_demo_hslider()
{
    return 0;
}
#endif

