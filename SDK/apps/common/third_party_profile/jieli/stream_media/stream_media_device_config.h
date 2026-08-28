#ifndef __STREAM_MEDIA_DEVICE_CONFIG_H__
#define __STREAM_MEDIA_DEVICE_CONFIG_H__

#include "typedef.h"
#include "stream_media_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 一、设备基本信息
// ============================================================================

// 芯片类型: 0x01=BR33
#ifndef SM_CFG_CHIP_TYPE
#define SM_CFG_CHIP_TYPE        0x01
#endif

// 设备类型: 0x01=耳机, 0x02=音箱, 0x03=手表
#ifndef SM_CFG_DEVICE_TYPE
#define SM_CFG_DEVICE_TYPE      0x03
#endif

// 协议版本号
#ifndef SM_CFG_PROTO_VER
#define SM_CFG_PROTO_VER        0x0100      // v1.0
#endif

// 固件版本号
#ifndef SM_CFG_FW_VER
#define SM_CFG_FW_VER           0x01000001  // v1.0.0.1
#endif

// ============================================================================
// 二、设备推流能力
// ============================================================================

// 设备推流状态
#ifndef SM_CFG_AVAILABLE
#define SM_CFG_AVAILABLE        1           // 1=可推流, 0=不可推流
#endif

// 分辨率
#ifndef SM_CFG_DEFAULT_WIDTH
#define SM_CFG_DEFAULT_WIDTH    320
#endif
#ifndef SM_CFG_DEFAULT_HEIGHT
#define SM_CFG_DEFAULT_HEIGHT   240
#endif

// 帧率
#ifndef SM_CFG_DEFAULT_FPS
#define SM_CFG_DEFAULT_FPS      10
#endif

// 图像质量范围
#ifndef SM_CFG_QUALITY_MIN
#define SM_CFG_QUALITY_MIN      0
#endif
#ifndef SM_CFG_QUALITY_MAX
#define SM_CFG_QUALITY_MAX      10
#endif

// 推流格式（协议定义: 0x00=JPEG, 0x01=H264, 0x02=JLJPEG）
#ifndef SM_CFG_FORMAT
#define SM_CFG_FORMAT           STREAM_FORMAT_H264
#endif

// 流媒体BLE MTU（设备端配置值，实际协商后的MTU通过 stream_media_core_get_mtu() 获取）
#ifndef SM_CFG_STREAM_BLE_MTU
#define SM_CFG_STREAM_BLE_MTU      	512
#endif

// 流媒体SPP MTU（app用作数据分包参考,设备这边通过这个值定义数组用来做数据缓存区）
#ifndef SM_CFG_STREAM_SPP_MTU
#define SM_CFG_STREAM_SPP_MTU      	2048
#endif

// APP H264 编码目标码率，单位 bps
#ifndef SM_CFG_STREAM_BITRATE
#define SM_CFG_STREAM_BITRATE    1000000
#endif

// I/P帧间隔
#ifndef SM_CFG_IP_INTERVAL_FRAMES
#define SM_CFG_IP_INTERVAL_FRAMES    20
#endif

// 命令超时
#ifndef SM_CFG_CMD_TIMEOUT_MS
#define SM_CFG_CMD_TIMEOUT_MS   1000
#endif


// 填充设备基本信息
void stream_media_cfg_get_device_info(u8 *chip_type, u8 *dev_type, u16 *proto_ver, u32 *fw_ver);
// 填充设备推流能力
void stream_media_cfg_get_capability(stream_media_capability_t *cap);


#ifdef __cplusplus
}
#endif

#endif // __STREAM_MEDIA_DEVICE_CONFIG_H__

