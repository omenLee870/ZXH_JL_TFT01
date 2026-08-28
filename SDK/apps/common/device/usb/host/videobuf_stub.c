#include "system/includes.h"
#include "videobuf.h"
#include "video.h"

void videobuf_queue_init(struct videobuf_queue *q, int align, const char *name) {}
int videobuf_reqbufs(struct videobuf_queue *q, struct video_reqbufs *req)
{
    return 0;
}
int videobuf_dqbuf(struct videobuf_queue *q, struct video_buffer *b)
{
    return -1;
}
int videobuf_qbuf(struct videobuf_queue *q, struct video_buffer *b)
{
    return 0;
}
int videobuf_streamon(struct videobuf_queue *q, u8 *channel)
{
    return 0;
}
int videobuf_streamoff(struct videobuf_queue *q, u8 channel)
{
    return 0;
}
struct videobuf_buffer *videobuf_stream_alloc(struct videobuf_queue *q, u32 size)
{
    return NULL;
}
struct videobuf_buffer *videobuf_stream_realloc(struct videobuf_queue *q, struct videobuf_buffer *b, int size)
{
    return NULL;
}
u32 videobuf_stream_free_space(struct videobuf_queue *q)
{
    return 0;
}
void videobuf_stream_free(struct videobuf_queue *q, struct videobuf_buffer *b) {}
int videobuf_stream_finish(struct videobuf_queue *q, struct videobuf_buffer *b)
{
    return 0;
}
void videobuf_queue_release(struct videobuf_queue *q) {}

void *video_buf_malloc(struct video_device *dev, u32 size)
{
    return NULL;
}
void *video_buf_realloc(struct video_device *dev, void *buf, int size)
{
    return NULL;
}
void video_buf_free(struct video_device *dev, void *buf) {}
u32 video_buf_free_space(struct video_device *dev)
{
    return 0;
}
u32 video_buf_size(void *buf)
{
    return 0;
}
void video_buf_stream_finish(struct video_device *dev, void *buf) {}

#include "device/device.h"
const struct device_operations video_dev_ops = {0};
