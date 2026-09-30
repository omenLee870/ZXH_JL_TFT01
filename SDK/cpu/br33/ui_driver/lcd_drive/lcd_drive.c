#include "app_config.h"
#include "includes.h"
#include "ui/ui_api.h"
#include "system/includes.h"
#include "system/timer.h"
#include "asm/spi_hw.h"
#include "asm/adv_timer.h"
#include "gpio.h"
#include "os/os_api.h"
#include "ui/lcd/lcd_drive.h"
#include "generic/rect.h"
#include "perf_counter/perf_counter.h"
#include "clock_manager/clock_manager.h"
#include "ui_draw/ui_basic.h"
#include "dmm_driver.h"
#include "periph/pwm_interface.h"
#include "tp_api.h"

#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if (TCFG_UI_ENABLE && TCFG_SPI_LCD_ENABLE)
#define lcd_debug printf

static u8  backlight_status = 0;
static u8  lcd_sleep_in     = 0;
static volatile u8 is_lcd_busy = 0;
static struct lcd_platform_data *lcd_dat = NULL;
static struct lcd_drive *__lcd = NULL;
static struct dbi_param *__this = NULL;
static void lcd_drv_cmd_list(u8 *cmd_list, int cmd_cnt);
static void lcd_drv_power_on(void *p);
static void lcd_drv_clear_screen(u32 color, int xstart, int xend, int ystart, int yend);
static OS_SEM lcd_sem;
static OS_SEM te_sem;
static int buffer_total_size;
static u8 *buffer;

static u8 *dmm_slice_buf0 = NULL;
static u8 *dmm_slice_buf1 = NULL;

struct pwm_if_handle lcd_pwm;

struct lcd_init_param {
    OS_SEM lcd_init_sem;
    OS_SEM tp_init_sem;
    u32 lcd_reinit : 1;
    u32 tp_reinit : 1;
};

struct lcd_init_param lcd_init_param_t = {0};
#define __lcd_init   (&lcd_init_param_t)

#define gpio_set_die(gpio, value)  			gpio_hw_set_die(IO_PORT_SPILT(gpio), value)
#define gpio_direction_output(gpio, value) 	gpio_hw_direction_output(IO_PORT_SPILT(gpio), value)
#define gpio_direction_input(gpio) 			gpio_hw_direction_input(IO_PORT_SPILT(gpio))
#define gpio_set_pull_up(gpio, value) 		gpio_hw_set_pull_up(IO_PORT_SPILT(gpio), value)
#define gpio_set_pull_down(gpio, value) 	gpio_hw_set_pull_down(IO_PORT_SPILT(gpio), value)
#define gpio_set_direction(gpio, value) 	gpio_hw_set_direction(IO_PORT_SPILT(gpio), value)

extern const int JLUI_USED_PSRAM_DISPLAY_BUF;
extern const int JLUI_GPU_DMA_TO_PSRAM;

// EN 控制
void lcd_en_ctrl(u8 val)
{
    if (lcd_dat == NULL) {
        return ;
    }
    enum gpio_mode mode = val ? PORT_OUTPUT_HIGH : PORT_OUTPUT_LOW;

    if ((lcd_dat->pin_en != NO_CONFIG_PORT) && (lcd_dat->pin_en_ex != NO_CONFIG_PORT) && lcd_dat->pin_en != lcd_dat->pin_en_ex) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_en),    PORT_HIGHZ);//防止电平翻转短路损坏io
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_en_ex), PORT_HIGHZ);
    }


#if 0//TCFG_LCD_TP_USE_SAME_PWR
    if (!val) { //tp和lcd共同控制电,部分模组tp 和 屏幕cs 有电阻导通，因此需要cs设置为高阻
        if (dbi_port_con->csx_en) {
            gpio_set_mode(IO_PORT_SPILT(DBI_CSX_VSYNC),    PORT_HIGHZ);
        }
    } else {
        if (dbi_port_con->csx_en) {
            gpio_set_mode(IO_PORT_SPILT(DBI_CSX_VSYNC),    PORT_OUTPUT_HIGH);
        }
    }
#endif
    enum gpio_drive_strength drive = PORT_DRIVE_STRENGT_64p0mA;
    if (lcd_dat->pin_en != NO_CONFIG_PORT) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_en), mode);
        gpio_set_drive_strength(IO_PORT_SPILT(lcd_dat->pin_en), drive);
    }
    if (lcd_dat->pin_en_ex != NO_CONFIG_PORT) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_en_ex), mode);
        gpio_set_drive_strength(IO_PORT_SPILT(lcd_dat->pin_en_ex), drive);
    }
}

// BL 控制
void lcd_bl_ctrl(u8 val)
{
    if (lcd_dat == NULL) {
        return ;
    }
    if (lcd_dat->pin_bl == NO_CONFIG_PORT) {
        return;
    }
    enum gpio_mode mode = val ? PORT_OUTPUT_HIGH : PORT_OUTPUT_LOW;
    gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_bl), mode);
}


// TE 控制
static int spi_te_stat()
{
    if (lcd_dat == NULL) {
        return -1;
    }
    if (lcd_dat->pin_te == NO_CONFIG_PORT) {
        return -1;
    }


    gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_te), PORT_INPUT_PULLUP_100K);

    return gpio_read(lcd_dat->pin_te);
}

#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
/*------------------
 * te中断配置
 * -----------------*/

static void te_signal_sync_int_handler(enum gpio_port port, u32 pin, enum gpio_irq_edge edge);

static void lcd_te_irq_init(u32 io_port_pin)
{
    if (io_port_pin == NO_CONFIG_PORT) {
        return;
    }

    struct gpio_irq_config_st lcd_te_int_irq_config = {
        .pin = NO_CONFIG_PORT,
        .irq_edge = PORT_IRQ_EDGE_FALL,
        .callback = te_signal_sync_int_handler,
        .irq_priority = 3,
    };


    u8 port_x = io_port_pin / IO_GROUP_NUM;
    u16 port_pin_x = BIT(io_port_pin % IO_GROUP_NUM);
    lcd_te_int_irq_config.pin =  port_pin_x;
    gpio_set_mode(IO_PORT_SPILT(io_port_pin), PORT_INPUT_PULLUP_10K);
    gpio_irq_config(port_x, &lcd_te_int_irq_config);
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief te_signal_sync_int_handler TE 信号中断回调
 */
/* ------------------------------------------------------------------------------------*/
static u32 te_frame_period = 16000; //先给一个最小周期的值,单位微秒
static u32 spi_frame_period = 0; //spi period先给一个最小周期的值,单位微秒
static u8 te_line_period;


/* ------------------------------------------------------------------------------------*/

//待整理

volatile int64_t dc_te_absolute_us;
volatile int64_t dc_start_absolute_us;
volatile  int  lcd_spi_last_us   = 25 * 1000;
u8 lcd_flush_num = 1; // 分块刷屏，一帧最少要刷几次

/* ------------------------------------------------------------------------------------*/



u32 lcd_get_te_frame_period_us(void)
{
    return te_frame_period;
}

u32 lcd_get_te_phase_us()
{
    int  lcd_te_display_line = 0x0;
    if (__this) {
        lcd_te_display_line = __this->te_offset_line;
    }
    //判断spi 和 te相位
    return te_line_period * lcd_te_display_line;
}


u32 lcd_get_spi_frame_period_us(void)
{
    if (!spi_frame_period) {
        float lcd_get_actual_fps();
        spi_frame_period = 1000000.0f / lcd_get_actual_fps() + (lcd_flush_num - 1) * 50;
    }
    return spi_frame_period;
}

u8 lcd_get_te_line_period_us(void)
{
    return te_line_period;
}

__attribute__((weak)) void lcd_te_sync_int_handler(void)
{
}

__attribute__((weak)) void lv_lcd_te_sync_int_handler(void)
{
}

static void te_signal_sync_int_handler(enum gpio_port port, u32 pin, enum gpio_irq_edge edge)
{


#if CONFIG_LVGL_UI_ENABLE

    lv_lcd_te_sync_int_handler();

#else

    lcd_te_sync_int_handler();

#endif

    /* 注册了上升、下降沿中断，如果上升沿则忽略、下降沿则post消息 */
    /*if (!spi_te_stat()) */
    /* { */
    /* os_sem_post(&te_sem);  */

    /* } */



#if 1 //跑几次，取最小值即可，然后可屏蔽代码写死TE行周期变量
    static u32 last_te_trig_time;
    static u8 statistics_cnt;
    if (statistics_cnt < 8) {
        if (last_te_trig_time == 0) {
            last_te_trig_time = jiffies_usec();
            return ;
        }

        ++statistics_cnt;

        u32 te_frame_period_new = jiffies_usec2offset(last_te_trig_time, jiffies_usec());
        te_frame_period = ((te_frame_period * statistics_cnt) + (te_frame_period_new * (10 - statistics_cnt))) / 10;
        last_te_trig_time = jiffies_usec();
        if (te_line_period != te_frame_period / (__this->lcd_height + 10)) {
            te_line_period =  te_frame_period / (__this->lcd_height + 10);
            printf("update line_period = %dus  \r\n", te_line_period);
        }
        printf("te_interupt frame_period = %dus, line_period = %dus  \r\n", te_frame_period, te_line_period);
    }
#endif

}


int lcd_te_detect(u16 gpio)
{
    int last_io_level = 0;
    int curr_io_level = 0;
    int io_level_change_count = 0;
    unsigned long begin_time = jiffies_msec();

    gpio_set_die(gpio, 1);
    gpio_set_direction(gpio, 1);
    while (jiffies_msec2offset(begin_time, jiffies_msec()) < 100) {
        curr_io_level = gpio_read(gpio);
        if (curr_io_level != last_io_level) {
            io_level_change_count++;
            if (io_level_change_count > 6) {
                printf("detect te singal, %d ms change %d times\n", (u32)(jiffies_msec() - begin_time), io_level_change_count);
                return 1;
            }
        }
        last_io_level = curr_io_level;
    }
    printf("don't detect te singal\n");
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_te_signal_sync_init TE信号同步初始化
 * 1、注册TE中断回调
 * 2、注册TE等待回调
 * 3、创建te_sem信号量
 */
/* ------------------------------------------------------------------------------------*/
void lcd_te_signal_sync_init()
{
    te_line_period = te_frame_period / __this->lcd_height; //预置一个最小情况的TE行周期，防止刚开始推屏的时候用到
#if (!TCFG_LCD_TE_SYNC_HW_ENABLE)
    lcd_te_irq_init(TCFG_LCD_TE_IO);
#else
    if (lcd_te_detect(TCFG_LCD_TE_IO)) {
        lcd_set_te_detect_gpio(TCFG_LCD_TE_IO, TE_SYNC_FALLING, 2000, 100);
    }
#endif
}

#endif


/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD DEVICE RESET
 *
 * Description: LCD 设备复位
 *
 * Arguments  : none
 *
 * Returns    : none
 *
 * Notes      : 1、判断是否在屏驱有重新定义LCD复位函数，
 * 					是使用屏驱上定义的LCD复位函数，
 * 					否使用GPIO控制板级配置的LCD复位IO
 *********************************************************************************************************
 */

static void lcd_reset(struct lcd_drive *lcd)
{
    if (lcd->reset) {
        lcd->reset();
    } else {
        if ((!lcd_dat) || (lcd_dat->pin_reset == NO_CONFIG_PORT)) {
            return ;
        }
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_reset), PORT_OUTPUT_HIGH);
        delay_ms(50);
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_reset), PORT_OUTPUT_LOW);
        delay_ms(15);
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_reset), PORT_OUTPUT_HIGH);
        delay_ms(120);
    }
}



/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD BACKLIGHT CONTROL
 *
 * Description: LCD 背光控制
 *
 * Arguments  : on LCD 背光开关标志，0为关，其它值为开
 *
 * Returns    : none
 *
 * Notes      : 1、判断是否在屏驱有重新定义背光控制函数，
 * 					是使用屏驱上定义的背光控制函数，
 * 					否使用GPIO控制板级配置的背光IO
 *
 *              2、配置背光状态标志，打开为true，关闭为false
 *********************************************************************************************************
 */

static void lcd_pwm_init()
{
#if (TCFG_BACKLIGHT_PWM_MODE)
    if (lcd_dat == NULL) {
        return ;
    }
    pwm_if_open(&lcd_pwm, lcd_dat->pin_bl, 0, NULL);
#endif
}

int lcd_drv_backlight_ctrl_base(u8 percent)
{
    if (__lcd->backlight_ctrl) {
        printf("__lcd->backlight_ctrl : 0x%x\n", (u32)__lcd->backlight_ctrl);
        __lcd->backlight_ctrl(percent);
    } else if (lcd_dat && lcd_dat->pin_bl != NO_CONFIG_PORT) {
#if (TCFG_BACKLIGHT_PWM_MODE == 0)
        if (percent > 0) {
            lcd_bl_ctrl(1);
        } else {
            lcd_bl_ctrl(0);
        }
#else
        if (percent > 100) {
            percent = 100;
        }
        pwm_if_set_duty(&lcd_pwm, percent);
#endif
    } else {
        return -1;
    }

    return 0;
}

int lcd_drv_backlight_ctrl(u8 percent)
{
    int ret = lcd_drv_backlight_ctrl_base(percent);
    if (ret < 0) {
        backlight_status = 0;
    } else {
        backlight_status = percent;
    }
    return ret;
}

int lcd_drv_power_ctrl(u8 on)
{
    if (!__lcd) {
        __lcd = lcd_drv_get_hdl(TCFG_LCD_MATCH_MODE, LCD_LOGO);
    }
    if (!__this) {
        __this = __lcd->param;
    }

    if (__lcd && __lcd->power_ctrl) {
        __lcd->power_ctrl(on);
    }

    return 0;
}


struct lcd_platform_data *lcd_get_platform_data()
{
    return lcd_dat;
}

static int find_begin(u8 *begin, u8 *end, int pos)
{
    u8 *p = &begin[pos];
    while ((p + 3) < end) {
        if ((p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]) == BEGIN_FLAG) {
            return (&p[4] - begin);
        }
        p++;
    }

    return -1;
}


static int find_end(u8 *begin, u8 *end, int pos)
{
    u8 *p = &begin[pos];
    while ((p + 3) < end) {
        if ((p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]) == END_FLAG) {
            return (&p[0] - begin);
        }
        p++;
    }

    return -1;
}
#define  LCD_CMD_DEBUG 1
void lcd_drv_cmd_list(u8 *cmd_list, int cmd_cnt)
{
    int i;
    int k;
    u8 *p8;

    u8 *temp = NULL;
    u16 temp_len = 1024;
    u16 len;
    temp = (u8 *)malloc(temp_len);

    for (i = 0; i < cmd_cnt;) {
        int begin = find_begin(cmd_list, &cmd_list[cmd_cnt], i);
        int cnt = 0;
        if ((begin != -1)) {
            int end = find_end(cmd_list, &cmd_list[cmd_cnt], begin);
            if (end != -1) {
                p8 = (u8 *)&cmd_list[begin];
                u8 *param;
                u32 addr;
                if (lcd_get_param(CMD_MODE) == CMD_24BIT) {
                    cnt = end - begin - 3;
                    param = &p8[3];
                    addr = (p8[0] << 16) | (p8[1] << 8) | p8[2];
                } else if (lcd_get_param(CMD_MODE) == CMD_16BIT) {
                    cnt = end - begin - 2;
                    param = &p8[2];
                    addr = (p8[0] << 8) | p8[1];
                } else {
                    cnt = end - begin - 1;
                    param = &p8[1];
                    addr = p8[0];
                }

                if (((p8[0] << 24) | (p8[1] << 16) | (p8[2] << 8) | p8[3]) == REGFLAG_DELAY_FLAG) {
                    printf("delay %d ms\n", p8[4]);
                    delay_ms(p8[4]);
                } else if (((p8[0] << 24) | (p8[1] << 16) | (p8[2] << 8) | p8[3]) == REGFLAG_CONFIRM_FLAG) {
                    u8 addr1 = p8[4];
                    u8 value = p8[5];
                    u8 timeout = p8[6];

                    printf("addr : 0x%x, value : 0x%x, timeout : %d\n", addr1, value, timeout);

                    u8 power_mode;
                    u32 jiffies_begin = jiffies_msec();
                    int wait_timeout = jiffies + msecs_to_jiffies(timeout); //超时时间设置
                    while (1) {
                        if (time_after(jiffies, wait_timeout)) {
                            printf("confirm fail! lcd power_mode status 0x%x wait timeout\n", value);
                            break;
                        }

                        lcd_read_cmd(addr, &power_mode, sizeof(power_mode));
                        if (power_mode == value) {
                            cnt++;
                            if (cnt > 10) {
                                break;
                            }
                        } else {
                            cnt = 0;
                        }
                    }
                    u32 jiffies_end = jiffies_msec();
                    printf("0x%x wait %d ms\n", addr1, jiffies_end - jiffies_begin);
                }  else {
                    int retval = lcd_write_cmd(addr, param, cnt);
#if LCD_CMD_DEBUG
                    if (temp) {
                        len = sprintf((char *)temp, "send : 0x%08x(%d), ", addr, cnt);
                        for (k = 0; k < cnt; k++) {
                            len += sprintf((char *)&temp[len], "0x%02x, ", param[k]);
                            if (len > (temp_len - 10)) {
                                len += sprintf((char *)&temp[len], "...");
                                break;
                            }
                        }

                        if (__this->lcd_type == LCD_TYPE_MIPI) {
                            if (retval >= 0) {
                                if (retval == 0) {
                                    len += sprintf((char *)&temp[len], "no bta\n");
                                } else if (retval == BTA_ACK_RX_ACK) {
                                    len += sprintf((char *)&temp[len], "bta success, rx ack\n");
                                } else {
                                    len += sprintf((char *)&temp[len], "bta success, error_report : 0x%x\n", retval);
                                }
                            } else {
                                len += sprintf((char *)&temp[len], "bta fail\n");
                            }
                        } else {
                            len += sprintf((char *)&temp[len], "\n");
                        }
                        if (len <= temp_len) {
                            printf("cmd:%s", temp);
                        }
                    }
#endif
                }

                i = end + 4;
            }
        }
    }
    if (temp) {
        free(temp);
    }
}



/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD DEVICE MATCH
 *
 * Description: LCD 设备初始化
 *
 * Arguments  : 匹配模式， 屏驱LOGO
 *
 * Returns    : 屏设备句柄
 *
 * Notes      : 1、当只有一个屏驱时，不进行匹配，直接返回该屏驱句柄
 *
 *              2、两种匹配方式 LOGO 或者 ID
 *
 *              3、若存在屏设备句柄时直接返回
 *********************************************************************************************************
 */
struct lcd_drive *lcd_drv_get_hdl(u8 mode, const char *logo)
{
    if (__lcd) {
        return __lcd;
    } else {
        if (mode == LCD_MATCH_BY_LOGO) {
            return lcd_drv_get_hdl_by_logo(logo);
        } else {
            return lcd_drv_get_hdl_by_id();
        }
    }
}

int __attribute__((weak)) ui_gpu_buf_max_size_set(int max_size)
{
    return 0;
}
/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD DEVICE INIT
 *
 * Description: LCD 设备初始化
 *
 * Arguments  : *p 板级配置的 LCD SPI 信息
 *
 * Returns    : 0 初始化成功
 * 				-1 初始化失败
 *
 * Notes      : 1、判断是否在板级文件配置SPI，是继续，否进入断言，
 *
 *              2、配置SPI可操作IO给IMD操作
 *
 *              3、LCD设备复位
 *
 *              4、SPI模块初始化，IMD模块初始化
 *********************************************************************************************************
 */
void lcd_drv_init(void *p)
{
    lcd_debug("lcd_drv_init ...\n");
    struct ui_devices_cfg *cfg = (struct ui_devices_cfg *)p;
    /* lcd_dat = (struct lcd_platform_data *)cfg->private_data; */
    if (lcd_dat == NULL) {
        lcd_dat = zalloc(sizeof(struct lcd_platform_data));
        ASSERT(lcd_dat);
    }
    memcpy(lcd_dat, cfg->private_data, sizeof(struct lcd_platform_data));

    ASSERT(lcd_dat, "Error! spi io not config");
    lcd_debug("spi pin rest:%d, en:%d\n", \
              lcd_dat->pin_reset, lcd_dat->pin_en);

    //可通过屏幕logo或者屏幕id匹配屏驱，任选一种匹配方式
    if (!__lcd) {
        __lcd = lcd_drv_get_hdl(TCFG_LCD_MATCH_MODE, LCD_LOGO);
        ASSERT(__lcd, ", don't find lcd_device");
    }
    if (!__this) {
        __this = __lcd->param;
        ASSERT(__this, ", don't find dbi_param");
    }

    /* 如果有使能IO，设置使能IO输出高电平 */
    lcd_en_ctrl(true);

    /* 把 CS、DC IO 的控制配置到IMD */
    if (__lcd->row_addr_align && __lcd->column_addr_align) {
        lcd_set_align(__lcd->row_addr_align, __lcd->column_addr_align);
    } else {
        lcd_set_align(1, 1);
    }


#if 0//LCD_POWER_DOWN_EN
    /* 初始化imd和硬件SPI等 */
    lcd_init(__this);

#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
    lcd_te_signal_sync_init();
#endif

    lcd_drv_power_on((void *) 1);

    lcd_pwm_init();

    return;
#endif

    /* lcd复位 */
    if (!(lcd_dat->dbi_hw.mipi.lcd_reset_enter && lcd_dat->dbi_hw.mipi.lcd_reset_exit)) {
        lcd_reset(__lcd);
    }

    /* 板级相关配置 */
    if (lcd_dat->board_pre_init) {
        lcd_dat->board_pre_init();
    }
    /* 初始化imd和硬件SPI等 */
    lcd_init(__this, &lcd_dat->dbi_hw);

    int buf_size = (__this->buffer_size + 3) / 4 * 4;
    buf_size = buf_size / 2;
    ui_gpu_buf_max_size_set(buf_size);
    u8 flush_line = buf_size / __this->lcd_width; //每次刷多少行
    if (flush_line > __this->lcd_width) {
        flush_line = __this->lcd_width;
    }
    lcd_flush_num = (__this->lcd_width + flush_line - 1) / flush_line;

#if (defined TCFG_LCD_CLK_DRIVE_STRENGT)
    gpio_set_drive_strength(IO_PORT_SPILT(TCFG_LCD_PIN_SCL), TCFG_LCD_CLK_DRIVE_STRENGT);
#endif
    /* 屏幕初始化指令 */
    lcd_drv_cmd_list(__lcd->lcd_cmd, __lcd->cmd_cnt);

#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
    lcd_te_signal_sync_init();
#endif

#if TCFG_LCD_MCU_JD5858_360x360_ENABLE || TCFG_LCD_MCU_ST7789V_240x320_ENABLE
    //mcu屏，first byte is dummy byte
    u8 id[4] = {0};
    lcd_read_cmd(0x04, id, sizeof(id));
    printf("id : %02x,%02x,%02x\n", id[1], id[2], id[3]);

    u8 power_mode[2] = {0};
    lcd_read_cmd(0x0a, power_mode, sizeof(power_mode));
    printf("power_mode : 0x%x\n", power_mode[1]);

    u8 pixel_format[2] = {0};
    lcd_read_cmd(0x0c, pixel_format, sizeof(pixel_format));
    printf("pixel_format : 0x%x\n", pixel_format[1]);
#else
    u8 id[3] = {0};
    lcd_read_cmd(0x04, id, sizeof(id));
    printf("id : %02x,%02x,%02x\n", id[0], id[1], id[2]);

    u8 power_mode = 0;
    lcd_read_cmd(0x0a, (u8 *)&power_mode, sizeof(power_mode));
    printf("power_mode : 0x%x\n", power_mode);

    u8 pixel_format = 0;
    lcd_read_cmd(0x0c, &pixel_format, sizeof(pixel_format));
    printf("pixel_format : 0x%x\n", pixel_format);
#endif

    lcd_pwm_init();
}

int lcd_dmm_link_init(uint16_t width, uint16_t height, uint8_t jstc_en, uint8_t rotate, int in_format, int out_format)
{
    dmm_reg_ccm_param_t reg_ccm_param = {0};
    dmm_reg_gamma_param_t reg_gamma_param = {0};
    dmm_reg_common_param_t reg_common_param = {0};
    dmm_reg_area_param_t reg_area_src0_param = {0};
    dmm_reg_area_param_t reg_area_src1_param = {0};
    dmm_reg_stride_param_t reg_stride_param = {0};
    dmm_reg_adr_param_t reg_adr_param = {0};

    u8 adr_mode = 1;

    if (dmm_slice_buf0 && dmm_slice_buf1) {
        return 1;
    }

    int in_bpp = dmm_get_format_bpp(in_format) / 8;
    int out_bpp = dmm_get_format_bpp(out_format) / 8;
    int in_stride = width * in_bpp;
    if (jstc_en) {
        in_stride = width / 8 * ((in_format == JLDMM_FORMAT_ARGB8888) ? 96 : 64);
        adr_mode = 0;
    } else if (rotate) {
        adr_mode = 0;
    }
    int out_stride = width * out_bpp;
    JLDMM_init();
    JLDMM_irq_init();
    reg_ccm_param.matrix[0] = 256;
    reg_ccm_param.matrix[5] = 256;
    reg_ccm_param.matrix[10] = 256;
    /* set_rgb_gain_ccm_param(1.5, 2.0, 1.0, &reg_ccm_param); */
    //set_brightenss_ccm_param(1.5, &reg_ccm_param);
    //set_contrast_ccm_param(1.5,128, &reg_ccm_param);
    //set_saturation_ccm_param(1.8, &reg_ccm_param);
    //set_hue_ccm_param(30, &reg_ccm_param);
    for (int k = 0; k < 256; k++) {
        reg_gamma_param.gamma[k] = k;
    }
    reg_common_param.in_adr_mode = adr_mode;
    if (jstc_en == 0 && adr_mode == 0) {
        in_stride = width * dmm_get_format_bpp(in_format) / 8 * ((in_format <= 0x0e) ? 8 : 16);
    }
    reg_common_param.in_color_fmt = in_format;
    reg_common_param.in_jstc_en = jstc_en;
    reg_common_param.in_src_mode = 0; //0:src0 1:左右 2上下 3:src1

    //spi屏和mcu屏采用单帧模式，其余的屏是连续帧模式
    reg_common_param.continue_mode = (__this->lcd_type == LCD_TYPE_SPI || __this->lcd_type == LCD_TYPE_MCU) ? 0 : 1;

    /* SPI/MCU 屏下 __lcd_dbi_link_run() 走的是"先 wait_done 再 run"。
     * 首帧之前没有任何帧跑过、不会有 frame end 中断来 post 信号量, 这里预置 1
     * 让首帧的 wait_done 立即返回, 不必干等 50ms 超时。
     * 本函数从第二帧起会在开头提前 return, 所以这段只执行一次 —— 这正是要的:
     * 之后的帧由上一帧的 frame end 中断正常 post。 */
    if (__this->lcd_type == LCD_TYPE_SPI || __this->lcd_type == LCD_TYPE_MCU) {
        JLDMM_set_frame_sem(1);
    }

    reg_area_src0_param.x_min = 0;
    reg_area_src0_param.x_max = width;
    reg_area_src0_param.y_min = 0;
    reg_area_src0_param.y_max = height;
    reg_stride_param.src0_stride = in_stride;
    reg_stride_param.src1_stride = 0;

    reg_stride_param.out_stride = out_stride;
    reg_common_param.out_buf_line = 7;
    reg_common_param.out_color_fmt = out_format;
    int slice_h1 = reg_common_param.out_buf_line + 1;

    if (dmm_slice_buf0 == NULL) {
        dmm_slice_buf0 = malloc(out_stride * slice_h1);
    }
    if (dmm_slice_buf1 == NULL) {
        dmm_slice_buf1 = malloc(out_stride * slice_h1);
    }
    reg_adr_param.dst_adr0 = dmm_slice_buf0;
    reg_adr_param.dst_adr1 = dmm_slice_buf1;

    dmm_set_ccm_param(&reg_ccm_param);
    dmm_set_gamma_param(&reg_gamma_param);
    dmm_set_common_param(&reg_common_param);
    dmm_set_src0_area_param(&reg_area_src0_param);
    dmm_set_src1_area_param(&reg_area_src1_param);
    dmm_set_stride_param(&reg_stride_param);
    dmm_set_dst_adr_param(&reg_adr_param);

    return 0;

}

int lcd_dmm_link_uninit(void)
{
    JLDMM_stop();

    /* EVA_DVBUS_TSEL->DPU_TID_DBI = DPU_SID_DMM; */
    EVA_DVBUS_TCON->DPU_TID_DBI = 0;
    EVA_DBI->DMA_CON.linkage_mode = 0;

    if (dmm_slice_buf0) {
        free(dmm_slice_buf0);
        dmm_slice_buf0 = NULL;
    }
    if (dmm_slice_buf1) {
        free(dmm_slice_buf1);
        dmm_slice_buf1 = NULL;
    }

    return 0;
}



/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       GET LCD DEVICE INFO
 *
 * Description: 获取 LCD 设备信息
 *
 * Arguments  : *info LCD 设备信息缓存结构体，根据结构体内容赋值即可
 *
 * Returns    : 0 获取成功
 * 				-1 获取失败
 *
 * Notes      : 1、根据参数结构体的内容，将LCD对应信息赋值给结构体元素
 *********************************************************************************************************
 */

static void lcd_drv_get_screen_info(struct lcd_info *info)
{
    /* imb的宽高 */
    ASSERT(__this);
    info->width = __this->in_width;
    info->height = __this->in_height;
    info->radius = __lcd->radius;
    info->fill_argb = __lcd->fill_argb;

    /* imb的输出格式 */
    info->color_format = __this->in_format;
    info->stride = info->width * lcd_get_input_format_bpp(info->color_format);
    info->rotate = __this->rotate;

    /* 屏幕类型 */
    info->interface = __this->lcd_type;

    info->fps = __this->fps;

    /* 对齐 */
    info->col_align = __lcd->column_addr_align;
    info->row_align = __lcd->row_addr_align;
    if (!info->col_align) {
        info->col_align = 1;
    }
    if (!info->row_align) {
        info->row_align = 1;
    }

    /* 背光状态 */
    info->bl_status = !!backlight_status;
    info->buf_num = __this->buffer_num;

    info->buffer = buffer;
    info->buffer_size = buffer_total_size;

    ASSERT(info->col_align, " = 0, lcd driver column address align error, default value is 1");
    ASSERT(info->row_align, " = 0, lcd driver row address align error, default value is 1");

    /* return 0; */
}

int lcd_get_screen_width()
{
    return __this->in_width;
}
int lcd_get_screen_height()
{
    return __this->in_height;
}
/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       MALLOC DISPLAY BUFFER
 *
 * Description: 申请 LCD 显存 buffer
 *
 * Arguments  : **buf 保存显存buffer指针
 * 				*size 保存显存buffer大小
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      : 1、根据LCD驱动中配置的显存大小和数量申请显存BUFFER
 *
 *				2、将显存buffer指针赋值给参数**buf，显存buffer大小赋值给参数*size
 *
 *				注意：buffer默认是lock状态，此时不能推屏，需由UI框架获取并写入数据后才能推屏
 *********************************************************************************************************
 */
extern u32 _LCD_BUF_STATIC_START;
extern u32 _LCD_BUF_STATIC_END;

static void lcd_drv_buffer_malloc(u8 **buf, u32 *size)
{
    /* int buf_size = ((__this->lcd_width * __this->lcd_height * 2) + 3) / 4 * 4; */
    ASSERT(__this);
    int buf_size = (__this->buffer_size + 3) / 4 * 4;	// 把buffer大小做四字节对齐

    /* printf("lcdbuf, s:0x%x, e:0x%x \n", (u32)&_LCD_BUF_STATIC_START, (u32)&_LCD_BUF_STATIC_END); */
    if (CONFIG_LCD_BUF_STATIC_RAM_LEN) {
        u32 buf_len = (u32)&_LCD_BUF_STATIC_END - (u32)&_LCD_BUF_STATIC_START;
        u32 use_len = buf_size * __this->buffer_num;
        /* printf("lcdbuf, size:0x%x, 0x%x \n", buf_len, use_len); */
        ASSERT(buf_len == use_len);
        *buf = (u8 *)&_LCD_BUF_STATIC_START;
    } else if (JLUI_USED_PSRAM_DISPLAY_BUF) {
        *buf = (u8 *)malloc_psram(buf_size * __this->buffer_num);
    } else if (TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE) {
        *buf = NULL;
    } else {
        *buf = (u8 *)malloc(buf_size * __this->buffer_num);
    }
#if (! TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE)
    if (*buf == NULL) {
        // 如果buffer申请失败
        *size = 0;
        return;
    }
#endif
    *size = buf_size * __this->buffer_num;
    buffer = *buf;
    buffer_total_size = *size;

    return;
}



/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       FREE DISPLAY BUFFER
 *
 * Description: 释放 LCD 显存 buffer
 *
 * Arguments  : *buf 显存buffer指针
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      : 1、使用memory API 释放显存buffer
 *********************************************************************************************************
 */

static void lcd_drv_buffer_free(u8 *buf)
{
    if (buf) {
        if (CONFIG_LCD_BUF_STATIC_RAM_LEN) {
        } else if (JLUI_USED_PSRAM_DISPLAY_BUF) {
            free_psram(buf);
        } else {
            free(buf);
        }
    }

    return;
}


/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD DRAW BUFFER
 *
 * Description: 把显存 buf 推送到屏幕
 *
 * Arguments  : *buf 显存buffer指针
 * 				len 显存buffer的数据量
 * 				wait 是否等待
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      : 1、使用 IMD 模块将显存buffer推给屏幕
 *********************************************************************************************************
 */
static int __lcd_dbi_link_run(u8 *buf, int xstart, int xend, int ystart, int yend)
{
    int dmm_in_format = 0;

    if (EVA_DBI->DMA_CON.linkage_mode && EVA_DVBUS_TSEL->DPU_TID_DBI == DPU_SID_DMM) {
        // DMM run
        dmm_in_format = (lcd_dat->dbi_hw.jstc_en) ? JLDMM_FORMAT_RGB888 : EVA_DBI->DMA_CON.format; //JLDMM_FORMAT_YUV420_BT601;
        lcd_dmm_link_init(__this->lcd_width, __this->lcd_height, lcd_dat->dbi_hw.jstc_en, __this->rotate, dmm_in_format, EVA_DBI->DMA_CON.format);
        /* printf("buf=0x%x dmm_src0_base=%x\n", (uint32_t)buf, EVA_DMM->IN0_BASE); */
        dmm_reg_adr_param_t reg_adr_param = {0};
        reg_adr_param.src0_adr = (uint8_t *)buf;
        if (dmm_in_format == JLDMM_FORMAT_YUV420_BT601 || dmm_in_format == JLDMM_FORMAT_YUV420_BT709) {
            reg_adr_param.src0_uv_adr = (uint8_t *)buf + (__this->lcd_width * __this->lcd_height);
        }
        dmm_set_src_adr_param(&reg_adr_param);
        /* dmm_debug_sfr_param(); */


        /* SPI/MCU 屏(单帧模式): 先等上一帧结束, 再 kick 新的一帧 */
        if (__this->lcd_type == LCD_TYPE_SPI || __this->lcd_type == LCD_TYPE_MCU) {
            JLDMM_wait_done();
            JLDMM_run();
        } else {
            JLDMM_run();
            JLDMM_wait_done();
        }


        return 1;
    } else if (EVA_DBI->DMA_CON.linkage_mode && EVA_DVBUS_TSEL->DPU_TID_DBI == DPU_SID_GPU) {
        //TODO: gpu run
        return 1;
    }
    // do nothing
    return 0;
}

static void lcd_drv_draw(u8 *buf, int xstart, int xend, int ystart, int yend)
{
    if (__lcd_dbi_link_run(buf, xstart, xend, ystart, yend)) {
        return;
    }

    lcd_draw(buf, xstart, xend, ystart, yend);
    __lcd_dbi_link_run(buf, xstart, xend, ystart, yend);
}


static void lcd_drv_draw_continue(u8 *buf, int xstart, int xend, int ystart, int yend)
{
    lcd_draw_continue(buf, xstart, xend, ystart, yend);
}




/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       GET LCD BACKLIGHT STATUS
 *
 * Description: 获取 LCD 背光状态
 *
 * Arguments  : none
 *
 * Returns    : 0 背光熄灭
 * 				1 背光点亮
 *
 * Notes      :
 *********************************************************************************************************
 */

int lcd_backlight_status()
{
    return !!backlight_status;
}

/*
 *********************************************************************************************************
 *                                       GET LCD BACKLIGHT STATUS
 *
 * Description: 获取 LCD sleep状态
 *
 * Arguments  : none
 *
 * Returns    : 0 sleep out
 * 				1 sleep in
 *
 * Notes      :
 *********************************************************************************************************
 */

int lcd_sleep_status()
{
    return lcd_sleep_in;
}

// 触摸屏sleep处理
__attribute__((weak)) void ctp_enter_sleep(void)
{
}
__attribute__((weak)) void ctp_exit_sleep(void)
{
}




/*
 * 弱函数重定义(升级复位前停止刷屏)
 * */
void update_process_before_cpu_reset(void)
{
    if (lcd_sleep_in) {

    } else {

    }
}




#if LCD_POWER_DOWN_EN

void lcd_reinit_wait_finish(void)
{
    if (__lcd_init && __lcd_init->lcd_reinit) {
        os_sem_pend(&__lcd_init->lcd_init_sem, 0);
        os_sem_del(&__lcd_init->lcd_init_sem, OS_DEL_ALWAYS);
        int ret = task_kill("lcd_init");
        ASSERT(!ret);
        __lcd_init->lcd_reinit = 0;
    }
}

static void lcd_drv_power_off()
{
    // 关闭之前检查上次是否已经完成
    lcd_reinit_wait_finish();

    /* lcd断电 */
    printf("lcd power off\n");
    lcd_drv_power_ctrl(false);
    struct lcd_platform_data *lcd_dat_t = lcd_get_platform_data();
    if (lcd_dat_t &&lcd_dat_t->pin_te != NO_CONFIG_PORT) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat_t->pin_te), PORT_HIGHZ);
    }

    if (lcd_dat_t &&lcd_dat_t->pin_reset != NO_CONFIG_PORT) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat_t->pin_reset), PORT_HIGHZ);
    }
}

static void lcd_init_task(void *p)
{
    int first_init = (int)p;
    if (!first_init) {
        /* lcd上电 */
        lcd_drv_power_ctrl(true);
    }
#if 0
    extern void eva_init_code(void);
    eva_init_code();
#endif
    /* lcd复位 */
    if (!(lcd_dat->dbi_hw.mipi.lcd_reset_enter && lcd_dat->dbi_hw.mipi.lcd_reset_exit)) {
        lcd_reset(__lcd);
    }

    /* 板级相关配置 */
    if (lcd_dat->board_pre_init) {
        lcd_dat->board_pre_init();
    }

    lcd_init(__lcd->param, &lcd_dat->dbi_hw); /* 初始化lcd控制器 */
#if 0
    extern void jlgpu_pnd_sem_init();
    jlgpu_pnd_sem_init();
    extern void gpu_hw_sfr_init();
    gpu_hw_sfr_init();
#endif
    if (lcd_dat && lcd_dat->pin_te != NO_CONFIG_PORT) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_te), PORT_INPUT_PULLUP_10K);
    }

    /* SPI发送屏幕初始化代码 */
    lcd_drv_cmd_list(__lcd->lcd_cmd, __lcd->cmd_cnt);

#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
    lcd_te_signal_sync_init();
#endif

    lcd_drv_clear_screen(0x000000, 0, __this->lcd_width - 1,  0, __this->lcd_height - 1);

    os_sem_post(&__lcd_init->lcd_init_sem);

    while (1) {
        os_time_dly(100);
    }
}

static void lcd_drv_power_on(void *p)
{
    int err;
    __lcd_init->lcd_reinit = 1;
    os_sem_create(&__lcd_init->lcd_init_sem, 0);
    err = task_create(lcd_init_task, p, "lcd_init");
    ASSERT(err == 0);
}
#endif

#if TP_POWER_DOWN_EN
extern void ctp_poweron(void);
extern void ctp_poweroff(void);

void tp_reinit_wait_finish(void)
{
    if (__lcd_init && __lcd_init->tp_reinit) {
        os_sem_pend(&__lcd_init->tp_init_sem, 0);
        os_sem_del(&__lcd_init->tp_init_sem, OS_DEL_ALWAYS);
        int ret = task_kill("tp_init");
        ASSERT(!ret);
        __lcd_init->tp_reinit = 0;
    }
}

static void tp_drv_power_off()
{
    // 关闭之前检查上次是否已经完成
    tp_reinit_wait_finish();

#if TCFG_TOUCH_PANEL_ENABLE
    TP_DEINIT();
    /* TP掉电 */
#if !TCFG_LCD_TP_USE_SAME_PWR
    ctp_poweroff();
#endif
#endif
}

static void tp_init_task(void *p)
{
    /* TP上电 */
#if !TCFG_LCD_TP_USE_SAME_PWR
    ctp_poweron();
#endif

#if TCFG_TP_BL6133_ENABLE
    extern void bl6133_test();
    extern void bl6133_int_en();

    /* TP初始化 */
    bl6133_test();

    /* TP int中断使能 */
    bl6133_int_en();
#endif

#if TCFG_TP_CST816D_ENABLE
    extern void cst816d_init();
    cst816d_init();
#endif // TCFG_TP_CST816D_ENABLE

#if TCFG_TP_FT3X68_ENABLE
    extern void fts_ts_init();
    fts_ts_init();
#endif

#if TCFG_TP_AXS5106_ENABLE
    extern void axs5106_init();
    axs5106_init();
#endif // TCFG_TP_AXS5106_ENABLE

    os_sem_post(&__lcd_init->tp_init_sem);
    while (1) {
        os_time_dly(100);
    }
}

static void tp_drv_power_on()
{
    int err;
    __lcd_init->tp_reinit = 1;
    os_sem_create(&__lcd_init->tp_init_sem, 0);
    err = task_create(tp_init_task, NULL, "tp_init");
    ASSERT(err == 0);
}
#endif

/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD SLEEP CONTROL
 *
 * Description: LCD 休眠控制
 *
 * Arguments  : enter 是否进入休眠，true 进入休眠，false 退出休眠
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      : 1、判断 LCD 是否正在使用，是等待使用结束，否进入下一步
 *
 * 				2、enter是否进入休眠，是使用LCD休眠函数进入休眠状态，否使用LCD退出休眠函数退出休眠状态
 *
 * 				3、lcd_sleep_in 记录LCD的休眠状态
 *********************************************************************************************************
 */
/* 由上层(UI)实现：返回用户设定亮度百分比(0~100)。
 * 未实现（返回 0）时，调用方回退到 backlight_status 缓存。
 * apps/watch/ui/jlui_app/ui_sys_param_api.c 提供了 strong 实现，按 VM 里的 LightLevel 档位换算。
 * 注意：本定义必须位于 lcd_sleep_ctrl() 之前 —— 后者内部的亮屏恢复流程会调用它，
 * 而本工程按 C99 编译（-Werror,-Wimplicit-function-declaration），后置定义会报隐式声明。 */
__attribute__((weak)) u8 lcd_user_brightness_percent(void)
{
    return 0;
}

int lcd_sleep_ctrl(u8 enter)
{
    struct lcd_sleep_headler *p;
    if ((!!enter) == lcd_sleep_in) {
        return -1;
    }
    while (is_lcd_busy);
    is_lcd_busy = 0x11;
#if (defined TCFG_EARPHONE_PROTOCOL) && (TCFG_EARPHONE_PROTOCOL)
    extern void set_sleep_control_status(u8 en);
    set_sleep_control_status(enter);
    void custom_client_sleep_control(u8 on);
    custom_client_sleep_control(enter);
#endif
    if (enter) {
        lcd_drv_backlight_ctrl_base(0);
        lcd_enter_sleep();
#if TCFG_TP_SLEEP_EN
        ctp_enter_sleep();
#endif /* #if TCFG_TP_SLEEP_EN */

#if TP_POWER_DOWN_EN
        tp_drv_power_off();
#endif

#if LCD_POWER_DOWN_EN == 0
        if (__lcd->entersleep) {
            __lcd->entersleep();
            lcd_sleep_in = true;
        }
#else
        if (__lcd->entersleep) {
            __lcd->entersleep();
        }
        lcd_drv_power_off();
        lcd_sleep_in = true;
#endif

        list_for_each_lcd_sleep_headler(p) {
            if (p->enter) {
                p->enter();
            }
        }

        /* 释放PSRAM帧缓存，在息屏时才释放，亮屏时不释放 */
        if (JLUI_GPU_DMA_TO_PSRAM) {
            extern void psram_frame_buf_release();
            psram_frame_buf_release();
        }

        clock_unlock("ui_show");
    } else {
        clock_lock("ui_show", clk_get_max_frequency());
        lcd_exit_sleep();
#if LCD_POWER_DOWN_EN == 0
        if (__lcd->exitsleep) {
            __lcd->exitsleep();
            lcd_sleep_in = false;
        }
#else
        lcd_drv_power_on(NULL);
        lcd_sleep_in = false;
#endif

        u8 bl = lcd_user_brightness_percent();
        if (!bl) {
            bl = backlight_status;
        }
        if (bl) {
            lcd_drv_backlight_ctrl(bl);
        }

#if TP_POWER_DOWN_EN
        tp_drv_power_on();
#endif

#if TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
        lcd_te_signal_sync_init();
#endif
#endif

#if TCFG_TP_SLEEP_EN
        ctp_exit_sleep();
#endif /* #if TCFG_TP_SLEEP_EN */

        list_for_each_lcd_sleep_headler(p) {
            if (p->exit) {
                p->exit();
            }
        }

        /* 背光已在本分支开头 lcd_drv_power_on() 之后立刻恢复，此处不再重复。 */
    }

    is_lcd_busy = 0;
    return 0;
}

void lcd_bl_open()
{
    /* lcd_pwm_init(); */
    u8 bl = lcd_user_brightness_percent();
    if (!bl) {
        bl = backlight_status;
    }
    if (bl) {
        lcd_drv_backlight_ctrl(bl);
    } else {
        /* lcd_drv_backlight_ctrl(0) 会下发 0x28(Display OFF)，而本函数语义是"开背光"，
         * 故为 0 时不下发，保持面板现状。正常情况下 bl 由档位换算恒 >= 10。 */
    }
}



/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       GET LCD DRIVE HANDLER
 *
 * Description: 获取LCD驱动句柄
 *
 * Arguments  : none
 *
 * Returns    : struct lcd_interface* LCD驱动接口句柄
 *
 * Notes      : 1、从LCD接口列表中找到LCD接口句柄并返回
 *********************************************************************************************************
 */
/* #error lcd_get_hdl */
struct lcd_interface *lcd_get_hdl()
{
    struct lcd_interface *p;

    ASSERT(lcd_interface_begin != lcd_interface_end, "don't find lcd interface!");
    for (p = lcd_interface_begin; p < lcd_interface_end; p++) {
        return p;
    }
    return NULL;
}

static void lcd_drv_set_draw_area(u16 xs, u16 xe, u16 ys, u16 ye)
{
    lcd_set_draw_area(xs, xe, ys, ye);
}

static void lcd_drv_clear_screen(u32 color, int xstart, int xend, int ystart, int yend)
{
    lcd_clear(color, xstart, xend, ystart, yend);
    lcd_wait_busy();
}

struct lcd_drive *lcd_drv_get_hdl_by_logo(const char *logo)
{
    struct lcd_drive *p;
    int lcd_type;
    int spi_mode;
    int spi_submode;
    int lcd_num = 0;

    /* printf("find logo %s\n", logo); */
    ASSERT(lcd_device_begin != lcd_device_end, "don't find lcd device!");

    //统计屏驱的个数
    for (p = lcd_device_begin; p < lcd_device_end; p++) {
        lcd_num++;
    }
    printf("find %d lcd\n", lcd_num);

    //只有一个屏驱时不进行匹配
    if (lcd_num == 1) {
        p = lcd_device_begin;
        printf("Due to lcd_num = %d, don't match any lcd device. Only one lcd device %s is selected by default.\n", lcd_num, p->logo ? p->logo : "null");
        return p;
    }

    //需确保所有使能的屏驱接口保持一致
    for (p = lcd_device_begin; p < lcd_device_end; p++) {
        if (p == lcd_device_begin) {
            lcd_type = ((struct dbi_param *)p->param)->lcd_type;
            spi_mode = ((struct dbi_param *)p->param)->spi.spi_mode & 0xf0;
            spi_submode = ((struct dbi_param *)p->param)->spi.spi_mode & 0x0f;
        } else {
            ASSERT(lcd_type == ((struct dbi_param *)p->param)->lcd_type, ", all lcd interface must the same");
            ASSERT(spi_mode == (((struct dbi_param *)p->param)->spi.spi_mode & 0xf0), ", all spi_mode must the same");
            ASSERT(spi_submode == (((struct dbi_param *)p->param)->spi.spi_mode & 0x0f), ", all spi_submode must the same");
        }
    }

    for (p = lcd_device_begin; p < lcd_device_end; p++) {
        printf("p->logo : %s\n", p->logo);
        if (p->logo && logo && !strcmp(p->logo, logo)) {
            return p;
        }
    }
    return NULL;
}

struct lcd_drive *lcd_drv_get_hdl_by_id()
{
    struct lcd_drive *p;
    int lcd_type;
    int spi_mode;
    int spi_submode;
    int lcd_num = 0;

    ASSERT(lcd_device_begin != lcd_device_end, "don't find lcd device!");

    //统计屏驱的个数
    for (p = lcd_device_begin; p < lcd_device_end; p++) {
        lcd_num++;
    }

    //只有一个屏驱时不进行匹配
    if (lcd_num == 1) {
        p = lcd_device_begin;
        printf("Due to lcd_num = %d, don't match any lcd device. Only one lcd device %s is selected by default.\n", lcd_num, p->logo ? p->logo : "null");
        return p;
    }


    //需确保所有使能的屏驱接口保持一致
    for (p = lcd_device_begin; p < lcd_device_end; p++) {
        if (p == lcd_device_begin) {
            lcd_type = ((struct dbi_param *)p->param)->lcd_type;
            spi_mode = ((struct dbi_param *)p->param)->spi.spi_mode & 0xf0;
            spi_submode = ((struct dbi_param *)p->param)->spi.spi_mode & 0x0f;
        } else {
            ASSERT(lcd_type == ((struct dbi_param *)p->param)->lcd_type, ", all lcd interface must the same");
            ASSERT(spi_mode == (((struct dbi_param *)p->param)->spi.spi_mode & 0xf0), ", all spi_mode must the same");
            ASSERT(spi_submode == (((struct dbi_param *)p->param)->spi.spi_mode & 0x0f), ", all spi_submode must the same");
        }
    }


    for (p = lcd_device_begin; p < lcd_device_end; p++) {
        struct dbi_param *this = (struct dbi_param *)p->param;

        lcd_en_ctrl(true);

        lcd_set_ctrl_pin_func(spi_te_stat);

        lcd_reset(p); /* lcd复位 */

        /* 板级相关配置 */
        if (lcd_dat->board_pre_init) {
            lcd_dat->board_pre_init();
        }
        lcd_init(this, &lcd_dat->dbi_hw); /* 初始化lcd控制器 */

        u32 lcd_read_id = 0;
        if (p->read_id) {
            lcd_read_id = p->read_id();
        }

        printf("p->logo : %s, p->lcd_id : 0x%x, read_id : 0x%x\n", p->logo ? p->logo : "null", p->lcd_id, lcd_read_id);

        if (p->lcd_id == lcd_read_id) {
            return p;
        }
    }

    return NULL;
}


int lcd_drv_get_info(void *info)
{
    struct lcd_interface *lcd;
    lcd = lcd_get_hdl();
    ASSERT(lcd);
    if (lcd->get_screen_info) {
        lcd->get_screen_info(info);
    }

    return 0;
}

REGISTER_LCD_INTERFACE(lcd) = {
    .init               = lcd_drv_init,
    .draw               = lcd_drv_draw,
    .draw_continue      = lcd_drv_draw_continue,
    .get_screen_info	= lcd_drv_get_screen_info,
    .buffer_malloc		= lcd_drv_buffer_malloc,
    .buffer_free		= lcd_drv_buffer_free,
    .backlight_ctrl		= lcd_drv_backlight_ctrl,
    .power_ctrl		    = lcd_drv_power_ctrl,
    .set_draw_area	    = lcd_drv_set_draw_area,
    .clear_screen	    = lcd_drv_clear_screen,
};


static u8 lcd_idle_query(void)
{
    return !is_lcd_busy;
}

REGISTER_LP_TARGET(lcd_lp_target) = {
    .name = "lcd",
    .is_idle = lcd_idle_query,
};


#endif /* #if (TCFG_UI_ENABLE && TCFG_SPI_LCD_ENABLE) */


