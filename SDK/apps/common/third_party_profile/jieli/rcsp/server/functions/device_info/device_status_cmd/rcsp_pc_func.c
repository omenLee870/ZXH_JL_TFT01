#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_pc_func.data.bss")
#pragma data_seg(".rcsp_pc_func.data")
#pragma const_seg(".rcsp_pc_func.text.const")
#pragma code_seg(".rcsp_pc_func.text")
#endif
#include "rcsp_pc_func.h"
#include "rcsp_device_info_func_common.h"
#include "rcsp_device_status.h"
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "JL_rcsp_attr.h"

#if RCSP_MODE && TCFG_APP_PC_EN
#include "pc.h"

//设置固件pc从机行为
bool rcsp_pc_func_set(void *priv, u8 *data, u16 len)
{
    // 在线状态(Bit2)不允许设置；播放状态/上下曲由 App 下发
    return true;
}

//获取固件pc从机信息
u32 rcsp_pc_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset = 0;
    if (mask & BIT(PC_INFO_ATTR_PLAY_STATUS)) {
        // PC 从机仅向主机发送媒体按键, 固件不跟踪主机播放状态, 默认上报暂停
        u8 play_status = 0;
        offset = add_one_attr(buf, buf_size, offset, PC_INFO_ATTR_PLAY_STATUS, (u8 *)&play_status, sizeof(play_status));
    }
    if (mask & BIT(PC_INFO_ATTR_PREV_NEXT)) {
        u8 prev_next = 0;
        offset = add_one_attr(buf, buf_size, offset, PC_INFO_ATTR_PREV_NEXT, (u8 *)&prev_next, sizeof(prev_next));
    }
    if (mask & BIT(PC_INFO_ATTR_ONLINE_STATUS)) {
        u8 online = pc_get_online_status();
        offset = add_one_attr(buf, buf_size, offset, PC_INFO_ATTR_ONLINE_STATUS, (u8 *)&online, sizeof(online));
    }
    return offset;
}

#endif
