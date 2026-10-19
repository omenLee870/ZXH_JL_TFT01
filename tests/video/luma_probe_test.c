#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "lv_video_luma_probe.h"
int main(void)
{
    uint8_t pixels[512], before[512];
    struct lv_video_luma_sample sample;
    memset(pixels, 16, sizeof(pixels));
    assert(!lv_video_luma_probe(pixels, sizeof(pixels), 16, 16, 32, &sample));
    assert(sample.minimum == 16 && sample.maximum == 16 && sample.count == 64);
    uint32_t dark_hash = sample.hash;
    /* Padding is bright but must never be sampled as image content. */
    for (unsigned y = 0; y < 16; ++y) memset(pixels + y * 32 + 16, 255, 16);
    assert(!lv_video_luma_probe(pixels, sizeof(pixels), 16, 16, 32, &sample));
    assert(sample.maximum == 16 && sample.hash == dark_hash);
    pixels[0] = 0; pixels[15 * 32 + 15] = 255;
    memcpy(before, pixels, sizeof(pixels));
    assert(!lv_video_luma_probe(pixels, sizeof(pixels), 16, 16, 32, &sample));
    assert(sample.minimum == 0 && sample.maximum == 255 && sample.hash != dark_hash);
    assert(!memcmp(before, pixels, sizeof(pixels)));
    assert(lv_video_luma_probe(NULL, sizeof(pixels), 16, 16, 32, &sample) < 0);
    assert(lv_video_luma_probe(pixels, sizeof(pixels), 0, 16, 32, &sample) < 0);
    assert(lv_video_luma_probe(pixels, sizeof(pixels), 16, 16, 0, &sample) < 0);
    assert(lv_video_luma_probe(pixels, sizeof(pixels), 16, 16, 15, &sample) < 0);
    assert(lv_video_luma_probe(pixels, 511, 16, 16, 32, &sample) < 0);
    assert(lv_video_luma_probe(pixels, sizeof(pixels), 16, 16, 32, NULL) < 0);
    assert(!lv_video_luma_probe(pixels, 1, 1, 1, 1, &sample));
    assert(sample.minimum == 0 && sample.maximum == 0 && sample.count == 64);
    puts("Video luma probe: black/range/hash, padded stride, bounds and read-only sampling PASS");
    return 0;
}
