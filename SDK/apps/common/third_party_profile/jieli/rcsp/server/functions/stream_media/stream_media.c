#include "system/includes.h"
#include "JL_rcsp_protocol.h"
#include "stream_media.h"
#include "le/ble_api.h"
#include "app_ble_spp_api.h"

#define LOG_TAG_CONST      RCSP
#define LOG_TAG     		"[stream media]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define JLSTM_VER   0x00
/* ==================== 发送重试策略 ==================== */
/**
 * @brief 选择 jlstm_tx_data 的 BLE 发送重试策略
 *
 * JLSTM_TX_RETRY_DEADWAIT   死等：每个子包无限重试直到发送成功
 *                             适用于 BLE 链路稳定、不允许丢帧的场景
 *
 * JLSTM_TX_RETRY_DROPFRAME  丢帧：每个子包重试超时后放弃整帧
 *                             超时后放弃整帧，防止帧率持续下降
 *                             适用于 BLE 带宽不足、优先保帧率的场景
 */
enum {
    JLSTM_TX_RETRY_DEADWAIT  = 0,
    JLSTM_TX_RETRY_DROPFRAME = 1,
};

/* 修改此值切换策略 */
#define JLSTM_TX_RETRY_MODE   JLSTM_TX_RETRY_DROPFRAME

#if JLSTM_TX_RETRY_MODE == JLSTM_TX_RETRY_DROPFRAME
/*
 * 帧级超时：保证最低帧率
 * 整帧发送超时后丢弃，避免积压拖垮帧率
 * 7fps → 1000/7 ≈ 143ms 预算
 */
#define JLSTM_MIN_FPS               7
#define JLSTM_TX_FRAME_TIMEOUT_MS   (1000 / JLSTM_MIN_FPS)
#endif

static jlstm_dev_capability_t g_jlstm_cap;
static u8 jlstm_get_first_bit_idx(u32 bitmap);
static jlstm_cam_cap_t *jlstm_find_cam_by_number(u8 number);
static jlstm_mic_cap_t *jlstm_find_mic_by_number(u8 number);

static u8 jlstm_idx = 0xff;
static u16 jlstm_seq = 0;
static u16 jlstm_mtu = 30; // rcsp每次连上后更新一次，后续由协议更新

// 【新增】异常中断标志位
// 当收到 Error Report 时置 1，jlstm_tx_data 检测到此标志则 abort
static volatile u8 jlstm_abort_flag = 0;
static u8 jlstm_abort_index = 0; // 记录是哪个 index 被 abort

u8 jlstm_create_tx_channel()
{
    jlstm_idx++;
    if (jlstm_idx == 0xff) { // 0xff作为异常序号
        jlstm_idx++;
    }
    return jlstm_idx;
}

#if 0
/**
 * @brief 解析并处理流媒体开始传输的响应 (Option 0x01 Response)
 * @param dat 指向 Response Payload 的指针 (不包含 RCSP Header, Option, SN, Status)
 *            注意：根据 JL_CMD_send 的回调机制，dat 通常指向用户数据区的起始位置
 * @param len Payload 总长度
 */
void jlstm_tx_response_deal(u8 *dat, int len)
{
    if (!dat || len < 2) {
        log_error("[JLSTM] Invalid response data");
        return;
    }

    // 1. 解析协商后的 MTU (前2字节，大端序)
    u16 negotiated_mtu = READ_BIG_U16(dat);
    jlstm_mtu = negotiated_mtu;
    log_info("[JLSTM] Negotiated MTU: %d", jlstm_mtu);

    // 2. 解析状态列表 (LTV 格式)
    // 指针跳过 MTU 字段
    u8 *list_ptr = dat + 2;
    int remaining_len = len - 2;

    while (remaining_len > 0) {
        // 读取 LTV 的 Length 字段
        if (remaining_len < 1) {
            break;
        }

        u8 ltv_len = list_ptr[0];

        // 校验：
        // 1. ltv_len 至少为 3 (Type + Index + Status)
        // 2. 整个块的大小 (1字节Length本身 + ltv_len) 不能超过剩余长度
        if (ltv_len < 3 || (1 + ltv_len) > remaining_len) {
            log_error("[JLSTM] Malformed LTV block: len=%d, rem=%d", ltv_len, remaining_len);
            break;
        }

        // 解析 LTV 内容
        u8 type   = list_ptr[1]; // 0x01: Mic, 0x02: Camera
        u8 index  = list_ptr[2];
        u8 status = list_ptr[3];

        log_info("[JLSTM] Resp Item: Type=%s, Index=%d, Status=%d",
                 (type == 0x01 ? "Mic" : (type == 0x02 ? "Cam" : "Unk")),
                 index, status);

        // 3. 触发回调
        struct jlstm_tx_response_ops *p;
        list_for_each_jlstm_tx_response(p) {
            if (p->callback) {
                // 调用新原型的回调: void (*callback)(u8 type, u8 index, u8 status);
                p->callback(type, index, status);
            }
        }

        // 4. 移动到下一个 LTV 块
        // 当前块占用空间 = 1 (Length字段) + ltv_len (内容长度)
        int block_size = 1 + ltv_len;
        list_ptr += block_size;
        remaining_len -= block_size;
    }
}
#endif

typedef struct {
    // u8 sn;              // 序列号
    u8 option;          // 选项
    u8 payload[0];      // 有效载荷
} rcsp_frame_data_t;
void rcsp_jlstm_cmd_deal(u8 OpCode_SN, u8 *data, u16 len)
{
    put_buf(data, len);

    rcsp_frame_data_t *frame_data = (rcsp_frame_data_t *)data;
    u8 option = frame_data->option;
    u8 *payload;
    u16 payload_len = len - 1;

    if (payload_len) {
        // 存在payload
        payload = frame_data->payload;
    }

    switch (option) {
    case JLSTM_OPTION_GET_DEVICE_INFO:
        log_info("Processing: Get device info");
        jlstm_rx_get_dev_info_deal(OpCode_SN);
        return;

    case JLSTM_OPTION_START_STREAM:
        log_info("Processing: Start stream");
        jlstm_rx_start_stream_deal(payload, payload_len, OpCode_SN);
        return;

    case JLSTM_OPTION_STOP_STREAM:
        log_info("Processing: Stop stream");
        jlstm_rx_stop_stream_deal(payload, payload_len, OpCode_SN);
        return;

    case JLSTM_OPTION_ERROR_REPORT:
        log_info("Processing: Error report");
        return;

    default:
        log_error("Unknown option: 0x%02X", option);
        return;
    }
}

/**
 * @brief 处理接收到的开始传输流媒体命令 (Option 0x01)
 * @note 此函数由 RCSP 协议层在收到 Option 0x01 时调用
 */
void jlstm_rx_start_stream_deal(u8 *dat, int len, u8 opCode_SN)
{
    if (!dat || len < 4) {
        log_error("[JLSTM] Invalid Start Stream Cmd Len");
        return;
    }

    u8 *ptr = dat;

    // 1. 解析头部
    u8 version = *ptr++;          // Version (应为 0x00)
    u16 expected_mtu = READ_BIG_U16(ptr);
    ptr += 2; // App 期望的 MTU
    u8 info = *ptr++;             // Info (Dir + Count)

    u8 dir = (info >> 7) & 0x01;  // 0: Pull (App看设备), 1: Push (设备推App)
    u8 count = info & 0x0F;       // 外设数量

    log_info("[JLSTM] Rx Start: Ver=%d, ExpMTU=%d, Dir=%d, Count=%d",
             version, expected_mtu, dir, count);

    // 2. 协商 MTU
    u16 system_max_mtu = 200;
    u16 negotiated_mtu = (expected_mtu < system_max_mtu) ? expected_mtu : system_max_mtu;

    // 确保 MTU 至少能容纳最小头部 (23 Bytes)，否则无法传输首包
    if (negotiated_mtu < JLSTM_HEADER_FIRST_LEN) {
        negotiated_mtu = JLSTM_HEADER_FIRST_LEN;
    }

    jlstm_mtu = negotiated_mtu;

    // 3. 准备回复缓冲区
    // 最大回复长度估算: 2(MTU) + Count * (Max LTV Size)
    // Camera status=0x02 时 LTV 最大为 1(Len)+1(Type)+8(Value)=10
    // 分配 128 字节足够处理多个外设的状态回复
    u8 resp_buf[128];
    u8 *resp_ptr = resp_buf;

    WRITE_BIG_U16(resp_ptr, negotiated_mtu);
    resp_ptr += 2;

    // 4. 解析并处理每个外设请求
    u8 remaining_len = len - 4; // 减去 Header (Ver+MTU+Info)
    u8 processed_count = 0;

    while (remaining_len > 0 && processed_count < count) {
        if (remaining_len < 1) {
            break;
        }

        u8 ltv_len = *ptr++;
        if (ltv_len < 1 || ltv_len > remaining_len) {
            break;
        }

        u8 type = *ptr++; // 0x01 Mic, 0x02 Camera
        u8 *val_ptr = ptr;
        int val_len = ltv_len - 1; // Length 字段包含了 Type 的长度

        u8 status = 0x00; // 默认成功
        u8 req_index = 0;

        // --- 根据 dir 和设备类型，选择对应的能力源进行校验 ---
        if (type == 0x02) { // Camera
            if (val_len >= 8) {
                req_index   = val_ptr[0];
                u8 number   = val_ptr[1];
                u8 codec    = val_ptr[2];
                u8 fps      = val_ptr[3];
                u16 width   = READ_BIG_U16(&val_ptr[4]);
                u16 height  = READ_BIG_U16(&val_ptr[6]);

                log_info("[JLSTM] Req Cam: Idx=%d, Dir=%s, Codec=%d, %dx%d@%dfps",
                         req_index, dir ? "Push" : "Pull", codec, width, height, fps);

                // 根据 dir 选择校验的能力源
                u32 cap_codec_bitmap = 0;
                u16 cap_w = 0, cap_h = 0;
                u8  cap_fps = 0;
                u8  dev_exists = 0;

                if (dir == 0) {
                    jlstm_cam_cap_t *cam = jlstm_find_cam_by_number(number);
                    if (cam) {
                        cap_codec_bitmap = cam->codec_bitmap;
                        cap_w = cam->max_width;
                        cap_h = cam->max_height;
                        cap_fps = cam->max_fps;
                        dev_exists = 1;
                    }
                } else {
                    if (g_jlstm_cap.local_dev_en && g_jlstm_cap.local_dev.has_video) {
                        cap_codec_bitmap = g_jlstm_cap.local_dev.video_cfg.codec_bitmap;
                        cap_w = g_jlstm_cap.local_dev.video_cfg.max_width;
                        cap_h = g_jlstm_cap.local_dev.video_cfg.max_height;
                        cap_fps = g_jlstm_cap.local_dev.video_cfg.max_fps;
                        dev_exists = 1;
                    }
                }

                if (dev_exists) {
                    u8 codec_supported = (cap_codec_bitmap & BIT(codec)) ? 1 : 0;
                    u8 res_supported   = (width <= cap_w && height <= cap_h) ? 1 : 0;
                    status = (codec_supported && res_supported) ? 0x00 : 0x02;

                    // 无论参数是否支持，都将设备注册的能力信息附上
                    u8  suggested_codec = jlstm_get_first_bit_idx(cap_codec_bitmap);
                    u8  suggested_fps   = cap_fps;
                    u16 suggested_w      = cap_w;
                    u16 suggested_h      = cap_h;

                    *resp_ptr++ = 0x09;
                    *resp_ptr++ = type;
                    *resp_ptr++ = req_index;
                    *resp_ptr++ = status;
                    *resp_ptr++ = suggested_codec;
                    *resp_ptr++ = suggested_fps;
                    WRITE_BIG_U16(resp_ptr, suggested_w);
                    resp_ptr += 2;
                    WRITE_BIG_U16(resp_ptr, suggested_h);
                    resp_ptr += 2;

                    log_info("[JLSTM] Cam status=%d(%s), attach: Codec=%d, %dx%d@%dfps",
                             status, dir ? "Push" : "Pull",
                             suggested_codec, suggested_w, suggested_h, suggested_fps);

                    if (status == 0x00) {
                        struct jlstm_tx_response_ops *p;
                        // 触发回调，通过 option 高位携带 dir 信息：
                        // JLSTM_OPTION_START_STREAM | (dir << 4) 使上层可感知推拉方向
                        u8 opt_with_dir = JLSTM_OPTION_START_STREAM | ((dir & 0x01) << 4);

                        list_for_each_jlstm_tx_response(p) {
                            if (p->callback) {
                                p->callback(opt_with_dir, type, req_index, status, number);
                            }
                        }
                    }
                    goto next_ltv;
                } else {
                    log_error("[JLSTM] Cam not available(Dir=%s)", dir ? "Push" : "Pull");
                    status = 0x01;
                }
            } else {
                status = 0x01;
            }
        } else if (type == 0x01) { // Mic
            if (val_len >= 4) {
                req_index   = val_ptr[0];
                u8 number   = val_ptr[1];
                u8 codec    = val_ptr[2];
                u8 sr_idx   = val_ptr[3];

                log_info("[JLSTM] Req Mic: Idx=%d, Dir=%s, Codec=%d, SR_Idx=%d",
                         req_index, dir ? "Push" : "Pull", codec, sr_idx);

                u32 cap_codec_bitmap = 0;
                u32 cap_sr_bitmap = 0;
                u8  dev_exists = 0;

                if (dir == 0) {
                    jlstm_mic_cap_t *mic = jlstm_find_mic_by_number(number);
                    if (mic) {
                        cap_codec_bitmap = mic->codec_bitmap;
                        cap_sr_bitmap = mic->sr_bitmap;
                        dev_exists = 1;
                    }
                } else {
                    if (g_jlstm_cap.local_dev_en && g_jlstm_cap.local_dev.has_audio) {
                        cap_codec_bitmap = g_jlstm_cap.local_dev.audio_cfg.codec_bitmap;
                        cap_sr_bitmap = g_jlstm_cap.local_dev.audio_cfg.sr_bitmap;
                        dev_exists = 1;
                    }
                }

                if (dev_exists) {
                    u8 codec_supported = (cap_codec_bitmap & BIT(codec)) ? 1 : 0;
                    u8 sr_supported    = (cap_sr_bitmap & BIT(sr_idx)) ? 1 : 0;
                    status = (codec_supported && sr_supported) ? 0x00 : 0x02;

                    u8 suggested_codec = jlstm_get_first_bit_idx(cap_codec_bitmap);
                    u8 suggested_sr    = jlstm_get_first_bit_idx(cap_sr_bitmap);

                    *resp_ptr++ = 0x05;
                    *resp_ptr++ = type;
                    *resp_ptr++ = req_index;
                    *resp_ptr++ = status;
                    *resp_ptr++ = suggested_codec;
                    *resp_ptr++ = suggested_sr;

                    log_info("[JLSTM] Mic status=%d(%s), attach: Codec=%d, SR_Idx=%d",
                             status, dir ? "Push" : "Pull", suggested_codec, suggested_sr);
                    goto next_ltv;
                } else {
                    log_error("[JLSTM] Mic not available(Dir=%s)", dir ? "Push" : "Pull");
                    status = 0x01;
                }
            } else {
                status = 0x01;
            }
        } else {
            status = 0x01; // 未知类型
        }

        // --- 构造回复 LTV (status=0x01 仅返回基础状态) ---
        *resp_ptr++ = 0x03;
        *resp_ptr++ = type;
        *resp_ptr++ = req_index;
        *resp_ptr++ = status;

next_ltv:

        processed_count++;

        // 移动指针到下一个 LTV
        ptr += val_len;
        remaining_len -= (1 + ltv_len); // 1 for Len byte
    }

    int resp_total_len = resp_ptr - resp_buf;

    // 5. 发送回复
    // 协议规定: Response OpCode 0x0037, Status Success
    JL_CMD_response_send(JL_OPCODE_STREAM_CONTROL, JL_PRO_STATUS_SUCCESS, opCode_SN, resp_buf, resp_total_len, 0, NULL);

    log_info("Sent Start Resp: MTU=%d, Items=%d", negotiated_mtu, processed_count);
}
/**
 * @brief 处理接收到的停止传输流媒体命令 (Option 0x02)
 * @note 此函数由 RCSP 协议层在收到 Option 0x02 时调用
 */
void jlstm_rx_stop_stream_deal(u8 *dat, int len, u8 opCode_SN)
{
    if (!dat || len < 2) {
        log_error("Invalid stop Stream Cmd Len");
        return;
    }

    u8 *ptr = dat;

    // 1. 解析头部
    u8 count = *ptr++;
    u8 index;
    u8 status = 0x00;   // 默认成功

    // 5. 发送回复
    // 协议规定: Response OpCode 0x0037, Status Success
    JL_CMD_response_send(JL_OPCODE_STREAM_CONTROL, JL_PRO_STATUS_SUCCESS, opCode_SN, NULL, 0, 0, NULL);
    for (int i = 0; i < count; i++) {
        index = *(ptr + i);
        // 触发回调
        struct jlstm_tx_response_ops *p;
        list_for_each_jlstm_tx_response(p) {
            if (p->callback) {
                p->callback(JLSTM_OPTION_STOP_STREAM, JLSTM_PARAM_INVALID, index, status, JLSTM_PARAM_INVALID);
            }
        }
    }
}

static jlstm_dev_capability_t g_jlstm_cap = {0};

void jlstm_set_device_capability(const jlstm_dev_capability_t *cap)
{
    if (cap) {
        memset(&g_jlstm_cap, 0x0, sizeof(jlstm_dev_capability_t));
        memcpy(&g_jlstm_cap, cap, sizeof(jlstm_dev_capability_t));
    }
}
static u8 jlstm_get_first_bit_idx(u32 bitmap)
{
    for (u8 i = 0; i < 32; i++) {
        if (bitmap & BIT(i)) {
            return i;
        }
    }
    return 0xFF; // 无支持的编码
}

/**
 * @brief 在设备能力列表中查找指定 number 的摄像头
 * @return 找到返回指针，否则返回 NULL
 */
static jlstm_cam_cap_t *jlstm_find_cam_by_number(u8 number)
{
    for (int i = 0; i < g_jlstm_cap.cam_count; i++) {
        if (g_jlstm_cap.cams[i].number == number) {
            return &g_jlstm_cap.cams[i];
        }
    }
    return NULL;
}

/**
 * @brief 在设备能力列表中查找指定 number 的麦克风
 * @return 找到返回指针，否则返回 NULL
 */
static jlstm_mic_cap_t *jlstm_find_mic_by_number(u8 number)
{
    for (int i = 0; i < g_jlstm_cap.mic_count; i++) {
        if (g_jlstm_cap.mics[i].number == number) {
            return &g_jlstm_cap.mics[i];
        }
    }
    return NULL;
}

/**
 * @brief 处理接收到的获取外设信息命令 (Option 0x00 Response)
 * @note 协议要求：Mic 和 Camera 都必须返回。若无设备，Length=1 (Type only, No Value).
 */
void jlstm_rx_get_dev_info_deal(u8 opCode_SN)
{
    // 最大可能长度:
    // 4 Cams * 12 bytes (1 Len + 1 Type + 10 Val) = 48
    // 4 Mics * 11 bytes (1 Len + 1 Type + 9 Val)  = 44
    // 1 Local Dev: 1 Len + 1 Type + 1 Num + 1 video_cfg_len + 9 video_cfg + 1 audio_cfg_len + 9 audio_cfg = 23
    // Total = 115 bytes. 分配 128 字节足够。
    u8 buf[128];
    u8 *ptr = buf;

    // 1. 构造 Camera 列表
    if (g_jlstm_cap.cam_count > 0) {
        for (int i = 0; i < g_jlstm_cap.cam_count; i++) {
            jlstm_cam_cap_t *cam = &g_jlstm_cap.cams[i];

            // Length = Type(1) + Value(10) = 11 (0x0B)
            *ptr++ = 0x0B;
            *ptr++ = 0x02; // Type: Camera

            // Value (10 Bytes)
            *ptr++ = cam->number;
            WRITE_BIG_U32(ptr, cam->codec_bitmap);
            ptr += 4;
            WRITE_BIG_U16(ptr, cam->max_width);
            ptr += 2;
            WRITE_BIG_U16(ptr, cam->max_height);
            ptr += 2;
            *ptr++ = cam->max_fps;
        }
    } else {
        // 没有 Camera，返回空 LTV: Length=1 (Type only)
        *ptr++ = 0x01;
        *ptr++ = 0x02; // Type: Camera
    }

    // 2. 构造 Mic 列表
    if (g_jlstm_cap.mic_count > 0) {
        for (int i = 0; i < g_jlstm_cap.mic_count; i++) {
            jlstm_mic_cap_t *mic = &g_jlstm_cap.mics[i];

            // Length = Type(1) + Value(9) = 10 (0x0A)
            *ptr++ = 0x0A;
            *ptr++ = 0x01; // Type: Mic

            // Value (9 Bytes)
            *ptr++ = mic->number;
            WRITE_BIG_U32(ptr, mic->codec_bitmap);
            ptr += 4;
            WRITE_BIG_U32(ptr, mic->sr_bitmap);
            ptr += 4;
        }
    } else {
        // 没有 Mic，返回空 LTV: Length=1 (Type only)
        *ptr++ = 0x01;
        *ptr++ = 0x01; // Type: Mic
    }

    // 3. 构造 Local Device 列表
    if (g_jlstm_cap.local_dev_en) {
        jlstm_local_dev_cap_t *local = &g_jlstm_cap.local_dev;

        // 计算各配置长度
        u8 video_cfg_len = local->has_video ? sizeof(jlstm_video_cfg_t) : 0;
        u8 audio_cfg_len = local->has_audio ? sizeof(jlstm_audio_cfg_t) : 0;

        // Length = Type(1) + Value(2+N+M) = 3 + N + M
        u8 ltv_len = 1 + 1 + 1 + video_cfg_len + 1 + audio_cfg_len; // Type + number + video_cfg_len + video_cfg + audio_cfg_len + audio_cfg
        *ptr++ = ltv_len;
        *ptr++ = 0x03; // Type: Local Device

        // Value
        *ptr++ = local->number;           // 本机设备编号

        // 视频配置
        *ptr++ = video_cfg_len;            // 视频配置长度(N)
        if (local->has_video) {
            WRITE_BIG_U32(ptr, local->video_cfg.codec_bitmap);
            ptr += 4;
            WRITE_BIG_U16(ptr, local->video_cfg.max_width);
            ptr += 2;
            WRITE_BIG_U16(ptr, local->video_cfg.max_height);
            ptr += 2;
            *ptr++ = local->video_cfg.max_fps;
        }

        // 音频配置
        *ptr++ = audio_cfg_len;            // 音频配置长度(M)
        if (local->has_audio) {
            WRITE_BIG_U32(ptr, local->audio_cfg.codec_bitmap);
            ptr += 4;
            WRITE_BIG_U32(ptr, local->audio_cfg.sr_bitmap);
            ptr += 4;
            *ptr++ = local->audio_cfg.channels;
        }
    }

    int total_len = ptr - buf;

    // 4. 发送回复
    JL_CMD_response_send(JL_OPCODE_STREAM_CONTROL, JL_PRO_STATUS_SUCCESS, opCode_SN, buf, total_len, 0, NULL);
    log_info("Sent Dev Info: CamCnt=%d, MicCnt=%d, LocalDev=%d, TotalLen=%d",
             g_jlstm_cap.cam_count, g_jlstm_cap.mic_count, g_jlstm_cap.local_dev_en, total_len);
}

/**
 * @brief 发起流媒体开始传输命令 (Option 0x01)
 *
 * @param dir           0: Pull (请求对方发), 1: Push (我发给对方)
 * @param expected_mtu  期望的 MTU (大端序处理在内部完成)
 * @param configs       外设配置数组指针
 * @param count         外设数量
 * @return int          0: 成功, -1: 失败
 */
int jlstm_tx_start(u8 dir, u16 expected_mtu, const jlstm_dev_config_t *configs, u8 count)
{
    // 1. 准入判断
    // if (!rcsp_is_connected()) { // 假设有此判断函数
    //     log_error("[JLSTM] RCSP not connected");
    //     return -1;
    // }

    if (count == 0 || count > JLSTM_MAX_DEV_COUNT) {
        log_error("[JLSTM] Invalid count: %d", count);
        return -1;
    }

    // 2. 计算 Payload 总长度
    // Option(1) + (Header: Ver(1) + Expected MTU(2) + Info(1)) = 5 bytes
    int option_len = 1;
    int payload_header_len = 4;
    int dev_params_len = 0;

    for (int i = 0; i < count; i++) {
        if (configs[i].type == JLSTM_DEV_CAMERA) {
            // Camera LTV: Length(1) + Type(1) + Value(8) = 10 bytes
            dev_params_len += 10;
        } else if (configs[i].type == JLSTM_DEV_MIC) {
            // Mic LTV: Length(1) + Type(1) + Value(4) = 6 bytes
            dev_params_len += 6;
        } else {
            log_error("Unsupported dev type: %d", configs[i].type);
            return -1;
        }
    }

    // 总长度 = Option + Version/MTU/Info + Dev Params
    int total_payload_len = option_len + payload_header_len + dev_params_len;

    // 3. 分配并构造 Payload
    u8 *txbuf = (u8 *)malloc(total_payload_len);
    if (txbuf == NULL) {
        return -1;
    }

    u8 *ptr = txbuf;

    // --- 填充 Option ---
    *ptr++ = 0x01; // Option: Start Stream

    // --- 填充头部 ---
    *ptr++ = JLSTM_VER;             // Version: 0x00
    WRITE_BIG_U16(ptr, expected_mtu); // Expected MTU
    ptr += 2;

    // Info: Bit7=dir, Bit0-3=count
    *ptr++ = ((dir & 0x01) << 7) | (count & 0x0F);

    // --- 填充外设 LTV 列表 ---
    for (int i = 0; i < count; i++) {
        const jlstm_dev_config_t *cfg = &configs[i];

        if (cfg->type == JLSTM_DEV_CAMERA) {
            // Camera LTV Block
            *ptr++ = 0x09; // Length: Type(1) + Value(8) = 9
            *ptr++ = 0x02; // Type: Camera
            *ptr++ = cfg->index;
            *ptr++ = cfg->number;
            *ptr++ = cfg->param.cam.codec;
            *ptr++ = cfg->param.cam.fps;
            WRITE_BIG_U16(ptr, cfg->param.cam.width);
            ptr += 2;
            WRITE_BIG_U16(ptr, cfg->param.cam.height);
            ptr += 2;

        } else if (cfg->type == JLSTM_DEV_MIC) {
            // Mic LTV Block
            *ptr++ = 0x05; // Length: Type(1) + Value(4) = 5
            *ptr++ = 0x01; // Type: Mic
            *ptr++ = cfg->index;
            *ptr++ = cfg->number;
            *ptr++ = cfg->param.mic.codec;
            *ptr++ = cfg->param.mic.sr;
        }
    }

    // 4. 发送 RCSP 命令 (Option 0x01)
    JL_ERR err = JL_CMD_send(JL_OPCODE_STREAM_CONTROL, txbuf, total_payload_len, JL_NEED_RESPOND, 0, NULL);


    if (err == 0) {
        log_info("Tx Start Cmd Sent Successfully");
    } else {
        log_error("Tx Start Cmd Failed: %d", err);
    }

    free(txbuf);
    return err;
}

// 基础头部 (所有包都有, 12 Bytes)
typedef struct __attribute__((packed))
{
    u16 opcode;        // 0x0037 (Big Endian)
    u8  data_type;     // Bit7: Last, Bit0: HasHeader, Bit1: HasData
    u8  reserved;      // 0x00
    u16 payload_len;   // Payload 长度 (Big Endian)
    u16 seq;           // 帧序号 (Big Endian)
    u32 offset;        // 偏移量 (Big Endian)
}
jlstm_pkt_base_t;

// 扩展头部 (仅首包 Offset==0 时有, 11 Bytes)
typedef struct __attribute__((packed))
{
    u32 frame_size;    // 整帧大小 (Big Endian)
    u32 timestamp;     // 时间戳 (Big Endian)
    u16 crc;           // CRC16 (Big Endian)
    u8  index;         // 外设 Index
}
jlstm_pkt_ext_t;

// 完整首包结构 (23 Bytes Header + Data)
typedef struct __attribute__((packed))
{
    jlstm_pkt_base_t base;
    jlstm_pkt_ext_t  ext;
    u8               data[0];
}
jlstm_pkt_first_t;

// 后续分包结构 (12 Bytes Header + Data)
typedef struct __attribute__((packed))
{
    jlstm_pkt_base_t base;
    u8               data[0];
}
jlstm_pkt_sub_t;

#define ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE 0x000b
#define ATT_CHARACTERISTIC_ae04_01_VALUE_HANDLE 0x000d
#define ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE 0x000e

int jlstm_tx_data(u8 index, u8 data_type, u32 timestamp, u16 crc, u8 *data, u32 data_len)
{
    if (!data || data_len == 0) {
        return -1;
    }
    if (jlstm_mtu < JLSTM_HEADER_FIRST_LEN) {
        log_error("MTU too small: %d", jlstm_mtu);
        return -1;
    }
    u16 crc_result = 0;

    u8 *tx_buf = zalloc(jlstm_mtu);
    if (!tx_buf) {
        return -1;
    }

    u32 remaining = data_len;
    u32 current_offset = 0;
    u16 current_seq = jlstm_seq; // 获取当前帧 Seq，整帧不变

#if JLSTM_TX_RETRY_MODE == JLSTM_TX_RETRY_DROPFRAME
    u32 frame_start_ms = jiffies_msec();  // 帧级超时计时起点
#endif

    // --- 1. 构造并发送首包 (Offset == 0) ---
    {
        // 在发送首包前，检查是否已经被要求终止
        if (jlstm_abort_flag && jlstm_abort_index == index) {
            log_info("Frame aborted before start");
            jlstm_abort_flag = 0; // 清除标志
            free(tx_buf);
            return -2; // 返回特定错误码表示被中断
        }

        jlstm_pkt_first_t *pkt = (jlstm_pkt_first_t *)tx_buf;

        // 填充基础头部
        // pkt->base.opcode      = HTONS(JLSTM_OPCODE);
        pkt->base.opcode      = JLSTM_OPCODE;
        pkt->base.data_type   = data_type & (~JLSTM_DT_LAST_FRAG); // 先清除 Last 标志
        pkt->base.reserved    = 0x00;
        pkt->base.seq         = HTONS(current_seq);
        pkt->base.offset      = HTONL(0);

        // 填充扩展头部 (仅首包)
        pkt->ext.frame_size   = HTONL(data_len);
        pkt->ext.timestamp    = HTONL(timestamp);
        pkt->ext.crc          = HTONS(crc);
        pkt->ext.index        = index;

        // 计算首包最大 Payload
        u16 max_payload = jlstm_mtu - JLSTM_HEADER_FIRST_LEN;
        u16 cur_payload = (remaining < max_payload) ? (u16)remaining : max_payload;

        pkt->base.payload_len = HTONS(cur_payload);

        // 拷贝数据
        memcpy(pkt->data, data, cur_payload);

        // 判断是否单包完成
        if (cur_payload == remaining) {
            pkt->base.data_type |= JLSTM_DT_LAST_FRAG; // 设置 Last 标志
        }

        // 发送首包（结合协议内容将数据发送出去）
        u16 ble_con_handle = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
        int ret = bt_rcsp_data_send(ble_con_handle, NULL, tx_buf, JLSTM_HEADER_FIRST_LEN + cur_payload, ATT_CHARACTERISTIC_ae04_01_VALUE_HANDLE, ATT_OP_NOTIFY);
        while (ret) {
            /* log_debug("%s ret:%d  retry\n", __func__, ret); */
            ret = bt_rcsp_data_send(ble_con_handle, NULL, tx_buf, JLSTM_HEADER_FIRST_LEN + cur_payload, ATT_CHARACTERISTIC_ae04_01_VALUE_HANDLE, ATT_OP_NOTIFY);
            os_time_dly(1);
        }

        // 更新状态
        remaining -= cur_payload;
        current_offset += cur_payload;
        data += cur_payload;

#if JLSTM_TX_RETRY_MODE == JLSTM_TX_RETRY_DROPFRAME
        /* 首包发送完毕，检查帧级超时 */
        if (jiffies_msec2offset(frame_start_ms, jiffies_msec()) > JLSTM_TX_FRAME_TIMEOUT_MS) {
            log_warn("drop frame: seq=%d timeout after first pkt, elapsed=%d ms",
                     current_seq, jiffies_msec2offset(frame_start_ms, jiffies_msec()));
            free(tx_buf);
            return -3;
        }
#endif
    }

    // --- 2. 构造并发送后续分包 (Offset > 0) ---
    while (remaining > 0) {
#if JLSTM_TX_RETRY_MODE == JLSTM_TX_RETRY_DROPFRAME
        /* 发下一个子包前，先检查帧级超时 */
        if (jiffies_msec2offset(frame_start_ms, jiffies_msec()) > JLSTM_TX_FRAME_TIMEOUT_MS) {
            log_warn("drop frame: seq=%d timeout off=%lu, elapsed=%d ms",
                     current_seq, current_offset, jiffies_msec2offset(frame_start_ms, jiffies_msec()));
            free(tx_buf);
            return -3;
        }
#endif
        jlstm_pkt_sub_t *pkt = (jlstm_pkt_sub_t *)tx_buf;

        // 填充基础头部
        // pkt->base.opcode      = HTONS(JLSTM_OPCODE);
        pkt->base.opcode      = JLSTM_OPCODE;
        pkt->base.data_type   = data_type; // 后续包通常只有裸数据 (0x02)
        pkt->base.reserved    = 0x00;
        pkt->base.seq         = HTONS(current_seq); // Seq 保持不变
        pkt->base.offset      = HTONL(current_offset);

        // 计算分包最大 Payload
        u16 max_payload = jlstm_mtu - JLSTM_HEADER_BASE_LEN;
        u16 cur_payload = (remaining < max_payload) ? (u16)remaining : max_payload;

        pkt->base.payload_len = HTONS(cur_payload);

        // 拷贝数据
        memcpy(pkt->data, data, cur_payload);

        // 判断是否最后一包
        if (cur_payload == remaining) {
            pkt->base.data_type |= JLSTM_DT_LAST_FRAG; // 设置 Last 标志 (e.g., 0x82)
        }

        // 发送分包（结合协议内容将数据发送出去）
        u16 ble_con_handle = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
        int ret = bt_rcsp_data_send(ble_con_handle, NULL, tx_buf, JLSTM_HEADER_BASE_LEN + cur_payload, ATT_CHARACTERISTIC_ae04_01_VALUE_HANDLE, ATT_OP_NOTIFY);
        if (ret == -92) {
            log_debug("%s ret:%d  retry\n", __func__, ret);
            os_time_dly(100);
        }
        while (ret) {
            /* log_debug("%s ret:%d  retry\n", __func__, ret); */
            ret = bt_rcsp_data_send(ble_con_handle, NULL, tx_buf, JLSTM_HEADER_BASE_LEN + cur_payload, ATT_CHARACTERISTIC_ae04_01_VALUE_HANDLE, ATT_OP_NOTIFY);
            os_time_dly(1);
        }

        // 更新状态
        remaining -= cur_payload;
        current_offset += cur_payload;
        data += cur_payload;
    }

    // 整帧发送完毕，Seq 累加
    jlstm_seq++;

    free(tx_buf);
    return 0;
}

/**
 * @brief 发起停止流媒体传输命令 (Option 0x02)
 *
 * @param count         需要停止的流媒体会话数量。
 * @param idx_list      指向一个 u8 数组的指针，数组中存放了所有需要停止传输的Index。
 * @return int          0: 成功, -1: 失败
 */
int jlstm_tx_stop(u8 count, u8 *idx_list)
{
    if (!idx_list || count == 0 || count > JLSTM_MAX_DEV_COUNT) {
        log_error("Invalid stop params");
        return -1;
    }

    // Payload 结构: Count(1) + Index List(N)
    int txlen = 1 + count;
    u8 *txbuf = malloc(txlen);
    if (txbuf == NULL) {
        return -1;
    }

    u8 *ptr = txbuf;
    *ptr++ = count; // Count
    memcpy(ptr, idx_list, count); // Index List

    // 发送 RCSP 命令: Opcode 0xC037, Option 0x02
    // 注意：JL_CMD_send 的第一个参数是 Opcode，Payload 包含 Option
    // 这里我们需要构造包含 Option 0x02 的完整 Payload
    // 但通常 JL_CMD_send 对于 0x37 这种多 Option 命令，可能需要特殊处理
    // 假设 JL_CMD_send 发送的是 Parameter 部分，我们需要把 Option 放在最前面

    u8 cmd_buf[16]; // 最大 1 + JLSTM_MAX_DEV_COUNT(15) = 16 bytes
    if (txlen > sizeof(cmd_buf)) {
        free(txbuf);
        return -1;
    }

    cmd_buf[0] = 0x02; // Option: Stop
    memcpy(&cmd_buf[1], txbuf, txlen);

    JL_ERR err = JL_CMD_send(JL_OPCODE_STREAM_CONTROL, cmd_buf, 1 + txlen, JL_NEED_RESPOND, 0, NULL);

    free(txbuf);

    if (err == 0) {
        log_info("Stop Cmd Sent Successfully");
    } else {
        log_error("Stop Cmd Failed: %d", err);
    }

    return err;
}

/**
 * @brief 处理接收到的流数据异常报告（收到接收端option:0x03时调用）
 */
void jlstm_rx_error_deal(u8 opCode_SN, u8 *dat, int len)
{
    if (!dat || len < 4) {
        log_error("Invalid error report data");
        return;
    }

    u8 index  = dat[0];
    u16 seq   = READ_BIG_U16(&dat[1]);
    u8 reason = dat[3];

    log_warn("Recv Error Report: Index=%d, Seq=%d, Reason=%d", index, seq, reason);

    // 【关键】设置中断标志
    // 只有当报错的 index 是当前活跃流时才生效（简单起见，这里直接设置，tx_data 里再判断）
    jlstm_abort_flag = 1;
    jlstm_abort_index = index;

    // 协议要求回复(Status Success)
    JL_CMD_response_send(JL_OPCODE_STREAM_CONTROL, JL_PRO_STATUS_SUCCESS, opCode_SN, NULL, 0, 0, NULL);
}

/**
 * @brief 处理 SDK 通过 AE03 推送过来的流媒体数据包
 *
 * 协议流数据格式（参考手表流媒体通信协议）：
 *   - 首包 (offset==0)，头部共 23 字节：
 *       opcode(2) + data_type(1) + reserved(1) + payload_len(2) +
 *       seq(2) + offset(4) + frame_size(4) + timestamp(4) + crc(2) + index(1)
 *   - 后续分包 (offset>0)，头部共 12 字节：
 *       opcode(2) + data_type(1) + reserved(1) + payload_len(2) + seq(2) + offset(4)
 *
 * 解析完成后回调已通过 REGISTER_JLSTM_RX_PUSH_OPS 注册的处理函数。
 */
void jlstm_rx_push_stream_data(u8 *data, u16 len)
{
    if (!data || len < JLSTM_HEADER_BASE_LEN) {
        log_error("[JLSTM] Push data too short: %d", len);
        return;
    }

    jlstm_pkt_base_t *base = (jlstm_pkt_base_t *)data;

    // 校验 opcode
    if (base->opcode != HTONS(JLSTM_OPCODE)) {
        log_error("[JLSTM] Push bad opcode: 0x%04x", base->opcode);
        return;
    }

    u32 offset      = HTONL(base->offset);
    u16 payload_len = HTONS(base->payload_len);

    jlstm_push_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    /* ── 每包基础信息 ── */
    frame.codec_type = 0x01;	//暂设固定值，后续通过协议通信内容判断编码类型
    frame.data_type   = base->data_type;
    frame.seq         = HTONS(base->seq);
    frame.offset      = offset;
    frame.payload_len = payload_len;

    if (offset == 0) {
        /* ── 首包扩展信息 ── */
        if (len < JLSTM_HEADER_FIRST_LEN) {
            log_error("[JLSTM] Push first pkt too short: %d", len);
            return;
        }
        jlstm_pkt_first_t *first = (jlstm_pkt_first_t *)data;
        frame.index      = first->ext.index;
        frame.frame_size = HTONL(first->ext.frame_size);
        frame.timestamp  = HTONL(first->ext.timestamp);
        frame.crc        = HTONS(first->ext.crc);
        frame.payload    = first->data;

        log_info("[JLSTM] Push rx first: idx=%d seq=%d fsize=%lu ts=%lu crc=0x%04x plen=%d",
                 frame.index, frame.seq, frame.frame_size, frame.timestamp, frame.crc, payload_len);
    } else {
        /* ── 后续分包 ── */
        jlstm_pkt_sub_t *sub = (jlstm_pkt_sub_t *)data;
        frame.payload = sub->data;

        log_info("[JLSTM] Push rx sub: seq=%d off=%lu plen=%d",
                 frame.seq, offset, payload_len);
    }

    // 回调上层注册的接收处理函数
    struct jlstm_rx_push_ops *p;
    list_for_each_jlstm_rx_push(p) {
        if (p->on_push_data) {
            p->on_push_data(&frame);
        }
    }
}







