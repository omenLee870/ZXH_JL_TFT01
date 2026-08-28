#ifndef UI_PLAYER_H
#define UI_PLAYER_H



#include "jlui/control.h"
#include "jlui/ui_core.h"


#define ui_player_for_id(id) \
	ui_element_for_id(id, struct ui_player)



struct ui_player {
    struct element elm; 	//must be first
    int fd;
    const struct ui_player_info *info;
    const struct element_event_handler *handler;
};


void register_ui_player_handler(const struct element_event_handler *handler);























#endif
