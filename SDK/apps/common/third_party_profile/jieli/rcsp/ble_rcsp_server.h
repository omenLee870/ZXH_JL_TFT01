// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_RCSP_SERVER_H
#define _BLE_RCSP_SERVER_H

#include <stdint.h>
#include "bt_common.h"
#include "ble_user.h"
#include "btstack/btstack_typedef.h"
#include "app_config.h"


#if (BT_AI_SEL_PROTOCOL&RCSP_MODE_EN)

static const uint8_t profile_data[1];


enum {
    BT_ADV_ENABLE,
    BT_ADV_DISABLE,
    BT_ADV_SET_EDR_CON_FLAG,
    BT_ADV_SET_BAT_CHARGE_L,
    BT_ADV_SET_BAT_CHARGE_R,
    BT_ADV_SET_BAT_CHARGE_C,
    BT_ADV_SET_BAT_PERCENT_L,
    BT_ADV_SET_BAT_PERCENT_R,
    BT_ADV_SET_BAT_PERCENT_C,
    BT_ADV_SET_NOTIFY_EN,
};

enum {
    TWS_ADV_SEQ_CHANGE = 0,
    TWS_VERSON_INFO,
    TWS_UPDATE_INFO,
};

void rcsp_ble_profile_init(void);
void bt_ble_init(void);
void bt_ble_exit(void);
void rcsp_bt_ble_init(void);
void rcsp_bt_ble_exit(void);
void rcsp_bt_ble_adv_enable(u8 enable);
void ble_module_enable(u8 en);

extern void bt_adv_seq_change(void);
void ble_app_disconnect(void);

void notify_update_connect_parameter(u8 table_index);

/**
 *	@brief 设置ble的地址
 */
void rcsp_app_ble_set_mac_addr(void *addr);

// 返回当前设备支持的最大连接数
u8 rcsp_max_support_con_dev_num();
// 根据已连接设备数量判断是否开关蓝牙广播
void rcsp_ble_adv_enable_with_con_dev();

/**
 * @brief 断开指定的ble
 *
 * @param ble_con_handle ble_con_handle
 */
void rcsp_disconn_designated_ble(u16 ble_con_handle);

/**
 * @brief 断开另一个ble
 *
 * @param ble_con_handle 保留的ble_con_handle，输入为0的时候，全部断开
 */
void rcsp_disconn_other_ble(u16 ble_con_handle);

/**
 * @brief 一定时间设置是否关闭可发现可连接
 * 			对应TCFG_DUAL_CONN_INQUIRY_SCAN_TIME功能配置
 *
 * @param close_inquiry_scan 是否关闭可发现可连接
 */
void rcsp_close_inquiry_scan(bool close_inquiry_scan);

// ble主机相关，by lingxuanfeng
/**
 * @brief ble主机接收回调
 *
 * @param buf
 * @param len
 */
void rcsp_ble_master_recieve_callback(void *buf, u16 len);

#endif // (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_RCSP_DEMO)
#endif // _BLE_RCSP_SERVER_H
