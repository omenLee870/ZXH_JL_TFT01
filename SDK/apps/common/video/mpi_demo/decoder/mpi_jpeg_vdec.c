#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os/os_api.h"

#include "mpi_common.h"
#include "mpi_vdec.h"

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
#define MPI_JPEG_VDEC_MALLOC(size) malloc_psram(size)
#define MPI_JPEG_VDEC_FREE(ptr)    free_psram(ptr)
#else
#define MPI_JPEG_VDEC_MALLOC(size) malloc(size)
#define MPI_JPEG_VDEC_FREE(ptr)    free(ptr)
#endif

#define MPI_JPEG_VDEC_IN_PATH              "storage/sd0/C/jpg/IMG_0000.JPG"
#define MPI_JPEG_VDEC_OUT_PATH             "storage/sd0/C/out.yuv"
#define MPI_JPEG_VDEC_FRAME_EXTRA          1024u
#define MPI_JPEG_VDEC_TIMEOUT_MS           1000

static uint32_t mpi_jpeg_vdec_frame_size(const mpi_vdec_probe_info_t *info)
{
    if (info == NULL) {
        return 0;
    }

    switch (info->format) {
    case MPI_PIXEL_FMT_YUV422_PACKED:
        return (uint32_t)info->align_width * info->align_height * 2u;
    case MPI_PIXEL_FMT_YUV420SP:
        return (uint32_t)info->align_width * info->align_height * 3u / 2u;
    case MPI_PIXEL_FMT_YUV444_PACKED:
        return (uint32_t)info->align_width * info->align_height * 3u;
    default:
        return 0;
    }
}

static int mpi_jpeg_vdec_read_file(const char *path,
                                   uint8_t **buf,
                                   uint32_t *len)
{
    FILE *fp;
    long file_len;
    uint8_t *data;

    if (path == NULL || buf == NULL || len == NULL) {
        return -1;
    }

    *buf = NULL;
    *len = 0;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("[MPI_JPEG_VDEC] open input file failed: %s\n", path);
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("[MPI_JPEG_VDEC] seek input file end failed\n");
        fclose(fp);
        return -1;
    }

    file_len = ftell(fp);
    if (file_len <= 0) {
        printf("[MPI_JPEG_VDEC] invalid input file size: %ld\n", file_len);
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        printf("[MPI_JPEG_VDEC] seek input file start failed\n");
        fclose(fp);
        return -1;
    }

    data = (uint8_t *)MPI_JPEG_VDEC_MALLOC((uint32_t)file_len);
    if (data == NULL) {
        printf("[MPI_JPEG_VDEC] malloc input buffer failed: %ld\n", file_len);
        fclose(fp);
        return -1;
    }

    if (fread(data, 1, (size_t)file_len, fp) != (size_t)file_len) {
        printf("[MPI_JPEG_VDEC] read input file failed: %s\n", path);
        MPI_JPEG_VDEC_FREE(data);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    *buf = data;
    *len = (uint32_t)file_len;

    return 0;
}

static mpi_ret_t mpi_jpeg_vdec_write_frame(const char *path,
        const mpi_video_frame_info_t *frame)
{
    FILE *fp;

    if (path == NULL || frame == NULL || frame->y_addr == NULL || frame->y_len == 0) {
        return MPI_ERR_INVALID_ARG;
    }

    fp = fopen(path, "w+");
    if (fp == NULL) {
        printf("[MPI_JPEG_VDEC] open output file failed: %s\n", path);
        return MPI_ERR_INVALID_ARG;
    }

    if (fwrite(frame->y_addr, 1, frame->y_len, fp) != frame->y_len) {
        printf("[MPI_JPEG_VDEC] write Y payload failed, len=%u\n", frame->y_len);
        fclose(fp);
        return MPI_ERR_STATE;
    }

    if (frame->c_addr != NULL && frame->c_len > 0) {
        if (fwrite(frame->c_addr, 1, frame->c_len, fp) != frame->c_len) {
            printf("[MPI_JPEG_VDEC] write C payload failed, len=%u\n", frame->c_len);
            fclose(fp);
            return MPI_ERR_STATE;
        }
    }

    fclose(fp);
    return MPI_SUCCESS;
}

void mpi_jpeg_vdec_test(void)
{
    uint8_t *jpg_buf = NULL;
    uint32_t jpg_len = 0;
    uint32_t frame_buf_size;
    mpi_ret_t ret;
    mpi_stream_t stream = {0};
    mpi_vdec_probe_info_t probe_info = {0};
    mpi_vdec_chn_attr_t vdec_attr = {0};
    mpi_video_frame_info_t frame = {0};
    mpi_vdec_chn_t vdec_chn = 0;

    printf("[MPI_JPEG_VDEC] start, input=%s\n", MPI_JPEG_VDEC_IN_PATH);

    if (mpi_jpeg_vdec_read_file(MPI_JPEG_VDEC_IN_PATH, &jpg_buf, &jpg_len) != 0) {
        return;
    }

    stream.addr = jpg_buf;
    stream.len = jpg_len;

    ret = mpi_vdec_probe_pic_info(&stream, &probe_info);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_JPEG_VDEC] probe pic info failed: %d, len=%u\n", ret, jpg_len);
        goto out_free_jpg;
    }

    printf("[MPI_JPEG_VDEC] input %ux%u align=%ux%u fmt=%u, jpg_len=%u\n",
           probe_info.width,
           probe_info.height,
           probe_info.align_width,
           probe_info.align_height,
           probe_info.format,
           jpg_len);

    frame_buf_size = mpi_jpeg_vdec_frame_size(&probe_info);
    if (frame_buf_size == 0) {
        printf("[MPI_JPEG_VDEC] unsupported output format: %u\n", probe_info.format);
        goto out_free_jpg;
    }

    ret = mpi_vdec_find_free_chn(&vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_JPEG_VDEC] find free VDEC chn failed: %d\n", ret);
        goto out_free_jpg;
    }

    vdec_attr.hw_dev = 0;
    vdec_attr.type = MPI_VDEC_TYPE_JPEG;
    vdec_attr.out_yuv_order = MPI_YUV_ORDER_YUYV;
    vdec_attr.output_mode = MPI_VDEC_OUTPUT_FRAME;
    vdec_attr.frame_buf_size = frame_buf_size + MPI_JPEG_VDEC_FRAME_EXTRA;

    ret = mpi_vdec_create_chn(vdec_chn, &vdec_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_JPEG_VDEC] create VDEC chn failed: %d\n", ret);
        goto out_free_jpg;
    }

    ret = mpi_vdec_start_chn(vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_JPEG_VDEC] start VDEC chn failed: %d\n", ret);
        goto out_destroy_vdec;
    }

    ret = mpi_vdec_send_stream(vdec_chn, &stream);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_JPEG_VDEC] send JPEG stream failed: %d\n", ret);
        goto out_stop_vdec;
    }

    ret = mpi_vdec_get_frame(vdec_chn, &frame, MPI_JPEG_VDEC_TIMEOUT_MS);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_JPEG_VDEC] get decoded frame failed: %d\n", ret);
        goto out_stop_vdec;
    }

    printf("[MPI_JPEG_VDEC] decoded frame %ux%u stride=%u fmt=%u order=%u y_len=%u c_len=%u\n",
           frame.width,
           frame.height,
           frame.stride,
           frame.format,
           frame.yuv_order,
           frame.y_len,
           frame.c_len);

    ret = mpi_jpeg_vdec_write_frame(MPI_JPEG_VDEC_OUT_PATH, &frame);
    if (ret == MPI_SUCCESS) {
        printf("[MPI_JPEG_VDEC] write decoded frame to %s\n", MPI_JPEG_VDEC_OUT_PATH);
    }

    (void)mpi_vdec_release_frame(vdec_chn, &frame);

out_stop_vdec:
    (void)mpi_vdec_stop_chn(vdec_chn);
out_destroy_vdec:
    (void)mpi_vdec_destroy_chn(vdec_chn);
out_free_jpg:
    MPI_JPEG_VDEC_FREE(jpg_buf);
}
