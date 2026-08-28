#ifndef MP4_FILE_IO_H
#define MP4_FILE_IO_H

#include <stdint.h>

#include "mov-buffer.h"

#define MP4_FILE_IO_PATH_LEN 256
#define MP4_FILE_IO_MODE_LEN 8
#define MP4_FILE_IO_CACHE_SIZE 4096

typedef struct {
    void *file;
    uint32_t cache_len;
    int64_t logical_pos;
    char path[MP4_FILE_IO_PATH_LEN];
    char mode[MP4_FILE_IO_MODE_LEN];
    uint8_t cache[MP4_FILE_IO_CACHE_SIZE];
} mp4_file_io_ctx_t;

int mp4_file_io_init(mp4_file_io_ctx_t *ctx, const char *path, const char *mode);
int mp4_file_io_flush(mp4_file_io_ctx_t *ctx);

const struct mov_buffer_t *mp4_file_io(void);

#endif /* MP4_FILE_IO_H */
