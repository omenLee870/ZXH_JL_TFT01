/*********************************************************************************************
    *   Filename        : app_connected.c

    *   Description     :

    *   Author          : Weixin Liang

    *   Email           : liangweixin@zh-jieli.com

    *   Last modifiled  : 2022-12-21 15:53

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "app_config.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "app_msg.h"

/* #if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) */
#if 0

/**************************************************************************************************
  Macros
**************************************************************************************************/
#define LOG_TAG_CONST       APP
#define LOG_TAG             "[CONNECTED]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

// 设备标识，用于区分不同设备，与实际声道没有直接的关联
#define CONNECTED_DEVICE_IDENTIFICATION_M  BIT(0)   //主机(central)
#define CONNECTED_DEVICE_IDENTIFICATION_L  BIT(1)   //从机(perip)左声道
#define CONNECTED_DEVICE_IDENTIFICATION_R  BIT(2)   //从机(perip)右声道

enum {
    CONNECTED_EVENT_SYNC_TYPE = 1,
    CONNECTED_KEY_SYNC_TYPE,
    CONNECTED_DATA_TRANSMIT_TYPE,
};

#define CONNECTED_SYNC_CALL_REGISTER(sync_call) \
    static const struct connected_sync_call sync_call sec(.connected_sync_call_func)

#define list_for_each_connected_sync_call(p) \
    for (p = conn_sync_call_func_begin; p < conn_sync_call_func_end; p++)

#define CONN_DATA_TRANS_STUB_REGISTER(stub) \
    static const struct conn_data_trans_stub stub sec(.conn_data_trans_stub)

#define list_for_each_connected_data_trans(p) \
    for (p = conn_data_trans_stub_begin; p < conn_data_trans_stub_end; p++)

#define CONNECTED_SYNC_CALL_TX                1
#define CONNECTED_SYNC_CALL_RX                2

#define EVENT_POOL_SIZE         64

/**************************************************************************************************
  Data Types
**************************************************************************************************/
struct connected_event_sync_call {
    u8 type;
    u8  tx;
    u16 timer;
    int uuid;
    int priv;
    int instant;
    struct list_head entry;
};

struct connected_data_transmit {
    u8 type;
    u8 length;
    int uuid;
};

struct connected_key_event_sync {
    u8 tx_seqn;
    u8 rx_seqn;
    u8 tx_index;
    u8 rx_index;
    u8 event_lock;
    u8 event_len[2];
    u8 key_events[2][EVENT_POOL_SIZE];
    u16 hi_timer;
};

struct connected_sync_call {
    int uuid;
    void (*func)(int priv, int sync_call_role);
    const char *task_name;
};

struct conn_data_trans_stub {
    int uuid;
    void (*func)(u16 acl_hdl, u8 arg_num, int *argv);
    const char *task_name;
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/
static struct list_head sync_call_head = LIST_HEAD_INIT(sync_call_head);
static struct connected_key_event_sync key_event_sync;
static DEFINE_SPINLOCK(key_sync);

extern const struct connected_sync_call  conn_sync_call_func_begin[];
extern const struct connected_sync_call  conn_sync_call_func_end[];
extern const struct conn_data_trans_stub conn_data_trans_stub_begin[];
extern const struct conn_data_trans_stub conn_data_trans_stub_end[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* --------------------------------------------------------------------------*/
/**
 * @brief 通过uuid找到对应回调函数
 *
 * @param uuid:注册回调时配置的ID
 *
 * @return 注册的回调结构体(struct connected_sync_call)指针
 */
/* ----------------------------------------------------------------------------*/
static const struct connected_sync_call *__get_sync_call_func_by_uuid(int uuid)
{
    const struct connected_sync_call *p;

    list_for_each_connected_sync_call(p) {
        if (p->uuid == uuid) {
            return p;
        }
    }
    return NULL;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 超时任务执行函数
 *
 * @param _sc:同步事件调用结构体(struct connected_event_sync_call)
 */
/* ----------------------------------------------------------------------------*/
static void __call_func_timeout(void *_sc)
{
    int msg[4];
    const struct connected_sync_call *p;
    struct connected_event_sync_call *sc = (struct connected_event_sync_call *)_sc;
    sc->timer = 0;

    p = __get_sync_call_func_by_uuid(sc->uuid);
    if (!p) {
        r_printf("sync call func not found!");
        free(sc);
        return;
    }

    int err = sc->tx ? CONNECTED_SYNC_CALL_TX : CONNECTED_SYNC_CALL_RX;

    if (!p->task_name) {
        p->func(sc->priv, err);
    } else {
        msg[0] = (int)p->func;
        msg[1] = 2;
        msg[2] = sc->priv;
        msg[3] = err;

        int os_err = os_taskq_post_type(p->task_name, Q_CALLBACK, 4, msg);
        if (os_err != OS_ERR_NONE) {
            log_error("sync_call_err: %x, %d\n", sc->uuid, os_err);
        }
    }

    free(sc);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 任务同步调用接口，约定事件双方同时执行对应的事件任务
 *
 * @param uuid:注册回调时配置的ID
 * @param priv:需要同步执行的事件
 * @param delay_ms:约定的延时时间
 * @param device_channel:需要跟本机同步执行的远端设备的标识
 *
 * @return len:实际发出去的数据长度
 */
/* ----------------------------------------------------------------------------*/
int connected_api_sync_call_by_uuid(int uuid, int priv, int delay_ms, u8 device_channel)
{
    if (!app_cis_is_connected(device_channel)) {
        return 0;
    }

    int buffer_len = sizeof(struct connected_event_sync_call);
    struct connected_event_sync_call *sc = zalloc(buffer_len);
    if (!sc) {
        return -ENOMEM;
    }

    sc->type    = CONNECTED_EVENT_SYNC_TYPE;
    sc->tx      = 1;
    sc->uuid    = uuid;
    sc->priv    = priv;
    sc->instant = delay_ms;
    int len = app_connected_send_acl_data(device_channel, (void *)sc, buffer_len);
    if (!len) {
        free(sc);
    }

    return len;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 同步事件发送成功接口
 *
 * @param data:发出去的数据
 * @param len:发出去的数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int connected_tx_events_suss(void *data, int len)
{
    struct connected_event_sync_call *sc = (struct connected_event_sync_call *)data;
    if (sc->type != CONNECTED_EVENT_SYNC_TYPE) {
        return false;
    }
    sc->timer = sys_hi_timeout_add(sc, __call_func_timeout, sc->instant);
    list_add_tail(&sc->entry, &sync_call_head);
    return true;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 同步事件接收成功接口
 *
 * @param acl_hdl:acl数据通道句柄
 * @param data:接收到的数据
 * @param len:接收到的数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int connected_rx_events_suss(u16 acl_hdl, const void *const data, int len)
{
    struct connected_event_sync_call sc;
    memcpy(&sc, data, sizeof(struct connected_event_sync_call));
    if (sc.type != CONNECTED_EVENT_SYNC_TYPE) {
        return false;
    }
    u8 *buffer = zalloc(len);
    if (!buffer) {
        ASSERT(0);
    }
    memcpy(buffer, data, len);
    sc.tx    = 0;
    sc.timer = sys_hi_timeout_add(buffer, __call_func_timeout, sc.instant);
    memcpy(buffer, &sc, sizeof(struct connected_event_sync_call));
    list_add_tail(&sc.entry, &sync_call_head);
    return true;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG状态回调函数，用于处理部分状态下事件同步执行的流程
 *
 * @param event:CIG状态事件
 * @param arg:状态事件附带的参数
 */
/* ----------------------------------------------------------------------------*/
static void connected_sync_call_events_handler(const CIG_EVENT event, void *arg)
{
    struct connected_event_sync_call *sc, *n;
    switch (event) {
    case CIG_EVENT_CENTRAL_CONNECT:
    case CIG_EVENT_PERIP_CONNECT:
        break;
    case CIG_EVENT_CENTRAL_DISCONNECT:
    case CIG_EVENT_PERIP_DISCONNECT:
        list_for_each_entry_safe(sc, n, &sync_call_head, entry) {
            if (sc->timer) {
                sys_hi_timeout_del(sc->timer);
            }
            list_del(&sc->entry);
            __call_func_timeout((void *)sc);
        }
        break;
    default:
        break;
    }
}

CONNECTED_SYNC_CALLBACK_REGISTER(connected_event_sync) = {
    .event_handler  = connected_sync_call_events_handler,
    .tx_events_suss = connected_tx_events_suss,
    .rx_events_suss = connected_rx_events_suss,
};

/* --------------------------------------------------------------------------*/
/**
 * @brief 发送数据给远端设备
 *
 * @param uuid:注册回调时配置的ID
 * @param data:需要发送的数据
 * @param len:发送数据的长度
 * @param device_channel:远端设备的标识
 *
 * @return 实际发送的数据长度
 */
/* ----------------------------------------------------------------------------*/
int connected_send_data_to_sibling(int uuid, void *data, u16 len, u8 device_channel)
{
    if (!app_cis_is_connected(device_channel)) {
        return 0;
    }

    int buffer_len = sizeof(struct connected_data_transmit) + len;
    struct connected_data_transmit *dt = zalloc(buffer_len);
    if (!dt) {
        return -ENOMEM;
    }

    dt->type = CONNECTED_DATA_TRANSMIT_TYPE;   //第一个Byte记录传输数据的类型
    dt->uuid = uuid;
    dt->length = len;
    memcpy((u8 *)dt + sizeof(*dt), data, len);

    int slen = app_connected_send_acl_data(device_channel, (void *)dt, buffer_len);
    if (!slen) {
        r_printf("connected_send_data_to_sibling err");
        free(dt);
    }

    return slen;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 数据发送成功接口
 *
 * @param data:实际发送出去的数据
 * @param len:数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int connected_data_tx_suss(void *data, int len)
{
    struct connected_data_transmit *dt = (struct connected_data_transmit *)data;
    if (dt->type != CONNECTED_DATA_TRANSMIT_TYPE) {
        return false;
    }
    free(data);
    return true;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 数据接收成功接口
 *
 * @param acl_hdl:acl数据通道句柄
 * @param data:实际接收到的数据
 * @param len:数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int connected_data_rx_suss(u16 acl_hdl, const void *const data, int len)
{
    int msg[6];
    struct connected_data_transmit dt;
    memcpy(&dt, data, sizeof(struct connected_data_transmit));
    if (dt.type != CONNECTED_DATA_TRANSMIT_TYPE) {
        return false;
    }

    u8 find = 0;
    const struct conn_data_trans_stub *p;
    list_for_each_connected_data_trans(p) {
        if (p->uuid == dt.uuid) {
            find = 1;
            break;
        }
    }
    if (!find) {
        r_printf("data trans callback func not found!");
        return false;
    }

    u8 *buffer = zalloc(dt.length);
    if (!buffer) {
        ASSERT(0);
    }
    memcpy(buffer, (u8 *)data + sizeof(struct connected_data_transmit), dt.length);
    if (!p->task_name) {
        msg[0] = CONNECTED_SYNC_CALL_RX;
        msg[1] = (int)buffer;
        msg[2] = dt.length;
        p->func(acl_hdl, 3, msg);
    } else {
        msg[0] = (int)p->func;
        msg[1] = 4;
        msg[2] = acl_hdl;
        msg[3] = CONNECTED_SYNC_CALL_RX;
        msg[4] = (int)buffer;
        msg[5] = dt.length;

        int os_err = os_taskq_post_type(p->task_name, Q_CALLBACK, 6, msg);
        if (os_err != OS_ERR_NONE) {
            log_error("data_trans_err: %x, %d\n", p->uuid, os_err);
            free(buffer);
        }
    }

    return true;
}

CONNECTED_SYNC_CALLBACK_REGISTER(connected_data_transmit) = {
    .event_handler  = NULL,
    .tx_events_suss = connected_data_tx_suss,
    .rx_events_suss = connected_data_rx_suss,
};

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件同步接口，默认在按键消息池处截取按键消息
 *
 * @param key_msg:具体的按键消息
 *
 * @return 按键消息转发长度
 */
/* ----------------------------------------------------------------------------*/
int connected_key_event_sync(int key_msg)
{
#if (!LEA_CIG_KEY_EVENT_SYNC)
    return 0;
#endif

    if (key_event_sync.event_lock) {
        return 0;
    }
    if (!app_cis_is_connected(CONNECTED_DEVICE_IDENTIFICATION_M | CONNECTED_DEVICE_IDENTIFICATION_L | CONNECTED_DEVICE_IDENTIFICATION_R)) {
        return 0;
    }

    int slen = 0;
    u8 len = sizeof(int) + 1;
    u8 *buffer = zalloc(len);
    u8 device_channel = CONNECTED_DEVICE_IDENTIFICATION_M | CONNECTED_DEVICE_IDENTIFICATION_L | CONNECTED_DEVICE_IDENTIFICATION_R;
    ASSERT(buffer);
    buffer[0] = CONNECTED_KEY_SYNC_TYPE;
    memcpy(buffer + 1, &key_msg, sizeof(int));

    if (buffer) {
        slen = app_connected_send_acl_data(device_channel, (void *)buffer, len);
        if (!slen) {
            free(buffer);
            app_send_message(key_msg, 0);
        }
    }

    return slen;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG状态回调函数，用于处理部分状态下按键同步执行的流程
 *
 * @param event:CIG状态事件
 * @param arg:状态事件附带的参数
 */
/* ----------------------------------------------------------------------------*/
static void connected_sync_key_events_handler(const CIG_EVENT event, void *arg)
{
    switch (event) {
    case CIG_EVENT_CENTRAL_CONNECT:
    case CIG_EVENT_PERIP_CONNECT:
        break;
    case CIG_EVENT_CENTRAL_DISCONNECT:
    case CIG_EVENT_PERIP_DISCONNECT:
        break;
    default:
        break;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件发送成功接口
 *
 * @param data:实际发送出去的数据
 * @param len:数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int connected_tx_key_events_suss(void *data, int len)
{
    u8 *buffer = (u8 *)data;
    int key_msg;
    if (buffer[0] != CONNECTED_KEY_SYNC_TYPE) {
        return false;
    }

    memcpy(&key_msg, &buffer[1], sizeof(key_msg));
    app_send_message(key_msg, 0);
    free(data);

    return true;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件接收成功接口
 *
 * @param acl_hdl:acl数据通道句柄
 * @param data:实际接收到的数据
 * @param len:数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int connected_rx_key_events_suss(u16 acl_hdl, const void *const data, int len)
{
    u8 *buffer = (u8 *)data;
    int key_msg;
    if (buffer[0] != CONNECTED_KEY_SYNC_TYPE) {
        return false;
    }

    memcpy(&key_msg, &buffer[1], sizeof(key_msg));
    app_send_message(key_msg, APP_KEY_MSG_FROM_CIS);

    return true;
}

CONNECTED_SYNC_CALLBACK_REGISTER(connected_key_sync) = {
    .event_handler  = connected_sync_key_events_handler,
    .tx_events_suss = connected_tx_key_events_suss,
    .rx_events_suss = connected_rx_key_events_suss,
};

/*==========================================================================
 *                               demo
 *=========================================================================*/
#if 0
static void common_state_sync_handler(int state, int sync_call_role)
{
    switch (state) {
    default:
        g_printf("%s %d, state:0x%x, sync_call_role:%d", __FUNCTION__, __LINE__, state, sync_call_role);
        break;
    }
}

CONNECTED_SYNC_CALL_REGISTER(common_state_sync) = {
    .uuid = 'C',
    .task_name = "app_core",
    .func = common_state_sync_handler,
};

static void data_transmit_handler(u16 acl_hdl, u8 arg_num, int *argv)
{
    int trans_role = argv[0];
    u8 *data = (u8 *)argv[1];
    int len = argv[2];
    g_printf("%s %d, length:%d, trans_role:%d", __FUNCTION__, __LINE__, len, trans_role);
    put_buf(data, len);
    //由于转发流程中申请了内存，因此执行完毕后必须释放
    if (data) {
        free(data);
    }
}

CONN_DATA_TRANS_STUB_REGISTER(data_transmit) = {
    .uuid = 'C',
    /* 没有给task_name赋值时，func默认在收数中断执行。当func在收数中断执行时，func内的代码尽量简短，避免对后续收数产生影响 */
    .task_name = "app_core",
    .func = data_transmit_handler,
};
#endif

#endif

