#ifdef ZXH_CAST_PAGE_TEST
#include "cast_page_test_platform.h"
#elif !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#include "lv_ui.h"
#include "custom.h"
#include "gui_guider.h"
#include "ui_update_page.h"
#include "screen_mirror.h"
#include <stdint.h>
#endif

#if defined(ZXH_CAST_PAGE_TEST) || !LV_USE_GUIBUILDER_SIMULATOR
static volatile uint32_t cast_request;
int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);

static void cast_page_on_ui(void *arg, ...)
{
    uint32_t token = (uint32_t)(uintptr_t)arg;
    gui_scr_t *current, *map;
    if (token != cast_request || !(token & 1u)) return;
    current = gui_scr_get_act();
    if (!current || ui_update_page_is_active() ||
        current->id == GUI_SCREEN_UPGRADE_PAGE ||
        current->id == GUI_SCREEN_HARDWARE_TEST ||
        current->id == GUI_SCREEN_PAGE_VIDEO) {
        printf("[CAST_PAGE] protected page, no switch\n");
        return;
    }
    if (current->id != GUI_SCREEN_PAGE_MAP) {
        map = ui_get_scr(GUI_SCREEN_PAGE_MAP);
        if (!map) { printf("[CAST_PAGE] map create failed\n"); return; }
        ui_load_scr_anim(&guider_ui, map, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        current = gui_scr_get_act();
    }
    if (current && current->id == GUI_SCREEN_PAGE_MAP) {
        /* Also repair a preloaded map whose load callback did not run. */
        map_video_enter();
        printf("[CAST_PAGE] map active\n");
    }
}

void screen_map_request_display(int active)
{
    uint32_t token = ((cast_request + 2u) & ~1u) | (active ? 1u : 0u);
    cast_request = token;
    if (active && lvgl_rpc_post_func(cast_page_on_ui, 1, (int)token))
        printf("[CAST_PAGE] UI request queue failed\n");
}
#endif
