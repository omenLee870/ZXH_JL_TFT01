#include "fs/fs.h"

#include "mpi_common.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vpss.h"

#define MPI_VI_VPSS_DEMO_CAMERA_ID             0
#define MPI_VI_VPSS_DEMO_SENSOR_NAME           "NT99141"//"NT99141"
#define MPI_VI_VPSS_DEMO_OUT_WIDTH             1280
#define MPI_VI_VPSS_DEMO_OUT_HEIGHT            720
#define MPI_VI_VPSS_DEMO_TIMEOUT_MS            5000
#define MPI_VI_VPSS_DEMO_SAVE_FILE              1

static void mpi_vi_vpss_demo_make_chn(int mod_id,
                                      int dev_id,
                                      int chn_id,
                                      mpi_chn_t *chn)
{
    if (chn == NULL) {
        return;
    }

    chn->mod_id = mod_id;
    chn->dev_id = dev_id;
    chn->chn_id = chn_id;
}

void mpi_vi_vpss_test(void)
{
    mpi_ret_t ret;
    mpi_vi_dev_t vi_dev = 0;
    mpi_vpss_grp_t vpss_grp = 0;
    mpi_vpss_chn_t vpss_chn = 0;
    mpi_vi_dev_attr_t vi_attr = {0};
    mpi_vi_output_info_t vi_output_info = {0};
    mpi_vpss_grp_attr_t vpss_grp_attr = {0};
    mpi_vpss_chn_attr_t vpss_chn_attr = {0};
    mpi_chn_t vi_src = {0};
    mpi_chn_t vpss_grp_dst = {0};
    mpi_chn_t vpss_chn_src = {0};

    ret = mpi_vi_find_free_dev(&vi_dev);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] find free VI dev failed: %d\n", ret);
        return;
    }

    vi_attr.camera_id = MPI_VI_VPSS_DEMO_CAMERA_ID;
    vi_attr.sensor_name = MPI_VI_VPSS_DEMO_SENSOR_NAME;

    ret = mpi_vi_set_dev_attr(vi_dev, &vi_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] set VI attr failed: %d\n", ret);
        return;
    }

    ret = mpi_vi_enable_dev(vi_dev);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] enable VI dev failed: %d\n", ret);
        return;
    }

    ret = mpi_vi_get_output_info(vi_dev, &vi_output_info);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] get VI output info failed: %d\n", ret);
        goto err_disable_vi;
    }

    printf("[MPI_VI_VPSS_DEMO] VI output: enabled=%u, is_raw=%u, width=%u, height=%u\n",
           vi_output_info.enabled,
           vi_output_info.is_raw,
           vi_output_info.width,
           vi_output_info.height);

    ret = mpi_vpss_find_free_grp(&vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] find free VPSS grp failed: %d\n", ret);
        goto err_disable_vi;
    }

    vpss_grp_attr.src_width = vi_output_info.width;
    vpss_grp_attr.src_height = vi_output_info.height;
    vpss_grp_attr.src_format = vi_output_info.is_raw ? MPI_VPSS_SRC_FMT_RAW : MPI_VPSS_SRC_FMT_YUV;

    ret = mpi_vpss_create_grp(vpss_grp, &vpss_grp_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] create VPSS grp failed: %d\n", ret);
        goto err_disable_vi;
    }

    ret = mpi_vpss_start_grp(vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] start VPSS grp failed: %d\n", ret);
        goto err_destroy_vpss_grp;
    }

    mpi_vi_vpss_demo_make_chn(MPI_MOD_ID_VI, vi_dev, 0, &vi_src);
    mpi_vi_vpss_demo_make_chn(MPI_MOD_ID_VPSS, vpss_grp, 0, &vpss_grp_dst);

    ret = mpi_vpss_find_free_chn(vpss_grp, &vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] find free VPSS chn failed: %d\n", ret);
        goto err_stop_vpss_grp;
    }

    vpss_chn_attr.width = MPI_VI_VPSS_DEMO_OUT_WIDTH;
    vpss_chn_attr.height = MPI_VI_VPSS_DEMO_OUT_HEIGHT;
    vpss_chn_attr.format = MPI_PIXEL_FMT_YUV422_PACKED;
    vpss_chn_attr.yuv_order = MPI_YUV_ORDER_YUYV;
    vpss_chn_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    vpss_chn_attr.buf_size =  vpss_chn_attr.width * vpss_chn_attr.height * 2 * 2 + 1024; // for YUV422

    ret = mpi_vpss_create_chn(vpss_grp, vpss_chn, &vpss_chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] create VPSS chn failed: %d\n", ret);
        goto err_stop_vpss_grp;
    }

    ret = mpi_vpss_enable_chn(vpss_grp, vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] enable VPSS chn failed: %d\n", ret);
        goto err_destroy_vpss_chn;
    }


    ret = mpi_sys_bind(&vi_src, &vpss_grp_dst);
    if (ret != MPI_SUCCESS) {
        printf("[MPI_VI_VPSS_DEMO] bind VI->VPSS failed: %d\n", ret);
        goto err_disable_vpss_chn;
    }

    while (1) {
        mpi_video_frame_info_t frame_info = {0};
        ret = mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &frame_info, MPI_VI_VPSS_DEMO_TIMEOUT_MS);
        if (ret != MPI_SUCCESS) {
            printf("[MPI_VI_VPSS_DEMO] get VPSS frame failed: %d\n", ret);
            break;
        }

        printf("[MPI_VI_VPSS_DEMO] got VPSS frame: addr=%p, len=%u, width=%u, height=%u, format=%u, yuv_order=%u\n",
               frame_info.y_addr, frame_info.y_len,
               frame_info.width, frame_info.height,
               frame_info.format, frame_info.yuv_order);

#if MPI_VI_VPSS_DEMO_SAVE_FILE
        char filename[64];
        snprintf(filename, sizeof(filename), "storage/sd0/C/yuv/out_%04u.yuv", frame_info.seq);
        void *fp = fopen(filename, "w+");
        if (fp) {
            fwrite((void *)frame_info.y_addr, 1, frame_info.y_len, fp);
            fwrite((void *)frame_info.c_addr, 1, frame_info.c_len, fp);
            printf("[MPI_VI_VPSS_DEMO] dumped VPSS frame to file: %s\n", filename);
            fclose(fp);
        } else {
            printf("[MPI_VI_VPSS_DEMO] open file failed: %s\n", filename);
        }
#endif


        if (mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frame_info) != MPI_SUCCESS) {
            printf("[MPI_VI_VPSS_DEMO] release VPSS frame failed: %d\n", ret);
            break;
        }

    }


err_unbind_vi_vpss:
    (void)mpi_sys_unbind(&vi_src, &vpss_grp_dst);
err_disable_vpss_chn:
    (void)mpi_vpss_disable_chn(vpss_grp, vpss_chn);
err_destroy_vpss_chn:
    (void)mpi_vpss_destroy_chn(vpss_grp, vpss_chn);
err_stop_vpss_grp:
    (void)mpi_vpss_stop_grp(vpss_grp);
err_destroy_vpss_grp:
    (void)mpi_vpss_destroy_grp(vpss_grp);
err_disable_vi:
    (void)mpi_vi_disable_dev(vi_dev);
}
