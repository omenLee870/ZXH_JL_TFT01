#include "system/includes.h"
#include "app_config.h"
#include "app_msg.h"
#include "btcontroller_config.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "btstack/le/ble_api.h"
#include "btstack/le/att.h"
#include "btstack/avctp_user.h"
#include "multi_protocol_main.h"


#if WECHAT_SPORT_ENABLE
#define log_info(x, ...)  printf("[wechat_sport]" x " ", ## __VA_ARGS__)
extern const u8 adt_profile_support;

const u8 sdp_att_service_data[60] = {
    0x36, 0x00, 0x31, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x21, 0x09, 0x00, 0x01, 0x35, 0x03,
    0x19, 0xfe, 0xe7, 0x09, 0x00, 0x04, 0x35, 0x13, 0x35, 0x06, 0x19, 0x01, 0x00, 0x09, 0x00, 0x1F,
    0x35, 0x09, 0x19, 0x00, 0x07, 0x09, 0x00, 0x01, 0x09, 0x00, 0x13, 0x09, 0x00, 0x05, 0x35, 0x03,
    0x19, 0x10, 0x02, 0x00
};

typedef struct {
    // linked list - assert: first field
    void *offset_item;
    // data is contained in same memory
    u32        service_record_handle;
    u8         *service_record;
} service_record_item_t;
#define SDP_RECORD_HANDLER_REGISTER(handler) \
    const service_record_item_t  handler \
    sec(.sdp_record_item)
SDP_RECORD_HANDLER_REGISTER(spp_att_record_item) = {
    .service_record = (u8 *)sdp_att_service_data,
    .service_record_handle = 0x00010021,
};

#ifndef EDR_ATT_HDL_UUID
#define EDR_ATT_HDL_UUID \
	(((u8)('E' + 'D' + 'R') << (1 * 8)) | \
	 ((u8)('A' + 'T' + 'T') << (0 * 8)))
#endif//EDR_ATT_HDL_UUID
void *wechat_sport_hdl = NULL;
u8 wechat_data[] = {0x01, 0x1f, 0x10, 0x00}; //1-3步数  4-6距离  7-9卡路里

#define ATT_CHARACTERISTIC_fec9_01_VALUE_HANDLE 0x000c
#define ATT_CHARACTERISTIC_fea1_01_VALUE_HANDLE 0x000e
#define ATT_CHARACTERISTIC_fea1_01_CLIENT_CONFIGURATION_HANDLE 0x000f

extern void *rcsp_server_ble_hdl;
extern void wechat_send_data(void *pive);
extern void *app_ble_hdl_alloc(void);
extern ble_cmd_ret_e app_ble_att_send_data(void *_hdl, u16 att_handle, u8 *data, u16 len, att_op_type_e att_op_type);
extern u16 app_ble_att_get_ccc_config(void *_hdl, u16 att_handle);
extern int app_ble_att_connect_type_set(void *_hdl, u8 connect_type);
extern void bredr_adt_init();

static uint16_t att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;
    log_info("%s handle = 0x%x\n", __func__, handle);
    switch (handle) {
    case ATT_CHARACTERISTIC_fea1_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = multi_att_get_ccc_config(connection_handle, handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;
    case ATT_CHARACTERISTIC_fea1_01_VALUE_HANDLE:
        if (buffer) {
            memcpy(buffer, wechat_data, 10);
        }
        att_value_len = 10;
        break;
    case ATT_CHARACTERISTIC_fec9_01_VALUE_HANDLE:
        att_value_len = 6;
        u8 mac_addr_buf[6] ;
        swapX(bt_get_mac_addr(), mac_addr_buf, 6);
        if (buffer) {
            memcpy(buffer, mac_addr_buf, 6);
            put_buf(buffer, 6);
        }
        break;
    }
    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;
}

uint16_t bt_wechat_sport_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    return att_read_callback(hdl, connection_handle, att_handle, offset, buffer, buffer_size);
}

static int att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    u16 handle = att_handle;
    log_info("%s handle = 0x%x\n", __func__, handle);

    switch (handle) {
    case ATT_CHARACTERISTIC_fea1_01_CLIENT_CONFIGURATION_HANDLE:
        multi_att_set_ccc_config(connection_handle, handle, buffer[0]);
        break;
    }
    return 0;
}

void bt_wechat_sport_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    att_write_callback(hdl, connection_handle, att_handle, transaction_mode, offset, buffer, buffer_size);
}

static void cbk_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{

}

void wechat_send_data(void *pive)
{
    if (app_ble_att_get_ccc_config(wechat_sport_hdl, ATT_CHARACTERISTIC_fea1_01_VALUE_HANDLE)) {
        app_ble_att_send_data(wechat_sport_hdl, ATT_CHARACTERISTIC_fea1_01_VALUE_HANDLE, wechat_data, 4, ATT_OP_AUTO_READ_CCC);
    }
    if (app_ble_att_get_ccc_config(rcsp_server_ble_hdl, ATT_CHARACTERISTIC_fea1_01_VALUE_HANDLE)) {
        app_ble_att_send_data(rcsp_server_ble_hdl, ATT_CHARACTERISTIC_fea1_01_VALUE_HANDLE, wechat_data, 4, ATT_OP_AUTO_READ_CCC);
    }
}

void wechat_sport_init()
{
    if (adt_profile_support) {
        if (wechat_sport_hdl == NULL) {
            wechat_sport_hdl = app_ble_hdl_alloc();
            if (wechat_sport_hdl == NULL) {
                ASSERT(0, "wechat_sport_hdl alloc err !!\n");
                return;
            }
        }
        app_ble_adv_address_type_set(wechat_sport_hdl, 0);
        app_ble_gatt_over_edr_connect_type_set(wechat_sport_hdl, 1);
        app_ble_hdl_uuid_set(wechat_sport_hdl, EDR_ATT_HDL_UUID);
        app_ble_profile_set(wechat_sport_hdl, rcsp_profile_data);
        app_ble_att_read_callback_register(wechat_sport_hdl, att_read_callback);
        app_ble_att_write_callback_register(wechat_sport_hdl, att_write_callback);
        app_ble_att_server_packet_handler_register(wechat_sport_hdl, cbk_packet_handler);
        app_ble_hci_event_callback_register(wechat_sport_hdl, cbk_packet_handler);
        app_ble_l2cap_packet_handler_register(wechat_sport_hdl, cbk_packet_handler);
    }


}


void att_profile_init()
{
    bredr_adt_init();
}

static int edr_att_btstack_event_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        att_profile_init();
        break;
    }
    return 0;
}
APP_MSG_HANDLER(edr_att_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = edr_att_btstack_event_handler,
};

#else

uint16_t bt_wechat_sport_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    return 0;
}
void bt_wechat_sport_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
}
#endif

