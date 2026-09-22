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

/*
 * AI 识图拍照：只负责拍一张并落盘，不涉及任何 RCSP 交互。
 * on_done 在拍照结束后回调（result: 0 成功, -1 失败；成功时 path 为落盘路径）。
 * 相机忙（正在拍照/录像）时直接以 result=-1 回调。
 */
extern void camera_api_take_photo_ai(void (*on_done)(int result, const char *path));

//停止拍照or停止录像
extern void camera_api_stop_rec_photo();

//获取摄像头状态
extern u8 get_camera_status();


#endif
