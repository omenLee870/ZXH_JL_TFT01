#ifndef ZXH_BLE_FRAME_H
#define ZXH_BLE_FRAME_H
#include <stdint.h>
#include <string.h>
#define ZXH_BLE_MESSAGE_MAX 1024
#define ZXH_BLE_PACKET_MAX 20
#define ZXH_BLE_HEADER_SIZE 6
#define ZXH_BLE_CHUNK_SIZE 14
struct zxh_ble_decoder {
    char data[ZXH_BLE_MESSAGE_MAX + 1];
    uint16_t total, used;
    uint8_t frame, index, receiving;
    uint32_t started;
};
static inline int zxh_ble_frame_feed(struct zxh_ble_decoder *d, const uint8_t *p, unsigned int n, uint32_t now)
{
    unsigned int total;
    if (!p || n <= ZXH_BLE_HEADER_SIZE || n > ZXH_BLE_PACKET_MAX || p[0] != 0x5a || p[1] != 0x58) goto invalid;
    total = p[4] | ((unsigned int)p[5] << 8);
    if (!total || total > ZXH_BLE_MESSAGE_MAX) goto invalid;
    if (!p[3]) {
        d->total = total; d->used = 0; d->frame = p[2]; d->index = 0; d->receiving = 1; d->started = now;
    }
    if (!d->receiving || d->frame != p[2] || d->index != p[3] || d->total != total ||
        (uint32_t)(now - d->started) > 5000u || d->used + n - ZXH_BLE_HEADER_SIZE > total) goto invalid;
    memcpy(d->data + d->used, p + ZXH_BLE_HEADER_SIZE, n - ZXH_BLE_HEADER_SIZE);
    d->used += n - ZXH_BLE_HEADER_SIZE; d->index++;
    if (d->used == total) { d->data[total] = 0; d->receiving = 0; return 1; }
    return 0;
invalid:
    d->receiving = 0; d->used = 0; return -1;
}
static inline unsigned int zxh_ble_frame_make(const char *text, unsigned int total, unsigned int offset,
                                             uint8_t frame, uint8_t index, uint8_t *out)
{
    unsigned int n = total - offset;
    if (n > ZXH_BLE_CHUNK_SIZE) n = ZXH_BLE_CHUNK_SIZE;
    out[0] = 0x5a; out[1] = 0x58; out[2] = frame; out[3] = index;
    out[4] = total; out[5] = total >> 8;
    memcpy(out + ZXH_BLE_HEADER_SIZE, text + offset, n);
    return n + ZXH_BLE_HEADER_SIZE;
}
#endif
