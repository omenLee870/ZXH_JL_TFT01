#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".stream_media_core.data.bss")
#pragma data_seg(".stream_media_core.data")
#pragma const_seg(".stream_media_core.text.const")
#pragma code_seg(".stream_media_core.text")
#endif

#include "system/includes.h"
#include "os/os_api.h"
#include "app_config.h"
#include "btstack/le/att.h"
#include "btstack/bluetooth.h"
#include "btstack_rcsp_user.h"
#include "app_ble_spp_api.h"
#include "stream_media_core.h"
#include "stream_media_protocol.h"
#include "stream_media_device_config.h"

// 命令收发 buffer
#define STREAM_CMD_BUF_SIZE  512
#define STREAM_DATA_BUF_SIZE 1024

// ============================================================================
// RCSP BF00 通道 Handle 定义（与 multi_protocol_main.c 中的 rcsp_profile_data 一致）
// ============================================================================
#define ATT_CHARACTERISTIC_bf01_01_VALUE_HANDLE 0x0011
#define ATT_CHARACTERISTIC_bf02_01_VALUE_HANDLE 0x0013
#define ATT_CHARACTERISTIC_bf03_01_VALUE_HANDLE 0x0016
#define ATT_CHARACTERISTIC_bf04_01_VALUE_HANDLE 0x0018

#define STREAM_MEDIA_ATT_CMD_TX   ATT_CHARACTERISTIC_bf01_01_VALUE_HANDLE   // BF01 WRITE_WITHOUT_RESPONSE  APP→设备 命令
#define STREAM_MEDIA_ATT_CMD_RX   ATT_CHARACTERISTIC_bf02_01_VALUE_HANDLE   // BF02 NOTIFY                   设备→APP 命令回复
#define STREAM_MEDIA_ATT_DATA_TX  ATT_CHARACTERISTIC_bf03_01_VALUE_HANDLE   // BF03 WRITE_WITHOUT_RESPONSE  APP→设备 流数据
#define STREAM_MEDIA_ATT_DATA_RX  ATT_CHARACTERISTIC_bf04_01_VALUE_HANDLE   // BF04 NOTIFY                   设备→APP 流数据/异常

// ============================================================================
// 回调注册
// ============================================================================

static stream_media_stream_start_cb_t     g_stream_start_cb = NULL;
static stream_media_stream_stop_cb_t      g_stream_stop_cb  = NULL;
static stream_media_data_push_cb_t        g_data_push_cb    = NULL;
static stream_media_nav_info_cb_t         g_nav_info_cb     = NULL;
static stream_media_state_change_cb_t     g_state_change_cb = NULL;
static stream_media_offline_nav_cb_t g_offline_nav_cb = NULL;

void stream_media_set_stream_start_cb(stream_media_stream_start_cb_t cb)
{
    g_stream_start_cb = cb;
}

void stream_media_set_stream_stop_cb(stream_media_stream_stop_cb_t cb)
{
    g_stream_stop_cb = cb;
}

void stream_media_set_data_push_cb(stream_media_data_push_cb_t cb)
{
    g_data_push_cb = cb;
}

void stream_media_set_nav_info_cb(stream_media_nav_info_cb_t cb)
{
    g_nav_info_cb = cb;
}

void stream_media_set_state_change_cb(stream_media_state_change_cb_t cb)
{
    g_state_change_cb = cb;
}

void stream_media_set_offline_nav_cb(stream_media_offline_nav_cb_t cb)
{
    g_offline_nav_cb = cb;
}
// ============================================================================
// 状态管理
// ============================================================================

static u8  g_proto_state = STREAM_STATE_IDLE;
static u8  g_cmd_sn = 0;
static u16 g_negotiated_mtu = 512;      // 协商后的流媒体传输 MTU

u8 stream_media_core_get_state(void)
{
    return g_proto_state;
}

u16 stream_media_core_get_mtu(void)
{
    return g_negotiated_mtu;
}

static void set_state(u8 new_state)
{
    u8 old = g_proto_state;
    if (old == new_state) {
        return;
    }
    g_proto_state = new_state;
    if (g_state_change_cb) {
        g_state_change_cb(old, new_state);
    }
}

// ============================================================================
// 发送封装：通过 RCSP BF00 通道发送
// ============================================================================
extern void *rcsp_server_ble_hdl;
extern void *bt_rcsp_spp_hdl;
// 发送接口：如果ble连上了优先采用ble
static int send_sm_data(const u8 *data, u16 len, u16 ble_att_handle, u8 att_op_type)
{
    // 优先走 BLE
    u16 con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
    if (con_hdl) {
        return bt_rcsp_data_send(con_hdl, NULL, (u8 *)data, len, ble_att_handle, att_op_type);
    }
#if TCFG_GATT_OVER_EDR_EN
    con_hdl = app_ble_get_hdl_con_handle(rcsp_server_edr_att_hdl);
    if (con_hdl) {
        /* return bt_rcsp_data_send(con_hdl, NULL, (u8 *)data, len, ble_att_handle, att_op_type); */
        return app_ble_att_send_data(rcsp_server_edr_att_hdl, ble_att_handle, (u8 *)data, len, att_op_type);
    }
#endif
    // 降级走 SPP
    u8 *spp_addr = app_spp_get_hdl_remote_addr(bt_rcsp_spp_hdl);
    if (spp_addr) {
        return app_spp_data_send(bt_rcsp_spp_hdl, (u8 *)data, len);
    }
    return -1;
}

// 发送命令回复
static int send_cmd_resp(const u8 *data, u16 len)
{
    return send_sm_data(data, len, STREAM_MEDIA_ATT_CMD_RX, ATT_OP_NOTIFY);
}

// 发送流数据
static int send_stream_data(const u8 *data, u16 len)
{
    return send_sm_data(data, len, STREAM_MEDIA_ATT_DATA_RX, ATT_OP_NOTIFY);
}

// ============================================================================
// 命令处理
// ============================================================================

static void handle_enter_cmd(const u8 *payload, u16 payload_len, u8 sn, u8 need_rsp)
{
    u8 status = STREAM_MEDIA_ERR_OK;
    if (payload_len < 1) {
        return;
    }

    if (need_rsp) {
        u8 buf[STREAM_CMD_BUF_SIZE];
        u8 chip_type = 0;
        u8 dev_type = 0;
        u16 proto_ver = 0;
        u32 fw_ver = 0;

        stream_media_cfg_get_device_info(&chip_type, &dev_type, &proto_ver, &fw_ver);

        int total = stream_media_build_enter_resp(buf, sizeof(buf), sn, status, chip_type, dev_type, proto_ver, fw_ver);
        if (total > 0) {
            send_cmd_resp(buf, total);
        }
    }
    set_state(STREAM_STATE_ENTERED);
}

static void handle_ctrl_query(const u8 *payload, u16 payload_len, u8 sn, u8 need_rsp)
{
    u8 status = STREAM_MEDIA_ERR_OK;

    stream_media_capability_t cap;
    memset(&cap, 0, sizeof(cap));
    stream_media_cfg_get_capability(&cap);

    // 解析app支持的mtu,进行mtu协商
    u16 app_mtu = 512;
    if (payload_len >= 3) {
        app_mtu = (u16)payload[1] | ((u16)payload[2] << 8);
    }
    if (app_mtu > 0 && app_mtu < cap.stream_mtu) {
        g_negotiated_mtu = app_mtu;
    } else {
        g_negotiated_mtu = cap.stream_mtu;
    }
    cap.stream_mtu = g_negotiated_mtu;  // 回复协商后的 MTU

    if (need_rsp) {
        u8 buf[STREAM_CMD_BUF_SIZE];
        int total = stream_media_build_ctrl_query_resp(buf, sizeof(buf), sn, status, (status == STREAM_MEDIA_ERR_OK) ? &cap : NULL);
        if (total > 0) {
            send_cmd_resp(buf, total);
        }
    }
    if (status == STREAM_MEDIA_ERR_OK) {
        set_state(STREAM_STATE_PARAM_READY);
    }
}

static void handle_ctrl_start(const u8 *payload, u16 payload_len, u8 sn, u8 need_rsp)
{
    if (payload_len < 8) {
        if (need_rsp) {
            u8 buf[STREAM_CMD_BUF_SIZE];
            int total = stream_media_build_ctrl_start_resp(buf, sizeof(buf), sn, STREAM_MEDIA_ERR_FAIL);
            if (total > 0) {
                send_cmd_resp(buf, total);
            }
        }
        return;
    }
    stream_media_stream_cfg_t cfg;
    cfg.width = (u16)payload[1] | ((u16)payload[2] << 8);
    cfg.height = (u16)payload[3] | ((u16)payload[4] << 8);
    cfg.fps = payload[5];
    cfg.quality = payload[6];
    cfg.format = payload[7];
    cfg.stream_mtu = stream_media_core_get_mtu();

    u8 status = STREAM_MEDIA_ERR_OK;
    if (need_rsp) {
        u8 buf[STREAM_CMD_BUF_SIZE];
        int total = stream_media_build_ctrl_start_resp(buf, sizeof(buf), sn, status);
        if (total > 0) {
            send_cmd_resp(buf, total);
        }
    }
    if (status == STREAM_MEDIA_ERR_OK) {
        set_state(STREAM_STATE_STREAMING);
        if (g_stream_start_cb) {
            g_stream_start_cb(&cfg);
        }
    }
}

static void handle_ctrl_stop(const u8 *payload, u16 payload_len, u8 sn, u8 is_cmd)
{
    (void)payload;
    (void)payload_len;
    if (is_cmd) {
        u8 buf[STREAM_CMD_BUF_SIZE];
        int total = stream_media_build_ctrl_stop(buf, sizeof(buf), sn, 1, STREAM_MEDIA_ERR_OK);
        if (total > 0) {
            send_cmd_resp(buf, total);
        }
    }
    if (g_stream_stop_cb) {
        g_stream_stop_cb();
    }
    set_state(STREAM_STATE_ENTERED);
}

static void handle_ctrl_abnormal(const u8 *payload, u16 payload_len)
{
    stream_media_abnormal_t abn;
    stream_media_parse_abnormal((u8 *)payload, payload_len, &abn);
}

static void handle_nav_cmd(const u8 *payload, u16 payload_len)
{
    if (g_nav_info_cb && payload_len > 0) {
        stream_media_nav_info_t info;
        memset(&info, 0, sizeof(info));
        u16 copy_len = payload_len < sizeof(info.guide_info) - 1
                       ? payload_len : sizeof(info.guide_info) - 1;
        memcpy(info.guide_info, payload, copy_len);
        info.guide_info[copy_len] = '\0';
        g_nav_info_cb(&info);
    }
}

static void handle_offline_nav_cmd(const u8 *payload, u16 payload_len, u8 sn)
{
    stream_media_offline_nav_info_t info;
    u8 status = STREAM_MEDIA_ERR_PARAM_UNSUPPORT;
    u8 resp_buf[32];

    if (stream_media_parse_offline_nav(payload, payload_len, &info) == 0) {
        printf("[SM] offline nav start=(%ld,%ld), end=(%ld,%ld)\n",
               (long)info.start_longitude_e6, (long)info.start_latitude_e6,
               (long)info.end_longitude_e6, (long)info.end_latitude_e6);

        if (info.start_name) {
            printf("[SM] start name: %.*s\n", (int)info.start_name_len, info.start_name);
        }
        if (info.end_name) {
            printf("[SM] end name: %.*s\n", (int)info.end_name_len, info.end_name);
        }

        status = g_offline_nav_cb ? g_offline_nav_cb(&info) : STREAM_MEDIA_ERR_PARAM_UNSUPPORT;
    }

    int total = stream_media_build_offline_nav_resp(resp_buf, sizeof(resp_buf), sn, status);
    if (total > 0) {
        send_cmd_resp(resp_buf, total);
    }
}


static void process_cmd_packet(const u8 *data, u16 data_len)
{
    u16 opcode;
    u8  data_type, sn;
    u16 payload_len;
    u8 *payload = stream_media_parse_cmd((u8 *)data, data_len, &opcode, &data_type, &sn, &payload_len);
    if (payload == NULL) {
        return;
    }

    u8 is_cmd  = (data_type & STREAM_MEDIA_TYPE_IS_CMD) != 0;
    u8 need_rsp = (data_type & STREAM_MEDIA_TYPE_NEED_RSP) != 0;

    switch (opcode) {
    case STREAM_MEDIA_OPCODE_ENTER:
        if (is_cmd) {
            handle_enter_cmd(payload, payload_len, sn, need_rsp);
        }
        break;
    case STREAM_MEDIA_OPCODE_CTRL:
        if (is_cmd && payload_len >= 1) {
            switch (payload[0]) {
            case STREAM_MEDIA_SUB_QUERY_PARAM:
                printf("STREAM_MEDIA_SUB_QUERY_PARAM\n");
                handle_ctrl_query(payload, payload_len, sn, need_rsp);
                break;
            case STREAM_MEDIA_SUB_START_STREAM:
                printf("STREAM_MEDIA_SUB_START_STREAM\n");
                handle_ctrl_start(payload, payload_len, sn, need_rsp);
                break;
            case STREAM_MEDIA_SUB_STOP_STREAM:
                printf("STREAM_MEDIA_SUB_STOP_STREAM\n");
                handle_ctrl_stop(payload, payload_len, sn, 1);
                break;
            case STREAM_MEDIA_SUB_ABNORMAL:
                printf("STREAM_MEDIA_SUB_ABNORMAL\n");
                handle_ctrl_abnormal(payload, payload_len);
                break;
            }
        }
        break;
    case STREAM_MEDIA_OPCODE_NAV:
        if (is_cmd) {
            handle_nav_cmd(payload, payload_len);
        }
        break;
    case STREAM_MEDIA_OPCODE_OFFLINE_NAV:
        if (is_cmd) {
            handle_offline_nav_cmd(payload, payload_len, sn);
        }
        break;
    default:
        printf("[SM] unsupported opcode=0x%04x\n", opcode);
        break;
    }
}

// ============================================================================
// 流数据接收处理（APP → 设备方向）
// ============================================================================

static void process_data_packet(const u8 *data, u16 data_len)
{
    stream_media_data_hdr_t hdr;
    if (stream_media_parse_data_hdr(data, data_len, &hdr) != 0) {
        return;
    }

    // offset=0 首包头 23 字节, offset≠0 分片包头 12 字节
    u16 hdr_size = (hdr.offset == 0) ? 23 : 12;
    const u8 *payload = data + hdr_size;
    u16 pay_len = data_len - hdr_size;

    u8 is_last = (hdr.data_type & STREAM_MEDIA_DATA_IS_LAST) != 0;
    if (g_data_push_cb) {
        g_data_push_cb(payload, pay_len, hdr.seq, hdr.offset,
                       hdr.frame_size, hdr.timestamp, hdr.crc, hdr.stream_type, is_last);
    }
}

// ============================================================================
// 数据接收入口（由 ble_rcsp_server.c 的 bt_rcsp_custom_att_write_callback 调用）
// ============================================================================

/* ble接收入口 */
// 命令数据接收 → BF01 写入时调用
void stream_media_on_cmd_recv(const u8 *data, u16 len)
{
    process_cmd_packet(data, len);
}

// 流数据接收 → BF03 写入时调用
void stream_media_on_data_recv(const u8 *data, u16 len)
{
    process_data_packet(data, len);
}

/* spp接收入口(根据spp流式的特性，接收回调时，数据可能分为多段，需要进行拼接) */
#define SM_SPP_BUF_SIZE  (SM_CFG_STREAM_SPP_MTU * 2)

static u8   g_spp_buf[SM_SPP_BUF_SIZE];
static u16  g_spp_buf_len = 0;

// 小端序读取 16 位无符号整数
static u16 rd16_sm(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
}

void stream_media_on_spp_recv(const u8 *data, u16 len)
{
    if (!data || len == 0) {
        return;
    }
    // 1. 追加到拼包缓冲区
    if (g_spp_buf_len + len > SM_SPP_BUF_SIZE) {
        printf("sm_spp: buf overflow, reset\n");
        g_spp_buf_len = 0;
        return;
    }
    memcpy(g_spp_buf + g_spp_buf_len, data, len);
    g_spp_buf_len += len;

    // 2. 循环解析完整包
    while (g_spp_buf_len >= STREAM_MEDIA_CMD_MIN_LEN) {

        u16 total;

        if (g_spp_buf[0] == 0x4A && g_spp_buf[1] == 0x4C && g_spp_buf[2] == 0x50) {
            // --- 命令包: JLP 头 + 0xAA 尾 ---
            u16 payload_len = rd16_sm(g_spp_buf + 4);
            total = STREAM_MEDIA_CMD_MIN_LEN + payload_len;

            if (g_spp_buf_len < total) {
                break;  // 包不完整，等下次回调
            }
            // 校验尾部
            if (g_spp_buf[total - 1] != STREAM_MEDIA_TAIL) {
                printf("sm_spp: cmd tail err, reset\n");
                g_spp_buf_len = 0;
                return;
            }
            process_cmd_packet(g_spp_buf, total);

        } else {
            // --- 数据包: opcode+data_type+... 头 ---
            // 至少需要 12 字节头 (opcode2+type1+reserved1+datalen2+seq2+offset4)
            if (g_spp_buf_len < 12) {
                break;
            }
            // 校验流数据头部 (0x0002 小端序 = 02 00)
            if (rd16_sm(g_spp_buf) != 0x0002) {
                printf("sm_spp: invalid data opcode, reset\n");
                g_spp_buf_len = 0;
                return;
            }
            u16 data_len   = rd16_sm(g_spp_buf + 4);
            u32 offset_val = (u32)g_spp_buf[8] | ((u32)g_spp_buf[9] << 8)
                             | ((u32)g_spp_buf[10] << 16) | ((u32)g_spp_buf[11] << 24);
            u16 hdr_size   = (offset_val == 0) ? 23 : 12;
            total = hdr_size + data_len;

            if (total > SM_SPP_BUF_SIZE) {
                printf("sm_spp: data pkt too big (%d), reset\n", total);
                g_spp_buf_len = 0;
                return;
            }
            if (g_spp_buf_len < total) {
                break;  // 包不完整，等下次回调
            }
            process_data_packet(g_spp_buf, total);
        }

        // 3. 移除已处理的完整包
        g_spp_buf_len -= total;
        if (g_spp_buf_len > 0) {
            memmove(g_spp_buf, g_spp_buf + total, g_spp_buf_len);
        }
    }
}

// ============================================================================
// 设备主动发送接口
// ============================================================================

int stream_media_send_abnormal(u16 seq, u8 reason)
{
    u8 buf[STREAM_CMD_BUF_SIZE];
    int total = stream_media_build_ctrl_abnormal(buf, sizeof(buf), g_cmd_sn, seq, reason);
    if (total < 0) {
        return -1;
    }
    g_cmd_sn++;
    return send_cmd_resp(buf, total);
}

int stream_media_push_frame(const u8 *frame_data, u32 frame_size,
                            u16 seq, u32 timestamp, u8 format)
{
    if (g_proto_state != STREAM_STATE_STREAMING) {
        return -2;
    }
    if (frame_data == NULL || frame_size == 0) {
        return -1;
    }

    u16 mtu = 512;
    u32 offset = 0;
    u16 crc = stream_media_crc16(frame_data, frame_size, 0x0000);
    int total_sent = 0;

    while (offset < frame_size) {
        u8 pkt_buf[STREAM_DATA_BUF_SIZE];
        const u8 *chunk_ptr = frame_data + offset;
        u16 chunk_size = (u16)(frame_size - offset);

        u8 data_type = STREAM_MEDIA_DATA_HAS_PAYLOAD;
        if (offset == 0) {
            data_type |= STREAM_MEDIA_DATA_HAS_HEAD;
            // 首包: CRC 在固定头中, payload 就是纯数据
            if (chunk_size > mtu) {
                chunk_size = mtu;
            }
        } else {
            if (chunk_size > mtu) {
                chunk_size = mtu;
            }
        }
        if (offset + chunk_size >= frame_size) {
            data_type |= STREAM_MEDIA_DATA_IS_LAST;
        }

        int total = stream_media_build_data_pkt(pkt_buf, sizeof(pkt_buf),
                                                data_type, seq, offset, frame_size, timestamp, crc,
                                                chunk_ptr, chunk_size);
        if (total < 0) {
            return -1;
        }

        int ret = send_stream_data(pkt_buf, total);
        if (ret < 0) {
            return -1;
        }

        offset += chunk_size;
        total_sent += chunk_size;
    }
    return total_sent;
}

// ============================================================================
// 初始化 / 退出（简化：RCSP 管理 BLE 连接，这里只重置状态）
// ============================================================================

static u8 g_core_inited = 0;

void stream_media_core_init(void)
{
    if (g_core_inited) {
        return;
    }
    g_core_inited = 1;
    g_negotiated_mtu = 512;
    set_state(STREAM_STATE_IDLE);
    g_cmd_sn = 0;
}

void stream_media_core_exit(void)
{
    if (!g_core_inited) {
        return;
    }
    g_core_inited = 0;
    if (g_stream_stop_cb) {
        g_stream_stop_cb();
    }
    set_state(STREAM_STATE_IDLE);
    g_cmd_sn = 0;
}

