#ifdef ZXH_CAST_PAGE_TEST
#include "cast_page_test_platform.h"
#elif !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#include "lv_ui.h"
#include "custom.h"
#include "gui_guider.h"
#include "ui_update_page.h"
#include "screen_mirror.h"
#include "app_meter_control.h"
#include "chongwuyibiao_donghua.h"
#include <stdint.h>
#endif

#if defined(ZXH_CAST_PAGE_TEST) || !LV_USE_GUIBUILDER_SIMULATOR
static volatile uint32_t cast_request;
static volatile int cast_pet_active;
#ifndef ZXH_CAST_PAGE_TEST
static lv_obj_t *cast_pet_root;

/** @brief 宠物页卸载时关闭显示链路；普通关机、倒车与升级页仍优先处理。 */
static void cast_pet_event(lv_event_t *event)
{
    if (lv_event_get_target(event) != cast_pet_root) return;
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        cast_pet_active = 0;
        map_video_leave();
    } else if (code == LV_EVENT_SCREEN_UNLOADED) {
        /* 完成卸载后才解除动画暂停，避免 UNLOAD_START 时活动根仍为宠物页而误重启动画。 */
        chongwuyibiao_donghua_yunxu(true);
    } else if (code == LV_EVENT_DELETE) {
        cast_pet_active = 0; cast_pet_root = NULL;
    } else if (code == LV_EVENT_SCREEN_LOADED && (cast_request & 1u) && app_meter_control_media_allowed()) {
        cast_pet_active = 1;
        chongwuyibiao_donghua_yunxu(false);
        map_video_enter();
    }
}
#endif

/** @brief 仅 UI 线程使用，决定投屏对象在宠物页前景显示，避免被宠物图层遮住。 */
int screen_cast_pet_active(void) { return cast_pet_active; }
int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);

static void cast_page_on_ui(void *arg, ...)
{
    uint32_t token = (uint32_t)(uintptr_t)arg;
    gui_scr_t *current, *pet;
    if (token != cast_request) return;
    if (!(token & 1u)) {
        cast_pet_active = 0;
        chongwuyibiao_donghua_yunxu(true);
        return;
    }
#ifndef ZXH_CAST_PAGE_TEST
    if (!app_meter_control_media_allowed()) return;
#endif
    current = gui_scr_get_act();
    if (!current || ui_update_page_is_active() ||
        current->id == GUI_SCREEN_UPGRADE_PAGE ||
        current->id == GUI_SCREEN_HARDWARE_TEST ||
        current->id == GUI_SCREEN_PAGE_VIDEO) {
        printf("[CAST_PAGE] protected page, no switch\n");
        return;
    }
    if (current->id != GUI_SCREEN_CHONGWUYIBIAO) {
        pet = ui_get_scr(GUI_SCREEN_CHONGWUYIBIAO);
        if (!pet) { printf("[CAST_PAGE] pet create failed\n"); return; }
        ui_load_scr_anim(&guider_ui, pet, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        current = gui_scr_get_act();
    }
    if (current && current->id == GUI_SCREEN_CHONGWUYIBIAO) {
#ifndef ZXH_CAST_PAGE_TEST
        if (cast_pet_root != current->scr) {
            cast_pet_root = current->scr;
            lv_obj_add_event_cb(cast_pet_root, cast_pet_event, LV_EVENT_ALL, NULL);
        }
#endif
        cast_pet_active = 1;
        chongwuyibiao_donghua_yunxu(false);
        map_video_enter();
        printf("[CAST_PAGE] navigation on pet page\n");
    }
}

void screen_map_request_display(int active)
{
    uint32_t token = ((cast_request + 2u) & ~1u) | (active ? 1u : 0u);
    cast_request = token;
    if (lvgl_rpc_post_func(cast_page_on_ui, 1, (int)token))
        printf("[CAST_PAGE] UI request queue failed\n");
}
#endif
