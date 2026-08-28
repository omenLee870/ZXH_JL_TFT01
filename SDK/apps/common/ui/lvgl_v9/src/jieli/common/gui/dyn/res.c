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
#include <stddef.h>
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

static lv_image_dsc_t *parse_image_res(char *buf);
static lv_font_t *parse_font_res(char *buf);
static lv_font_t *parse_font_res_view(const uint8_t *data, uint32_t len);
static int32_t resource_type_to_runtime(uint8_t type);
static LvPageResList *find_res_page(int32_t page_id);
static bool resource_view_matches_type(const uint8_t *data, uint32_t len, uint8_t resource_type);
static bool resource_view_is_fnt(const uint8_t *data, uint32_t len);
static bool resource_view_is_jl(const uint8_t *data, uint32_t len, uint8_t *resource_type);
static bool resource_view_image_payload_is_bounded(const uint8_t *data, uint32_t payload_size);
static uint32_t read_le32(const uint8_t *data);

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

bool res_page_add_resource(int32_t page_id, dyn_resource_view_t resource)
{
    LvPageResList *list = res_list_get_page(page_id);
    if (list == NULL) {
        return false;
    }

    LvPageRes *old_res = (LvPageRes *)lv_ll_get_head(&list->res_ll);
    while (old_res != NULL) {
        if (old_res->id == (int32_t)resource.resource_id) {
            LV_LOG_WARN("duplicate resource id: %d", (int)resource.resource_id);
            return false;
        }
        old_res = (LvPageRes *)lv_ll_get_next(&list->res_ll, old_res);
    }

    if ((resource.resource_type == JLDYN_RES_IMAGE || resource.resource_type == JLDYN_RES_FONT) &&
        !resource_view_matches_type(resource.data.ptr, resource.data.len, resource.resource_type)) {
        LV_LOG_WARN("invalid resource data, id:%d", (int)resource.resource_id);
        return false;
    }

    /* dyn 文件缓冲区解析结束后会释放，因此图片/字体资源必须在这里拷贝或重建为运行时对象。 */
    void *parsed_res = res_parse_view(resource.data.ptr, resource.data.len);
    if (parsed_res == NULL && (resource.resource_type == JLDYN_RES_IMAGE || resource.resource_type == JLDYN_RES_FONT)) {
        LV_LOG_WARN("parse resource failed, id:%d", (int)resource.resource_id);
        return false;
    }

    LvPageRes *new_res = (LvPageRes *)lv_ll_ins_tail(&list->res_ll);
    if (new_res == NULL) {
        return false;
    }
    new_res->id = (int32_t)resource.resource_id;
    new_res->type = resource_type_to_runtime(resource.resource_type);
    new_res->data = parsed_res;
    return true;
}

bool res_page_rename(int32_t old_page_id, int32_t new_page_id)
{
    if (old_page_id == new_page_id) {
        return true;
    }

    LvPageResList *list = find_res_page(old_page_id);
    if (list == NULL || find_res_page(new_page_id) != NULL) {
        return false;
    }

    list->id = new_page_id;
    return true;
}

void *res_parse(char *buf)
{
    if (buf == NULL) {
        return NULL;
    }

    if (res_font_is_fnt(buf)) {
        return parse_font_res(buf);
    }

    if (buf[0] != 'J' || buf[1] != 'L') {
        return NULL;
    }

    Resource *resource = (Resource *)buf;
    if (resource->header.res_type == RES_TYPE_IMAGE) {
        return parse_image_res(buf);
    } else if (resource->header.res_type == RES_TYPE_FONT) {
        return parse_font_res(buf);
    }
    return NULL;
}

void *res_parse_view(const uint8_t *data, uint32_t len)
{
    uint8_t resource_type = 0;

    if (resource_view_is_fnt(data, len)) {
        return parse_font_res_view(data, len);
    }
    if (!resource_view_is_jl(data, len, &resource_type)) {
        return NULL;
    }

    /* dyn RESOURCE item 的 data_len 必须继续传到资源解析器，避免内部格式越过资源项边界。 */
    if (resource_type == RES_TYPE_IMAGE) {
        return parse_image_res((char *)data);
    }
    if (resource_type == RES_TYPE_FONT) {
        return parse_font_res_view(data, len);
    }
    return NULL;
}

LvPageResList *res_list_get_page(int32_t page_id)
{
    LvPageResList *list = find_res_page(page_id);
    if (list != NULL) {
        return list;
    }

    //创建页面资源列表
    LvPageResList *new_list = (LvPageResList *)lv_ll_ins_head(&res_list);
    if (new_list == NULL) {
        return NULL;
    }
    new_list->id = page_id;
    lv_ll_init(&new_list->res_ll, sizeof(LvPageRes));
    return new_list;
}

bool res_list_has_page(int32_t page_id)
{
    return find_res_page(page_id) != NULL;
}

void *res_get(int32_t page_id, int32_t res_id)
{
    if (!res_list_has_page(page_id)) {
        return NULL;
    }

    LvPageResList *list = res_list_get_page(page_id);
    if (list == NULL) {
        return NULL;
    }

    LvPageRes *res = (LvPageRes *)lv_ll_get_head(&list->res_ll);
    while (res != NULL) {
        if (res->id == res_id) {
            return res->data;
        }
        res = (LvPageRes *)lv_ll_get_next(&list->res_ll, res);
    }
    return NULL;
}

void res_page_free(int32_t page_id)
{
    if (!res_list_has_page(page_id)) {
        return;
    }

    LvPageResList *list = res_list_get_page(page_id);
    if (list == NULL) {
        return;
    }

    // 遍历释放所有资源
    LvPageRes *res = (LvPageRes *)lv_ll_get_head(&list->res_ll);
    while (res != NULL) {
        LvPageRes *next = (LvPageRes *)lv_ll_get_next(&list->res_ll, res);

        // 根据资源类型释放资源数据
        if (res->data) {
            switch (res->type) {
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
    LvPageResList *list = (LvPageResList *)lv_ll_get_head(&res_list);
    while (list != NULL) {
        LvPageResList *next = (LvPageResList *)lv_ll_get_next(&res_list, list);

        // 释放该页面的所有资源
        res_page_free(list->id);

        list = next;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_image_dsc_t *parse_image_res(char *buf)
{
    return res_image_parse(buf);
}

static lv_font_t *parse_font_res(char *buf)
{
    return res_font_parse(buf);
}

static lv_font_t *parse_font_res_view(const uint8_t *data, uint32_t len)
{
    return res_font_parse_view(data, len);
}

static int32_t resource_type_to_runtime(uint8_t type)
{
    switch (type) {
    case JLDYN_RES_IMAGE:
        return RES_TYPE_IMAGE;
    case JLDYN_RES_FONT:
        return RES_TYPE_FONT;
    case JLDYN_RES_LOTTIE:
        return RES_TYPE_LOTTIE;
    case JLDYN_RES_VIDEO:
        return RES_TYPE_VIDEO;
    default:
        return type;
    }
}

static LvPageResList *find_res_page(int32_t page_id)
{
    LvPageResList *list = (LvPageResList *)lv_ll_get_head(&res_list);
    while (list != NULL) {
        if (list->id == page_id) {
            return list;
        }
        list = (LvPageResList *)lv_ll_get_next(&res_list, list);
    }
    return NULL;
}

static bool resource_view_matches_type(const uint8_t *data, uint32_t len, uint8_t resource_type)
{
    uint8_t jl_resource_type = 0;
    if (resource_type == JLDYN_RES_FONT && resource_view_is_fnt(data, len)) {
        return true;
    }
    if (!resource_view_is_jl(data, len, &jl_resource_type)) {
        return false;
    }

    if (resource_type == JLDYN_RES_IMAGE) {
        return jl_resource_type == RES_TYPE_IMAGE;
    }
    if (resource_type == JLDYN_RES_FONT) {
        return jl_resource_type == RES_TYPE_FONT;
    }
    return true;
}

static bool resource_view_is_fnt(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len < 8U || memcmp(data + 4, "head", 4) != 0) {
        return false;
    }

    uint32_t head_len = read_le32(data);
    return head_len >= 8U && head_len <= len;
}

static bool resource_view_is_jl(const uint8_t *data, uint32_t len, uint8_t *resource_type)
{
    if (data == NULL || len < sizeof(ResourceDiskHeader) || data[0] != 'J' || data[1] != 'L') {
        return false;
    }

    uint32_t payload_size = read_le32(data + 8);
    if (payload_size > len - sizeof(ResourceDiskHeader)) {
        return false;
    }
    if (data[4] == RES_TYPE_IMAGE && !resource_view_image_payload_is_bounded(data, payload_size)) {
        return false;
    }

    if (resource_type != NULL) {
        *resource_type = data[4];
    }
    return true;
}

static bool resource_view_image_payload_is_bounded(const uint8_t *data, uint32_t payload_size)
{
    const uint32_t data_size_offset = sizeof(ResourceDiskHeader) + (uint32_t)offsetof(lv_image_dsc_t, data_size);
    if (payload_size < sizeof(lv_image_dsc_t) || data_size_offset + 4U < data_size_offset) {
        return false;
    }

    uint32_t image_data_size = read_le32(data + data_size_offset);
    return image_data_size <= payload_size - sizeof(lv_image_dsc_t);
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

#endif /*LV_USE_GUI_PARSE_DYN*/
