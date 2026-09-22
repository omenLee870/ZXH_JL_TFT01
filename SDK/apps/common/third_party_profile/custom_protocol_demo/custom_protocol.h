#ifndef _CUSTOM_PROTOCOL_H_
#define _CUSTOM_PROTOCOL_H_

#include "system/includes.h"
#include "btstack/le/ble_api.h"

void custom_demo_all_init(void);
void custom_demo_all_exit(void);
int custom_demo_adv_enable(u8 enable);
int custom_demo_ble_send(u8 *data, u32 len);
int custom_demo_ble_is_connect(void);
int custom_demo_ble_disconnect(void);
int custom_demo_ble_get_att_mtu(void);
void custom_demo_ble_set_adv_interval(u16 adv_interval);
void custom_demo_ble_set_name(const char *name);
u8 *custom_demo_ble_get_addr(void);
/**
 * @return remote_type_e 枚举值：
 *         REMOTE_TYPE_UNKNOWN(0) —— 未连接 / 配对加密尚未完成 / 对方未响应探测；
 *         REMOTE_TYPE_ANDROID(1)、REMOTE_TYPE_IOS(2)
 *
 * @note   需使能 ANCS(TRANS_ANCS_EN) 才能区分 iOS/Android
 */
int custom_demo_ble_get_remote_type(void);
/**
 * @note   param指针指向的内存的生存周期，需同蓝牙工作周期一致
 */
int custom_demo_send_request_connect_parameter(struct conn_update_param_t *param);

int custom_demo_spp_send(u8 *data, u32 len);
void custom_demo_edr_set_name(u8 *name);
void custom_demo_edr_set_addr(u8 *addr, u8 len);

#endif

