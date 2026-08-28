#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os/os_api.h"

#include "mpi_common.h"
#include "mpi_sys.h"
#include "mpi_vdec.h"
#include "mpi_vpss.h"

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
#define MPI_VDEC_VPSS_MALLOC(size) malloc_psram(size)
#define MPI_VDEC_VPSS_FREE(ptr)    free_psram(ptr)
#else
#define MPI_VDEC_VPSS_MALLOC(size) malloc(size)
#define MPI_VDEC_VPSS_FREE(ptr)    free(ptr)
#endif

/* 输入/输出文件 */
#define MPI_VDEC_VPSS_IN_PATH              "storage/sd0/C/jpg/IMG_0000.JPG"
#define MPI_VDEC_VPSS_OUT_PATH             "storage/sd0/C/out.yuv"
/* 解码输出行数 */
#define MPI_VDEC_VPSS_VDEC_LINE_CNT        16u
#define MPI_VDEC_VPSS_VDEC_CACHE_LINES     32u

/* vpss输出尺寸格式 */
#define VPSS_OUTPUT_WIDTH               320u
#define VPSS_OUTPUT_HEIGHT              240u
#define VPSS_OUTPUT_FORMAT              MPI_PIXEL_FMT_YUV420SP

/* MPI内部缓存管理需要额外内存 */
#define MPI_VDEC_VPSS_VDEC_FRAME_EXTRA     1024u

static uint32_t mpi_vdec_vpss_frame_size(uint16_t width,
        uint16_t height,
        mpi_pixel_format_t format)
{
    switch (format) {
    case MPI_PIXEL_FMT_YUV422_PACKED:
        return (uint32_t)width * height * 2u;
    case MPI_PIXEL_FMT_YUV420SP:
        return (uint32_t)width * height * 3u / 2u;
    case MPI_PIXEL_FMT_YUV444_PACKED:
        return (uint32_t)width * height * 3u;
    default:
        return 0;
    }
}

static int mpi_vdec_vpss_read_file(const char *path,
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
        printf("[MPI_VDEC_VPSS] open input file failed: %s\n", path);
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("[MPI_VDEC_VPSS] seek input file end failed\n");
        fclose(fp);
        return -1;
    }

    file_len = ftell(fp);
    if (file_len <= 0) {
        printf("[MPI_VDEC_VPSS] invalid input file size: %ld\n", file_len);
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        printf("[MPI_VDEC_VPSS] seek input file start failed\n");
        fclose(fp);
        return -1;
    }

    data = (uint8_t *)MPI_VDEC_VPSS_MALLOC((uint32_t)file_len);
    if (data == NULL) {
        printf("[MPI_VDEC_VPSS] malloc input buffer failed: %ld\n", file_len);
        fclose(fp);
        return -1;
    }

    if (fread(data, 1, (size_t)file_len, fp) != (size_t)file_len) {
        printf("[MPI_VDEC_VPSS] read input file failed: %s\n", path);
        MPI_VDEC_VPSS_FREE(data);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    *buf = data;
    *len = (uint32_t)file_len;

    return 0;
}

static mpi_ret_t mpi_vdec_vpss_write_frame(const char *path,
        const mpi_video_frame_info_t *frame)
{
    FILE *fp;

    if (path == NULL || frame == NULL || frame->y_addr == NULL || frame->y_len == 0) {
        return MPI_ERR_INVALID_ARG;
    }

    fp = fopen(path, "w+");
    if (fp == NULL) {
        printf("[MPI_VDEC_VPSS] open output file failed: %s\n", path);
        return MPI_ERR_INVALID_ARG;
    }

    if (fwrite(frame->y_addr, 1, frame->y_len, fp) != frame->y_len) {
        printf("[MPI_VDEC_VPSS] write Y payload failed, len=%u\n", frame->y_len);
        fclose(fp);
        return MPI_ERR_STATE;
    }

    if (frame->c_addr != NULL && frame->c_len > 0) {
        if (fwrite(frame->c_addr, 1, frame->c_len, fp) != frame->c_len) {
            printf("[MPI_VDEC_VPSS] write C payload failed, len=%u\n", frame->c_len);
            fclose(fp);
            return MPI_ERR_STATE;
        }
    }

    fclose(fp);
    return MPI_SUCCESS;
}

static void mpi_vdec_vpss_make_chn(int mod_id, int dev_id, int chn_id, mpi_chn_t *chn)
{
    if (chn == NULL) {
        return;
    }

    chn->mod_id = mod_id;
    chn->dev_id = dev_id;
    chn->chn_id = chn_id;
}

void mpi_vdec_vpss_test(void)
{
    uint8_t *jpg_buf = NULL;
    uint32_t jpg_len = 0;
    uint32_t vdec_buf_size;
    mpi_ret_t ret;
    mpi_stream_t stream = {0};
    mpi_vdec_probe_info_t probe_info = {0};
    mpi_vdec_chn_attr_t vdec_attr = {0};
    mpi_vpss_grp_attr_t grp_attr = {0};
    mpi_vpss_chn_attr_t chn_attr = {0};
    mpi_video_frame_info_t frame = {0};
    mpi_vdec_chn_t vdec_chn = 0;
    mpi_vpss_grp_t vpss_grp = 0;
    mpi_vpss_chn_t vpss_chn = 0;
    mpi_chn_t vdec_src = {0};
    mpi_chn_t vpss_dst = {0};

    printf("[MPI_VDEC_VPSS] start, input=%s\n", MPI_VDEC_VPSS_IN_PATH);

    if (mpi_vdec_vpss_read_file(MPI_VDEC_VPSS_IN_PATH, &jpg_buf, &jpg_len) != 0) {
        return;
    }

    stream.addr = jpg_buf;
    stream.len = jpg_len;

    ret = mpi_vdec_probe_pic_info(&stream, &probe_info);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] probe pic info failed: %d, len=%u\n", ret, jpg_len);
        goto out_free_jpg;
    }

    printf("[MPI_VDEC_VPSS] input %ux%u align=%ux%u fmt=%u, jpg_len=%u\n",
           probe_info.width,
           probe_info.height,
           probe_info.align_width,
           probe_info.align_height,
           probe_info.format,
           jpg_len);

    vdec_buf_size = mpi_vdec_vpss_frame_size(probe_info.align_width,
                    MPI_VDEC_VPSS_VDEC_CACHE_LINES,
                    probe_info.format) + MPI_VDEC_VPSS_VDEC_FRAME_EXTRA;
    if (vdec_buf_size == MPI_VDEC_VPSS_VDEC_FRAME_EXTRA) {
        printf("[MPI_VDEC_VPSS] unsupported input format: %u\n", probe_info.format);
        goto out_free_jpg;
    }

    ret = mpi_vdec_find_free_chn(&vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] find free VDEC chn failed: %d\n", ret);
        goto out_free_jpg;
    }

    vdec_attr.hw_dev = 0;
    vdec_attr.type = MPI_VDEC_TYPE_JPEG;
    vdec_attr.out_yuv_order = MPI_YUV_ORDER_YUYV;
    vdec_attr.output_mode = MPI_VDEC_OUTPUT_LINE;
    vdec_attr.line_cnt = MPI_VDEC_VPSS_VDEC_LINE_CNT;
    vdec_attr.frame_buf_size = vdec_buf_size;

    ret = mpi_vdec_create_chn(vdec_chn, &vdec_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] create VDEC chn failed: %d\n", ret);
        goto out_free_jpg;
    }

    ret = mpi_vpss_find_free_grp(&vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] find free VPSS grp failed: %d\n", ret);
        goto out_destroy_vdec;
    }

    grp_attr.src_width = probe_info.align_width;
    grp_attr.src_height = probe_info.align_height;
    grp_attr.src_format = MPI_VPSS_SRC_FMT_YUV;
    grp_attr.src_pixel_format = probe_info.format;
    grp_attr.src_yuv_order = MPI_YUV_ORDER_YUYV;

    ret = mpi_vpss_create_grp(vpss_grp, &grp_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] create VPSS grp failed: %d\n", ret);
        goto out_destroy_vdec;
    }

    ret = mpi_vpss_start_grp(vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] start VPSS grp failed: %d\n", ret);
        goto out_destroy_vpss_grp;
    }

    ret = mpi_vpss_find_free_chn(vpss_grp, &vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] find free VPSS chn failed: %d\n", ret);
        goto out_stop_vpss_grp;
    }

    chn_attr.width = VPSS_OUTPUT_WIDTH;
    chn_attr.height = VPSS_OUTPUT_HEIGHT;
    chn_attr.format = VPSS_OUTPUT_FORMAT;
    chn_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    // 2 frames cache
    chn_attr.buf_size = mpi_vdec_vpss_frame_size(chn_attr.width, chn_attr.height, chn_attr.format) * 2 + MPI_VDEC_VPSS_VDEC_FRAME_EXTRA;

    ret = mpi_vpss_create_chn(vpss_grp, vpss_chn, &chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] create VPSS chn failed: %d\n", ret);
        goto out_stop_vpss_grp;
    }

    ret = mpi_vpss_enable_chn(vpss_grp, vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] enable VPSS chn failed: %d\n", ret);
        goto out_destroy_vpss_chn;
    }

    mpi_vdec_vpss_make_chn(MPI_MOD_ID_VDEC, vdec_attr.hw_dev, vdec_chn, &vdec_src);
    mpi_vdec_vpss_make_chn(MPI_MOD_ID_VPSS, vpss_grp, 0, &vpss_dst);

    ret = mpi_vdec_start_chn(vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] start VDEC chn failed: %d\n", ret);
        goto out_disable_vpss_chn;
    }

    ret = mpi_sys_bind(&vdec_src, &vpss_dst);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] bind VDEC->VPSS failed: %d\n", ret);
        goto out_stop_vdec;
    }

    ret = mpi_vdec_send_stream(vdec_chn, &stream);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] send JPEG stream failed: %d\n", ret);
        goto out_unbind_vdec_vpss;
    }

    ret = mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &frame, 1000);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] get VPSS frame failed: %d\n", ret);
        goto out_unbind_vdec_vpss;
    }

    printf("[MPI_VDEC_VPSS] output frame %ux%u stride=%u fmt=%u order=%u y_len=%u c_len=%u\n",
           frame.width,
           frame.height,
           frame.stride,
           frame.format,
           frame.yuv_order,
           frame.y_len,
           frame.c_len);

    ret = mpi_vdec_vpss_write_frame(MPI_VDEC_VPSS_OUT_PATH, &frame);
    if (ret == MPI_SUCCESS) {
        printf("[MPI_VDEC_VPSS] write VPSS frame to %s\n", MPI_VDEC_VPSS_OUT_PATH);
    }

    (void)mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frame);

out_unbind_vdec_vpss:
    (void)mpi_sys_unbind(&vdec_src, &vpss_dst);
out_stop_vdec:
    (void)mpi_vdec_stop_chn(vdec_chn);
out_disable_vpss_chn:
    (void)mpi_vpss_disable_chn(vpss_grp, vpss_chn);
out_destroy_vpss_chn:
    (void)mpi_vpss_destroy_chn(vpss_grp, vpss_chn);
out_stop_vpss_grp:
    (void)mpi_vpss_stop_grp(vpss_grp);
out_destroy_vpss_grp:
    (void)mpi_vpss_destroy_grp(vpss_grp);
out_destroy_vdec:
    (void)mpi_vdec_destroy_chn(vdec_chn);
out_free_jpg:
    MPI_VDEC_VPSS_FREE(jpg_buf);
}
