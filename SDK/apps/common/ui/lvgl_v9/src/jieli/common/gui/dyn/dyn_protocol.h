/**
 * @file dyn_protocol.h
 *
 * dyn 二进制协议的常量和轻量视图结构，供运行时解析器使用。
 */

#ifndef DYN_PROTOCOL_H
#define DYN_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define JLDYN_PROTOCOL_VERSION 1

#define JLDYN_FILE_HEADER_SIZE           24U
#define JLDYN_SECTION_DIR_ENTRY_SIZE     16U
#define JLDYN_PAGE_ITEM_HEADER_SIZE      16U
#define JLDYN_CONTROL_RECORD_HEADER_SIZE 12U
#define JLDYN_CONTROL_BLOCK_HEADER_SIZE  8U
#define JLDYN_ITEM_HEADER_SIZE           8U
#define JLDYN_EVENT_PAYLOAD_HEADER_SIZE  4U
#define JLDYN_RESOURCE_ITEM_HEADER_SIZE  12U
#define JLDYN_PREVIEW_ITEM_HEADER_SIZE   8U

typedef enum {
    JLDYN_SECTION_PAGE = 1,
    JLDYN_SECTION_RESOURCE = 2,
    JLDYN_SECTION_PREVIEW = 3,
} JLDynSectionType;

typedef enum {
    JLDYN_CONTROL_BLOCK_PROP = 1,
    JLDYN_CONTROL_BLOCK_STYLE = 2,
    JLDYN_CONTROL_BLOCK_EVENT = 3,
    JLDYN_CONTROL_BLOCK_BIND = 4,
} JLDynControlBlockType;

typedef enum {
    JLDYN_RES_IMAGE =  1,
    JLDYN_RES_FONT =   2,
    JLDYN_RES_LOTTIE = 3,
    JLDYN_RES_VIDEO =  4,
    JLDYN_RES_OTHER =  255,
} JLDynResourceType;

typedef enum {
    DYN_OK = 0,
    DYN_ERR_SHORT,
    DYN_ERR_MAGIC,
    DYN_ERR_VERSION,
    DYN_ERR_LVGL_MISMATCH,
    DYN_ERR_RANGE,
    DYN_ERR_ALIGN,
    DYN_ERR_CRC,
    DYN_ERR_DUPLICATE_ID,
    DYN_ERR_UNSUPPORTED_SECTION_VERSION,
    DYN_ERR_BAD_COUNT,
    DYN_ERR_BAD_TREE,
    DYN_ERR_ALLOC,
    DYN_ERR_UNSUPPORTED_CONTROL,
} dyn_err_t;

typedef struct {
    const uint8_t *ptr;
    uint32_t len;
} dyn_view_t;

typedef struct {
    uint8_t section_type;
    uint8_t section_version;
    uint32_t section_offset;
    uint32_t section_size;
    uint16_t item_count;
    uint16_t section_crc16;
} dyn_section_entry_t;

typedef struct {
    uint32_t page_id;
    uint32_t page_size;
    uint16_t control_count;
    dyn_view_t page_name;
    dyn_view_t controls;
} dyn_page_view_t;

typedef struct {
    uint8_t control_type;
    uint8_t block_count;
    uint16_t child_count;
    uint32_t control_id;
    uint32_t record_size;
    dyn_view_t blocks;
} dyn_control_view_t;

typedef struct {
    uint8_t block_type;
    uint16_t item_count;
    uint32_t block_len;
    dyn_view_t items;
} dyn_block_view_t;

typedef struct {
    uint32_t item_id;
    uint32_t item_len;
    dyn_view_t payload;
} dyn_item_view_t;

typedef struct {
    uint32_t resource_id;
    uint8_t resource_type;
    dyn_view_t data;
} dyn_resource_view_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*DYN_PROTOCOL_H*/
