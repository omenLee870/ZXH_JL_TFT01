#ifndef MPI_PUBLIC_VENC_H
#define MPI_PUBLIC_VENC_H

#include "mpi_common.h"

typedef int mpi_venc_chn_t;
typedef int mpi_venc_dev_t;

typedef enum {
    MPI_VENC_TYPE_JPEG = 0,
    MPI_VENC_TYPE_JPEG_NCH = 1,
    MPI_VENC_TYPE_SOFT_264 = 2,
} mpi_venc_type_t;

typedef enum {
    MPI_VENC_INPUT_OFFLINE = 0,
    MPI_VENC_INPUT_ONLINE,
} mpi_venc_input_mode_t;

typedef struct {
    mpi_venc_dev_t hw_dev;
    uint8_t type;
    uint8_t input_mode;
    uint8_t quality;
    uint16_t in_width;
    uint16_t in_height;
    mpi_pixel_format_t in_format;
    mpi_yuv_order_t in_yuv_order;
    void *buf_addr;
    uint32_t buf_size;
    uint32_t fps;
    uint32_t abr_kbps;
    uint32_t stream_buf_size;
    uint8_t photo_mode;
    const char *photo_path;
    uint8_t thumbnail_create;
    uint8_t *head_app1;
    uint32_t head_app1_len;
    uint32_t gop;
    uint8_t qp_min;
    uint8_t qp_max;
    uint8_t thread_num;
    uint8_t thread_prio;
    void (*photo_file_cb)(void *fp, void *priv);
    void *photo_file_cb_priv;
} mpi_venc_chn_attr_t;

mpi_ret_t mpi_venc_find_free_chn(mpi_venc_chn_t *chn);
mpi_ret_t mpi_venc_create_chn(mpi_venc_chn_t chn, const mpi_venc_chn_attr_t *attr);
mpi_ret_t mpi_venc_destroy_chn(mpi_venc_chn_t chn);
mpi_ret_t mpi_venc_start_chn(mpi_venc_chn_t chn);
mpi_ret_t mpi_venc_stop_chn(mpi_venc_chn_t chn);
mpi_ret_t mpi_venc_set_chn_attr(mpi_venc_chn_t chn, const mpi_venc_chn_attr_t *attr);
mpi_ret_t mpi_venc_get_stream(mpi_venc_chn_t chn, mpi_stream_t *stream, int milli_sec);
mpi_ret_t mpi_venc_release_stream(mpi_venc_chn_t chn, const mpi_stream_t *stream);
mpi_ret_t mpi_venc_wait_photo_done(mpi_venc_chn_t chn, int milli_sec);
mpi_ret_t mpi_venc_send_frame(mpi_venc_chn_t chn, const mpi_video_frame_info_t *frame);
mpi_ret_t mpi_venc_reset_bits_rate(mpi_venc_chn_t chn, uint32_t bits_rate);

#endif /* MPI_PUBLIC_VENC_H */

