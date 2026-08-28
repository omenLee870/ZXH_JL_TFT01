#ifndef JLDTP_MULTI_CHANNEL_H
#define JLDTP_MULTI_CHANNEL_H

#include "jldtp/jldtp.h"
#include "generic/typedef.h"


// 前向声明
typedef struct jldtp_mc_ctrl_t jldtp_mc_ctrl_t;
typedef struct channel_ctrl_t channel_ctrl_t;

// 多通道句柄类型
typedef jldtp_mc_ctrl_t *jldtp_mc_handle_t;

// 通道句柄类型
typedef channel_ctrl_t *jldtp_channel_t;



// 通道配置结构
typedef struct {
    u8 channel_id;          // 通道ID
    u8 priority;            // 优先级, 最大值:JLDTP_MAX_PRIORITY_NUM
    u16 timeout_ms;         // 超时时间(ms)
    u16 max_tx_buf_size;    // 通道最大发送缓存大小
    const char *task_name;      //处理接收数据的任务名
} jldtp_channel_config_t;

typedef struct {
    u8 channel_id;
    u8 flags;
    u16 data_length;
} jldtp_mc_header_t;

// 数据回调函数类型
typedef jldtp_result_t (*jldtp_mc_data_callback_t)(void *user_data, jldtp_frame_t *frame,
        u8 *data, u16 data_length);

// ==================== 多通道管理函数 ====================

/**
 * @brief 创建多通道控制器
 *
 * @param jldtp_handle 底层JLDTP句柄
 * @param max_channels 最大通道数
 * @return jldtp_mc_handle_t 多通道句柄，失败返回NULL
 */
jldtp_mc_handle_t jldtp_mc_create(jldtp_handle_t jldtp_handle, u8 max_channels);

/**
 * @brief 销毁多通道控制器
 *
 * @param handle 多通道句柄
 */
void jldtp_mc_destroy(jldtp_mc_handle_t handle);


void jldtp_mc_register_event_callback(jldtp_mc_handle_t handle, void (*callback)(int event));

/**
 * @brief 处理接收到的数据（用于外部数据注入）
 *
 * @param handle 多通道句柄
 * @param data 接收到的数据
 * @param data_len 数据长度
 */
void jldtp_mc_process_rx_data(jldtp_mc_handle_t handle, u8 *data, int data_len);


u8 jldtp_mc_get_role(jldtp_mc_handle_t handle);


// ==================== 通道管理函数 ====================

/**
 * @brief 打开通道
 *
 * @param handle 多通道句柄
 * @param config 通道配置
 * @return jldtp_channel_t 通道句柄，失败返回NULL
 */
jldtp_channel_t jldtp_mc_open_channel(jldtp_mc_handle_t handle, const jldtp_channel_config_t *config);

/**
 * @brief 关闭通道
 *
 * @param channel_handle 通道句柄
 * @return jldtp_result_t 执行结果
 */
jldtp_result_t jldtp_mc_close_channel(jldtp_channel_t channel_handle);

/**
 * @brief 注册通道数据回调函数
 *
 * @param channel_handle 通道句柄
 * @param user_data 用户数据
 * @param callback 数据回调函数
 */
void jldtp_mc_register_data_callback(jldtp_channel_t channel_handle,
                                     void *user_data, jldtp_mc_data_callback_t callback);

/**
 * @brief 发送数据
 *
 * @param channel_handle 通道句柄
 * @param data 要发送的数据
 * @param data_len 数据长度
 * @param timeout_ms 超时时间(ms)
 * @return jldtp_result_t 执行结果
 */
jldtp_result_t jldtp_mc_send_data(jldtp_channel_t channel_handle,
                                  const u8 *data, u16 data_len);

jldtp_result_t jldtp_mc_send_data_with_head(jldtp_channel_t channel_handle,
        const u8 *usr_head, u16 usr_head_length,
        const u8 *data, u16 data_length);


#endif // JLDTP_MULTI_CHANNEL_H
