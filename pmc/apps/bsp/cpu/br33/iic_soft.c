#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iic.bss")
#pragma data_seg(".iic.data")
#pragma const_seg(".iic.const")
#pragma code_seg(".iic.text")
#pragma str_literal_override(".iic.text")
#endif /* #ifdef SUPPORT_MS_EXTENSIONS */

#include "includes.h"
#include "gpio.h"
#include "iic_soft.h"



static u8 soft_iic_state[MAX_SOFT_IIC_NUM];
static struct iic_master_config soft_iic_cfg_cache[MAX_SOFT_IIC_NUM];

//input pull up
#define IIC_SCL_H(scl)    \
	gpio_set_direction(scl, 1)

#define IIC_SCL_L(scl)    \
	gpio_set_direction(scl, 0); \
	gpio_set_output_value(scl, 0)


#define IIC_SDA_DIR(sda, val) \
	gpio_set_direction(sda, val)

//input pull up
#define IIC_SDA_H(sda)    \
	gpio_set_direction(sda, 1)

#define IIC_SDA_L(sda)    \
	gpio_set_direction(sda, 0); \
	gpio_set_output_value(sda, 0)

#define IIC_SDA_READ(sda) \
    gpio_read(sda)

#define soft_iic_delay(num) \
	delay_cnt= num; \
    while (delay_cnt--) { \
        asm("nop"); \
    }
static inline u32 iic_get_delay(soft_iic_dev iic)
{
    u32 hsb_clk = 16000000;//clk_get("sys");
    u32 delay_num = 0;
    return delay_num;
}

extern const struct iic_master_config soft_iic_cfg_const[MAX_SOFT_IIC_NUM];
struct iic_master_config *get_soft_iic_config(soft_iic_dev iic)
{
    return (struct iic_master_config *)&soft_iic_cfg_const[iic];
}
enum iic_state_enum soft_iic_init(soft_iic_dev iic, struct iic_master_config *i2c_config)
{
    ASSERT(iic < MAX_SOFT_IIC_NUM, "iic > MAX_SOFT_IIC_NUM");
    if (i2c_config == NULL) {
        printf("error: soft iic%d param error!\n", iic);
        return IIC_ERROR_PARAM_ERROR;
    }
    if (iic >= MAX_SOFT_IIC_NUM) {
        printf("error: soft iic index:%d error!\n", iic);
        return IIC_ERROR_INDEX_ERROR;
    }
    if ((soft_iic_state[iic]&BIT(7)) != 0) {
        printf("error: soft iic%d has been occupied!\n", iic);
        return IIC_ERROR_INIT_FAIL;
    }
    soft_iic_state[iic] = BIT(7);//init ok
    memcpy(&soft_iic_cfg_cache[iic], i2c_config, sizeof(struct iic_master_config));

    //freq:

    if (i2c_config->io_mode) {//pull up
        gpio_set_pull_up(i2c_config->scl_io, 1);
        gpio_set_pull_up(i2c_config->sda_io, 1);
    } else {
        gpio_set_pull_up(i2c_config->scl_io, 0);
        gpio_set_pull_up(i2c_config->sda_io, 0);
    }
    gpio_set_hd(i2c_config->scl_io, i2c_config->hdrive);
    gpio_set_hd(i2c_config->sda_io, i2c_config->hdrive);
    IIC_SDA_H(i2c_config->sda_io);
    IIC_SCL_H(i2c_config->scl_io);
    gpio_set_pull_down(i2c_config->scl_io, 0);
    gpio_set_pull_down(i2c_config->sda_io, 0);
    gpio_set_die(i2c_config->scl_io, 1);//en 1.1v
    gpio_set_die(i2c_config->sda_io, 1);//en 1.1v
    /* gpio_set_dieh(i2c_config->scl_io, 1);//en 3.3v */
    /* gpio_set_dieh(i2c_config->sda_io, 1);//en 3.3v */
    return IIC_OK;//ok
}

enum iic_state_enum soft_iic_deinit(soft_iic_dev iic)
{
    ASSERT(iic < MAX_SOFT_IIC_NUM, "iic > MAX_SOFT_IIC_NUM");
    if (soft_iic_state[iic] == 0) {
        printf("error: soft iic%d has been no init!\n", iic);
        return IIC_ERROR_NO_INIT;
    }
    soft_iic_state[iic] = 0;//no init

    gpio_set_direction(soft_iic_cfg_cache[iic].scl_io, 1);
    gpio_set_direction(soft_iic_cfg_cache[iic].sda_io, 1);
    gpio_set_pull_up(soft_iic_cfg_cache[iic].scl_io, 0);
    gpio_set_pull_up(soft_iic_cfg_cache[iic].sda_io, 0);
    gpio_set_die(soft_iic_cfg_cache[iic].scl_io, 0);
    gpio_set_die(soft_iic_cfg_cache[iic].sda_io, 0);
    /* gpio_set_dieh(i2c_config->scl_io, 0);//en 3.3v */
    /* gpio_set_dieh(i2c_config->sda_io, 0);//en 3.3v */
    gpio_set_hd(soft_iic_cfg_cache[iic].scl_io, PORT_DRIVE_STRENGT_2p4mA);
    gpio_set_hd(soft_iic_cfg_cache[iic].sda_io, PORT_DRIVE_STRENGT_2p4mA);
    return IIC_OK;//ok
}

enum iic_state_enum soft_iic_suspend(soft_iic_dev iic)
{
    ASSERT(iic < MAX_SOFT_IIC_NUM, "iic > MAX_SOFT_IIC_NUM");
    if ((soft_iic_state[iic] & 0xc0) != 0x80) {
        printf("error: soft iic%d is no init or suspend!\n", iic);
        return IIC_ERROR_SUSPEND_FAIL;
    }
    if ((soft_iic_state[iic] & 0x3f) != 0) {
        printf("error: soft iic%d is busy!\n", iic);
        return IIC_ERROR_BUSY;
    }
    soft_iic_state[iic] |= BIT(6);//suspend ok

    gpio_set_direction(soft_iic_cfg_cache[iic].scl_io, 1);
    gpio_set_direction(soft_iic_cfg_cache[iic].sda_io, 1);
    gpio_set_pull_up(soft_iic_cfg_cache[iic].scl_io, 0);
    gpio_set_pull_up(soft_iic_cfg_cache[iic].sda_io, 0);
    gpio_set_die(soft_iic_cfg_cache[iic].scl_io, 0);
    gpio_set_die(soft_iic_cfg_cache[iic].sda_io, 0);
    /* gpio_set_dieh(i2c_config->scl_io, 0);//en 3.3v */
    /* gpio_set_dieh(i2c_config->sda_io, 0);//en 3.3v */
    gpio_set_hd(soft_iic_cfg_cache[iic].scl_io, PORT_DRIVE_STRENGT_2p4mA);
    gpio_set_hd(soft_iic_cfg_cache[iic].sda_io, PORT_DRIVE_STRENGT_2p4mA);
    return IIC_OK;//ok
}

enum iic_state_enum soft_iic_resume(soft_iic_dev iic)
{
    ASSERT(iic < MAX_SOFT_IIC_NUM, "iic > MAX_SOFT_IIC_NUM");
    if ((soft_iic_state[iic] & 0xc0) != 0xc0) {
        printf("error: soft iic%d is no init or no suspend!\n", iic);
        return IIC_ERROR_RESUME_FAIL;
    }
    soft_iic_state[iic] &= ~ BIT(6); //resume ok


    if (soft_iic_cfg_cache[iic].io_mode) {//pull up
        gpio_set_pull_up(soft_iic_cfg_cache[iic].scl_io, 1);
        gpio_set_pull_up(soft_iic_cfg_cache[iic].sda_io, 1);
    } else {
        gpio_set_pull_up(soft_iic_cfg_cache[iic].scl_io, 0);
        gpio_set_pull_up(soft_iic_cfg_cache[iic].sda_io, 0);
    }
    gpio_set_hd(soft_iic_cfg_cache[iic].scl_io, soft_iic_cfg_cache[iic].hdrive);
    gpio_set_hd(soft_iic_cfg_cache[iic].sda_io, soft_iic_cfg_cache[iic].hdrive);
    IIC_SDA_H(soft_iic_cfg_cache[iic].sda_io);
    IIC_SCL_H(soft_iic_cfg_cache[iic].scl_io);
    gpio_set_pull_down(soft_iic_cfg_cache[iic].scl_io, 0);
    gpio_set_pull_down(soft_iic_cfg_cache[iic].sda_io, 0);
    gpio_set_die(soft_iic_cfg_cache[iic].scl_io, 1);
    gpio_set_die(soft_iic_cfg_cache[iic].sda_io, 1);
    /* gpio_set_dieh(i2c_config->scl_io, 1);//en 3.3v */
    /* gpio_set_dieh(i2c_config->sda_io, 1);//en 3.3v */
    return IIC_OK;//ok
}



//return:0:error, 1:ok
enum iic_state_enum soft_iic_check_busy(soft_iic_dev iic)
{
    ASSERT(iic < MAX_SOFT_IIC_NUM, "iic > MAX_SOFT_IIC_NUM");
    if (soft_iic_state[iic] & 0x0f) {
        return IIC_ERROR_BUSY;//error
    }
    soft_iic_state[iic]++;//busy
    return IIC_OK;//ok
}
void soft_iic_idle(soft_iic_dev iic)
{
    soft_iic_state[iic] &= 0xf0;//idle
}

enum iic_state_enum soft_iic_start(soft_iic_dev iic)
{
    ASSERT(iic < MAX_SOFT_IIC_NUM, "iic > MAX_SOFT_IIC_NUM");
    u32 delay_cnt;
    u32 dly_t = iic_get_delay(iic);
    /* printf("soft_iic_init hsb clock:%d, delay cnt:%d\n",clk_get("sys"),dly_t); */

    IIC_SDA_H(soft_iic_cfg_cache[iic].sda_io);
    soft_iic_delay(dly_t);

    IIC_SCL_H(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t * 2);

    IIC_SDA_L(soft_iic_cfg_cache[iic].sda_io);
    soft_iic_delay(dly_t);

    IIC_SCL_L(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t);
    return IIC_OK;//ok
}

void soft_iic_stop(soft_iic_dev iic)
{
    u32 delay_cnt;
    u32 dly_t = iic_get_delay(iic);
    IIC_SDA_L(soft_iic_cfg_cache[iic].sda_io);
    soft_iic_delay(dly_t);

    IIC_SCL_H(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t * 2);

    IIC_SDA_H(soft_iic_cfg_cache[iic].sda_io);
    soft_iic_delay(dly_t);
    soft_iic_idle(iic);
}

void soft_iic_reset(soft_iic_dev iic)//同iic_v2
{
    ASSERT(iic < MAX_SOFT_IIC_NUM, "iic > MAX_SOFT_IIC_NUM");
    soft_iic_start(iic);
    soft_iic_stop(iic);
    soft_iic_idle(iic);
}

static u8 soft_iic_check_ack(soft_iic_dev iic)
{
    u8 ack;
    u32 delay_cnt;
    u32 dly_t = iic_get_delay(iic);

    IIC_SDA_DIR(soft_iic_cfg_cache[iic].sda_io, 1);
    IIC_SCL_L(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t);

    IIC_SCL_H(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t);

    if (IIC_SDA_READ(soft_iic_cfg_cache[iic].sda_io) == 0) {
        ack = 1;
    } else {
        ack = 0;
    }
    soft_iic_delay(dly_t);

    IIC_SCL_L(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t);
    IIC_SDA_DIR(soft_iic_cfg_cache[iic].sda_io, 0);
    IIC_SDA_L(soft_iic_cfg_cache[iic].sda_io);

    return ack;//1:有应答, 0:无
}

static void soft_iic_rx_ack(soft_iic_dev iic)
{
    u32 delay_cnt;
    u32 dly_t = iic_get_delay(iic);
    IIC_SDA_L(soft_iic_cfg_cache[iic].sda_io);
    soft_iic_delay(dly_t);

    IIC_SCL_H(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t * 2);

    IIC_SCL_L(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t);
}

static void soft_iic_rx_nack(soft_iic_dev iic)
{
    u32 delay_cnt;
    u32 dly_t = iic_get_delay(iic);
    IIC_SDA_H(soft_iic_cfg_cache[iic].sda_io);
    soft_iic_delay(dly_t);

    IIC_SCL_H(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t * 2);

    IIC_SCL_L(soft_iic_cfg_cache[iic].scl_io);
    soft_iic_delay(dly_t);
}

u8 soft_iic_tx_byte(soft_iic_dev iic, u8 byte)
{
    u32 delay_cnt;
    u32 dly_t = iic_get_delay(iic);

    local_irq_disable();
    IIC_SCL_L(soft_iic_cfg_cache[iic].scl_io);
    for (u32 i = 0; i < 8; i++) {  //MSB FIRST
        if ((byte << i) & 0x80) {
            IIC_SDA_H(soft_iic_cfg_cache[iic].sda_io);
        } else {
            IIC_SDA_L(soft_iic_cfg_cache[iic].sda_io);
        }
        soft_iic_delay(dly_t);

        IIC_SCL_H(soft_iic_cfg_cache[iic].scl_io);
        soft_iic_delay(dly_t * 2);

        IIC_SCL_L(soft_iic_cfg_cache[iic].scl_io);
        soft_iic_delay(dly_t);
    }
    u8 ack = soft_iic_check_ack(iic);//1:有应答, 0:无
    local_irq_enable();
    return ack;
}

u8 soft_iic_rx_byte(soft_iic_dev iic, u8 ack, s8 *err)
{
    u32 delay_cnt;
    u32 dly_t = iic_get_delay(iic);
    u8 byte = 0;

    local_irq_disable();
    IIC_SDA_DIR(soft_iic_cfg_cache[iic].sda_io, 1);

    for (u32 i = 0; i < 8; i++) {
        soft_iic_delay(dly_t);

        IIC_SCL_H(soft_iic_cfg_cache[iic].scl_io);
        soft_iic_delay(dly_t);

        byte = byte << 1;
        if (IIC_SDA_READ(soft_iic_cfg_cache[iic].sda_io)) {
            byte |= 1;
        }
        soft_iic_delay(dly_t);

        IIC_SCL_L(soft_iic_cfg_cache[iic].scl_io);
        soft_iic_delay(dly_t);
    }

    IIC_SDA_DIR(soft_iic_cfg_cache[iic].sda_io, 0);
    if (ack) {
        soft_iic_rx_ack(iic);
    } else {
        soft_iic_rx_nack(iic);
    }

    local_irq_enable();
    if (err) {
        *err = IIC_OK;
    }
    return byte;
}

//return: =len:ok
int soft_iic_read_buf(soft_iic_dev iic, void *buf, int len)
{
    int i = 0;

    if (!buf || !len) {
        return IIC_ERROR_PARAM_ERROR;
    }
    for (i = 0; i < len - 1; i++) {
        ((u8 *)buf)[i] = soft_iic_rx_byte(iic, 1, NULL);
    }
    ((u8 *)buf)[len - 1] = soft_iic_rx_byte(iic, 0, NULL);
    return len;
}

//return: =len:ok
int soft_iic_write_buf(soft_iic_dev iic, const void *buf, int len)
{
    int i;
    u8 ack;

    if (!buf || !len) {
        return IIC_ERROR_PARAM_ERROR;
    }
    for (i = 0; i < len; i++) {
        ack = soft_iic_tx_byte(iic, ((u8 *)buf)[i]);
        if (ack == 0) {
            break;
        }
    }
    return i;
}

