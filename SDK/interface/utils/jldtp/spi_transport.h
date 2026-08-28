#ifndef SPI_TRANSPORT_H
#define SPI_TRANSPORT_H

#include "jldtp/jldtp.h"
#include "asm/spi_hw.h"
#include "cpu/periph/spi.h"

/* ======================== SPI 传输驱动选择 ========================
 *
 * 两颗驱动宏二选一，同时只能启用一个：
 *   JLDTP_SPI_USE_ROLE_SWITCH = 1   → 编译 spi_transport_role_switch.c
 *   JLDTP_SPI_USE_FIX_LEN     = 1   → 编译 spi_transport_fix_len.c
 */
#ifndef JLDTP_SPI_USE_ROLE_SWITCH
#define JLDTP_SPI_USE_ROLE_SWITCH  0   // 角色切换模式驱动 (默认关闭)
#define JLDTP_SPI_USE_FIX_LEN      1   // 固定帧长模式驱动 (默认启用)
#endif

#if JLDTP_SPI_USE_ROLE_SWITCH && JLDTP_SPI_USE_FIX_LEN
#error "JLDTP_SPI_USE_ROLE_SWITCH 和 JLDTP_SPI_USE_FIX_LEN 不能同时启用！"
#endif
#if !JLDTP_SPI_USE_ROLE_SWITCH && !JLDTP_SPI_USE_FIX_LEN
#error "至少需要启用一个 SPI 传输驱动！"
#endif

/* 固定帧长模式: 帧尾部开销 [0xF0][len(u16)][crc(u8)] = 4 字节 */
#define JLDTP_SPI_FIX_LEN_TAIL 4

/* 尾部 SOF 标记字节 (位于帧末尾倒数第4字节) */
#define JLDTP_SPI_FIX_LEN_SOF   0xF0

typedef struct {
    hw_spi_dev spi;             // SPI设备号: HW_SPI1 / HW_SPI2
    u8 role;                    // SPI_ROLE_MASTER / SPI_ROLE_SLAVE
    u8 mode;                    // SPI模式: SPI_MODE_BIDIR_1BIT等
    u32 clk_pin;                // CLK引脚
    u32 do_pin;                 // DO(MOSI)引脚
    u32 di_pin;                 // DI(MISO)引脚
    u32 d2_pin;                 // D2(wp)引脚，未使用时填0xff
    u32 d3_pin;                 // D3(hold)引脚，未使用时填0xff
    u32 cs_pin;                 // CS引脚，未使用时填0xff（不使用CS模式）
    u32 freq;                   // SPI时钟频率(Hz)
    u16 mtu_size;               // 传输MTU大小
    u8 irq_priority;            // 中断优先级
} jldtp_spi_config_t;

extern const jldtp_transport_t jldtp_spi_transport;

/**
 * @brief 获取SPI传输层接口并填充jldtp配置（full_duplex/max_trans_time）
 * @param dtp_cfg     jldtp配置（输入公共字段，输出full_duplex/max_trans_time）
 * @param periph_cfg  SPI外设配置（jldtp_spi_config_t指针，输出role/mtu_size/irq_priority）
 * @param jldtp_role  JLDTP_MASTER / JLDTP_SLAVE
 * @return SPI传输层接口指针
 */
const jldtp_transport_t *jldtp_spi_transport_get(jldtp_config_t *dtp_cfg,
        void *periph_cfg, u8 jldtp_role);

#endif
