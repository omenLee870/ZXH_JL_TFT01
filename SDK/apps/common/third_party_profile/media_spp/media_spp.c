/*
 * 流媒体独立 SPP 通道(media_spp)
 * 在 RCSP 标准 SPP(0x1101/ch1)之外, 再注册一条独立 UUID 的 SPP 服务并申请独立句柄,
 * 供流媒体独占, 避免与协议命令/文件回传抢占带宽。结构参照 md 文档 4.1 的 SPP demo。
 */

#include "system/includes.h"
#include "app_config.h"
#include "app_msg.h"
#include "btstack/avctp_user.h"
#include "app_ble_spp_api.h"
#include "media_spp.h"

#define LOG_TAG_CONST       BT_BLE
#define LOG_TAG             "[MEDIA_SPP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if MEDIA_SPP_CHANNEL_EN

#define MEDIA_SPP_LOCAL_ID            0x0a        // app_spp local_id, 对应 RFCOMM 扩展通道 A(=10)
#define MEDIA_SPP_RECORD_HANDLE       0x00010041  // SDP record handle, 全局唯一

#ifndef MEDIA_SPP_DATA_LOG_EN
#define MEDIA_SPP_DATA_LOG_EN         0           // 1=打印收发数据(调试), 0=关闭
#endif

static void *media_spp_hdl = NULL;
static media_spp_recv_handler_t g_media_spp_recv_handler = NULL;
static media_spp_state_handler_t g_media_spp_state_handler = NULL;

static void media_spp_recieve_callback(void *hdl, void *remote_addr, u8 *buf, u16 len)
{
#if MEDIA_SPP_DATA_LOG_EN
    printf("media spp recv len=%d\n", len);
    put_buf(buf, len);
#endif
    if (g_media_spp_recv_handler) {
        g_media_spp_recv_handler(hdl, remote_addr, buf, len);
    }
}

static void media_spp_state_callback(void *hdl, void *remote_addr, u8 state)
{
    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("media spp connect#########\n");
        app_spp_set_filter_remote_addr(hdl, remote_addr);
        break;
    case SPP_USER_ST_DISCONN:
        log_info("media spp disconnect#########\n");
#if TCFG_BT_H264_SCR_ENABLE
        {
            int msg[2];
            int err;
            void screen_mirror_bt_h264_deinit(void);
            msg[0] = (int)screen_mirror_bt_h264_deinit;
            msg[1] = 0; /* 回调参数个数 */

            err = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
            if (err) {
                log_error("post h264 deinit failed: %d\n", err);
            }
        }
#endif
        break;
    }
    if (g_media_spp_state_handler) {
        g_media_spp_state_handler(hdl, remote_addr, state);
    }
}

/* SDP 服务记录: record handle + ServiceClassID(FE02...) + RFCOMM ch0x0A + 服务名 JL_MSP */
const u8 media_spp_service_data[96] = {
    0x36, 0x00, 0x5b, 0x09, 0x00, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x41, 0x09, 0x00, 0x01, 0x36, 0x00,
    0x11, 0x1c, 0xfe, 0x02, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5f, 0x9b,
    0x34, 0xfb, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0e, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00, 0x36, 0x00,
    0x05, 0x19, 0x00, 0x03, 0x08, 0x0a, 0x09, 0x00, 0x09, 0x36, 0x00, 0x17, 0x36, 0x00, 0x14, 0x1c,
    0xfe, 0x02, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb,
    0x09, 0x01, 0x00, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4a, 0x4c, 0x5f, 0x4d, 0x53, 0x50, 0x00, 0x00,
};

typedef struct {
    void *offset_item;
    u32 service_record_handle;
    u8 *service_record;
} media_spp_sdp_item_t;
#define MEDIA_SPP_SDP_RECORD_REGISTER(handler) \
    const media_spp_sdp_item_t handler sec(.sdp_record_item)

MEDIA_SPP_SDP_RECORD_REGISTER(media_spp_sdp_record_item) = {
    .service_record = (u8 *)media_spp_service_data,
    .service_record_handle = MEDIA_SPP_RECORD_HANDLE,
};

int media_spp_init(void)
{
    if (media_spp_hdl != NULL) {
        return 0;
    }

    media_spp_hdl = app_spp_hdl_alloc(MEDIA_SPP_LOCAL_ID);
    if (media_spp_hdl == NULL) {
        log_error("media spp hdl alloc err !\n");
        return -1;
    }
    log_info("media spp hdl alloc success, hdl=%08x\n", (u32)media_spp_hdl);

    app_spp_recieve_callback_register(media_spp_hdl, media_spp_recieve_callback);
    app_spp_state_callback_register(media_spp_hdl, media_spp_state_callback);
    app_spp_wakeup_callback_register(media_spp_hdl, NULL);

    return 0;
}

int media_spp_exit(void)
{
    if (media_spp_hdl != NULL) {
        if (app_spp_get_hdl_remote_addr(media_spp_hdl) != NULL) {
            app_spp_disconnect(media_spp_hdl);
        }
        app_spp_hdl_free(media_spp_hdl);
        media_spp_hdl = NULL;
    }
    g_media_spp_recv_handler = NULL;
    g_media_spp_state_handler = NULL;
    return 0;
}

int media_spp_send(u8 *buf, u16 len)
{
    if (media_spp_hdl == NULL || app_spp_get_hdl_remote_addr(media_spp_hdl) == NULL) {
        log_error("media spp not connected, send fail\n");
        return -1;
    }
#if MEDIA_SPP_DATA_LOG_EN
    printf("media spp send len=%d\n", len);
    put_buf(buf, len);
#endif
    return app_spp_data_send(media_spp_hdl, buf, len);
}

u8 media_spp_is_connected(void)
{
    if (media_spp_hdl == NULL) {
        return 0;
    }
    return (app_spp_get_hdl_remote_addr(media_spp_hdl) != NULL) ? 1 : 0;
}

void *media_spp_get_hdl(void)
{
    return media_spp_hdl;
}

int media_spp_recv_handler_register(media_spp_recv_handler_t handler)
{
    g_media_spp_recv_handler = handler;
    return 0;
}

int media_spp_state_handler_register(media_spp_state_handler_t handler)
{
    g_media_spp_state_handler = handler;
    return 0;
}

#else /* MEDIA_SPP_CHANNEL_EN */

int media_spp_init(void)
{
    return -1;
}

int media_spp_exit(void)
{
    return 0;
}

int media_spp_send(u8 *buf, u16 len)
{
    return -1;
}

u8 media_spp_is_connected(void)
{
    return 0;
}

void *media_spp_get_hdl(void)
{
    return NULL;
}

int media_spp_recv_handler_register(media_spp_recv_handler_t handler)
{
    return -1;
}

int media_spp_state_handler_register(media_spp_state_handler_t handler)
{
    return -1;
}

#endif /* MEDIA_SPP_CHANNEL_EN */
