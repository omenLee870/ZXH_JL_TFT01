#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "app_config.h"
#include "rcsp_hid_inter.h"
#include "usb/device/custom_hid.h"
#define LOG_TAG_CONST       USB
#define LOG_TAG             "[RCSP_HID]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

static void (*rcsp_hid_recieve_callback)(void *priv, void *buf, u16 len) = NULL;

void rcsp_hid_recieve(void *priv, void *buf, u16 len)
{
    if (rcsp_hid_recieve_callback) {
        rcsp_hid_recieve_callback(priv, buf, len);
    }
}

bool JL_rcsp_hid_fw_ready(void *priv)
{
    return custom_hid_get_ready(0) ? true : false;
}

static int update_send_user_data_do(void *priv, void *data, u16 len)
{
    //-------------------!!!!!!!!!!考虑关闭RCSP_BTMATE_EN使能编译报错
    extern void dongle_send_data_to_pc_3(u8 * data, u16 len);
    dongle_send_data_to_pc_3(data, len);
    return 0;
}

static int update_regiest_recieve_cbk(void *priv, void *cbk)
{
    printf("update_regiest_recieve_cbk, %x\n", (u32)cbk);
    rcsp_hid_recieve_callback = cbk;
    return 0;
}

static int update_regiest_state_cbk(void *priv, void *cbk)
{
    return 0;
}

static const struct rcsp_hid_operation_t rcsp_hid_update_operation = {
    .send_data = update_send_user_data_do,
    .regist_recieve_cbk = update_regiest_recieve_cbk,
    .regist_state_cbk = update_regiest_state_cbk,
};

void rcsp_hid_get_operation_table(struct rcsp_hid_operation_t **interface_pt)
{
    *interface_pt = (void *)&rcsp_hid_update_operation;
}

