#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os/os_api.h"

#include "fb.h"
#include "mpi_sys.h"
#include "mpi_vdec.h"
#include "mpi_vo.h"
#include "mpi_vpss.h"

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
#define H264_DEMO_MALLOC(size) malloc_psram(size)
#define H264_DEMO_FREE(ptr)    free_psram(ptr)
#else
#define H264_DEMO_MALLOC(size) malloc(size)
#define H264_DEMO_FREE(ptr)    free(ptr)
#endif

#define H264_FILE_PATH              "storage/sd0/C/out.264"
#define H264_READ_CHUNK             (16u * 1024u)
#define H264_VDEC_FRAME_POOL_SIZE   (512u * 1024u)
#define H264_VDEC_STREAM_BUF_SIZE   (64u * 1024u)
#define H264_DISP_WIDTH             320
#define H264_DISP_HEIGHT            386

static int get_h264_stream(FILE *fp, uint8_t *buf, uint32_t buf_size, uint32_t *out_len)
{
    if (fp == NULL || buf == NULL || out_len == NULL) {
        return -1;
    }

    *out_len = (uint32_t)fread(buf, 1, buf_size, fp);
    if (*out_len == 0) {
        return -1;
    }
    return 0;
}

void mpi_h264_vpss_vo_test(void)
{
    FILE *fp = NULL;
    uint8_t *read_buf = NULL;
    uint32_t read_len;
    uint32_t stream_count = 0;
    mpi_ret_t ret;
    mpi_vdec_chn_attr_t vdec_attr = {0};
    mpi_vdec_probe_info_t pic_info = {0};
    mpi_stream_t stream = {0};
    mpi_vpss_grp_attr_t grp_attr = {0};
    mpi_vpss_chn_attr_t chn_attr = {0};
    mpi_vo_chn_attr_t vo_attr = {0};

    const mpi_vdec_chn_t vdec_chn = 0;
    const mpi_vpss_grp_t vpss_grp = 0;
    const mpi_vpss_chn_t vpss_chn = 0;
    const mpi_vo_chn_t vo_chn = 0;
    mpi_chn_t vdec_src = {MPI_MOD_ID_VDEC, 0, vdec_chn};
    mpi_chn_t vpss_dst = {MPI_MOD_ID_VPSS, vpss_grp, 0};
    mpi_chn_t vpss_src = {MPI_MOD_ID_VPSS, vpss_grp, vpss_chn};
    mpi_chn_t vo_dst = {MPI_MOD_ID_VO, 0, vo_chn};

    printf("[H264_VPSS_VO] start, input=%s\n", H264_FILE_PATH);

    fp = fopen(H264_FILE_PATH, "rb");
    if (fp == NULL) {
        printf("[H264_VPSS_VO] open file failed\n");
        return;
    }

    read_buf = (uint8_t *)H264_DEMO_MALLOC(H264_READ_CHUNK);
    if (read_buf == NULL) {
        printf("[H264_VPSS_VO] malloc read buf failed\n");
        goto out_close_file;
    }

    vdec_attr.hw_dev = 0;
    vdec_attr.type = MPI_VDEC_TYPE_SOFT_H264;
    vdec_attr.output_mode = MPI_VDEC_OUTPUT_FRAME;
    vdec_attr.frame_buf_size = H264_VDEC_FRAME_POOL_SIZE;
    vdec_attr.stream_buf_size = H264_VDEC_STREAM_BUF_SIZE;
    ret = mpi_vdec_create_chn(vdec_chn, &vdec_attr);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] create vdec failed: %d\n", ret);
        goto out_free_buf;
    }

    ret = mpi_vdec_start_chn(vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] start vdec failed: %d\n", ret);
        goto out_destroy_vdec;
    }

    if (get_h264_stream(fp, read_buf, H264_READ_CHUNK, &read_len) != 0) {
        printf("[H264_VPSS_VO] read first stream failed\n");
        goto out_stop_vdec_only;
    }
    stream.addr = read_buf;
    stream.len = read_len;

    ret = mpi_vdec_probe_pic_info(&stream, &pic_info);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] probe pic info failed: %d, len=%u\n", ret, stream.len);
        goto out_stop_vdec_only;
    }

    printf("[H264_VPSS_VO] input %ux%u fmt=%u\n",
           pic_info.width, pic_info.height, pic_info.format);

    grp_attr.src_width = pic_info.width;
    grp_attr.src_height = pic_info.height;
    grp_attr.src_format = MPI_VPSS_SRC_FMT_YUV;
    grp_attr.src_pixel_format = pic_info.format;
    grp_attr.src_yuv_order = 0;
    ret = mpi_vpss_create_grp(vpss_grp, &grp_attr);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] create vpss grp failed: %d\n", ret);
        goto out_stop_vdec_only;
    }

    ret = mpi_vpss_start_grp(vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] start vpss grp failed: %d\n", ret);
        goto out_destroy_vpss_grp;
    }

    chn_attr.width = H264_DISP_WIDTH;
    chn_attr.height = H264_DISP_HEIGHT;
    chn_attr.format = MPI_PIXEL_FMT_YUV422_PACKED;
    chn_attr.yuv_order = MPI_YUV_ORDER_YUYV;
    chn_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    ret = mpi_vpss_set_chn_attr(vpss_grp, vpss_chn, &chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] set vpss chn failed: %d\n", ret);
        goto out_stop_vpss_grp;
    }

    ret = mpi_vpss_enable_chn(vpss_grp, vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] enable vpss chn failed: %d\n", ret);
        goto out_stop_vpss_grp;
    }

    memcpy(vo_attr.fb_name, "fb5", 4);
    vo_attr.fb_num = 2;
    vo_attr.fb_width = H264_DISP_WIDTH;
    vo_attr.fb_height = H264_DISP_HEIGHT;
    vo_attr.window_width = H264_DISP_WIDTH;
    vo_attr.window_height = H264_DISP_HEIGHT;
    vo_attr.fb_format = FB_COLOR_FORMAT_YUV422;
    vo_attr.combine = 1;
    ret = mpi_vo_create_chn(vo_chn, &vo_attr);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] create vo failed: %d\n", ret);
        goto out_disable_vpss_chn;
    }

    ret = mpi_vo_start_chn(vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] start vo failed: %d\n", ret);
        goto out_destroy_vo;
    }

    ret = mpi_sys_bind(&vpss_src, &vo_dst);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] bind vpss->vo failed: %d\n", ret);
        goto out_stop_vo;
    }

    ret = mpi_sys_bind(&vdec_src, &vpss_dst);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] bind vdec->vpss failed: %d\n", ret);
        goto out_unbind_vpss_vo;
    }

    while (1) {
        ret = mpi_vdec_send_stream(vdec_chn, &stream);
        if (ret != MPI_SUCCESS) {
            printf("[H264_VPSS_VO] send stream to vdec failed: %d\n", ret);
            goto out_unbind_vdec_vpss;
        }
        stream_count++;

        if (get_h264_stream(fp, read_buf, H264_READ_CHUNK, &read_len) != 0) {
            break;
        }
        stream.len = read_len;
    }

    ret = mpi_vdec_flush_stream(vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("[H264_VPSS_VO] flush stream failed: %d\n", ret);
        goto out_unbind_vdec_vpss;
    }

    printf("[H264_VPSS_VO] finish, chunks=%u\n", stream_count);

out_unbind_vdec_vpss:
    mpi_sys_unbind(&vdec_src, &vpss_dst);
out_unbind_vpss_vo:
    mpi_sys_unbind(&vpss_src, &vo_dst);
out_stop_vo:
    mpi_vo_stop_chn(vo_chn);
out_destroy_vo:
    mpi_vo_destroy_chn(vo_chn);
out_disable_vpss_chn:
    mpi_vpss_disable_chn(vpss_grp, vpss_chn);
out_stop_vpss_grp:
    mpi_vpss_stop_grp(vpss_grp);
out_destroy_vpss_grp:
    mpi_vpss_destroy_grp(vpss_grp);
out_stop_vdec_only:
    mpi_vdec_stop_chn(vdec_chn);
out_destroy_vdec:
    mpi_vdec_destroy_chn(vdec_chn);
out_free_buf:
    H264_DEMO_FREE(read_buf);
out_close_file:
    fclose(fp);
}

