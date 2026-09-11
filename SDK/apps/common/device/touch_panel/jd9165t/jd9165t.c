#include "app_config.h"
#include "system/includes.h"
#include "gpio.h"
#include "ui/ui_api.h"
#include "clock_manager/clock_manager.h"
#include "perf_counter/perf_counter.h"

#if (defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE)
#include "ui_core.h"
#define TP_TASK_NAME                 "touch_task" ///< 触摸面板任务名
#endif

#if TCFG_TOUCH_PANEL_ENABLE
#if TCFG_TP_JD9165T_ENABLE

#if defined(TCFG_TP_PIN_IIC_CH) && (TCFG_TP_PIN_IIC_CH >= TCFG_IIC_OFFSET_SOFT)
#define _IIC_USE_SOFT
#else
#define _IIC_USE_HW
#endif
#include "iic_api.h"


#define LOG_TAG "[TP]"
#define PRINTF(format, ...)         printf(format, ## __VA_ARGS__)
#if 0
#define log_info(format, ...)       PRINTF(LOG_TAG "info:"  format, ## __VA_ARGS__)
#define log_debug(format, ...)       PRINTF(LOG_TAG "debug:" format, ## __VA_ARGS__)
#define log_error(format, ...)       PRINTF(LOG_TAG "error:" format, ## __VA_ARGS__)
#else
#define log_info(format, ...)       //PRINTF(LOG_TAG "info:"  format, ## __VA_ARGS__)
#define log_debug(format, ...)
#define log_error(format, ...)       PRINTF(LOG_TAG "error:" format, ## __VA_ARGS__)
#endif

#define abs(x)  __builtin_abs(x)//((x)>0?(x):-(x) )

// 触摸信息配置
#if (defined TCFG_LCD_QSPI_SH8501A_240x296_ENABLE && TCFG_LCD_QSPI_SH8501A_240x296_ENABLE)
#define X_MIRROR  		0
#define Y_MIRROR  		0
#define VK_X      		240
#define VK_Y            296
#define TP_FPS			60
#elif ((defined TCFG_LCD_SPI_ICNA3306_ENABLE) &&(TCFG_LCD_SPI_ICNA3306_ENABLE))
#define X_MIRROR  		0
#define Y_MIRROR  		0
#define VK_X      		240
#define VK_Y            296
#define TP_FPS			60
#elif (defined TCFG_LCD_QSPI_GC9B71_320x386_ENABLE && TCFG_LCD_QSPI_GC9B71_320x386_ENABLE)
#define X_MIRROR  		0
#define Y_MIRROR  		0
#define VK_X      		320
#define VK_Y            386
#define TP_FPS			60
#elif (defined TCFG_LCD_QSPI_ST77916_320x385_ENABLE && TCFG_LCD_QSPI_ST77916_320x385_ENABLE)
#define X_MIRROR  		0
#define Y_MIRROR  		0
#define VK_X      		320
#define VK_Y            386
#define TP_FPS			60
#elif (defined TCFG_LCD_QSPI_RAMLESS_ST77903_400x400_ENABLE && TCFG_LCD_QSPI_RAMLESS_ST77903_400x400_ENABLE)
#define X_MIRROR  		0
#define Y_MIRROR  		0
#define VK_X      		400
#define VK_Y            400
#define TP_FPS			60
#elif (defined TCFG_LCD_QSPI_JD9161C_480x480_ENABLE && TCFG_LCD_QSPI_JD9161C_480x480_ENABLE)
#define X_MIRROR  		0
#define Y_MIRROR  		0
#define VK_X      		480
#define VK_Y            480
#define TP_FPS			60
#elif (defined TCFG_LCD_GC9307_172X320 && TCFG_LCD_GC9307_172X320)
#define X_MIRROR  		1
#define Y_MIRROR  		0
#define VK_X      		320
#define VK_Y            172
#define TP_FPS			60
#elif (defined TCFG_LCD_MIPI_JD9165T_1024x600_ENABLE && TCFG_LCD_MIPI_JD9165T_1024x600_ENABLE)
#define SW_X_Y	  		1
#define X_MIRROR  		0
#define Y_MIRROR  		0
#define VK_X      		1024
#define VK_Y            600
#define TP_FPS			60
#else
#define Y_MIRROR  		1
#define X_MIRROR  		1
#define VK_X      		360
#define VK_Y            360
#define TP_FPS			60
#endif
#define VK_MIN			(VK_X < VK_Y ? VK_X : VK_Y)
#define VK_MAX			(VK_X > VK_Y ? VK_X : VK_Y)

// 滑屏阈值
#define VK_X_MOVE      	(VK_MIN/5)
#define VK_Y_MOVE   	(VK_MIN/5)
#define VK_X_Y_DIFF    	(VK_MIN/10)
#if defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
#define SINGLE_SLIDING  (10)            //保证只有一个方向滑动阈值
#endif

// 惯性阈值
#define ENERGY_XDISTANCE	4//(VK_MIN/30)
#define ENERGY_YDISTANCE	4//(VK_MIN/30)

#define ENERGY_NUM			3 //取最后n次中最大差值
#define ENERGY_T_MS			(1000000/TP_FPS/1000)

// reset gpio
#define JD9165T_RESET_H() gpio_set_mode(IO_PORT_SPILT(TCFG_TP_RESET_IO), PORT_OUTPUT_HIGH)
#define JD9165T_RESET_L() gpio_set_mode(IO_PORT_SPILT(TCFG_TP_RESET_IO), PORT_OUTPUT_LOW)

// iic
#define JD9165T_IIC_DELAY   1
#define HYN_I2C_ADDR 		0x68 /* 7-bit i2c addr */ //0x38

// tp upgrade
// 目前升级tp固件的流程还没有做，后续使用到的话需要重新修改流程
#define JD9165T_UPGRADE_ENABLE		0
#if JD9165T_UPGRADE_ENABLE
#define JD9165T_UPGRADE_ADD (0x6A) /* 7-bit i2c addr */
#define PER_LEN				512
#endif

#define I2C_MASTER_CI2C0 	0
#define i2c_t 				uint8_t

typedef enum {
    TP_RUN_STEP_STOP = 0,
    TP_RUN_STEP_INIT,
    TP_RUN_STEP_RUN_IDLE,
    TP_RUN_STEP_RUN,
} TP_RUN_STEP_E;

// iic
typedef struct {
    u8 init;
    hw_iic_dev iic_hdl;
} ft_param;

// 惯性
struct touch_kinetic_energy {
    u16 x;
    u16 y;
};

static struct touch_kinetic_energy tke[ENERGY_NUM]; // 惯性

static volatile TP_RUN_STEP_E tp_step = TP_RUN_STEP_STOP;
static u8 tp_last_staus = ELM_EVENT_TOUCH_UP;	// 状态记录
static int tp_down_cnt = 0; // 长按计数
static bool touch_flag = 0; // 触摸标记
static OS_SEM touch_sem; // 触摸中断post

static ft_param module_param = {
#ifdef TCFG_TP_PIN_IIC_CH
#if (TCFG_TP_PIN_IIC_CH >= TCFG_IIC_OFFSET_SOFT)
    .iic_hdl = TCFG_TP_PIN_IIC_CH - TCFG_IIC_OFFSET_SOFT,
#else
    .iic_hdl = TCFG_TP_PIN_IIC_CH,
#endif
#else
    .iic_hdl = 0,
#endif
}; // iic
#define __this   (&module_param)

#if CONFIG_LVGL_UI_ENABLE
struct point {
    int x;
    int y;
};
typedef struct {
    struct point point;
    int state;
} lv_touch_data_t;
static lv_touch_data_t g_touch_data;

#define LV_INDEV_STATE_PR  0
#define LV_INDEV_STATE_REL 1
static int vk_state = LV_INDEV_STATE_REL;

#endif /* #if CONFIG_LVGL_UI_ENABLE */


static void jd9165t_reset();

static void ctp_delay_us(unsigned int time)
{
    udelay(time);
}
static void ctp_delay_ms(unsigned short time)
{
    if (time >= 10) {
        os_time_dly(time / 10 + 1);
    } else {
        mdelay(time);
    }
}

/*------------------
 * tp中断配置
 * -----------------*/
static void tp_irq_callback(enum gpio_port port, u32 pin, enum gpio_irq_edge edge)
{
    log_debug("port%d.%d:%d-cb1\n", port, pin, edge);

    touch_flag = 1;
    os_sem_post(&touch_sem);
}

struct gpio_irq_config_st tp_int_irq_config = {
    .pin = NO_CONFIG_PORT,
    .irq_edge = PORT_IRQ_EDGE_FALL,
    .callback = tp_irq_callback,
    .irq_priority = 3,
};

void tp_irq_init(void)
{
    u8 port_x = TCFG_TP_INT_IO / IO_GROUP_NUM;
    u16 port_pin_x = BIT(TCFG_TP_INT_IO % IO_GROUP_NUM);
    tp_int_irq_config.pin =  port_pin_x;
    log_debug("[%s] port_x:%x, port_pin_x:%x", __func__, port_x, port_pin_x);
    gpio_set_mode(IO_PORT_SPILT(TCFG_TP_INT_IO), PORT_INPUT_PULLUP_10K);
    gpio_irq_config(port_x, &tp_int_irq_config);
}

void tp_irq_deinit(void)
{
    gpio_irq_deinit(IO_PORT_SPILT(TCFG_TP_INT_IO));
    gpio_set_mode(IO_PORT_SPILT(TCFG_TP_INT_IO), PORT_HIGHZ);
}

static int i2c_write_regs(i2c_t dev, uint16_t address, uint16_t reg,
                          void *data, size_t length, uint8_t flags)
{
    int ret;
    /* printf("i2c_read_regs_ex.\n"); */

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, address << 1)) {
        iic_stop(__this->iic_hdl);
        log_error("iic_tx_byte fail1!\n");
        return -1;
    }
    ctp_delay_us(JD9165T_IIC_DELAY);
    /* ret = iic_write_buf(__this->iic_hdl, reg & 0xff, 1); */
    iic_tx_byte(__this->iic_hdl, reg);
    ctp_delay_us(JD9165T_IIC_DELAY);

    ctp_delay_us(JD9165T_IIC_DELAY);
    ret = iic_write_buf(__this->iic_hdl, data, length);
    ctp_delay_us(JD9165T_IIC_DELAY);
    iic_stop(__this->iic_hdl);
    ctp_delay_us(JD9165T_IIC_DELAY);

    return 0;
}

static int i2c_read_regs(i2c_t dev, uint16_t address, uint16_t reg,
                         void *data, size_t length, uint8_t flags)
{
    int ret;
    /* printf("i2c_read_regs_ex.\n"); */

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, address << 1)) {
        iic_stop(__this->iic_hdl);
        log_error("iic_tx_byte fail2!\n");
        return -1;
    }
    ctp_delay_us(JD9165T_IIC_DELAY);
    /* ret = iic_write_buf(__this->iic_hdl, reg & 0xff, 1); */
    iic_tx_byte(__this->iic_hdl, reg);
    ctp_delay_us(JD9165T_IIC_DELAY);

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, (address << 1) | 0x01)) {
        iic_stop(__this->iic_hdl);
        return -1;
    }

    ctp_delay_us(JD9165T_IIC_DELAY);
    ret = iic_read_buf(__this->iic_hdl, data, length);
    ctp_delay_us(JD9165T_IIC_DELAY);
    iic_stop(__this->iic_hdl);
    ctp_delay_us(JD9165T_IIC_DELAY);

    return 0;
}

#if JD9165T_UPGRADE_ENABLE
static int i2c_read_regs_ex(i2c_t dev, uint16_t address, void *reg,
                            void *data, size_t length, uint8_t flags)
{
    int ret;
    /* printf("i2c_read_regs_ex.\n"); */

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, address << 1)) {
        iic_stop(__this->iic_hdl);
        log_error("iic_tx_byte fail3!\n");
        return -1;
    }
    ctp_delay_us(JD9165T_IIC_DELAY);
    ret = iic_write_buf(__this->iic_hdl, reg, 2);
    ctp_delay_us(JD9165T_IIC_DELAY);

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, (address << 1) | 0x01)) {
        iic_stop(__this->iic_hdl);
        return -1;
    }

    ctp_delay_us(JD9165T_IIC_DELAY);
    ret = iic_read_buf(__this->iic_hdl, data, length);
    ctp_delay_us(JD9165T_IIC_DELAY);
    iic_stop(__this->iic_hdl);
    ctp_delay_us(JD9165T_IIC_DELAY);

    return 0;
}

static int i2c_write_regs_ex(i2c_t dev, uint16_t address, void *reg,
                             const void *data, size_t length, uint8_t flags)
{
    int ret;
    int i;
    u8 *_reg = (u8 *)reg;
    u8 *_data = (u8 *)data;

    /* printf("i2c_write_regs_ex. reg:0x%02x%02x\n",_reg[0],_reg[1]); */
    /* printf("data:\n"); */
    /* for(i=0;i<length;i++) { */
    /* printf("%02x ",_data[i]); */
    /* } */
    /* printf("\n"); */
    /* printf("address = 0x%x\n",address<<1); */

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, address << 1)) {
        iic_stop(__this->iic_hdl);
        log_error("iic_tx_byte fail4!\n");/*tp升级时，boot_mode有时间窗口限制，会打印识别，看最终能否成功就行*/
        return -1;
    }
    /* ret = iic_write_buf(__this->iic_hdl, reg, 2); */
    iic_tx_byte(__this->iic_hdl, _reg[0]);
    iic_tx_byte(__this->iic_hdl, _reg[1]);
    /* printf("ret = %d\n",ret); */
    ctp_delay_us(JD9165T_IIC_DELAY);
    ret = iic_write_buf(__this->iic_hdl, data, length);
    /* printf("ret = %d\n",ret); */
    ctp_delay_us(JD9165T_IIC_DELAY);
    iic_stop(__this->iic_hdl);
    ctp_delay_us(JD9165T_IIC_DELAY);

    return 0;
}

static u16 ctp_flash_iic_write(u8 slave_addr, u8 *buf, u16 len)
{
    u16 i = 0;
    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, slave_addr)) {
        iic_stop(__this->iic_hdl);
        return 0;
    }

    for (i = 0; i < len; i++) {
        ctp_delay_us(JD9165T_IIC_DELAY);
        if (0 == iic_tx_byte(__this->iic_hdl, buf[i])) {
            iic_stop(__this->iic_hdl);
            return i;
        }
    }
    iic_stop(__this->iic_hdl);
    return i;
}

//return =len:ok; other:fail
static u16 ctp_flash_iic_read(u8 slave_addr, u8 *buf, u16 len)
{
    u16 i = 0;
    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, slave_addr + 1)) {
        iic_stop(__this->iic_hdl);
        return 0;
    }
    for (i = 0; i < len - 1; i++) {
        ctp_delay_us(JD9165T_IIC_DELAY);
        *buf++ = iic_rx_byte(__this->iic_hdl, 1, NULL);
    }
    i++;
    *buf = iic_rx_byte(__this->iic_hdl, 0, NULL);
    iic_stop(__this->iic_hdl);
    return i;
}


static int jd9165t_enter_bootmode(void)
{
    uint8_t retryCnt = 10;

#if 0
    gpio_output_set(GPIO15, 0);
    uos_sleep(4);/* 20 ms */
    gpio_output_set(GPIO15, 1);
    uos_sleep(1);/* 5ms */
#else
    jd9165t_reset();
#endif
    log_debug("%s %d \r\n", __FUNCTION__, __LINE__);

    while (retryCnt--) {
        uint8_t cmd[3];
        cmd[0] = 0xAB;

        unsigned char reg[2] = {0xA0, 0x01};
        if (0 == system_iic_write_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg, 2, cmd, 1)) { // enter program mode
            ctp_delay_ms(2); // 4ms
            continue;
        }

        unsigned char reg1[2] = {0xA0, 0x03};
        if (0 == system_iic_read_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg1, 2, cmd, 1)) { // read flag
            ctp_delay_ms(2); // 4ms
            continue;
        } else {
            log_debug("cmd[0] = 0x%02x\n", cmd[0]);
            if (cmd[0] != /*0x55*/0xc1) {
                ctp_delay_ms(2); // 4ms
                continue;
            } else {
                /* printf("%s %d \r\n", __FUNCTION__, __LINE__); */
                log_info("jd9165t_enter_bootmode succ!\n");
                return 0;
            }
        }
    }
    log_debug("%s %d retryCnt %d\r\n", __FUNCTION__, __LINE__, retryCnt);
    return -1;
}

static int jd9165t_update(uint16_t startAddr, uint16_t len, uint8_t *src)
{
    uint16_t sum_len;
    uint8_t cmd[10];

    if (jd9165t_enter_bootmode() == -1) {
        //printf("%s %d \r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    sum_len = 0;
    do {
        if (sum_len >= len) {
            return -1;
        }

        // send address
        cmd[0] = startAddr & 0xFF;
        cmd[1] = startAddr >> 8;

        unsigned char reg0[2] = {0xA0, 0x14};
        system_iic_write_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg0, 2, cmd, 2);
        unsigned char reg1[2] = {0xA0, 0x18};
        system_iic_write_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg1, 2, src, PER_LEN);

        cmd[0] = 0xEE;
        unsigned char reg2[2] = {0xA0, 0x04};
        system_iic_write_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg2, 2, cmd, 1);
        ctp_delay_ms(100);
        //printf("%s %d \r\n", __FUNCTION__, __LINE__);
        uint8_t retrycnt = 50;
        while (retrycnt--) {
            cmd[0] = 0;
            unsigned char reg_read[2] = {0xA0, 0x05};
            system_iic_read_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg_read, 2, cmd, 1);
            //printf("%s %d \r\n", __FUNCTION__, __LINE__);
            if (cmd[0] == 0x55) {
                // success
                break;
            }
            ctp_delay_ms(10);
        }

        startAddr += PER_LEN;
        src += PER_LEN;
        sum_len += PER_LEN;
        //printf("%s %d \r\n", __FUNCTION__, __LINE__);
    } while (len);

    // exit program mode
    cmd[0] = 0x00;
    unsigned char reg3[2] = {0xA0, 0x03};
    system_iic_write_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg3, 2, cmd, 1);

    return 0;
}

static uint32_t jd9165t_read_checksum(uint16_t startAddr, uint16_t len)
{
    union {
        uint32_t sum;
        uint8_t buf[4];
    } checksum;

    unsigned char cmd[3];
    char readback[4] = {0};

    if (jd9165t_enter_bootmode() == -1) {
        return -1;
    }

    cmd[0] = 0;
    unsigned char reg[2] = {0xA0, 0x03};
    if (0 == system_iic_write_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg, 2, cmd, 1)) {
        return -1;
    }
    ctp_delay_ms(500);

    checksum.sum = 0;
    unsigned char reg_read[2] = {0xA0, 0x08};
    if (0 == system_iic_read_nbytes(__this->iic_hdl, JD9165T_UPGRADE_ADD, reg_read, 2, checksum.buf, 2)) {
        return -1;
    }
    log_debug("%s checksum.sum 0x%x \r\n", __FUNCTION__, checksum.sum);
    return checksum.sum;
}

static bool jd9165t_upgrade(void)
{
    uint8_t lvalue;

    if (jd9165t_enter_bootmode() == 0) {
        log_debug("%s %d \r\n", __FUNCTION__, __LINE__);
        if (sizeof(app_bin) > 10) {
            log_debug("%s %d \r\n", __FUNCTION__, __LINE__);
            uint16_t startAddr = app_bin[1];
            uint16_t length = app_bin[3];
            uint16_t checksum = app_bin[5];
            startAddr <<= 8;
            startAddr |= app_bin[0];
            length <<= 8;
            length |= app_bin[2];
            checksum <<= 8;
            checksum |= app_bin[4];

            log_debug("checksum = 0x%x\n", checksum);
            if (jd9165t_read_checksum(startAddr, length) != checksum) {
#if CONFIG_JL_UI_ENABLE
                ui_auto_shut_down_disable();//防止息屏时间太短导致升级出现异常
#endif
                jd9165t_update(startAddr, length, (uint8_t *)(app_bin + 6));
                jd9165t_read_checksum(startAddr, length);
#if CONFIG_JL_UI_ENABLE
                ui_auto_shut_down_enable();
#endif
                log_debug("%s %d \r\n", __FUNCTION__, __LINE__);
            }
        }
        return true;
    }

    return false;
}
#endif

static void jd9165t_reset()
{
    JD9165T_RESET_H();
    log_debug("jd9165t_reset_h\n");
    ctp_delay_ms(5);
    JD9165T_RESET_L();
    log_debug("jd9165t_reset_l\n");
    ctp_delay_ms(20);
    JD9165T_RESET_H();
    log_debug("jd9165t_reset_h\n");
    //ctp_delay_us(5000);//5ms
    ctp_delay_ms(10);
}

static void ctp_sleep_jd9165t(void)
{
    JD9165T_RESET_L();          // RESET 拉低
    ctp_delay_ms(10);

    system_iic_deinit(__this->iic_hdl);

    gpio_set_mode(IO_PORT_SPILT(TCFG_TP_INT_IO), PORT_HIGHZ);
    gpio_set_mode(IO_PORT_SPILT(TCFG_TP_RESET_IO), PORT_HIGHZ);

    log_info("ctp_sleep_jd9165t");
}

static void ctp_wakeup_jd9165t(void)
{
    /* #ifdef  RESET_PIN_WAKEUP */
    jd9165t_reset();
    gpio_set_mode(IO_PORT_SPILT(TCFG_TP_INT_IO), PORT_INPUT_PULLUP_10K);
    system_iic_init(__this->iic_hdl, get_iic_config(__this->iic_hdl));
    /* #endif */
    /* #ifdef INT_PIN_WAKEUP */
    /*     bl_ts_int_wakeup(); */
    /* #endif */
    /* bl_ts_set_intmode(1);//开io中断 */
    log_info("ctp_wakeup_jd9165t");
}


#if (!CONFIG_LVGL_UI_ENABLE)

static void tke_start(int x, int y)
{
    memcpy(&tke[0], &tke[1], sizeof(struct touch_kinetic_energy) * (ENERGY_NUM - 1));

    tke[ENERGY_NUM - 1].x = x;
    tke[ENERGY_NUM - 1].y = y;
    log_info("tke start %d, %d\n", x, y);
}

static void tke_stop(int x, int y, struct touch_event *up_t)
{
    struct touch_event t = {0};
    int tke_x = 0;
    int tke_y = 0;
    u8 xdir, ydir;

    for (int i = 0; i < (ENERGY_NUM - 1); i++) {
        if ((tke[i].x == 0xffff) || (tke[i].y == 0xffff)) {
            continue;
        }
        int tmp_x = (int)tke[i + 1].x - (int)tke[i].x;
        int tmp_y = (int)tke[i + 1].y - (int)tke[i].y;
        if (abs(tke_x) < abs(tmp_x)) {
            tke_x = tmp_x;
        }
        if (abs(tke_y) < abs(tmp_y)) {
            tke_y = tmp_y;
        }
        log_info("[%d], x:0x%x, y:0x%x", i, tke[i].x, tke[i].y);
        log_info("stke %d, %d\n", tke_x, tke_y);
    }
    log_info("stke end %d, %d\n", tke_x, tke_y);

    if (((abs(tke_x) >= ENERGY_XDISTANCE) || abs(tke_y) >= ENERGY_YDISTANCE)) {
        up_t->has_energy = 1;
        ui_touch_msg_post(up_t);

        if (abs(tke_x) < ENERGY_XDISTANCE) {
            tke_x = 0;
        }
        if (abs(tke_y) < ENERGY_YDISTANCE) {
            tke_y = 0;
        }
        xdir = (tke_x < 0) ? 1 : 2;
        ydir = (tke_y < 0) ? 1 : 2;
        /* printf("xdir %d, ydir %d\n", xdir, ydir); */

        t.event = ELM_EVENT_TOUCH_ENERGY;
        t.x = (tke_x << 16) | (ENERGY_T_MS & 0xffff);
        t.y = (tke_y << 16) | (ydir << 8) | (xdir & 0xff);
        ui_touch_msg_post(&t);

        /* printf("tke out %d, %d, %d, %d, %d\n", (t.x & 0xffff), (t.x >> 16), (t.y >> 16), (t.y & 0xff), (t.y >> 8) & 0xff); */

    } else {
        //在这里发送up消息，因为没有惯性，所以直接发送
        ui_touch_msg_post(up_t);
    }

    memset(&tke, 0xff, sizeof(tke));

}


static void tpd_down(int raw_x, int raw_y, int x, int y, int p)
{
    struct touch_event t;
    static int first_x = 0;
    static int first_y = 0;
    static u8 move_flag = 0;

    if (x < 0) {
        x = 0;
    }
    if (x > (VK_X - 1)) {
        x = VK_X - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > (VK_Y - 1)) {
        y = VK_Y - 1;
    }
#if Y_MIRROR
    x = VK_X - x - 1;
#endif

#if X_MIRROR
    y = VK_Y - y - 1;
#endif

    if ((tp_last_staus == ELM_EVENT_TOUCH_DOWN) && (x == first_x) && (y == first_y)) {
        tp_down_cnt++;
        if (tp_down_cnt < 30) {
            return;
        }
        tp_last_staus = ELM_EVENT_TOUCH_HOLD;
        tp_down_cnt = 0;

        t.event = tp_last_staus;
        t.x = x;
        t.y = y;
        ui_touch_msg_post(&t);
        return;
    }

    log_info("D[%4d %4d %4d]\n", x, y, p);
    if (tp_last_staus != ELM_EVENT_TOUCH_UP) {
        int x_move = abs(x - first_x);
        int y_move = abs(y - first_y);

        if (!move_flag && (x_move >= VK_X_MOVE || y_move >= VK_Y_MOVE) && (abs(x_move - y_move) >= VK_X_Y_DIFF)) {
            if (x_move > y_move) {
                if (x > first_x) {
#if TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
                    if (y_move < SINGLE_SLIDING) {
                        tp_last_staus = ELM_EVENT_TOUCH_R_MOVE;
                    }
#else
                    tp_last_staus = ELM_EVENT_TOUCH_R_MOVE;
#endif
                } else {
                    tp_last_staus = ELM_EVENT_TOUCH_L_MOVE;
                }

            } else {

                if (y > first_y) {
                    tp_last_staus = ELM_EVENT_TOUCH_D_MOVE;
                } else {
                    tp_last_staus = ELM_EVENT_TOUCH_U_MOVE;
                }
            }
            move_flag = 1;
        } else {
            if ((x == first_x) && (y == first_y)) {
                return;
            }
            tp_last_staus = ELM_EVENT_TOUCH_MOVE;
            /* return; */
        }
        /* tp_last_staus = ELM_EVENT_TOUCH_HOLD; */
        tke_start(x, y);
    } else {
        tp_last_staus = ELM_EVENT_TOUCH_DOWN;
        first_x = x;
        first_y = y;
        move_flag = 0;
        tke_start(x, y);
    }

    t.event = tp_last_staus;
    t.x = x;
    t.y = y;
    ui_touch_msg_post(&t);
}



static void tpd_up(int raw_x, int raw_y, int x, int y, int p)
{
    struct touch_event t = {0};

    if (tp_last_staus == ELM_EVENT_TOUCH_UP) {
        return ; // 重复
    }

    if (x < 0) {
        x = 0;
    }
    if (x > (VK_X - 1)) {
        x = VK_X - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > (VK_Y - 1)) {
        y = VK_Y - 1;
    }

#if Y_MIRROR
    x = VK_X - x - 1;
#endif

#if X_MIRROR
    y = VK_Y - y - 1;
#endif

    log_info("U[%4d %4d %4d]\n", x, y, 0);
    tp_last_staus = ELM_EVENT_TOUCH_UP;
    tp_down_cnt = 0;
    t.event = tp_last_staus;
    t.x = x;
    t.y = y;

    //不在这里发送up消息，在tke_stop里面!!!
    /* ui_touch_msg_post(&t); */

    tke_stop(x, y, &t);

}

static bool jd9165t_read(void)
{
    unsigned char buf[8];
    u8 reg_addr[4];
    reg_addr[0] = 0x20;
    reg_addr[1] = 0x01;
    reg_addr[2] = 0x11;
    reg_addr[3] = 0x20;

    system_iic_read_nbytes(__this->iic_hdl, HYN_I2C_ADDR, &reg_addr[0], 4, &buf[0], 8);
    int point_x = buf[4] | ((buf[3] & 0xF) << 8);
    int point_y = buf[6] | ((buf[5] & 0xF) << 8);
    if (buf[0] != 0) {
#if SW_X_Y
        tpd_down(0, 0, point_y, point_x, 0);
#else
        tpd_down(0, 0, point_x, point_y, 0);
#endif
    } else {
#if SW_X_Y
        tpd_up(0, 0, point_y, point_x, 0);
#else
        tpd_up(0, 0, point_x, point_y, 0);
#endif
    }
    return true;
}
#endif

#if CONFIG_LVGL_UI_ENABLE
void get_touch_x_y_status(int32_t *x, int32_t *y, uint8_t *status)
{
    *x = g_touch_data.point.x;
    *y = g_touch_data.point.y;
    *status = g_touch_data.state;
}

static bool jd9165t_lvgl_read(lv_touch_data_t *data)
{
    unsigned char buf[8];
    int update = 0;
    static int x_last, y_last;
    u8 reg_addr[4];
    reg_addr[0] = 0x20;
    reg_addr[1] = 0x01;
    reg_addr[2] = 0x11;
    reg_addr[3] = 0x20;

    system_iic_read_nbytes(__this->iic_hdl, HYN_I2C_ADDR, &reg_addr[0], 4, &buf[0], 8);

    data->state = (buf[0] != 0) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    if (buf[0] != 0) { /* 有有效触摸时才更新坐标 */
#if SW_X_Y
        data->point.y = buf[4] | ((buf[3] & 0xF) << 8);
        data->point.x = buf[6] | ((buf[5] & 0xF) << 8);
#else
        data->point.x = buf[4] | ((buf[3] & 0xF) << 8);
        data->point.y = buf[6] | ((buf[5] & 0xF) << 8);
#endif
        x_last = data->point.x;
        y_last = data->point.y;
    } else { /* 无触摸(抬起)时,保留上一次有效坐标,避免上报0xFFF(4095)越界坐标 */
        data->point.x = x_last;
        data->point.y = y_last;
    }

    log_debug("finger:%d, (%d, %d), %s\n", buf[0], data->point.x, data->point.y, (data->state == LV_INDEV_STATE_PR) ? "pressed" : "released");
    if ((data->point.x == VK_X) && (data->point.y == VK_Y)) {
        vk_state = data->state;
        //printf("vk %s\n", (vk_state == LV_INDEV_STATE_PR) ? "press" : "release");
    }

    return false; /* no more data to read */
}

static void JD9165T_get_touch_xy(void)
{
    if (!touch_flag) {
        return ;
    }

    touch_flag = 0;

    lv_touch_data_t touch_data;
    jd9165t_lvgl_read(&touch_data);
    g_touch_data.point.x = touch_data.point.x;
    g_touch_data.point.y = touch_data.point.y;
    g_touch_data.state = (touch_data.state == LV_INDEV_STATE_PR) ? 1 : 0;
}


__attribute__((weak)) int lcd_touch_interrupt_event(const char *tp_task_name, u16 x, u16 y, u8 status)
{
    log_info("Please implement this function to post coordinate data to UI!");
    return 0;
}
#endif /* #if CONFIG_LVGL_UI_ENABLE */


static void jd9165t_hw_init(void)
{
    u8 buf[2];
    u8 reg_addr[6];
    int ret;

    memset(&tke, 0xff, sizeof(tke));

    log_info("jd9165t init...\n");
    ret = system_iic_init(__this->iic_hdl, get_iic_config(__this->iic_hdl));
    log_debug("[%s] iic_init ret:%d", __func__, ret);

    jd9165t_reset();
    ctp_delay_ms(100);
    buf[0] = 0;
    reg_addr[0] = 0x40;
    reg_addr[1] = 0x00;
    reg_addr[2] = 0x80;
    reg_addr[3] = 0x76;
    reg_addr[4] = 0x00;
    reg_addr[5] = 0x02;
    system_iic_read_nbytes(__this->iic_hdl, HYN_I2C_ADDR, &reg_addr[0], 6, &buf[0], 2);

    uint16_t id = (buf[1] << 8) | buf[0];
    log_info("tp id : 0x%04x\n", id);

    u8 buf_ver[8] = {0};
    reg_addr[0] = 0x20;
    reg_addr[1] = 0x00;
    reg_addr[2] = 0x0f;
    reg_addr[3] = 0x1c;
    system_iic_read_nbytes(__this->iic_hdl, HYN_I2C_ADDR, &reg_addr[0], 4, &buf_ver[0], 8);
    log_info("tp version : 0x%x\n", buf_ver[0]);
    log_info("tp version : 0x%x\n", buf_ver[1]);
    log_info("tp version : 0x%x\n", buf_ver[2]);
    log_info("tp version : 0x%x\n", buf_ver[3]);

#if JD9165T_UPGRADE_ENABLE
    if (!jd9165t_upgrade()) {
        log_error("jd9165t_upgrade fail!\n");
        /* ASSERT(0); */
    }
#endif
    jd9165t_reset();

    //配置触摸脉冲报点时间间隔不要超过屏帧率, ~20ms
    //buf[0]=0x4;
    //i2c_write_regs(I2C_MASTER_CI2C0, HYN_I2C_ADDR, 0xee, &buf[0], 1, 0);
}

static void touch_interupt_task(void *p)
{
    int sem_timeout = 0;

    jd9165t_hw_init();

    os_sem_create(&touch_sem, 0);
    tp_irq_init();

    while (1) {
        tp_step = TP_RUN_STEP_RUN_IDLE;
        os_sem_pend(&touch_sem, sem_timeout);
        tp_step = TP_RUN_STEP_RUN;

#if CONFIG_LVGL_UI_ENABLE

        JD9165T_get_touch_xy(); //每次都读出触摸坐标，防止延迟读取读的是旧坐标


        if (lcd_touch_interrupt_event(TP_TASK_NAME, g_touch_data.point.x, g_touch_data.point.y, g_touch_data.state)) { //UI线程跑得慢，触摸事件处理不过来
            sem_timeout = 2; //按压坐标可丢失，抬起事件不可丢失，特意延后20ms再次读取坐标推送事件
            continue;
        }

        sem_timeout = 0;

#else /* #if CONFIG_LVGL_UI_ENABLE */

        if (touch_flag) {
            touch_flag = 0;
            jd9165t_read();
        }
#endif /* #if CONFIG_LVGL_UI_ENABLE */

    }
}



void ctp_enter_sleep(void)
{
    ctp_sleep_jd9165t();
}
void ctp_exit_sleep(void)
{
    ctp_wakeup_jd9165t();
}

void jd9165t_wait_init()
{
    while (tp_step == TP_RUN_STEP_INIT) {
        os_time_dly(1);
    }
}

void jd9165t_init()
{
    tp_step = TP_RUN_STEP_INIT;
    task_create(touch_interupt_task, NULL, "touch_task");
}

void jd9165t_deinit()
{
    jd9165t_wait_init();

    tp_irq_deinit();
    while (tp_step == TP_RUN_STEP_RUN) {
        os_time_dly(1);
    }
    int ret = task_kill("touch_task");
    ASSERT(!ret);
    os_sem_del(&touch_sem, OS_DEL_ALWAYS);
    system_iic_deinit(__this->iic_hdl);

    gpio_set_mode(IO_PORT_SPILT(TCFG_TP_RESET_IO), PORT_HIGHZ);

    tp_step = TP_RUN_STEP_STOP;
}
static u8 tp_idle_query(void)
{
    return !tp_step;
}

#if (!TP_DOUBLE_CLICK_WAKEUP)
REGISTER_LP_TARGET(tp_lp_target) = {
    .name = "tp_jd9165t",
    .is_idle = tp_idle_query,
};
#endif


#endif  //TCFG_TP_JD9165T_ENABLE

#endif



