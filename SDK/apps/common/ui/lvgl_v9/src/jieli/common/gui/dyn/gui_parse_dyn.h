/**
 * @file gui_parse_dyn.h
 *
 */

#ifndef GUI_PARSE_DYN_H
#define GUI_PARSE_DYN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../lvgl.h"

#if LV_USE_GUI_PARSE_DYN != 0

#include "dyn_protocol.h"
#include "../gui_common.h"
#include "../scr/gui_scr.h"

/*********************
 *      DEFINES
 *********************/

#define LV_OBJ_SET_STYLE_LINE_ROUNDED_ID 0x00100031
#define LV_OBJ_SET_STYLE_LENGTH_ID       0x00100032

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    int32_t id;  // 资源ID
    int8_t type; // 资源类型
    int8_t nop1;
    int8_t nop2;
    int8_t nop3;
    void *data;  //从dyn资源中处理转换后的资源
} LvPageRes;

typedef struct {
    int32_t id;  // 页面ID
    lv_ll_t res_ll; // 资源列表
} LvPageResList; //页面资源列表

typedef lv_event_cb_t (*gui_get_event_cb_t)(int32_t id);

typedef int32_t (*gui_bind_send_cb_t)(int32_t msg_id, void *value, int32_t len);
typedef void *(*gui_bind_get_cb_t)(int32_t msg_id);
typedef lv_subject_t *(*gui_bind_get_subject_cb_t)(int32_t id);
typedef void *(*gui_bind_get_data_cb_t)();

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Initialize the dyn data
 */
void gui_dyn_init();

/**
 * @brief Parse the dyn data to object
 * @param path dyn file path
 * @param parent parent object
 * @return lv_obj_t* object
 */
lv_obj_t *gui_dyn_parse_obj(const char *path, lv_obj_t *parent);

/**
 * @brief Parse the dyn data to screen
 * @param path dyn file path
 * @return gui_scr_t* screen
 */
gui_scr_t *gui_dyn_parse_scr(const char *path);

/**
 * @brief Set the event get cb
 * @param cb
 */
void gui_dyn_set_event_get_cb(gui_get_event_cb_t cb);

/**
 * @brief Set the bind send cb
 * @param cb
 */
void gui_dyn_set_bind_send_cb(gui_bind_send_cb_t cb);

/**
 * @brief Set the bind get cb
 * @param cb
 */
void gui_dyn_set_bind_get_cb(gui_bind_get_cb_t cb);

/**
 * @brief Set the bind get subject cb
 * @param cb
 */
void gui_dyn_set_bind_get_subject_cb(gui_bind_get_subject_cb_t cb);

/**
 * @brief Set the bind get data cb
 * @param cb
 */
void gui_dyn_set_bind_get_data_cb(gui_bind_get_data_cb_t cb);

/**
 * @brief 外部 dyn 数据读取器
 *
 * 用于普通 fopen 打不开的路径（如 packres 归档内的表盘：
 * storage/virfat_flash/C/watch1/watch1.dyn）。两段式调用：
 *   buf == NULL ：返回所需字节数（>0），失败返回 -1；
 *   buf != NULL ：写入最多 buf_size 字节并回填 *out_size，成功返回 0。
 * 缓冲区由 gui_parse_dyn 用 lv_malloc 分配，读取器只负责填数据。
 */
typedef int32_t (*gui_dyn_ext_reader_t)(const char *path, void *buf, int32_t buf_size, int32_t *out_size);

/**
 * @brief 注册外部 dyn 数据读取器（未注册时行为与不带该功能时完全一致）
 * @param reader 读取器，传 NULL 取消注册
 */
void gui_dyn_set_ext_reader(gui_dyn_ext_reader_t reader);

/**
 * @brief Get the preview image
 * @param path dyn file path
 * @return The preview image
 */
lv_image_dsc_t *gui_dyn_get_preview(const char *path);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*GUI_PARSE_DYN_H*/
