/**
 * @file gui_scr.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "gui_scr.h"
#if LV_USE_GUI_SCR != 0

#include <string.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool scr_is_act(lv_obj_t *obj);
static bool check_scr_obj_valid(gui_scr_t *screen, void *ui, bool is_create);
static void scr_free_unloaded_handler(lv_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_ll_t scr_ll;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void gui_scr_init(void)
{
    lv_ll_init(&scr_ll, sizeof(gui_scr_t));
}

gui_scr_t *gui_scr_create(int32_t id, const char *name, lv_obj_t *scr, gui_scr_setup_cb_t setup_cb,
                          gui_scr_unload_cb_t unload_cb)
{
    gui_scr_t *screen = gui_scr_get(id);
    if (screen == NULL) {
        screen = lv_ll_ins_tail(&scr_ll);
        LV_ASSERT_MALLOC(screen);
        if (screen == NULL) {
            return NULL;
        }

        lv_memset(screen, 0, sizeof(gui_scr_t));
    }

    screen->id = id;
    screen->name = name;
    screen->scr = scr;
    screen->is_del = true;
    screen->is_act = false;
    screen->setup_cb = setup_cb;
    screen->unload_cb = unload_cb;

    if (scr != NULL) {
        screen->is_del = false;
        screen->is_act = scr_is_act(scr);
    }

    return screen;
}

void gui_scr_del(gui_scr_t *screen)
{
    if (screen == NULL || screen->is_act) {
        return;
    }

    if (screen->scr != NULL) {
        lv_obj_delete(screen->scr);
        screen->scr = NULL;
    }

    LV_ASSERT_NULL(screen);
    lv_ll_remove(&scr_ll, screen);
    lv_free(screen);
}

void gui_scr_del_all(void)
{
    gui_scr_t *screen = lv_ll_get_head(&scr_ll);
    while (screen) {
        gui_scr_t *next = lv_ll_get_next(&scr_ll, screen);
        gui_scr_del(screen);
        screen = next;
    }
}

gui_scr_t *gui_scr_unload_act(bool is_clear, bool is_del, void *ui)
{
    gui_scr_t *screen = gui_scr_get_act();
    if (screen == NULL) {
        return NULL;
    }

    screen->is_del = is_del;
    if ((is_clear || is_del) && screen->unload_cb != NULL) {
        screen->unload_cb(ui);
    }

    if (is_clear) {
        lv_obj_clean(lv_screen_active());
    }

    screen->is_act = false;
    return screen;
}

/*=====================
 * Setter functions
 *====================*/

void gui_scr_set_act(gui_scr_t *screen, void *ui)
{
    gui_scr_t *old_screen = gui_scr_get_act();
    if (screen == NULL || old_screen == screen) {
        return;
    }

    if (old_screen != NULL) {
        gui_scr_unload_act(false, false, ui);
    }

    screen->is_act = false;
    screen->is_del = true;

    if (check_scr_obj_valid(screen, ui, true) == false) {
        return;
    }

    lv_screen_load(screen->scr);
    screen->is_act = true;
    screen->is_del = false;
}

void gui_scr_set_act_anim(gui_scr_t *screen, void *ui, lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay,
                          bool is_clear, bool auto_del)
{
    gui_scr_t *old_screen = gui_scr_get_act();
    if (screen == NULL || old_screen == screen) {
        return;
    }

    if (old_screen != NULL) {
        gui_scr_unload_act(is_clear, auto_del, ui);
    }

    screen->is_act = false;
    screen->is_del = true;

    if (check_scr_obj_valid(screen, ui, true) == false) {
        return;
    }

    if (old_screen && (is_clear || auto_del)) {
        lv_obj_remove_event_cb(old_screen->scr, scr_free_unloaded_handler);
        lv_obj_add_event_cb(old_screen->scr, scr_free_unloaded_handler, LV_EVENT_SCREEN_UNLOADED, old_screen);
    }

    lv_screen_load_anim(screen->scr, anim_type, time, delay, auto_del);
    screen->is_act = true;
    screen->is_del = false;
}

/*=====================
 * Getter functions
 *====================*/

gui_scr_t *gui_scr_get_act(void)
{
    lv_obj_t *scr = lv_screen_active();
    gui_scr_t *head = lv_ll_get_head(&scr_ll);
    while (head) {
        if (head->scr == scr) {
            return head;
        }
        head = lv_ll_get_next(&scr_ll, head);
    }
    return NULL;
}

gui_scr_t *gui_scr_get(int32_t id)
{
    gui_scr_t *head = lv_ll_get_head(&scr_ll);
    while (head) {
        if (head->id == id) {
            return head;
        }
        head = lv_ll_get_next(&scr_ll, head);
    }
    return NULL;
}

gui_scr_t *gui_scr_get_by_name(const char *name)
{
    gui_scr_t *head = lv_ll_get_head(&scr_ll);
    while (head) {
        if (strcmp(head->name, name) == 0) {
            return head;
        }
        head = lv_ll_get_next(&scr_ll, head);
    }
    return NULL;
}

gui_scr_t *gui_scr_get_by_obj(lv_obj_t *obj)
{
    if (obj == NULL) {
        return NULL;
    }
    gui_scr_t *head = lv_ll_get_head(&scr_ll);
    while (head) {
        if (head->scr == obj) {
            return head;
        }
        head = lv_ll_get_next(&scr_ll, head);
    }
    return NULL;
}

int32_t gui_scr_get_id(gui_scr_t *screen)
{
    if (screen == NULL) {
        return -1;
    }
    return screen->id;
}

const char *gui_scr_get_name(gui_scr_t *screen)
{
    if (screen == NULL) {
        return NULL;
    }
    return screen->name;
}

bool gui_scr_is_act(gui_scr_t *screen)
{
    if (screen == NULL) {
        return false;
    }
    lv_obj_t *scr = lv_screen_active();
    return screen->scr == scr;
}

bool gui_scr_is_del(gui_scr_t *screen)
{
    if (screen == NULL) {
        return false;
    }
    return screen->is_del;
}

uint32_t gui_scr_get_len(void)
{
    return lv_ll_get_len(&scr_ll);
}

bool gui_scr_is_empty(void)
{
    return lv_ll_is_empty(&scr_ll);
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool scr_is_act(lv_obj_t *obj)
{
    if (obj == NULL) {
        return false;
    }

    if (lv_display_get_screen_active(NULL) == obj) {
        return true;
    }
    return false;
}

bool check_scr_obj_valid(gui_scr_t *screen, void *ui, bool is_create)
{
    if (screen == NULL) {
        return false;
    }

    if (screen->scr == NULL || lv_obj_is_valid(screen->scr) == false) {
        if (is_create && screen->setup_cb != NULL) {
            screen->scr = screen->setup_cb(ui);
            if (screen->scr == NULL || lv_obj_is_valid(screen->scr) == false) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

static void scr_free_unloaded_handler(lv_event_t *e)
{
    gui_scr_t *screen = (gui_scr_t *)lv_event_get_user_data(e);
    if (screen == NULL || screen->scr == NULL) {
        return;
    }

    screen->scr = NULL;
}

#endif /*LV_USE_GUI_SCR*/
