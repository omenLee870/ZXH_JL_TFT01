#ifndef UI_FILE_ATTRS_H
#define UI_FILE_ATTRS_H



#include "jlui/ui_core.h"
#include "jlui/control.h"


#define ui_file_attrs_for_id(id) \
	ui_element_for_id(id, struct ui_fattrs)


#define FATTRS_CHILD_NUM 	 (FATTRS_CHILD_END - FATTRS_CHILD_BEGIN)



struct ui_fattrs {
    struct element elm;
    char fsize_str[16];
    struct ui_file_attrs attrs;
    const struct ui_fattrs_info *info;
    const struct element_event_handler *handler;
};




int ui_file_attrs_show(struct element *elm, struct ui_file_attrs *attrs);


int ui_file_attrs_get(struct element *elm, struct ui_file_attrs *attrs);

int ui_file_attrs_set(struct element *elm, struct ui_file_attrs *attrs);
































#endif
