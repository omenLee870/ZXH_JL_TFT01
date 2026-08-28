#ifndef _mp4_muxer_h_
#define _mp4_muxer_h_

#include <stddef.h>
#include <stdint.h>

typedef struct mp4_muxer_t mp4_muxer_t;

typedef enum {
    MP4_MUXER_OBJECT_H264 = 0x21,
    MP4_MUXER_OBJECT_AAC = 0x40,
    MP4_MUXER_OBJECT_JPEG = 0x6C,
    MP4_MUXER_OBJECT_OPUS = 0xAD,
    MP4_MUXER_OBJECT_G711A = 0xFD,
    MP4_MUXER_OBJECT_G711U = 0xFE,
} mp4_muxer_object_t;

mp4_muxer_t *mp4_muxer_open(const char *path, int flags);
int mp4_muxer_add_video(mp4_muxer_t *muxer, mp4_muxer_object_t object, int width, int height, const void *extra_data, size_t extra_data_size);
int mp4_muxer_add_audio(mp4_muxer_t *muxer, mp4_muxer_object_t object, int channel_count, int bits_per_sample, int sample_rate);
int mp4_muxer_write_video(mp4_muxer_t *muxer, const void *data, size_t bytes, uint32_t pts, uint32_t dts, int flags);
int mp4_muxer_write_audio(mp4_muxer_t *muxer, const void *data, size_t bytes, uint32_t pts, uint32_t dts, int flags);
void mp4_muxer_close(mp4_muxer_t *muxer);

#endif /* _mp4_muxer_h_ */
