#ifndef __STREAM_MEDIA_H
#define __STREAM_MEDIA_H

#include "system/includes.h"
#include "app_config.h"
#include "btstack_rcsp_user.h"

// 选项码
#define JLSTM_OPTION_GET_DEVICE_INFO  0x00
#define JLSTM_OPTION_START_STREAM     0x01
#define JLSTM_OPTION_STOP_STREAM      0x02
#define JLSTM_OPTION_ERROR_REPORT     0x03

#define JLSTM_PARAM_INVALID      0xFF       // 无效参数


struct jlstm_tx_response_ops {
    void (*callback)(u8 option, u8 type, u8 index, u8 status, u8 num);
};

#define REGISTER_JLSTM_TX_RESPONSE_OPS(tx_ops) \
        const struct jlstm_tx_response_ops jlstm_tx_response_operation_##tx_ops sec(.jlstm_tx_response_operation)

extern const struct jlstm_tx_response_ops jlstm_tx_response_begin[];
extern const struct jlstm_tx_response_ops jlstm_tx_response_end[];

#define list_for_each_jlstm_tx_response(p) \
    for (p = (struct jlstm_tx_response_ops*)jlstm_tx_response_begin; p < (struct jlstm_tx_response_ops*)jlstm_tx_response_end; p++)

/* SDK 推送流媒体帧信息（首包解析结果） */
typedef struct {
    /* ── 首包扩展信息 (仅 offset==0 时有效) ── */
    u8  index;       // 外设会话索引
    u32 frame_size;  // 整帧大小
    u32 timestamp;   // 时间戳
    u16 crc;         // CRC16

    /* ── 每包基础信息 (所有分包均有效) ── */
    u8 codec_type;	//编码类型(0x00:jpeg;0x01:H264)
    u8  data_type;   // 数据类型标志 (Bit0:HasHeader, Bit1:HasData, Bit7:LastFrag)
    u16 seq;         // 帧序号
    u32 offset;      // 当前分包偏移量
    u8  *payload;    // 数据指针
    u16 payload_len; // 数据长度
} jlstm_push_frame_t;

/**
 * @brief 推送流媒体数据接收回调接口
 * 当 SDK 通过 AE03 推送流数据时，底层解析完成后通过此回调通知上层。
 */
struct jlstm_rx_push_ops {
    /**
     * @param frame 指向解析后的当前分包信息。
     *              - 首包专有字段 (index / frame_size / timestamp / crc) 仅 offset==0 时有效。
     *              - 每包基础字段 (data_type / seq / offset / payload / payload_len) 始终有效。
     *              - 当 (frame->data_type & JLSTM_DT_LAST_FRAG) 时，表示该帧最后一包。
     */
    void (*on_push_data)(const jlstm_push_frame_t *frame);
};

#define REGISTER_JLSTM_RX_PUSH_OPS(rx_ops) \
        const struct jlstm_rx_push_ops jlstm_rx_push_operation_##rx_ops sec(.jlstm_rx_push_operation)

extern const struct jlstm_rx_push_ops jlstm_rx_push_begin[];
extern const struct jlstm_rx_push_ops jlstm_rx_push_end[];

#define list_for_each_jlstm_rx_push(p) \
    for (p = (struct jlstm_rx_push_ops*)jlstm_rx_push_begin; p < (struct jlstm_rx_push_ops*)jlstm_rx_push_end; p++)

#define HTONS(x) ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

#define HTONL(x) ((((x) & 0xFF000000) >> 24) | \
                  (((x) & 0x00FF0000) >> 8)  | \
                  (((x) & 0x0000FF00) << 8)  | \
                  (((x) & 0x000000FF) << 24))

/* 协议常量定义 */
#define JLSTM_MAX_DEV_COUNT    15   //协议中option:0x01规定 info 字节中 count 占 4 bits (0-15)，且 0 为无效值，故最大为 15
#define JLSTM_OPCODE          0x0037
#define JLSTM_HEADER_BASE_LEN 12  // 基础头部长度 (Offset > 0)
#define JLSTM_HEADER_EXT_LEN  11  // 扩展头部额外长度 (FrameSize+Time+CRC+Index)
#define JLSTM_HEADER_FIRST_LEN (JLSTM_HEADER_BASE_LEN + JLSTM_HEADER_EXT_LEN) // 23 Bytes

// Data Type Bit Definitions
#define JLSTM_DT_LAST_FRAG    BIT(7) // Bit 7: 最后分片
#define JLSTM_DT_HAS_HEADER   BIT(0) // Bit 0: 带完整头 (JPEG Header)
#define JLSTM_DT_HAS_DATA     BIT(1) // Bit 1: 带裸数据 (JPEG Data)

#define JLSTM_MAX_CAMERAS_CNT 2
#define JLSTM_MAX_MICS_CNT    2

typedef struct __attribute__((packed))
{
    u8      number;         // 物理编号
    u32     codec_bitmap;   // 编码位图
    u16     max_width;
    u16     max_height;
    u8      max_fps;
}
jlstm_cam_cap_t;

typedef struct __attribute__((packed))
{
    u8      number;         // 物理编号
    u32     codec_bitmap;   // 编码位图
    u32     sr_bitmap;      // 采样率位图
}
jlstm_mic_cap_t;

// 视频配置内容 (对应协议 video_cfg_data)
typedef struct __attribute__((packed))
{
    u32     codec_bitmap;   // 视频编码位图 (bit0:JPEG, bit1:H264)
    u16     max_width;      // 分辨率-宽度
    u16     max_height;     // 分辨率-高度
    u8      max_fps;        // 最大帧率
}
jlstm_video_cfg_t;

// 音频配置内容 (对应协议 audio_cfg_data)
typedef struct __attribute__((packed))
{
    u32     codec_bitmap;   // 音频编码位图 (bit0:PCM, bit1:SPEEX, bit2:OPUS, bit3:MSBC, bit4:JLA_V2)
    u32     sr_bitmap;      // 采样率位图
    u8      channels;       // 声道数位图 (bit0:单声道, bit1:多声道)
}
jlstm_audio_cfg_t;

// 本机设备能力 (对应协议 Local Device, Type=0x03)
typedef struct __attribute__((packed))
{
    u8                  number;         // 本机设备编号
    u8                  has_video;      // 是否支持图像显示 (0:不支持, 1:支持)
    jlstm_video_cfg_t   video_cfg;      // 视频配置 (has_video=1时有效)
    u8                  has_audio;      // 是否支持音频播放 (0:不支持, 1:支持)
    jlstm_audio_cfg_t   audio_cfg;      // 音频配置 (has_audio=1时有效)
}
jlstm_local_dev_cap_t;

typedef struct __attribute__((packed))
{
    u8              cam_count;
    jlstm_cam_cap_t cams[JLSTM_MAX_CAMERAS_CNT];

    u8              mic_count;
    jlstm_mic_cap_t mics[JLSTM_MAX_MICS_CNT];

    u8                  local_dev_en;       // 是否启用本机设备 (0:不启用, 1:启用)
    jlstm_local_dev_cap_t local_dev;        // 本机设备能力
}
jlstm_dev_capability_t;

/* 开始传输流媒体数据（指令相关参数） */
typedef enum {
    JLSTM_DEV_MIC = 0x01,
    JLSTM_DEV_CAMERA = 0x02,
    JLSTM_DEV_LOCAL = 0x03
} jlstm_dev_type_t;

typedef struct {
    u8 type;       // 0x01: Mic, 0x02: Camera
    u8 index;      // 会话 ID
    u8 number;     // 设备编号
    union {
        struct {
            u8 codec; // Mic: 0:PCM, 1:SPEEX, 2:OPUS...
            u8 sr;    // Mic: 0:8k, 3:16k...
        } mic;
        struct {
            u8 codec; // Cam: 0:JPEG, 1:H264
            u8 fps;
            u16 width;
            u16 height;
        } cam;
    } param;
} jlstm_dev_config_t;



u8 jlstm_create_tx_channel();
int jlstm_tx_start(u8 dir, u16 expected_mtu, const jlstm_dev_config_t *configs, u8 count);
int jlstm_tx_stop(u8 count, u8 *idx_list);
void jlstm_tx_response_deal(u8 *dat, int len);
void jlstm_rx_get_dev_info_deal(u8 opCode_SN);
void jlstm_rx_start_stream_deal(u8 *dat, int len, u8 opCode_SN);
void jlstm_rx_stop_stream_deal(u8 *dat, int len, u8 opCode_SN);
void jlstm_set_device_capability(const jlstm_dev_capability_t *cap);

/**
 * @brief 发送一帧流媒体数据 (自动分包)
 *
 * @param index     [in] 外设会话索引 (Session Index)
 *                  - 必须与 jlstm_tx_start 中为该外设分配的 index 严格一致。
 *                  - 用于接收端区分是 Camera 流还是 Mic 流。
 *
 * @param data_type [in] 数据类型标志位 (Data Type Flags)
 *                  - Bit 7: 最后分片标志 (Last Fragment)。通常由函数内部自动设置最后一包，调用者无需关心。
 *                  - Bit 1: 是否携带裸数据 (Has Data)。
 *                  - Bit 0: 是否携带完整头 (Has Header, e.g., JPEG SOI/App0)。
 *                  - 常用组合:
 *                    - 0x03 (Binary 11): 既有 JPEG 头又有图像数据 (标准首帧或独立帧)。
 *                    - 0x02 (Binary 10): 只有图像裸数据 (后续分片通常隐含此意，但首包若只发头可用 0x01)。
 *                    - 0x01 (Binary 01): 只有 JPEG 头 (少见，用于分离头和数据的场景)。
 *
 * @param timestamp [in] 时间戳 (Timestamp)
 *                  - 单位通常为毫秒 (ms)。
 *                  - 仅在首包 (Offset=0) 中传输，用于同步音视频或记录采集时间。
 *
 * @param crc       [in] 整帧数据的 CRC16 校验值
 *                  - 对完整的原始数据 (data_len 字节) 进行 CRC16 计算的结果。
 *                  - 仅在首包 (Offset=0) 中传输，接收端用于验证整帧完整性。
 *
 * @param data      [in] 原始数据指针
 *                  - 指向待发送的完整帧数据缓冲区 (例如完整的 JPEG 图片数据)。
 *                  - 函数内部会根据 MTU 自动将其拆分为多个蓝牙包。
 *
 * @param data_len  [in] 原始数据总长度 (Bytes)
 *                  - 完整帧的大小。
 *                  - 如果 data_len > (MTU - 23)，函数会自动进行分包处理。
 *
 * @return int      0: 发送成功; -1: 失败 (如内存不足、MTU 过小等)
 */
int jlstm_tx_data(u8 index, u8 data_type, u32 timestamp, u16 crc, u8 *data, u32 data_len);

void rcsp_jlstm_cmd_deal(u8 OpCode_SN, u8 *data, u16 len);

/**
 * @brief 处理 SDK 通过 AE03 通道推送过来的流媒体原始数据
 *
 * 此函数由 BLE 写回调（att_write_callback）在收到 AE03 写入时调用。
 * 内部解析流数据包头，区分首包与后续分包，并回调 REGISTER_JLSTM_RX_PUSH_OPS 注册的处理函数。
 *
 * @param data 收到的原始数据指针
 * @param len  数据长度
 */
void jlstm_rx_push_stream_data(u8 *data, u16 len);

#endif

