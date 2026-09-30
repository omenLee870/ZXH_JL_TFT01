#include "bt.h"
#include "app_config.h"
#include "app_main.h"
#include "app_msg.h"
#include "btstack/avctp_user.h"
#include "dual_conn.h"
#include "ui/ui_api.h"
#include "file_player.h"
#if TCFG_VSTREAM_TRANSFER
#include "kid_wt_app.h"
#endif
#define LOG_TAG             "[EMITTER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USER_EMITTER_ENABLE

#define BT_EMITTER_TEST         0

#define  SEARCH_BD_ADDR_LIMITED 0
#define  SEARCH_BD_NAME_LIMITED 1
#define  SEARCH_CUSTOM_LIMITED  2
#define  SEARCH_NULL_LIMITED    3

#define SEARCH_LIMITED_MODE  SEARCH_BD_NAME_LIMITED

struct list_head inquiry_noname_list;
struct inquiry_noname_remote {
    struct list_head entry;
    u8 match;
    s8 rssi;
    u8 addr[6];
    u32 class;
};

u8 restore_remote_device_info_profile(bd_addr_t mac_addr, u8 device_num, u8 id, u8 profile);

static u8 read_name_start = 0;
static u8 search_spp_device = 0;

enum emitter_search_state {
    EMITTER_SEARCH_IDLE = 0,
    EMITTER_SEARCH_RUNNING,
    EMITTER_SEARCH_STOPPING,
};

static volatile u8 emitter_search_state = EMITTER_SEARCH_IDLE;
static volatile u8 emitter_user_operation_active;
static u8 emitter_a2dp_source_connected;
static u8 emitter_connect_target_valid;
static u8 emitter_connect_target_addr[6];
static u16 emitter_search_stop_timer;

#define EMITTER_SEARCH_STOP_CHECK_MS    1000

extern u8 get_link_inquiry_state(void);
u8 emitter_search_stop_handle(u8 result);
static void emitter_search_stop_check(void *priv);

static void emitter_search_stop_watchdog_start(void)
{
    if (!emitter_search_stop_timer) {
        emitter_search_stop_timer = sys_timeout_add(NULL,
                                    emitter_search_stop_check,
                                    EMITTER_SEARCH_STOP_CHECK_MS);
    }
}

__attribute__((weak))
void bt_emitter_search_complete_reconnect_deal(void)
{
}

void bt_emitter_user_operation_set(u8 active)
{
    emitter_user_operation_active = !!active;
    log_info("[EMITTER_SEARCH_TRACE] user_operation_active=%d",
             emitter_user_operation_active);
}

u8 bt_emitter_user_operation_is_active(void)
{
    return emitter_user_operation_active;
}

extern struct file_player *get_music_file_player(void); //返回第一个打开的音乐播放器指针
extern int music_file_get_player_status(struct file_player *music_player);
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙发射发起搜索设备
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_search_device(void)
{
    log_info("[EMITTER_SEARCH_TRACE] search_req state=%d init=%d role=%d close=%d",
             emitter_search_state, bt_check_already_initializes(),
             g_bt_hdl.emitter_or_receiver, is_bredr_close());
    if (is_bredr_close() == 1) {
        ASSERT(0, "bt close should user bt_emitter_start_search_device()");
    }
    if (!bt_check_already_initializes()) {
        log_info("[EMITTER_SEARCH_TRACE] search_reject reason=bt_not_initialized");
        return;
    }
    if (emitter_search_state != EMITTER_SEARCH_IDLE) {
        log_info("[EMITTER_SEARCH_TRACE] search_reject reason=busy state=%d",
                 emitter_search_state);
        return;
    }
    u8 inquiry_length = 20;   // inquiry_length * 1.28s
    emitter_search_state = EMITTER_SEARCH_RUNNING;
    bt_cmd_prepare(USER_CTRL_SEARCH_DEVICE, 1, &inquiry_length);
    log_info("[EMITTER_SEARCH_TRACE] search_started state=%d inquiry_len=%d",
             emitter_search_state, inquiry_length);
}
void bt_search_stop(void)
{
    log_info("[EMITTER_SEARCH_TRACE] stop_req state=%d", emitter_search_state);
    if (emitter_search_state == EMITTER_SEARCH_IDLE) {
        log_info("[EMITTER_SEARCH_TRACE] stop_ignore reason=idle");
        return;
    }
    if (emitter_search_state == EMITTER_SEARCH_STOPPING) {
        emitter_search_stop_watchdog_start();
        log_info("[EMITTER_SEARCH_TRACE] stop_ignore reason=stopping");
        return;
    }
    emitter_search_state = EMITTER_SEARCH_STOPPING;
    bt_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL);
    emitter_search_stop_watchdog_start();
    log_info("[EMITTER_SEARCH_TRACE] stop_sent state=%d", emitter_search_state);
}

void emitter_bt_connect(u8 *mac)
{
    if (!mac) {
        log_info("[EMITTER_CONN_TRACE] connect_req mac=NULL");
        return;
    }
    log_info("[EMITTER_CONN_TRACE] connect_req mac=%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    memcpy(emitter_connect_target_addr, mac, sizeof(emitter_connect_target_addr));
    emitter_connect_target_valid = 1;
    /* Keep manual emitter connects in the dual-connection state machine even
     * when BR/EDR has to be opened first.  Otherwise the first request is not
     * recorded as an active PAGE and a second UI click can start an overlapping
     * connection to the same headset. */
    dual_conn_user_bt_connect(mac);
}

void bt_emitter_connect_target_clear(void)
{
    emitter_connect_target_valid = 0;
    memset(emitter_connect_target_addr, 0, sizeof(emitter_connect_target_addr));
}

void bt_emitter_start_search_device()
{
    log_info("[EMITTER_SEARCH_TRACE] start_req role=%d close=%d init=%d",
             g_bt_hdl.emitter_or_receiver, is_bredr_close(),
             bt_check_already_initializes());
    if (g_bt_hdl.emitter_or_receiver != BT_EMITTER_EN) {
        log_info("[EMITTER_SEARCH_TRACE] start_reject reason=not_emitter");
        return ;
    }
    if (is_bredr_close() == 1) {
        log_info("[EMITTER_SEARCH_TRACE] start_path=bredr_reopen");
        bredr_search_device();
    } else {
        log_info("[EMITTER_SEARCH_TRACE] start_path=direct_search");
        /* user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL); */
        /* 关闭可发现 */
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        ////发起扫描
        bt_search_device();
    }
}

/* 扫描是否真正能发起: 需要协议栈已开始初始化(init_start, 由 bt_search_device()
 * 内部 bt_check_already_initializes() 判定)且发射模式已就绪(emitter_or_receiver
 * == BT_EMITTER_EN, 由 bt_emitter_init() 在 BT_STATUS_INIT_OK 时置位)。两者缺一
 * 都会让 bt_emitter_start_search_device()/bt_search_device() 静默返回而不发起查询。 */
int bt_emitter_is_search_ready(void)
{
    return bt_check_already_initializes() &&
           (g_bt_hdl.emitter_or_receiver == BT_EMITTER_EN);
}

#if (SEARCH_LIMITED_MODE == SEARCH_BD_ADDR_LIMITED)
u8 bd_addr_filt[][6] = {
    {0x8E, 0xA7, 0xCA, 0x0A, 0x5E, 0xC8}, /*S10_H*/
    {0xA7, 0xDD, 0x05, 0xDD, 0x1F, 0x00}, /*ST-001*/
    {0xE9, 0x73, 0x13, 0xC0, 0x1F, 0x00}, /*HBS 730*/
    {0x38, 0x7C, 0x78, 0x1C, 0xFC, 0x02}, /*Bluetooth*/
};
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙发射搜索通过地址过滤
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
u8 search_bd_addr_filt(u8 *addr)
{
    u8 i;
    log_info("bd_addr:");
    log_info_hexdump(addr, 6);
    for (i = 0; i < (sizeof(bd_addr_filt) / sizeof(bd_addr_filt[0])); i++) {
        if (memcmp(addr, bd_addr_filt[i], 6) == 0) {
            /* printf("bd_addr match:%d\n", i); */
            return TRUE;
        }
    }
    /*log_info("bd_addr not match\n"); */
    return FALSE;
}
#endif

#if (SEARCH_LIMITED_MODE == SEARCH_BD_NAME_LIMITED)
u8 bd_name_filt[][32] = {
    "TG-294",
    "JL709_VOL_ADAP",
};

u8 bd_spp_name_filt[20][30] = {
    "AC69_BT_SDK",
};
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙发射搜索通过名字过滤
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
u8 search_bd_name_filt(char *data, u8 len, u32 dev_class, char rssi)
{
    char bd_name[64] = {0};
    u8 i;
    char *targe_name = NULL;

    if ((len > (sizeof(bd_name))) || (len == 0) || !data) {
        //printf("bd_name_len error:%d\n", len);
        return FALSE;
    }

    memset(bd_name, 0, sizeof(bd_name));
    memcpy(bd_name, data, len);
    log_info("name:%s,len:%d,class %x ,rssi %d\n", bd_name, len, dev_class, rssi);
    if (search_spp_device) {
        for (i = 0; i < (sizeof(bd_spp_name_filt) / sizeof(bd_spp_name_filt[0])); i++) {
            if (memcmp(data, bd_spp_name_filt[i], len) == 0) {
                puts("\n*****find dev ok******\n");
                return TRUE;
            }
        }
    } else {
        for (i = 0; i < (sizeof(bd_name_filt) / sizeof(bd_name_filt[0])); i++) {
            if (memcmp(data, bd_name_filt[i], len) == 0) {
                puts("\n*****find dev ok******\n");
                return TRUE;
            }
        }
    }
    return FALSE;

}
#endif
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙发射搜索结果回调处理
   @param    name : 设备名字
			 name_len: 设备名字长度
			 addr:   设备地址
			 dev_class: 设备类型
			 rssi:   设备信号强度
   @return   无
   @note
 			蓝牙设备搜索结果，可以做名字/地址过滤，也可以保存搜到的所有设备
 			在选择一个进行连接，获取其他你想要的操作。
 			返回TRUE，表示搜到指定的想要的设备，搜索结束，直接连接当前设备
 			返回FALSE，则继续搜索，直到搜索完成或者超时
*/
/*----------------------------------------------------------------------------*/
u8 emitter_search_result(char *name, u8 name_len, u8 *addr, u32 dev_class, char rssi)
{
    log_info("line:%d name:%s,len:%d,class %x ,rssi %d\n", __LINE__, name, name_len, dev_class, rssi);
    if (name == NULL) {
        struct inquiry_noname_remote *remote = malloc(sizeof(struct inquiry_noname_remote));
        ASSERT(remote);
        remote->match  = 0;
        remote->class = dev_class;
        remote->rssi = rssi;
        memcpy(remote->addr, addr, 6);
        local_irq_disable();
        list_add_tail(&remote->entry, &inquiry_noname_list);
        local_irq_enable();
        if (read_name_start == 0) {
            read_name_start = 1;
            bt_cmd_prepare(USER_CTRL_READ_REMOTE_NAME, 6, addr);
        }
    }

    if (name) {
#if (defined (CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE))
#if (TCFG_UI_ENABLE_BT_EMITTER && TCFG_USER_EMITTER_ENABLE)
        extern void bt_menu_list_add(char *name, u8 * mac, u8 rssi);
        bt_menu_list_add(name, addr, rssi);
#endif
#endif
        log_info("line:%d name:%s,len:%d,class %x ,rssi %d\n", __LINE__, name, name_len, dev_class, rssi);
#if TCFG_VSTREAM_TRANSFER
        kid_bt_search_bd_name_filt(name, name_len, addr, rssi);
#endif
        return FALSE;
    }


#if (SEARCH_LIMITED_MODE == SEARCH_BD_NAME_LIMITED)
    return search_bd_name_filt(name, name_len, dev_class, rssi);
#endif

#if (SEARCH_LIMITED_MODE == SEARCH_BD_ADDR_LIMITED)
    return search_bd_addr_filt(addr);
#endif

#if (SEARCH_LIMITED_MODE == SEARCH_CUSTOM_LIMITED)
    /*以下为搜索结果自定义处理*/
    char bt_name[63] = {0};
    u8 len;
    if (name_len == 0) {
        log_info("No_eir\n");
    } else {
        len = (name_len > 63) ? 63 : name_len;
        /* display bd_name */
        memcpy(bt_name, name, len);
        log_info("name:%s,len:%d,class %x ,rssi %d\n", bt_name, name_len, dev_class, rssi);
    }

    /* display bd_addr */
    log_debug_hexdump(addr, 6);

    /* You can connect the specified bd_addr by below api      */
    /* dual_conn_user_bt_connect(addr); */

    return FALSE;
#endif

#if (SEARCH_LIMITED_MODE == SEARCH_NULL_LIMITED)
    /*没有指定限制，则搜到什么就连接什么*/
    return TRUE;
#endif
    return FALSE;
}
void bt_emitter_init()
{
    if (emitter_search_stop_timer) {
        sys_timeout_del(emitter_search_stop_timer);
        emitter_search_stop_timer = 0;
    }
    emitter_search_state = EMITTER_SEARCH_IDLE;
    /* bt_emitter_start = 1; */
    INIT_LIST_HEAD(&inquiry_noname_list);
    bt_inquiry_result_handle_register(emitter_search_result);
    lmp_set_sniff_establish_by_remote(1);
    bt_emitter_set_enable_flag(1);
    g_bt_hdl.emitter_or_receiver = BT_EMITTER_EN;
    bt_a2dp_source_init(NULL, 0, 1);
#if (TCFG_BT_SUPPORT_HFP_AG==1)
    bt_hfp_ag_buf_init(NULL, 0, 1);
#endif

#if BT_EMITTER_TEST
    bt_emitter_start_search_device();
#endif

    /* 扫描由配对页的 "打开蓝牙搜索设备" 按钮(ble_start_device_search)显式触发,
     * 不再在 BT 初始化时自动扫描, 避免与按钮触发的扫描竞争、以及扫描结果在
     * 用户按下按钮后被 kwt_dev_list_clr() 清空。 */
}

u8 emitter_search_stop_handle(u8 result)
{
    log_info("[EMITTER_SEARCH_TRACE] complete_handle result=%d state=%d",
             result, emitter_search_state);
    struct  inquiry_noname_remote *remote, *n;
    u8 wait_connect_flag = 1;
    if (emitter_search_state == EMITTER_SEARCH_IDLE) {
        log_info("[EMITTER_SEARCH_TRACE] complete_ignore reason=idle");
        return 0;
    }
    if (emitter_search_stop_timer) {
        sys_timeout_del(emitter_search_stop_timer);
        emitter_search_stop_timer = 0;
    }
    emitter_search_state = EMITTER_SEARCH_IDLE;
    log_info("[EMITTER_SEARCH_TRACE] complete_accept new_state=%d",
             emitter_search_state);

    if (!list_empty(&inquiry_noname_list)) {
        bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    }
    list_for_each_entry_safe(remote, n, &inquiry_noname_list, entry) {
        if (!result && remote->match) {
            dual_conn_user_bt_connect(remote->addr);
            wait_connect_flag = 0;
        }
        list_del(&remote->entry);
        free(remote);
    }
    read_name_start = 0;
    if (wait_connect_flag) {
        /* log_info("wait conenct\n"); */
        /* if (bt_get_total_connect_dev() == 2) { */
        /*     write_scan_conn_enable(0, 0); */
        /* } else if (bt_get_total_connect_dev() == 1 && bt_get_connect_status() != BT_STATUS_WAITINT_CONN) { */
        /*     write_scan_conn_enable(0, 1); */
        /* } else { */
        /*     write_scan_conn_enable(1, 1); */
        /* } */
    }
    return 1;
}

static void emitter_search_stop_check(void *priv)
{
    emitter_search_stop_timer = 0;

    if (emitter_search_state != EMITTER_SEARCH_STOPPING) {
        return;
    }
    if (get_link_inquiry_state()) {
        emitter_search_stop_watchdog_start();
        return;
    }

    /* The controller has stopped inquiry but its completion event was lost.
     * Finish the local state transition once, then notify UI and release any
     * deferred phone reconnect just like HCI_EVENT_INQUIRY_COMPLETE does. */
    if (emitter_search_stop_handle(1)) {
        UI_MSG_POST("bt_emitter_status:hci_event=%4",
                    HCI_EVENT_INQUIRY_COMPLETE);
        bt_emitter_search_complete_reconnect_deal();
    }
}

void printf_malloc(void *pive)
{
    int count = 10000;
    /* malloc_list_status_printf_all(count); */
}
u8 bt_emitter_stu_set(u8 *addr, u8 pp)
{
    log_info("total con dev:%d ", bt_get_total_connect_dev());
    if (pp && (bt_get_total_connect_dev() == 0) && !(bt_emitter_get_curr_channel_state() & A2DP_SRC_CH)) {
        pp = 0;
    }
    if (pp) {
        //开音频编码
        app_var.a2dp_source_open_flag = 1;
        /* sys_timer_add(NULL, printf_malloc, 300); */
        bt_emitter_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL);
    } else {
        //关音频编码
        app_var.a2dp_source_open_flag = 0;
    }
    bt_emitter_send_media_toggle(NULL, pp);
    return pp;
}

u8 bt_emitter_pp(u8 pp)
{
    return bt_emitter_stu_set(NULL, pp);
}

void emitter_open(u8 source)
{
    bt_emitter_pp(1);
}

void emitter_close(u8 source)
{
    bt_emitter_pp(0);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙发射接收到设备按键消息
   @param    cmd:按键命令
   @return   无
   @note
 			发射器收到接收器发过来的控制命令处理
 			根据实际需求可以在收到控制命令之后做相应的处理
 			蓝牙库里面定义的是weak函数，直接再定义一个同名可获取信息
*/
/*----------------------------------------------------------------------------*/
void emitter_rx_avctp_opid_deal(u8 cmd, u8 id) //属于库的弱函数重写
{
    log_debug("avctp_rx_cmd:%x\n", cmd);


    switch (cmd) {
    case AVCTP_OPID_NEXT:
        log_info("AVCTP_OPID_NEXT\n");
        app_send_message(APP_MSG_MUSIC_NEXT, 0);
        break;
    case AVCTP_OPID_PREV:
        log_info("AVCTP_OPID_PREV\n");
        app_send_message(APP_MSG_MUSIC_PREV, 0);
        break;
    case AVCTP_OPID_PAUSE:
    case AVCTP_OPID_STOP:
        log_info("AVCTP_OPID_PAUSE\n");
        app_send_message(APP_MSG_BT_EMITTER_PAUSE, 0);
        break;
    case AVCTP_OPID_PLAY:
        log_info("AVCTP_OPID_PP\n");
        app_send_message(APP_MSG_BT_EMITTER_PLAY, 0);
        break;
    case AVCTP_OPID_VOLUME_UP:
        log_info("AVCTP_OPID_VOLUME_UP\n");
        app_send_message(APP_MSG_VOL_UP, 0);
        break;
    case AVCTP_OPID_VOLUME_DOWN:
        log_info("AVCTP_OPID_VOLUME_DOWN\n");
        app_send_message(APP_MSG_VOL_DOWN, 0);
        break;
    default:
        break;
    }
    return ;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙发射接收设备同步音量
   @param    vol:接收到设备同步音量
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void emitter_rx_vol_change(u8 vol) //属于库的弱函数重写
{
    log_info("vol_change:%d \n", vol);
}

////回链手机
u8 connect_last_source_device_from_vm()
{
    u8 mac_addr[6];
    u8 flag = 0;
    flag = restore_remote_device_info_profile((u8 *)&mac_addr, 1, get_remote_dev_info_index(), REMOTE_SOURCE);
    if (flag) {
        //connect last conn
        printf("last source device addr from vm:");
        put_buf(mac_addr, 6);
        dual_conn_user_bt_connect(mac_addr);
    }
    return flag;
}

static int bt_emitter_btstack_event_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        log_info(" BT_STATUS_INIT_OK \n");
        bt_emitter_init();
        break;
    case BT_STATUS_START_CONNECTED:
        log_info("[EMITTER_CONN_TRACE] stack_ev START_CONNECTED ev=0x%x value=0x%x",
                 bt->event, bt->value);
        break;
    case BT_STATUS_ENCRY_COMPLETE:
        log_info("[EMITTER_CONN_TRACE] stack_ev ENCRY_COMPLETE ev=0x%x value=0x%x",
                 bt->event, bt->value);
        break;
    case BT_STATUS_FIRST_CONNECTED:
    case BT_STATUS_SECOND_CONNECTED:
        log_info("[EMITTER_CONN_TRACE] connected ev=0x%x peer=%02x:%02x:%02x:%02x:%02x:%02x target_valid=%d",
                 bt->event, bt->args[0], bt->args[1], bt->args[2],
                 bt->args[3], bt->args[4], bt->args[5], emitter_connect_target_valid);
        if (emitter_connect_target_valid &&
            !memcmp(emitter_connect_target_addr, bt->args, 6)) {
            bt_emitter_connect_target_clear();
            log_info("[EMITTER_CONN_TRACE] target_match post_ui target_connected=1");
            UI_MSG_POST("bt_emitter_status:target_connected=%4", 1);
        } else {
            log_info("[EMITTER_CONN_TRACE] target_mismatch no_ui target=%02x:%02x:%02x:%02x:%02x:%02x",
                     emitter_connect_target_addr[0], emitter_connect_target_addr[1],
                     emitter_connect_target_addr[2], emitter_connect_target_addr[3],
                     emitter_connect_target_addr[4], emitter_connect_target_addr[5]);
        }
        break;
    case BT_STATUS_CONN_A2DP_CH:
        log_info("++++++++ BT_STATUS_CONN_A2DP_CH +++++++++ 0x%x \n", bt->value);
        if (bt->value & A2DP_SRC_CH) {
            emitter_a2dp_source_connected = 1;
            log_info("[EMITTER_CONN_TRACE] a2dp_src_up post_ui a2dp_source=1");
            UI_MSG_POST("bt_emitter_status:a2dp_source=%4", 1);
#if BT_EMITTER_TEST
            bt_emitter_pp(1);
#endif
        } else {
            log_info("[EMITTER_CONN_TRACE] a2dp_not_src no_ui value=0x%x", bt->value);
        }
        break;
    case BT_STATUS_DISCON_A2DP_CH:
        /* BT_STATUS_CONN_A2DP_CH is shared by Sink and Source. */
        if (emitter_a2dp_source_connected &&
            !(bt_emitter_get_curr_channel_state() & A2DP_SRC_CH)) {
            emitter_a2dp_source_connected = 0;
            log_info("[EMITTER_CONN_TRACE] a2dp_src_down post_ui a2dp_source=0");
            UI_MSG_POST("bt_emitter_status:a2dp_source=%4", 0);
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(emitter_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = bt_emitter_btstack_event_handler,
};

static int bt_emitter_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("-----------bt_hci_event_handler reason %x %x", bt->event, bt->value);
    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        if (emitter_search_stop_handle(bt->value)) {
            log_info("[EMITTER_SEARCH_TRACE] complete_post_ui result=%d", bt->value);
            UI_MSG_POST("bt_emitter_status:hci_event=%4", bt->event);
            bt_emitter_search_complete_reconnect_deal();
        } else {
            log_info("[EMITTER_SEARCH_TRACE] complete_drop_ui result=%d", bt->value);
        }
#if TCFG_VSTREAM_TRANSFER
        extern void kwt_conn_handler(void);
        kwt_conn_handler();
#endif
        break;
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info("[EMITTER_CONN_TRACE] hci_conn_complete value=0x%x post_ui hci_value", bt->value);
        UI_MSG_POST("bt_emitter_status:hci_value=%4", bt->value);
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        log_info("[EMITTER_CONN_TRACE] hci_disconn value=0x%x a2dp_src_flag=%d post_ui hci_event",
                 bt->value, emitter_a2dp_source_connected);
        /* Some disconnect paths do not report BT_STATUS_DISCON_A2DP_CH. */
        if (emitter_a2dp_source_connected &&
            !(bt_emitter_get_curr_channel_state() & A2DP_SRC_CH)) {
            emitter_a2dp_source_connected = 0;
            UI_MSG_POST("bt_emitter_status:a2dp_source=%4", 0);
        }
#if !TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
        UI_MSG_POST("bt_emitter_status:hci_event=%4", bt->event);
#else
        UI_MSG_POST("bt_emitter_status:hci_event=%4:hci_value=%4", bt->event, bt->value);
#endif
#if TCFG_VSTREAM_TRANSFER
        extern void kwt_disconn_handler(void);
        kwt_disconn_handler();
#endif
        break;
    }
    return 0;
}

static int bt_emitter_hci_msg_handler(int *msg)
{
    struct bt_event *event = (struct bt_event *)msg;
    bt_emitter_hci_event_handler(event);
    return 0;
}

APP_MSG_HANDLER(emitter_hci_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = bt_emitter_hci_msg_handler,
};

static int bt_emitter_app_msg_handler(int *msg)
{
    struct file_player *file_player = NULL;
    switch (msg[0]) {
    case APP_MSG_BT_EMITTER_PLAY:
#if TCFG_APP_MUSIC_EN
        file_player = get_music_file_player();
        if (music_file_get_player_status(file_player) != FILE_PLAYER_START) {
            app_send_message(APP_MSG_MUSIC_PP, 0);
        }
#endif
        bt_emitter_pp(1);
        break;
    case APP_MSG_BT_EMITTER_PAUSE:
#if TCFG_APP_MUSIC_EN
        file_player = get_music_file_player();
        if (music_file_get_player_status(file_player) == FILE_PLAYER_START) {
            app_send_message(APP_MSG_MUSIC_PP, 0);
        }
#endif
        bt_emitter_pp(0);
        break;
    default:
        break;
    }
    return 0;
}
APP_MSG_HANDLER(bt_emitter_app_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = bt_emitter_app_msg_handler,
};

#endif
