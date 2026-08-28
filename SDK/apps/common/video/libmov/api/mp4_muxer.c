#include "mp4_muxer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmov_alloc.h"
#include "mp4_file_io.h"
#include "mp4-writer.h"

struct mp4_muxer_t {
    mp4_file_io_ctx_t file_buffer;
    struct mp4_writer_t *writer;
    int video_track;
    int audio_track;
    char path[MP4_FILE_IO_PATH_LEN];
};

static void mp4_muxer_reset(mp4_muxer_t *muxer)
{
    if (muxer == NULL) {
        return;
    }

    memset(muxer, 0, sizeof(*muxer));
    muxer->video_track = -1;
    muxer->audio_track = -1;
}

mp4_muxer_t *mp4_muxer_open(const char *path, int flags)
{
    mp4_muxer_t *muxer;
    size_t path_len;

    if (path == NULL) {
        printf("[mp4_muxer] open failed: path is null\n");
        return NULL;
    }

    path_len = strlen(path);
    if (path_len == 0u || path_len >= MP4_FILE_IO_PATH_LEN) {
        printf("[mp4_muxer] open failed: invalid path length=%u\n", (unsigned)path_len);
        return NULL;
    }

    muxer = (mp4_muxer_t *)libmov_calloc(1, sizeof(*muxer));
    if (muxer == NULL) {
        printf("[mp4_muxer] open failed: calloc muxer failed\n");
        return NULL;
    }

    mp4_muxer_reset(muxer);
    memcpy(muxer->path, path, path_len + 1u);

    if (mp4_file_io_init(&muxer->file_buffer, path, "w+") != 0) {
        printf("[mp4_muxer] open failed: mp4_file_io_init failed, path=%s\n", path);
        libmov_free(muxer);
        return NULL;
    }

    muxer->writer = mp4_writer_create(0, mp4_file_io(), &muxer->file_buffer, flags);
    if (muxer->writer == NULL) {
        printf("[mp4_muxer] open failed: mp4_writer_create failed, path=%s flags=%d\n", path, flags);
        libmov_free(muxer);
        return NULL;
    }

    return muxer;
}

int mp4_muxer_add_video(mp4_muxer_t *muxer, mp4_muxer_object_t object, int width, int height, const void *extra_data, size_t extra_data_size)
{
    if (muxer == NULL || muxer->writer == NULL || width <= 0 || height <= 0) {
        printf("[mp4_muxer] add_video failed: invalid args muxer=%p writer=%p width=%d height=%d\n",
               (void *)muxer,
               muxer ? (void *)muxer->writer : NULL,
               width,
               height);
        return -1;
    }
    if (muxer->video_track >= 0) {
        printf("[mp4_muxer] add_video failed: video track already exists=%d\n", muxer->video_track);
        return -1;
    }

    muxer->video_track = mp4_writer_add_video(muxer->writer, object, width, height, extra_data, extra_data_size);
    if (muxer->video_track < 0) {
        printf("[mp4_muxer] add_video failed: mp4_writer_add_video failed, object=%u width=%d height=%d\n",
               (unsigned)object,
               width,
               height);
        return -1;
    }
    return 0;
}

int mp4_muxer_add_audio(mp4_muxer_t *muxer,
                        mp4_muxer_object_t object,
                        int channel_count,
                        int bits_per_sample,
                        int sample_rate)
{
    if (muxer == NULL || muxer->writer == NULL ||
        channel_count <= 0 || bits_per_sample <= 0 || sample_rate <= 0) {
        printf("[mp4_muxer] add_audio failed: invalid args muxer=%p writer=%p ch=%d bits=%d rate=%d\n",
               (void *)muxer,
               muxer ? (void *)muxer->writer : NULL,
               channel_count,
               bits_per_sample,
               sample_rate);
        return -1;
    }
    if (muxer->audio_track >= 0) {
        printf("[mp4_muxer] add_audio failed: audio track already exists=%d\n", muxer->audio_track);
        return -1;
    }

    muxer->audio_track = mp4_writer_add_audio(muxer->writer,
                         object,
                         channel_count,
                         bits_per_sample,
                         sample_rate,
                         NULL,
                         0);
    if (muxer->audio_track < 0) {
        printf("[mp4_muxer] add_audio failed: mp4_writer_add_audio failed, object=%u ch=%d bits=%d rate=%d\n",
               (unsigned)object,
               channel_count,
               bits_per_sample,
               sample_rate);
        return -1;
    }
    return 0;
}

int mp4_muxer_write_video(mp4_muxer_t *muxer,
                          const void *data,
                          size_t bytes,
                          uint32_t pts,
                          uint32_t dts,
                          int flags)
{
    if (muxer == NULL || muxer->writer == NULL || muxer->video_track < 0 || data == NULL || bytes == 0u) {
        printf("[mp4_muxer] write_video failed: invalid args muxer=%p writer=%p track=%d data=%p bytes=%u\n",
               (void *)muxer,
               muxer ? (void *)muxer->writer : NULL,
               muxer ? muxer->video_track : -1,
               data,
               (unsigned)bytes);
        return -1;
    }

    if (mp4_writer_write(muxer->writer, muxer->video_track, data, bytes, pts, dts, flags) != 0) {
        printf("[mp4_muxer] write_video failed: mp4_writer_write failed, track=%d pts=%u dts=%u flags=%d bytes=%u\n",
               muxer->video_track,
               pts,
               dts,
               flags,
               (unsigned)bytes);
        return -1;
    }
    return 0;
}

int mp4_muxer_write_audio(mp4_muxer_t *muxer,
                          const void *data,
                          size_t bytes,
                          uint32_t pts,
                          uint32_t dts,
                          int flags)
{
    if (muxer == NULL || muxer->writer == NULL || muxer->audio_track < 0 || data == NULL || bytes == 0u) {
        printf("[mp4_muxer] write_audio failed: invalid args muxer=%p writer=%p track=%d data=%p bytes=%u\n",
               (void *)muxer,
               muxer ? (void *)muxer->writer : NULL,
               muxer ? muxer->audio_track : -1,
               data,
               (unsigned)bytes);
        return -1;
    }

    if (mp4_writer_write(muxer->writer, muxer->audio_track, data, bytes, pts, dts, flags) != 0) {
        printf("[mp4_muxer] write_audio failed: mp4_writer_write failed, track=%d pts=%u dts=%u flags=%d bytes=%u\n",
               muxer->audio_track,
               pts,
               dts,
               flags,
               (unsigned)bytes);
        return -1;
    }
    return 0;
}

void mp4_muxer_close(mp4_muxer_t *muxer)
{
    if (muxer == NULL) {
        return;
    }
    printf("[mp4_muxer] Closing muxer for %s\n", muxer->path);
    if (muxer->writer != NULL) {
        mp4_writer_destroy(muxer->writer);
        muxer->writer = NULL;
    }
    printf("[mp4_muxer] Closed muxer for %s\n", muxer->path);

    libmov_free(muxer);
}

