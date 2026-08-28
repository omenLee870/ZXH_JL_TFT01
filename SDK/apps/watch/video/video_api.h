#ifndef _VIDEO_API_H_
#define _VIDEO_API_H_


#include  "generic/includes.h"


enum {
    CAMERA_STATE_IDLE = 0,
    CAMERA_STATE_PHOTO,
    CAMERA_STATE_VIDEO,
};

//开始录像
extern void camera_api_record_video();

//拍照or停止录像
extern void camera_api_take_photo_or_stop_rec();

//停止拍照or停止录像
extern void camera_api_stop_rec_photo();

//获取摄像头状态
extern u8 get_camera_status();


#endif
