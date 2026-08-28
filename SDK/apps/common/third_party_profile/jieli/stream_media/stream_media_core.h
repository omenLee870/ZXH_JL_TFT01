#ifndef __STREAM_MEDIA_CORE_H__
#define __STREAM_MEDIA_CORE_H__

#include "typedef.h"
#include "stream_media_protocol.h"

// 推流配置（APP 下发开始推流时传入的参数）
typedef struct {
    u16 width;
    u16 height;
    u8  fps;
    u8  quality;
    u8  format;         // STREAM_FORMAT_JPEG / H264 / JLJPEG
    u16 stream_mtu;     // 协商后的 MTU
} stream_media_stream_cfg_t;

// 推流开始回调：收到 APP 的开始推流命令
typedef void (*stream_media_stream_start_cb_t)(const stream_media_stream_cfg_t *cfg);

// 推流结束回调：收到 APP 的结束推流命令
typedef void (*stream_media_stream_stop_cb_t)(void);

// 收到流数据回调（APP 推送的流媒体帧数据，从数据通道收到）
typedef void (*stream_media_data_push_cb_t)(const u8 *data, u16 len,
        u16 seq, u32 offset,
        u32 frame_size, u32 timestamp,
        u16 crc, u8 stream_type, u8 is_last);

// 导航信息回调
typedef void (*stream_media_nav_info_cb_t)(const stream_media_nav_info_t *info);

// 状态变更回调
typedef void (*stream_media_state_change_cb_t)(u8 old_state, u8 new_state);

// 离线导航回调
typedef u8 (*stream_media_offline_nav_cb_t)(const stream_media_offline_nav_info_t *info);

// ============================================================================
// 注册回调 & 初始化 / 退出
// ============================================================================

void stream_media_core_init(void);
void stream_media_core_exit(void);
u8   stream_media_core_get_state(void);
u16  stream_media_core_get_mtu(void);

void stream_media_set_stream_start_cb(stream_media_stream_start_cb_t cb);
void stream_media_set_stream_stop_cb(stream_media_stream_stop_cb_t cb);
void stream_media_set_data_push_cb(stream_media_data_push_cb_t cb);
void stream_media_set_nav_info_cb(stream_media_nav_info_cb_t cb);
void stream_media_set_state_change_cb(stream_media_state_change_cb_t cb);
void stream_media_set_offline_nav_cb(stream_media_offline_nav_cb_t cb);

// ============================================================================
// 设备主动发送接口
// ============================================================================

// 发送异常上报
int  stream_media_send_abnormal(u16 seq, u8 reason);

// 推送流数据给 APP（设备采集的画面帧，通过数据通道发送）
int  stream_media_push_frame(const u8 *frame_data, u32 frame_size,
                             u16 seq, u32 timestamp, u8 format);

// ============================================================================
// 数据接收入口（由 ble_rcsp_server.c 的 bt_rcsp_custom_att_write_callback 调用）
// ============================================================================

// 命令数据接收 → BF01 写入时调用
void stream_media_on_cmd_recv(const u8 *data, u16 len);

// 流数据接收 → BF03 写入时调用
void stream_media_on_data_recv(const u8 *data, u16 len);

// SPP 通道统一接收入口 → 根据包头自动区分命令包（JLP 开头）和数据包
void stream_media_on_spp_recv(const u8 *data, u16 len);

#endif // __STREAM_MEDIA_CORE_H__

