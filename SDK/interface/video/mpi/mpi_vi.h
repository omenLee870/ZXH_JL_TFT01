#ifndef MPI_PUBLIC_VI_H
#define MPI_PUBLIC_VI_H

#include "mpi_common.h"

typedef int mpi_vi_dev_t;

typedef struct {
    uint8_t camera_id;
    char *sensor_name;
} mpi_vi_dev_attr_t;

typedef struct {
    uint8_t enabled;
    uint8_t is_raw;
    uint16_t width;
    uint16_t height;
    uint16_t fps;
} mpi_vi_output_info_t;

mpi_ret_t mpi_vi_find_free_dev(mpi_vi_dev_t *vi_dev);
mpi_ret_t mpi_vi_set_dev_attr(mpi_vi_dev_t vi_dev, const mpi_vi_dev_attr_t *attr);
mpi_ret_t mpi_vi_enable_dev(mpi_vi_dev_t vi_dev);
mpi_ret_t mpi_vi_disable_dev(mpi_vi_dev_t vi_dev);
mpi_ret_t mpi_vi_get_output_info(mpi_vi_dev_t vi_dev, mpi_vi_output_info_t *info);

#endif /* MPI_PUBLIC_VI_H */

