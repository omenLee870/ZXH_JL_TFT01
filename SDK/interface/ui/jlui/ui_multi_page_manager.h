#ifndef __UI_MULTI_PAGE_MANAGER_H__
#define __UI_MULTI_PAGE_MANAGER_H__

#include "system/includes.h"

enum PAGE_PRIO {
    PAGE_PRIO_TOP,      // 顶层显示
    PAGE_PRIO_BOTTOM,   // 底层显示
};

struct multi_page_info {
    int page_id;        // 页面id
    int page_index;     // 页面索引
    enum PAGE_PRIO prio;// 页面显示优先级
};

int ui_multi_page_init(const struct multi_page_info *info_table, int num);

int ui_multi_page_check_id(int id);

int ui_multi_page_check_index(int index);

int ui_multi_page_get_index_by_id(int id);

int ui_multi_page_get_index_by_prio(int prio, int *tab);

int ui_multi_page_get_id_by_prio(int prio, int *tab);

#endif
