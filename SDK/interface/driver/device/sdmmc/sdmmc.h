#ifndef SDMMC_MODULE_H
#define SDMMC_MODULE_H


#include "generic/typedef.h"
#include "generic/ioctl.h"


#define SD_CMD_DECT     0
#define SD_CLK_DECT     1
#define SD_IO_LEV_DECT  2
#define SD_IO_ADC_DECT  3
#define SD_PG_ADC_DECT  4
#define SD_NO_DECT 		5

#define SD_IO_DECT SD_IO_LEV_DECT

#define SD_PWR_SDPG     0
#define SD_PWR_NULL     1

#define SD_CLASS_0      0
#define SD_CLASS_2      1
#define SD_CLASS_4      2
#define SD_CLASS_6      3
#define SD_CLASS_10     4

#define SD_IOCTL_GET_CLASS  _IOR('S', 0, 4)


typedef enum {
    SD_ALL_IO_INIT,
    SD_ALL_IO_UNINIT,
    SD_ALL_IOMC_SUSPEND,
    SD_ALL_IOMC_RESUME,

    SD_CMD_IO_INIT,
    SD_CMD_IO_UNINIT,
    SD_CMD_IOMC_SUSPEND,
    SD_CMD_IOMC_RESUME,

    SD_CLK_IO_INIT,
    SD_CLK_IO_UNINIT,
    SD_CLK_IOMC_SUSPEND,
    SD_CLK_IOMC_RESUME,

    SD_DAT_IO_INIT,
    SD_DAT_IO_UNINIT,
    SD_DAT_IOMC_SUSPEND,
    SD_DAT_IOMC_RESUME,

    SD_DAT123_IO_INIT,
    SD_DAT123_IO_UNINIT,
    SD_DAT123_IOMC_SUSPEND,
    SD_DAT123_IOMC_RESUME,
} sdio_operat_enum;


typedef enum {
    SDPG_IO_HI_Z,
    SDPG_IO_OUTPUT_H,
    SDPG_IO_OUTPUT_H_HD,
    SDPG_IO_OUTPUT_H_HD_HD0,
    SDPG_IO_OUTPUT_H_HD_HD0_SDPG_USER,
    SDPG_IO_OUTPUT_H_HD_HD0_SDPG_JL,
    SDPG_IO_OUTPUT_L,
} sdpg_output_enum;


struct sdmmc_card_dect_platform_data {
    u8 detect_mode;
    u8 detect_io_level;
    u8 detect_cmd_sdc;
    u16 detect_less_ad;
    u16 detect_ad_limit_l;
    u16 detect_ad_limit_h;
    u32 detect_io[3];
    u32 detect_scan_time;
    int (*detect_func)(void *);
    void (*detect_io_operation)(void *, u32);
    int (*detect_io_private_operation)(void *, u32);
};

struct sdmmc_card_platform_data {
    u8 sdc;                                         //sdc0/1/2
    u8 data_width;                                  //dat线的宽度1和4可选
    u32 port[6];                                    //crossbar任意普通IO可选,顺序：cmd,clk,dat0,dat1,dat2,dat3
    u32 speed;                                      //sd模块当前的时钟频率
    void (*power)(int);
    int (*power_status)(void);
    void (*port_operation)(void *, u32);
    int (*port_private_operation)(void *, u32);
    const struct sdmmc_card_dect_platform_data *dect_pdata;
};


#define SDMMC_CARD_DECT_PLATFORM_DATA_BEGIN(data) \
	static const struct sdmmc_card_dect_platform_data data

#define SDMMC_CARD_DECT_PLATFORM_DATA_END() \
    .detect_ad_limit_l = 0, \
    .detect_ad_limit_h = 0, \
    .detect_scan_time = 250, \
    .detect_io_operation = sd_dect_gpio_operation, \

#define SDMMC_CARD_PLATFORM_DATA_BEGIN(data) \
	static const struct sdmmc_card_platform_data data

#define SDMMC_CARD_PLATFORM_DATA_END() \
    .port_operation = sd_gpio_operation, \


typedef struct sdc_brk_data {
    u8 check_mode;
    u8 clk_polarity;
    u8 dat_polarity;
} sdc_brk_t;


void sdc_brk_init(u32 sdc, sdc_brk_t *brk);

void sdc_brk_set_clk_polarity(u32 sdc, u32 polarity);

int sdc_brk_enter(u32 sdc);

int sdc_brk_wait_pend(u32 sdc);

void sdc_brk_exit(u32 sdc);

void sdc_brk_close(u32 sdc);


void sdpg_config(int power_level);

int get_sdpg_status(void);

void sd_gpio_operation(void *pdata, u32 mode);
int sd_gpio_private_operation(void *pdata, u32 mode);

void sd_dect_gpio_operation(void *pdata, u32 mode);
int sd_dect_gpio_private_operation(void *pdata, u32 mode);


int sd_io_detect(void *pdata);


void sdmmc_dev_force_online(char *sdx);

void sdmmc_dev_force_offline(char *sdx);

void sdmmc_dev_dect_scan_timer_suspend(char *sdx);

void sdmmc_dev_dect_scan_timer_resume(char *sdx);


int sdmmc_dev_enter_lowpower(char *sdx);

void sdmmc_dev_lowpower_disable(char *sdx, u32 disable);


extern const struct device_operations sd_dev_ops;


#endif

