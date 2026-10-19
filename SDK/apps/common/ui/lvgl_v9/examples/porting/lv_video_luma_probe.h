#ifndef LV_VIDEO_LUMA_PROBE_H
#define LV_VIDEO_LUMA_PROBE_H
#include <stddef.h>
#include <stdint.h>

/* Read-only sparse luma sample. It does not classify a dark scene as a failed camera. */
struct lv_video_luma_sample { unsigned minimum, maximum, count; uint32_t hash; };
static inline int lv_video_luma_probe(const uint8_t *pixels, size_t bytes,
    unsigned width, unsigned height, unsigned stride, struct lv_video_luma_sample *out)
{
    unsigned row, col;
    if (!pixels || !out || !width || !height || stride < width || height > bytes / stride) return -1;
    out->minimum = 255; out->maximum = out->count = 0; out->hash = 2166136261u;
    for (row = 0; row < 8; ++row) {
        size_t y = (size_t)(height - 1) * row / 7;
        for (col = 0; col < 8; ++col) {
            size_t x = (size_t)(width - 1) * col / 7;
            unsigned value = pixels[y * stride + x];
            if (value < out->minimum) out->minimum = value;
            if (value > out->maximum) out->maximum = value;
            out->hash = (out->hash ^ value) * 16777619u;
            ++out->count;
        }
    }
    return 0;
}
#endif
