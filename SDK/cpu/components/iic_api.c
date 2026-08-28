#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iic_api.data.bss")
#pragma data_seg(".iic_api.data")
#pragma const_seg(".iic_api.text.const")
#pragma code_seg(".iic_api.text")
#endif
#include "iic_api.h"
#include "asm/wdt.h"
#include "atomic.h"
#include "init.h"

#define LOG_TAG_CONST       IIC
#define LOG_TAG             "[iic_api]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

static OS_MUTEX iic_mutex[2] = {0};
static atomic_t iic_ref[2];
//********************hw-iic for tp/alipay se/compass**********************//
int iic_api_init(void)
{
    os_mutex_create(&iic_mutex[0]);
    os_mutex_create(&iic_mutex[1]);
    return 0;
}
platform_initcall(iic_api_init);


int system_iic_init(hw_iic_dev iic, struct iic_master_config *i2c_config)
{
    int ret = 0;
    os_mutex_pend(&iic_mutex[iic], 0);
    ASSERT(!config_asser || (atomic_read(&iic_ref[iic]) == 0));
    if (atomic_inc_return(&iic_ref[iic]) == 1) {

        ret =  hw_iic_init(iic, i2c_config);
    }
    os_mutex_post(&iic_mutex[iic]);

    return ret;
}

int system_iic_deinit(hw_iic_dev iic)
{
    int ret = 0;
    os_mutex_pend(&iic_mutex[iic], 0);
    if (atomic_read(&iic_ref[iic]) != 0 && atomic_dec_return(&iic_ref[iic])) {
        os_mutex_post(&iic_mutex[iic]);
        return ret;
    }
    ret = hw_iic_deinit(iic);
    os_mutex_post(&iic_mutex[iic]);

    return ret;
}

void system_iic_reset(hw_iic_dev iic)
{
    os_mutex_pend(&iic_mutex[iic], 0);

    hw_iic_reset(iic);

    os_mutex_post(&iic_mutex[iic]);
}

int system_iic_suspend(hw_iic_dev iic)
{
    int ret;

    os_mutex_pend(&iic_mutex[iic], 0);
    ret = hw_iic_suspend(iic);
    os_mutex_post(&iic_mutex[iic]);

    return ret;
}

int system_iic_resume(hw_iic_dev iic)
{
    int ret;

    os_mutex_pend(&iic_mutex[iic], 0);
    ret = hw_iic_resume(iic);
    os_mutex_post(&iic_mutex[iic]);

    return ret;
}

int system_iic_read_nbytes(hw_iic_dev iic,
                           unsigned char dev_7bit_addr, //7位设备地址
                           unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
                           unsigned char *read_buf, int read_len)//缓存buf，读取长度
{
    int ret;
    os_mutex_pend(&iic_mutex[iic], 0);

    ret = hw_i2c_master_read_nbytes_from_device_reg(iic, dev_7bit_addr << 1, reg_addr, reg_len, read_buf, read_len);

    os_mutex_post(&iic_mutex[iic]);
    return ret;
}

int system_iic_write_nbytes(hw_iic_dev iic,
                            unsigned char dev_7bit_addr, //7位设备地址
                            unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
                            unsigned char *write_buf, int write_len)//数据buf, 写入长度
{
    int ret;
    os_mutex_pend(&iic_mutex[iic], 0);

    ret = hw_i2c_master_write_nbytes_to_device_reg(iic, dev_7bit_addr << 1, reg_addr, reg_len, write_buf, write_len);

    os_mutex_post(&iic_mutex[iic]);
    return ret;
}


/******************************soft iic*****************************/
//如果无reg_addr:reg_addr=NULL,reg_len=0
//return: <0:error,  =read_len:ok
int soft_i2c_master_read_nbytes_from_device_reg(soft_iic_dev iic,
        unsigned char dev_addr, //设备地址
        unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
        unsigned char *read_buf, int read_len)//缓存buf，读取长度
{
    u8 ack;
    int ret = 0;
    if (soft_iic_check_busy(iic) != IIC_OK) { //busy
        ret = IIC_ERROR_BUSY; //busy
        goto _read_exit2;
    }
    soft_iic_start(iic);

    if ((reg_addr != NULL) && (reg_len != 0)) {
        ack = soft_iic_tx_byte(iic, dev_addr);
        if (ack == 0) {
            log_error("dev_addr no ack!");
            ret = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
            goto _read_exit1;
        }

        for (u8 i = 0; i < reg_len; i++) {
            ack = soft_iic_tx_byte(iic, reg_addr[i]);
            if (ack == 0) {
                log_error("reg_addr no ack!");
                ret = IIC_ERROR_REG_ADDR_ACK_ERROR; //无应答
                goto _read_exit1;
            }
        }
        soft_iic_start(iic);
    }

    ack = soft_iic_tx_byte(iic, dev_addr | BIT(0));
    if (ack == 0) {
        log_error("dev_addr no ack!");
        ret = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
        goto _read_exit1;
    }

    ret = soft_iic_read_buf(iic, read_buf, read_len);
_read_exit1:
    soft_iic_stop(iic);
_read_exit2:
    return ret;
}


//如果无reg_addr:reg_addr=NULL,reg_len=0
//return: =write_len:ok, other:error
int soft_i2c_master_write_nbytes_to_device_reg(soft_iic_dev iic,
        unsigned char dev_addr, //设备地址
        unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
        unsigned char *write_buf, int write_len)//数据buf, 写入长度
{
    int res;
    u8 ack;
    if (soft_iic_check_busy(iic) != IIC_OK) { //busy
        res = IIC_ERROR_BUSY; //busy
        goto _write_exit2;
    }

    soft_iic_start(iic);
    ack = soft_iic_tx_byte(iic, dev_addr);
    if (ack == 0) {
        log_error("dev_addr no ack!");
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
        goto _write_exit1;
    }

    if ((reg_addr != NULL) && (reg_len != 0)) {
        for (u8 i = 0; i < reg_len; i++) {
            ack = soft_iic_tx_byte(iic, reg_addr[i]);
            if (ack == 0) {
                log_error("reg_addr no ack!");
                res = IIC_ERROR_REG_ADDR_ACK_ERROR; //无应答
                goto _write_exit1;
            }
        }
    }

    for (res = 0; res < write_len; res++) {
        if (0 == soft_iic_tx_byte(iic, write_buf[res])) {
            log_error("write data no ack!");
            goto _write_exit1;
        }
    }
_write_exit1:
    soft_iic_stop(iic);
_write_exit2:
    return res;
}









/******************************hw iic master*****************************/
#if defined CONFIG_IIC_VERSION2
#define HW_IIC_MASTER_ISR_EN 1
#else
#define HW_IIC_MASTER_ISR_EN 0
#endif
//如果无reg_addr:reg_addr=NULL,reg_len=0
//return: other:error,  =read_len:ok
int hw_i2c_master_read_nbytes_from_device_reg(hw_iic_dev iic,
        unsigned char dev_addr, //设备地址
        unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
        unsigned char *read_buf, int read_len)//缓存buf，读取长度
{
#if HW_IIC_MASTER_ISR_EN
#if defined(P11_HW_IIC_NUM)&&P11_HW_IIC_NUM
    if (iic != HW_P11_IIC_0)
#endif
    {
        struct hw_iic_master_isr_transmit iic_isr_info = {
            .dev_addr = dev_addr,
            .restart_flag = 1,
            .reg_buf = reg_addr,
            .reg_len = reg_len,
            .data_buf = read_buf,
            .rx_len = read_len,
            .tx_len = 0,
        };
        if ((reg_addr == NULL) || (reg_len == 0)) {
            iic_isr_info .dev_addr = dev_addr | BIT(0);
            iic_isr_info .restart_flag = 0;
        }
        enum iic_state_enum iic_sta = hw_iic_master_isr_transmit_cfg(iic, &iic_isr_info, 20);//byte间隔超时20:20*30ms
        if (iic_sta != IIC_OK) {
            log_error("iic%d isr sta:%d\n", iic, iic_sta);
            if (iic_sta == IIC_ERROR_MASTER_ERROR) {
                hw_iic_reset(iic);
            }
            return iic_sta;
        }
        return iic_isr_info.xfer_postion;
    }
#endif

    u8 ack;
    int ret = 0;
    if (hw_iic_check_busy(iic) != IIC_OK) { //busy
        ret = IIC_ERROR_BUSY; //busy
        goto _read_exit2;
    }
    ret = hw_iic_start(iic);
    if (ret < 0) {
        log_error("iic lock busy!%d", ret);
        goto _read_exit2;
    }

    if ((reg_addr != NULL) && (reg_len != 0)) {
        ack = hw_iic_tx_byte(iic, dev_addr);
        if (ack == 0) {
            log_error("dev_addr no ack!");
            ret = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
            goto _read_exit1;
        }

        for (u8 i = 0; i < reg_len; i++) {
            ack = hw_iic_tx_byte(iic, reg_addr[i]);
            if (ack == 0) {
                log_error("reg_addr no ack!");
                ret = IIC_ERROR_REG_ADDR_ACK_ERROR; //无应答
                goto _read_exit1;
            }
        }
        hw_iic_start(iic);
    }

    ack = hw_iic_tx_byte(iic, dev_addr | BIT(0));
    if (ack == 0) {
        log_error("dev_addr no ack!");
        ret = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
        goto _read_exit1;
    }

    ret = hw_iic_read_buf(iic, read_buf, read_len);
_read_exit1:
    hw_iic_stop(iic);
    if (ret != read_len) {
        hw_iic_err_reset(iic);
        hw_iic_reset(iic);
    }
_read_exit2:
    return ret;
}


//如果无reg_addr:reg_addr=NULL,reg_len=0
//return: =write_len:ok, other:error
int hw_i2c_master_write_nbytes_to_device_reg(hw_iic_dev iic,
        unsigned char dev_addr, //设备地址
        unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
        unsigned char *write_buf, int write_len)//数据buf, 写入长度
{
#if HW_IIC_MASTER_ISR_EN
#if defined(P11_HW_IIC_NUM)&&P11_HW_IIC_NUM
    if (iic != HW_P11_IIC_0)
#endif
    {
        struct hw_iic_master_isr_transmit iic_isr_info = {
            .dev_addr = dev_addr,
            .restart_flag = 0,
            .reg_buf = reg_addr,
            .reg_len = reg_len,
            .data_buf = write_buf,
            .rx_len = 0,
            .tx_len = write_len,
        };
        enum iic_state_enum iic_sta = hw_iic_master_isr_transmit_cfg(iic, &iic_isr_info, 20);//byte间隔超时20:20*30ms
        if (iic_sta != IIC_OK) {
            log_error("iic%d isr sta:%d\n", iic, iic_sta);
            if (iic_sta == IIC_ERROR_MASTER_ERROR) {
                hw_iic_reset(iic);
            }
            return iic_sta;
        }
        return iic_isr_info.xfer_postion;
    }
#endif

    int res;
    u8 ack;
    if (hw_iic_check_busy(iic) != IIC_OK) { //busy
        res = IIC_ERROR_BUSY; //busy
        goto _write_exit2;
    }

    res = hw_iic_start(iic);
    if (res < 0) {
        log_error("iic lock busy!%d", res);
        goto _write_exit2;
    }
    ack = hw_iic_tx_byte(iic, dev_addr);
    if (ack == 0) {
        log_error("dev_addr no ack!");
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
        goto _write_exit1;
    }

    if ((reg_addr != NULL) && (reg_len != 0)) {
        for (u8 i = 0; i < reg_len; i++) {
            ack = hw_iic_tx_byte(iic, reg_addr[i]);
            if (ack == 0) {
                log_error("reg_addr no ack!");
                res = IIC_ERROR_REG_ADDR_ACK_ERROR; //无应答
                goto _write_exit1;
            }
        }
    }

#if 0
    for (res = 0; res < write_len; res++) {
        if (0 == hw_iic_tx_byte(iic, write_buf[res])) {
            log_error("write data no ack!");
            goto _write_exit1;
        }
    }
#else
    res = hw_iic_write_buf(iic, write_buf, write_len);
#endif
_write_exit1:
    hw_iic_stop(iic);
    if (res != write_len) {
        hw_iic_err_reset(iic);
        hw_iic_reset(iic);
    }
_write_exit2:
    return res;
}




/******************************hw iic slave*****************************/

//rx协议:start,addr write,data0,data1,,,,,,stop
int hw_iic_slave_polling_rx(hw_iic_dev iic, u8 *rx_buf)
{
    int rx_cnt = 0;
    int rx_state = 0;

    log_info("--iic slave polling rx --");
    local_irq_disable();//关闭所有中断
    rx_state = hw_iic_slave_rx_prepare(iic, 0, 600000);//1s
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx
    } else if (rx_state  == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        log_error("iic slave wait addr timeout!");
        local_irq_enable();
        return 0;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        log_error("iic slave wait end!");
        local_irq_enable();
        return 0;
    }

    rx_state = hw_iic_slave_rx_byte(iic, &rx_buf[0]);//addr
    if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx
    } else if (rx_state == IIC_SLAVE_RX_ADDR_NO_MATCH) { //error
        log_error("iic slave rx addr error!");
        local_irq_enable();
        return 0;
    } else if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx

    }
    rx_state = hw_iic_slave_rx_prepare(iic, 1, 100000);//1s, 1:收到数据应答
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx
    } else if (rx_state  == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        log_error("iic slave wait reg timeout!");
        local_irq_enable();
        return 0;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        log_info("iic slave wait end!");
        local_irq_enable();
        return 0;
    }

    rx_cnt = hw_iic_slave_rx_nbyte(iic, &rx_buf[1]);
    local_irq_enable();
    /* log_info("rx addr:%x, slave addr:%x", rx_buf[0], hw_iic_slave_get_addr(iic)); */
    rx_cnt++;
    log_info_hexdump(rx_buf, rx_cnt);
    log_info("~~~~~iic rx polling end~~~~~\n");
    memset(rx_buf, 0, rx_cnt);
    return rx_cnt;
}

//tx协议:start,addr read,data0,data1,,,,,nack,stop
int hw_iic_slave_polling_tx(hw_iic_dev iic, u8 *tx_buf)
{
    u8 slave_rx_data[3] = {0, 0, 0};
    int rx_cnt = 0, tx_cnt = 0;
    int rx_state = 0;
    log_info("--iic slave polling tx --");
    local_irq_disable();//关闭所有中断
    rx_state = hw_iic_slave_rx_prepare(iic, 0, 600000);//1s
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        log_error("iic slave wait addr timeout!\n");
        local_irq_enable();
        return 0;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        log_error("iic slave wait end!\n");
        local_irq_enable();
        return 0;
    }

    rx_state = hw_iic_slave_rx_byte(iic, &slave_rx_data[rx_cnt++]);//addr
    if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx
    } else if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx
        hw_iic_slave_tx_byte(iic, tx_buf[tx_cnt++]);
        goto _tx_strat;
    } else { //error
        log_error("iic slave rx addr error!\n");
        local_irq_enable();
        return 0;
    }

_tx_strat:
    tx_cnt = hw_iic_slave_tx_nbyte(iic, &tx_buf[tx_cnt]);

    local_irq_enable();
    /* log_info("rx0 addr:%x, slave addr:%x", slave_rx_data[0], hw_iic_slave_get_addr(iic)); */
    log_info_hexdump(slave_rx_data, rx_cnt);
    log_info("~~~~~iic tx polling end~~~~~\n");
    return tx_cnt + 1; //ok
}

