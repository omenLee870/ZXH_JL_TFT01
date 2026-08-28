#ifndef     __SCREEN_EAR_INTERFACE_H__
#define     __SCREEN_EAR_INTERFACE_H__

#include "typedef.h"

/*-------------------CMD start----------------------*/
#define CUSTOM_ALL_INFO_CMD                             0xff  // 所有信息
#define CUSTOM_BT_CONNECT_STATE_CMD                     0x1   // bt连接状态
#define CUSTOM_BLE_CONNECT_STATE_CMD                    0x2   // ble连接状态
#define CUSTOM_BLE_BATTERY_STATE_CMD                    0x3   // 电量信息
#define CUSTOM_BLE_VOLUMEN_CMD                          0x4   // 音量信息
#define CUSTOM_BLE_TIME_DATE_CMD                        0x5   // 时间信息
#define CUSTOM_EQ_DATE_CMD                              0x6   // EQ信息
#define CUSTOM_ANC_DATE_CMD                             0x7   // ANC信息
#define CUSTOM_CALL_STATE_CMD                           0x8   // 通话状态获取
#define CUSTOM_PHONE_CALL_INFO_CMD                      0x9   // 通话号码信息

#define CUSTOM_BLE_VOL_CONTROL_CMD                      0x32 // 音量控制
#define CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD              0x33 // 音乐状态控制
#define CUSTOM_BLE_ANC_MODE_CONTROL_CMD                 0x34 // ANC 模式控制
#define CUSTOM_BLE_EQ_MODE_CONTROL_CMD                  0x35 // EQ 模式控制
#define CUSTOM_BLE_PLAY_MODE_CONTROL_CMD                0x36 // 设置播放模式
#define CUSTOM_BLE_ALARM_CLOCK_CONTROL_CMD              0x37 // 播放闹钟
#define CUSTOM_BLE_FINE_EARPHONE_CMD                    0x38 // 查找手机
#define CUSTOM_BLE_FLASHLIGHT_CONTROL_CMD               0x39 // 手电筒
#define CUSTOM_BLE_SWITCH_LANGUAGE                      0x40 //切换语言 1中文，2英文
#define CUSTOM_BLE_CONTRAL_CALL                         0x41 //控制接听挂断  1接听 2挂断
#define CUSTOM_BLE_USER_ADD_CMD                         0X42 //用户自定义命令
#define CUSTOM_BLE_CONTRAL_DOUYIN                       0X43  //控制抖音操作
#define CUSTOM_BLE_CONTRAL_PHOTO                        0X44  //控制拍照操作

#define CUSTOM_EDR_CONTRAL_CONN                         0X45  // 控制edr连接
#define CUSTOM_EDR_SYNC_INFO                            0X46  // 同步经典蓝牙信息
#define CUSTOM_EDR_SIRI_CTRL                            0X47  // 语音助手设置
#define CUSTOM_BLE_CONTRAL_PHONEOUT                     0X48  // 拨出电话
#define CUSTOM_BLE_CONTRAL_KEY                          0X49  // 按键同步设置
#define CUSTOM_SLEEP_CTRL_CMD                           0x4A  // 屏幕亮灭状态
#define CUSTOM_EDR_CLEAR_COMP                           0x4B  // br等待仓连接
/*-------------------CMD end----------------------*/


typedef void (*screen_app_receive_handle)(u8 cmd, u8 *data, u8 len);
typedef int (*screen_app_operation_send_handle)(u8 *data, u16 len);

void register_receive_handle(screen_app_receive_handle callback);
void register_send_handle(screen_app_operation_send_handle callback);
void custom_ble_client_write_no_respond(u8 cmd, u8 *data, u8 len);
int ble_notify_recv_data_handler(uint8_t *buffer, uint16_t buffer_size);

#endif
