#include "sparkdesk_main.h"
#include "authentication.h"
#include "stdlib.h"
#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "init.h"
#include "key_event_deal.h"
#include "device/device.h"
#include "app_power_manage.h"
#include "btstack/avctp_user.h"
#include "asm/charge.h"
#include "cJSON.h"
#include "media/includes.h"
#include "audio_config.h"
#include "ai_interaction/ai_audio.h"
#include "websocket_define.h"
#include "ifly_socket.h"
#include "ui/ui_api.h"
#include "ifly_common.h"

#if TCFG_IFLYTEK_ENABLE

#define LOG_TAG_CONST       NET_IFLY
#define LOG_TAG             "[IFLY_AI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#ifdef TCFG_IFLYTEK_APP_ID
#define APP_ID       			TCFG_IFLYTEK_APP_ID
#else
#define APP_ID       			"123"
#endif

#define MAX_SPARKDESK_TOKENS   	((MAX_SPARKDESK_LEN - 100) / 6) //1个tokens相当于1.5个汉字，1个汉字最大4个byte，即1个tokens约6个byte
#define RECV_LAST_FRAME   		2

typedef enum {
    IFLY_AI_STATUS_NULL = 0,
    IFLY_AI_STATUS_START,	// 启动
    IFLY_AI_STATUS_SEND,	// 数据已经发给socket
    IFLY_AI_STATUS_RECV,	// 有接受到数据
    IFLY_AI_STATUS_RECV_END,// 接受完成
    IFLY_AI_STATUS_RECV_ERROR,// 接受错误
    IFLY_AI_STATUS_EXIT,	// 已经退出
} ifly_ai_status;

struct sparkdesk_info_t {
    u8 force_stop;
    u8 recv_finish;
    ifly_ai_status status;
    ifly_ai_param *param;
};

static struct sparkdesk_info_t sparkdesk_info;
static struct ifly_websocket_struct sparkdesk_socket;

//json解析函数_ai
static void ifly_sparkdesk_recv_cb(u8 *j_str, u32 len, u8 type)
{
    ifly_ai_param *param = sparkdesk_info.param;

    if (sparkdesk_info.force_stop) {
        return;
    }
    if (sparkdesk_info.status >= IFLY_AI_STATUS_RECV_END) {
        return;
    }
    if ((param == NULL) || (param->ai_res == NULL) || (param->ai_res_len == 0)) {
        return;
    }
    // websocket 层在 recv_len == buf_len 时不会补 '\0'，必须按长度解析，否则 strlen 会越界读
    cJSON *cjson_root = cJSON_ParseWithLength((char *)j_str, len);
    if (cjson_root == NULL) {
        log_error("cjson error...\r\n");
        if (sparkdesk_info.status <= IFLY_AI_STATUS_RECV_END) {
            sparkdesk_info.status = IFLY_AI_STATUS_RECV_ERROR;
        }
        return;
    }
    sparkdesk_info.status = IFLY_AI_STATUS_RECV;

    cJSON *cjson_header = cJSON_GetObjectItem(cjson_root, "header");
    cJSON *cjson_status = cJSON_GetObjectItem(cjson_header, "status");
    cJSON *cjson_payload = cJSON_GetObjectItem(cjson_root, "payload");
    cJSON *cjson_choices = cJSON_GetObjectItem(cjson_payload, "choices");
    cJSON *cjson_text = cJSON_GetObjectItem(cjson_choices, "text");

    // 心跳/结束帧可能没有 text 节点，解引用前必须判空，否则读地址 0 触发 read hmem exception
    if (cjson_text == NULL) {
        log_error("no text node\n");
        cJSON_Delete(cjson_root);
        return;
    }

    u32 ai_res_len = strlen(param->ai_res);
    // 直接遍历链表，不依赖 arr_size，避免长度与实际节点数不一致时解引用到非法节点
    for (cJSON *arr_item = cjson_text->child; arr_item != NULL; arr_item = arr_item->next) {
        cJSON *cjson_content = cJSON_GetObjectItem(arr_item, "content");
        if (cjson_content == NULL) {
            continue;
        }
        char *cjson_str = cJSON_Print(cjson_content);
        // 打印失败会返回 NULL，strlen(NULL) 会读地址 0
        if (cjson_str == NULL) {
            log_error("cjson print error\n");
            continue;
        }
        u32 json_len = strlen(cjson_str);
        if ((ai_res_len + json_len + 1) > param->ai_res_len) {
            log_error("len error\n");
        } else {
            memcpy(&param->ai_res[ai_res_len], cjson_str, json_len + 1);
            ai_res_len += json_len;
        }
        cJSON_free(cjson_str);
    }

    int res_len = strlen(param->ai_res);
    str_remove_quote(param->ai_res, res_len);
    log_info("final res:%s\n", param->ai_res);

    // status 节点缺失时不能解引用
    if ((cjson_status != NULL) && (cjson_status->valueint == RECV_LAST_FRAME)) {
        sparkdesk_info.status = IFLY_AI_STATUS_RECV_END;
        sparkdesk_info.recv_finish = 1;
        param->event_cb(IFLY_AI_EVT_RECV_OK, param);
    }

    cJSON_Delete(cjson_root);
}


//ai数据模块
static char *ifly_sparkdesk_format_audio_data(void)
{
    char *data_str = NULL;
    cJSON *cjson_test = NULL;
    cJSON *cjson_header = NULL;
    cJSON *cjson_parameter = NULL;
    cJSON *cjson_payload = NULL;
    cJSON *cjson_chat = NULL;
    cJSON *cjson_message = NULL;
    cJSON *cjson_text = NULL;
    cJSON *cjson_texts = NULL;

    cjson_test = cJSON_CreateObject();
    cjson_header = cJSON_CreateObject();
    cjson_parameter = cJSON_CreateObject();
    cjson_payload = cJSON_CreateObject();
    cjson_chat = cJSON_CreateObject();
    cjson_message = cJSON_CreateObject();
    cjson_text = cJSON_CreateObject();

    cJSON_AddStringToObject(cjson_header, "app_id", APP_ID);
    cJSON_AddItemToObject(cjson_test, "header", cjson_header);

    cJSON_AddStringToObject(cjson_chat, "domain", "lite");
    cJSON_AddNumberToObject(cjson_chat, "max_tokens", MAX_SPARKDESK_TOKENS);
    cJSON_AddItemToObject(cjson_parameter, "chat", cjson_chat);
    cJSON_AddItemToObject(cjson_test, "parameter", cjson_parameter);

    cjson_texts = cJSON_AddArrayToObject(cjson_message, "text");
    cJSON_AddStringToObject(cjson_text, "role", "user");
    cJSON_AddStringToObject(cjson_text, "content", sparkdesk_info.param->content);
    cJSON_AddItemToArray(cjson_texts, cjson_text);
    cJSON_AddItemToObject(cjson_payload, "message", cjson_message);
    cJSON_AddItemToObject(cjson_test, "payload", cjson_payload);

    data_str = cJSON_Print(cjson_test);

    cJSON_Delete(cjson_test);

    return data_str;
}

static bool ifly_sparkdesk_get_send(u8 **buf, u32 *len)
{
    if (sparkdesk_info.force_stop) {
        log_info("ai task kill!\n");
        return false;
    }
    if (sparkdesk_info.status >= IFLY_AI_STATUS_RECV_END) {
        log_info("ai task kill!\n");
        return false;
    }
    if (sparkdesk_info.status < IFLY_AI_STATUS_SEND) {
        sparkdesk_info.status = IFLY_AI_STATUS_SEND;
        char *input_src_json = ifly_sparkdesk_format_audio_data();
        if (input_src_json == NULL) {
            log_error("get json err \n");
            return false;
        }
        *buf = (u8 *)input_src_json;
        *len = strlen(input_src_json);
        return true;
    }
    os_time_dly(2);
    return true;
}

static int ifly_sparkdesk_event_cb(ifly_socket_event_enum evt, void *param)
{
    switch (evt) {
    case IFLY_SOCKET_EVT_SEND_OK:
        cJSON_free(param);
        os_time_dly(5);
        break;
    case IFLY_SOCKET_EVT_SEND_ERROR:
        cJSON_free(param);
        break;
    case IFLY_SOCKET_EVT_INIT_OK:
        break;
    case IFLY_SOCKET_EVT_INIT_ERROR:
    case IFLY_SOCKET_EVT_HANSHACK_ERROR:
    case IFLY_SOCKET_EVT_ACCIDENT_END:
    case IFLY_SOCKET_EVT_END:
    case IFLY_SOCKET_EVT_FORCE_END:
        if ((evt != IFLY_SOCKET_EVT_END) && (evt != IFLY_SOCKET_EVT_FORCE_END)) {
            if (!sparkdesk_info.recv_finish) {
                sparkdesk_info.param->event_cb(IFLY_AI_EVT_NETWORK_FAIL, sparkdesk_info.param);
            }
        }
        break;
    case IFLY_SOCKET_EVT_EXIT:
        sparkdesk_info.status = IFLY_AI_STATUS_EXIT;
        sparkdesk_info.param->event_cb(IFLY_AI_EVT_EXIT, sparkdesk_info.param);
        if (sparkdesk_socket.auth) {
            printf("@@@@@@ free sparkdesk auth!!!\n");
            net_iflytek_free(sparkdesk_socket.auth);
            sparkdesk_socket.auth = NULL;
        }
        break;
    default:
        break;
    }
    return 0;
}

bool ifly_sparkdesk_start(ifly_ai_param *param)
{
    memset(&sparkdesk_info, 0, sizeof(struct sparkdesk_info_t));
    memset(&sparkdesk_socket, 0, sizeof(struct ifly_websocket_struct));

#if TCFG_IFLYTEK_USE_PSRAM
    cJSON_Hooks hooks;
    hooks.malloc_fn = net_iflytek_malloc;
    hooks.free_fn = net_iflytek_free;
    cJSON_InitHooks(&hooks);
#endif

    sparkdesk_info.param = param;
    sparkdesk_socket.auth = (u8 *)ifly_authentication("wss://spark-api.xf-yun.com/v1.1/chat",
                            "spark-api.xf-yun.com",
                            "GET /v1.1/chat HTTP/1.1", 20);
    if (!sparkdesk_socket.auth) {
        sparkdesk_info.param->event_cb(IFLY_AI_EVT_NETWORK_FAIL, sparkdesk_info.param);
        return false;
    }

    sparkdesk_socket.task_name = "ifly_ai";
    sparkdesk_socket.socket_mode = WEBSOCKET_MODE;
    sparkdesk_socket.recv_cb = ifly_sparkdesk_recv_cb;
    sparkdesk_socket.get_send = ifly_sparkdesk_get_send;
    sparkdesk_socket.event_cb = ifly_sparkdesk_event_cb;

    sparkdesk_info.status = IFLY_AI_STATUS_START;

    //创建链接
    bool ret = ifly_websocket_client_create(&sparkdesk_socket);
    if (ret == false) {
        sparkdesk_info.status = IFLY_AI_STATUS_NULL;
        net_iflytek_free(sparkdesk_socket.auth);
        sparkdesk_socket.auth = NULL;
    }

    return ret;
}

void ifly_sparkdesk_stop(u8 force_stop, u32 to_ms)
{
    log_info("ai stop!\n");
    sparkdesk_info.force_stop = force_stop;
    while (sparkdesk_socket.auth) { // 结束时auth会自动释放
        os_time_dly(1);
        if (to_ms <= 10) {
            break;
        }
        to_ms -= 10;
    }
    if (to_ms < 1000) {
        to_ms = 1000;
    }
    sparkdesk_info.force_stop = 1;
    ifly_websocket_client_release(&sparkdesk_socket, to_ms);
}

bool ifly_sparkdesk_is_work()
{
    if ((sparkdesk_info.status != IFLY_AI_STATUS_NULL) && (sparkdesk_info.status != IFLY_AI_STATUS_EXIT)) {
        return true;
    }
    return false;
}

#endif

