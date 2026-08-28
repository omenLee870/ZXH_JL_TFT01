#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_cmd_recieve.data.bss")
#pragma data_seg(".rcsp_cmd_recieve.data")
#pragma const_seg(".rcsp_cmd_recieve.text.const")
#pragma code_seg(".rcsp_cmd_recieve.text")
#endif
#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_functions/rcsp_config.h"
#include "rcsp.h"
#include "ble_rcsp_server.h"
#include "rcsp_device_status.h"
#include "rcsp_device_feature.h"
#include "rcsp_switch_device.h"
#include "file_transfer.h"
#include "file_delete.h"
#include "file_trans_back.h"
#include "dev_format.h"
#include "rcsp_event.h"
#include "btstack/avctp_user.h"
#include "app_action.h"
#include "rcsp_adv_bluetooth.h"
#include "rcsp_extra_flash_cmd.h"
#include "rcsp_update.h"
#include "fs.h"
#include "file_bluk_trans_prepare.h"
#include "sensors_data_opt.h"
#include "file_simple_transfer.h"
#include "custom_cfg.h"
#include "JL_rcsp_api.h"
#include "rcsp_bt_manage.h"
#include "rcsp_manage.h"
#include "rcsp_command.h"
#include "adv_1t2_setting.h"
#include "rcsp_watch_device_config.h"
#include "rcsp_watch_info.h"


#if TCFG_USER_TWS_ENABLE
#include "classic/tws_api.h"
#include "bt_tws.h"
#endif

#if	(defined CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE)
#include "asm/debug_record.h"
#endif

#include "fs/sdfile.h"

#define LOG_TAG_CONST       RCSP
#define LOG_TAG     		"[rcsp cmd r]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

////<<<<<<<<APP 下发命令响应处理
#if (RCSP_MODE)

/* #define RCSP_DEBUG_EN */
#ifdef  RCSP_DEBUG_EN
#define rcsp_putchar(x)                	log_char(x)
#define rcsp_printf                    	log_info
#define rcsp_put_buf(x,len)				log_info_hexdump(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif

#define ASSET_CMD_DATA_LEN(len, limit) 	\
	do{	\
		if(len >= limit){	\
		}else{				\
			return ;   \
		}\
	}while(0);


static void get_target_feature(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ;
    }

    rcspModel->A_platform = data[4];

    u32 mask = READ_BIG_U32(data);
    u32 wlen = 0;
    u8 *resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    ASSERT(resp, "func = %s, line = %d, no ram!!\n", __FUNCTION__, __LINE__);
    wlen = rcsp_target_feature_parse_packet(priv, resp, TARGET_FEATURE_RESP_BUF_SIZE, mask);
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp, wlen, ble_con_handle, spp_remote_addr);
    free(resp);
}

#if (BT_MIC_EN)
extern int mic_rec_pram_init(u32 enc_type, u8 opus_type, u16(*speech_send)(u8 *buf, u16 len), u16 frame_num, u16 cbuf_size);
extern int ai_mic_rec_start(void);
extern int ai_mic_rec_close(void);
#define AUDIO_CODING_PCM          0x00000100
#define AUDIO_CODING_OPUS         0x00100000
#define AUDIO_CODING_SPEEX        0x00200000

/*speex同步字*/
#define SPEEX_SYNCWORD            0xACBDEAAA

struct app_record_handle {
    u16 record_ble_con_handle;
    u8 *record_spp_remote_addr;
    u32 enc_type;
    u16 frame_size;
    u32 sample_rate;
    u8 *send_buf;
    int send_len;
};

static struct app_record_handle *g_app_rec_hdl = NULL;
static u16 app_record_send_data(u8 *buf, u16 len)
{
    int ret = 0;
    struct app_record_handle *hdl = g_app_rec_hdl;
    u16 syncword_len = 0;
    u32 sync_word = SPEEX_SYNCWORD;
    if (hdl) {
        if (hdl->enc_type == AUDIO_CODING_SPEEX) {
            /*数据格式：
             *0xACBDEAAA+2byte的1帧编码数据的长度+帧0+帧1+帧2+帧3+帧4+0xACBDEAAA+2byte的帧长+帧0+帧1+帧2+帧3+帧4...*/
            syncword_len = 4 + 2;
            /*加同步字*/
            if (!hdl->send_buf) {
                hdl->send_buf = zalloc(len + syncword_len);
                hdl->send_len = len + syncword_len;
            } else {
                if (hdl->send_len < (len + syncword_len)) {
                    free(hdl->send_buf);
                    hdl->send_buf = zalloc(len + syncword_len);
                    hdl->send_len = len + syncword_len;
                }
            }
            /*添加同步字*/
            memcpy(hdl->send_buf, &sync_word, sizeof(sync_word));
            /*添加1帧的帧长*/
            memcpy(hdl->send_buf + sizeof(sync_word), &hdl->frame_size, sizeof(hdl->frame_size));
            /*添加编码数据*/
            memcpy(hdl->send_buf + syncword_len, buf, len);

            buf = hdl->send_buf;
        }
        ret = JL_DATA_send(JL_OPCODE_DATA, JL_OPCODE_APP_RECORDING, buf, len + syncword_len, JL_NOT_NEED_RESPOND, hdl->record_ble_con_handle, hdl->record_spp_remote_addr);
        if (ret) {
            printf("send data err: %d, %d\n", ret, len);
        }
    }
    return ret;
}

static void app_record_satrt(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    printf("func:%s,line:%d", __func__, __LINE__);
    int ret = 0;
    u32 enc_type;
    // 获取编码类型
    switch (data[0]) {
    case 0 :
        enc_type = AUDIO_CODING_PCM;
        break;
    case 1 :
        enc_type = AUDIO_CODING_SPEEX;
        break;
    case 2 :
        enc_type = AUDIO_CODING_OPUS;
        break;
    default:
        goto __exit0;
        break;
    }
    /*
    // 获取采样率
    u32 sample_rate;
    switch (data[1]) {
    case 0x08 :
        sample_rate = 8000;
        break;
    case 0x10 :
        sample_rate = 16000;
        break;
    default:
        goto __exit0;
        break;
    }*/


    struct app_record_handle *hdl = zalloc(sizeof(struct app_record_handle));
    if (!hdl) {
        goto __exit0;
    }
    /*记录发生数据需要的参数*/
    hdl->record_ble_con_handle = ble_con_handle;
    hdl->record_spp_remote_addr = spp_remote_addr;
    hdl->enc_type = enc_type;

    /*注册发数回调*/
    if (enc_type == AUDIO_CODING_PCM) {
        /*pcm数据量比较大，需要一帧发一次，cbuf需要大一点*/
        ret = mic_rec_pram_init(enc_type, 0, app_record_send_data, 1, 1024);
    } else if (enc_type == AUDIO_CODING_SPEEX) {
        /*speex录音和app互交需要每5帧加一个同步字
         *数据格式：
         *0xACBDEAAA+2byte的1帧编码数据的长度+帧0+帧1+帧2+帧3+帧4+0xACBDEAAA+2byte的帧长+帧0+帧1+帧2+帧3+帧4...*/
        hdl->frame_size = 42;
        ret = mic_rec_pram_init(enc_type, 0, app_record_send_data, 5, 1024);
    } else {
        ret = mic_rec_pram_init(enc_type, 0, app_record_send_data, 4, 1024);
    }
    if (ret) {
        goto __exit1;
    }
    /*打开录音*/
    ret = ai_mic_rec_start();
    if (ret) {
        goto __exit1;
    }
    g_app_rec_hdl = hdl;
    /*回复状态*/
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
    return;

__exit1:
    free(hdl);
__exit0:
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
}

static void app_record_stop(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    printf("func:%s,line:%d", __func__, __LINE__);
    struct app_record_handle *hdl = g_app_rec_hdl;
    if (hdl) {
        /*关闭录音*/
        int ret = ai_mic_rec_close();
        if (ret == 0) {
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
        } else {
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
        }
        if (hdl->send_buf) {
            free(hdl->send_buf);
            hdl->send_buf = NULL;
        }
        free(hdl);
        g_app_rec_hdl = NULL;
    }
}
#endif

#if RCSP_DEVICE_STATUS_ENABLE
static void get_sys_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    ASSET_CMD_DATA_LEN(len, 1);
    u8 function = data[0];
    u8 *resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    resp[0] = function;
    u32 rlen = rcsp_device_status_get(priv, function, data + 1, len - 1, resp + 1, TARGET_FEATURE_RESP_BUF_SIZE - 1);
    if (rlen == 0) {
        rcsp_printf("get_sys_info LTV NULL !!!!\n");
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp, (u16)rlen + 1, ble_con_handle, spp_remote_addr);
    free(resp);
}

#if RCSP_ADV_ASSISTED_HEARING
static u8 _hearing_aid_operating_flag = 0;
void set_hearing_aid_operating_flag()
{
    _hearing_aid_operating_flag = 1;
}
#endif // RCSP_ADV_ASSISTED_HEARING

static void set_sys_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    ASSET_CMD_DATA_LEN(len, 1);

    u8 function = data[0];
    bool ret = rcsp_device_status_set(priv, OpCode, OpCode_SN, function, data + 1, len - 1, ble_con_handle, spp_remote_addr);
#if (RCSP_ADV_ASSISTED_HEARING)
    if (_hearing_aid_operating_flag) {
        _hearing_aid_operating_flag = 0;
        return;
    }
#endif
    if (ret == true) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
    }
}
static void function_cmd_handle(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    ASSET_CMD_DATA_LEN(len, 1);

    u8 function = data[0];
    bool ret = rcsp_device_status_cmd_set(priv, OpCode, OpCode_SN, function, data + 1, len - 1, ble_con_handle, spp_remote_addr);
    if (ret == true) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
    }
}
#endif

static void disconnect_edr(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ;
    }
    rcsp_printf("notify disconnect edr\n");
    u8 op = 0;
    if (len) {
        op = data[0];
    }

    switch (op) {
    case 0 :
        if (bt_get_curr_channel_state() != 0) {
            bt_cmd_prepare(USER_CTRL_A2DP_CMD_CLOSE, 0, NULL);
            bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 6, spp_remote_addr);
        }
        break;
    case 1:
        rcsp_msg_post(USER_MSG_RCSP_CONNECT_RESET, 1, (int)priv);
#if (0 == BT_CONNECTION_VERIFY)
        JL_rcsp_reset_bthdl_auth(ble_con_handle, spp_remote_addr);
#endif
        break;
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
}

#if RCSP_FILE_OPT
static void file_bs_start(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    /* rcsp_printf("JL_OPCODE_FILE_BROWSE_REQUEST_START\n"); */
    /* rcsp_put_buf(data, len); */
    if (rcsp_browser_busy() == false) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
        rcsp_browser_start(data, len);
    }
}
#endif

#if RCSP_BT_CONTROL_ENABLE
static void open_bt_scan(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u8 result = 0;
    bool ret = rcsp_msg_post(USER_MSG_RCSP_BT_SCAN_OPEN, 1, (int)priv);
    if (ret == true) {
        result = 1;
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &result, 1, ble_con_handle, spp_remote_addr);
}

static void stop_bt_scan(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u8 result = 0;
    bool ret = rcsp_msg_post(USER_MSG_RCSP_BT_SCAN_STOP, 1, (int)priv);
    if (ret == true) {
        result = 1;
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &result, 1, ble_con_handle, spp_remote_addr);
}

static void connect_bt_spec_addr(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u8 result = 0;
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ;
    }
    if (len != 6) {
        rcsp_printf("connect_bt_spec_addr data len err = %d\n", len);
    }
    memcpy(rcspModel->emitter_con_addr, data, 6);
    bool ret = rcsp_msg_post(USER_MSG_RCSP_BT_CONNECT_SPEC_ADDR, 1, (int)priv);
    if (ret == true) {
        result = 1;
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &result, 1, ble_con_handle, spp_remote_addr);
}
#endif

#if RCSP_ADV_FIND_DEVICE_ENABLE

extern char bt_tws_get_local_channel();
extern int tws_api_get_role(void);
static u8 last_find_device_buf[6] = {0};
void reset_find_device_buf(void)
{
    /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    memset(last_find_device_buf, 0, sizeof(last_find_device_buf));
}
static void find_device_handle(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ;
    }
    if (!rcspModel->find_dev_en) {
        return;
    }
    if (BT_CALL_HANGUP != bt_get_call_status()) {
        return;
    }

    u8 type = data[0]; // 0:查找手机; 1:查找设备; 2:查询状态
    if (type == 0) {
        return;
    } else if (type == 2) {
        /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* log_info_hexdump(last_find_device_buf, sizeof(last_find_device_buf)); */
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, last_find_device_buf, sizeof(last_find_device_buf), ble_con_handle, spp_remote_addr);
        return;
    }

    memcpy(last_find_device_buf, data, len);
    /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    /* log_info_hexdump(last_find_device_buf, sizeof(last_find_device_buf)); */

    u8 opt = data[1]; // 0:关闭铃声; 1:播放铃声
    //cppcheck-suppress unreadVariable
    u8 other_opt[3] = {0};
    /* log_info("rcsp_find %s, %s, %d, type:%d, opt:%d\n", __FILE__, __FUNCTION__, __LINE__, type, opt); */

#if RCSP_MODE == RCSP_MODE_EARPHONE

    if (opt) { // 设置播放铃声播放时间
        u16 timeout = READ_BIG_U16(data + 2);
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            memcpy(other_opt, &timeout, sizeof(timeout));
            extern void find_device_stop_timer(u8 * param, u32 msec);
            /* log_info("rcsp_find %s, %s, %d, timeout:%d\n", __FILE__, __FUNCTION__, __LINE__, timeout); */
            /* log_info_hexdump(other_opt, sizeof(other_opt)); */
            find_device_stop_timer(other_opt, 300);
        } else
#endif
        {
            /* log_info("rcsp_find %s, %s, %d, timeout:%d\n", __FILE__, __FUNCTION__, __LINE__, timeout); */
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_FIND_DEVICE_STOP, (u8 *)&timeout, sizeof(timeout));
        }
    } else {
        // 关闭铃声
        /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        rcsp_stop_find_device(NULL);
    }

#else // RCSP_MODE == RCSP_MODE_EARPHONE

    if (opt) {
        // 播放铃声
        u16 timeout = READ_BIG_U16(data + 2);
        extern void find_device_timeout_handle(u32 sec);
        find_device_timeout_handle(timeout);
    } else {
        // 关闭铃声
        rcsp_stop_find_device(NULL);
#if (RCSP_MODE == RCSP_MODE_WATCH)
        extern void rcsp_find_phone_reset(void);
        rcsp_find_phone_reset();
        if (UI_GET_WINDOW_ID() == ID_WINDOW_FINDPHONE) {
            UI_WINDOW_BACK_SHOW(2);
        }
#endif
    }

#endif // RCSP_MODE == RCSP_MODE_EARPHONE

    /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
#if TCFG_RCSP_DUAL_CONN_ENABLE
    /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    // 一拖二需要通知手机更新状态
    //				 查询状态	关闭铃声	超时时间(不限制)
    u8 send_buf[4] = {0x02, 	0x00, 		0x00, 0x00};
    JL_CMD_send(JL_OPCODE_SYS_FIND_DEVICE, send_buf, sizeof(send_buf), JL_NOT_NEED_RESPOND, 0, NULL);
#endif

#if RCSP_MODE == RCSP_MODE_EARPHONE

    // len为4时没有way和player两种设置
    if (4 == len) {
        /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        return;
    }

    // 铃声播放时才给way和player赋值
    if (6 == len && opt) {
        other_opt[0] = data[4]; // way 0:全部播放; 1:左侧播放; 2:右侧播放
        other_opt[1] = data[5]; // player 0:手机端播放 1:Device端播放
        /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* log_info_hexdump(other_opt, 2); */
    }

    // 300ms超时，发送对端同步执行
#if TCFG_USER_TWS_ENABLE && RCSP_ADV_EN
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        extern void find_device_sync(u8 * param, u32 msec);
        find_device_sync(other_opt, 300);
        return;
    }
#endif
    /* log_info("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_FIND_DEVICE_RESUME, other_opt, sizeof(other_opt));

#endif // RCSP_MODE == RCSP_MODE_EARPHONE

}

#endif // RCSP_ADV_FIND_DEVICE_ENABLE

#define RES_MD5_FILE	SDFILE_RES_ROOT_PATH"md5.bin"
static void get_md5_handle(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ;
    }
    u8 md5[32] = {0};
    FILE *fp = fopen(RES_MD5_FILE, "r");
    if (!fp) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
        return;
    }
    u32 r_len = fread((void *)md5, 32, 1, fp);
    if (r_len != 32) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
        fclose(fp);
        return;
    }
    /* rcsp_printf("get [md5] succ:"); */
    /* rcsp_put_buf(md5, 32); */
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, md5, 32, ble_con_handle, spp_remote_addr);
    fclose(fp);
}

static void get_low_latency_param(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ;
    }
    u8 low_latency_param[6] = {0};
#if RCSP_MODE == RCSP_MODE_EARPHONE
    low_latency_param[0] = 0;
    low_latency_param[1] = 20;
    low_latency_param[2] = 100 >> 24;
    low_latency_param[3] = 100 >> 16;
    low_latency_param[4] = 100 >> 8;
    low_latency_param[5] = 100;
#endif

    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, low_latency_param, 6, ble_con_handle, spp_remote_addr);
}

#if TCFG_RTC_ENABLE
extern bool rcsp_rtc_func_set_ex(void *priv, u8 *data, u16 len);
extern u16 rcsp_rtc_func_get_ex(void *priv, u8 *buf, u16 buf_size, u8 mask);
static void rcsp_alarm_ex(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u16 rlen = 0;
    u8 op = data[0];
    bool ret = 1;
    u8 *resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    resp[0] = op;
    if (op == 0x00) {
        rlen = rcsp_rtc_func_get_ex(priv, resp + 1, TARGET_FEATURE_RESP_BUF_SIZE - 1, data[1]) + 1;
        /* rcsp_put_buf(resp, rlen); */
    } else if (op == 0x01) {
        /* rcsp_put_buf(data, len); */
        ret = rcsp_rtc_func_set_ex(priv, data + 1, len - 1);
        rlen = 1;
    }
    if (ret == true) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp, rlen, ble_con_handle, spp_remote_addr);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, resp, rlen, ble_con_handle, spp_remote_addr);
    }
    free(resp);
}
#endif

#if TCFG_DEV_MANAGER_ENABLE
static void rcsp_device_parm_extra(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 op = data[0];
    switch (op) {
    case 0x0:
        rcsp_file_transfer_download_parm_extra(OpCode_SN, data, len);
        break;;
    case 0x1:
        break;;
    case 0x2:
        break;;
    default:
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_PARAM_ERR, OpCode_SN, 0, 0, 0, NULL);
        break;
    }
}
#endif

#if	(defined CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE)
#define EXCEPTION_DATA_LEN 240//要小于rcsp的mtu
extern u8 check_le_pakcet_sent_finish_flag(void);
extern bool rcsp_send_list_is_empty(void);
static void rcsp_request_exception_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ;
    }
    u8 sub_code = data[0];
    if (sub_code == 0) {
        u8 exception_info_param[6] = {0};
        struct debug_record_info m_debug_record_info;
        user_debug_record_info_get(&m_debug_record_info);
        if (m_debug_record_info.record_len > 0) {
            u32 record_len = m_debug_record_info.record_len;
            record_len = READ_BIG_U32(&record_len);
            memcpy(exception_info_param, &record_len, 4);
            // 计算crc
            JL_ERR err = 0;
            u32 offset = 0;
            //cppcheck-suppress unreadVariable
            u32 len_tmp = 0;
            u16 file_crc = 0;//占用2byte
            while (offset < m_debug_record_info.record_len) {
                len_tmp = (m_debug_record_info.record_len - offset) > EXCEPTION_DATA_LEN ? EXCEPTION_DATA_LEN : (m_debug_record_info.record_len - offset);
                file_crc = CRC16_with_initval(m_debug_record_info.record_buf + offset, len_tmp, file_crc);
                offset += len_tmp;
            }
            file_crc = READ_BIG_U16(&file_crc);
            memcpy(exception_info_param + 4, &file_crc, 2);
            err = JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, exception_info_param, 6, ble_con_handle, spp_remote_addr);
            if (err == 0) {
                // 发送异常信息
                offset = 0;
                len_tmp = 0;
                while (offset < m_debug_record_info.record_len) {
                    len_tmp = (m_debug_record_info.record_len - offset) > EXCEPTION_DATA_LEN ? EXCEPTION_DATA_LEN : (m_debug_record_info.record_len - offset);
                    err = JL_DATA_send(JL_OPCODE_DATA, JL_OPCODE_REQUEST_EXCEPTION_INFO, m_debug_record_info.record_buf + offset, len_tmp, JL_NOT_NEED_RESPOND, ble_con_handle, spp_remote_addr);
                    if (JL_ERR_SEND_BUSY == err) {
                        continue;
                    }
                    offset += len_tmp;
                }

                while (!(rcsp_send_list_is_empty() && check_le_pakcet_sent_finish_flag())) {
                    os_time_dly(10);
                }

                u8 end_param[1] = {1};
                JL_CMD_send(OpCode, end_param, 1, JL_NEED_RESPOND, ble_con_handle, spp_remote_addr);
                return ;
            } else {
                rcsp_printf("%s fail!", __func__);
            }
        } else {
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, exception_info_param, 6, ble_con_handle, spp_remote_addr);
        }
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
    }
}
#endif /* #if (define CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE) */

static void get_watch_device_config(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u32 wlen = 0;
    u8 *resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    ASSERT(resp, "func = %s, line = %d, no ram!!\n", __FUNCTION__, __LINE__);
    wlen = rcsp_watch_get_device_config(priv, resp, TARGET_FEATURE_RESP_BUF_SIZE);
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp, wlen, ble_con_handle, spp_remote_addr);
    free(resp);
}







void rcsp_cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    switch (OpCode) {
    case JL_OPCODE_GET_TARGET_FEATURE:
        get_target_feature(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#if (BT_MIC_EN)
    case JL_OPCODE_APP_RECORDING:
        app_record_satrt(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_APP_RECORD_END:
        app_record_stop(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#endif
    case JL_OPCODE_DISCONNECT_EDR:
        disconnect_edr(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_SWITCH_DEVICE:
        switch_device(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#if RCSP_DEVICE_STATUS_ENABLE
    case JL_OPCODE_SYS_INFO_GET:
        get_sys_info(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_SYS_INFO_SET:
        set_sys_info(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_FUNCTION_CMD:
        function_cmd_handle(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#endif
#if RCSP_FILE_OPT
    case JL_OPCODE_FILE_BROWSE_REQUEST_START:
        file_bs_start(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#endif
#if RCSP_BT_CONTROL_ENABLE
    case JL_OPCODE_SYS_OPEN_BT_SCAN:
        open_bt_scan(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_SYS_STOP_BT_SCAN:
        stop_bt_scan(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_SYS_BT_CONNECT_SPEC:
        connect_bt_spec_addr(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#endif
#if RCSP_ADV_FIND_DEVICE_ENABLE
    case JL_OPCODE_SYS_FIND_DEVICE:
        find_device_handle(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#endif
    case JL_OPCODE_GET_MD5:
        get_md5_handle(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_LOW_LATENCY_PARAM:
        get_low_latency_param(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_CUSTOMER_USER:
        rcsp_user_cmd_recieve(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_ACTION_PREPARE:
        app_rcsp_task_prepare(1, data[0], OpCode_SN);
        break;
#if TCFG_DEV_MANAGER_ENABLE
    case JL_OPCODE_FILE_TRANSFER_START:
        rcsp_file_transfer_download_start(priv, OpCode_SN, data, len);
        break;
    case JL_OPCODE_FILE_TRANSFER_CANCEL:
        rcsp_file_transfer_download_passive_cancel(OpCode_SN, data, len);
        break;
    case JL_OPCODE_FILE_DELETE:
        rcsp_file_delete_start(OpCode_SN, data, len);
        break;
    case JL_OPCODE_DEVICE_FORMAT:
        rcsp_dev_format_start(OpCode_SN, data, len);
        break;
    case JL_OPCODE_ONE_FILE_DELETE:
        rcsp_file_delete_one_file(OpCode_SN, data, len);
        break;
    case JL_OPCODE_ONE_FILE_TRANS_BACK:
        rcsp_file_trans_back_opt(priv, OpCode_SN, data, len);
        break;
    case JL_OPCODE_FILE_BLUK_TRANSFER:
        rcsp_file_bluk_trans_prepare(priv, OpCode_SN, data, len);
        break;
    case JL_OPCODE_DEVICE_PARM_EXTRA:
        rcsp_device_parm_extra(priv, OpCode, OpCode_SN, data, len);
        break;
#endif
#if (TCFG_DEV_MANAGER_ENABLE && JL_RCSP_SIMPLE_TRANSFER)
    case JL_OPCODE_SIMPLE_FILE_TRANS:
        rcsp_file_simple_transfer_for_small_file(priv, OpCode_SN, data, len);
        break;
#endif
#if TCFG_RTC_ENABLE
    case JL_OPCODE_ALARM_EXTRA:
        rcsp_alarm_ex(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#endif
#if	(defined CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE)
    case JL_OPCODE_REQUEST_EXCEPTION_INFO:
        rcsp_request_exception_info(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#endif
    case JL_OPTCODE_DEVICE_CONFIG_GET:
        get_watch_device_config(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
    case JL_OPCODE_MASS_DATA:
        extern void mass_data_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 * data, u16 len);
        mass_data_recieve(priv, OpCode, OpCode_SN, data, len);
        break;
#if TCFG_AI_INTERACTION_ENABLE
    case JL_OPCODE_AI_DIAL:
        extern void put_aidial(void *priv, u8 OpCode, u8 OpCode_SN, u8 * data, u16 len);
        put_aidial(priv, OpCode, OpCode_SN, data, len);
        break;
#endif

    case JL_OPCODE_COMMON_INFO_SET:
        rcsp_common_info_set_cmd_deal(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;

#if TCFG_RCSP_DUAL_CONN_ENABLE
    case JL_OPCODE_1T2_DEVICE_EDR_INFO_LIST:
        rcsp_get_1t2_bt_device_name_list(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        break;
#endif

    case JL_OPCODE_STREAM_CONTROL:
        extern void rcsp_jlstm_cmd_deal(u8 OpCode_SN, u8 * data, u16 len);
        rcsp_jlstm_cmd_deal(OpCode_SN, data, len);
        break;

    default:
#if JL_RCSP_SENSORS_DATA_OPT
        if (0 == JL_rcsp_sensors_data_opt(priv, OpCode, OpCode_SN, data, len)) {
            break;
        }
#endif
#if JL_RCSP_EXTRA_FLASH_OPT
        if (0 == JL_rcsp_extra_flash_cmd_resp(priv, OpCode, OpCode_SN, data, len)) {
            break;
        }
#endif
#if RCSP_ADV_EN
        if (0 == JL_rcsp_adv_cmd_resp(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr)) {
            break;
        }
#elif (defined TCFG_CSC_BT_APP) && TCFG_CSC_BT_APP
        if (0 == JL_rcsp_adv_cmd_resp(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr)) {
            break;
        }
#endif
#if RCSP_UPDATE_EN
        if (0 == JL_rcsp_update_cmd_resp(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr)) {
            break;
        }
#endif
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_UNKOWN_CMD, OpCode_SN, 0, 0, ble_con_handle, spp_remote_addr);
        break;
    }
}

#endif//RCSP_MODE
