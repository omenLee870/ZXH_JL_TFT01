#ifndef MPI_PUBLIC_VDEC_H
#define MPI_PUBLIC_VDEC_H

#include "mpi_common.h"

typedef int mpi_vdec_chn_t;
typedef int mpi_vdec_dev_t;

typedef enum {
    MPI_VDEC_TYPE_JPEG = 0,
    MPI_VDEC_TYPE_SOFT_H264,
} mpi_vdec_type_t;

typedef enum {
    MPI_VDEC_OUTPUT_FRAME = 0,
    MPI_VDEC_OUTPUT_LINE,
} mpi_vdec_output_mode_t;

typedef void (*mpi_vdec_stream_notify_fn)(void *priv,
        const mpi_stream_t *stream,
        mpi_ret_t ret);

typedef struct {
    mpi_vdec_dev_t hw_dev;
    mpi_vdec_type_t type;
    mpi_yuv_order_t out_yuv_order;
    mpi_vdec_output_mode_t output_mode;
    uint16_t line_cnt;
    void *frame_buf_addr;
    uint32_t frame_buf_size;
    uint32_t stream_buf_size; /* Soft-H264 unresolved tail cache size. */
} mpi_vdec_chn_attr_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t align_width;
    uint16_t align_height;
    mpi_pixel_format_t format;
} mpi_vdec_probe_info_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t align_width;
    uint16_t align_height;
    uint16_t line_cnt;
    uint16_t stride;
    mpi_pixel_format_t format;
    mpi_yuv_order_t yuv_order;
    mpi_vdec_output_mode_t output_mode;
    uint32_t y_len;
    uint32_t c_len;
    uint32_t block_count;
    uint32_t seq;
} mpi_vdec_pic_info_t;

mpi_ret_t mpi_vdec_find_free_chn(mpi_vdec_chn_t *chn);
mpi_ret_t mpi_vdec_create_chn(mpi_vdec_chn_t chn, const mpi_vdec_chn_attr_t *attr);
mpi_ret_t mpi_vdec_destroy_chn(mpi_vdec_chn_t chn);
mpi_ret_t mpi_vdec_start_chn(mpi_vdec_chn_t chn);
mpi_ret_t mpi_vdec_stop_chn(mpi_vdec_chn_t chn);
mpi_ret_t mpi_vdec_set_chn_attr(mpi_vdec_chn_t chn, const mpi_vdec_chn_attr_t *attr);
/* Default mode is blocking. Call after create and before start to switch mode. */
mpi_ret_t mpi_vdec_set_block_mode(mpi_vdec_chn_t chn,
                                  uint8_t blocking,
                                  mpi_vdec_stream_notify_fn notify,
                                  void *notify_priv);
/* Soft-H264 may modify stream->addr in place before return or notify callback. */
mpi_ret_t mpi_vdec_send_stream(mpi_vdec_chn_t chn, const mpi_stream_t *stream);
mpi_ret_t mpi_vdec_flush_stream(mpi_vdec_chn_t chn);
mpi_ret_t mpi_vdec_probe_pic_info(const mpi_stream_t *stream,
                                  mpi_vdec_probe_info_t *info);
mpi_ret_t mpi_vdec_get_pic_info(mpi_vdec_chn_t chn, const mpi_stream_t *stream, mpi_vdec_pic_info_t *info);
mpi_ret_t mpi_vdec_get_frame(mpi_vdec_chn_t chn, mpi_video_frame_info_t *frame, int milli_sec);
mpi_ret_t mpi_vdec_release_frame(mpi_vdec_chn_t chn, const mpi_video_frame_info_t *frame);

#endif /* MPI_PUBLIC_VDEC_H */

