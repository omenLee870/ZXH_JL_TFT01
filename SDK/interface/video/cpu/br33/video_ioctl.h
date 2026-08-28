#ifndef VIDEO_IOCTL_H
#define VIDEO_IOCTL_H

#include "generic/ioctl.h"
//------->video.h
#define VIDEO_DEFAULT_BUF_MODE	0x0
#define VIDEO_PPBUF_MODE		0x1
#define VIDEO_WL80_SPEC_PICTURE_MODE		BIT(0)
#define VIDEO_WL80_SPEC_DOUBLE_REC_MODE		BIT(1)
#define VIDEO_WL80_SPEC_720P_REC_MODE		BIT(2)

#define VIDEO_TAG(a,b,c,d)  (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))


enum video_state {
    VIDEO_STATE_START,
    VIDEO_STATE_PAUSE,
    VIDEO_STATE_RESUME,
    VIDEO_STATE_STOP,
    VIDEO_STATE_SAVE_FILE,
    VIDEO_STATE_SET_OSD,
    VIDEO_STATE_SET_VOICE,
    VIDEO_STATE_GET_INFO,
    VIDEO_STATE_PKG_LEN,
    VIDEO_STATE_PKG_MUTE,
    VIDEO_STATE_SET_OSD_STR,
    VIDEO_STATE_CFG,
    VIDEO_STATE_CFG_ROI,
    VIDEO_STATE_GET_PKG_TIME,
    VIDEO_STATE_SET_NEXT_IFRAME,
    VIDEO_STATE_STOP_COUNT_DOWN,
    VIDEO_STATE_SET_DR,
    VIDEO_STATE_CAP_IFRAME,
    VIDEO_STATE_RESET_BITS_RATE,
    VIDEO_STATE_PKG_PAUSE_RUN,
    VIDEO_STATE_V4_PAUSE_RUN,
    VIDEO_STATE_SET_ENC_CROP,
    VIDEO_STATE_SET_DIS_CROP,
    VIDEO_STATE_SET_AUDIO_CALLBACK,
    VIDEO_STATE_SET_DIS_PAUSE,
    VIDEO_STATE_SET_AUDIO_VOLUME,
};


enum {
    VIDREQ_GET_OVERLAY_BUFFER,
    VIDREQ_PUT_OVERLAY_BUFFER,
    VIDREQ_FREE_OVERLAY_BUFFER,
    VIDREQ_GET_CAPTURE_BUFFER,
    VIDREQ_SET_CAPTURE_BUFFER,
    VIDREQ_PUT_CAPTURE_BUFFER,
    VIDREQ_FREE_CAPTURE_BUFFER,
    VIDREQ_GET_IMAGE_CAPTURE_BUFFER,
    VIDREQ_PUT_IMAGE_CAPTURE_BUFFER,
    VIDREQ_IMAGE_CAPTURE_COMPLETED,
    VIDREQ_IMAGE_ZOOM, // 10
    /*VIDREQ_IMAGE_DISP_ZOOM,*/
    VIDREQ_IMAGE_ENC_START,
    /*VIDREQ_GET_DISP_CAPABILITY,*/
    VIDREQ_GET_ENCODER_CHANNEL,
    /*VIDREQ_PUT_IMAGE_TO_BUFFER, // 15*/
    /*VIDREQ_RESET_DISP_IMAGE,*/
    VIDREQ_RESET_CBUFFER,
    /*VIDREQ_PUT_DEC_DATA,*/
    VIDREQ_GET_DEC_DATA,
    VIDREQ_RESET_DOWN_CBUFFER, //20
    VIDREQ_RESET_UP_CBUFFER,
    VIDREQ_RESET_ENCODER,
    VIDREQ_REKSTART_ENCODER,
    VIDREQ_WAIT_ENC_END_STOP_IMC,
    VIDREQ_GET_FNUM,
    VIDREQ_PAUSE_ENC,
    VIDREQ_CONTINUE_ENC,
    VIDREQ_PAUSE_ENC_CHANNEL,
    VIDREQ_CONTINUE_ENC_CHANNEL,
    VIDREQ_GET_TIME_LABEL,
    VIDREQ_SET_SOFT_IMAGE_LABEL,
    VIDREQ_CAMERA_OUT,
    VIDREQ_RESET_UVC_ENC,
    VIDREQ_ENCODE_ONE_IMAGE,
    VIDREQ_SET_JPG_THUMBNAILS,
    VIDREQ_ASYNC_PUT_DEC_DATA,
    VIDREQ_BUFF_MOVE,
    VIDREQ_GET_MANUAL_ENC_YUV,
    VIDREQ_MANUAL_ENC_STREAM,
    VIDREQ_GET_SRC_FMT,
    VIDREQ_FLUSHINV_DEC_DATA,
    VIDREQ_INSERT_WATERMARKIGN,
    VIDREQ_MANUAL_DISP_PUTMAP,
    VIDREQ_KICK_UP_ONE_FRMAE,
    VIDREQ_GET_CORRECT_SIZE,
};

enum video_rec_quality {
    VIDEO_LOW_Q,
    VIDEO_MID_Q,
    VIDEO_HIG_Q,
};

enum video_camera_type {
    VIDEO_CAMERA_NORMAL = 0,
    VIDEO_CAMERA_UVC,
    VIDEO_CAMERA_VIRTUAL,
    VIDEO_CAMERA_JLC,
    VIDEO_CAMERA_MUX,
};


#define  VIDEO_DIR_NONE   0
#define  VIDEO_HOR_MIRROR BIT(0)
#define  VIDEO_VER_MIRROR BIT(1)

enum video_pix_format {
    VID_PIX_FMT_H264,
    VID_PIX_FMT_MJPG,
    VID_PIX_FMT_IMG,
    VID_PIX_FMT_YUV,
};

enum image_pix_format {
    IMG_PIX_FMT_JPG,
    IMG_PIX_FMT_YUV,
};

enum video_osd_mode {
    VID_OSD_TEXT = 0,
    VID_OSD_GRAPH,
};

enum video_osd_type {
    VIDEO_STREAM_OSD,
    VIDEO_IMAGE_OSD,
};

#define VIDEO_REC_NUM       2       //double rec or single rec

#define VIDEO_PLATFORM_DATA_BEGIN(vdata) \
	static const struct video_platform_data vdata = { \


#define VIDEO_PLATFORM_DATA_END() \
	};

#define MANUAL_CHANNEL  3

#define REGISTER_VIDEO_SUBDEVICE(dev, id) \
	struct video_subdevice dev SEC_USED(.video_subdev.##id) = { \
		.subid = id, \
        .name = #dev, \

#define REGISTER_VIDEO_DEVICE(dev, _name) \
    struct video_device_ops dev SEC_USED(.video_device) = { \
        .name = _name, \


//------->video_ioctl.h

/* Values for 'capabilities' field */
#define VIDEO_CAP_VIDEO_CAPTURE		0x00000001  /* Is a video capture device */
#define VIDEO_CAP_VIDEO_OUTPUT		0x00000002  /* Is a video output device */
#define VIDEO_CAP_VIDEO_OVERLAY		0x00000004  /* Can do video overlay */

#define VIDEO_CAP_CAMERA_FPS        "camera_fps"


#define VIDEO_CAP_TUNER			0x00010000  /* has a tuner */
#define VIDEO_CAP_AUDIO			0x00020000  /* has audio support */
#define VIDEO_CAP_RADIO			0x00040000  /* is a radio device */
#define VIDEO_CAP_MODULATOR		0x00080000  /* has a modulator */

#define VIDEO_CAP_READWRITE              0x01000000  /* read/write systemcalls */
#define VIDEO_CAP_ASYNCIO                0x02000000  /* async I/O */
#define VIDEO_CAP_STREAMING              0x04000000  /* streaming I/O ioctls */


/*#define VIDEO_FPS(a) 	((a)*100)*/



#define VIDEO_REC_SET_FPS	4
#define VIDEO_REC_SET_FILL_FRAME	5
#define VIDEO_REC_SET_CAMERA_FPS    6
#define VIDEO_REC_SET_CYC_TIME      8
#define VIDEO_REC_SET_DELAY_LIMIT   9
#define VIDEO_REC_RESET_BITS_RATE   10
#define VIDEO_REC_SET_TIME_STAMP    11


#define VIDEO_PIX_FMT_RAW  			0x00000001
#define VIDEO_PIX_FMT_JPEG 			0x00000002
#define VIDEO_PIX_FMT_H264 			0x00000004

#define VIDEO_PIX_FMT_YUV420 		0x00000100
#define VIDEO_PIX_FMT_YUYV          0x00000200
#define VIDEO_PIX_FMT_YV12          0x00000400
#define VIDEO_PIX_FMT_NV12          0x00000800
#define VIDEO_PIX_FMT_YUV422 		0x00001000
#define VIDEO_PIX_FMT_YUV444 		0x00001100
#define VIDEO_PIX_FMT_YUV 			0x0000ff00



#define VIDIOC_QUERYCAP 		        _IOR('V', 0, sizeof(struct video_capability))
#define VIDIOC_GET_FMT 			        _IOR('V', 1, sizeof(struct video_format))
#define VIDIOC_SET_FMT 			        _IOW('V', 1, sizeof(struct video_format))
#define VIDIOC_OVERLAY 			        _IOW('V', 2, sizeof(int))
#define VIDIOC_STREAM_ON 		        _IOW('V', 3, sizeof(enum video_buf_type))
#define VIDIOC_STREAM_OFF 		        _IOW('V', 4, sizeof(enum video_buf_type))
#define VIDIOC_REQBUFS 			        _IOW('V', 5, sizeof(struct video_reqbufs))
#define VIDIOC_QUERYBUF 		        _IOR('V', 6, sizeof(struct videobuf_buffer))
#define VIDIOC_QBUF 			        _IOR('V', 7, sizeof(struct videobuf_buffer))
#define VIDIOC_DQBUF 			        _IOW('V', 8, sizeof(struct videobuf_buffer))
/*#define VIDIOC_GET_AUDIO 		        _IOR('V', 9, sizeof(struct video_audio))
#define VIDIOC_SET_AUDIO 	 	        _IOW('V', 9, sizeof(struct video_audio))*/
#define VIDIOC_IMAGE_CAPTURE 	        _IOR('V', 10, sizeof(struct video_image_capture))
#define VIDIOC_DEL_IMAGE 		        _IOW('V', 11, sizeof(struct video_image_capture))
#define VIDIOC_PLAY   			        _IOW('V', 12, sizeof(int))
#define VIDIOC_GET_S_ATTR               _IOR('V', 13, sizeof(struct video_enc_attr))
#define VIDIOC_SET_S_ATTR               _IOW('V', 13, sizeof(struct video_enc_attr))
#define VIDIOC_GET_D_ATTR               _IOR('V', 14, sizeof(struct video_enc_attr))
#define VIDIOC_SET_D_ATTR               _IOW('V', 14, sizeof(struct video_enc_attr))
#define VIDIOC_GET_FPS                  _IOR('V', 15, sizeof(struct video_dev_fps))
#define VIDIOC_SET_FPS                  _IOW('V', 15, sizeof(struct video_dev_fps))
#define VIDIOC_GET_OSD                  _IOR('V', 16, sizeof(struct video_osd))
#define VIDIOC_SET_OSD                  _IOW('V', 16, sizeof(struct video_osd))
#define VIDIOC_OSD_ENABLE               _IOW('V', 17, sizeof(bool))
#define VIDIOC_OSD_DISABLE              _IOW('V', 18, sizeof(bool))
#define VIDIOC_GET_FRAME_CNT            _IOW('V', 19, sizeof(int))
#define VIDIOC_TRY_STOP_FRAME_INTERVAL  _IOW('V', 20, sizeof(int))
#define VIDIOC_SET_STOP_FRAME_INTERVAL  _IOW('V', 21, sizeof(int))
#define VIDIOC_GET_OSD_ATTR             _IOR('V', 22, sizeof(struct video_osd_attr))
#define VIDIOC_SET_OSD_ATTR             _IOW('V', 23, sizeof(struct video_osd_attr))
#define VIDIOC_START_IFREME             _IOW('V', 24, sizeof(int))
#define VIDIOC_APPEND_USER_DATA         _IOW('V', 25, sizeof(struct h264_user_data))
#define VIDIOC_V4_PAUSE_RUN   			_IOW('V', 26, sizeof(int))
#define VIDIOC_SET_AVC                  _IOW('V', 27, sizeof(struct app_enc_info))
#define VIDIOC_SET_OVERLAY				_IOW('V', 26, sizeof(int))
#define VIDIOC_GET_OVERLAY				_IOR('V', 26, sizeof(int))
#define VIDIOC_CLEAR_OVERLAY			_IOW('V', 27, sizeof(int))
#define VIDIOC_BUF_MOVE                 _IOW('V', 28, sizeof(int))
#define VIDIOC_SET_ENC_CROP             _IOW('V', 29, sizeof(struct video_source_crop))
#define VIDIOC_SET_DIS_CROP             _IOW('V', 30, sizeof(struct video_source_crop))
#define VIDIOC_GET_MIRROR               _IOR('V', 31, sizeof(struct video_mirror))
#define VIDIOC_SET_MIRROR               _IOW('V', 31, sizeof(struct video_mirror))
#define VIDIOC_GET_JAGGY                _IOR('V', 32, sizeof(struct video_jaggy))
#define VIDIOC_SET_JAGGY                _IOW('V', 32, sizeof(struct video_jaggy))
#define VIDIOC_REQDBUF 			        _IOW('V', 33, sizeof(struct video_reqbufs))
#define VIDIOC_RDBUF 			        _IOR('V', 34, sizeof(struct videobuf_buffer))
#define VIDIOC_IDBUF 			        _IOW('V', 35, sizeof(struct videobuf_buffer))
#define VIDIOC_YUV_CAPTURE 	        	_IOR('V', 36, sizeof(struct video_image_capture))
#define VIDIOC_CLOSE_CAMERA             _IOW('V', 37, sizeof(int))
#define VIDIOC_SET_CAMERA_INFO          _IOW('V', 38, sizeof(struct camera_device_info))
#define VIDIOC_PAUSE                    _IOW('V', 39, sizeof(int))
#define VIDIOC_RESUME                   _IOW('V', 40, sizeof(int))
#define VIDIOC_GET_SOURCE_CH            _IOW('V', 41, sizeof(int))
#define VIDIOC_SAVE_FILE                _IOW('V', 42, sizeof(int))
#define VIDIOC_SET_CYC_TIME             _IOW('V', 43, sizeof(int))
#define VIDIOC_SET_FILE                 _IOW('V', 44, sizeof(int))






enum video_buf_type {
    VIDEO_BUF_TYPE_VIDEO_CAPTURE,
    VIDEO_BUF_TYPE_IMAGE_CAPTURE,
    VIDEO_BUF_TYPE_VIDEO_OVERLAY,
    VIDEO_BUF_TYPE_VIDEO_PLAY,
    VIDEO_BUF_TYPE_YUV_CAPTURE,
    VIDEO_BUF_TYPE_UVC_CAPTURE,
    VIDEO_BUF_TYPE_ISP_TRAIN,
    VIDEO_BUF_TYPE_QUICK_IMAGE,
    VIDEO_BUF_TYPE_STICKER_IMAGE, //大头贴
    VIDEO_PIPE_PRE_CREATE,
};

enum video_three_way_type {
    VIDEO_THREE_WAY_NONE,
    VIDEO_THREE_WAY_JPEG,
    VIDEO_THREE_WAY_H264,
    VIDEO_THREE_WAY_DOU_RAW,

};









#endif

