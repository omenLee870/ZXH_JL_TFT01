/**
 * @file event.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "event.h"
#include "dyn_reader.h"

#if LV_USE_GUI_PARSE_DYN != 0

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
static gui_get_event_cb_t gui_get_event_cb = NULL;

/**********************
 *      MACROS
 **********************/

#define EVENT_FUNC_ID 0x00000000 // 事件函数ID

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool event_set_item(uint32_t action_type, dyn_view_t payload, lv_obj_t *obj, int32_t page_id)
{
    (void)page_id;
    if (gui_get_event_cb == NULL) {
        LV_LOG_WARN("gui_get_event_cb is NULL");
        return true;
    }

    const uint8_t *p = payload.ptr;
    const uint8_t *end = payload.ptr + payload.len;
    uint32_t event_code;
    if (!dyn_read_le32(&p, end, &event_code)) {
        return false;
    }

    switch (action_type) {
    case EVENT_FUNC_ID: {
        uint32_t func_id;
        if (!dyn_read_le32(&p, end, &func_id) || p != end) {
            return false;
        }
        lv_event_cb_t cb = gui_get_event_cb((int32_t)func_id);
        if (cb != NULL) {
            lv_obj_add_event_cb(obj, cb, event_code, NULL);
        }
    }
    break;

    default:
        LV_LOG_WARN("unknown event action %d", (int)action_type);
        break;
    }
    return true;
}

void event_set_get_cb(gui_get_event_cb_t cb)
{
    gui_get_event_cb = cb;
}

#endif /*LV_USE_GUI_PARSE_DYN*/
