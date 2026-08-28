#ifndef MPI_PUBLIC_VO_H
#define MPI_PUBLIC_VO_H

#include "mpi_common.h"

typedef int mpi_vo_chn_t;

typedef struct {
    char fb_name[10];
    uint8_t fb_num;
    uint16_t fb_width;
    uint16_t fb_height;
    uint16_t window_width;
    uint16_t window_height;
    uint16_t left;
    uint16_t top;
    uint8_t fb_format;
    uint8_t rotate;
    uint8_t mirror;
    uint8_t combine;

    int (*fb_draw_end_callback)(void *priv);//fb绘制完回调
} mpi_vo_chn_attr_t;

mpi_ret_t mpi_vo_find_free_chn(mpi_vo_chn_t *chn);
mpi_ret_t mpi_vo_create_chn(mpi_vo_chn_t chn, const mpi_vo_chn_attr_t *attr);
mpi_ret_t mpi_vo_destroy_chn(mpi_vo_chn_t chn);
mpi_ret_t mpi_vo_set_chn_attr(mpi_vo_chn_t chn, const mpi_vo_chn_attr_t *attr);
mpi_ret_t mpi_vo_start_chn(mpi_vo_chn_t chn);
mpi_ret_t mpi_vo_stop_chn(mpi_vo_chn_t chn);

#endif /* MPI_PUBLIC_VO_H */

