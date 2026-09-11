#ifndef __KID_WT_VIDEO_H__
#define __KID_WT_VIDEO_H__

int  kid_wt_video_xsmt_start(void);
void kid_wt_video_xsmt_stop(void);
int  kid_wt_video_xsmt_is_running(void);
int  kid_wt_video_xsmt_get_status(void);
void kid_wt_video_xsmt_get_last_params(int *w, int *h, int *fps, int *abr);

int  kid_wt_video_recv_start(void);
void kid_wt_video_recv_stop(void);
int  kid_wt_video_recv_is_running(void);
int  kid_wt_video_recv_get_status(void);

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define KWT_CAMERA_SENSOR           "GC0308"
/* 对讲视频帧 320x240(传感器 4:3 等比): 该尺寸同时是发送端编码、接收端解码
 * 输出 fb5(远端画面)以及 LVGL 控件尺寸, 收发两机(同款 kid camera 直连, 线上不交换
 * 分辨率)必须以同配置编译。 */
#define KWT_CAMERA_WIDTH            320
#define KWT_CAMERA_HEIGHT           240
#define KWT_CAMERA_JPEG_QUALITY     40
#define KWT_CAMERA_ABR_KBPS         (50 * 8)    /* match s_info.abr_kbps */
#define KWT_CAMERA_STREAM_BUF_SZ    (64 * 1024)
#define KWT_CAMERA_FPS              10          /* match s_info.fps */
#define KWT_VPSS_LINE_NUM           16u         /* rows buffered in VPSS chn */

/* ============================================================================
 * Test switches: independently disable the encode (sender) / decode (receiver)
 * paths.  1 = disabled (that path's start() becomes a no-op).  Default 0.
 * Encode = camera→VENC→vstream_send_frame; decode = vstream_recv→VDEC→display.
 * ============================================================================ */
#ifndef KWT_DISABLE_ENCODE
#define KWT_DISABLE_ENCODE          0
#endif
#ifndef KWT_DISABLE_DECODE
#define KWT_DISABLE_DECODE          0
#endif

#if KWT_USE_H264
/* SOFT_264 tuning (mirrors apps/common/video/mpi_demo/h264/mpi_vi_vpss_h264.c).
 * gop/qp are optional in soft264 (defaults gop=fps, qp 10..42) but set here to
 * match the reference demo. thread_num is capped at 4 by soft264. */
#define KWT_CAMERA_H264_GOP          40
#define KWT_CAMERA_H264_QP_MIN       10
#define KWT_CAMERA_H264_QP_MAX       42
#define KWT_CAMERA_H264_THREAD_NUM    2
#define KWT_CAMERA_H264_THREAD_PRIO   25
#endif

/* 共享 fb 名 + 本地 PIP 几何 — 生产者(kid_wt_video.c)与控件创建
 * (ui_action_video_intercom.c)共用。两文件仅通过 fb 名协调:
 * fb5 = 远端接收画面, fb6 = 本地摄像头 PIP。 */
#define KWT_RECV_FB_NAME    "fb5"
#define KWT_PIP_FB_NAME     "fb6"
#define KWT_PIP_WIDTH       80
#define KWT_PIP_HEIGHT      60
#define KWT_PIP_LEFT        (KWT_CAMERA_WIDTH  - KWT_PIP_WIDTH)   /* 240 */
#define KWT_PIP_TOP         (KWT_CAMERA_HEIGHT - KWT_PIP_HEIGHT)  /* 180 */


#endif // __KID_WT_VIDEO_H__
