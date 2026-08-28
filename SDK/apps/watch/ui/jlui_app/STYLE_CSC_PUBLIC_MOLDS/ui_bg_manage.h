#ifndef __UI_BG_MANAGE_H
#define __UI_BG_MANAGE_H

#include "app_config.h"
#include "jlui/ui_core.h"


#define CSBG_MAX_NUN    10

typedef enum {
    CSBG_TYPE_WALLPAPER,                    // 普通功能页背景图
#if (defined TCFG_UI_USE_COMMON_BACKGROUND_SWITCH_ENABLE && TCFG_UI_USE_COMMON_BACKGROUND_SWITCH_ENABLE)
    CSBG_TYPE_LOCKSCREEN_WALLPAPER,         // 锁屏背景图
#endif
    CSBG_TYPE_ALL,                          // 背景图类型数量,传这个值全部操作上面所有类型的背景图
} csbg_type_e;


struct csbg_vm_hande {
    // 需要存vm的信息
    int csbg_cur_style[CSBG_TYPE_ALL];                         /*当前背景图片序号*/
};

typedef struct scbg_manage_info {
    int csbg_items;                     /*当前背景图片数量*/
    char *csbg_path[CSBG_MAX_NUN];      /*背景图片路径 e.g."storage/virfat_flash/C/csbg0"*/
    UI_RESFILE *view_file;
    struct flash_file_info view_file_info;
} scbg_manage_info_t;



struct csbg_manage_handle {
    struct csbg_vm_hande vm_info;       /*记录*/
    int csbg_update_flag;               /*表示背景图片需要刷新*/
    int csbg_curr_type;               /*当前背景图片类型*/
    // scbg_manage_info_t * curr_info;   // 用于切换刷新
    scbg_manage_info_t csbg_info[CSBG_TYPE_ALL];       /*背景图片信息*/
};

int ui_csbg_res_check();
int csbg_add_item(char *csbg_name, csbg_type_e type);
char *csbg_get_cur_path(csbg_type_e type);
int csbg_get_style(csbg_type_e type);
int csbg_set_style(int style, csbg_type_e type);
void csbg_change_handler(csbg_type_e type);
int app_set_csbg_by_name(char *csbg_name, csbg_type_e type);
int csbg_del_item(char *csbg_name, csbg_type_e type);
char *app_csbg_get_cur_csbg_name(csbg_type_e type);
char *csbg_get_num_path(u8 num, csbg_type_e type);
int csbg_get_items_num(csbg_type_e type);
int csbg_show(struct element *element, struct draw_context *dc, csbg_type_e type);


void csbg_debug_printf(void);

#endif
