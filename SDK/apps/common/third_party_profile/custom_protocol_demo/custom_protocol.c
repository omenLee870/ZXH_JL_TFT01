#include "app_config.h"
#include "app_msg.h"
/* #include "earphone.h" */
#include "bt_tws.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "multi_protocol_main.h"
#include "custom_protocol.h"

#if 1
#define log_info(x, ...)       printf("[CUST-PROT]" x " ", ## __VA_ARGS__)
#define log_info_hexdump       put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif


#if (BT_AI_SEL_PROTOCOL & CUSTOM_DEMO_EN)

extern int bt_modify_name(u8 *new_name);
extern void bt_update_mac_addr(u8 *addr);


void *custom_demo_ble_hdl = NULL;
void *custom_demo_spp_hdl = NULL;

#if GATT_OVER_EDR_DEMO_EN
void *att_over_edr_hdl = NULL;
#define EDR_ATT_HDL_UUID \
	(((u8)('E' + 'D' + 'R') << (1 * 8)) | \
	 ((u8)('A' + 'T' + 'T') << (0 * 8)))
#endif


/*************************************************
                  BLE 相关内容
*************************************************/

const uint8_t custom_demo_profile_data[] = {
    //////////////////////////////////////////////////////
    //
    // 0x0001 PRIMARY_SERVICE  1800
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18,

    /* CHARACTERISTIC,  2a00, READ | WRITE | DYNAMIC, */
    // 0x0002 CHARACTERISTIC 2a00 READ | WRITE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x0a, 0x03, 0x00, 0x00, 0x2a,
    // 0x0003 VALUE 2a00 READ | WRITE | DYNAMIC
    0x08, 0x00, 0x0a, 0x01, 0x03, 0x00, 0x00, 0x2a,

    //////////////////////////////////////////////////////
    //
    // 0x0004 PRIMARY_SERVICE  ae00
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x00, 0xae,

    /* CHARACTERISTIC,  ae01, WRITE_WITHOUT_RESPONSE | DYNAMIC, */
    // 0x0005 CHARACTERISTIC ae01 WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x04, 0x06, 0x00, 0x01, 0xae,
    // 0x0006 VALUE ae01 WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x08, 0x00, 0x04, 0x01, 0x06, 0x00, 0x01, 0xae,

    /* CHARACTERISTIC,  ae02, NOTIFY, */
    // 0x0007 CHARACTERISTIC ae02 NOTIFY
    0x0d, 0x00, 0x02, 0x00, 0x07, 0x00, 0x03, 0x28, 0x10, 0x08, 0x00, 0x02, 0xae,
    // 0x0008 VALUE ae02 NOTIFY
    0x08, 0x00, 0x10, 0x00, 0x08, 0x00, 0x02, 0xae,
    // 0x0009 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x09, 0x00, 0x02, 0x29, 0x00, 0x00,

    // END
    0x00, 0x00,
};

//
// characteristics <--> handles
//
#define ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE 0x0003
#define ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE 0x0006
#define ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE 0x0008
#define ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE 0x0009

static u16 custom_adv_interval_min = 150;
static int s_ble_att_mtu = 0;
static remote_type_e s_connect_remote_type = REMOTE_TYPE_UNKNOWN;   //remote's system type


static void ble_att_check_remote_result(u16 con_handle, remote_type_e remote_type);


static void connection_update_complete_success(u8 *packet)
{
    int con_handle, conn_interval, conn_latency, conn_timeout;

    con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}

static void custom_cbk_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    u16 con_handle;
    // log_info("cbk packet_type:0x%x, packet[0]:0x%x, packet[2]:0x%x", packet_type, packet[0], packet[2]);
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_CAN_SEND_NOW:
            log_info("ATT_EVENT_CAN_SEND_NOW");
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            s_ble_att_mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU:%d", s_ble_att_mtu);
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            con_handle = little_endian_read_16(packet, 3);
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            if (packet[2] == 0) {
                /*ENCRYPTION is ok */
                /*要配对加密后才能检测对方系统类型*/
                att_server_set_check_remote(con_handle, ble_att_check_remote_result);
            }
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = little_endian_read_16(packet, 4);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x", con_handle);
                connection_update_complete_success(packet + 8);
                // reverse_bd_addr(&packet[8], addr);
                put_buf(&packet[8], 6);
                s_connect_remote_type = REMOTE_TYPE_UNKNOWN;
                break;
            case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
                con_handle = little_endian_read_16(packet, 4);
                log_info("L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE %0x", con_handle);
                break;
            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                log_info("HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE\n");
                connection_update_complete_success(packet);
                break;
            default:
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x", packet[5]);
            s_ble_att_mtu = 0;
            s_connect_remote_type = REMOTE_TYPE_UNKNOWN;
            custom_demo_adv_enable(1);
            break;
        default:
            break;
        }
        break;
    }
    return;
}

static uint16_t custom_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;
    log_info("<-------------read_callback, handle= 0x%04x,buffer= %08x", handle, (u32)buffer);
    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        const char *gap_name = bt_get_local_name();
        att_value_len = strlen(gap_name);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &gap_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s", gap_name);
#if 0
            //此处为了方便自动更新连接参数进行测试
            static struct conn_update_param_t param = {25, 32, 16, 600};
            custom_demo_send_request_connect_parameter(&param);
#endif
        }
        break;
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;
    default:
        break;
    }
    log_info("att_value_len= %d", att_value_len);
    return att_value_len;
    return 0;
}

static int custom_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;
    u16 handle = att_handle;
    log_info("<-------------write_callback, handle= 0x%04x,size = %d", handle, buffer_size);
    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;
    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        log_info("rx(%d):\n", buffer_size);
        put_buf(buffer, buffer_size);
        // test
        custom_demo_ble_send(buffer, buffer_size);
        break;
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
        log_info("\nwrite ccc:%04x, %02x\n", handle, buffer[0]);
        multi_att_set_ccc_config(connection_handle, handle, buffer[0]);
        break;
    default:
        break;
    }
    return 0;
}

static u8 custom_fill_adv_data(u8 *adv_data)
{
    u8 offset = 0;
    const char *name_p = bt_get_local_name();
    int name_len = strlen(name_p);
    offset += make_eir_packet_data(&adv_data[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)name_p, name_len);
    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return 0;
    }
    return offset;
}

static u8 custom_fill_rsp_data(u8 *rsp_data)
{
    return 0;
}


int custom_demo_adv_enable(u8 enable)
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    uint8_t advData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t rspData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t len = 0;

    if (enable == app_ble_adv_state_get(custom_demo_ble_hdl)) {
        return 0;
    }
    if (enable) {
        app_ble_set_adv_param(custom_demo_ble_hdl, custom_adv_interval_min, adv_type, adv_channel);
        len = custom_fill_adv_data(advData);
        if (len) {
            app_ble_adv_data_set(custom_demo_ble_hdl, advData, len);
        }
        len = custom_fill_rsp_data(rspData);
        if (len) {
            app_ble_rsp_data_set(custom_demo_ble_hdl, rspData, len);
        }
    }
    app_ble_adv_enable(custom_demo_ble_hdl, enable);
    return 0;
}

int custom_demo_ble_send(u8 *data, u32 len)
{
    int ret = 0;
    int i;
    log_info("custom_demo_ble_send len = %d", len);
    put_buf(data, len);
    ret = app_ble_att_send_data(custom_demo_ble_hdl, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
    if (ret) {
        log_info("send fail\n");
    }
    return ret;
}

#if GATT_OVER_EDR_DEMO_EN
int custom_demo_gatt_over_edr_send(u8 *data, u32 len)
{
    int ret = 0;
    int i;
    printf("custom_demo_ble_send len = %d", len);
    put_buf(data, len);
    ret = app_ble_att_send_data(att_over_edr_hdl, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
    if (ret) {
        printf("send fail\n");
    }
    return ret;
}
#endif

int custom_demo_ble_is_connect(void)
{
    u16 tmp_con_handle = app_ble_get_hdl_con_handle(custom_demo_ble_hdl);
    return (tmp_con_handle) ? TRUE : FALSE;
}

static void ble_att_check_remote_result(u16 con_handle, remote_type_e remote_type)
{
    log_info("con_handle %d:remote_type= %d\n", con_handle, remote_type);
    s_connect_remote_type = remote_type;
}

int custom_demo_ble_disconnect(void)
{
    if (app_ble_get_hdl_con_handle(custom_demo_ble_hdl)) {
        app_ble_disconnect(custom_demo_ble_hdl);
    }
    return 0;
}

int custom_demo_ble_get_att_mtu(void)
{
    return s_ble_att_mtu;
}

void custom_demo_ble_set_adv_interval(u16 adv_interval)
{
    custom_adv_interval_min = adv_interval;
    if (app_ble_adv_state_get(custom_demo_ble_hdl)) {
        custom_demo_adv_enable(0);
        custom_demo_adv_enable(1);
    }
}

void custom_demo_ble_set_name(const char *name)
{
    bt_modify_name((u8 *)name);
    if (app_ble_adv_state_get(custom_demo_ble_hdl)) {
        custom_demo_adv_enable(0);
        custom_demo_adv_enable(1);
    }
}

u8 *custom_demo_ble_get_addr(void)
{
    return app_ble_adv_addr_get(custom_demo_ble_hdl);
}

int custom_demo_ble_get_remote_type(void)
{
    return s_connect_remote_type;
}

int custom_demo_send_request_connect_parameter(struct conn_update_param_t *param)
{
    ble_cmd_ret_e ret;
    u16 tmp_con_handle = app_ble_get_hdl_con_handle(custom_demo_ble_hdl);

    if (param == NULL) {
        return BLE_CMD_PARAM_ERROR;
    }
    if (!tmp_con_handle) {
        return BLE_CMD_STACK_NOT_RUN;
    }
    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    ret = ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, tmp_con_handle, param);
    return ret;
}

/*************************************************
                  BLE 相关内容 end
*************************************************/


/*************************************************
                ancs ams 相关内容
*************************************************/

extern bool gatt_client_check_ios_config(u8 mode);
extern void ancs_ams_set_ios_pair_request_enable(u8 enable);

//ANCS
#if TRANS_ANCS_EN
//profile event
#define ANCS_SUBEVENT_CLIENT_CONNECTED                              0xF0
#define ANCS_SUBEVENT_CLIENT_NOTIFICATION                           0xF1
#define ANCS_SUBEVENT_CLIENT_DISCONNECTED                           0xF2

#define ANCS_MESSAGE_MANAGE_EN                                      1
void ancs_client_init(void);
void ancs_client_exit(void);
void ancs_client_register_callback(btstack_packet_handler_t callback);
const char *ancs_client_attribute_name_for_id(int id);
void ancs_set_notification_buffer(u8 *buffer, u16 buffer_size);
u32 get_notification_uid(void);
u16 get_controlpoint_handle(void);
void ancs_set_out_callback(void *cb);
//ancs info buffer
#define ANCS_INFO_BUFFER_SIZE  (1024)
static u8 ancs_info_buffer[ANCS_INFO_BUFFER_SIZE];
#else
#define ANCS_MESSAGE_MANAGE_EN                                      0
#endif

//ams
#if TRANS_AMS_EN
//profile event
#define AMS_SUBEVENT_CLIENT_CONNECTED                               0xF3
#define AMS_SUBEVENT_CLIENT_NOTIFICATION                            0xF4
#define AMS_SUBEVENT_CLIENT_DISCONNECTED                            0xF5

void ams_client_init(void);
void ams_client_exit(void);
void ams_client_register_callback(btstack_packet_handler_t handler);
const char *ams_get_entity_id_name(u8 entity_id);
const char *ams_get_entity_attribute_name(u8 entity_id, u8 attr_id);
#endif

static void custom_ancs_ams_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
#if TRANS_ANCS_EN
        case HCI_EVENT_ANCS_META:
            switch (hci_event_ancs_meta_get_subevent_code(packet)) {
            case ANCS_SUBEVENT_CLIENT_NOTIFICATION:
                log_info("ANCS_SUBEVENT_CLIENT_NOTIFICATION\n");
                const char *attribute_name;
                attribute_name = ancs_client_attribute_name_for_id(ancs_subevent_client_notification_get_attribute_id(packet));
                if (!attribute_name) {
                    log_info("ancs unknow attribute_id :%d", ancs_subevent_client_notification_get_attribute_id(packet));
                    break;
                } else {
                    u16 attribute_strlen = little_endian_read_16(packet, 7);
                    u8 *attribute_str = (void *)little_endian_read_32(packet, 9);
                    log_info("Notification: %s - %s \n", attribute_name, attribute_str);
                }
                break;

            case ANCS_SUBEVENT_CLIENT_CONNECTED:
                log_info("ANCS_SUBEVENT_CLIENT_CONNECTED\n");
                break;

            case ANCS_SUBEVENT_CLIENT_DISCONNECTED:
                log_info("ANCS_SUBEVENT_CLIENT_DISCONNECTED\n");
                break;

            default:
                break;
            }

            break;
#endif

#if TRANS_AMS_EN
        case HCI_EVENT_AMS_META:
            switch (packet[2]) {
            case AMS_SUBEVENT_CLIENT_NOTIFICATION: {
                log_info("AMS_SUBEVENT_CLIENT_NOTIFICATION\n");
                u16 Entity_Update_len = little_endian_read_16(packet, 7);
                u8 *Entity_Update_data = (void *)little_endian_read_32(packet, 9);
                /* log_info("EntityID:%d, AttributeID:%d, Flags:%d, utf8_len(%d):",\ */
                /* Entity_Update_data[0],Entity_Update_data[1],Entity_Update_data[2],Entity_Update_len-3); */
                log_info("%s(%s), Flags:%d, utf8_len(%d)", ams_get_entity_id_name(Entity_Update_data[0]),
                         ams_get_entity_attribute_name(Entity_Update_data[0], Entity_Update_data[1]),
                         Entity_Update_data[2], Entity_Update_len - 3);

#if 1 //for printf debug
                static u8 music_files_buf[128];
                u8 str_len = Entity_Update_len - 3;
                if (str_len > sizeof(music_files_buf)) {
                    str_len = sizeof(music_files_buf) - 1;
                }
                memcpy(music_files_buf, &Entity_Update_data[3], str_len);
                music_files_buf[str_len] = 0;
                log_info("string:%s\n", music_files_buf);
#endif

                log_info_hexdump(&Entity_Update_data[3], Entity_Update_len - 3);
                /* if (Entity_Update_data[0] == 1 && Entity_Update_data[1] == 2) { */
                /* log_info("for test: send pp_key"); */
                /* ams_send_request_command(AMS_RemoteCommandIDTogglePlayPause); */
                /* } */
            }
            break;

            case AMS_SUBEVENT_CLIENT_CONNECTED:
                log_info("AMS_SUBEVENT_CLIENT_CONNECTED\n");
                break;

            case AMS_SUBEVENT_CLIENT_DISCONNECTED:
                log_info("AMS_SUBEVENT_CLIENT_DISCONNECTED\n");
                break;
            default:
                break;
            }
            break;
#endif
        }
        break;
    }
}

static void ancs_notification_message(u8 *packet, u16 size)
{
#if ANCS_MESSAGE_MANAGE_EN
    u8 *value;
    u32 ancs_notification_uid;
    value = &packet[8];
    ancs_notification_uid = little_endian_read_32(value, 4);
    log_info("Notification: EventID %02x, EventFlags %02x, CategoryID %02x, CategoryCount %u, UID %04x",
             value[0], value[1], value[2], value[3], little_endian_read_32(value, 4));

    if (value[1] & BIT(2)) {
        log_info("is PreExisting Message!!!");
    }

    if (value[0] == 2) { //0:added 1:modifiled 2:removed
        log_info("remove message:ancs_notification_uid %04x", ancs_notification_uid);
        extern void notice_remove_info_from_ancs(u32 uid);
    } else if (value[0] == 0) {
        extern void notice_set_info_from_ancs(void *name, void *data, u16 len);
        log_info("add message:ancs_notification_uid %04x", ancs_notification_uid);
    }
#endif
}

#if TRANS_ANCS_EN
void hangup_ans_call_handle(u8 en)
{

    u32 notification_id;
    u16 control_point_handle;

    log_info("hang_up or answer\n");
    notification_id = get_notification_uid();
    control_point_handle = get_controlpoint_handle();
    u8 ble_hangup[] = {0x02, 0, 0, 0, 0, en};
    memcpy(&ble_hangup[1], &notification_id, 4);
    log_info_hexdump(ble_hangup, 6);
    u8 ble_hangup_size = 6;
    ble_op_att_send_data(control_point_handle, (void *)&ble_hangup, ble_hangup_size, ATT_OP_WRITE);

}
#endif

//底层调用
void ancs_update_status(u8 status)
{
    switch (status) {
    case 1:
        log_info("ancs trunk start \n");
        break;
    case 2:
        log_info("ancs trunk end \n");
        break;
    default:
        break;
    }
}

//底层调用
void ble_profile_init(void)
{
#if TRANS_ANCS_EN || TRANS_AMS_EN
    gatt_client_check_ios_config(0);
#endif
}

//底层调用
/* ancs等待加密接口*/
void ancs_client_wait_request_pairing(u16 con_handle)
{
    u16 ble_con_handle = app_ble_get_hdl_con_handle(custom_demo_ble_hdl);
    log_info("%s con_handle:%d ble_con_handle:%d", __func__, con_handle, ble_con_handle);
    if (ble_con_handle == con_handle) {
        sm_api_request_pairing(con_handle);
    }
}

/*************************************************
              ancs ams 相关内容 end
*************************************************/


/*************************************************
                  SPP 相关内容
*************************************************/
static void custom_spp_state_callback(void *hdl, void *remote_addr, u8 state)
{
    int i;
    int bond_flag = 0;
    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("custom spp connect#########\n");
        // 将 custom_demo_spp_hdl 绑定到连接上的设备地址，否则后续会收到所有已连接设备地址的事件和数据
        app_spp_set_filter_remote_addr(custom_demo_spp_hdl, remote_addr);
        break;
    case SPP_USER_ST_DISCONN:
        log_info("custom spp disconnect#########\n");
        break;
    };
}

static void custom_spp_recieve_callback(void *hdl, void *remote_addr, u8 *buf, u16 len)
{
    log_info("custom_spp_recieve_callback len=%d\n", len);
    put_buf(buf, len);

    // test send
    custom_demo_spp_send(buf, len);
}

int custom_demo_spp_send(u8 *data, u32 len)
{
    return app_spp_data_send(custom_demo_spp_hdl, data, len);
}

/*************************************************
                  SPP 相关内容 end
*************************************************/

void custom_demo_all_init(void)
{
    log_info("custom_demo_all_init\n");
    const uint8_t *edr_addr = bt_get_mac_addr();
    log_info("edr addr:");
    put_buf((uint8_t *)edr_addr, 6);

    // BLE init
    if (custom_demo_ble_hdl == NULL) {
        custom_demo_ble_hdl = app_ble_hdl_alloc();
        if (custom_demo_ble_hdl == NULL) {
            log_info("custom_demo_ble_hdl alloc err !\n");
            return;
        }
        app_ble_set_mac_addr(custom_demo_ble_hdl, (void *)edr_addr);
        app_ble_profile_set(custom_demo_ble_hdl, custom_demo_profile_data);
        app_ble_att_read_callback_register(custom_demo_ble_hdl, custom_att_read_callback);
        app_ble_att_write_callback_register(custom_demo_ble_hdl, custom_att_write_callback);
        app_ble_att_server_packet_handler_register(custom_demo_ble_hdl, custom_cbk_packet_handler);
        app_ble_hci_event_callback_register(custom_demo_ble_hdl, custom_cbk_packet_handler);
        app_ble_l2cap_packet_handler_register(custom_demo_ble_hdl, custom_cbk_packet_handler);

        custom_demo_adv_enable(1);
    }
    // BLE init end

    // SPP init
    if (custom_demo_spp_hdl == NULL) {
        custom_demo_spp_hdl = app_spp_hdl_alloc(0x0);
        if (custom_demo_spp_hdl == NULL) {
            log_info("custom_demo_spp_hdl alloc err !\n");
            return;
        }
        app_spp_recieve_callback_register(custom_demo_spp_hdl, custom_spp_recieve_callback);
        app_spp_state_callback_register(custom_demo_spp_hdl, custom_spp_state_callback);
        app_spp_wakeup_callback_register(custom_demo_spp_hdl, NULL);
    }
    // SPP init end

#if GATT_OVER_EDR_DEMO_EN
    // att_over_edr init
    if (att_over_edr_hdl == NULL) {
        att_over_edr_hdl = app_ble_hdl_alloc();
        if (att_over_edr_hdl == NULL) {
            printf("att_over_edr_hdl alloc err !\n");
            return;
        }
        app_ble_profile_set(att_over_edr_hdl, custom_demo_profile_data);
        app_ble_adv_address_type_set(att_over_edr_hdl, 0);
        app_ble_gatt_over_edr_connect_type_set(att_over_edr_hdl, 1);
        app_ble_hdl_uuid_set(att_over_edr_hdl, EDR_ATT_HDL_UUID);
        app_ble_att_read_callback_register(att_over_edr_hdl, custom_att_read_callback);
        app_ble_att_write_callback_register(att_over_edr_hdl, custom_att_write_callback);
        app_ble_att_server_packet_handler_register(att_over_edr_hdl, custom_cbk_packet_handler);
        app_ble_hci_event_callback_register(att_over_edr_hdl, custom_cbk_packet_handler);
        app_ble_l2cap_packet_handler_register(att_over_edr_hdl, custom_cbk_packet_handler);
    }
#endif


#if TRANS_ANCS_EN || TRANS_AMS_EN
    if ((!config_le_sm_support_enable) || (!config_le_gatt_client_num)) {
        log_info("ANCS need sm and client support!!!\n");
        ASSERT(0);
    }

    if (config_le_gatt_client_num) {
        //setup GATT client
        gatt_client_init();
    }

#endif

#if TRANS_ANCS_EN
    log_info("ANCS init...");
    //setup ANCS clent
    ancs_client_init();
    ancs_set_notification_buffer(ancs_info_buffer, sizeof(ancs_info_buffer));
    ancs_client_register_callback(&custom_ancs_ams_cbk_packet_handler);
    ancs_set_out_callback(ancs_notification_message);
#endif

#if TRANS_AMS_EN
    log_info("AMS init...");
    ams_client_init();
    ams_client_register_callback(&custom_ancs_ams_cbk_packet_handler);
    ams_entity_attribute_config(AMS_IDPlayer_ENABLE | AMS_IDQueue_ENABLE | AMS_IDTrack_ENABLE);
    /* ams_entity_attribute_config(AMS_IDTrack_ENABLE); */
#endif

#if TRANS_ANCS_EN || TRANS_AMS_EN || TCFG_BLE_BRIDGE_EDR_ENALBE
    ancs_ams_set_ios_pair_request_enable(0);
    gatt_client_check_ios_config(1);
    app_ble_set_ancs_connection_flag(custom_demo_ble_hdl, 1);
#if GATT_OVER_EDR_DEMO_EN
    app_ble_set_ancs_connection_flag(att_over_edr_hdl, 1);
#endif
#endif

#if !TRANS_ANCS_EN && !TRANS_AMS_EN && TCFG_BLE_BRIDGE_EDR_ENALBE
    ancs_ams_set_ios_pair_request_enable(1);
    gatt_client_check_ios_config(2);
#endif
}

void custom_demo_all_exit(void)
{
    log_info("custom_demo_all_exit\n");

    // BLE exit
    if (app_ble_get_hdl_con_handle(custom_demo_ble_hdl)) {
        app_ble_disconnect(custom_demo_ble_hdl);
    }
    app_ble_hdl_free(custom_demo_ble_hdl);
    custom_demo_ble_hdl = NULL;

    // SPP init
    if (NULL != app_spp_get_hdl_remote_addr(custom_demo_spp_hdl)) {
        app_spp_disconnect(custom_demo_spp_hdl);
    }
    app_spp_hdl_free(custom_demo_spp_hdl);
    custom_demo_spp_hdl = NULL;
#if GATT_OVER_EDR_DEMO_EN
    // gatt over edr exit
    if (app_ble_get_hdl_con_handle(att_over_edr_hdl)) {
        app_ble_disconnect(att_over_edr_hdl);
    }
    app_ble_hdl_free(att_over_edr_hdl);
    att_over_edr_hdl = NULL;
#endif

}

void custom_demo_edr_set_name(u8 *name)
{
    if (name == NULL) {
        return;
    }
    bt_modify_name(name);
    if (!bt_get_total_connect_dev()) {
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);

        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    }
}

void custom_demo_edr_set_addr(u8 *addr, u8 len)
{
    if (!((addr) && (len == 6))) {
        return;
    }
    bt_update_mac_addr(addr);
    lmp_hci_write_local_address(addr);
    if (!bt_get_total_connect_dev()) {
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);

        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    }
}

#endif

