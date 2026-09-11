#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt.data.bss")
#pragma data_seg(".bt.data")
#pragma const_seg(".bt.text.const")
#pragma code_seg(".bt.text")
#endif
#include "app_config.h"
#include "system/includes.h"
#include "media/includes.h"
#include "app_tone.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "esco_recoder.h"
#include "bt.h"
#include "idle.h"
#include "bt_slience_detect.h"

#include "ui/ui_api.h"
#include "ui_manage.h"

#include "app_common.h"
#include "app_action.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/a2dp_media_codec.h"
#include "btctrler/btctrler_task.h"
#include "btctrler/btcontroller_modules.h"
#include "user_cfg.h"
#include "audio_cvp.h"
#include "bt_common.h"
#include "bt_ble.h"
#include "pbg_user.h"
#include "btstack/bluetooth.h"
#include "spp_online_db.h"

#include "bt_event_func.h"

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#if TCFG_ANC_BOX_ENABLE
#include "app_ancbox.h"
#endif

#if JL_RCSP_SENSORS_DATA_OPT
#include "sport_info_opt.h"
#include "shm_info_storage.h"
#endif

#include "bt_tws.h"
#include "asm/charge.h"
#include "app_charge.h"
#include "app_chargestore.h"
#include "app_testbox.h"
#include "app_online_cfg.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "clock_manager/clock_manager.h"
#include "bt_background.h"
#include "app_version.h"
#include "sdfile.h"
#include "dual_conn.h"
#include "alarm.h"
#include "rtc.h"
#include "btstack/third_party/rcsp/btstack_rcsp_user.h"

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
#include "app_le_connected.h"
#endif

#if TCFG_APP_BT_EN

#define LOG_TAG             "[WATCH]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define AVC_VOLUME_UP			0x41
#define AVC_VOLUME_DOWN			0x42
#define AVC_PLAY			    0x44
#define AVC_PAUSE			    0x46

#if TCFG_USER_BLE_CTRL_BREDR_EN
extern const int CONFIG_EDR_INIT_TIMEOUT; // 超时时间
#define BT_BREDR_INTI_TIMEOUT_MS			CONFIG_EDR_INIT_TIMEOUT
#else
#define BT_BREDR_INTI_TIMEOUT_MS			(0)
#endif
static u16 bt_bredr_to_id = 0;
static u16 bt_bredr_cnt = (u16) - 1;

BT_USER_COMM_VAR bt_user_comm_var;
struct bt_mode_var g_bt_hdl;
static u16 power_mode_timer = 0;
static u8 sniff_out = 0;

u8 a2dp_support_delay_report = 1;  ///蓝牙 delay report使能，蓝牙库使用，请勿删除

static u16 reset_idle_period_timeout = 0;

static int app_bt_init();
static int app_bt_exit();

static spinlock_t bt_code_ram;
static u8 *bt_code_run_addr = NULL;
extern u32 __bt_movable_slot_start[];
extern u32 __bt_movable_slot_end[];
extern u8 __bt_movable_region_start[];
extern u8 __bt_movable_region_end[];
extern const int support_reusable_special_update;
extern void bt_set_support_3M_size(u8 en);

#if defined USER_SUPPORT_PROFILE_PAN && USER_SUPPORT_PROFILE_PAN
static u16 pan_conn_id = 0;
#endif

u8 lmp_get_conn_num(void);
void bt_close_bredr();
void bt_close_bredr_timeout_start();
void bt_close_bredr_timeout_stop();
u8 ble_update_get_ready_jump_flag(void);
void multi_protocol_bt_exit(void);
void bt_sniff_feature_init();

u8 get_sniff_out_status()
{
    return sniff_out;
}
void clear_sniff_out_status()
{
    sniff_out = 0;
}
#if TCFG_FLASH_ERASE_WAIT_BLE
volatile static u8 ble_busy = 0;
/* extern int jiffies_msec2offset(unsigned long begin_msec, unsigned long end_msec); */
extern void ll_vendor_ble_busy_msg_cb(void (*callback)(uint8_t busy));
void ble_busy_status_set(u8 busy)
{
    ble_busy = busy;
}
u8 ble_busy_status_get()
{
    return ble_busy;
}
//等待蓝牙空闲，较长时间关中断前（如擦flash）需调用，等idle后再执行
void ble_busy_wait_idle(u8 type)
{
    unsigned long begin_msec = jiffies_msec();
    //通常不会超过100ms
    while (ble_busy) {
        int msec = jiffies_msec2offset(begin_msec, jiffies_msec());
        if (msec > 300) { //超时
            printf("[error]%s timeout!!", __func__);
            break;
        }
        os_time_dly(1);
    }
}
void ble_busy_status_init()
{
    ble_busy  = 0;
    //注册ble busy状态回调
    ll_vendor_ble_busy_msg_cb(ble_busy_status_set);

}
#else
void ble_busy_wait_idle(u8 type)
{

}
#endif


/*----------------------------------------------------------------------------*/
/**@brief   判断蓝牙通话是否在运行
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_must_work(void)
{
    if ((app_var.siri_stu) && (app_var.siri_stu != 3)) {
        // siri不退出
        return true;
    }

    if ((bt_get_call_status() == BT_CALL_OUTGOING)
        || (bt_get_call_status() == BT_CALL_ALERT)
        || (bt_get_call_status() == BT_CALL_INCOMING)
        || (bt_get_call_status() == BT_CALL_ACTIVE)
       ) {
        extern bool get_bt_esco_by_watch();
        if (get_bt_esco_by_watch()) {
            // 通话不退出
            return true;
        }
    }
    return false;
}


/**********进入蓝牙dut模式
*  mode=0:使能可以进入dut，原本流程不变。
*  mode=1:删除一些其它切换状态，产线中通过工具调用此接口进入dut模式，提高测试效率
 *********************/
void bt_bredr_enter_dut_mode(u8 mode, u8 inquiry_scan_en)
{
    puts("<<<<<<<<<<<<<bt_bredr_enter_dut_mode>>>>>>>>>>>>>>\n");
    clock_alloc("DUT", SYS_48M);
    bredr_set_dut_enble(1, 1);
    if (mode) {
        g_bt_hdl.auto_connection_counter = 0;
#if TCFG_USER_TWS_ENABLE
        bt_page_scan_for_test(inquiry_scan_en);
#endif
    }
}

static void bt_dut_clk_deal(void *priv)
{
    int sel = (int)priv;
    if (sel) { //enter BQB
        clock_force_lock("BQB", 96 * MHz);
    } else { //exit BQB
        clock_unlock("BQB");
    }
}

void bt_dut_clk_change(u8 sel)
{
    sys_timeout_add((void *)((int)sel), bt_dut_clk_deal, 5);
}


static u8 *bt_get_sdk_ver_info(u8 *len)
{
    const char *p = sdk_version_info_get();
    if (len) {
        *len = strlen(p);
    } else {
        return NULL;
    }

    log_info("sdk_ver:%s %x\n", p, *len);
    return (u8 *)p;
}

#if BT_CTKD_CONN_SPEED
void dut_idle_run_slot(u16 slot);
static void reset_idle_period_slot(void *p)
{
    reset_idle_period_timeout = 0;
    dut_idle_run_slot(1000);
}
extern void link_task_idle_run(int slot);
void ble_ctkd_pair_complete(hci_con_handle_t con_handle)
{
#if TCFG_USER_BT_CLASSIC_ENABLE
    if (reset_idle_period_timeout) {
        sys_timeout_del(reset_idle_period_timeout);
        reset_idle_period_timeout = 0;
    }
    printf("ble_ctkd_pair_complete\n");
    dut_idle_run_slot(400);
    reset_idle_period_timeout = sys_timeout_add(NULL, reset_idle_period_slot, 20 * 1000);
#endif
}
#endif


void bredr_handle_register()
{
#if (TCFG_BT_SUPPORT_SPP==1)
#if APP_ONLINE_DEBUG
    online_spp_init();
#endif
    bt_spp_data_deal_handle_register(spp_data_handler);
#endif
    bt_fast_test_handle_register(bt_fast_test_api);//测试盒快速测试接口
#if TCFG_BT_VOL_SYNC_ENABLE
    bt_music_vol_change_handle_register(set_music_device_volume, phone_get_device_vol);
#endif
#if TCFG_BT_DISPLAY_BAT_ENABLE
    bt_get_battery_value_handle_register(bt_get_battery_value);   /*电量显示获取电量的接口*/
#endif

    //样机进入dut被测试仪器链接上回调
    bt_dut_test_handle_register(bt_dut_api);

    //获取远端设备蓝牙名字回调
    bt_read_remote_name_handle_register(bt_read_remote_name);

#if TCFG_BT_MUSIC_INFO_ENABLE
    //获取歌曲信息回调
    bt_music_info_handle_register(user_get_bt_music_info);
#endif

}

void bt_function_select_init()
{
    //3M包
#if 0
    set_bt_data_rate_acl_3mbs_mode(1);
    bt_set_support_3M_size(1);
#endif

#if TCFG_USER_BT_CLASSIC_ENABLE && (!TCFG_USER_BLE_CTRL_BREDR_EN) && (!TCFG_NORMAL_SET_DUT_MODE) && (CONFIG_BT_MODE == BT_NORMAL)
    //set_edr_wait_conn_run_slot(1500, 12, 20, 10);
#endif

#if TCFG_BT_DUAL_CONN_ENABLE || TCFG_USER_EMITTER_ENABLE
    bt_set_user_ctrl_conn_num(2);
#else
    bt_set_user_ctrl_conn_num(1);
#endif
    bt_set_support_msbc_flag(TCFG_BT_MSBC_EN);

#if (!CONFIG_A2DP_GAME_MODE_ENABLE)
    bt_set_support_aac_flag(TCFG_BT_SUPPORT_AAC);
    bt_set_aac_bitrate(TCFG_AAA_BITRATE);
#endif

#if defined(TCFG_BT_SUPPORT_LHDC)
    bt_set_support_lhdc_flag(TCFG_BT_SUPPORT_LHDC);
#endif

#if defined(TCFG_BT_SUPPORT_LDAC)
    bt_set_support_ldac_flag(TCFG_BT_SUPPORT_LDAC);
#endif

#if TCFG_BT_DISPLAY_BAT_ENABLE
    bt_set_update_battery_time(60);
#else
    bt_set_update_battery_time(0);
#endif
    /*回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16*/
    bt_set_page_timeout_value(0);

    /*回连时超时参数设置。ms单位。做主机有效*/
    bt_set_super_timeout_value(8000);

#if TCFG_BT_DUAL_CONN_ENABLE
    bt_set_auto_conn_device_num(2);
#endif

#if TCFG_BT_VOL_SYNC_ENABLE
    vol_sys_tab_init();
#endif
    /* io_capabilities
     * 0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput
     *  authentication_requirements: 0:not protect  1 :protect
    */
    bt_set_simple_pair_param(3, 0, 2);

    /*测试盒连接获取参数需要的一些接口注册*/
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_VBAT_VALUE, get_vbat_value);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_VBAT_PERCENT, get_vbat_percent);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_BURN_CODE, sdfile_get_burn_code);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_SDK_VERSION, bt_get_sdk_ver_info);

    bt_set_sbc_cap_bitpool(TCFG_BT_SBC_BITPOOL);
#if TCFG_USER_BLE_ENABLE
    u8 tmp_ble_addr[6];
#if TCFG_BT_BLE_BREDR_SAME_ADDR
    memcpy(tmp_ble_addr, (void *)bt_get_mac_addr(), 6);
#else
    bt_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
#endif
    le_controller_set_mac((void *)tmp_ble_addr);
    puts("-----edr + ble 's address-----\n");
    printf_buf((void *)bt_get_mac_addr(), 6);
    printf_buf((void *)tmp_ble_addr, 6);
#endif

#if (CONFIG_BT_MODE != BT_NORMAL)
    set_bt_enhanced_power_control(1);
#endif

#if TCFG_BT_BACKGROUND_GOBACK
    bt_set_user_background_goback(1);
#else
    bt_set_user_background_goback(0);
#endif

#if (USER_SUPPORT_PROFILE_PBAP==1)
    ////设置蓝牙设备类型
    __change_hci_class_type(BD_CLASS_CAR_AUDIO);
#endif

#if TCFG_BT_CALL_PHONE_BY_WATCH
    lmp_negotiate_esco_parm(1);
#endif

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
    if (get_bt_le_audio_config_for_vm()) {
        log_info("le_audio en");
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN)))
        //g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_ONE;
#endif
    }
    if (get_bt_le_audio_config()) {
        bt_change_hci_class_type(BD_CLASS_WEARABLE_HEADSET | LE_AUDIO_CLASS);   //经典蓝牙地址跟le audio地址一样要置上BIT(14)
    }
#endif

}

#if JL_RCSP_SENSORS_DATA_OPT
static void bt_disconn_ui()
{
#if (defined CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE)
    if (UI_GET_WINDOW_ID() != ID_WINDOW_CONN_NEW_PHONE) {
        UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_BT_DISCONN_TIPS, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_PHONE);
    }
#endif
}

static void bt_disconn_func(void(*func)(void))
{
    // 读取vm是否要提示断开信息
    u8 bt_disconn_tip = !!sport_info_swtich_record_get(SPORT_INFO_SWTICH_TYPE_BT_DISCONN);
    if (bt_disconn_tip) {
        int msg[2];
        msg[0] = (int)func;
        msg[1] = 1;
        os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    }
}
#endif

#if defined USER_SUPPORT_PROFILE_PAN && USER_SUPPORT_PROFILE_PAN
static void pan_conn_handler(void *priv)
{
    extern int user_pan_send_cmd(u8 * addr, u32 cmd, u32 value, u8 * data);
    user_pan_send_cmd(NULL, 2, 0, NULL);
    if (pan_conn_id) {
        sys_timeout_del(pan_conn_id);
        pan_conn_id = 0;
    }
}
#endif

/*
 * 对应原来的状态处理函数，连接，电话状态等
 */
static int bt_connction_status_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        //3M包
#if 1
        set_bt_data_rate_acl_3mbs_mode(1);
        bt_set_support_3M_size(1);
#endif

        bt_status_init_ok();
        app_send_message(APP_MSG_STATUS_INIT_OK, APP_MODE_BT);
        break;

    case BT_STATUS_SECOND_CONNECTED:
        bt_clear_current_poweron_memory_search_index(0);
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED\n");

#if((RCSP_MODE == RCSP_MODE_EARPHONE) && RCSP_UPDATE_EN)
        if (rcsp_update_get_role_switch()) {
            tws_api_role_switch();
            tws_api_auto_role_switch_disable();
        }
#endif

#if TCFG_CHARGESTORE_ENABLE
        chargestore_set_phone_connect();
#endif
#if TCFG_USER_TWS_ENABLE
        bt_tws_phone_connected();
#endif
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info("BT_STATUS_DISCONNECT\n");
        if (app_var.goto_poweroff_flag) {
            break;
        } else {
#if JL_RCSP_SENSORS_DATA_OPT
            bt_disconn_func(bt_disconn_ui);    // 蓝牙断连ui显示
#endif
        }
#if TCFG_CHARGESTORE_ENABLE
        chargestore_set_phone_disconnect();
#endif
        bt_close_bredr_timeout_start();
#if defined USER_SUPPORT_PROFILE_PAN && USER_SUPPORT_PROFILE_PAN
        if (pan_conn_id) {
            sys_timeout_del(pan_conn_id);
            pan_conn_id = 0;
        }
#endif
        break;

    case BT_STATUS_CONN_A2DP_CH:
        log_info("++++++++ BT_STATUS_CONN_A2DP_CH +++++++++  \n");
        break;
    case BT_STATUS_DISCON_A2DP_CH:
        log_info("++++++++ BT_STATUS_DISCON_A2DP_CH +++++++++  \n");
        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
        log_info("BT_STATUS_AVRCP_INCOME_OPID:%d\n", bt->value);
        extern void bt_music_post_msg_to_lvgl_ui(const char *msg, ...);
        if (bt->value == AVC_VOLUME_UP) {

        } else if (bt->value == AVC_VOLUME_DOWN) {
        } else if (bt->value == AVC_PLAY) {
            bt_music_post_msg_to_lvgl_ui("music_state_on");
            bt_music_player_time_timer_deal(1);
        } else if (bt->value == AVC_PAUSE) {
            bt_music_post_msg_to_lvgl_ui("music_state_off");
            bt_music_player_time_timer_deal(0);
        }
        break;
    case  BT_STATUS_RECONN_OR_CONN:
#if TCFG_BT_SUPPORT_PBAP_LIST
        log_info("USER_CTRL_PBAP_CONNECT");
        bt_cmd_prepare(USER_CTRL_PBAP_CONNECT, 0, NULL);
#endif
#if TCFG_BT_SUPPORT_MAP
        bt_cmd_prepare(USER_CTRL_MAP_READ_TIME, 0, NULL);
#endif
#if defined USER_SUPPORT_PROFILE_PAN && USER_SUPPORT_PROFILE_PAN
        if (!pan_conn_id) {    // 延迟500ms再连接pan，避免连接失败
            pan_conn_id = sys_timeout_add(NULL, pan_conn_handler, 500);
        }
#endif
        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}

enum {
    TEST_STATE_INIT = 1,
    TEST_STATE_EXIT = 3,
};

enum {
    ITEM_KEY_STATE_DETECT = 0,
    ITEM_IN_EAR_DETECT,
};

static u8 in_ear_detect_test_flag = 0;
static void testbox_in_ear_detect_test_flag_set(u8 flag)
{
    in_ear_detect_test_flag = flag;
}

u8 testbox_in_ear_detect_test_flag_get(void)
{
    return in_ear_detect_test_flag;
}

static void bt_in_ear_detection_test_state_handle(u8 state, u8 *value)
{
    switch (state) {
    case TEST_STATE_INIT:
        testbox_in_ear_detect_test_flag_set(1);
        //start trim
        break;
    case TEST_STATE_EXIT:
        testbox_in_ear_detect_test_flag_set(0);
        break;
    }
}

static void bt_vendor_meta_event_handle(u8 sub_evt, u8 *arg, u8 len)
{
    log_info("vendor event:%x\n", sub_evt);
    log_info_hexdump(arg, 6);

    if (sub_evt != HCI_SUBEVENT_VENDOR_TEST_MODE_CFG) {
        log_info("unknow_sub_evt:%x\n", sub_evt);
        return;
    }

    u8 test_item = arg[0];
    u8 state = arg[1];

    if (ITEM_IN_EAR_DETECT == test_item) {
        bt_in_ear_detection_test_state_handle(state, NULL);
    }
}

extern u32 classic_update_task_exist_flag_get(void);
extern void vm_update_recover(bool update_faild);
static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("-----------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (bt->value == VENDOR_TEST_DISCONNECTED) {
#if TCFG_TEST_BOX_ENABLE
            if (testbox_get_status()) {
                if (bt_get_remote_test_flag()) {
                    testbox_clear_connect_status();
                }
            }
#endif
            bt_set_remote_test_flag(0);
#if (RCSP_MODE) // 对应br28的SMART_BOX_EN宏
            extern void testbox_update_mode_set(u8 mode);
            testbox_update_mode_set(0);
#endif
            log_info("clear_test_box_flag");
#if TCFG_TEST_BOX_WIRELESS_EN
            if (ble_update_get_ready_jump_flag()) {
                return 0;
            }
#endif
#if (CONFIG_REUSABLE_RESERVE)
            if (support_reusable_special_update) {
                extern void reusable_update_flag_clear(void);
                reusable_update_flag_clear();
            }
#elif (CONFIG_RESFS_UPDATE_ENABLE)
            if (support_reusable_special_update) {
                extern void resfs_update_flag_clear(void);
                resfs_update_flag_clear();
            }
#endif
            if (classic_update_task_exist_flag_get()) {
                // 防止测试盒升级中断后vm不还原
                vm_update_recover(true);
            }
            cpu_reset();
            return 0;
        } else {

#if (RCSP_MODE) // 对应br28的SMART_BOX_EN宏
            extern void testbox_update_mode_set(u8 mode);
            testbox_update_mode_set(bt->value);
#endif

#if (CONFIG_BT_MODE == BT_NORMAL)
#if BT_AI_SEL_PROTOCOL
            //1:edr con;2:ble con;
            if (VENDOR_TEST_LEGACY_CONNECTED_BY_BT_CLASSIC == bt->value) {
                bt_ble_adv_enable(0);
            }
#endif
#endif
#if TCFG_USER_TWS_ENABLE
            if (VENDOR_TEST_CONNECTED_WITH_TWS != bt->value) {
                bt_tws_poweroff();
            }
#endif
        }
    }


    switch (bt->event) {
    case HCI_EVENT_VENDOR_META:
        bt_vendor_meta_event_handle(bt->value, bt->args, 6);
        break;
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        bt_hci_event_inquiry(bt);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        break;

    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");

        if (bt->value ==  ERROR_CODE_CONNECTION_TIMEOUT) {
            bt_hci_event_connection_timeout(bt);
        }
        bt_hci_event_disconnect(bt) ;
        break;

    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            testbox_in_ear_detect_test_flag_set(0);
            break;

        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
            //case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt) ;
            break;

        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;

        default:
            break;

        }
        break;
    default:
        break;

    }
    return 0;
}

u8 bt_app_exit_check(void)
{
    int esco_state;
    if (app_var.siri_stu && app_var.siri_stu != 3 && bt_get_esco_coder_busy_flag()) {
        // siri不退出
        return 0;
    }
    esco_state = bt_get_call_status();
    if (esco_state == BT_CALL_OUTGOING  ||
        esco_state == BT_CALL_ALERT     ||
        esco_state == BT_CALL_INCOMING  ||
        esco_state == BT_CALL_ACTIVE) {
        // 通话不退出
        return 0;
    }

    return 1;
}

bool bt_check_already_initializes(void)
{
    return g_bt_hdl.init_start;
}

struct app_mode *app_enter_bt_mode(int arg)
{
    int msg[16];
    struct bt_event *event;
    struct app_mode *next_mode;

    app_bt_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), bt_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        event = (struct bt_event *)(msg + 1);

        switch (msg[0]) {
#if TCFG_USER_TWS_ENABLE
        case MSG_FROM_TWS:
            bt_tws_connction_status_event_handler(msg + 1);
            break;
#endif
        case MSG_FROM_BT_STACK:
            bt_connction_status_event_handler(event);
            break;
        case MSG_FROM_BT_HCI:
            bt_hci_event_handler(event);
            break;
        case MSG_FROM_APP:
            bt_app_msg_handler(msg + 1);
            break;
        }

        app_default_msg_handler(msg);
    }

    app_bt_exit();

    return next_mode;
}

static int bt_tone_play_end_callback(void *priv, enum stream_event event)
{
#if TCFG_USER_TWS_ENABLE && TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END
    if (event == STREAM_EVENT_STOP) {
        bt_tws_poweron();
    }
#endif
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙非后台模式退出蓝牙等待蓝牙状态可以退出
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_no_background_exit_check(void *priv)
{
    if (g_bt_hdl.init_ok == 0) {
        return;
    }
    if (esco_player_runing() || a2dp_player_runing()) {
        return ;
    }
#if TCFG_USER_BLE_ENABLE
    bt_ble_exit();
#endif
#if (BT_AI_SEL_PROTOCOL & (RCSP_MODE_EN | GFPS_EN | MMA_EN | FMNA_EN | REALME_EN | SWIFT_PAIR_EN | DMA_EN | ONLINE_DEBUG_EN | CUSTOM_DEMO_EN | LE_AUDIO_CIS_RX_EN))
    multi_protocol_bt_exit();
#endif
    btstack_exit();
    sys_timer_del(g_bt_hdl.exit_check_timer);
    g_bt_hdl.init_ok = 0;
    g_bt_hdl.init_start = 0;
    g_bt_hdl.exit_check_timer = 0;
    bt_set_stack_exiting(0);
    g_bt_hdl.exiting = 0;
    g_bt_hdl.bt_direct_init = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙非后台模式退出模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_nobackground_exit()
{
    if (!g_bt_hdl.init_start) {
        g_bt_hdl.exiting = 0;
        return 0;
    }
#if TCFG_USER_TWS_ENABLE
    tws_dual_conn_close();
    bt_tws_poweroff();
#else
    dual_conn_close();
#endif
    bt_set_stack_exiting(1);
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    if (g_bt_hdl.auto_connection_timer) {
        sys_timeout_del(g_bt_hdl.auto_connection_timer);
        g_bt_hdl.auto_connection_timer = 0;
    }

    if (g_bt_hdl.exit_check_timer == 0) {
        g_bt_hdl.exit_check_timer = sys_timer_add(NULL, bt_no_background_exit_check, 10);
        printf("set exit timer\n");
    }

    return 0;
}

static int app_bt_init()
{

#if TCFG_CODE_RUN_RAM_BT_CODE
    int bt_code_size = __bt_movable_region_end - __bt_movable_region_start;
    printf("bt_code_size:%d\n", bt_code_size);
    mem_stats();

    if (bt_code_size && bt_code_run_addr == NULL) {
        bt_code_run_addr = phy_malloc(bt_code_size);

    }
    spin_lock(&bt_code_ram);
    if (bt_code_run_addr) {
        printf("bt_code_run_addr:0x%x", (unsigned int)bt_code_run_addr);
        code_movable_load(__bt_movable_region_start, bt_code_size, bt_code_run_addr, __bt_movable_slot_start, __bt_movable_slot_end);
    }
    spin_unlock(&bt_code_ram);

    mem_stats();

#endif

    g_bt_hdl.init_start = 1;//蓝牙协议栈已经开始初始化标志位
    g_bt_hdl.init_ok = 0;
    g_bt_hdl.exiting = 0;
    g_bt_hdl.wait_exit = 0;
    g_bt_hdl.ignore_discon_tone = 0;
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if (TCFG_BT_BACKGROUND_ENABLE)      //后台返回到蓝牙模式如果是通过模式切换返回的还是要播放提示音
    if (g_bt_hdl.background.backmode == BACKGROUND_GOBACK_WITH_MODE_SWITCH && !bt_background_switch_mode_check())
#endif  //endif TCFG_BLUETOOTH_BACK_MODE
    {

#if TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END
        tone_player_stop();
        play_tone_file_callback(get_tone_files()->bt_mode, NULL, bt_tone_play_end_callback);
#else
        tone_player_stop();
        play_tone_file(get_tone_files()->bt_mode);
#endif
    }
#if (TCFG_BT_BACKGROUND_ENABLE)
    if (g_bt_hdl.background.background_working) {
        g_bt_hdl.init_ok = 1;
        bt_background_resume();
        app_send_message(APP_MSG_ENTER_MODE, APP_MODE_BT);
        return 0;
    }

    bt_background_init(bt_hci_event_handler, bt_connction_status_event_handler);
#endif  //endif TCFG_BLUETOOTH_BACK_MODE

    bt_function_select_init();
    bredr_handle_register();
    EARPHONE_STATE_INIT();
    btstack_init();
#if TCFG_FLASH_ERASE_WAIT_BLE
    ble_busy_status_init();
#endif//TCFG_FLASH_ERASE_WAIT_BLE
#if TCFG_USER_TWS_ENABLE
    tws_profile_init();
#endif

    void bt_sniff_feature_init();
    bt_sniff_feature_init();
    app_var.dev_volume = -1;

#if TCFG_PITCH_SPEED_NODE_ENABLE
    app_var.pitch_mode = PITCH_0;    //设置变调初始模式
#endif
    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_BT);

    return 0;
}

int bt_mode_try_exit()
{
    putchar('k');

    if (g_bt_hdl.wait_exit) {
        //等待蓝牙断开或者音频资源释放或者电话资源释放
        if (!g_bt_hdl.exiting) {
            g_bt_hdl.wait_exit++;
            if (g_bt_hdl.wait_exit > 3) {
                //wait two round to do some hci event or other stack event
                return 0;
            }
        }
        return -EBUSY;
    }
    g_bt_hdl.wait_exit = 1;
    g_bt_hdl.exiting = 1;
#if(TCFG_BT_BACKGROUND_ENABLE == 0) //非后台退出不播放提示音
    g_bt_hdl.ignore_discon_tone = 1;
#endif
    //only need to do once
#if (TCFG_BT_BACKGROUND_ENABLE)
    bt_background_suspend();
#else
    bt_nobackground_exit();
#endif
    return -EBUSY;
}

static int app_bt_exit()
{
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_BT);

#if TCFG_CODE_RUN_RAM_BT_CODE
    if (bt_code_run_addr) {
        mem_stats();

        spin_lock(&bt_code_ram);
        code_movable_unload(__bt_movable_region_start, __bt_movable_slot_start, __bt_movable_slot_end);
        spin_unlock(&bt_code_ram);

        phy_free(bt_code_run_addr);
        bt_code_run_addr = NULL;
        mem_stats();
        printf("\n-------------bt_exit ok-------------\n");
    }
#endif

    sys_auto_shut_down_disable();

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙直接开关
   @param
   @return
   @note   如果想后台开机不需要进蓝牙，可以在poweron模式
   			直接调用这个函数初始化蓝牙
*/
/*----------------------------------------------------------------------------*/

void bt_direct_init()
{
    if (g_bt_hdl.bt_direct_init || g_bt_hdl.init_ok) {
        return;
    }
    log_info(" bt_direct_init  \n");

    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    g_bt_hdl.ignore_discon_tone = 0;
    g_bt_hdl.bt_direct_init = 1;
    g_bt_hdl.init_start = 1;//蓝牙协议栈已经开始初始化标志位

    bt_function_select_init();
    bredr_handle_register();
    EARPHONE_STATE_INIT();
    btstack_init();
#if TCFG_FLASH_ERASE_WAIT_BLE
    ble_busy_status_init();
#endif//TCFG_FLASH_ERASE_WAIT_BLE
    bt_sniff_feature_init();

}
/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台直接关闭
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_direct_close(void)
{
    if (!g_bt_hdl.init_start) {
        g_bt_hdl.exiting = 0;
        return;
    }

    log_info(" bt_direct_close");

    g_bt_hdl.ignore_discon_tone = 1;
    g_bt_hdl.bt_direct_init = 0;
    sys_auto_shut_down_disable();

    bt_set_stack_exiting(1);
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    if (g_bt_hdl.auto_connection_timer) {
        sys_timeout_del(g_bt_hdl.auto_connection_timer);
        g_bt_hdl.auto_connection_timer = 0;
    }

    if (g_bt_hdl.exit_check_timer == 0) {
        g_bt_hdl.exit_check_timer = sys_timer_add(NULL, bt_no_background_exit_check, 10);
        printf("set exit timer\n");
    }

}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙  关闭bredr
   @param
   @return
   @note   主要把bredr 运行状态都关闭即可
*/
/*----------------------------------------------------------------------------*/
static int bt_close_bredr_do(int priv)
{
    u16 do_cnt = priv;
    if (do_cnt != bt_bredr_cnt) {
        log_info("%s, cnt:%d,%d \n", __func__, do_cnt, bt_bredr_cnt);
        return 0;
    }
    if (g_bt_hdl.bt_close_bredr == 1) {
        bt_close_bredr_timeout_stop();
        return 0;
    }
    /* printf("%s",__func__); */
    g_bt_hdl.bt_close_bredr = 1;
    UI_MSG_POST("edr_button:button=%4", 0);
    if (g_bt_hdl.auto_connection_timer) {
        sys_timeout_del(g_bt_hdl.auto_connection_timer);
        g_bt_hdl.auto_connection_timer = 0;
    }
    if (BT_BREDR_INTI_TIMEOUT_MS && bt_bredr_to_id) {
        sys_timeout_del(bt_bredr_to_id);
        bt_bredr_to_id = 0;
    }
    bt_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);

    os_time_dly(10);
    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    sys_auto_sniff_controle(0, NULL);
    btctrler_task_close_bredr();
    bt_set_stack_exiting(1);
    return 0;
}

static void bt_timeout_close_bredr(void *priv)
{
    bt_bredr_to_id = 0;
    u16 do_cnt = (u16)priv;
#if (RCSP_MODE) // 对应br28的SMART_BOX_EN宏
    // 如果当前是蓝牙测试盒升级，就不走关闭经典蓝牙流程
    // 如果当前是测试盒ble升级，则继续走关闭经典蓝牙流程
    extern u8 testbox_update_mode_get(void);
    if (1 == testbox_update_mode_get()) {
        return;
    }
#endif
    if (do_cnt != bt_bredr_cnt) {
        printf("%s, cnt:%d,%d \n", __func__, do_cnt, bt_bredr_cnt);
        return ;
    }
    if (BT_STATUS_WAITINT_CONN != bt_get_connect_status()) {
        printf("bt connect:0x%x \n", bt_get_connect_status());
        return ;
    }
#if TCFG_USER_EMITTER_ENABLE
    if (BT_STATUS_WAITINT_CONN != bt_emitter_get_connect_status()) {
        printf("bt connect:0x%x \n", bt_emitter_get_connect_status());
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        return ;
    }
#endif
    printf("bt_timeout_close_bredr \n");
    play_tone_file(get_tone_files()->bt_discon[1]);
    bt_close_bredr(do_cnt);
}

void bt_close_bredr_timeout_start()
{
    if (BT_BREDR_INTI_TIMEOUT_MS) {
        if (bt_bredr_to_id) {
            sys_timeout_del(bt_bredr_to_id);
            bt_bredr_to_id = 0;
        }
        bt_bredr_to_id = sys_timeout_add((char *)((int)bt_bredr_cnt), bt_timeout_close_bredr, BT_BREDR_INTI_TIMEOUT_MS);
    }
}

void bt_close_bredr_timeout_stop()
{
    if (BT_BREDR_INTI_TIMEOUT_MS && bt_bredr_to_id) {
        sys_timeout_del(bt_bredr_to_id);
        bt_bredr_to_id = 0;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙  开启bredr
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int bt_init_bredr_do(int priv)
{
    u16 do_cnt = priv;
    if (do_cnt != bt_bredr_cnt) {
        printf("%s, cnt:%d,%d \n", __func__, do_cnt, bt_bredr_cnt);
        return 0;
    }
    if (g_bt_hdl.bt_close_bredr == 0) {
        bt_discovery_and_connectable_using_loca_mac_addr(1, 1);
        if (BT_BREDR_INTI_TIMEOUT_MS && bt_bredr_to_id) {
            sys_timeout_del(bt_bredr_to_id);
            bt_bredr_to_id = 0;
            bt_bredr_to_id = sys_timeout_add((char *)((int)bt_bredr_cnt), bt_timeout_close_bredr, BT_BREDR_INTI_TIMEOUT_MS);
        }
        return 0;
    }
    g_bt_hdl.bt_close_bredr = 0;
    UI_MSG_POST("edr_button:button=%4", 1);
    bt_set_stack_exiting(0);
    btctrler_task_init_bredr();
#if TCFG_EDR_SCAN_CONN_CTRL
    int edr_num = btstack_get_num_of_remote_device_recorded();
    int ble_num = bt_rcsp_ble_conn_num();
    log_info("%s edr_num:%d, ble_num:%d", __func__, edr_num, ble_num);
    if (edr_num > 0) {
        bt_discovery_and_connectable_using_loca_mac_addr(0, 1);
    } else {
        if (ble_num > 0) {
            /*一键连接，ble已经连接了时候，edr状态为可连接*/
            bt_discovery_and_connectable_using_loca_mac_addr(0, 1);
        } else {
            bt_discovery_and_connectable_using_loca_mac_addr(1, 1);
        }
    }
#else
    bt_discovery_and_connectable_using_loca_mac_addr(1, 1);
#endif
    sys_auto_sniff_controle(1, NULL);
    if (BT_BREDR_INTI_TIMEOUT_MS) {
        if (bt_bredr_to_id) {
            sys_timeout_del(bt_bredr_to_id);
            bt_bredr_to_id = 0;
        }
        bt_bredr_to_id = sys_timeout_add((char *)((int)bt_bredr_cnt), bt_timeout_close_bredr, BT_BREDR_INTI_TIMEOUT_MS);
    }
    return 0;
}

static int bredr_conn_last_dev_do(int priv)
{
    u16 do_cnt = priv;
    if (do_cnt != bt_bredr_cnt) {
        printf("%s, cnt:%d,%d \n", __func__, do_cnt, bt_bredr_cnt);
        return 0;
    }
    bt_init_bredr_do(priv);
#if TCFG_USER_EMITTER_ENABLE
    connect_last_source_device_from_vm();
#else
    u8 mac_addr[6];
    int num = btstack_get_num_of_remote_device_recorded();
    if (num) {
        btstack_get_remote_addr(mac_addr, num - 1);
        dual_conn_user_bt_connect(mac_addr);
    }
#endif /* #if TCFG_USER_EMITTER_ENABLE */
    return 0;
}

struct conn_dev_data {
    int bt_bredr_cnt;
    u8 mac[6];
};
/* ***************************************************************************/
/**
 * @brief   ：bredr_conn_dev_do
 初始化edr并连接指定mac设备
 *
 * @参数    ：priv
 *
 * @返回参数：
 */
/* ***************************************************************************/
static int bredr_conn_dev_do(int priv)
{
    struct conn_dev_data *ptr = (struct conn_dev_data *)priv;
    ASSERT(ptr);
    int do_cnt = ptr->bt_bredr_cnt;
    if (do_cnt != bt_bredr_cnt) {
        printf("%s, cnt:%d,%d \n", __func__, do_cnt, bt_bredr_cnt);
        free(ptr);
        return 0;
    }
    bt_init_bredr_do(do_cnt);
    bt_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, ptr->mac);
    free(ptr);
    return 0;
}

/* ***************************************************************************/
/**
 * @brief   ：bredr_search_device_do
    初始化edr并发起inquiry搜索设备
 *
 * @参数    ：priv
 *
 * @返回参数：
 */
/* ***************************************************************************/
static int bredr_search_device_do(int priv)
{
    u16 do_cnt = priv;
    if (do_cnt != bt_bredr_cnt) {
        printf("%s, cnt:%d,%d \n", __func__, do_cnt, bt_bredr_cnt);
        return 0;
    }
    bt_init_bredr_do(priv);
#if TCFG_USER_EMITTER_ENABLE
    /* ////关闭可发现可链接 */
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_search_device();
#endif /* #if TCFG_USER_EMITTER_ENABLE */
    return 0;
}

void bt_close_bredr()
{
    bt_bredr_cnt ++;
    /* printf("%s close=%d",__func__,__this->bt_close_bredr); */
    if (!strcmp(os_current_task(), "app_core")) {
        bt_close_bredr_do(bt_bredr_cnt);
    } else {
        int msg[3] = {0};
        msg[0] = (int)bt_close_bredr_do;
        msg[1] = 1;
        msg[2] = (int)bt_bredr_cnt;
        do {
            int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
            if (os_err == OS_ERR_NONE) {
                break;
            }
            if (os_err != OS_Q_FULL) {
                return ;
            }
            os_time_dly(1);
        } while (1);
    }
}

void bt_init_bredr()
{
    bt_bredr_cnt ++;
    if (!strcmp(os_current_task(), "app_core")) {
        bt_init_bredr_do(bt_bredr_cnt);
    } else {
        int msg[3] = {0};
        msg[0] = (int)bt_init_bredr_do;
        msg[1] = 1;
        msg[2] = (int)bt_bredr_cnt;
        do {
            int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
            if (os_err == OS_ERR_NONE) {
                break;
            }
            if (os_err != OS_Q_FULL) {
                return ;
            }
            os_time_dly(1);
        } while (1);
    }
}

void bredr_conn_last_dev()
{
    bt_bredr_cnt ++;
    if (!strcmp(os_current_task(), "app_core")) {
        bredr_conn_last_dev_do(bt_bredr_cnt);
    } else {
        int msg[3] = {0};
        msg[0] = (int)bredr_conn_last_dev_do;
        msg[1] = 1;
        msg[2] = (int)bt_bredr_cnt;
        do {
            int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
            if (os_err == OS_ERR_NONE) {
                break;
            }
            if (os_err != OS_Q_FULL) {
                return ;
            }
            os_time_dly(1);
        } while (1);
    }
}

void bredr_conn_dev(u8 *mac)
{
    ASSERT(mac);
    bt_bredr_cnt ++;
    static struct conn_dev_data *info;
    info = malloc(sizeof(struct conn_dev_data));
    ASSERT(info);
    info->bt_bredr_cnt = (int)bt_bredr_cnt;
    memcpy(info->mac, mac, 6);
    if (!strcmp(os_current_task(), "app_core")) {
        bredr_conn_dev_do((int)info);
    } else {
        int msg[3] = {0};
        msg[0] = (int)bredr_conn_dev_do;
        msg[1] = 1;
        msg[2] = (int)info;
        do {
            int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
            if (os_err == OS_ERR_NONE) {
                break;
            }
            if (os_err != OS_Q_FULL) {
                free(info);
                return ;
            }
            os_time_dly(1);
        } while (1);
    }
}

void bredr_search_device()
{
    bt_bredr_cnt ++;
    if (!strcmp(os_current_task(), "app_core")) {
        bredr_search_device_do(bt_bredr_cnt);
    } else {
        int msg[3] = {0};
        msg[0] = (int)bredr_search_device_do;
        msg[1] = 1;
        msg[2] = (int)bt_bredr_cnt;
        do {
            int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
            if (os_err == OS_ERR_NONE) {
                break;
            }
            if (os_err != OS_Q_FULL) {
                return ;
            }
            os_time_dly(1);
        } while (1);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙  bredr状态
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 is_bredr_close(void)
{
    printf("%s,g_bt_hdl.bt_close_bredr=%d", __func__, g_bt_hdl.bt_close_bredr);
    return g_bt_hdl.bt_close_bredr;
}
#if TCFG_BT_SUPPORT_MAP
#define    PROFILE_CMD_TRY_AGAIN_LATER      -1004
void bt_get_time_date()
{
    int error = bt_cmd_prepare(USER_CTRL_HFP_GET_PHONE_DATE_TIME, 0, NULL);
    printf("<%s> >>>>>error = %d\n", __func__, error);
    if (error == PROFILE_CMD_TRY_AGAIN_LATER) {
        sys_timeout_add(NULL, bt_get_time_date, 100);
    }
}
void phone_date_and_time_feedback(u8 *data,  u16 len)
{
    printf("<%s> time：%s ", __func__, data);
#if TCFG_APP_RTC_EN
    /*获取到的格式"24/07/15, 17:52:22"*/
    struct sys_time phone_time;
    phone_time.year = 2000 + (data[0] - '0') * 10 + (data[1] - '0');
    phone_time.month = (data[3] - '0') * 10 + (data[4] - '0');
    phone_time.day = (data[6] - '0') * 10 + (data[7] - '0');
    phone_time.hour = (data[10] - '0') * 10 + (data[11] - '0');
    phone_time.min = (data[13] - '0') * 10 + (data[14] - '0');
    phone_time.sec = (data[16] - '0') * 10 + (data[17] - '0');
    log_info("sys_time %d:%d:%d %d:%d:%d", phone_time.year, phone_time.month, phone_time.day, phone_time.hour, phone_time.min, phone_time.sec);
    rtc_update_time_api(&phone_time);
#endif
}
void map_get_time_data(char *time, int status)
{
    if (status == 0 && time != NULL) {
        printf("<%s> time：%s ", __func__, time);
#if TCFG_APP_RTC_EN
        /*获取到的格式"20240715T174441+0800"*/
        struct sys_time phone_time;
        phone_time.year = ((time[0] - '0') * 1000) + ((time[1] - '0') * 100) + ((time[2] - '0') * 10) + (time[3] - '0');
        phone_time.month = (time[4] - '0') * 10 + (time[5] - '0');
        phone_time.day = (time[6] - '0') * 10 + (time[7] - '0');
        phone_time.hour = (time[9] - '0') * 10 + (time[10] - '0');
        phone_time.min = (time[11] - '0') * 10 + (time[12] - '0');
        phone_time.sec = (time[13] - '0') * 10 + (time[14] - '0');
        log_info("sys_time %d:%d:%d %d:%d:%d", phone_time.year, phone_time.month, phone_time.day, phone_time.hour, phone_time.min, phone_time.sec);
        rtc_update_time_api(&phone_time);
#endif
    } else  {
        printf("<%s> >>>map get fail\n", __func__);
        sys_timeout_add(NULL, bt_get_time_date, 100);
    }
}
#endif

#if 0
//opp传输文件demo
static FILE *fp = NULL;
__attribute__((weak))
int opp_continue_handle(u8 *packet, u8 event, u32 send_read_len)
{
    u8 data[] = "storage/sd0/C/23333.txt";
    u8 data_name[] = "23333.txt";
    switch (event) {
    case 0xa0://将name数据填入packet；将文件总长度返回
        fp = fopen(data, "r");
        memcpy(packet, data_name, strlen(data_name));
        if (fp == NULL) {
            printf("检查tf卡，或者name错误\n");
            return -1;
        }
        u32 fp_len = flen(fp);
        return fp_len;
    case 0x90://上一包文件数据发送完成，填入下一包
        /* r_printf(">> send_read_len = %d\n",send_read_len); */
        if (fp == NULL) {
            printf("检查tf卡，或者name错误\n");
            return -1;
        }
        u32 read_len = fread(fp, packet, send_read_len);
        /* r_printf(">> read_len = %d\n",read_len); */
        if (read_len < send_read_len) {
            fclose(fp);
        }
        return read_len;
    case 0xC3://结束事件
        if (fp) {
            r_printf(">>fclose(fp) \n");
            fclose(fp);
        }
        break;
    }
    return -1; //错误返回-1

}
//opp接收文件demo
__attribute__((weak))
int opp_rx_data(u8 *packet, u32 packet_len, u8 event)
{
    /* put_buf(packet+3, packet_len); */
    switch (event) {
    case 0x01: //收到name数据
        u8 sd_root_path[255] = "storage/sd0/C/\\U";
        u8 sd_root_path_len = strlen(sd_root_path);
        for (int i = 0; i < packet_len - 2; i += 2) {
            sd_root_path[sd_root_path_len + i] = packet[1 + i];        //高低位交换
            sd_root_path[sd_root_path_len + i + 1] = packet[i];
        }
        printf("name: %s", sd_root_path);
        put_buf(sd_root_path, sd_root_path_len + packet_len);
        fp = fopen(sd_root_path, "w+");
        if (fp == NULL) {
            printf("检查tf卡，或者name错误\n");
            return -1;
        }
        break;
    case 0x02: //收到文件数据
        putchar('W');
        if (fp == NULL) {
            printf("检查tf卡，或者name错误\n");
            return -1;
        }
        fwrite(fp, packet, packet_len);
        break;
    case 0x03: //收到结束命令
        putchar('F');
        if (fp) {
            fclose(fp);
            fp = NULL;
        }
        break;
    }
    return 1;
}
#endif
#if TCFG_FINDMY_ENABLE
extern int bt_modify_name(u8 *new_name);
void fmy_other_device_bt_name_set(u8 *name)
{
    log_info("fmy change eir local name");
    bt_modify_name(name);
    // just set when in scan state
    if (!lmp_get_conn_num()) {
        log_info("bt scan disable");
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        log_info("bt scan enable");
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    }
}
#endif

#if TCFG_BT_PA_ENABLE
extern void bt_set_rxtx_status_io(u32 tx_pin, u32 rx_pin);
void bt_pa_init(void)
{
    log_info("%s %d", __func__, __LINE__);

//     /**根据具体的pa芯片和匹配料，具体调整该参数
//       *公版环境CB2401, AT2401 使用0x08
//       */
//     extern void balun_interface(u8 recfg_en, u8 txbalun);
//     balun_interface(1, 0x8);

    bt_set_rxtx_status_io(TCFG_BT_PA_TX_IO, TCFG_BT_PA_RX_IO);

    //使能PA芯片
    if (TCFG_BT_PA_PWR_IO != NO_CONFIG_PORT) {
        if (TCFG_BT_PA_PWR_IO_LEVEL == 1) {
            gpio_set_mode(IO_PORT_SPILT(TCFG_BT_PA_PWR_IO), PORT_OUTPUT_HIGH);
        } else if (TCFG_BT_PA_PWR_IO_LEVEL == 0) {
            gpio_set_mode(IO_PORT_SPILT(TCFG_BT_PA_PWR_IO), PORT_OUTPUT_LOW);
        }
    }
}
#endif

static int bt_mode_try_enter(int arg)
{
    return 0;
}

static const struct app_mode_ops bt_mode_ops = {
    .try_enter      = bt_mode_try_enter,
    .try_exit       = bt_mode_try_exit,
};

REGISTER_APP_MODE(bt_mode) = {
    .name   = APP_MODE_BT,
    .index  = APP_MODE_BT_INDEX,
    .ops    = &bt_mode_ops,
};
#endif


