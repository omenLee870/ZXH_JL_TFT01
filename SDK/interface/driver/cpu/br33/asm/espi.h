#ifndef  __ESPI_H__
#define  __ESPI_H__

#include "typedef.h"
#include "generic/ioctl.h"


#define ESPI_PORTA_CS 				IO_PORTE_03  //pd0
#define ESPI_PORTA_CLK 			IO_PORTE_04  //pd4
#define ESPI_PORTA_D0 				IO_PORTE_00
#define ESPI_PORTA_D1 				IO_PORTE_02
#define ESPI_PORTA_D2 				IO_PORTE_01
#define ESPI_PORTA_D3 				(-1)//IO_PORTE_05
#define ESPI_PORTA_D4 				IO_PORTD_02
#define ESPI_PORTA_D5 				IO_PORTD_03
#define ESPI_PORTA_D6 				IO_PORTD_04
#define ESPI_PORTA_D7 				IO_PORTD_05
#define ESPI_PORTA_DQS 				IO_PORTD_06

#define     ESPI_STAGE_DIS      0
#define     ESPI_1_LINE         1
#define     ESPI_2_LINE         2
#define     ESPI_4_LINE         3
#define     ESPI_8_LINE         4

#define     ESPI_INST_8_BIT     0
#define     ESPI_INST_16_BIT    1
#define     ESPI_INST_24_BIT    2
#define     ESPI_INST_32_BIT    3

#define     ESPI_STR        0
#define     ESPI_DTR        1

#define     ESPI_WRITE_MODE  0
#define     ESPI_READ_MODE   1
#define     ESPI_MMP_MODE    2

#define  ESPI_MMP_CACHE_ADDR2FLASH_ADDR(x)  ((x)&0x7ffffff)//64Mb
#define  ESPI_FLASH_ADDR2MMP_CACHE_ADDR(x)  ((x)+0x14000000)//64Mb

struct espi_io_t {
    u32 data;
    u16 len: 4;
    u16 dtr: 1;
    u16 line: 3;
};

struct espi_data_t {
    u16 len: 4;
    u16 dtr: 1;
    u16 line: 3;
    u8 data[8];
};

struct espi_control {
    u8  width[4];
    u8  cmd;
    u8  dummy;
};

struct psram_trim_cfg {
    u16 adapt;
    u8 phase;
};

void hw_open_espi_power(u8 power_io, u8 power_level, u16 wait_us, u8 espi_width);
void hw_close_espi_power(u8 power_io, u8 power_level, u16 wait_us);

void espi_cpu_mode_resume(void);
void espi_cpu_mode_suspend(void);
u8 espi_get_cpu_mode(void);
u32 clock_espi_switching(u32 espi_io_freq);
void espi_hw_init(u32 espi_io_freq, u8 tshsl, u8 page_size, u8 width);
void espi_hw_clk_change(u8 read, u32 clk);
void espi_hw_manual_cs_out(u8 sta);
void espi_hw_cs_io_cfg(u8 en);
void espi_hw_clk_io_cfg(u8 en);
void espi_hw_resume_cfg(void);
void espi_hw_suspend_cfg(void);
void espi_wait_pnd(void);
void update_psram_trim_cfg(const struct psram_trim_cfg *cfg);

void espi_hw_ioctrl(const struct espi_io_t *const instruction,
                    const struct espi_io_t *const address,
                    const struct espi_io_t *const alby,
                    const struct espi_data_t *const tx_data,
                    struct espi_data_t *const rx_data,
                    u32 dummy_cycle);
void espi_hw_ioctrl_read(const struct espi_io_t *const inst,
                         const struct espi_io_t *const address,
                         struct espi_data_t *const rx_data,
                         u32 dummy_cycle);
void espi_hw_ioctrl_write(const struct espi_io_t *const inst,
                          const struct espi_io_t *const address,
                          const struct espi_data_t *const tx_data,
                          u32 dummy_cycle);

void espi_hw_send_cmd(const struct espi_io_t *const inst);

void espi_hw_transfer_data_cfg(struct espi_data_t *const data, u8 dir);//dir:0:tx; 1:rx
void espi_hw_transfer_data(u32 *data, u8 len, u8 dir);//dir:0:tx; 1:rx

void espi_mmp_mode_xn_init(const struct espi_control *r_cmd, const struct espi_control *w_cmd, u8 is4byte_mode);//no continue no write
void espi_hw_info_dump();
#endif  /*ESPI_H*/

