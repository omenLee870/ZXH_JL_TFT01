#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".stream_media_cfg.data.bss")
#pragma data_seg(".stream_media_cfg.data")
#pragma const_seg(".stream_media_cfg.text.const")
#pragma code_seg(".stream_media_cfg.text")
#endif

#include "app_config.h"
#include "stream_media_device_config.h"
#include "JL_rcsp_attr.h"
#include "btstack/third_party/common/app_ble_spp_api.h"

// ============================================================================
// 一、设备基本信息填充
// ============================================================================

void stream_media_cfg_get_device_info(u8 *chip_type, u8 *dev_type,
                                      u16 *proto_ver, u32 *fw_ver)
{
    *chip_type = SM_CFG_CHIP_TYPE;
    *dev_type  = SM_CFG_DEVICE_TYPE;
    *proto_ver = SM_CFG_PROTO_VER;
    *fw_ver    = SM_CFG_FW_VER;
}

// ============================================================================
// 二、设备推流能力填充
// ============================================================================

extern void *rcsp_server_ble_hdl;
extern void *bt_rcsp_spp_hdl;

// 获取当前连接对应的流媒体 MTU：BLE=512, SPP=2048
static u16 get_stream_mtu(void)
{
    u16 con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
    if (con_hdl) {
        return SM_CFG_STREAM_BLE_MTU;     // BLE 通道
    }
    u8 *spp_addr = app_spp_get_hdl_remote_addr(bt_rcsp_spp_hdl);
    if (spp_addr) {
        return SM_CFG_STREAM_SPP_MTU;    // SPP 通道
    }
    return SM_CFG_STREAM_BLE_MTU;         // 默认
}

void stream_media_cfg_get_capability(stream_media_capability_t *cap)
{
    if (!cap) {
        return;
    }
    memset(cap, 0, sizeof(*cap));

    // 推流状态
    cap->available = SM_CFG_AVAILABLE;

    // 分辨率
    cap->width_cnt  = 1;
    cap->width_list[0]  = SM_CFG_DEFAULT_WIDTH;
    cap->height_cnt = 1;
    cap->height_list[0] = SM_CFG_DEFAULT_HEIGHT;

    // 帧率
    cap->fps_cnt    = 1;
    cap->fps_list[0] = SM_CFG_DEFAULT_FPS;

    // 图像质量
    cap->quality_min = SM_CFG_QUALITY_MIN;
    cap->quality_max = SM_CFG_QUALITY_MAX;

    // 推流格式
    cap->format_cnt = 1;
    cap->format_list[0] = SM_CFG_FORMAT;

    // MTU
    cap->stream_mtu = get_stream_mtu();

    // 码率,bps
    cap->bitrate_bps = SM_CFG_STREAM_BITRATE;

    // I/P帧间隔
    cap->ip_interval_frames = SM_CFG_IP_INTERVAL_FRAMES;
}


