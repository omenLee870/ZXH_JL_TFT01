#ifndef MPI_PUBLIC_SYS_H
#define MPI_PUBLIC_SYS_H

#include "mpi_common.h"

mpi_ret_t mpi_sys_bind(const mpi_chn_t *src, const mpi_chn_t *dst);
mpi_ret_t mpi_sys_unbind(const mpi_chn_t *src, const mpi_chn_t *dst);
int mpi_sys_has_bind(const mpi_chn_t *chn);

#endif /* MPI_PUBLIC_SYS_H */

