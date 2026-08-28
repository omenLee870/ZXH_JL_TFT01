#ifndef UART_TRANSPORT_H
#define UART_TRANSPORT_H


#include "jldtp/jldtp.h"


typedef struct {
    u32 tx_pin;
    u32 rx_pin;
    u32 baud_rate;
    u16 mtu_size;
    u8  irq_priority;
} jldtp_uart_config_t;

extern const jldtp_transport_t jldtp_uart_transport;

/**
 * @brief 获取UART传输层接口并填充jldtp配置（full_duplex/max_trans_time）
 * @param dtp_cfg     jldtp配置（输入公共字段，输出full_duplex/max_trans_time）
 * @param periph_cfg  UART外设配置（jldtp_uart_config_t指针，输出mtu_size/irq_priority）
 * @param jldtp_role  JLDTP_MASTER / JLDTP_SLAVE
 * @return UART传输层接口指针
 */
const jldtp_transport_t *jldtp_uart_transport_get(jldtp_config_t *dtp_cfg,
        void *periph_cfg, u8 jldtp_role);

#endif
