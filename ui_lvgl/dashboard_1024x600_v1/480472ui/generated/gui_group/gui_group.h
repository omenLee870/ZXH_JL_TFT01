/*Generate Code, Do NOT Edit!*/
#ifndef GUI_GROUP_H
#define GUI_GROUP_H
#ifdef __cplusplus
extern "C" {
#endif

#include "../gui_guider.h"

void gui_group_init();
void gui_group_set_default(lv_group_t *group);
void gui_group_add_obj(lv_group_t *group, lv_obj_t *obj, bool is_add);

#ifdef __cplusplus
}
#endif
#endif /* GUI_GROUP_H */
