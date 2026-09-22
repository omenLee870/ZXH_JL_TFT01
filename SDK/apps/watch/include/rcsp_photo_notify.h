#ifndef __RCSP_PHOTO_NOTIFY_H__
#define __RCSP_PHOTO_NOTIFY_H__

#include "app_config.h"

#if RCSP_MODE

/**
 * @brief 通知 APP 读取指定照片文件（设备主动拍照保存到 SD 卡后调用）
 *
 * 对应协议 §4.4 "DEVICE端主动拍照并通知" 分支：
 * 设备拍照 -> 发送 cmd:0x8038 op:0x00 resource_info -> APP 通过大文件传输拉取照片。
 *
 * @param photo_path 照片文件完整路径，例如 "storage/sd0/C/video/IMG_0001.JPG"
 * @param session_id 会话 ID：AI 对话(0x32)场景传当前会话 ID，纯拍照场景传 0
 * @return 0=成功, 非0=失败（路径为空/存储未知/文件不存在/发送失败）
 */
int rcsp_photo_notify_resource(const char *photo_path, u8 session_id);

#endif /* RCSP_MODE */

#endif /* __RCSP_PHOTO_NOTIFY_H__ */
