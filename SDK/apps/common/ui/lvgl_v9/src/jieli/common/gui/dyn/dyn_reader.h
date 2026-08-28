/**
 * @file dyn_reader.h
 *
 */

#ifndef DYN_READER_H
#define DYN_READER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "dyn_protocol.h"
#include <stdbool.h>
#include <stddef.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *     TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

uint32_t dyn_align4(uint32_t value);

/* 边界检查不能被 CRC 替代，CRC 本身也依赖有效范围。 */
bool dyn_can_read(const uint8_t *p, const uint8_t *end, uint32_t len);

bool dyn_read_u8(const uint8_t **p, const uint8_t *end, uint8_t *out);
bool dyn_read_le16(const uint8_t **p, const uint8_t *end, uint16_t *out);
bool dyn_read_le32(const uint8_t **p, const uint8_t *end, uint32_t *out);
bool dyn_read_bytes_view(const uint8_t **p, const uint8_t *end, uint32_t len, dyn_view_t *view);
bool dyn_skip_padding4(const uint8_t **p, const uint8_t *end, uint32_t payload_len);

/* block/items 视图仍指向原 dyn 缓冲区，不拷贝负载。 */
bool dyn_read_block(const uint8_t **p, const uint8_t *end, dyn_block_view_t *block);

/* payload 不包含 item 头和 4 字节对齐填充。 */
bool dyn_read_item(const uint8_t **p, const uint8_t *end, dyn_item_view_t *item);

/* 只有这些 block 的负载是 item TLV 流。 */
bool dyn_block_has_item_stream(uint8_t block_type);

uint16_t dyn_crc16(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*DYN_READER_H*/
