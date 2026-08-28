#include "fs/fs.h"

#include "mpi_common.h"
#include "mpi_sys.h"
#include "mpi_venc.h"
#include "mpi_vi.h"
#include "mpi_vpss.h"

#define MPI_H264_DEMO_CAMERA_ID             0
#define MPI_H264_DEMO_SENSOR_NAME           "GC0308"
#define MPI_H264_DEMO_OUT_PATH              "storage/sd0/C/out.264"
#define MPI_H264_DEMO_OUT_WIDTH             320
#define MPI_H264_DEMO_OUT_HEIGHT            240
#define MPI_H264_DEMO_FRAME_COUNT           200
#define MPI_H264_DEMO_FPS                   20
#define MPI_H264_DEMO_GOP                   40
#define MPI_H264_DEMO_QP_MIN                10
#define MPI_H264_DEMO_QP_MAX                42
#define MPI_H264_DEMO_THREAD_NUM             2
#define MPI_H264_DEMO_THREAD_PRIO            25
#define MPI_H264_DEMO_BITRATE_KBPS          300
#define MPI_H264_DEMO_TIMEOUT_MS            5000
#define MPI_H264_DEMO_VENC_STREAM_BUF_SIZE  (100 * 1024)

static uint32_t mpi_h264_demo_vpss_buf_size(uint16_t width, uint16_t height)
{
    return (uint32_t)width * height * 3u / 2u * 2u + 1024;
}

static void mpi_h264_demo_make_chn(int mod_id, int dev_id, int chn_id, mpi_chn_t *chn)
{
    if (chn == NULL) {
        return;
    }

    chn->mod_id = mod_id;
    chn->dev_id = dev_id;
    chn->chn_id = chn_id;
}

static mpi_ret_t mpi_h264_demo_write_stream(void *fp,
        mpi_venc_chn_t venc_chn,
        uint32_t frame_index)
{
    mpi_stream_t stream = {0};
    mpi_ret_t ret;

    if (fp == NULL) {
        return MPI_ERR_INVALID_ARG;
    }

    ret = mpi_venc_get_stream(venc_chn, &stream, MPI_H264_DEMO_TIMEOUT_MS);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] get venc stream %u failed: %d\n", frame_index, ret);
        return ret;
    }

    if (stream.addr != NULL && stream.len > 0) {
        fwrite((void *)stream.addr, 1, stream.len, fp);
        printf("[MPI_H264_DEMO] write frame %u, len=%u, seq=%u\n",
               frame_index, stream.len, stream.seq);
    }

    ret = mpi_venc_release_stream(venc_chn, &stream);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] release venc stream %u failed: %d\n", frame_index, ret);
    }

    return ret;
}

void mpi_vi_vpss_h264_test(void)
{
    mpi_ret_t ret;
    mpi_vi_dev_t vi_dev = 0;
    mpi_vpss_grp_t vpss_grp = 0;
    mpi_vpss_chn_t vpss_chn = 0;
    mpi_venc_chn_t venc_chn = 0;
    mpi_vi_dev_attr_t vi_attr = {0};
    mpi_vi_output_info_t vi_output_info = {0};
    mpi_vpss_grp_attr_t vpss_grp_attr = {0};
    mpi_vpss_chn_attr_t vpss_chn_attr = {0};
    mpi_venc_chn_attr_t venc_attr = {0};
    mpi_chn_t vi_src = {0};
    mpi_chn_t vpss_grp_dst = {0};
    mpi_chn_t vpss_chn_src = {0};
    mpi_chn_t venc_chn_dst = {0};
    void *fp = NULL;
    uint32_t i;

    ret = mpi_vi_find_free_dev(&vi_dev);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] find free VI dev failed: %d\n", ret);
        return;
    }

    vi_attr.camera_id = MPI_H264_DEMO_CAMERA_ID;
    vi_attr.sensor_name = MPI_H264_DEMO_SENSOR_NAME;

    ret = mpi_vi_set_dev_attr(vi_dev, &vi_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] set VI attr failed: %d\n", ret);
        return;
    }

    ret = mpi_vi_enable_dev(vi_dev);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] enable VI dev failed: %d\n", ret);
        return;
    }

    ret = mpi_vi_get_output_info(vi_dev, &vi_output_info);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] get VI output info failed: %d\n", ret);
        goto err_disable_vi;
    }

    printf("[MPI_H264_DEMO] VI output: enabled=%u, is_raw=%u, width=%u, height=%u\n",
           vi_output_info.enabled,
           vi_output_info.is_raw,
           vi_output_info.width,
           vi_output_info.height);

    ret = mpi_vpss_find_free_grp(&vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] find free VPSS grp failed: %d\n", ret);
        goto err_disable_vi;
    }

    vpss_grp_attr.src_width = vi_output_info.width;
    vpss_grp_attr.src_height = vi_output_info.height;
    vpss_grp_attr.src_format = vi_output_info.is_raw ? MPI_VPSS_SRC_FMT_RAW : MPI_VPSS_SRC_FMT_YUV;

    ret = mpi_vpss_create_grp(vpss_grp, &vpss_grp_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] create VPSS grp failed: %d\n", ret);
        goto err_disable_vi;
    }

    ret = mpi_vpss_start_grp(vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] start VPSS grp failed: %d\n", ret);
        goto err_destroy_vpss_grp;
    }

    mpi_h264_demo_make_chn(MPI_MOD_ID_VI, vi_dev, 0, &vi_src);
    mpi_h264_demo_make_chn(MPI_MOD_ID_VPSS, vpss_grp, 0, &vpss_grp_dst);

    ret = mpi_vpss_find_free_chn(vpss_grp, &vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] find free VPSS chn failed: %d\n", ret);
        goto err_unbind_vi_vpss;
    }

    vpss_chn_attr.width = MPI_H264_DEMO_OUT_WIDTH;
    vpss_chn_attr.height = MPI_H264_DEMO_OUT_HEIGHT;
    vpss_chn_attr.format = MPI_PIXEL_FMT_YUV420SP;
    vpss_chn_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    vpss_chn_attr.buf_size = mpi_h264_demo_vpss_buf_size(vpss_chn_attr.width,
                             vpss_chn_attr.height);

    ret = mpi_vpss_create_chn(vpss_grp, vpss_chn, &vpss_chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] create VPSS chn failed: %d\n", ret);
        goto err_unbind_vi_vpss;
    }

    ret = mpi_vpss_enable_chn(vpss_grp, vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] enable VPSS chn failed: %d\n", ret);
        goto err_destroy_vpss_chn;
    }

    ret = mpi_venc_find_free_chn(&venc_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] find free VENC chn failed: %d\n", ret);
        goto err_disable_vpss_chn;
    }

    venc_attr.hw_dev = 0;
    venc_attr.type = MPI_VENC_TYPE_SOFT_264;
    venc_attr.input_mode = MPI_VENC_INPUT_OFFLINE;
    venc_attr.in_width = vpss_chn_attr.width;
    venc_attr.in_height = vpss_chn_attr.height;
    venc_attr.in_format = vpss_chn_attr.format;
    venc_attr.in_yuv_order = vpss_chn_attr.yuv_order;
    venc_attr.fps = MPI_H264_DEMO_FPS;
    venc_attr.abr_kbps = MPI_H264_DEMO_BITRATE_KBPS;
    venc_attr.stream_buf_size = MPI_H264_DEMO_VENC_STREAM_BUF_SIZE;
    venc_attr.gop = MPI_H264_DEMO_GOP;
    venc_attr.qp_min = MPI_H264_DEMO_QP_MIN;
    venc_attr.qp_max = MPI_H264_DEMO_QP_MAX;
    venc_attr.thread_num = MPI_H264_DEMO_THREAD_NUM;
    venc_attr.thread_prio = MPI_H264_DEMO_THREAD_PRIO;


    ret = mpi_venc_create_chn(venc_chn, &venc_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] create soft264 VENC failed: %d\n", ret);
        goto err_disable_vpss_chn;
    }

    ret = mpi_venc_start_chn(venc_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] start soft264 VENC failed: %d\n", ret);
        goto err_destroy_venc_chn;
    }

    mpi_h264_demo_make_chn(MPI_MOD_ID_VPSS, vpss_grp, vpss_chn, &vpss_chn_src);
    mpi_h264_demo_make_chn(MPI_MOD_ID_VENC, venc_attr.hw_dev, venc_chn, &venc_chn_dst);

    ret = mpi_sys_bind(&vi_src, &vpss_grp_dst);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] bind VI->VPSS failed: %d\n", ret);
        goto err_stop_vpss_grp;
    }

    ret = mpi_sys_bind(&vpss_chn_src, &venc_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] bind VPSS->VENC failed: %d\n", ret);
        goto err_stop_venc_chn;
    }

    fp = fopen(MPI_H264_DEMO_OUT_PATH, "w+");
    if (fp == NULL) {
        printf("[MPI_H264_DEMO] open output file failed: %s\n", MPI_H264_DEMO_OUT_PATH);
        goto err_unbind_vpss_venc;
    }

    for (i = 0; i < MPI_H264_DEMO_FRAME_COUNT; i++) {
        ret = mpi_h264_demo_write_stream(fp, venc_chn, i);
        if (ret != MPI_SUCCESS) {
            break;
        }
    }

    printf("[MPI_H264_DEMO] done, encoded %u/%u frames to %s\n",
           i, MPI_H264_DEMO_FRAME_COUNT, MPI_H264_DEMO_OUT_PATH);

    goto err_close_file;

err_close_file:
    if (fp != NULL) {
        fclose(fp);
    }
err_unbind_vpss_venc:
    (void)mpi_sys_unbind(&vpss_chn_src, &venc_chn_dst);
err_stop_venc_chn:
    ret = mpi_venc_stop_chn(venc_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] stop VENC failed: %d\n", ret);
    }
err_destroy_venc_chn:
    ret = mpi_venc_destroy_chn(venc_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_H264_DEMO] destroy VENC failed: %d\n", ret);
    }
err_disable_vpss_chn:
    (void)mpi_vpss_disable_chn(vpss_grp, vpss_chn);
err_destroy_vpss_chn:
    (void)mpi_vpss_destroy_chn(vpss_grp, vpss_chn);
err_unbind_vi_vpss:
    (void)mpi_sys_unbind(&vi_src, &vpss_grp_dst);
err_stop_vpss_grp:
    (void)mpi_vpss_stop_grp(vpss_grp);
err_destroy_vpss_grp:
    (void)mpi_vpss_destroy_grp(vpss_grp);
err_disable_vi:
    (void)mpi_vi_disable_dev(vi_dev);
}
