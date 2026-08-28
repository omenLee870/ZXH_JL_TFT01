#include "system/includes.h"
#include "stream_media_protocol.h"

// ============================================================================
// CRC16实现
// ============================================================================
u16 stream_media_crc16(const u8 *data, int len, u16 last_crc)
{
    return CRC16_with_initval(data, (u32)len, last_crc);
}

// ============================================================================
// 内部辅助：写入小端序多字节值
// ============================================================================

static u8 *wr16(u8 *p, u16 v)
{
    *p++ = (u8)(v & 0xFF);
    *p++ = (u8)((v >> 8) & 0xFF);
    return p;
}

static u8 *wr32(u8 *p, u32 v)
{
    *p++ = (u8)(v & 0xFF);
    *p++ = (u8)((v >> 8) & 0xFF);
    *p++ = (u8)((v >> 16) & 0xFF);
    *p++ = (u8)((v >> 24) & 0xFF);
    return p;
}

static u16 rd16(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
}

static u32 rd32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

// ============================================================================
// 内部辅助：写入命令包通用头部，返回 payload 起始指针
// ============================================================================

static u8 *write_cmd_header(u8 *buf, u16 opcode, u8 data_type, u8 sn, u16 payload_len)
{
    u8 *p = buf;
    // head body data (3 bytes, 协议顺序 0x4A 0x4C 0x50 = "JLP")
    *p++ = 0x4A;    // 'J'
    *p++ = 0x4C;    // 'L'
    *p++ = 0x50;    // 'P'
    // reserved
    *p++ = 0x00;
    // length (payload data 长度)
    p = wr16(p, payload_len);
    // opCode
    p = wr16(p, opcode);
    // data type info
    *p++ = data_type;
    // sn
    *p++ = sn;
    return p; // 返回 payload 起始位置
}

// ============================================================================
// 命令包构建
// ============================================================================

int stream_media_build_enter_cmd(u8 *buf, u16 buf_size, u8 sn, u8 phone_type)
{
    u16 payload_len = 1;
    if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
        return -1;
    }
    u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_ENTER,
                             STREAM_MEDIA_TYPE_IS_CMD | STREAM_MEDIA_TYPE_NEED_RSP,
                             sn, payload_len);
    *p++ = phone_type;
    *p++ = STREAM_MEDIA_TAIL;
    return (int)(p - buf);
}

int stream_media_build_enter_resp(u8 *buf, u16 buf_size, u8 sn, u8 status,
                                  u8 chip_type, u8 dev_type, u16 proto_ver, u32 fw_ver)
{
    // LTV: 芯片类型 3B + 设备类型 3B + 协议版本号 4B + 固件版本号 6B = 16B
    u16 ltv_len = 3 + 3 + 4 + 6;
    u16 payload_len = 1 + ltv_len; // status + LTV
    if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
        return -1;
    }
    u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_ENTER, 0x00, sn, payload_len);
    // status
    *p++ = status;
    if (status == STREAM_MEDIA_ERR_OK) {
        // LTV: 芯片类型
        *p++ = 2;
        *p++ = LTV_TYPE_CHIP_TYPE;
        *p++ = chip_type;
        // LTV: 设备类型
        *p++ = 2;
        *p++ = LTV_TYPE_DEVICE_TYPE;
        *p++ = dev_type;
        // LTV: 协议版本号
        *p++ = 3;
        *p++ = LTV_TYPE_PROTO_VER;
        p = wr16(p, proto_ver);
        // LTV: 固件版本号
        *p++ = 5;
        *p++ = LTV_TYPE_FW_VER;
        p = wr32(p, fw_ver);
    }
    *p++ = STREAM_MEDIA_TAIL;
    return (int)(p - buf);
}

int stream_media_build_ctrl_query(u8 *buf, u16 buf_size, u8 sn)
{
    // payload: sub_opcode (1 byte)
    u16 payload_len = 1;
    if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
        return -1;
    }
    u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_CTRL,
                             STREAM_MEDIA_TYPE_IS_CMD | STREAM_MEDIA_TYPE_NEED_RSP,
                             sn, payload_len);
    *p++ = STREAM_MEDIA_SUB_QUERY_PARAM;
    *p++ = STREAM_MEDIA_TAIL;
    return (int)(p - buf);
}

int stream_media_build_ctrl_query_resp(u8 *buf, u16 buf_size, u8 sn, u8 status,
                                       const stream_media_capability_t *cap)
{
    // 预计算 LTV 总长
    u16 ltv_len = 0;
    if (status == STREAM_MEDIA_ERR_OK && cap != NULL) {
        ltv_len = 3; // available: len(2)+type(1)+val(2) = actually 2+1+1 = check below
        // Actually LTV: Length(1) + Type(1) + Value(n)
        // available: 1+1+1 = 3B
        ltv_len = 3;                                                    // available
        ltv_len += 2 + cap->width_cnt * 2;                              // width list
        ltv_len += 2 + cap->height_cnt * 2;                             // height list
        ltv_len += 2 + cap->fps_cnt * 1;                                // fps list
        ltv_len += 2 + 2;                                               // quality range
        ltv_len += 2 + cap->format_cnt * 1;                             // format list
        ltv_len += 2 + 2;                                               // mtu
        ltv_len += 2 + 4;   											// bitrate: Length + Type + u32 Value
        ltv_len += 2 + 1; 												// IP interval
    }
    u16 payload_len = 2 + ltv_len; // status + LTV
    if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
        return -1;
    }
    u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_CTRL, 0x00, sn, payload_len);
    *p++ = status;
    *p++ = STREAM_MEDIA_SUB_QUERY_PARAM;
    if (status == STREAM_MEDIA_ERR_OK && cap != NULL) {
        // available
        *p++ = 2;
        *p++ = LTV_TYPE_AVAILABLE;
        *p++ = cap->available;
        // width list
        *p++ = 1 + cap->width_cnt * 2;
        *p++ = LTV_TYPE_WIDTH_LIST;
        for (int i = 0; i < cap->width_cnt; i++) {
            p = wr16(p, cap->width_list[i]);
        }
        // height list
        *p++ = 1 + cap->height_cnt * 2;
        *p++ = LTV_TYPE_HEIGHT_LIST;
        for (int i = 0; i < cap->height_cnt; i++) {
            p = wr16(p, cap->height_list[i]);
        }
        // fps list
        *p++ = 1 + cap->fps_cnt * 1;
        *p++ = LTV_TYPE_FPS_LIST;
        for (int i = 0; i < cap->fps_cnt; i++) {
            *p++ = cap->fps_list[i];
        }
        // quality range
        *p++ = 3;
        *p++ = LTV_TYPE_QUALITY_RANGE;
        *p++ = cap->quality_min;
        *p++ = cap->quality_max;
        // format list
        *p++ = 1 + cap->format_cnt * 1;
        *p++ = LTV_TYPE_FORMAT_LIST;
        for (int i = 0; i < cap->format_cnt; i++) {
            *p++ = cap->format_list[i];
        }
        // mtu
        *p++ = 3;
        *p++ = LTV_TYPE_MTU_SIZE;
        p = wr16(p, cap->stream_mtu);

        // bitrate
        *p++ = 5;
        *p++ = LTV_TYPE_BITRATE;
        p = wr32(p, cap->bitrate_bps);

        // IP interval
        *p++ = 2;                         // Type(1) + Value(1)
        *p++ = LTV_TYPE_IP_INTERVAL;
        *p++ = cap->ip_interval_frames;
    }
    *p++ = STREAM_MEDIA_TAIL;
    return (int)(p - buf);
}

int stream_media_build_ctrl_start(u8 *buf, u16 buf_size, u8 sn,
                                  u16 width, u16 height, u8 fps, u8 quality, u8 format)
{
    // payload: sub_opcode(1) + width(2) + height(2) + fps(1) + quality(1) + format(1) = 8
    u16 payload_len = 8;
    if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
        return -1;
    }
    u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_CTRL,
                             STREAM_MEDIA_TYPE_IS_CMD | STREAM_MEDIA_TYPE_NEED_RSP,
                             sn, payload_len);
    *p++ = STREAM_MEDIA_SUB_START_STREAM;
    p = wr16(p, width);
    p = wr16(p, height);
    *p++ = fps;
    *p++ = quality;
    *p++ = format;
    *p++ = STREAM_MEDIA_TAIL;
    return (int)(p - buf);
}

int stream_media_build_ctrl_start_resp(u8 *buf, u16 buf_size, u8 sn, u8 status)
{
    u16 payload_len = 2; // status(1) + sub_opCode(1)
    if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
        return -1;
    }
    u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_CTRL, 0x00, sn, payload_len);
    *p++ = status;
    *p++ = STREAM_MEDIA_SUB_START_STREAM;
    *p++ = STREAM_MEDIA_TAIL;
    return (int)(p - buf);
}

int stream_media_build_ctrl_stop(u8 *buf, u16 buf_size, u8 sn, u8 is_resp, u8 status)
{
    if (is_resp) {
        // 回复包: payload = status(1) + sub_opcode(1)
        u16 payload_len = 2;
        if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
            return -1;
        }
        u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_CTRL, 0x00, sn, payload_len);
        *p++ = status;
        *p++ = STREAM_MEDIA_SUB_STOP_STREAM;
        *p++ = STREAM_MEDIA_TAIL;
        return (int)(p - buf);
    } else {
        // 命令包: payload = sub_opcode(1)
        u16 payload_len = 1;
        if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
            return -1;
        }
        u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_CTRL,
                                 STREAM_MEDIA_TYPE_IS_CMD | STREAM_MEDIA_TYPE_NEED_RSP,
                                 sn, payload_len);
        *p++ = STREAM_MEDIA_SUB_STOP_STREAM;
        *p++ = STREAM_MEDIA_TAIL;
        return (int)(p - buf);
    }
}

int stream_media_build_ctrl_abnormal(u8 *buf, u16 buf_size, u8 sn, u16 seq, u8 reason)
{
    // payload: sub_opcode(1) + seq(2) + reason(1) = 4
    u16 payload_len = 4;
    if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
        return -1;
    }
    u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_CTRL,
                             STREAM_MEDIA_TYPE_IS_CMD | STREAM_MEDIA_TYPE_NEED_RSP,
                             sn, payload_len);
    *p++ = STREAM_MEDIA_SUB_ABNORMAL;
    p = wr16(p, seq);
    *p++ = reason;
    *p++ = STREAM_MEDIA_TAIL;
    return (int)(p - buf);
}

int stream_media_build_nav_cmd(u8 *buf, u16 buf_size, u8 sn, const char *json_str, u16 json_len)
{
    if (buf_size < STREAM_MEDIA_CMD_MIN_LEN + json_len) {
        return -1;
    }
    u8 *p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_NAV,
                             STREAM_MEDIA_TYPE_IS_CMD, // 不需要回复
                             sn, json_len);
    memcpy(p, json_str, json_len);
    p += json_len;
    *p++ = STREAM_MEDIA_TAIL;
    return (int)(p - buf);
}

// ============================================================================
// 流数据包构建（独立数据通道）
// ============================================================================

int stream_media_build_data_pkt(u8 *buf, u16 buf_size, u8 data_type,
                                u16 seq, u32 offset,
                                u32 frame_size, u32 timestamp, u16 crc,
                                const u8 *payload, u16 payload_len)
{
    // 基础 stream_data 长度: seq(2)+offset(4)+payload(n) = 6+n
    u16 stream_data_len = 6 + payload_len;
    if (offset == 0) {
        // 完整头: +frame_size(4)+timestamp(4)+crc(2) = 10
        stream_data_len += 10;
    }
    // data_len = opCode(2) + data_type(1) + reserved(1) + data_len(2) + stream_data
    u16 data_len = 6 + stream_data_len;
    u16 total = 2 + data_len;  // opCode(2) + 后续

    if (buf_size < total) {
        return -1;
    }
    u8 *p = buf;
    p = wr16(p, 0x0002);  // opCode（数据包使用固定值，区别于命令包）
    *p++ = data_type;
    *p++ = 0x00; // reserved
    p = wr16(p, data_len);
    p = wr16(p, seq);
    p = wr32(p, offset);
    if (offset == 0) {
        p = wr32(p, frame_size);
        p = wr32(p, timestamp);
        p = wr16(p, crc);
    }
    if (payload_len > 0 && payload != NULL) {
        memcpy(p, payload, payload_len);
        p += payload_len;
    }
    return (int)(p - buf);
}

// ============================================================================
// 解析
// ============================================================================

u8 *stream_media_parse_cmd(u8 *data, u16 data_len,
                           u16 *opcode, u8 *data_type, u8 *sn,
                           u16 *payload_len)
{
    if (data_len < STREAM_MEDIA_CMD_MIN_LEN) {
        return NULL;
    }
    // 检查头 0x4A 0x4C 0x50
    if (data[0] != 0x4A || data[1] != 0x4C || data[2] != 0x50) {
        return NULL;
    }
    // reserved
    // length (2 bytes, little endian)
    *payload_len = rd16(data + 4);
    // 检查尾
    if (data_len < STREAM_MEDIA_CMD_MIN_LEN + *payload_len) {
        return NULL;
    }
    if (data[STREAM_MEDIA_CMD_MIN_LEN - 1 + *payload_len] != STREAM_MEDIA_TAIL) {
        return NULL;
    }
    *opcode = rd16(data + 6);
    *data_type = data[8];
    *sn = data[9];
    // payload 起始位置
    return data + 10;
}

int stream_media_parse_data_hdr(const u8 *data, u16 data_len, stream_media_data_hdr_t *hdr)
{
    if (data_len < 12) {  // opCode(2)+data_type(1)+reserved(1)+data_len(2)+seq(2)+offset(4)=12
        return -1;
    }
    memset(hdr, 0, sizeof(*hdr));
    hdr->opcode = rd16(data);
    hdr->data_type = data[2];
    hdr->reserved = data[3];
    hdr->data_len = rd16(data + 4);
    hdr->seq = rd16(data + 6);
    hdr->offset = rd32(data + 8);

    if (hdr->offset == 0 && data_len >= 22) {
        // 完整头: +frame_size(4)+timestamp(4)+crc(2) = 10
        hdr->frame_size = rd32(data + 12);
        hdr->timestamp  = rd32(data + 16);
        hdr->crc        = rd16(data + 20);
        hdr->stream_type  = data[22];
    }
    return 0;
}

// ============================================================================
// LTV 解析
// ============================================================================

u8 *stream_media_ltv_find(u8 *payload, u16 payload_len, u8 type, u8 *value_len)
{
    u8 *p = payload;
    u8 *end = payload + payload_len;
    while (p + 2 <= end) {
        u8 len = p[0];
        u8 t   = p[1];

        if (p + 1 + len > end) {
            break; // 数据不完整
        }
        if (t == type) {
            *value_len = len - 1; // Value 长度 = Length - Type(1 byte)
            return p + 2;         // 指向 Value
        }
        p += 1 + len; // 跳过当前 LTV
    }
    return NULL;
}

// ============================================================================
// 解析 capability
// ============================================================================

int stream_media_parse_capability(u8 *payload, u16 payload_len, stream_media_capability_t *cap)
{
    memset(cap, 0, sizeof(*cap));
    u8 *val;
    u8 vlen;

    // available
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_AVAILABLE, &vlen);
    if (val && vlen >= 1) {
        cap->available = val[0];
    }

    // width list
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_WIDTH_LIST, &vlen);
    cap->width_cnt = 0;
    if (val) {
        for (int i = 0; i < vlen / 2 && cap->width_cnt < 16; i++) {
            cap->width_list[cap->width_cnt++] = rd16(val + i * 2);
        }
    }

    // height list
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_HEIGHT_LIST, &vlen);
    cap->height_cnt = 0;
    if (val) {
        for (int i = 0; i < vlen / 2 && cap->height_cnt < 16; i++) {
            cap->height_list[cap->height_cnt++] = rd16(val + i * 2);
        }
    }

    // fps list
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_FPS_LIST, &vlen);
    cap->fps_cnt = 0;
    if (val) {
        for (int i = 0; i < vlen && cap->fps_cnt < 16; i++) {
            cap->fps_list[cap->fps_cnt++] = val[i];
        }
    }

    // quality range
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_QUALITY_RANGE, &vlen);
    if (val && vlen >= 2) {
        cap->quality_min = val[0];
        cap->quality_max = val[1];
    }

    // format list
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_FORMAT_LIST, &vlen);
    cap->format_cnt = 0;
    if (val) {
        for (int i = 0; i < vlen && cap->format_cnt < 8; i++) {
            cap->format_list[cap->format_cnt++] = val[i];
        }
    }

    // mtu
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_MTU_SIZE, &vlen);
    if (val && vlen >= 2) {
        cap->stream_mtu = rd16(val);
    }

    // bitrate
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_BITRATE, &vlen);
    if (val && vlen >= 4) {
        cap->bitrate_bps = rd32(val);
    }

    // IP interval
    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_IP_INTERVAL, &vlen);
    if (val && vlen >= 1) {
        cap->ip_interval_frames = val[0];
    }

    return 0;
}


// ============================================================================
// 解析离线导航
// ============================================================================

int stream_media_parse_offline_nav(const u8 *payload, u16 payload_len, stream_media_offline_nav_info_t *info)
{
    u8 *value;
    u8 value_len;

    if (!payload || !info) {
        return -1;
    }

    memset(info, 0, sizeof(*info));

    value = stream_media_ltv_find((u8 *)payload, payload_len, OFFLINE_NAV_LTV_COORDINATES, &value_len);
    if (!value || value_len != 16) {
        return -1;
    }

    info->start_longitude_e6 = (s32)rd32(value);
    info->start_latitude_e6  = (s32)rd32(value + 4);
    info->end_longitude_e6   = (s32)rd32(value + 8);
    info->end_latitude_e6    = (s32)rd32(value + 12);

    if (info->start_longitude_e6 < -180000000 || info->start_longitude_e6 > 180000000 ||
        info->end_longitude_e6 < -180000000 || info->end_longitude_e6 > 180000000 ||
        info->start_latitude_e6 < -90000000 || info->start_latitude_e6 > 90000000 ||
        info->end_latitude_e6 < -90000000 || info->end_latitude_e6 > 90000000) {
        return -1;
    }

    value = stream_media_ltv_find((u8 *)payload, payload_len, OFFLINE_NAV_LTV_START_NAME, &value_len);
    if (value) {
        info->start_name = (const char *)value;
        info->start_name_len = value_len;
    }

    value = stream_media_ltv_find((u8 *)payload, payload_len, OFFLINE_NAV_LTV_END_NAME, &value_len);
    if (value) {
        info->end_name = (const char *)value;
        info->end_name_len = value_len;
    }

    return 0;
}

// ============================================================================
// 离线导航命令回复
// ============================================================================
int stream_media_build_offline_nav_resp(u8 *buf, u16 buf_size, u8 sn, u8 status)
{
    u16 payload_len = 1;
    u8 *p;

    if (!buf ||
        buf_size < STREAM_MEDIA_CMD_MIN_LEN + payload_len) {
        return -1;
    }

    p = write_cmd_header(buf, STREAM_MEDIA_OPCODE_OFFLINE_NAV, 0x00, sn, payload_len);

    *p++ = status;
    *p++ = STREAM_MEDIA_TAIL;

    return (int)(p - buf);
}

// ============================================================================
// 解析进入回复
// ============================================================================

int stream_media_parse_enter_resp(u8 *payload, u16 payload_len,
                                  u8 *chip_type, u8 *dev_type, u16 *proto_ver, u32 *fw_ver)
{
    u8 *val;
    u8 vlen;

    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_CHIP_TYPE, &vlen);
    if (val && vlen >= 1) {
        *chip_type = val[0];
    }

    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_DEVICE_TYPE, &vlen);
    if (val && vlen >= 1) {
        *dev_type = val[0];
    }

    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_PROTO_VER, &vlen);
    if (val && vlen >= 2) {
        *proto_ver = rd16(val);
    }

    val = stream_media_ltv_find(payload, payload_len, LTV_TYPE_FW_VER, &vlen);
    if (val && vlen >= 4) {
        *fw_ver = rd32(val);
    }

    return 0;
}

// ============================================================================
// 解析异常上报 payload
// ============================================================================

int stream_media_parse_abnormal(u8 *payload, u16 payload_len, stream_media_abnormal_t *abn)
{
    // payload: sub_opcode(1) + seq(2) + reason(1) = 4
    if (payload_len < 4) {
        return -1;
    }
    abn->seq = rd16(payload + 1);
    abn->reason = payload[3];
    return 0;
}

