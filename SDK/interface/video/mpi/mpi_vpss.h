#ifndef MPI_PUBLIC_VPSS_H
#define MPI_PUBLIC_VPSS_H

#include "mpi_common.h"

typedef int mpi_vpss_grp_t;
typedef int mpi_vpss_chn_t;

typedef enum {
    MPI_VPSS_OUTPUT_OFFLINE = 0,
    MPI_VPSS_OUTPUT_ONLINE,
} mpi_vpss_output_mode_t;

typedef enum {
    MPI_VPSS_SRC_FMT_YUV = 0,
    MPI_VPSS_SRC_FMT_RAW,
} mpi_vpss_src_format_t;

typedef enum {
    MPI_VPSS_HW_AUTO = 0,
    MPI_VPSS_HW_IMC_DST0,
    MPI_VPSS_HW_IMC_DST1,
    MPI_VPSS_HW_IMC_DST2,
    MPI_VPSS_HW_IMC_DST3,
    MPI_VPSS_HW_IMC_DST4,
    MPI_VPSS_HW_IMC_DST5,
    MPI_VPSS_HW_IMC_DST6,
    MPI_VPSS_HW_IMC_DST7,
} mpi_vpss_hw_id_t;

typedef struct {
    uint16_t src_width;
    uint16_t src_height;
    uint8_t src_format;
    uint8_t pre_scale_enable;
    uint8_t width_scale_factor;
    uint8_t height_scale_factor;
    mpi_pixel_format_t src_pixel_format;
    mpi_yuv_order_t src_yuv_order;
    uint16_t src_fps;
    uint8_t replay_isp_mode;
} mpi_vpss_grp_attr_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    mpi_pixel_format_t format;
    mpi_yuv_order_t yuv_order;
    uint8_t mirror;
    uint8_t output_mode;
    mpi_vpss_hw_id_t hw_id;
    uint16_t dst_fps;
    void *buf_addr;
    uint32_t buf_size;
} mpi_vpss_chn_attr_t;

typedef enum {
    MPI_VPSS_OSD_MODE_1BIT = 1,
    MPI_VPSS_OSD_MODE_2BIT = 2,
} mpi_vpss_osd_mode_t;

typedef struct {
    uint8_t mode;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint32_t color[4];
    void *buf_addr;
    uint32_t buf_size;
} mpi_vpss_osd_attr_t;

typedef struct {
    uint16_t x_start;    /* crop start x */
    uint16_t y_start;    /* crop start y */
    uint16_t crop_w;     /* crop width */
    uint16_t crop_h;     /* crop height */
} mpi_vpss_crop_param_t;

mpi_ret_t mpi_vpss_find_free_grp(mpi_vpss_grp_t *grp);
mpi_ret_t mpi_vpss_find_free_chn(mpi_vpss_grp_t grp, mpi_vpss_chn_t *chn);
mpi_ret_t mpi_vpss_create_grp(mpi_vpss_grp_t grp, const mpi_vpss_grp_attr_t *attr);
mpi_ret_t mpi_vpss_destroy_grp(mpi_vpss_grp_t grp);
mpi_ret_t mpi_vpss_set_grp_attr(mpi_vpss_grp_t grp, const mpi_vpss_grp_attr_t *attr);
mpi_ret_t mpi_vpss_get_grp_attr(mpi_vpss_grp_t grp, mpi_vpss_grp_attr_t *attr);
mpi_ret_t mpi_vpss_start_grp(mpi_vpss_grp_t grp);
mpi_ret_t mpi_vpss_stop_grp(mpi_vpss_grp_t grp);
mpi_ret_t mpi_vpss_create_chn(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn, const mpi_vpss_chn_attr_t *attr);
mpi_ret_t mpi_vpss_destroy_chn(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn);
mpi_ret_t mpi_vpss_set_chn_attr(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn, const mpi_vpss_chn_attr_t *attr);
mpi_ret_t mpi_vpss_get_chn_attr(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn, mpi_vpss_chn_attr_t *attr);
mpi_ret_t mpi_vpss_enable_chn(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn);
mpi_ret_t mpi_vpss_disable_chn(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn);
mpi_ret_t mpi_vpss_get_chn_frame(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn, mpi_video_frame_info_t *frame, int milli_sec);
mpi_ret_t mpi_vpss_release_chn_frame(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn, const mpi_video_frame_info_t *frame);
mpi_ret_t mpi_vpss_send_frame(mpi_vpss_grp_t grp, const mpi_video_frame_info_t *frame, int timeout_ms);
mpi_ret_t mpi_vpss_set_chn_osd_attr(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn, const mpi_vpss_osd_attr_t *attr);
mpi_ret_t mpi_vpss_get_chn_osd_attr(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn, mpi_vpss_osd_attr_t *attr);
mpi_ret_t mpi_vpss_update_chn_osd_attr(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn, const mpi_vpss_osd_attr_t *attr);
mpi_ret_t mpi_vpss_enable_chn_osd(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn);
mpi_ret_t mpi_vpss_disable_chn_osd(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn);

mpi_ret_t mpi_vpss_chn_set_crop(mpi_vpss_grp_t grp,
                                mpi_vpss_chn_t chn,
                                const mpi_vpss_crop_param_t *param);
mpi_ret_t mpi_vpss_chn_get_crop(mpi_vpss_grp_t grp,
                                mpi_vpss_chn_t chn,
                                mpi_vpss_crop_param_t *param);
mpi_ret_t mpi_vpss_chn_disable_crop(mpi_vpss_grp_t grp,
                                    mpi_vpss_chn_t chn);

#endif /* MPI_PUBLIC_VPSS_H */


