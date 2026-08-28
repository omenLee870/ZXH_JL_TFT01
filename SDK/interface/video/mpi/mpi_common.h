#ifndef MPI_PUBLIC_COMMON_H
#define MPI_PUBLIC_COMMON_H

#include <stdint.h>

#define MPI_SUCCESS            0
#define MPI_ERR_INVALID_ARG   -1
#define MPI_ERR_NOMEM         -2
#define MPI_ERR_BUSY          -3
#define MPI_ERR_NOT_FOUND     -4
#define MPI_ERR_NOT_READY     -5
#define MPI_ERR_NOT_SUPPORT   -6
#define MPI_ERR_EXIST         -7
#define MPI_ERR_STATE         -8
#define MPI_ERR_BIND          -9
#define MPI_ERR_EMPTY        -10
#define MPI_ERR_FULL         -11

typedef int mpi_ret_t;

typedef uint8_t mpi_pixel_format_t;
typedef uint8_t mpi_yuv_order_t;

enum {
    MPI_PIXEL_FMT_YUV422_PACKED = 0,
    MPI_PIXEL_FMT_YUV444_PACKED = 1,
    MPI_PIXEL_FMT_YUV420SP = 2,
    MPI_PIXEL_FMT_RAW8 = 3,
    MPI_PIXEL_FMT_RAW10 = 4,
    MPI_PIXEL_FMT_RAW12 = 5,
};

enum {
    MPI_YUV_ORDER_YUYV = 0,
    MPI_YUV_ORDER_YVYU,
    MPI_YUV_ORDER_UYVY,
    MPI_YUV_ORDER_VYUY,
};

typedef enum {
    MPI_MOD_ID_SYS = 0,
    MPI_MOD_ID_VI,
    MPI_MOD_ID_ISP,
    MPI_MOD_ID_VPSS,
    MPI_MOD_ID_VENC,
    MPI_MOD_ID_VO,
    MPI_MOD_ID_VDEC,
    MPI_MOD_ID_REP,
    MPI_MOD_ID_BUTT
} mpi_mod_id_t;

#if defined CONFIG_CPU_BR33
#define MPI_VI_MAX_DEVS      1
#define MPI_VPSS_MAX_GRPS    2
#define MPI_VPSS_MAX_CHNS    2
#define MPI_VENC_MAX_CHNS    8
#define MPI_VENC_MAX_HW_DEVS 1
#define MPI_VO_MAX_CHNS      8
#define MPI_VO_MAX_FB_NUM    4
#define MPI_VDEC_MAX_CHNS    8
#define MPI_VDEC_MAX_HW_DEVS 1
#define MPI_REP_MAX_DEVS     2
#elif defined CONFIG_CPU_DV25
#define MPI_VI_MAX_DEVS      2
#define MPI_VPSS_MAX_GRPS    5
#define MPI_VPSS_MAX_CHNS    8
#define MPI_VENC_MAX_CHNS    10
#define MPI_VENC_MAX_HW_DEVS 2
#define MPI_VO_MAX_CHNS      8
#define MPI_VO_MAX_FB_NUM    4
#define MPI_VDEC_MAX_CHNS    8
#define MPI_VDEC_MAX_HW_DEVS 2
#define MPI_REP_MAX_DEVS     2
#else
#define MPI_VI_MAX_DEVS      0
#define MPI_VPSS_MAX_GRPS    0
#define MPI_VPSS_MAX_CHNS    0
#define MPI_VENC_MAX_CHNS    0
#define MPI_VENC_MAX_HW_DEVS 0
#define MPI_VO_MAX_CHNS      0
#define MPI_VO_MAX_FB_NUM    0
#define MPI_VDEC_MAX_CHNS    0
#define MPI_VDEC_MAX_HW_DEVS 0
#define MPI_REP_MAX_DEVS     0
#endif

typedef struct {
    int mod_id;
    int dev_id;
    int chn_id;
} mpi_chn_t;

typedef struct {
    uint8_t created;
    uint8_t started;
    uint8_t bound;
} mpi_chn_state_t;

#define MPI_STREAM_TYPE_MASK    0xffff0000u
#define MPI_STREAM_TYPE_UNKNOWN 0x00000000u
#define MPI_STREAM_TYPE_JPEG    0x00010000u

#define MPI_STREAM_TYPE_H264    0x00020000u
#define MPI_STREAM_H264_SPS     0x00000001u
#define MPI_STREAM_H264_PPS     0x00000002u
#define MPI_STREAM_H264_SEI     0x00000004u
#define MPI_STREAM_H264_IDR     0x00000008u
#define MPI_STREAM_H264_P       0x00000010u
#define MPI_STREAM_H264_TYPE(flags) (MPI_STREAM_TYPE_H264 | (flags))

typedef struct {
    const uint8_t *addr;
    uint32_t len;
    uint32_t seq;
    uint32_t type;
    uint32_t timestamp_ms;
} mpi_stream_t;

typedef struct {
    uint8_t *y_addr;
    uint8_t *c_addr;
    uint32_t y_len;
    uint32_t c_len;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    mpi_pixel_format_t format;
    mpi_yuv_order_t yuv_order;
    uint32_t seq;
    uint32_t blk_index;
} mpi_video_frame_info_t;

#endif /* MPI_PUBLIC_COMMON_H */

