#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iic.bss")
#pragma data_seg(".iic.data")
#pragma const_seg(".iic.const")
#pragma code_seg(".iic.text")
#pragma str_literal_override(".iic.text")
#endif /* #ifdef SUPPORT_MS_EXTENSIONS */

#include "includes.h"
#include "sdk_config.h"
#include "gpio.h"
#include "iic_api.h"
/* #include "atomic.h" */

#define LOG_TAG_CONST       IIC
#define LOG_TAG             "[iic_api]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/* static atomic_t iic_ref[2]; */
//********************hw-iic for tp/alipay se/compass**********************//
int iic_api_init(void)
{
    return 0;
}

int system_iic_init(hw_iic_dev iic, struct iic_master_config *i2c_config)
{
    int ret = 0;
    local_irq_disable();
    /* ASSERT(!config_asser || (atomic_read(&iic_ref[iic]) == 0)); */
    /* if (atomic_inc_return(&iic_ref[iic]) == 1) { */

    ret =  hw_iic_init(iic, i2c_config);
    /* } */
    local_irq_enable();

    return ret;
}

int system_iic_deinit(hw_iic_dev iic)
{
    int ret = 0;
    return ret;
    /* } */
    ret = hw_iic_deinit(iic);

    return ret;
}

void system_iic_reset(hw_iic_dev iic)
{
    local_irq_disable();

    hw_iic_reset(iic);

    local_irq_enable();
}

int system_iic_suspend(hw_iic_dev iic)
{
    int ret;

    local_irq_disable();
    ret = hw_iic_suspend(iic);
    local_irq_enable();

    return ret;
}

int system_iic_resume(hw_iic_dev iic)
{
    int ret;

    local_irq_disable();
    ret = hw_iic_resume(iic);
    local_irq_enable();

    return ret;
}

int system_iic_read_nbytes(hw_iic_dev iic,
                           unsigned char dev_7bit_addr, //7位设备地址
                           unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
                           unsigned char *read_buf, int read_len)//缓存buf，读取长度
{
    int ret;
    local_irq_disable();

    ret = hw_i2c_master_read_nbytes_from_device_reg(iic, dev_7bit_addr << 1, reg_addr, reg_len, read_buf, read_len);

    local_irq_enable();
    return ret;
}

int system_iic_write_nbytes(hw_iic_dev iic,
                            unsigned char dev_7bit_addr, //7位设备地址
                            unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
                            unsigned char *write_buf, int write_len)//数据buf, 写入长度
{
    int ret;
    local_irq_disable();

    ret = hw_i2c_master_write_nbytes_to_device_reg(iic, dev_7bit_addr << 1, reg_addr, reg_len, write_buf, write_len);

    local_irq_enable();
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
        ack = soft_iic_tx_byte(iic, dev_addr & 0xfe);
        if (ack == 0) {
            printf("<err>:dev_addr no ack!\n");
            ret = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
            goto _read_exit1;
        }

        for (u8 i = 0; i < reg_len; i++) {
            ack = soft_iic_tx_byte(iic, reg_addr[i]);
            if (ack == 0) {
                printf("<err>:reg_addr no ack!\n");
                ret = IIC_ERROR_REG_ADDR_ACK_ERROR; //无应答
                goto _read_exit1;
            }
        }

        soft_iic_start(iic);
    }

    ack = soft_iic_tx_byte(iic, dev_addr | BIT(0));
    if (ack == 0) {
        printf("<err>:dev_addr no ack!\n");
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
        printf("<err>:dev_addr no ack!\n");
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
        goto _write_exit1;
    }

    if ((reg_addr != NULL) && (reg_len != 0)) {
        for (u8 i = 0; i < reg_len; i++) {
            ack = soft_iic_tx_byte(iic, reg_addr[i]);
            if (ack == 0) {
                printf("<err>:reg_addr no ack!\n");
                res = IIC_ERROR_REG_ADDR_ACK_ERROR; //无应答
                goto _write_exit1;
            }
        }
    }

    for (res = 0; res < write_len; res++) {
        if (0 == soft_iic_tx_byte(iic, write_buf[res])) {
            printf("<err>:write data no ack!\n");
            goto _write_exit1;
        }
    }
_write_exit1:
    soft_iic_stop(iic);
_write_exit2:
    return res;
}









/******************************hw iic master*****************************/

#define HW_IIC_MASTER_ISR_MODE_SEL    0 //1:中断模式,0:轮询

//如果无reg_addr:reg_addr=NULL,reg_len=0
//return: other:error,  =read_len:ok
int hw_i2c_master_read_nbytes_from_device_reg(hw_iic_dev iic,
        unsigned char dev_addr, //设备地址
        unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
        unsigned char *read_buf, int read_len)//缓存buf，读取长度
{
#if HW_IIC_MASTER_ISR_MODE_SEL && HW_IIC_MASTER_ISR_EN
#if defined(P11_HW_IIC_NUM)&&P11_HW_IIC_NUM
    if (iic != HW_P11_IIC_0)
#endif
    {
        struct hw_iic_master_isr_transmit iic_isr_info = {
            .dev_addr = dev_addr & 0xfe,
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
        printf("<err>:iic lock busy!%d\n", ret);
        goto _read_exit2;
    }
    if ((reg_addr != NULL) && (reg_len != 0)) {
        ack = hw_iic_tx_byte(iic, dev_addr & 0xfe);
        if (ack == 0) {
            printf("<err>:dev_addr no ack!\n");
            ret = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
            goto _read_exit1;
        }

        for (u8 i = 0; i < reg_len; i++) {
            ack = hw_iic_tx_byte(iic, reg_addr[i]);
            if (ack == 0) {
                printf("<err>:reg_addr no ack!\n");
                ret = IIC_ERROR_REG_ADDR_ACK_ERROR; //无应答
                goto _read_exit1;
            }
        }

        hw_iic_start(iic);
    }

    ack = hw_iic_tx_byte(iic, dev_addr | BIT(0));
    if (ack == 0) {
        printf("<err>:dev_addr no ack!\n");
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
        printf("<err>:iic lock busy!%d\n", res);
        goto _write_exit2;
    }
    ack = hw_iic_tx_byte(iic, dev_addr);
    if (ack == 0) {
        printf("<err>:dev_addr no ack!\n");
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR; //无应答
        goto _write_exit1;
    }

    if ((reg_addr != NULL) && (reg_len != 0)) {
        for (u8 i = 0; i < reg_len; i++) {
            ack = hw_iic_tx_byte(iic, reg_addr[i]);
            if (ack == 0) {
                printf("<err>:reg_addr no ack!\n");
                res = IIC_ERROR_REG_ADDR_ACK_ERROR; //无应答
                goto _write_exit1;
            }
        }
    }

#if 0
    for (res = 0; res < write_len; res++) {
        if (0 == hw_iic_tx_byte(iic, write_buf[res])) {
            printf("<err>:write data no ack!\n");
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

    printf("--iic slave polling rx --\n");
    local_irq_disable();//关闭所有中断
    rx_state = hw_iic_slave_rx_prepare(iic, 0, 600000);//1s
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx
    } else if (rx_state  == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        printf("<err>:iic slave wait addr timeout!\n");
        local_irq_enable();
        return 0;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        printf("<err>:iic slave wait end!\n");
        local_irq_enable();
        return 0;
    }

    rx_state = hw_iic_slave_rx_byte(iic, &rx_buf[0]);//addr
    if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx
    } else if (rx_state == IIC_SLAVE_RX_ADDR_NO_MATCH) { //error
        printf("<err>:iic slave rx addr error!\n");
        local_irq_enable();
        return 0;
    } else if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx

    }
    rx_state = hw_iic_slave_rx_prepare(iic, 1, 100000);//1s, 1:收到数据应答
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx
    } else if (rx_state  == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        printf("<err>:iic slave wait reg timeout!\n");
        local_irq_enable();
        return 0;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        printf("iic slave wait end!\n");
        local_irq_enable();
        return 0;
    }

    rx_cnt = hw_iic_slave_rx_nbyte(iic, &rx_buf[1]);
    local_irq_enable();
    /* printf("rx addr:%x, slave addr:%x\n", rx_buf[0], hw_iic_slave_get_addr(iic)); */
    rx_cnt++;
    /* log_info_hexdump(rx_buf, rx_cnt); */
    printf("~~~~~iic rx polling end~~~~~\n");
    memset(rx_buf, 0, rx_cnt);
    return rx_cnt;
}

//tx协议:start,addr read,data0,data1,,,,,nack,stop
int hw_iic_slave_polling_tx(hw_iic_dev iic, u8 *tx_buf)
{
    u8 slave_rx_data[3] = {0, 0, 0};
    int rx_cnt = 0, tx_cnt = 0;
    int rx_state = 0;
    printf("--iic slave polling tx --\n");
    local_irq_disable();//关闭所有中断
    rx_state = hw_iic_slave_rx_prepare(iic, 0, 600000);//1s
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        printf("<err>:iic slave wait addr timeout!\n");
        local_irq_enable();
        return 0;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        printf("<err>:iic slave wait end!\n");
        local_irq_enable();
        return 0;
    }

    rx_state = hw_iic_slave_rx_byte(iic, &slave_rx_data[rx_cnt++]);//addr
    if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx
    } else if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx
        hw_iic_slave_tx_byte(iic, tx_buf[tx_cnt++]);
        goto _tx_strat;
    } else { //error
        printf("<err>:iic slave rx addr error!\n");
        local_irq_enable();
        return 0;
    }

_tx_strat:
    tx_cnt = hw_iic_slave_tx_nbyte(iic, &tx_buf[tx_cnt]);

    local_irq_enable();
    /* printf("rx0 addr:%x, slave addr:%x\n", slave_rx_data[0], hw_iic_slave_get_addr(iic)); */
    /* log_info_hexdump(slave_rx_data, rx_cnt); */
    printf("~~~~~iic tx polling end~~~~~\n");
    return tx_cnt + 1; //ok
}


void hw_iic_master_module_init()
{
    struct iic_master_config hw_iic_config = {
        .role = IIC_MASTER,
        .scl_io = PMC_HW_IIC_SCL,
        .sda_io = PMC_HW_IIC_SDA,
        .io_mode = PMC_HW_IIC_PU_EN	,//1:上拉或0:浮空
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,   //enum GPIO_HDRIVE 0:2.4MA, 1:8MA, 2:26.4MA, 3:40MA
        .master_frequency = PMC_HW_IIC_FREQ	, //软件iic频率不准(hz)
        .io_filter = 1,  //软件无效
        .ie_en = 0,//1:注册中断
        .irq_priority = 3,//优先级
    };
    hw_iic_dev iic_dev = HW_IIC_0;
    hw_iic_init(HW_IIC_0, &hw_iic_config);
}
