#include "app_config.h"
#include "ble_user.h"
#include "btstack/le/att.h"
#include "events_adapter.h"
#include "syscfg_id.h"
#include "ble_rcsp_multi_client.h"
#include "smartbox_info_manager.h"
#include "chgbox_box.h"
#include "rcsp_cfg.h"
#include "system/timer.h"

#if CONFIG_APP_UI_ENABLE && CONFIG_WATCH_CASE_ENABLE
#include "ui_api.h"
#include "ui_sys_param.h"
#include "third_party/screen_trans_lib/screen_ear_interface.h"
#endif

#define LOG_TAG_CONST       EARPHONE_PROT
#define LOG_TAG     		"[EARPHONE_PROT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "utils/debug.h"

#if TCFG_EARPHONE_PROTOCOL
#if (( BT_AI_SEL_PROTOCOL & RCSP_MODE_EN ) && RCSP_BLE_CLIENT_EN)

extern void sbox_emitter_addr_set(void);

/**********************************************************
 *      仓与耳机的协议控制
 *********************************************************/


struct custom_music_info local_music_info;
char lyrics_content_array[3][STR_MAX_SIZE] = {"\n", "\n", "\n"};
char lyrics_artist_name_array[STR_MAX_SIZE] = "\n"; /*jlui框架使用不显示内容 使用'\n'*/

//播放模式用于让耳机关机
void custom_client_send_poweroff_earphone(void)
{
    u8 send_data = 1;
    custom_ble_client_write_no_respond(CUSTOM_BLE_PLAY_MODE_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}

//播放闹钟
//0x12:播放闹钟 0x03:关闭闹钟
void custom_client_send_alarm_earphone(u8 cmd)
{
    u8 send_data = cmd;
    custom_ble_client_write_no_respond(CUSTOM_BLE_ALARM_CLOCK_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}

//查找耳机
/*  0x10 查找L
    0x01 查找R
    0x11 查找LR都查找
    0x03 查找关闭
    0x12 闹钟响起
    */
void custom_client_send_find_earphone(u8 cmd)
{
    u8 send_data = cmd; // vol up
    custom_ble_client_write_no_respond(CUSTOM_BLE_FINE_EARPHONE_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}

//音量控制
void custom_client_send_volume_up(void)
{
    u8 send_data = 0x01; // vol up
    custom_ble_client_write_no_respond(CUSTOM_BLE_VOL_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}
void custom_client_send_volume_down(void)
{
    u8 send_data = 0x02; // vol down
    custom_ble_client_write_no_respond(CUSTOM_BLE_VOL_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}

//设置音乐状态
void custom_client_send_music_ctrl(u8 state)
{
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, state);
    custom_ble_client_write_no_respond(CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD, &state, sizeof(state));
}
void custom_client_send_music_play(void)
{
    u8 send_data = 0x01; // play
    /* status_tick = lv_tick_get(); */
    custom_ble_client_write_no_respond(CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}
void custom_client_send_music_pause(void)
{
    u8 send_data = 0x02; // pause
    /* status_tick = lv_tick_get(); */
    custom_ble_client_write_no_respond(CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}
void custom_client_send_music_prev(void)
{
    u8 send_data = 0x03; // prev
    custom_ble_client_write_no_respond(CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}
void custom_client_send_music_next(void)
{
    u8 send_data = 0x04; // next
    custom_ble_client_write_no_respond(CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}


//设置anc
void custom_client_send_anc_mode(u8 mode)
{
    custom_ble_client_write_no_respond(CUSTOM_BLE_ANC_MODE_CONTROL_CMD, &mode, sizeof(mode));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, mode);
}

void custom_client_send_eq_mode(u8 val)
{
    u8 send_data = val;
    custom_ble_client_write_no_respond(CUSTOM_BLE_EQ_MODE_CONTROL_CMD, &send_data, sizeof(send_data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, send_data);
}

//设置语言

// void custom_client_send_language(u8 data)
// {
//     u8 send_data = data +1; // 关闭
//     custom_ble_client_write_no_respond(CUSTOM_BLE_SWITCH_LANGUAGE, &send_data, sizeof(send_data));
// }


//设置接听挂断 1接听 2挂断 3mute 4mute off
void custom_client_send_call(u8 data)
{
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, data);
    custom_ble_client_write_no_respond(CUSTOM_BLE_CONTRAL_CALL, &data, sizeof(data));
}

void custom_client_send_ble_settings(u8 data)
{

    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, data);
    custom_ble_client_write_no_respond(CUSTOM_BLE_VOL_CONTROL_CMD, &data, sizeof(data));
}
//设置耳机BLE进入 no lantacy发送  0xf1 设置 0xf0退出
void custom_client_send_ble_setting(u8 data)
{
    int msg[3];
    int err = 0;

    msg[0] = (int)custom_client_send_ble_settings;
    msg[1] = 1;
    msg[2] = (int)data;

    err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
    if (err) {
        log_info("custom_client_send_ble_setting post fail\n");
    }
}


void custom_client_send_box_all_info(void)
{
    u8 data = sbox_local_page_get();
    custom_ble_client_write_no_respond(CUSTOM_ALL_INFO_CMD, &data, sizeof(data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, data);
}
//0:上滑 1：下滑 2：左滑 3：右滑 4：点赞
void custom_client_send_ctrl_tiktop(u8 cmd)
{
    u8 data = cmd;
    custom_ble_client_write_no_respond(CUSTOM_BLE_CONTRAL_DOUYIN, &data, sizeof(data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, data);
}

void custom_client_send_ctrl_photo(u8 cmd)
{
    u8 data = cmd;
    custom_ble_client_write_no_respond(CUSTOM_BLE_CONTRAL_PHOTO, &data, sizeof(data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, data);
}

void custom_client_send_ctrl_edr_conn(u8 cmd)
{
    u8 data = cmd;
    custom_ble_client_write_no_respond(CUSTOM_EDR_CONTRAL_CONN, &data, sizeof(data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, data);
}

void custom_client_send_ctrl_edr_info(void)
{
    custom_ble_client_write_no_respond(CUSTOM_EDR_SYNC_INFO, sbox_emitter_addr_get(), sizeof(custom_edr_info));
    log_info("%s %d \n", __FUNCTION__, __LINE__);
}
static u8 sleep_control_status = 0;
u8 get_sleep_control_status(void)
{
    return sleep_control_status;
}
void set_sleep_control_status(u8 en)
{
    sleep_control_status = en;
}

void custom_client_sleep_control(u8 on)
{
    u8 data = on;
    custom_ble_client_write_no_respond(CUSTOM_SLEEP_CTRL_CMD, &data, sizeof(data));
    log_info("%s %d send_data:%d\n", __FUNCTION__, __LINE__, data);
}

/*
    让耳机呼出电话，电话号码长度>3 && <30 且为字符串格式输入
*/
void custom_client_send_ctrl_phoneout(u8 *buf)
{
    log_info("%s %d \n", __FUNCTION__, __LINE__);
    custom_ble_client_write_no_respond(CUSTOM_BLE_CONTRAL_PHONEOUT, buf, strlen((const char *)buf));
}

/*
    设置耳机SIRI功能 1byte   0:开启  1：关闭
*/
void custom_client_send_ctrl_siri(u8 siri_status)
{
    log_info("%s %d \n", __FUNCTION__, __LINE__);
    custom_ble_client_write_no_respond(CUSTOM_EDR_SIRI_CTRL, &siri_status, 1);
}

/*设置耳机按键功能 8byte
L:单击 双击 三击 长按
R:单击 双击 三击 长按
enum {
    SBOX_NO_FUNTION = 0,      //(⽆功能)
    SBOX_PLAY_PAUSE,          //播放/暂停
    SBOX_VOL_UP,              //音量加
    SBOX_VOL_DOWN,            //音量减
    SBOX_MUSIC_PREV,          //上⼀曲
    SBOX_MUSIC_NEXT,          //下⼀曲
    SBOX_EQ_SWITCH,           //切换EQ
    SBOX_VOICE_ASSISTANT,     //语⾳助⼿
    SBOX_LOW_LATENCY,         //低延时模式(普通模式/低延迟模式)
    SBOX_ANC_MODE,            //anc模式(正常模式/降噪模式/通透模式)
    SBOX_FUN_MAX_ = 0xFF,
};///手势功能枚举
*/
void custom_client_send_ctrl_key(u8 *my_set)
{
    log_info("%s %d \n", __FUNCTION__, __LINE__);
    custom_ble_client_write_no_respond(CUSTOM_BLE_CONTRAL_KEY, my_set, strlen((const char *)my_set));
}

/*设置耳机EQ信息功能(10段EQ) 11byte
    mode+gain
    struct eq_seg_info eq_tab_custom[] = {
#if TCFG_USER_EQ_MODE_NUM > 6
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0, AUDIO_EQ_Q},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0, AUDIO_EQ_Q},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0, AUDIO_EQ_Q},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0, AUDIO_EQ_Q},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0, AUDIO_EQ_Q},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0, AUDIO_EQ_Q},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0, AUDIO_EQ_Q},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0, AUDIO_EQ_Q},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0, AUDIO_EQ_Q},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0, AUDIO_EQ_Q},
}
*/
void custom_client_send_ctrl_eqinfo(u8 mode, u8 *eq_gain)
{
    log_info("%s %d \n", __FUNCTION__, __LINE__);
    u8 process_info[11];
    memcpy(process_info, &mode, 1);
    memcpy(process_info + 1, eq_gain, 10);
    custom_ble_client_write_no_respond(CUSTOM_BLE_EQ_MODE_CONTROL_CMD, process_info, sizeof(process_info));
}

void ble_connect_snyc_info(void *p)
{
    log_info("%s line:%d\n", __func__, __LINE__);
    sbox_emitter_addr_set();

    custom_client_send_ctrl_edr_info();
    if (get_sleep_control_status()) {
        custom_client_sleep_control(1);
    }
}

u8 *get_phone_number_from_call_info(u8 *buf, u8 *data, u8 len)
{
    log_info("func: %s, line: %d", __func__, __LINE__);
    if (!buf || !data) {
        log_error("[ERROR] buf or data is null");
        return NULL;
    }

    if (len < 0 || len > 30) {
        log_error("[ERROR] len is invalid");
        return NULL;
    }

    u8 *ptr = buf;

    for (int i = 1; i < len; i++) {
        if (data[i] >= '0' && data[i] <= '9') {
            *ptr++ = data[i];
        }

    }
    *ptr = '\0';
    log_info("phone number: %s", buf);

    return buf;
}

static char album_name_info[256] = {0};
static char lyrics_cnt = 0;
int ble_recv_musicdata_handler(uint8_t *buffer, uint16_t buffer_size)
{
    if (buffer[0] == 0xee && buffer[1] == 0xbb && buffer[4] == 0x2 && buffer[6] == 0x3) {
        printf("<%s> lyrics_artist_name_array change", __func__);
        int offset = 10;
        struct custom_music_info *local_music_infos = &local_music_info;
        memset(local_music_infos->artist_name, 0, local_music_infos->name_len);
        memset(local_music_infos->album_name, 0, local_music_infos->album_len);
        memset(local_music_infos->title, 0, local_music_infos->title_len);
        log_info("ble_recv_musicdata_handler\n");
        local_music_infos->type_artist_name = buffer[4];
        local_music_infos->name_len = buffer[5];
        local_music_infos->type_album_name = buffer[6];
        local_music_infos->album_len = buffer[7];
        local_music_infos->type_title = buffer[8];
        local_music_infos->title_len = buffer[9];

        memcpy(local_music_infos->artist_name, buffer + offset, local_music_infos->name_len);
        offset += local_music_infos->name_len;
        memcpy(local_music_infos->album_name, buffer + offset, local_music_infos->album_len);
        offset += local_music_infos->album_len;
        memcpy(local_music_infos->title, buffer + offset, local_music_infos->title_len);
        /* offset += local_music_infos->title_len; */
// 将lyrics_artist_name_array数组全部置为0
        memset(lyrics_artist_name_array, 0, sizeof(lyrics_artist_name_array));
// 将local_music_infos->artist_name的内容复制到lyrics_artist_name_array数组中
        memcpy(lyrics_artist_name_array, local_music_infos->artist_name, local_music_infos->name_len);
// 如果album_name_info和lyrics_artist_name_array不相等，或者album_name_info的长度为0
        if ((memcmp(album_name_info, lyrics_artist_name_array, sizeof(lyrics_artist_name_array)) != 0) || (strlen(album_name_info) == 0)) {
            // 将album_name_info数组全部置为0
            memset(album_name_info, 0, sizeof(album_name_info));
            // 将local_music_infos->artist_name的内容复制到album_name_info数组中
            memcpy(album_name_info, local_music_infos->artist_name, local_music_infos->name_len);
            // 将lyrics_cnt置为0
            lyrics_cnt = 0;
            // 将lyrics_content_array数组全部置为0
            memset(lyrics_content_array, 0, sizeof(lyrics_content_array));
            // 将local_music_infos->title的内容复制到lyrics_content_array数组的第一个元素中
            memcpy(lyrics_content_array[0], local_music_infos->title, local_music_infos->title_len);
            // 将lyrics_cnt置为1
            lyrics_cnt = 1;
            // log_info("album_name_info:%s\n", album_name_info);
        } else {
            // 如果lyrics_cnt小于LYRICS_MAX_NUM - 1
            if (lyrics_cnt < LYRICS_MAX_NUM - 1) {
                // 将local_music_infos->title的内容复制到lyrics_content_array数组的第lyrics_cnt个元素中
                memcpy(lyrics_content_array[lyrics_cnt], local_music_infos->title, local_music_infos->title_len);
            }
            // 如果lyrics_cnt大于等于LYRICS_MAX_NUM - 1
            else if (lyrics_cnt >= LYRICS_MAX_NUM - 1) {
                // 定义一个临时数组lyrics_content_array_temp
                char lyrics_content_array_temp[LYRICS_MAX_NUM - 1][256] = {0};

                // 将lyrics_cnt置为LYRICS_MAX_NUM - 1
                lyrics_cnt = (LYRICS_MAX_NUM - 1);
                // 如果lyrics_content_array数组的最后一个元素为空
                if (strlen(lyrics_content_array[lyrics_cnt]) == 0) {
                    // 将local_music_infos->title的内容复制到lyrics_content_array数组的最后一个元素中
                    memcpy(lyrics_content_array[lyrics_cnt], local_music_infos->title, local_music_infos->title_len);
                    // log_info("1_lyrics_content_array%s", lyrics_content_array[lyrics_cnt]);
                } else {
                    // 将lyrics_content_array数组中的元素依次向后移动一位
                    for (u8 i = 0; i < lyrics_cnt; i++) {
                        // 将lyrics_content_array数组的第i+1个元素的内容复制到临时数组lyrics_content_array_temp的第i个元素中
                        memcpy(lyrics_content_array_temp[i], lyrics_content_array[i + 1], sizeof(lyrics_content_array_temp[0]));
                        // 将lyrics_content_array数组的第i个元素置为0
                        memset(lyrics_content_array[i], 0, sizeof(lyrics_content_array[0]));
                        memcpy(lyrics_content_array[i], lyrics_content_array_temp[i], sizeof(lyrics_content_array_temp[0]));
                        // log_info("lyrics_content_array_temp[%d]%s", i, lyrics_content_array_temp[i]);
                    }
                    memset(lyrics_content_array[lyrics_cnt], 0, sizeof(lyrics_content_array[0]));
                    memcpy(lyrics_content_array[lyrics_cnt], local_music_infos->title, local_music_infos->title_len);
                    // log_info("2_lyrics_content_array%s", lyrics_content_array[lyrics_cnt]);
                }
            }
            lyrics_cnt++;

        }

        b_printf("\r\nlyrics_content_array: %s\r\nlyrics_artist_name_array: %s", lyrics_content_array[0], lyrics_artist_name_array);

        // log_info("ble_recv_musicdata_handler\n");
        UI_MSG_POST("up_music_title:p=%4", local_music_infos->title);
        log_info("artist_name:%s len:%d\n", local_music_infos->artist_name, local_music_infos->name_len);
        log_info("album_name:%s len:%d\n", local_music_infos->album_name, local_music_infos->album_len);
        log_info("title:%s len:%d\n", local_music_infos->title, local_music_infos->title_len);

#if 0
        str_queue_in(lyrics_queue, local_music_infos->title);
        str_queue_printf();

        extern void lv_lyrics_update_content();

        int msg[2] = {0};
        msg[0] = (int)lv_lyrics_update_content;
        msg[1] = 1;
        do {
            int os_err = os_taskq_post_type("ui", Q_CALLBACK, 2, msg);
            if (os_err) {
                printf("lv_lyrics_update_content fail, err type: %d", os_err);
            }
        } while (0);
#endif
        /* extern void music_text_update_timer(void *p); */
        /* music_text_update_timer(NULL); */
        return 1;
    }
    return 0;
}




/**********************************************************
 *      主机扫描、状态控制
 *********************************************************/

static void  sbox_power_on_page_check_cb(void *p)
{
    log_info("%s %d %d", __func__, ble_client_get_cur_work_state(), sbox_box_clid_status_get());
    if ((ble_client_get_cur_work_state() == BLE_ST_SCAN) && (sbox_box_clid_status_get() == LID_CLOSE)) {
        ble_client_module_enable(0);
    }
}

/*开关盖状态处理*/
void sbox_state_handler(int chgbox_event)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_info("%s rets:%x [%d 0x%x %d]", __func__, rets, chgbox_event, ble_client_get_cur_work_state(), sbox_box_clid_status_get());

    switch (chgbox_event) {
    case CHGBOX_EVENT_OPEN_LID:
        sbox_box_clid_status_set(LID_OPEN);
        if (ble_client_get_cur_work_state() == BLE_ST_NULL) {
            log_warn("ble multi client not initialized");
            break;
        }
        if (ble_client_get_cur_work_state() != BLE_ST_CONNECT) {
            ble_client_module_enable(1);
        }
        break;

    case CHGBOX_EVENT_CLOSE_LID:
        sbox_box_clid_status_set(LID_CLOSE);
        if (ble_client_get_cur_work_state() == BLE_ST_NULL) {
            log_warn("ble multi client not initialized");
            return;
        }

        ble_client_module_enable(0);
        log_info("l_inbox:%d r_inbox:%d", sbox_l_inbox_get(), sbox_r_inbox_get());
        if ((sbox_l_inbox_get() == IN_BOX) && (sbox_r_inbox_get() == IN_BOX)) {
            ble_multi_client_disconnect();
        }
        break;

    case CHGBOX_EVENT_EAR_L_ONLINE:
        ear_inbox_state_deal(1);
        /*避免开机时候，状态识别太晚*/
        if ((sbox_box_clid_status_get() == LID_CLOSE) && (sbox_r_inbox_get() == IN_BOX)) {
            ble_client_module_enable(0);
        }

        break;
    case CHGBOX_EVENT_EAR_L_OFFLINE:
        ear_inbox_state_deal(3);
        break;
    case CHGBOX_EVENT_EAR_R_ONLINE:
        ear_inbox_state_deal(2);
        /*避免开机时候，状态识别太晚*/
        if ((sbox_box_clid_status_get() == LID_CLOSE) && (sbox_l_inbox_get() == IN_BOX)) {
            ble_client_module_enable(0);
        }
        break;
    case CHGBOX_EVENT_EAR_R_OFFLINE:
        ear_inbox_state_deal(4);
        break;
    default:
        break;
    }
}


/*开机过程的检测*/
void sbox_power_on_check(void)
{
    u8 lid_status = sbox_box_clid_status_get();
    log_info("%s %d %d %d", __func__, lid_status, sbox_l_inbox_get(), sbox_r_inbox_get());
    switch (lid_status) {
    case  LID_OPEN:
        ble_client_module_enable(1);
        break;
    case LID_CLOSE:
        if ((sbox_l_inbox_get() == OUT_OF_BOX) || (sbox_r_inbox_get() == OUT_OF_BOX)) {
            ble_client_module_enable(1);
            sys_timeout_add(0, sbox_power_on_page_check_cb, TCFG_BLE_POWERON_PAGE_TIME); //关盖时候,设定时间还连接不上就停止扫描
        }
        break;
    default:
        break;
    }
}


/*读取耳机mac地址*/
void sbox_get_earphone_mac(u8 *addr)
{
    u8 addr_tmp[6] = {1, 2, 3, 4, 5, 6};
    if (!addr) {
        log_error("%s addr is NULL!", __func__);
        return;
    }

    log_info("%s:\n", __func__);
    int ret = syscfg_read(CFG_CHGBOX_ADDR, addr_tmp, 6);
    if (ret == 6) {
        memcpy(addr, addr_tmp, 6);
    } else {
        log_error("%s not initialized!", __func__);
    }
    log_info_hexdump(addr_tmp, 6);
}

/*更新主机要去连接的远端地址*/
void sbox_update_earphone_mac(void)
{
    u8 addr_tmp[6] = {1, 2, 3, 4, 5, 6};
    log_info("%s:\n", __func__);

    int ret = syscfg_read(CFG_CHGBOX_ADDR, addr_tmp, 6);
    if (ret == 6) {
        ble_client_set_remote_addr(addr_tmp);
    } else {
        log_error("%s not initialized!", __func__);
    }
    log_info_hexdump(addr_tmp, 6);
}

#endif //#if (( BT_AI_SEL_PROTOCOL & RCSP_MODE_EN ) && RCSP_BLE_CLIENT_EN)
#endif //#if TCFG_EARPHONE_PROTOCOL

