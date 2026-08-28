#ifndef __STREAM_MEDIA_PROTOCOL_H__
#define __STREAM_MEDIA_PROTOCOL_H__

#include "typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 一、协议常量
// ============================================================================

// 命令包固定头尾
#define STREAM_MEDIA_HEAD           0x4A4C50    // "JLP"
#define STREAM_MEDIA_TAIL           0xAA

// 命令包固定偏移
#define STREAM_MEDIA_HEAD_SIZE      3           // head body data
#define STREAM_MEDIA_RESERVED_SIZE  1
#define STREAM_MEDIA_LENGTH_SIZE    2
#define STREAM_MEDIA_OPCODE_SIZE    2
#define STREAM_MEDIA_TYPE_SIZE      1
#define STREAM_MEDIA_SN_SIZE        1
#define STREAM_MEDIA_TAIL_SIZE      1
// 命令包最小长度 = 3+1+2+2+1+1+1 = 11 (不含 payload)
#define STREAM_MEDIA_CMD_MIN_LEN    11

// 命令超时（ms）
#define STREAM_MEDIA_CMD_TIMEOUT_MS 1000
#define STREAM_MEDIA_CMD_RETRY_MAX  1

// ============================================================================
// 二、OpCode 定义
// ============================================================================

#define STREAM_MEDIA_OPCODE_ENTER       0x0001  // 进入命令
#define STREAM_MEDIA_OPCODE_CTRL        0x0002  // 流控制命令
#define STREAM_MEDIA_OPCODE_NAV         0x0003  // 导航信息命令
#define STREAM_MEDIA_OPCODE_OFFLINE_NAV 0x0004  // 离线导航命令

// 流控制子命令
#define STREAM_MEDIA_SUB_QUERY_PARAM    0x01    // 查询设备支持的参数
#define STREAM_MEDIA_SUB_START_STREAM   0x02    // 开始传输流媒体
#define STREAM_MEDIA_SUB_STOP_STREAM    0x03    // 结束传输流媒体
#define STREAM_MEDIA_SUB_ABNORMAL       0x04    // 异常上报

// ============================================================================
// 三、错误代码
// ============================================================================

enum {
    STREAM_MEDIA_ERR_OK              = 0x00, // 成功
    STREAM_MEDIA_ERR_FAIL            = 0x01, // 失败
    STREAM_MEDIA_ERR_PARAM_UNSUPPORT = 0x02, // 参数不支持
    STREAM_MEDIA_ERR_BUSY            = 0x03, // 设备忙碌
    STREAM_MEDIA_ERR_ABNORMAL        = 0xFF, // 设备异常
};

// 异常原因
enum {
    STREAM_MEDIA_ABNORMAL_CRC_FAIL   = 0x01, // CRC 校验失败
    STREAM_MEDIA_ABNORMAL_DEC_FAIL   = 0x02, // 解码失败
    STREAM_MEDIA_ABNORMAL_NO_HEAD    = 0x03, // 缺失头数据
    STREAM_MEDIA_ABNORMAL_SEQ_ERR    = 0x04, // 帧序号错误
    STREAM_MEDIA_ABNORMAL_OFFSET_ERR = 0x05, // 偏移量错误
    STREAM_MEDIA_ABNORMAL_UNKNOWN    = 0x06, // 其他未知错误
};

// ============================================================================
// 四、data type info bit 定义
// ============================================================================

#define STREAM_MEDIA_TYPE_IS_CMD     0x01    // bit0: 1=命令包, 0=回复包
#define STREAM_MEDIA_TYPE_NEED_RSP   0x02    // bit1: 1=需要回复, 0=不需要回复

// 流数据 data type bit 定义
#define STREAM_MEDIA_DATA_HAS_HEAD   0x01    // bit0: 带完整头
#define STREAM_MEDIA_DATA_HAS_PAYLOAD 0x02   // bit1: 带裸数据
#define STREAM_MEDIA_DATA_IS_LAST    0x80    // bit7: 最后分片

// ============================================================================
// 五、LTV 数据类型定义（进入命令回复 / 查询参数回复）
// ============================================================================

// 进入命令回复 LTV Type
enum {
    LTV_TYPE_CHIP_TYPE      = 0x01, // 芯片类型
    LTV_TYPE_DEVICE_TYPE    = 0x02, // 设备类型
    LTV_TYPE_PROTO_VER      = 0x03, // 协议版本号（2 Bytes）
    LTV_TYPE_FW_VER         = 0x04, // 固件版本号（4 Bytes）
};

// 查询参数回复 LTV Type
enum {
    LTV_TYPE_AVAILABLE      = 0x01, // 设备可推流状态
    LTV_TYPE_WIDTH_LIST     = 0x02, // 支持的宽度列表
    LTV_TYPE_HEIGHT_LIST    = 0x03, // 支持的高度列表
    LTV_TYPE_FPS_LIST       = 0x04, // 支持的帧率列表
    LTV_TYPE_QUALITY_RANGE  = 0x05, // 图像质量范围（2 Bytes）
    LTV_TYPE_FORMAT_LIST    = 0x06, // 支持的格式列表
    LTV_TYPE_MTU_SIZE       = 0x07, // 流媒体传输 MTU（2 Bytes）
    LTV_TYPE_BITRATE        = 0x08, // 码率，4 Bytes, bps
    LTV_TYPE_IP_INTERVAL    = 0x09, // I/P帧间隔
};

// 离线导航LTV
enum {
    OFFLINE_NAV_LTV_COORDINATES = 0x01,
    OFFLINE_NAV_LTV_START_NAME  = 0x02,
    OFFLINE_NAV_LTV_END_NAME    = 0x03,
};
// ============================================================================
// 六、格式 / 设备类型 定义
// ============================================================================

enum {
    STREAM_FORMAT_JPEG      = 0x01,
    STREAM_FORMAT_H264      = 0x02,
    STREAM_FORMAT_JLJPEG    = 0x03,
};

enum {
    PHONE_TYPE_ANDROID      = 0,
    PHONE_TYPE_IOS          = 1,
    PHONE_TYPE_HARMONY      = 2,
};

// ============================================================================
// 七、协议状态机
// ============================================================================

enum {
    STREAM_STATE_IDLE       = 0,    // 空闲
    STREAM_STATE_ENTERED    = 1,    // 已进入会话
    STREAM_STATE_PARAM_READY = 2,   // 参数已查询
    STREAM_STATE_STREAMING  = 3,    // 正在推流
};

// ============================================================================
// 八、数据结构
// ============================================================================


// 流媒体数据包头（独立数据通道）
// 协议格式:
//   opCode(2) | data_type(1) | reserved(1) | length(2) | stream_data(n)
//   stream_data: seq(2) | offset(4) | [frame_size(4) | timestamp(4) | crc(2)] | payload(n)
//   [] 内字段仅 offset==0 时有效
typedef struct  __attribute__((packed))
{
    u16 opcode;             // 命令号
    u8  data_type;          // 数据类型标识 (bit0:完整头, bit1:裸数据, bit7:最后分片)
    u8  reserved;           // 保留
    u16 data_len;           // 数据包长度（= 6 + stream_data 长度）
    u16 seq;                // 帧序号
    u32 offset;             // 偏移量
    // 以下仅 offset==0 时有效（完整头）
    u32 frame_size;         // 帧总大小
    u32 timestamp;          // 时间戳
    u16 crc;                // CRC-16 校验值（小端序）
    u8 stream_type;			// 流数据类型
}
stream_media_data_hdr_t;


// 流媒体设备能力（查询参数回复解析结果）
typedef struct {
    u8  available;          // 是否可推流
    u16 width_list[16];     // 支持的宽度列表
    u8  width_cnt;
    u16 height_list[16];    // 支持的高度列表
    u8  height_cnt;
    u8  fps_list[16];       // 支持的帧率列表
    u8  fps_cnt;
    u8  quality_min;        // 质量最小值
    u8  quality_max;        // 质量最大值
    u8  format_list[8];     // 支持的格式列表
    u8  format_cnt;
    u16 stream_mtu;         // 流媒体传输 MTU
    u32 bitrate_bps;        // App 编码目标码率，bps
    u8  ip_interval_frames; // I帧间隔
} stream_media_capability_t;

// 流控制回复包头（解析后）
typedef struct {
    u8  status;             // 执行状态
    u8  sub_opcode;         // 子命令
} stream_media_ctrl_resp_t;

// 异常上报数据
typedef struct {
    u16 seq;                // 异常帧序号
    u8  reason;             // 异常原因
} stream_media_abnormal_t;

// 导航信息（从 JSON 解析后）
typedef struct {
    u32 remain_mileage;     // 剩余里程（米）
    u32 remain_time;        // 剩余时间（秒）
    u32 arrival_time;       // 到达时间（Unix 时间戳）
    u8  direction;          // 方向类型: 0=straight,1=left,2=right,3=uturn,4=arrived
    char guide_info[128];   // 诱导信息
} stream_media_nav_info_t;

// 离线导航起点、终点信息
typedef struct {
    s32 start_longitude_e6;
    s32 start_latitude_e6;
    s32 end_longitude_e6;
    s32 end_latitude_e6;

    /*
     * 名称指向命令payload，非零结尾。
     * 仅在回调执行期间有效；异步使用必须自行复制。
     */
    const char *start_name;
    u8 start_name_len;

    const char *end_name;
    u8 end_name_len;
} stream_media_offline_nav_info_t;

// ============================================================================
// 九、API 声明
// ============================================================================

// --- CRC16 ---
u16 stream_media_crc16(const u8 *data, int len, u16 last_crc);

// --- 命令包构建 ---
// payload 由调用方提供，返回总长度
int  stream_media_build_enter_cmd(u8 *buf, u16 buf_size, u8 sn, u8 phone_type);
int  stream_media_build_enter_resp(u8 *buf, u16 buf_size, u8 sn, u8 status,
                                   u8 chip_type, u8 dev_type, u16 proto_ver, u32 fw_ver);
int  stream_media_build_ctrl_query(u8 *buf, u16 buf_size, u8 sn);
int  stream_media_build_ctrl_query_resp(u8 *buf, u16 buf_size, u8 sn, u8 status,
                                        const stream_media_capability_t *cap);
int  stream_media_build_ctrl_start(u8 *buf, u16 buf_size, u8 sn,
                                   u16 width, u16 height, u8 fps, u8 quality, u8 format);
int  stream_media_build_ctrl_start_resp(u8 *buf, u16 buf_size, u8 sn, u8 status);
int  stream_media_build_ctrl_stop(u8 *buf, u16 buf_size, u8 sn, u8 is_resp, u8 status);
int  stream_media_build_ctrl_abnormal(u8 *buf, u16 buf_size, u8 sn, u16 seq, u8 reason);
int  stream_media_build_nav_cmd(u8 *buf, u16 buf_size, u8 sn, const char *json_str, u16 json_len);

// --- 流数据包构建（独立数据通道） ---
int  stream_media_build_data_pkt(u8 *buf, u16 buf_size, u8 data_type,
                                 u16 seq, u32 offset,
                                 u32 frame_size, u32 timestamp, u16 crc,
                                 const u8 *payload, u16 payload_len);

// --- 解析 ---
// 验证+解析命令包头，返回 payload 指针和长度
u8  *stream_media_parse_cmd(u8 *data, u16 data_len,
                            u16 *opcode, u8 *data_type, u8 *sn,
                            u16 *payload_len);
// 解析流数据包头
int  stream_media_parse_data_hdr(const u8 *data, u16 data_len, stream_media_data_hdr_t *hdr);

// LTV 解析：从 payload 中提取指定 Type 的 Value
u8  *stream_media_ltv_find(u8 *payload, u16 payload_len, u8 type, u8 *value_len);

// 解析查询参数回复 → capability 结构体
int  stream_media_parse_capability(u8 *payload, u16 payload_len, stream_media_capability_t *cap);

// 解析进入回复 → 设备信息
int  stream_media_parse_enter_resp(u8 *payload, u16 payload_len,
                                   u8 *chip_type, u8 *dev_type, u16 *proto_ver, u32 *fw_ver);

// 解析异常上报 payload
int  stream_media_parse_abnormal(u8 *payload, u16 payload_len, stream_media_abnormal_t *abn);

// 解析离线导航指令
int stream_media_parse_offline_nav(const u8 *payload, u16 payload_len, stream_media_offline_nav_info_t *info);

// 离线导航命令回复
int stream_media_build_offline_nav_resp(u8 *buf, u16 buf_size, u8 sn, u8 status);

#ifdef __cplusplus
}
#endif

#endif // __STREAM_MEDIA_PROTOCOL_H__

