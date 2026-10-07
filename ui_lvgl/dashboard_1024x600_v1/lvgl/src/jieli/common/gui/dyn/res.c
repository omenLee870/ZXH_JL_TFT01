/**
 * @file res.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "res.h"
#include "res_font.h"
#include "res_image.h"

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

static lv_image_dsc_t * parse_image_res(char * buf);
static lv_font_t * parse_font_res(char * buf);

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_ll_t res_list;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void res_list_init()
{
    lv_ll_init(&res_list, sizeof(LvPageResList));
}

void res_page_parse(char * buf)
{
    if(buf == NULL) {
        return;
    }
    if(buf[0] != 'J' || buf[1] != 'L') {
        return;
    }

    JLDynResource * res = (JLDynResource *)buf;
    JLDynControl * control = (JLDynControl *)(buf + sizeof(JLDynResource));
    int32_t page_id = control->id;

    //获取页面ID
    int32_t res_cnt = res->res_cnt;
    if(res_cnt == 0) {
        return;
    }

    LvPageResList * list = res_list_get_page(page_id);
    if(list == NULL) {
        return;
    }

    for(int32_t i = 0; i < res_cnt; i++) {
        int32_t res_offset = *(int32_t *)(buf + res->res_offset + i * sizeof(int32_t));
        JLDynPageResource * dyn_res = (JLDynPageResource *)(buf + res_offset);
        int32_t res_id = dyn_res->id;
        int32_t res_type = dyn_res->type;
        int32_t res_size = dyn_res->size;
        int32_t dyn_offset = dyn_res->offset;

        //获取资源
        char * res_data = (char *)dyn_res + sizeof(JLDynPageResource);

        //根据资源类型，获取资源
        void * parsed_res = res_parse(res_data);

        //将资源添加到列表中
        LvPageRes * new_res = (LvPageRes *)lv_ll_ins_tail(&list->res_ll);
        if(new_res == NULL) {
            return;
        }
        new_res->id = res_id;
        new_res->type = res_type;
        new_res->data = parsed_res;
    }
}

void * res_parse(char * buf)
{
    if(buf == NULL) {
        return NULL;
    }

    if(res_font_is_fnt(buf)) {
        return parse_font_res(buf);
    }

    if(buf[0] != 'J' || buf[1] != 'L') {
        return NULL;
    }

    Resource * resource = (Resource *)buf;
    if(resource->header.res_type == RES_TYPE_IMAGE) {
        return parse_image_res(buf);
    }
    else if(resource->header.res_type == RES_TYPE_FONT) {
        return parse_font_res(buf);
    }
    return NULL;
}

LvPageResList * res_list_get_page(int32_t page_id)
{
    LvPageResList * list = (LvPageResList *)lv_ll_get_head(&res_list);
    while(list != NULL) {
        if(list->id == page_id) {
            return list;
        }
        list = (LvPageResList *)lv_ll_get_next(&res_list, list);
    }

    //创建页面资源列表
    LvPageResList * new_list = (LvPageResList *)lv_ll_ins_head(&res_list);
    if(new_list == NULL) {
        return NULL;
    }
    new_list->id = page_id;
    lv_ll_init(&new_list->res_ll, sizeof(LvPageRes));
    return new_list;
}

bool res_list_has_page(int32_t page_id)
{
    LvPageResList * list = (LvPageResList *)lv_ll_get_head(&res_list);
    while(list != NULL) {
        if(list->id == page_id) {
            return true;
        }
        list = (LvPageResList *)lv_ll_get_next(&res_list, list);
    }

    return false;
}

void * res_get(int32_t page_id, int32_t res_id)
{
    if(!res_list_has_page(page_id)) {
        return NULL;
    }

    LvPageResList * list = res_list_get_page(page_id);
    if(list == NULL) {
        return NULL;
    }

    LvPageRes * res = (LvPageRes *)lv_ll_get_head(&list->res_ll);
    while(res != NULL) {
        if(res->id == res_id) {
            return res->data;
        }
        res = (LvPageRes *)lv_ll_get_next(&list->res_ll, res);
    }
    return NULL;
}

void res_page_free(int32_t page_id)
{
    if(!res_list_has_page(page_id)) {
        return;
    }

    LvPageResList * list = res_list_get_page(page_id);
    if(list == NULL) {
        return;
    }

    // 遍历释放所有资源
    LvPageRes * res = (LvPageRes *)lv_ll_get_head(&list->res_ll);
    while(res != NULL) {
        LvPageRes * next = (LvPageRes *)lv_ll_get_next(&list->res_ll, res);

        // 根据资源类型释放资源数据
        if(res->data) {
            switch(res->type) {
                case RES_TYPE_IMAGE: {
                        res_image_free(res->data);
                        break;
                    }
                case RES_TYPE_FONT: {
                        res_font_free(res->data);
                        break;
                    }
                default:
                    lv_free(res->data);
                    break;
            }
        }

        // 从链表中移除并释放资源节点
        lv_ll_remove(&list->res_ll, res);
        lv_free(res);

        res = next;
    }

    // 释放页面资源列表
    lv_ll_remove(&res_list, list);
    lv_free(list);
}

void res_free_all()
{
    // 遍历所有页面资源列表
    LvPageResList * list = (LvPageResList *)lv_ll_get_head(&res_list);
    while(list != NULL) {
        LvPageResList * next = (LvPageResList *)lv_ll_get_next(&res_list, list);

        // 释放该页面的所有资源
        res_page_free(list->id);

        list = next;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_image_dsc_t * parse_image_res(char * buf)
{
    return res_image_parse(buf);
}

static lv_font_t * parse_font_res(char * buf)
{
    return res_font_parse(buf);
}

#endif /*LV_USE_GUI_PARSE_DYN*/
