#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_adv_bluetooth.data.bss")
#pragma data_seg(".rcsp_adv_bluetooth.data")
#pragma const_seg(".rcsp_adv_bluetooth.text.const")
#pragma code_seg(".rcsp_adv_bluetooth.text")
#endif
#include "rcsp_config.h"
#include "ble_rcsp_server.h"
#include "rcsp_adv_bluetooth.h"
#include "rcsp_setting_opt.h"
#include "rcsp_setting_sync.h"
#include "custom_cfg.h"
#include "btstack/avctp_user.h"
#include "rcsp_manage.h"
#include "rcsp_music_info_setting.h"
#include "rcsp_bt_manage.h"
#include "ble_rcsp_adv.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_protocol.h"
#include "JL_rcsp_attr.h"


#if RCSP_MODE && (!RCSP_ADV_EN)
#if (defined TCFG_CSC_BT_APP) && TCFG_CSC_BT_APP

#define RCSP_DEBUG_EN
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif


static u8 adv_setting_result = 0;

static u8 adv_set_deal_one_attr(u8 *buf, u8 size, u8 offset)
{
    u8 rlen = buf[offset];
    if ((offset + rlen + 1) > (size - offset)) {
        rcsp_printf("\n\ndeal attr end!\n\n");
        return rlen;
    }
    u8 type = buf[offset + 1];
    u8 *pbuf = &buf[offset + 2];
    u8 dlen = rlen - 1;
    adv_setting_result = 0;

    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(type);
    if (setting_opt_hdl) {
        adv_setting_result = set_setting_extra_handle(setting_opt_hdl, pbuf, &dlen);
    }
    if (!adv_setting_result) {
        set_rcsp_opt_setting(setting_opt_hdl, NULL);
    }

    return rlen + 1;
}


static u32 JL_opcode_set_adv_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    rcsp_printf("JL_opcode_set_adv_info:\n");
    rcsp_printf_buf(data, len);
    u8 offset = 0;
    while (offset < len) {
        offset += adv_set_deal_one_attr(data, len, offset);
    }
    u8 ret = 0;
    if (adv_setting_result) {
        ret = adv_setting_result;
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &ret, 1, ble_con_handle, spp_remote_addr);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &ret, 1, ble_con_handle, spp_remote_addr);
    }
#if TCFG_RCSP_DUAL_CONN_ENABLE
    // 一拖二则需要手机重新拉取C0设置的设备信息
    u8 adv_cmd = 0x0;
    adv_info_device_request(&adv_cmd, sizeof(adv_cmd));
#endif
    return 0;
}


static u8 rcsp_adv_get_and_fill_adv_info(u8 *buf, u16 max_len, u8 offset, u8 type)
{
    u8 *adv_data = NULL;
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(type);
    u8 size = get_setting_extra_handle(setting_opt_hdl, &adv_data, NULL);
    if (size && adv_data) {
        size = add_one_attr(buf, max_len, offset, type, (void *)adv_data, size);
    }
    return size;
}

static u32 JL_opcode_get_adv_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u8 buf[256] = {0};
    u8 offset = 0;

    u32 ret = 0;
    u32 mask = READ_BIG_U32(data);
    rcsp_printf("FEATURE MASK : %x\n", mask);
    /* #define ATTR_TYPE_BAT_VALUE      (0) */
    /* #define ATTR_TYPE_EDR_NAME       (1) */
    //get version
    if (mask & BIT(ATTR_TYPE_BAT_VALUE)) {
        rcsp_printf("ATTR_TYPE_BAT_VALUE\n");
        u8 bat[3];
        void bt_adv_get_bat(u8 * buf);
        bt_adv_get_bat(bat);
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_BAT_VALUE, bat, 3);
    }

#if RCSP_ADV_NAME_SET_ENABLE
    if (mask & BIT(ATTR_TYPE_EDR_NAME)) {
        rcsp_printf("ATTR_TYPE_EDR_NAME\n");
        offset += rcsp_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_EDR_NAME);
    }
#endif // RCSP_ADV_NAME_SET_ENABLE

#if RCSP_ADV_KEY_SET_ENABLE
    if (mask & BIT(ATTR_TYPE_KEY_SETTING)) {
        rcsp_printf("ATTR_TYPE_KEY_SETTING\n");
        offset += rcsp_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_KEY_SETTING);
    }
    if (mask & BIT(ATTR_TYPE_ANC_VOICE_KEY)) {
        rcsp_printf("ATTR_TYPE_ANC_VOICE_KEY\n");
        offset += rcsp_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_ANC_VOICE_KEY);
    }
#endif // RCSP_ADV_KEY_SET_ENABLE

#if RCSP_ADV_LED_SET_ENABLE
    if (mask & BIT(ATTR_TYPE_LED_SETTING)) {
        rcsp_printf("ATTR_TYPE_LED_SETTING\n");
        offset += rcsp_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_LED_SETTING);
    }
#endif // RCSP_ADV_LED_SET_ENABLE

#if RCSP_ADV_MIC_SET_ENABLE
    if (mask & BIT(ATTR_TYPE_MIC_SETTING)) {
        rcsp_printf("ATTR_TYPE_MIC_SETTING\n");
        offset += rcsp_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_MIC_SETTING);
    }
#endif // RCSP_ADV_MIC_SET_ENABLE

#if RCSP_ADV_WORK_SET_ENABLE
    if (mask & BIT(ATTR_TYPE_WORK_MODE)) {
        rcsp_printf("ATTR_TYPE_WORK_MODE\n");
        offset += rcsp_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_WORK_MODE);
    }
#endif // RCSP_ADV_WORK_SET_ENABLE

#if RCSP_ADV_PRODUCT_MSG_ENABLE
    if (mask & BIT(ATTR_TYPE_PRODUCT_MESSAGE)) {
        rcsp_printf("ATTR_TYPE_PRODUCT_MESSAGE\n");
        u16 vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
        u16 pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
        u8 tversion[6];
        tversion[0] = 0x05;
        tversion[1] = 0xD6;
        tversion[3] = vid & 0xFF;
        tversion[2] = vid >> 8;
        tversion[5] = pid & 0xFF;
        tversion[4] = pid >> 8;
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_PRODUCT_MESSAGE, (void *)tversion, 6);
    }
#endif // RCSP_ADV_PRODUCT_MSG_ENABLE

    rcsp_printf_buf(buf, offset);

    ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, buf, offset, ble_con_handle, spp_remote_addr);

    return ret;
}




int JL_rcsp_adv_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    rcsp_printf("JL_rcsp_adv_cmd_resp\n");
    switch (OpCode) {
    case JL_OPCODE_SET_ADV:
        rcsp_printf(" JL_OPCODE_SET_ADV\n");
        JL_opcode_set_adv_info(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_GET_ADV:
        rcsp_printf(" JL_OPCODE_GET_ADV\n");
        JL_opcode_get_adv_info(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    default:
        return 1;
    }
    return 0;
}


#endif

#endif
