/**
 * @file gui_scr_manager.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "gui_scr_manager.h"

#if LV_USE_GUI_SCR_MANAGER != 0

/*********************
 *      DEFINES
 *********************/

#define CHK_MANAGER_VALID(scr_manager) if (scr_manager == NULL) return -1;
#define CHK_LIST_VALID(scr_ll) if (scr_ll == NULL) return -1;

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool is_exist(lv_ll_t * scr_ll, int32_t id);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void gui_scr_manager_init(gui_scr_manager_t * scr_manager)
{
    if(scr_manager == NULL) {
        return;
    }

    lv_ll_init(&scr_manager->scr_ll, sizeof(int32_t));
    scr_manager->id = 0;
    scr_manager->cnt = 0;
    scr_manager->effect = 0;
}

int32_t gui_scr_manager_add(gui_scr_manager_t * scr_manager, int32_t id)
{
    CHK_MANAGER_VALID(scr_manager);

    if(is_exist(&scr_manager->scr_ll, id)) {
        return -1;
    }

    int32_t * id_p = lv_ll_ins_tail(&scr_manager->scr_ll);
    *id_p = id;
    scr_manager->cnt++;
    return 0;
}

int32_t gui_scr_manager_insert(gui_scr_manager_t * scr_manager, int32_t id, uint32_t idx)
{
    CHK_MANAGER_VALID(scr_manager);

    if(is_exist(&scr_manager->scr_ll, id)) {
        return -1;
    }

    if(idx == 0) {
        int32_t * id_new_p = lv_ll_ins_head(&scr_manager->scr_ll);
        *id_new_p = id;
        scr_manager->cnt++;
        return 0;
    }

    int32_t * id_p = lv_ll_get_head(&scr_manager->scr_ll);
    for(uint32_t i = 0; i < idx; i++) {
        if(id_p == NULL) {
            break;
        }
        id_p = lv_ll_get_next(&scr_manager->scr_ll, id_p);
    }

    if(id_p == NULL) {
        int32_t * id_new_p = lv_ll_ins_tail(&scr_manager->scr_ll);
        *id_new_p = id;
        scr_manager->cnt++;
        return 0;
    }

    int32_t * id_new_p = lv_ll_ins_prev(&scr_manager->scr_ll, id_p);
    *id_new_p = id;
    scr_manager->cnt++;
    return 0;
}

int32_t gui_scr_manager_head(gui_scr_manager_t * scr_manager)
{
    CHK_MANAGER_VALID(scr_manager);

    int32_t * id_p = lv_ll_get_head(&scr_manager->scr_ll);
    if(id_p == NULL) {
        return -1;
    }
    return *id_p;
}

int32_t gui_scr_manager_tail(gui_scr_manager_t * scr_manager)
{
    CHK_MANAGER_VALID(scr_manager);

    int32_t * id_p = lv_ll_get_tail(&scr_manager->scr_ll);
    if(id_p == NULL) {
        return -1;
    }
    return *id_p;
}

int32_t gui_scr_manager_next(gui_scr_manager_t * scr_manager, int32_t id)
{
    CHK_MANAGER_VALID(scr_manager);

    int32_t * id_p = lv_ll_get_head(&scr_manager->scr_ll);
    while(id_p != NULL) {
        if(*id_p == id) {
            id_p = lv_ll_get_next(&scr_manager->scr_ll, id_p);
            if(id_p == NULL) {
                return -1;
            }
            return *id_p;
        }
        id_p = lv_ll_get_next(&scr_manager->scr_ll, id_p);
    }
    return -1;
}

int32_t gui_scr_manager_prev(gui_scr_manager_t * scr_manager, int32_t id)
{
    CHK_MANAGER_VALID(scr_manager);

    int32_t * id_p = lv_ll_get_head(&scr_manager->scr_ll);
    int32_t * id_prev = NULL;
    while(id_p != NULL) {
        if(*id_p == id) {
            if(id_prev == NULL) {
                return -1;
            }
            return *id_prev;
        }
        id_prev = id_p;
        id_p = lv_ll_get_next(&scr_manager->scr_ll, id_p);
    }
    return -1;
}

int32_t gui_scr_manager_get_idx(gui_scr_manager_t * scr_manager, int32_t id)
{
    CHK_MANAGER_VALID(scr_manager);

    int32_t * id_p = lv_ll_get_head(&scr_manager->scr_ll);
    uint32_t idx = 0;
    while(id_p != NULL) {
        if(*id_p == id) {
            return idx;
        }
        idx++;
        id_p = lv_ll_get_next(&scr_manager->scr_ll, id_p);
    }
    return -1;
}

int32_t gui_scr_manager_swap(gui_scr_manager_t * scr_manager, int32_t id1, int32_t id2)
{
    CHK_MANAGER_VALID(scr_manager);

    int32_t * id_p = lv_ll_get_head(&scr_manager->scr_ll);
    int32_t * id1_p = NULL;
    int32_t * id2_p = NULL;
    while(id_p != NULL) {
        if(*id_p == id1) {
            id1_p = id_p;
        }
        if(*id_p == id2) {
            id2_p = id_p;
        }
        if(id1_p != NULL && id2_p != NULL) {
            break;
        }
        id_p = lv_ll_get_next(&scr_manager->scr_ll, id_p);
    }

    if(id1_p == NULL || id2_p == NULL) {
        return -1;
    }

    *id1_p = id2;
    *id2_p = id1;
    return 0;
}

int32_t gui_scr_manager_del(gui_scr_manager_t * scr_manager, int32_t id)
{
    CHK_MANAGER_VALID(scr_manager);

    int32_t * id_p = lv_ll_get_head(&scr_manager->scr_ll);
    while(id_p != NULL) {
        if(*id_p == id) {
            lv_ll_remove(&scr_manager->scr_ll, id_p);
            scr_manager->cnt--;
            return 0;
        }
        id_p = lv_ll_get_next(&scr_manager->scr_ll, id_p);
    }
    return -1;
}

int32_t gui_scr_manager_del_by_idx(gui_scr_manager_t * scr_manager, uint32_t idx)
{
    CHK_MANAGER_VALID(scr_manager);

    int32_t * id_p = lv_ll_get_head(&scr_manager->scr_ll);
    for(uint32_t i = 0; i < idx; i++) {
        if(id_p == NULL) {
            return -1;
        }
        id_p = lv_ll_get_next(&scr_manager->scr_ll, id_p);
    }

    if(id_p == NULL) {
        return -1;
    }

    lv_ll_remove(&scr_manager->scr_ll, id_p);
    scr_manager->cnt--;
    return 0;
}

int32_t gui_scr_manager_del_all(gui_scr_manager_t * scr_manager)
{
    CHK_MANAGER_VALID(scr_manager);

    lv_ll_clear(&scr_manager->scr_ll);
    scr_manager->cnt = 0;
    return 0;
}

/*=====================
 * Setter functions
 *====================*/

/*=====================
 * Getter functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool is_exist(lv_ll_t * scr_ll, int32_t id)
{
    int32_t * id_p = lv_ll_get_head(scr_ll);
    while(id_p != NULL) {
        if(*id_p == id) {
            return true;
        }
        id_p = lv_ll_get_next(scr_ll, id_p);
    }
    return false;
}

#endif /*LV_USE_GUI_SCR_MANAGER*/