#ifndef MPI_VIDEO_NET_H
#define MPI_VIDEO_NET_H

int mpi_video_net_open_stream(const char *path, int w, int h, int fps, int abr);
int mpi_video_net_close_stream(void);
int mpi_video_net_get_status(void);
const char *mpi_video_net_get_last_path(void);
int mpi_video_net_get_last_params(int *w, int *h, int *fps, int *abr);
int mpi_video_net_get_uvc_resolution(int *w, int *h);
int mpi_video_net_get_reso_count(void);
int mpi_video_net_get_reso_by_index(int index, int *w, int *h);

int  mpi_video_net_start_rec(const char *path);
void mpi_video_net_stop_rec(void);
int  mpi_video_net_get_rec_status(void);

#endif /* MPI_VIDEO_NET_H */
