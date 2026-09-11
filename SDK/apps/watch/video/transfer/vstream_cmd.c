#include "app_config.h"
#include "typedef.h"
#include "os/os_api.h"
#include "task.h"
#include "lbuf.h"
#include "crc.h"
#include "vstream_common.h"
#include "vstream_cmd.h"
#include "transfer_stubs.h"

/* #define LOG_TAG_CONST       UI */
#define LOG_TAG             "[KWT_CMD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_WARN_ENABLE
#undef LOG_WARN_ENABLE
#define LOG_WARN_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#include "kid_mem_dbg.h"

/* #define log_error printf */
/* #define log_warn printf — use debug.h LOG_WARN_ENABLE instead */

static u8 vstream_sn = 0;

#define VSTREAM_CMD_DATA_CRC_CHECK 	1//命令包数据校验
struct vstream_cmd_status {
    u8 video_status;	//对讲中
    u8 voice_status;	//语音中
    u8 voice_effect;	//远端音效
    u8 video_cmd_sn;	//sn
    u8 video_break_status;//中断
};//远端状态值缓存
static struct vstream_cmd_status cmd_status;
#define __this (&cmd_status)

static int vstream_cmd_send_with_adk(u8 cmd, u8 *data, int data_len);
int vstream_cmd_send_recv(u8 cmd, u8 cmd_sn, u8 *data, u8 data_len);
//=================================================================//
//				获取状态接口api
//=================================================================//
int vstream_cmd_video_status_get()
{
    return __this->video_status;
}
int vstream_cmd_voice_status_get()
{
    return __this->voice_status;
}
int vstream_cmd_voice_effect_get()
{
    return __this->voice_effect;
}
int vstream_cmd_video_break_get()
{
    return __this->video_break_status;
}
int vstream_cmd_video_status_set(u8 status)
{
    __this->video_status = status;
    log_debug("%s status:%d\n", __func__, status);
    return __this->video_status;
}
int vstream_cmd_voice_status_set(u8 status)
{
    __this->voice_status = status;
    log_debug("%s status:%d\n", __func__, status);
    return __this->voice_status;
}
int vstream_cmd_voice_effect_set(u8 effect)
{
    __this->voice_effect = effect;
    log_debug("%s effect:%d\n", __func__, effect);
    return __this->voice_effect;
}
int vstream_cmd_video_break_set(u8 status)
{
    __this->video_break_status = status;
    log_debug("%s status:%d\n", __func__, status);
    return __this->video_break_status;
}
//=================================================================//
//				发送命令接口api
//=================================================================//
int vstream_cmd_send_video_start()
{
    vstream_cmd_video_break_set(0);
    return vstream_cmd_send_with_adk(VSTREAM_CMD_VIDEO_START, NULL, 0);
}
int vstream_cmd_send_video_stop()
{
    vstream_cmd_video_break_set(0);
    vstream_cmd_video_status_set(0);
    return vstream_cmd_send_with_adk(VSTREAM_CMD_VIDEO_STOP, NULL, 0);
}
int vstream_cmd_send_voice_start()
{
    u8 voice_effect = walkie_talkie_get_effect();
    return vstream_cmd_send_with_adk(VSTREAM_CMD_VOICE_START, &voice_effect, 1);
}
int vstream_cmd_send_voice_stop()
{
    return vstream_cmd_send_with_adk(VSTREAM_CMD_VOICE_STOP, NULL, 0);
}
int vstream_cmd_send_video_reconnect()
{
    return vstream_cmd_send_with_adk(VSTREAM_CMD_VIDEO_RECONN, NULL, 0);
}
//=================================================================//
//				应答命令接口api
//=================================================================//

int vstream_cmd_answer_video_ctrl(u8 ctrl)
{
    if (ctrl == VSTREAM_CMD_CONNENT) {
        // B端接受通话时立即标记已连接，防止后续VIDEO_START重置UI
        vstream_cmd_video_status_set(1);
    }
    int ret = vstream_cmd_send_recv(VSTREAM_CMD_VIDEO_START, __this->video_cmd_sn, &ctrl, 1);
    __this->video_cmd_sn = 0;
    return ret;
}
static int vstream_cmd_ask_video_start(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    log_info("%s cmd:0x%x sn:%d \n", __func__, cmd, cmd_sn);
    int ret = VSTREAM_CMD_NO_ERR;
    // 已处于通话中，忽略新的来电请求
    if (vstream_cmd_video_status_get()) {
        return ret;
    }
    __this->video_cmd_sn = cmd_sn;
    vstream_cmd_video_break_set(0);
    walkie_talkie_talk_sel_show();
    return ret;
}
static int vstream_cmd_ask_video_stop(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;

    vstream_cmd_video_break_set(0);
    walkie_talkie_hangup_show();
    return ret;
}
static int vstream_cmd_ask_voice_start(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;

    //配置解码音效
    vstream_cmd_voice_effect_set(data[0]);
    //配置语音状态
    vstream_cmd_voice_status_set(1);
    //显示语音logo
    walkie_talkie_talk_voice_pic_show();
    return ret;
}
static int vstream_cmd_ask_voice_stop(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;
    //清除语音状态
    vstream_cmd_voice_status_set(0);
    //隐藏语音logo
    walkie_talkie_talk_voice_pic_hide();
    return ret;
}
static int vstream_cmd_ask_video_reconnect(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    printf("%s cmd:0x%x sn:%d \n", __func__, cmd, cmd_sn);
    int ret = VSTREAM_CMD_NO_ERR;
    vstream_cmd_video_status_set(1);
    vstream_cmd_video_break_set(0);
    vstream_cmd_send_recv(cmd, cmd_sn, NULL, 0);
    return ret;
}
//默认应答
static int vstream_cmd_answer_default(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    return vstream_cmd_send_recv(cmd, cmd_sn, NULL, 0);
}
//=================================================================//
//				回复包处理接口api
//=================================================================//
static int vstream_cmd_recv_video_start(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;
    if (data && len) {
        //设置对讲状态
        vstream_cmd_video_status_set(data[0]);
        if (data[0] == 0) {
            //切换页面
            walkie_talkie_page_return();
        }
    }
    return ret;
}
static int vstream_cmd_recv_video_stop(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;
    walkie_talkie_hangup_show();
    return ret;
}
static int vstream_cmd_recv_video_reconn(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;
    vstream_cmd_video_break_set(0);
    return ret;
}
//=================================================================//
//				公共流程
//=================================================================//
//包信息校验
static int vstream_cmd_packet_check(u8 *total_data, int total_len)
{
    struct packet_head *send_head = (struct packet_head *)total_data;
    struct packet_end *send_end = (struct packet_end *)(send_head->data + send_head->data_len);
    if (send_head->magic_a != VSTREAM_MAGIC_A) {
        log_error("%s magic_a:%x \n", __func__, send_head->magic_a);
        return -VSTREAM_CMD_PACKET_ERR;
    }
    if (send_end->magic_b != VSTREAM_MAGIC_B) {
        log_error("%s magic_b:%x \n", __func__, send_head->magic_a);
        return -VSTREAM_CMD_PACKET_ERR;
    }
#if VSTREAM_CMD_DATA_CRC_CHECK
    int data_crc = CRC16((u8 *)send_head->data, send_head->data_len);
    if (send_head->data_crc != data_crc) {
        log_error("%s crc:%x:%x\n", __func__, send_head->data_crc, data_crc);
        return -VSTREAM_CMD_PACKET_ERR;
    }
#endif
    return VSTREAM_CMD_NO_ERR;
}
int vstream_send_file(u8 cmd, u8 *data, int data_len)
{
    int send_ret = 0;
    //send
    int retry = 5;
    do {
        send_ret = vstream_send_frame(data, data_len, VSTREAM_TYPE_FILE);
        log_debug("%s cmd:%d data_len:%d retry:%d ret:%d\n", __func__, cmd, data_len, retry, send_ret);
        if (send_ret == data_len) {
            retry = 0;
        } else {
            os_time_dly(3);
            retry --;
        }
    } while (retry);
    /* ASSERT(send_ret); */
    if (send_ret == 0) {
        return -VSTREAM_CMD_SEND_ERR;
    }
    return 0;
}
//命令发送A->B
int vstream_cmd_send_with_adk_sn(u8 cmd, u8 *cmd_sn, u8 *data, int data_len)
{
    int send_ret = 0;
    int send_len = sizeof(struct  packet_head) + sizeof(struct packet_end) + data_len;
    u8 *send_data = malloc(send_len);
    if (!send_data) {
        log_error("%s malloc(%d) failed\n", __func__, send_len);
        KWT_MEM_STATS("cmd.send_with_adk malloc fail");
        return -VSTREAM_CMD_SEND_ERR;
    }
    struct packet_head *send_head = (struct packet_head *)send_data;
    struct packet_end *send_end = (struct packet_end *)(send_data + (sizeof(struct packet_head) + data_len));
    memset(send_head, 0, sizeof(struct  packet_head));
    memset(send_end, 0, sizeof(struct packet_end));
    send_head->magic_a = VSTREAM_MAGIC_A;
    send_head->type = VSTREAM_CMD_TYPE_SEND_WITH_ASK;
    send_head->cmd = cmd;
    send_head->cmd_sn = vstream_sn++;
    send_head->data_len = data_len;
#if VSTREAM_CMD_DATA_CRC_CHECK
    send_head->data_crc = CRC16(data, data_len);
#endif//VSTREAM_CMD_DATA_CRC_CHECK

    memcpy(send_head->data, data, data_len);
    send_end->magic_b = VSTREAM_MAGIC_B;

    if (cmd_sn) {
        *cmd_sn = send_head->cmd_sn;
    }

    log_info("%s cmd:%d data_len:%d\n", __func__, cmd, data_len);
    /* log_debug_hexdump(send_data, send_len); */
    //send
    int retry = 5;
    do {
        send_ret = vstream_send_frame(send_data, send_len, VSTREAM_TYPE_CMD);
        log_debug("%s cmd:%d data_len:%d retry:%d\n", __func__, cmd, data_len, retry);
        if (send_ret == send_len) {
            retry = 0;
        } else {
            os_time_dly(3);
            retry --;
        }
    } while (retry);
    free(send_data);
    //没发出去
    /* ASSERT(send_ret); */
    if (send_ret == 0) {
        return -VSTREAM_CMD_SEND_ERR;
    }
    return 0;
}
int vstream_cmd_send_with_adk(u8 cmd, u8 *data, int data_len)
{
    u8 sn;
    return  vstream_cmd_send_with_adk_sn(cmd, &sn, data, data_len);
}
//回复命令
int vstream_cmd_send_recv(u8 cmd, u8 cmd_sn, u8 *data, u8 data_len)
{
    int send_ret = 0;
    int send_len = sizeof(struct  packet_head) + sizeof(struct packet_end) + data_len;
    u8 *send_data = malloc(send_len);
    if (!send_data) {
        log_error("%s malloc(%d) failed\n", __func__, send_len);
        KWT_MEM_STATS("cmd.send_recv malloc fail");
        return -VSTREAM_CMD_SEND_ERR;
    }
    struct packet_head *send_head = (struct packet_head *)send_data;
    struct packet_end *send_end = (struct packet_end *)(send_data + (sizeof(struct packet_head) + data_len));
    memset(send_head, 0, sizeof(struct  packet_head));
    memset(send_end, 0, sizeof(struct packet_end));
    send_head->magic_a = VSTREAM_MAGIC_A;
    send_head->type = VSTREAM_CMD_TYPE_RECV;
    send_head->cmd = cmd;
    send_head->cmd_sn = cmd_sn;
    send_head->data_len = data_len;
#if VSTREAM_CMD_DATA_CRC_CHECK
    send_head->data_crc = CRC16(data, data_len);
#endif//VSTREAM_CMD_DATA_CRC_CHECK

    memcpy(send_head->data, data, data_len);
    send_end->magic_b = VSTREAM_MAGIC_B;
    log_info("%s cmd:%d data_len:%d\n", __func__, cmd, data_len);
    /* log_debug_hexdump(send_data, send_len); */
    //send
    int retry = 3;
    do {
        send_ret = vstream_send_frame(send_data, send_len, VSTREAM_TYPE_CMD);
        log_debug("%s cmd:%d data_len:%d retry:%d\n", __func__, cmd, data_len, retry);
        if (send_ret == send_len) {
            retry = 0;
        } else {
            retry --;
        }
    } while (retry);
    free(send_data);
    //没发出去
    ASSERT(send_ret);
    if (send_ret == 0) {
        return -VSTREAM_CMD_SEND_ERR;
    }
    return 0;
}

//回复包处理A->B->A
static int vstream_cmd_recv_deal(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;
    log_info("%s cmd:%d cmd:sn%d data_len:%d", __func__, cmd, cmd_sn, len);
    switch (cmd) {
    case VSTREAM_CMD_VIDEO_START:
        //开始视频通讯
        ret = vstream_cmd_recv_video_start(cmd, cmd_sn, data, len);
        break;
    case VSTREAM_CMD_VIDEO_STOP:
        ret = vstream_cmd_recv_video_stop(cmd, cmd_sn, data, len);
        //结束视频通讯
        break;
    case VSTREAM_CMD_VIDEO_RECONN:
        vstream_cmd_recv_video_reconn(cmd, cmd_sn, data, len);
        break;
    case VSTREAM_CMD_CHAT_MEME:
    case VSTREAM_CMD_CHAT_TEXT:
    case VSTREAM_CMD_CHAT_MUSIC:
    case VSTREAM_CMD_CHAT_VOICE:
    case VSTREAM_CMD_CHAT_PHOTO:
        ret = vstream_cmd_recv_chat(cmd, cmd_sn, data, len);
        break;
    default:
        break;
    }
    if (ret) {
        log_error("%s err:%d\n", __func__, ret);
    }
    return ret;
}
//应答B->A
static int vstream_cmd_answer(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;
    log_info("%s cmd:%d cmd:sn%d data_len:%d", __func__, cmd, cmd_sn, len);
    switch (cmd) {
    case  VSTREAM_CMD_VIDEO_START:
        //弹窗确认是否通讯;
        ret = vstream_cmd_ask_video_start(cmd, cmd_sn, data, len);
        break;
    case  VSTREAM_CMD_VIDEO_STOP:
        //关闭弹窗
        ret = vstream_cmd_ask_video_stop(cmd, cmd_sn, data, len);
        break;
    case VSTREAM_CMD_VOICE_START:
        ret = vstream_cmd_ask_voice_start(cmd, cmd_sn, data, len);
        break;
    case VSTREAM_CMD_VOICE_STOP:
        ret = vstream_cmd_ask_voice_stop(cmd, cmd_sn, data, len);
        break;
    case  VSTREAM_CMD_VIDEO_RECONN:
        ret = vstream_cmd_ask_video_reconnect(cmd, cmd_sn, data, len);
        break;
    case VSTREAM_CMD_CHAT_MEME:
    case VSTREAM_CMD_CHAT_TEXT:
    case VSTREAM_CMD_CHAT_MUSIC:
    case VSTREAM_CMD_CHAT_VOICE:
    case VSTREAM_CMD_CHAT_PHOTO:
        ret = vstream_cmd_ask_chat(cmd, cmd_sn, data, len);
        break;
    default:
        ret = vstream_cmd_answer_default(cmd, cmd_sn, data, len);
        break;
    }
    if (ret) {
        log_error("%s err:%d\n", __func__, ret);
    }
    return ret;
}
//应答/接收数据入口
int vstream_cmd_recv(u8 *data, int len)
{
    int ret = VSTREAM_CMD_NO_ERR;
    ret = vstream_cmd_packet_check(data, len);
    if (ret) {
        log_warn("%s err:%d\n", __func__, ret);
        return ret;
    }
    log_debug("%s %d", __func__, __LINE__);
    /* log_debug_hexdump(data, len); */
    struct packet_head *head = (struct packet_head *)data;
    if (head->type == VSTREAM_CMD_TYPE_SEND_WITH_ASK) {
        ret = vstream_cmd_answer(head->cmd, head->cmd_sn, head->data, head->data_len);
    } else if (head->type == VSTREAM_CMD_TYPE_RECV) {
        ret = vstream_cmd_recv_deal(head->cmd, head->cmd_sn, head->data, head->data_len);
    } else {
        ret = -VSTREAM_CMD_TYPE_ERR;
    }

    if (ret) {
        log_error("%s err:%d\n", __func__, ret);
    }
    return ret;
}
