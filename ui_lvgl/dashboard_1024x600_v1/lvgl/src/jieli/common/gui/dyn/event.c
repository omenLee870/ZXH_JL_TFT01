/**
 * @file event.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "event.h"

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

void event_set(char * buf, int32_t size, lv_obj_t * obj, int32_t page_id)
{
    if(buf == NULL || size <= 0) {
        return;
    }
    if(gui_get_event_cb == NULL) {
        LV_LOG_WARN("gui_get_event_cb is NULL");
        return;
    }

    JLDynControl * control = (JLDynControl *)buf;
    char * event_buf = buf + control->event_offset;
    int32_t offset = 0;
    int32_t event_size = control->bind_offset - control->event_offset;
    while(offset < event_size) {
        int32_t event_code = *(int32_t *)(event_buf + offset);
        int32_t type_id = *(int32_t *)(event_buf + offset + sizeof(int32_t));
        offset += sizeof(int32_t) * 2;
        switch(type_id) {
            case EVENT_FUNC_ID: {
                    int32_t func_id = *(int32_t *)(event_buf + offset);
                    offset += sizeof(int32_t);
                    lv_event_cb_t cb = gui_get_event_cb(func_id);
                    if(cb != NULL) {
                        lv_obj_add_event_cb(obj, cb, event_code, NULL);
                    }
                }
                break;

            default:
                break;
        }
    }
}

void event_set_get_cb(gui_get_event_cb_t cb)
{
    gui_get_event_cb = cb;
}

#endif /*LV_USE_GUI_PARSE_DYN*/
