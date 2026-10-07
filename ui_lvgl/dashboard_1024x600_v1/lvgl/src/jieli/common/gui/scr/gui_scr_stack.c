/**
 * @file gui_scr_stack.c
 *
 */


/*********************
 *      INCLUDES
 *********************/
#include "gui_scr_stack.h"
#if LV_USE_GUI_SCR_STACK != 0


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_ll_t scr_p_ll;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void gui_scr_stack_init(void)
{
    lv_ll_init(&scr_p_ll, sizeof(gui_scr_t *));
}

void gui_scr_stack_push(gui_scr_t * screen)
{
    gui_scr_t ** screen_p = lv_ll_ins_tail(&scr_p_ll);
    *screen_p = screen;
}

gui_scr_t * gui_scr_stack_pop(void)
{
    gui_scr_t * screen = NULL;
    gui_scr_t ** screen_p = lv_ll_get_tail(&scr_p_ll);
    if(screen_p != NULL) {
        screen = *screen_p;
        lv_ll_remove(&scr_p_ll, screen_p);
        lv_free(screen_p);
    }
    return screen;
}

gui_scr_t * gui_scr_stack_top(void)
{
    gui_scr_t * screen = NULL;
    gui_scr_t ** screen_p = lv_ll_get_tail(&scr_p_ll);
    if(screen_p != NULL) {
        screen = *screen_p;
    }
    return screen;
}

void gui_scr_stack_clear(void)
{
    lv_ll_clear(&scr_p_ll);
}

/*=====================
 * Getter functions
 *====================*/

uint32_t gui_scr_stack_get_len(void)
{
    return lv_ll_get_len(&scr_p_ll);
}

bool gui_scr_stack_is_empty(void)
{
    return lv_ll_is_empty(&scr_p_ll);
}

#endif /*LV_USE_GUI_SCR_STACK*/
