/**
 *
 * @brief
 */

#ifndef RES_H
#define RES_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "gui_parse_dyn.h"

#if LV_USE_GUI_PARSE_DYN != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

// 资源类型
typedef enum {
    RES_TYPE_IMAGE = 1,
    RES_TYPE_FONT = 2,
    RES_TYPE_LOTTIE = 3,
    RES_TYPE_VIDEO = 4,
} ResourceType;

typedef union {
    struct {
        uint32_t block : 2;
        uint32_t reserved : 30;
    } image;
    uint32_t reserved32;
} ResourceHeaderNop;

// 资源头结构体
typedef struct {
    uint8_t res_type;     // 资源类型：1:图片 2:字体 3:Lottie动画 4:视频
    uint8_t res_compress; // 压缩方式：1:无压缩 2:zip压缩 3:RLE压缩 4:ETC2 5:LZ4 6:FNT
    uint16_t res_crc;     // 资源crc16
    uint32_t res_size;    // 资源大小
    ResourceHeaderNop nop;
} BinResourceHeader;

// JL资源结构体
typedef struct {
    char JL[2];           // JL
    uint16_t version;     // 版本号 lvgl8 8、lvgl9 9
    BinResourceHeader header;
    void * resource;
} Resource;

/* dyn 文件里的资源头是固定磁盘布局，最后一个字段始终按 32 位写入。 */
typedef struct {
    char JL[2];
    uint16_t version;
    BinResourceHeader header;
    uint32_t resource_offset;
} ResourceDiskHeader;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Initialize the resource list
 */
void res_list_init();

/**
 * @brief Parse the page resource
 * @param buf The buffer of the dyn file
 */
void res_page_parse(char * buf);

/**
 * @brief Parse the resource
 * @param buf The buffer of the resource
 * @return The resource(lv_image_dsc_t or lv_font_t)
 */
void * res_parse(char * buf);

/**
 * @brief Get the page resource list
 * @param page_id The page id
 * @return The page resource list
 */
LvPageResList * res_list_get_page(int32_t page_id);

/**
 * @brief Check if the page resource list exists
 * @param page_id The page id
 * @return true if the page resource list exists, false otherwise
 */
bool res_list_has_page(int32_t page_id);

/**
 * @brief Get the resource
 * @param page_id The page id
 * @param res_id The resource id
 * @return The resource(lv_image_dsc_t or lv_font_t)
 */
void * res_get(int32_t page_id, int32_t res_id);

/**
 * @brief Free the page resource
 * @param page_id The page id
 */
void res_page_free(int32_t page_id);

/**
 * @brief Free all the resources
 */
void res_free_all();

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
}
#endif

#endif /*RES_H*/
