#ifndef __JLDTP_H__
#define __JLDTP_H__

#include "generic/typedef.h"
#include "generic/list.h"

#define JLDTP_MAX_PRIORITY_NUM 4

typedef void *jldtp_handle_t;

typedef enum {
    JLDTP_EVENT_CONNECTED,
    JLDTP_EVENT_DISCONNECTED,
    JLDTP_EVENT_TX_SUSS,
    JLDTP_EVENT_TX_TIMEOUT,
} jldtp_event_t;

// 错误码定义
typedef enum {
    JLDTP_SUCCESS = 0,
    JLDTP_ERROR_INVALID_PARAM = -1,
    JLDTP_ERROR_INVALID_DATA = -2,
    JLDTP_ERROR_MEMORY = -3,
    JLDTP_ERROR_CRC = -4,
    JLDTP_FRAME_BUSY = -5,
    JLDTP_ERROR_FRAME_TOO_LONG = -6,
    JLDTP_ERROR_DEVICE = -7,
    JLDTP_ERROR_TIMEOUT = -8,
    JLDTP_ERROR_NO_CONNECT = -9,
} jldtp_result_t;


// 角色定义
#define    JLDTP_MASTER                 0
#define    JLDTP_SLAVE                  1
#define    JLDTP_TWS_LEFT_AS_MASTER     2   //TWS左耳做主
#define    JLDTP_TWS_LEFT_AS_SLAVE      3   //TWS左耳做从
#define    JLDTP_AUTO                   4



// 传输设备接口
typedef struct {
    void *(*open)(jldtp_handle_t handle, const void *arg);
    void (*close)(void *handle);
    int (*read)(void *handle, u8 *data, int length);
    int (*write)(void *handle, u8 *data, int length);
} jldtp_transport_t;

// 配置结构
typedef struct {
    u8 role;                    //主从角色
    u8 max_retries;             //最大重发次数
    u8 full_duplex;
    u8 response_delay;
    u16 mtu_size;               //传输mtu大小, 超过会被分包
    u16 max_trans_time;         //mtu大小数据最大传输时间, 主机用来判断发送超时
    u16 poll_interval;          //空闲状态下主机的查询间隔
} jldtp_config_t;


typedef struct {
    struct list_head entry;
    u8 head[2];
    u16 crc;
    u16 data_len;
    u8 flags;          // 帧类型：DATA/ACK/NAK
    u8 tx_seq : 4;             // 序列号
    u8 rx_seq : 4;
    u8 data[0];
} jldtp_frame_t;


typedef int (*jldtp_data_callback_t)(void *user_data, jldtp_frame_t *frame);
typedef void (*jldtp_event_callback_t)(void *user_data, jldtp_event_t event, int arg1, int arg2);

// API 函数声明
jldtp_handle_t jldtp_create(const jldtp_config_t *config);

void jldtp_destroy(jldtp_handle_t handle);

jldtp_result_t jldtp_start(jldtp_handle_t handle);

void jldtp_set_task_name(jldtp_handle_t handle, const char *task_name);

int jldtp_get_mtu_size(jldtp_handle_t handle);

u8 jldtp_get_role(jldtp_handle_t handle);

void jldtp_set_role(jldtp_handle_t handle, u8 role);

void jldtp_register_callback(jldtp_handle_t handle, void *user_data,
                             jldtp_data_callback_t data_callback,
                             jldtp_event_callback_t event_callback);

jldtp_result_t jldtp_register_transport(jldtp_handle_t handle,
                                        const jldtp_transport_t *transport, const void *arg);

u8 *jldtp_alloc_tx_buffer(jldtp_handle_t handle, u16 data_length);

jldtp_result_t jldtp_commit_tx_buffer(jldtp_handle_t handle, u8 *buffer,
                                      u16 data_len, u8 priority);

void jldtp_clear_tx_buffer(jldtp_handle_t handle, int offset, u8 value);

jldtp_result_t jldtp_send_data(jldtp_handle_t handle, u8 *buffer,
                               u16 data_len, u8 priority);

void jldtp_process_rx_data(jldtp_handle_t handle, u8 *data, int data_len);

void jldtp_notify_rx_ready(jldtp_handle_t handle);

void jldtp_notify_tx_complete(jldtp_handle_t handle, int completed_count);

u8 jldtp_get_connect_status(jldtp_handle_t handle);

void jldtp_free_frame(jldtp_frame_t *frame);

#endif // __JLDTP_H__
