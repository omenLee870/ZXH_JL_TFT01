#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_rcsp_server.data.bss")
#pragma data_seg(".ble_rcsp_server.data")
#pragma const_seg(".ble_rcsp_server.text.const")
#pragma code_seg(".ble_rcsp_server.text")
#endif
/*********************************************************************************************
    *   Filename        : ble_rcsp_module.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2022-06-08 11:14

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/

// *****************************************************************************
// *****************************************************************************
#include "system/includes.h"
#include "app_config.h"
#include "rcsp_config.h"
#include "app_action.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "app_power_manage.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "app_chargestore.h"

#include "btcrypt.h"
#include "custom_cfg.h"
#include "rcsp_music_info_setting.h"
#include "ble_rcsp_adv.h"
#include "ble_rcsp_server.h"
#include "classic/tws_api.h"
#include "rcsp_update.h"
#include "ble_rcsp_multi_common.h"
#include "btstack/btstack_event.h"
#include "multi_protocol_main.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_protocol.h"
#include "adv_1t2_setting.h"
#include "ble_fmy_fmna.h"
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif
#include "app_ble_spp_api.h"
#include "btstack_rcsp_user.h"
#include "le_alipay.h"

#include "adv_hearing_aid_setting.h"

#if TCFG_UI_ENABLE
#include "ui/lcd/lcd_drive.h"

#if CONFIG_JL_UI_ENABLE
#include "jlui_app/ui_sys_param.h"
#endif
#endif

#if ( BT_AI_SEL_PROTOCOL & RCSP_MODE_EN )

#if 1
#define log_info(x, ...)       printf("[LE-RCSP]" x " ", ## __VA_ARGS__)
#define log_info_hexdump       put_buf
#define log_error(x, ...)       printf("<error>[LE-SMART]" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#define log_info_hexdump(...)
#define log_error(...)
#endif

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};


//------
#define ATT_LOCAL_PAYLOAD_SIZE    (517)//(247)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512*2)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));

//---------------
//---------------
#define ADV_INTERVAL_MIN          (160*5) //n*0.625ms
/* #define ADV_INTERVAL_MIN          (0x30) */

#if (TCFG_BLE_ADV_DYNAMIC_SWITCH && TCFG_UI_ENABLE)
#define ADV_INTERVAL_QUICK          (160*1)
#define ADV_INTERVAL_NORMAL         (160*5)
#define ADV_INTERVAL_SLOW           (160*10)
#undef ADV_INTERVAL_MIN
#define ADV_INTERVAL_MIN            ADV_INTERVAL_QUICK // ADV_INTERVAL_NORMAL
void rcsp_bt_ble_adv_interval_reset(void);
#endif /* #if (TCFG_BLE_ADV_DYNAMIC_SWITCH && TCFG_UI_ENABLE) */


//加密设置
static const uint8_t sm_min_key_size = 7;

//连接参数设置
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
static uint8_t connection_update_cnt = 0; //
static const struct conn_update_param_t connection_param_table[] = {
    {25, 32, 16, 600},
    {12, 28, 14, 600},//11
    {8,  20, 20, 600},//3.7
    /* {12, 28, 4, 600},//3.7 */
    /* {12, 24, 30, 600},//3.05 */
};

static const struct conn_update_param_t connection_param_table_update[] = {
    {96, 120, 0,  600},
    {60,  80, 0,  600},
    {60,  80, 0,  600},
    /* {8,   20, 0,  600}, */
    {6, 12, 0, 400},/*ios 提速*/
};

static u8 conn_update_param_index = 0;
#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))
#define CONN_TRY_NUM			  10 // 重复尝试次数

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif


static char *gap_device_name = "JL_ble_test";
static u8 gap_device_name_len = 0;
volatile static u8 ble_work_state = 0;
static u8 adv_ctrl_en;
static u8 ble_init_flag;

static u8 cur_peer_addr_info[7]; /*当前连接对方地址信息*/
static u8 check_rcsp_auth_flag; /*检查认证是否通过,ios发起回连edr*/
static u8 pair_reconnect_flag;  /*配对回连标记*/

u8 alipay_adv_en;/*upay绑定模式*/

static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

static u16 set_adv_interval;
static volatile u8 adv_enable;//广播开关

static ble_state_e rcsp_get_ble_work_state(void);

extern u8 get_rcsp_connect_status(void);
extern const int config_btctler_le_hw_nums;

/* static const char *const phy_result[] = { */
/*     "None", */
/*     "1M", */
/*     "2M", */
/*     "Coded" */
/* }; */
//------------------------------------------------------
//ANCS
#if TRANS_ANCS_EN
//profile event
#define ANCS_SUBEVENT_CLIENT_CONNECTED                              0xF0
#define ANCS_SUBEVENT_CLIENT_NOTIFICATION                           0xF1
#define ANCS_SUBEVENT_CLIENT_DISCONNECTED                           0xF2

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
#if TCFG_FINDMY_ENABLE
u8 fmy_owner_connect = false;
#endif
extern uint16_t bt_wechat_sport_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
extern void bt_wechat_sport_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
extern const u8 adt_profile_support;
extern u8 rcsp_adt_support; //edr att是否接入rcsp

//------------------------------------------------------

//------------------------------------------------------
//广播参数设置
static void advertisements_setup_init();
extern const char *bt_get_local_name();
static int get_buffer_vaild_len(void *priv);
//------------------------------------------------------
#if CONFIG_USE_RANDOM_ADDRESS_ENABLE
typedef enum {
    RANDOM_ADDRESS_TYPE_OFF = 0,
    RANDOM_ADDRESS_TYPE_STATIC,
    RANDOM_ADDRESS_NON_RESOLVABLE,
    RANDOM_ADDRESS_RESOLVABLE,
} random_address_type_t;
extern bool ble_set_make_random_address(uint8_t random_type);

static const u8 slave_address_random_type = RANDOM_ADDRESS_RESOLVABLE;
static const u16 slave_random_period_seconds = (15 * 60); //15 mins
static u16 slave_random_period_timer_id;
static u8 le_smart_random_address[6];

static void slave_random_make_random_address(void)
{
    if (ble_set_make_random_address(slave_address_random_type)) {
        le_controller_get_random_mac(le_smart_random_address);
        log_info("---ble's random generate address:");
        put_buf(le_smart_random_address, 6);
    } else {
        log_error("make random address fail!!!");
        ASSERT(0);
    }
}

static void slave_random_period_timer_handler(void *priev)
{
    //rotate address when in adv state
    if (app_ble_adv_state_get(rcsp_server_ble_hdl)) {
        log_info("period to change addr");
        rcsp_bt_ble_adv_enable(0);
        slave_random_make_random_address();
        rcsp_bt_ble_adv_enable(1);
    } else {
        log_info("period to change addr");
        slave_random_make_random_address();
    }
}
#endif

#if TCFG_FINDMY_ENABLE
extern int bt_modify_name(u8 *new_name);
void fmy_other_device_ble_name_set(u8 *name)
{
    log_info("fmy change smartbox ble adv name");
    bt_modify_name(name);
    // open close adv when in adv state to make difference
    if (app_ble_adv_state_get(rcsp_server_ble_hdl)) {
        rcsp_bt_ble_adv_enable(0);
        rcsp_bt_ble_adv_enable(1);
    }
}

static u8 le_smart_check_is_fmy_owner(u8 addr_type, u8 *addr)
{
    u8 temp_addr[6];
    u8 le_smart_cur_public_addr[6];

    // Swap the address bytes
    swapX(addr, temp_addr, 6);

    // Check if the owner pair is set
    if (fmy_is_owner_pair_set()) {
        // Get the current public address from the list
        if (ble_list_get_id_addr(temp_addr, addr_type, le_smart_cur_public_addr)) {
            log_info("le smartbox cur public address:");
            log_info_hexdump(le_smart_cur_public_addr, 6);

            // Check if the current address is the owner's address
            if (fmy_is_owner_address(le_smart_cur_public_addr)) {
                log_info("fmy owner connect");
                fmy_original_name_set();
                return true;
            } else {
                log_info("non fmy owner connect");
            }
        } else {
            log_info("can not get pub mac");
        }
        fmy_suffix_name_set();
    } else {
        log_info("fmy no pair, not need to change name");
    }
    return false;
    /* return 0; */
}
#endif

static void send_request_connect_parameter(hci_con_handle_t connection_handle, u8 table_index)
{
    struct conn_update_param_t *param = NULL; //static ram
    switch (conn_update_param_index) {
    case 0:
        param = (void *)&connection_param_table[table_index];
        break;
    case 1:
        param = (void *)&connection_param_table_update[table_index];
        break;
    default:
        break;
    }

    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (connection_handle) {
        ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, connection_handle, param);
    }
}

static void check_connetion_updata_deal(hci_con_handle_t connection_handle)
{
    //cppcheck-suppress knownConditionTrueFalse
#if (BT_AI_SEL_PROTOCOL & LE_AUDIO_CIS_RX_EN)
    extern u8 le_audio_get_adv_conn_success();
    //le audio 连接之后不允许其他位置更新参数
    if (connection_update_enable && le_audio_get_adv_conn_success()) {
#else
    if (connection_update_enable) {
#endif
        if (connection_update_cnt < (CONN_PARAM_TABLE_CNT * CONN_TRY_NUM)) {
            send_request_connect_parameter(connection_handle, connection_update_cnt / CONN_TRY_NUM);
        }
    }
}

void notify_update_connect_parameter(u8 table_index)
{
    u16 temp_con_handle;
    u8 conn_update_param_index_record = conn_update_param_index;

    if (app_ble_get_hdl_con_handle(rcsp_server_ble_hdl)) {
        temp_con_handle = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
    } else if (app_ble_get_hdl_con_handle(rcsp_server_ble_hdl)) {
        temp_con_handle = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
    } else {
        return;
    }

    if ((u8) - 1 != table_index) {
        conn_update_param_index = 1;
        send_request_connect_parameter(temp_con_handle, table_index);
    } else {
        if (connection_update_cnt >= (CONN_PARAM_TABLE_CNT * CONN_TRY_NUM)) {
            log_info("connection_update_cnt >= CONN_PARAM_TABLE_CNT");
            connection_update_cnt = 0;
        }
        send_request_connect_parameter(temp_con_handle, connection_update_cnt / CONN_TRY_NUM);
    }
    conn_update_param_index = conn_update_param_index_record;
}


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

extern void rcsp_user_event_ble_handler(ble_state_e ble_status, u8 flag);
static void set_ble_work_state(ble_state_e state)
{
    log_info("ble_work_st:%x->%x\n", ble_work_state, state);
    ble_work_state = state;
    // 需要放到app_core处理ble状态的函数
    rcsp_user_event_ble_handler(state, 1);
}

static ble_state_e rcsp_get_ble_work_state(void)
{
    return ble_work_state;
}

static void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        case SM_EVENT_PAIR_PROCESS:
            log_info("======PAIR_PROCESS: %02x\n", event->data[0]);
            log_info_hexdump(event->data, 4);
            switch (event->data[0]) {
            case SM_EVENT_PAIR_SUB_RECONNECT_START:
                pair_reconnect_flag = 1;
                break;

            case SM_EVENT_PAIR_SUB_PIN_KEY_MISS:
            case SM_EVENT_PAIR_SUB_PAIR_FAIL:
            case SM_EVENT_PAIR_SUB_PAIR_TIMEOUT:
            case SM_EVENT_PAIR_SUB_ADD_LIST_SUCCESS:
            case SM_EVENT_PAIR_SUB_ADD_LIST_FAILED:
            default:
                break;
            }
            break;
        }
        break;
    }
}

void rcsp_user_cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    cbk_sm_packet_handler(packet_type, channel, packet, size);
}

static void can_send_now_wakeup(void)
{
    putchar('E');
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }

}

_WEAK_
u8 ble_update_get_ready_jump_flag(void)
{
    return 0;
}

static bool g_close_inquiry_scan = false;
extern void rcsp_clean_update_hdl_for_end_update(u16 ble_con_handle, u8 *spp_remote_addr);
void rcsp_ble_adv_enable_with_con_dev();
/**
 * @brief 一定时间设置是否关闭可发现可连接
 * 			对应TCFG_DUAL_CONN_INQUIRY_SCAN_TIME功能配置
 *
 * @param close_inquiry_scan 是否关闭可发现可连接
 */
void rcsp_close_inquiry_scan(bool close_inquiry_scan)
{
    g_close_inquiry_scan = close_inquiry_scan;
    log_info("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
#if TCFG_USER_TWS_ENABLE
    if ((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            // 根据已连接设备数量判断是否开关蓝牙广播
            rcsp_ble_adv_enable_with_con_dev();
        }
    }
#else
    // 根据已连接设备数量判断是否开关蓝牙广播
    rcsp_ble_adv_enable_with_con_dev();
#endif
}

// 返回当前设备支持的最大连接数
u8 rcsp_max_support_con_dev_num()
{
#if TCFG_RCSP_DUAL_CONN_ENABLE
    // 一拖二
    u8 max_con_dev = 2;
    if (g_close_inquiry_scan) {
        max_con_dev = 1;
    }
    if (!rcsp_get_1t2_switch()) {
        max_con_dev = 1;
    }
#else
    u8 max_con_dev = 1;
#endif
    return max_con_dev;
}

void rcsp_app_ble_set_mac_addr(void *addr)
{
    app_ble_set_mac_addr(rcsp_server_ble_hdl, addr);
    app_ble_set_mac_addr(rcsp_server_ble_hdl1, addr);
}

// 根据已连接设备数量判断是否开关蓝牙广播
void rcsp_ble_adv_enable_with_con_dev()
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("%s, rets=0x%x\n", __FUNCTION__, rets_addr);

    u8 max_con_dev = rcsp_max_support_con_dev_num();
    u8 conn_num = bt_rcsp_device_conn_num();
    log_info("%s, %s, %d, max:%d, conn_num:%d\n", __FILE__, __FUNCTION__, __LINE__, max_con_dev, conn_num);
#if TCFG_USER_TWS_ENABLE
    if (TWS_ROLE_MASTER == tws_api_get_role()) {
        if (conn_num >= max_con_dev) {
            rcsp_bt_ble_adv_enable(0);
        } else {
            ble_module_enable(1);
        }
    }
#else
    if (conn_num >= max_con_dev || alipay_adv_en) {
        rcsp_bt_ble_adv_enable(0);
    } else {
        ble_module_enable(1);
    }
#endif
}

extern u8 is_bredr_close(void);
extern void bredr_conn_last_dev();
extern u8 get_link_inquiry_state();
static void rcsp_att_check_remote_result(u16 con_handle, remote_type_e remote_type)
{
    log_info("smart %02x:remote_type= %02x\n", con_handle, remote_type);
    /* connect_remote_type = remote_type; */
    //to do
#if TCFG_BLE_BRIDGE_EDR_ENALBE
    if (pair_reconnect_flag == 1) { //ble回连ios时启动edr回连
        if (is_bredr_close() == 1) {
            bredr_conn_last_dev();
#if TCFG_USER_EMITTER_ENABLE
        } else if (get_link_inquiry_state() == 1) {
            bt_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL);
            log_info(">>>connect last source edr...\n");
            connect_last_source_device_from_vm();

#endif
        } else {
#if TCFG_USER_EMITTER_ENABLE
            log_info(">>>connect last edr...\n");
            connect_last_source_device_from_vm();
#else
            bt_cmd_prepare(USER_CTRL_CONNECT_LAST_REMOTE_INFO, 0, NULL);
#endif
        }
    }
#endif
}

void set_address_for_adv_handle(u8 adv_handle, u8 *addr);
static bool le_smart_check_connect_remote(u8 addr_type, u8 *addr)
{
    u8 temp_addr[6];
    swapX(addr, temp_addr, 6);
    if (ble_list_check_addr_is_exist(temp_addr, addr_type)) {
        log_info("remote in list owner connect");
        return true;
    } else {
        log_info("remote not in list, nonowner connect");
        return false;
    }
}
extern void bt_rcsp_set_conn_info(u16 con_handle, void *remote_addr, bool isconn);
/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                if (!hci_subevent_le_enhanced_connection_complete_get_role(packet)) {
                    //is master
                    break;
                }
                hci_con_handle_t con_handle = little_endian_read_16(packet, 4);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                connection_update_complete_success(packet + 8);
                bt_rcsp_set_conn_info(con_handle, NULL, true);
                log_info_hexdump(packet + 7, 7);
                memcpy(cur_peer_addr_info, packet + 7, 7);
#if TCFG_FINDMY_ENABLE
                fmy_owner_connect =  le_smart_check_is_fmy_owner(packet[7], &packet[8]);
#endif

#if RCSP_ADV_EN
                set_connect_flag(SECNE_CONNECTED);
                bt_ble_adv_ioctl(BT_ADV_SET_NOTIFY_EN, 1, 1);
#endif
                set_ble_work_state(BLE_ST_CONNECT);
                // 设置广播需要放在绑定bt_rcsp_set_conn_info之后
                rcsp_ble_adv_enable_with_con_dev();
                pair_reconnect_flag = 0;
#if (defined TCFG_CSC_BT_APP) && TCFG_CSC_BT_APP
                void screen_bt_ble_rcsp_send_info(void *p);
                sys_timeout_add(NULL, screen_bt_ble_rcsp_send_info, 1000);
#endif
            }
            break;
            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                log_info("HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE\n");
                connection_update_complete_success(packet);
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
            hci_con_handle_t dis_con_handle = little_endian_read_16(packet, 3);
            log_info("%s, %s, %d, dis_con_handle:%d\n", __FILE__, __FUNCTION__, __LINE__, dis_con_handle);
#if (TCFG_BLE_ADV_DYNAMIC_SWITCH && TCFG_UI_ENABLE)
            rcsp_bt_ble_adv_interval_reset();
#endif
            bt_rcsp_set_conn_info(dis_con_handle, NULL, false);
            rcsp_clean_update_hdl_for_end_update(dis_con_handle, NULL);
#if TCFG_RCSP_DUAL_CONN_ENABLE
            rcsp_1t2_reset_edr_info_for_ble_disconn(dis_con_handle);
#endif

#if RCSP_ADV_MUSIC_INFO_ENABLE
            extern void stop_get_music_timer(u8 en);
            stop_get_music_timer(1);
#endif
            // 设置广播需要放在解除绑定bt_rcsp_set_conn_info之后


#if RCSP_ADV_EN
            set_connect_flag(SECNE_UNCONNECTED);
#endif
            if (bt_rcsp_ble_conn_num() == 0) {
                set_ble_work_state(BLE_ST_DISCONN);
            }
#if RCSP_UPDATE_EN
            if (get_jl_update_flag()) {
                break;
            }
#if TCFG_RCSP_DUAL_CONN_ENABLE
            else {
                extern u32 classic_update_task_exist_flag_get(void);
                if (classic_update_task_exist_flag_get()) {
                    break;
                }
            }
#endif
#endif
            // 设置广播需要放在解除绑定bt_rcsp_set_conn_info之后
            //cppcheck-suppress knownConditionTrueFalse
            if (!ble_update_get_ready_jump_flag()) {
#if TCFG_USER_TWS_ENABLE
                if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
                    log_info(")))))))) 1\n");
                    rcsp_ble_adv_enable_with_con_dev();
                } else {
                    log_info("))))))))>>>> %d\n", tws_api_get_role());
                    if (tws_api_get_role() == TWS_ROLE_MASTER) {
                        rcsp_ble_adv_enable_with_con_dev();
                    }
                }
#else
                rcsp_ble_adv_enable_with_con_dev();
#endif
            }
#if TCFG_FINDMY_ENABLE
            if (fmy_owner_connect) {
                fmy_suffix_name_set();
                fmy_owner_connect = false;
            }
#endif

            connection_update_cnt = 0;
            break;
        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            u16 temp_con_hdl = little_endian_read_16(packet, 2);
            u32 tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!\n");
                check_connetion_updata_deal(temp_con_hdl);
            } else {
                connection_update_cnt = (CONN_PARAM_TABLE_CNT * CONN_TRY_NUM);
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            u16 temp_con_handle = little_endian_read_16(packet, 3);

            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            if (packet[2] == 0) {
                /*ENCRYPTION is ok */
                check_connetion_updata_deal(temp_con_handle);
                /*要配对加密后才能检测对方系统类型*/
                att_server_set_check_remote(temp_con_handle, rcsp_att_check_remote_result);
            }

            break;

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
#if TCFG_DATA_STORAGE_ENABLE
                    extern void notice_set_info_from_ancs(void *name, void *data, u16 len);
                    notice_set_info_from_ancs((void *)attribute_name, (void *)attribute_str, attribute_strlen);
#endif
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

void rcsp_user_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    cbk_packet_handler(packet_type, channel, packet, size);
}

void ancs_update_status(u8 status)
{
    switch (status) {
    case 1:
        log_info("ancs trunk start \n");
        break;
    case 2:
        log_info("ancs trunk end \n");
#if TRANS_ANCS_EN && TCFG_DATA_STORAGE_ENABLE
        extern void notice_add_info_from_ancs();
        notice_add_info_from_ancs();
#endif
        break;
    default:
        break;
    }
}

static int make_set_adv_data(void)
{
    return rcsp_make_set_adv_data();
}

static int make_set_rsp_data(void)
{
    return rcsp_make_set_rsp_data();
}


u8 *ble_get_gatt_profile_data(u16 *len)
{
    *len = sizeof(profile_data);
    return (u8 *)profile_data;
}

bool ble_switch_new_adv_interval(u16 adv_interval)
{
    if (set_adv_interval == adv_interval) {
        return true;
    }

    log_info("switch adv_interval:%d-->%d\n", adv_interval, set_adv_interval);
    set_adv_interval = adv_interval;


    if (BLE_ST_ADV == rcsp_get_ble_work_state()) {
        log_info("adv switch do\n");
        rcsp_bt_ble_adv_enable(0);
        rcsp_bt_ble_adv_enable(1);
    }
    return true;
}

//广播参数设置
static void advertisements_setup_init()
{
    uint8_t adv_type = APP_ADV_IND;
    uint8_t adv_channel = 7;
    int   ret = 0;
    u16 adv_interval = set_adv_interval;//0x30;

    /* if (adt_profile_support && rcsp_adt_support) { */
    /*     adv_type = APP_ADV_SCAN_IND; */
    /* } */

    //ble_op_set_adv_param(adv_interval, adv_type, adv_channel);
    app_ble_set_adv_param(rcsp_server_ble_hdl, adv_interval, adv_type, adv_channel);
    app_ble_set_adv_param(rcsp_server_ble_hdl1, adv_interval, adv_type, adv_channel);

    {
        ret |= make_set_adv_data();
        ret |= make_set_rsp_data();
    }

    if (ret) {
        log_error("advertisements_setup_init fail !!!!!!\n");
        return;
    }

}

static void ancs_notification_message(u8 *packet, u16 size)
{
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
#if TRANS_ANCS_EN && TCFG_DATA_STORAGE_ENABLE
        extern void notice_remove_info_from_ancs(u32 uid);
        notice_remove_info_from_ancs(ancs_notification_uid);
#endif
    } else if (value[0] == 0) {
        log_info("add message:ancs_notification_uid %04x", ancs_notification_uid);
#if TRANS_ANCS_EN && TCFG_DATA_STORAGE_ENABLE
        extern void notice_set_info_from_ancs(void *name, void *data, u16 len);
        notice_set_info_from_ancs("UID", &ancs_notification_uid, sizeof(ancs_notification_uid));
#endif
    }
}

/* ancs等待加密接口*/
void ancs_client_wait_request_pairing(u16 con_handle)
{
    u16 ble_con_handle = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
    if (ble_con_handle == con_handle) {
        sm_api_request_pairing(con_handle);
    }
}

#define RCSP_TCFG_BLE_SECURITY_EN          0/*是否发请求加密命令*/
extern void le_device_db_init(void);
extern bool gatt_client_check_ios_config(u8 mode);
extern void ancs_ams_set_ios_pair_request_enable(u8 enable);
void ble_profile_init(void)
{
#if TRANS_ANCS_EN || TRANS_AMS_EN
    gatt_client_check_ios_config(0);
#endif
}
void rcsp_ble_profile_init(void)
{
    log_info("ble profile init\n");
    /* le_device_db_init(); */

#if TRANS_ANCS_EN || TRANS_AMS_EN
    if ((!config_le_sm_support_enable) || (!config_le_gatt_client_num)) {
        log_error("ANCS need sm and client support!!!\n");
        ASSERT(0);
    }
#endif

    if (config_le_gatt_client_num) {
        //setup GATT client
        gatt_client_init();
    }

#if TRANS_ANCS_EN
    log_info("ANCS init...");
    //setup ANCS clent
    ancs_client_init();
    ancs_set_notification_buffer(ancs_info_buffer, sizeof(ancs_info_buffer));
    ancs_client_register_callback(&cbk_packet_handler);
    ancs_set_out_callback(ancs_notification_message);
#endif

#if TRANS_AMS_EN
    log_info("AMS init...");
    ams_client_init();
    ams_client_register_callback(&cbk_packet_handler);
    ams_entity_attribute_config(AMS_IDPlayer_ENABLE | AMS_IDQueue_ENABLE | AMS_IDTrack_ENABLE);
    /* ams_entity_attribute_config(AMS_IDTrack_ENABLE); */
#endif
#if TRANS_ANCS_EN || TRANS_AMS_EN || TCFG_BLE_BRIDGE_EDR_ENALBE
    ancs_ams_set_ios_pair_request_enable(0);
    gatt_client_check_ios_config(1);
    app_ble_set_ancs_connection_flag(rcsp_server_ble_hdl, 1);
    if (adt_profile_support && rcsp_adt_support) {
        app_ble_set_ancs_connection_flag(rcsp_server_edr_att_hdl, 1);
    }
#endif
#if !TRANS_ANCS_EN && !TRANS_AMS_EN && TCFG_BLE_BRIDGE_EDR_ENALBE
    ancs_ams_set_ios_pair_request_enable(1);
    gatt_client_check_ios_config(2);
#endif
#if BT_AI_SEL_PROTOCOL & ALIPAY_EN
    alipay_init();
#endif
}

static int set_adv_enable(void *priv, u32 en)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("%s, rets=0x%x\n", __FUNCTION__, rets_addr);
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en && en) {
        log_info("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
        return APP_BLE_OPERATION_ERROR;
    }

    {
        if (en) {
            /*控制开adv*/
#if !TCFG_RCSP_DUAL_CONN_ENABLE
            if (bt_rcsp_spp_conn_num() > 0) {
                log_info("spp connect type\n");
                log_info("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
                return APP_BLE_OPERATION_ERROR;
            }

            // 防止ios只连上ble的情况下，android(spp)回连导致ble断开后重新开广播的情况
            if (bt_rcsp_device_conn_num() > 0) {
                log_info("spp is connecting\n");
                log_info("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
                return APP_BLE_OPERATION_ERROR;
            }
#endif
        }
    }

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    log_info("adv_en:%d\n", en);
    set_ble_work_state(next_state);
    if (en) {
        advertisements_setup_init();

        if (app_ble_get_hdl_con_handle(rcsp_server_ble_hdl)) {
            app_ble_adv_enable(rcsp_server_ble_hdl1, en);
        } else {
            app_ble_adv_enable(rcsp_server_ble_hdl, en);
        }
        bt_ble_rcsp_adv_enable();
        u8 adv_handle = app_ble_adv_handle_get(rcsp_server_ble_hdl);
        u8 *addr = app_ble_adv_addr_get(rcsp_server_ble_hdl);
        set_address_for_adv_handle(adv_handle, addr);
    } else {
        app_ble_adv_enable(rcsp_server_ble_hdl, en);
        app_ble_adv_enable(rcsp_server_ble_hdl1, en);
    }
    return APP_BLE_NO_ERROR;
}

/**
 * @brief 断开指定的ble
 *
 * @param ble_con_handle ble_con_handle
 */
void rcsp_disconn_designated_ble(u16 ble_con_handle)
{
    log_info("%s, %s, %d, dis_con_handle:%d\n", __FILE__, __FUNCTION__, __LINE__, ble_con_handle);
    if (ble_con_handle) {
        u16 ble_con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
        u16 ble_con_hdl1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
        u16 dis_con_handle = (ble_con_handle == ble_con_hdl) ? ble_con_hdl : ble_con_hdl1;
        if (dis_con_handle == ble_con_hdl) {
            app_ble_disconnect(rcsp_server_ble_hdl);
        } else if (dis_con_handle == ble_con_hdl1) {
            app_ble_disconnect(rcsp_server_ble_hdl1);
        }
    }
}

/**
 * @brief 断开另一个ble
 *
 * @param ble_con_handle 保留的ble_con_handle，输入为0的时候，全部断开
 */
void rcsp_disconn_other_ble(u16 ble_con_handle)
{
    if (ble_con_handle) {
        u16 ble_con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
        u16 ble_con_hdl1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
        u16 dis_con_handle = (ble_con_handle == ble_con_hdl) ? ble_con_hdl1 : ble_con_hdl;
        if (dis_con_handle == ble_con_hdl) {
            app_ble_disconnect(rcsp_server_ble_hdl);
        } else if (dis_con_handle == ble_con_hdl1) {
            app_ble_disconnect(rcsp_server_ble_hdl1);
        }
    } else {
        app_ble_disconnect(rcsp_server_ble_hdl);
        app_ble_disconnect(rcsp_server_ble_hdl1);
    }
}

static int ble_disconnect(void *priv)
{
    if (bt_rcsp_ble_conn_num() > 0) {
        /* if (BLE_ST_SEND_DISCONN != rcsp_get_ble_work_state()) { */
        log_info(">>>ble send disconnect\n");
        set_ble_work_state(BLE_ST_SEND_DISCONN);
        app_ble_disconnect(rcsp_server_ble_hdl);
        app_ble_disconnect(rcsp_server_ble_hdl1);
        /* } else { */
        /* log_info(">>>ble wait disconnect...\n"); */
        /* } */
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

/**
 * @brief ble主机接收回调
 *
 * @param buf
 * @param len
 */
void rcsp_ble_master_recieve_callback(void *buf, u16 len)
{
    /* log_info("ble_rx(%d):\n", len); */
    /* log_info_hexdump(buf, len); */
    if (app_recieve_callback) {
        app_recieve_callback(0, buf, len);
    }
}

// rcsp内部调用
void rcsp_bt_ble_adv_enable(u8 enable)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
#if CONFIG_USE_RANDOM_ADDRESS_ENABLE
    if (enable) {
        log_info(">>>> SMARTBOX RPA ADDRESS SET");
        app_ble_adv_address_type_set(rcsp_server_ble_hdl, slave_address_random_type);
        le_controller_set_random_mac(le_smart_random_address);
        put_buf(le_smart_random_address, 6);
    }
#endif
    log_info("%s, enable:%d, rets=0x%x\n", __FUNCTION__, enable, rets_addr);
    adv_enable = enable;
    UI_MSG_POST("ble_button:button=%4", adv_enable);
    set_adv_enable(0, enable);
}

extern void ble_client_module_enable(u8 en);
void ble_module_enable(u8 en)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("%s, rets=0x%x\n", __FUNCTION__, rets_addr);
    log_info("%s:%d\n", __FUNCTION__, en);
    if (en) {
        adv_ctrl_en = 1;
#if (TCFG_BLE_ADV_DYNAMIC_SWITCH && TCFG_UI_ENABLE)
        rcsp_bt_ble_adv_interval_reset();
#endif
        rcsp_bt_ble_adv_enable(1);
    } else {
        rcsp_bt_ble_adv_enable(0);
        adv_ctrl_en = 0;
        if (bt_rcsp_ble_conn_num() > 0) {
            ble_disconnect(NULL);
        }
    }
}

u8 bt_ble_get_adv_enable(void)
{
    return adv_enable;
}

void rcsp_bt_ble_init(void)
{
    log_info("***** rcsp_bt_ble_init *****\n");

    if (ble_init_flag) {
        return;
    }
#if CONFIG_USE_RANDOM_ADDRESS_ENABLE
    slave_random_make_random_address();
    slave_random_period_timer_id = sys_timer_add(NULL, slave_random_period_timer_handler, slave_random_period_seconds * 1000);
#endif
    set_adv_interval = ADV_INTERVAL_MIN;


    gap_device_name = (char *)bt_get_local_name();
    gap_device_name_len = strlen(gap_device_name);
    if (gap_device_name_len > BT_NAME_LEN_MAX) {
        gap_device_name_len = BT_NAME_LEN_MAX;
    }

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    set_ble_work_state(BLE_ST_INIT_OK);
    bt_ble_init_do();

    ble_vendor_set_default_att_mtu(ATT_LOCAL_PAYLOAD_SIZE);

    const u8 *comm_addr = bt_get_mac_addr();
    u8 tmp_ble_addr[6] = {0};
#if  TCFG_BT_BLE_BREDR_SAME_ADDR
    memcpy(tmp_ble_addr, comm_addr, 6);
#else
    bt_make_ble_address(tmp_ble_addr, (u8 *)comm_addr);
#endif

    /* log_info("init tmp_ble_addr:\n"); */
    /* log_info_hexdump(tmp_ble_addr, 6); */
    app_ble_set_mac_addr(rcsp_server_ble_hdl, tmp_ble_addr);
    app_ble_set_mac_addr(rcsp_server_ble_hdl1, tmp_ble_addr);

    u8 lp_mode = 0;
#if (defined (CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE))
    lp_mode = get_ui_sys_param(low_power_mode);
#endif
    if (!lp_mode) {
        ble_module_enable(1);
    }

#if(RCSP_MODE && (RCSP_MODE !=  RCSP_MODE_WATCH))
    bt_ble_rcsp_adv_enable();
#endif

    ble_init_flag = 1;
}

void rcsp_bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");
    if (!ble_init_flag) {
        return;
    }
#if CONFIG_USE_RANDOM_ADDRESS_ENABLE
    sys_timer_del(slave_random_period_timer_id);
    slave_random_period_timer_id  = 0;
#endif

    /* ble_module_enable(0); */
    bt_ble_rcsp_adv_disable_timer();
    if (bt_rcsp_ble_conn_num() > 0) {
        ble_disconnect(NULL);
    }
#if RCSP_MODE
    extern void rcsp_exit(void);
    rcsp_exit();
#endif

    ble_init_flag = 0;

}


void ble_app_disconnect(void)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("%s, rets=0x%x\n", __FUNCTION__, rets_addr);
    ble_disconnect(NULL);
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

#define ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE 0x0009
#define ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE 0x000b
#define ATT_CHARACTERISTIC_bf01_01_VALUE_HANDLE 0x0011
#define ATT_CHARACTERISTIC_bf03_01_VALUE_HANDLE 0x0016

void bt_rcsp_custom_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    if (att_handle == ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE) {
        check_connetion_updata_deal(connection_handle);
    }

    // AE03 通道：SDK 推送流媒体数据，转发给流媒体模块处理
    if (att_handle == ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE) {
        /* printf("len:%d\n", buffer_size); */
        /* put_buf(buffer, buffer_size>32?32:buffer_size); */
        extern void jlstm_rx_push_stream_data(u8 * data, u16 len);
        jlstm_rx_push_stream_data(buffer, buffer_size);
    }

    // 流媒体协议：BF01 命令 / BF03 流数据
    if (buffer && buffer_size) {
        switch (att_handle) {
        case ATT_CHARACTERISTIC_bf01_01_VALUE_HANDLE:
            printf("bf01 len:%d\n", buffer_size);
            put_buf(buffer, buffer_size > 32 ? 32 : buffer_size);
            void stream_media_on_cmd_recv(const u8 * data, u16 len);
            stream_media_on_cmd_recv(buffer, buffer_size);
            break;
        case ATT_CHARACTERISTIC_bf03_01_VALUE_HANDLE:
            /* printf("bf03 len:%d\n", buffer_size); */
            /* put_buf(buffer, buffer_size>32?32:buffer_size); */
            /* put_buf(buffer, buffer_size); */
            void stream_media_on_data_recv(const u8 * data, u16 len);
            stream_media_on_data_recv(buffer, buffer_size);
            break;
        }
    }

#if WECHAT_SPORT_ENABLE
    bt_wechat_sport_att_write_callback(hdl, connection_handle, att_handle, transaction_mode, offset, buffer, buffer_size);
#endif
}

// SPP 自定义数据接收入口
void bt_rcsp_custom_recieve_callback(u16 ble_con_hdl, void *remote_addr,
                                     u8 *buf, u16 len, uint16_t att_handle)
{
    // BLE 侧 stream_media 已在 bt_rcsp_custom_att_write_callback 中按 att_handle 分发
    if (ble_con_hdl) {
        return;
    }

    // SPP 侧数据：RCSP 协议包已由 JL_protocol_data_recieve 处理，
    // 这里只处理不属于 RCSP 的数据（stream_media 等自定义协议）
    if (remote_addr && buf && len > 0) {
        extern bool rcsp_protocol_head_check(u8 * buf, u16 len);
        if (rcsp_protocol_head_check(buf, len)) {
            return;  // RCSP 协议数据，不处理
        }
        extern void stream_media_on_spp_recv(const u8 * data, u16 len);
        stream_media_on_spp_recv(buf, len);
    }
}

uint16_t bt_rcsp_custom_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    return bt_wechat_sport_att_read_callback(hdl, connection_handle, att_handle, offset, buffer, buffer_size);
}


#if (TCFG_BLE_ADV_DYNAMIC_SWITCH && TCFG_UI_ENABLE)
#include "ui/ui_api.h"

#define TCFG_BLE_ADV_QUICK_TIMEOUT			(10 *1000L)
#define TCFG_BLE_ADV_NORMAL_TIMEOUT			(30 *1000L)

extern int lcd_sleep_status();

static u16 rcsp_bt_ble_lcd_sleep_to = 0;

static void rcsp_bt_ble_lcd_sleep_enter(void);
static void rcsp_bt_ble_lcd_sleep_exit(void);

static int rcsp_bt_ble_lcd_sleep_deal(int to)
{
    /* log_info("\n\n le_lcd_sleep_deal:%d \n\n", to); */
    ble_switch_new_adv_interval(to);
    return 0;
}

static void rcsp_bt_ble_lcd_sleep_normal_to_deal(void *priv)
{
    /* log_info("\n\n le_lcd_sleep_normal_to_deal \n\n"); */
    rcsp_bt_ble_lcd_sleep_to = 0;
    int msg[3];
    msg[0] = (int)rcsp_bt_ble_lcd_sleep_deal;
    msg[1] = 1;
    msg[2] = (int)ADV_INTERVAL_SLOW;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

static void rcsp_bt_ble_lcd_sleep_quick_to_deal(void *priv)
{
    /* log_info("\n\n le_lcd_sleep_quick_to_deal \n\n"); */
    rcsp_bt_ble_lcd_sleep_to = 0;
    int msg[3];
    msg[0] = (int)rcsp_bt_ble_lcd_sleep_deal;
    msg[1] = 1;
    msg[2] = (int)ADV_INTERVAL_NORMAL;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
    rcsp_bt_ble_lcd_sleep_to = sys_timeout_add(NULL, rcsp_bt_ble_lcd_sleep_normal_to_deal, TCFG_BLE_ADV_NORMAL_TIMEOUT);
}

static void rcsp_bt_ble_adv_interval_reset(void)
{
    set_adv_interval = ADV_INTERVAL_MIN;
    if (lcd_sleep_status()) { // 息屏中
        rcsp_bt_ble_lcd_sleep_enter();
    }
}

static void rcsp_bt_ble_lcd_sleep_enter(void)
{
    // 灭屏
    if (rcsp_bt_ble_lcd_sleep_to) {
        sys_timeout_del(rcsp_bt_ble_lcd_sleep_to);
        rcsp_bt_ble_lcd_sleep_to = 0;
    }
    rcsp_bt_ble_lcd_sleep_to = sys_timeout_add(NULL, rcsp_bt_ble_lcd_sleep_quick_to_deal, TCFG_BLE_ADV_QUICK_TIMEOUT);
}

static void rcsp_bt_ble_lcd_sleep_exit(void)
{
    // 亮屏
    if (rcsp_bt_ble_lcd_sleep_to) {
        sys_timeout_del(rcsp_bt_ble_lcd_sleep_to);
        rcsp_bt_ble_lcd_sleep_to = 0;
    }
    int msg[3];
    msg[0] = (int)rcsp_bt_ble_lcd_sleep_deal;
    msg[1] = 1;
    msg[2] = (int)ADV_INTERVAL_QUICK;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

REGISTER_LCD_SLEEP_HEADLER(rcsp_bt_ble_lcd_sleep) = {
    .name = "vad",
    .enter = rcsp_bt_ble_lcd_sleep_enter,
    .exit = rcsp_bt_ble_lcd_sleep_exit,
};

#endif

#endif

