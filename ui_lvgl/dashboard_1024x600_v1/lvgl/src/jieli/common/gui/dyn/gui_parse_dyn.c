/**
 * @file gui_parse_dyn.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "gui_parse_dyn.h"

#if LV_USE_GUI_PARSE_DYN != 0

#include "bind.h"
#include "control.h"
#include "event.h"
#include "prop.h"
#include "style.h"
#include "res.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static char * get_dyn_data(const char * path, JLDynResource * res, int32_t * size);
static void free_dyn_data(char * buf);
static lv_obj_t * parse_obj(char * buf, int32_t size, lv_obj_t * parent);
static void page_delete_event_cb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void gui_dyn_init()
{
    res_list_init();
}

lv_obj_t * gui_dyn_parse_obj(const char * path, lv_obj_t * parent)
{
    JLDynResource res;
    int32_t size;
    char * buf = get_dyn_data(path, &res, &size);
    if(buf == NULL) {
        return NULL;
    }

    if(res.JL[0] != 'J' || res.JL[1] != 'L') {
        free_dyn_data(buf);
        return NULL;
    }

    lv_obj_t * obj = parse_obj(buf, size, parent);
    free_dyn_data(buf);
    return obj;
}

gui_scr_t * gui_dyn_parse_scr(const char * path)
{
    JLDynResource res;
    int32_t size;
    char * buf = get_dyn_data(path, &res, &size);
    if(buf == NULL) {
        return NULL;
    }

    if(res.JL[0] != 'J' || res.JL[1] != 'L') {
        free_dyn_data(buf);
        return NULL;
    }

    lv_obj_t * obj = parse_obj(buf, size, NULL);
    if(obj == NULL) {
        free_dyn_data(buf);
        return NULL;
    }

    JLDynControl * control = (JLDynControl *)(buf + sizeof(JLDynResource));
    gui_scr_t * scr = gui_scr_create(control->id, "", obj, NULL, NULL);
    free_dyn_data(buf);
    return scr;
}

void gui_dyn_set_event_get_cb(gui_get_event_cb_t cb)
{
    event_set_get_cb(cb);
}

void gui_dyn_set_bind_send_cb(gui_bind_send_cb_t cb)
{
    bind_set_send_cb(cb);
}

void gui_dyn_set_bind_get_cb(gui_bind_get_cb_t cb)
{
    bind_set_get_cb(cb);
}

void gui_dyn_set_bind_get_subject_cb(gui_bind_get_subject_cb_t cb)
{
    bind_set_get_subject_cb(cb);
}

void gui_dyn_set_bind_get_data_cb(gui_bind_get_data_cb_t cb)
{
    bind_set_get_data_cb(cb);
}

lv_image_dsc_t * gui_dyn_get_preview(const char * path)
{
    JLDynResource res;
    int32_t size;
    char * buf = get_dyn_data(path, &res, &size);
    if(buf == NULL) {
        return NULL;
    }

    if(res.JL[0] != 'J' || res.JL[1] != 'L') {
        free_dyn_data(buf);
        return NULL;
    }

    if(res.preview_offset == 0) {
        free_dyn_data(buf);
        return NULL;
    }

    char * preview_buf = (char *)buf + res.preview_offset;
    lv_image_dsc_t * preview = (lv_image_dsc_t *)res_parse(preview_buf);
    free_dyn_data(buf);
    return preview;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief Get the dyn data
 *
 * @param path dyn file path
 * @param res dynamic resource header
 * @param size data size
 * @return char* data buffer
 */
static char * get_dyn_data(const char * path, JLDynResource * res, int32_t * size)
{
    lv_fs_file_t file;
    lv_fs_res_t res_open = lv_fs_open(&file, path, LV_FS_MODE_RD);
    if(res_open != LV_FS_RES_OK) {
        LV_LOG_WARN("open file failed(%d): %s", res_open, path);
        return NULL;
    }

    lv_fs_res_t res_seek = lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    if(res_seek != LV_FS_RES_OK) {
        LV_LOG_WARN("seek file failed");
        lv_fs_close(&file);
        return NULL;
    }

    uint32_t file_size;
    lv_fs_res_t res_tell = lv_fs_tell(&file, &file_size);
    if(res_tell != LV_FS_RES_OK) {
        LV_LOG_WARN("tell file failed");
        lv_fs_close(&file);
        return NULL;
    }

    lv_fs_seek(&file, 0, LV_FS_SEEK_SET);

    char * buf = (char *)lv_malloc(file_size);
    lv_fs_res_t res_read = lv_fs_read(&file, buf, file_size, NULL);
    if(res_read != LV_FS_RES_OK) {
        lv_free(buf);
        lv_fs_close(&file);
        return NULL;
    }

    lv_fs_res_t res_close = lv_fs_close(&file);
    if(res_close != LV_FS_RES_OK) {
        lv_free(buf);
        return NULL;
    }

    if(res != NULL) {
        lv_memcpy(res, buf, sizeof(JLDynResource));
    }

    *size = file_size;
    return buf;
}

/**
 * @brief Free the dyn data
 *
 * @param buf data buffer
 */
static void free_dyn_data(char * buf)
{
    if(buf != NULL) {
        lv_free(buf);
    }
}

/**
 * @brief Parse the obj
 *
 * @param buf
 * @param size
 * @param parent
 * @return lv_obj_t*
 */
static lv_obj_t * parse_obj(char * buf, int32_t size, lv_obj_t * parent)
{
    //解析资源列表
    res_page_parse(buf);

    JLDynResource * res = (JLDynResource *)buf;
    JLDynControl * control = (JLDynControl *)(buf + sizeof(JLDynResource));
    int32_t control_size = res->res_offset - sizeof(JLDynResource);
    lv_obj_t * obj = control_parse((char *)control, control_size, parent, control->id);
    if(obj != NULL && lv_obj_is_valid(obj)) {
        lv_obj_add_event_cb(obj, page_delete_event_cb, LV_EVENT_DELETE, (void *)control->id);
    }
    return obj;
}

/**
 * @brief Page delete event callback
 *
 * @param e
 */
static void page_delete_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t page_id = (int32_t)lv_event_get_user_data(e);
    res_page_free(page_id); //释放页面资源
}

#endif /*LV_USE_GUI_PARSE_DYN*/
