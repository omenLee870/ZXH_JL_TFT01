/**
 * @file dyn_reader.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "dyn_reader.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

uint32_t dyn_align4(uint32_t value)
{
    return (value + 3U) & ~3U;
}

bool dyn_can_read(const uint8_t *p, const uint8_t *end, uint32_t len)
{
    return p != NULL && end != NULL && p <= end && (size_t)(end - p) >= len;
}

bool dyn_read_u8(const uint8_t **p, const uint8_t *end, uint8_t *out)
{
    if (!dyn_can_read(*p, end, 1U)) {
        return false;
    }
    *out = **p;
    *p += 1;
    return true;
}

bool dyn_read_le16(const uint8_t **p, const uint8_t *end, uint16_t *out)
{
    const uint8_t *cur = *p;
    if (!dyn_can_read(cur, end, 2U)) {
        return false;
    }
    *out = (uint16_t)cur[0] | ((uint16_t)cur[1] << 8);
    *p = cur + 2;
    return true;
}

bool dyn_read_le32(const uint8_t **p, const uint8_t *end, uint32_t *out)
{
    const uint8_t *cur = *p;
    if (!dyn_can_read(cur, end, 4U)) {
        return false;
    }
    *out = (uint32_t)cur[0] | ((uint32_t)cur[1] << 8) | ((uint32_t)cur[2] << 16) | ((uint32_t)cur[3] << 24);
    *p = cur + 4;
    return true;
}

bool dyn_read_bytes_view(const uint8_t **p, const uint8_t *end, uint32_t len, dyn_view_t *view)
{
    if (!dyn_can_read(*p, end, len)) {
        return false;
    }
    view->ptr = *p;
    view->len = len;
    *p += len;
    return true;
}

bool dyn_skip_padding4(const uint8_t **p, const uint8_t *end, uint32_t payload_len)
{
    const uint32_t padding = (4U - (payload_len & 3U)) & 3U;
    if (!dyn_can_read(*p, end, padding)) {
        return false;
    }
    for (uint32_t i = 0; i < padding; i++) {
        if ((*p)[i] != 0) {
            return false;
        }
    }
    *p += padding;
    return true;
}

bool dyn_read_block(const uint8_t **p, const uint8_t *end, dyn_block_view_t *block)
{
    uint8_t reserved;
    if (!dyn_read_u8(p, end, &block->block_type) || !dyn_read_u8(p, end, &reserved) ||
        !dyn_read_le16(p, end, &block->item_count) || !dyn_read_le32(p, end, &block->block_len)) {
        return false;
    }
    if (reserved != 0) {
        return false;
    }
    if (!dyn_read_bytes_view(p, end, block->block_len, &block->items)) {
        return false;
    }
    return dyn_skip_padding4(p, end, block->block_len);
}

bool dyn_read_item(const uint8_t **p, const uint8_t *end, dyn_item_view_t *item)
{
    if (!dyn_read_le32(p, end, &item->item_id) || !dyn_read_le32(p, end, &item->item_len)) {
        return false;
    }
    if (!dyn_read_bytes_view(p, end, item->item_len, &item->payload)) {
        return false;
    }
    return dyn_skip_padding4(p, end, item->item_len);
}

bool dyn_block_has_item_stream(uint8_t block_type)
{
    return block_type == JLDYN_CONTROL_BLOCK_PROP || block_type == JLDYN_CONTROL_BLOCK_STYLE ||
           block_type == JLDYN_CONTROL_BLOCK_EVENT || block_type == JLDYN_CONTROL_BLOCK_BIND;
}

uint16_t dyn_crc16(const uint8_t *data, uint32_t len)
{
    const uint8_t *buf = (const uint8_t *)data;
    const uint16_t crc_gen = 0x1021;
    uint16_t crc = 0;
    uint8_t j;

    while (len--) {
        crc ^= (*buf++ << 8);

        for (j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ crc_gen;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}
