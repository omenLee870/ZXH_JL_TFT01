/**
 * @file dyn_utils.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "dyn_utils.h"
#include <string.h>

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

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool dyn_read_string_alloc(const uint8_t **p, const uint8_t *end, char **out)
{
    uint32_t len;
    dyn_view_t view;
    if (!dyn_read_le32(p, end, &len) || !dyn_read_bytes_view(p, end, len, &view) || !dyn_skip_padding4(p, end, len)) {
        return false;
    }

    char *text = lv_malloc(len + 1U);
    if (text == NULL) {
        return false;
    }
    lv_memset(text, 0, len + 1U);
    lv_memcpy(text, view.ptr, len);
    *out = text;
    return true;
}

void dyn_digitclock_set_text(lv_obj_t *obj, const char *fmt, const struct tm *time)
{
    char text[128];
    size_t len = strftime(text, sizeof(text), fmt, time);
    if (len == 0) {
        text[0] = '\0';
    }
    lv_label_set_text(obj, text);
}

#endif /*LV_USE_GUI_PARSE_DYN*/
