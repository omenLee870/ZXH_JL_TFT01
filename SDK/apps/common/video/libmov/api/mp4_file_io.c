#include "mp4_file_io.h"

#include <limits.h>
#include <string.h>

#include "fs/fs.h"

static void mp4_file_reset_runtime(mp4_file_io_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ctx->file = NULL;
    ctx->cache_len = 0u;
    ctx->logical_pos = 0;
}

static int mp4_file_flush_internal(mp4_file_io_ctx_t *ctx)
{
    if (ctx == NULL || ctx->file == NULL) {
        return -1;
    }
    if (ctx->cache_len == 0u) {
        return 0;
    }
    if ((uint64_t)fwrite(ctx->cache, 1, ctx->cache_len, (FILE *)ctx->file) != ctx->cache_len) {
        return -1;
    }
    ctx->cache_len = 0u;
    return 0;
}

int mp4_file_io_init(mp4_file_io_ctx_t *ctx, const char *path, const char *mode)
{
    size_t path_len;
    size_t mode_len;

    if (ctx == NULL) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));
    if (path == NULL || mode == NULL) {
        return -1;
    }

    path_len = strlen(path);
    mode_len = strlen(mode);
    if (path_len == 0u || path_len >= sizeof(ctx->path) ||
        mode_len == 0u || mode_len >= sizeof(ctx->mode)) {
        return -1;
    }

    memcpy(ctx->path, path, path_len + 1u);
    memcpy(ctx->mode, mode, mode_len + 1u);
    mp4_file_reset_runtime(ctx);
    return 0;
}

int mp4_file_io_flush(mp4_file_io_ctx_t *ctx)
{
    return mp4_file_flush_internal(ctx);
}

static int mp4_file_open(void *param)
{
    mp4_file_io_ctx_t *ctx = (mp4_file_io_ctx_t *)param;
    int64_t pos;

    if (ctx == NULL || ctx->path[0] == '\0' || ctx->mode[0] == '\0') {
        return -1;
    }
    if (ctx->file != NULL) {
        return 0;
    }

    ctx->file = fopen(ctx->path, ctx->mode);
    if (ctx->file == NULL) {
        mp4_file_reset_runtime(ctx);
        return -1;
    }

    pos = (int64_t)ftell((FILE *)ctx->file);
    if (pos < 0) {
        fclose((FILE *)ctx->file);
        mp4_file_reset_runtime(ctx);
        return -1;
    }

    ctx->cache_len = 0u;
    ctx->logical_pos = pos;
    return 0;
}

static int mp4_file_close(void *param)
{
    mp4_file_io_ctx_t *ctx = (mp4_file_io_ctx_t *)param;
    int ret = 0;
    int close_ret;

    if (ctx == NULL) {
        return -1;
    }
    if (ctx->file == NULL) {
        mp4_file_reset_runtime(ctx);
        return 0;
    }

    if (mp4_file_flush_internal(ctx) != 0) {
        ret = -1;
    }

    close_ret = fclose((FILE *)ctx->file);
    if (close_ret != 0) {
        ret = -1;
    }

    mp4_file_reset_runtime(ctx);
    return ret;
}

static int mp4_file_read(void *param, void *data, uint64_t bytes)
{
    mp4_file_io_ctx_t *ctx = (mp4_file_io_ctx_t *)param;

    if (ctx == NULL || ctx->file == NULL || data == NULL || bytes > UINT32_MAX) {
        return -1;
    }

    if (mp4_file_flush_internal(ctx) != 0) {
        return -1;
    }

    if (bytes == (uint64_t)fread(data, 1, (u32)bytes, (FILE *)ctx->file)) {
        ctx->logical_pos += (int64_t)bytes;
        return 0;
    }
    return -1;
}

static int mp4_file_write(void *param, const void *data, uint64_t bytes)
{
    mp4_file_io_ctx_t *ctx = (mp4_file_io_ctx_t *)param;
    uint32_t remaining;

    if (ctx == NULL || ctx->file == NULL || data == NULL || bytes > UINT32_MAX) {
        return -1;
    }
    if (bytes == 0u) {
        return 0;
    }

    remaining = (uint32_t)(sizeof(ctx->cache) - ctx->cache_len);
    if (bytes <= remaining) {
        memcpy(ctx->cache + ctx->cache_len, data, (size_t)bytes);
        ctx->cache_len += (uint32_t)bytes;
        ctx->logical_pos += (int64_t)bytes;
        return 0;
    }

    if (mp4_file_flush_internal(ctx) != 0) {
        return -1;
    }

    if (bytes >= sizeof(ctx->cache)) {
        if (bytes != (uint64_t)fwrite((void *)data, 1, (u32)bytes, (FILE *)ctx->file)) {
            return -1;
        }
        ctx->logical_pos += (int64_t)bytes;
        return 0;
    }

    memcpy(ctx->cache, data, (size_t)bytes);
    ctx->cache_len = (uint32_t)bytes;
    ctx->logical_pos += (int64_t)bytes;
    return 0;
}

static int mp4_file_seek(void *param, int64_t offset)
{
    mp4_file_io_ctx_t *ctx = (mp4_file_io_ctx_t *)param;
    int ret;

    if (ctx == NULL || ctx->file == NULL || offset > INT_MAX || offset < INT_MIN) {
        return -1;
    }

    if (mp4_file_flush_internal(ctx) != 0) {
        return -1;
    }

    ret = fseek((FILE *)ctx->file, (int)offset, offset >= 0 ? SEEK_SET : SEEK_END);
    if (ret == 0) {
        ctx->logical_pos = (int64_t)ftell((FILE *)ctx->file);
    }
    return ret;
}

static int64_t mp4_file_tell(void *param)
{
    mp4_file_io_ctx_t *ctx = (mp4_file_io_ctx_t *)param;

    if (ctx == NULL || ctx->file == NULL) {
        return -1;
    }

    return ctx->logical_pos;
}

const struct mov_buffer_t *mp4_file_io(void)
{
    static struct mov_buffer_t s_io = {
        .open = mp4_file_open,
        .close = mp4_file_close,
        .read = mp4_file_read,
        .write = mp4_file_write,
        .seek = mp4_file_seek,
        .tell = mp4_file_tell,
    };
    return &s_io;
}
