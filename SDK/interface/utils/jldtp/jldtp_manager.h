#ifndef JLDTP_MANAGER_H
#define JLDTP_MANAGER_H


#include "jldtp/jldtp_multi_channel.h"
#include "jldtp/jldtp_channel_ids.h"

enum {
    MULTI_CHIP_ID_1 = 0,
    MULTI_CHIP_ID_2,
    MULTI_CHIP_ID_3,
    MULTI_CHIP_ID_4,
};


enum jldtp_cmd {
    JLDTP_CMD_BEGIN = 20,
    JLDTP_CMD_TWS_ADDR,
    JLDTP_CMD_ENTER_MODE,
    JLDTP_CMD_OPEN_AUDIO_PLAYER,
    JLDTP_CMD_CLOSE_AUDIO_PLAYER,

};

/* 传输类型宏（供预处理器 #if 使用，enum值在 #if 中不可用） */
#define CHIP_CONN_TYPE_UART     0
#define CHIP_CONN_TYPE_SPI      1

struct multi_chip_conn_config {
    u8 role;                     // JLDTP_MASTER / JLDTP_SLAVE
    u8 remote_chip_id;           // MULTI_CHIP_ID_1~4
    u16 mtu_size;                // 传输MTU大小
    void *periph_cfg;            // 外设配置指针
};

int jldtp_manager_init(const struct multi_chip_conn_config *configs, u8 config_num);

void jldtp_manager_deinit(void);

jldtp_mc_handle_t jldtp_manager_get_mc_handle(u8 chip_id);

jldtp_handle_t jldtp_manager_get_dtp_handle(u8 chip_id);

int jldtp_manager_send_msg(u8 chip_id, enum jldtp_cmd cmd, u32 arg);

int jldtp_manager_send_data(u8 chip_id, enum jldtp_cmd cmd, u8 *data, u16 len);

int jldtp_manager_get_role(u8 chip_id);

void jldtp_manager_set_role(u8 chip_id, u8 role);

jldtp_channel_t jldtp_manager_open_channel(u8 chip_id, jldtp_channel_config_t *config);

/* ======================== JLDTP 统一测试接口 ======================== */

/** @brief 启用 JLDTP 统一测试（默认关闭，不区分SPI/UART）
 *  测试内容：
 *    - 带宽统计 (bps)
 *    - 响应时间 (avg / min / max)
 *    - 包长 512 到 MTU 分5组,每组测10次,做统计
 *    - 测试期间暂停心跳/控制帧,仅传输数据帧
 */
#define JLDTP_TEST_ENABLE 0

#if JLDTP_TEST_ENABLE

void jldtp_test_init(jldtp_handle_t handle, u8 role, u16 mtu);
void jldtp_test_deinit(void);

#if 0
/* SPI 传输层 ISR 时序测试 GPIO 翻转宏（逻辑分析仪用） */
#define JLDTP_TEST_GPIO_TOGGLE_PA8()  do { JL_PORTA->OUT ^= BIT(8); } while(0)
#define JLDTP_TEST_GPIO_TOGGLE_PA9()  do { JL_PORTA->OUT ^= BIT(9); } while(0)
#define JLDTP_TEST_GPIO_TOGGLE_PA10()  do { JL_PORTA->OUT ^= BIT(10); } while(0)
#else
#define JLDTP_TEST_GPIO_TOGGLE_PA8()
#define JLDTP_TEST_GPIO_TOGGLE_PA9()
#define JLDTP_TEST_GPIO_TOGGLE_PA10()
#endif

#endif /* JLDTP_TEST_ENABLE */

#endif

