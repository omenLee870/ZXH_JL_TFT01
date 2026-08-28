#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".norflash_espi.data.bss")
#pragma data_seg(".norflash_espi.data")
#pragma const_seg(".norflash_espi.text.const")
#pragma code_seg(".norflash_espi.text")
#endif

#include "app_config.h"
#include "norflash_sfc.h"
#include "norflash_espi.h"
#include "clock.h"
#include "asm/espi.h"

#undef LOG_TAG_CONST
#define LOG_TAG     "[FLASH_ESPI]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"


#define FLASH_POWERON_TIME       (1000)//1ms  flash 上电使用延时
#define FLASH_EXIT_LOWPOWER_TIME  (200)//200us  退出低功耗使用延时
#define FLASH_POWEROFF_TIME      (200)//200us  掉电延时

#if CONFIG_EX_FLASH_POWER_IO_CTRL
#define FLASH_POWER_ON_LEVEL      1//默认是1上电，如果不是修改这个
#else
#define FLASH_POWER_ON_LEVEL	0
#endif



#if 0//(defined(TCFG_NORFLASH_SFC_DEV_ENABLE) && TCFG_NORFLASH_SFC_DEV_ENABLE)
#define NORFLASH_ESPI_DRIVER_EN     1
#else
#define NORFLASH_ESPI_DRIVER_EN     0
#endif

#define ESPI_FLASH_CPU_READ_MODE_EN 0
#define NORFLASH_ESPI_DTR_EN        0//不支持
#define     ESPI_TSHSL              50//ns

static void __norflash_flash_power_check();

#if NORFLASH_ESPI_DRIVER_EN

static void enter_espi_cpu_code(void)
{
    /* cpu_suspend_other_core(CPU_SUSPEND_TYPE_SFC); */
    /* OS_ENTER_CRITICAL(); */
    espi_cpu_mode_resume();

    /* _exit_continue_mode(); */
}
static void exit_espi_cpu_code()
{
    espi_cpu_mode_suspend();
    /* _enter_continue_mode2(); */

    /* OS_EXIT_CRITICAL(); */
    /* cpu_resume_other_core(CPU_SUSPEND_TYPE_SFC); */
}


/****************************************************************/



struct flash_info_t {
    u32 id;
    u32 capacity;
    u8 uuid[16];
    OS_MUTEX mutex;
    u8 mutex_cunt;
    u8 is4byte_mode;
    u8 read_width;
    u8 read_mode;
};

static struct flash_info_t flash_info;


static void norflash_espi_enter_powerdown(void)
{
    const struct espi_io_t inst = {
        .data = WINBOND_POWER_DOWN,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    espi_hw_send_cmd(&inst);
}
static u8 norflash_espi_powerdown_release(u32 delay_us)//10>3us
{
    const struct espi_io_t inst = {
        .data = WINBOND_RELEASE,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    struct espi_data_t rx_data = {
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    espi_hw_ioctrl_read(&inst, NULL, &rx_data, 24);
    udelay(delay_us);
    return rx_data.data[0];
}

static u8 norflash_espi_wait_ok(u32 delay_ms)//return:1:err;0:ok
{
    u32 delay_us = delay_ms * 100;
    const struct espi_io_t inst = {
        .data = WINBOND_READ_SR1,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    struct espi_data_t rx_data = {
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    while (--delay_us) {
        espi_hw_ioctrl_read(&inst, NULL, &rx_data, 0);

        if (!(rx_data.data[0] & BIT(0))) {
            break;
        }
        udelay(10);
    }
    if (delay_us == 0) {
        return 1;//err
    }
    return 0;
}
static void norflash_espi_soft_reset(u32 delay_us)//10>3us
{
    struct espi_io_t inst = {
        .data = 0x66,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    espi_hw_send_cmd(&inst);

    inst.data = 0x99;
    espi_hw_send_cmd(&inst);
    udelay(delay_us);
}

static u32 norflash_espi_read_id()
{
    const struct espi_io_t inst = {
        .data = WINBOND_JEDEC_ID,//0x9f
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    struct espi_data_t rx_data = {
        .len = 3,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    espi_hw_ioctrl_read(&inst, NULL, &rx_data, 0);
    u32 id = rx_data.data[0] << 16 | rx_data.data[1] << 8 | rx_data.data[2];
    /* printf_buf(rx_data.data, 8); */
    memcpy((u8 *)&flash_info.id, (u8 *)&id, 4);
    flash_info.capacity = 64 * 1024 << ((id & 0xff) - 0x10);
    log_info("norflash_id:0x%x", flash_info.id);
    log_info("flash_capacity = %dK", flash_info.capacity / 1024);

    return id;
}
static void norflash_espi_read_uuid()//16bytes???
{
    const struct espi_io_t inst = {
        .data = WINBOND_UUID,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    u8 addr_len = 3;
    if (flash_info.is4byte_mode) {
        addr_len++;
    }
    const struct espi_io_t addr = {
        .data = 0,
        .len = addr_len,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    struct espi_data_t rx_data = {
        .len = 8,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    espi_hw_cs_io_cfg(0);
    espi_hw_manual_cs_out(0);
    espi_hw_ioctrl_read(&inst, &addr, &rx_data, 8);
    espi_hw_transfer_data_cfg(&rx_data, 1);
    memcpy(flash_info.uuid, rx_data.data, 8);
    espi_hw_transfer_data((u32 *)(flash_info.uuid + 8), 8, 1);
    espi_hw_manual_cs_out(1);
    espi_hw_cs_io_cfg(1);

    printf_buf(flash_info.uuid, 16);
}
static void norflash_espi_send_write_enable()
{
    const struct espi_io_t inst = {
        .data = WINBOND_WRITE_ENABLE,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    espi_hw_send_cmd(&inst);
}
static void norflash_espi_send_write_sr_enable()
{
    const struct espi_io_t inst = {
        .data = WINBOND_WRITE_SR_ENABLE,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    espi_hw_send_cmd(&inst);
}

static void norflash_espi_enter_4byte_addr()
{
    const struct espi_io_t inst = {
        .data = 0xb7,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    espi_hw_send_cmd(&inst);
}
static void norflash_espi_exit_4byte_addr()
{
    const struct espi_io_t inst = {
        .data = 0xe9,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    if (flash_info.is4byte_mode) {
        espi_hw_send_cmd(&inst);
    }
}
static void norflash_espi_read_status_reg(u8 *status_reg1, u8 *status_reg2)
{
    struct espi_io_t inst = {
        .data = WINBOND_READ_SR1,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    struct espi_data_t rx_data = {
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    espi_hw_ioctrl_read(&inst, NULL, &rx_data, 0);
    *status_reg1 = rx_data.data[0];
    log_debug("read status_reg1: 0x%x\n", *status_reg1);

    inst.data = WINBOND_READ_SR2;
    espi_hw_ioctrl_read(&inst, NULL, &rx_data, 0);
    *status_reg2 = rx_data.data[0];
    log_debug("read status_reg2: 0x%x\n", *status_reg2);

    inst.data = 0x15;
    espi_hw_ioctrl_read(&inst, NULL, &rx_data, 0);
    u8 status_reg3 = rx_data.data[0];
    log_debug("read status_reg3: 0x%x\n", status_reg3);

}
static void norflash_espi_write_status_reg(u8 status_reg1, u8 status_reg2, u8 sr_mode, void (*norflash_write_enable)(void))
{
    norflash_write_enable();
    struct espi_io_t inst = {
        .data = WINBOND_WRITE_SR1,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    struct espi_data_t tx_data = {
        .data[0] = status_reg1,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    if (sr_mode) {//分开写
        espi_hw_ioctrl_write(&inst, NULL, &tx_data, 0);
        norflash_espi_wait_ok(2 * 60000);
        if (sr_mode == 2) {
            return;
        }
        norflash_write_enable();
        inst.data = WINBOND_WRITE_SR2;
        tx_data.data[0] = status_reg2;
        espi_hw_ioctrl_write(&inst, NULL, &tx_data, 0);
        norflash_espi_wait_ok(2 * 60000);
    } else {
        tx_data.data[1] = status_reg2;
        tx_data.len = 2;
        espi_hw_ioctrl_write(&inst, NULL, &tx_data, 0);
        norflash_espi_wait_ok(2 * 60000);
    }
}

static void norflash_espi_set_quad(u8 enable)
{
    u8 status_reg1, status_reg2;
    norflash_espi_read_status_reg(&status_reg1, &status_reg2);
    if ((!!(status_reg2 & BIT(1)))  == enable) {
        log_debug("norflash_set_quad succ\n");
        return;
    }
    if (enable) {
        status_reg2 |= BIT(1);
    } else {
        status_reg2 &= ~BIT(1);
    }
    const u8 w_reg1 = status_reg1 & 0xfc; // clear WEL WIP
    const u8 w_reg2 = status_reg2;
    //write reg  分开两次写寄存器，博雅flash不支持连续写,普冉分开写才能保存qe位
    norflash_espi_write_status_reg(w_reg1, w_reg2, 1, norflash_espi_send_write_enable);
    norflash_espi_read_status_reg(&status_reg1, &status_reg2);
    if ((!!(status_reg2 & BIT(1)))  == enable) {
        log_debug("norflash_set_quad succ\n");
        return;
    }
    norflash_espi_write_status_reg(w_reg1, w_reg2, 0, norflash_espi_send_write_enable);
}

/* static u32 norflash_already_in_continue_mode = 0; */
#define norflash_espi_mutex_init()   os_mutex_create(&flash_info.mutex)

void norflash_espi_mutex_enter()
{
    ASSERT((0 == cpu_in_irq() && 0 == cpu_irq_disabled()));
    os_mutex_pend(&flash_info.mutex, 0);
    flash_info.mutex_cunt++;
    /* SFR(JL_ESPI->CON0,  0, 1,  1);//espi off */
}

void norflash_espi_mutex_exit()
{
    /* IcuWaitIdle() ; */
    /* DcuWaitIdle() ; */
    /* SFR(JL_ESPI->CON0,  0, 1,  0);//espi off */
    ASSERT(flash_info.mutex_cunt);
    flash_info.mutex_cunt--;
    os_mutex_post(&flash_info.mutex);
}



#if ESPI_FLASH_CPU_READ_MODE_EN
static u32 norflash_espi_cpu_read(u8 *buf, u32 addr, u32 len, u8 dtr_en) //len:any//no continue,no 0单线 //no bbh
{
    if ((dtr_en == 0) && (flash_info.read_mode > 0) && (flash_info.read_width == 2)) { //espi不支持03h,bbh
        flash_info.read_mode = 0;
    }
    u8 flash_mode = ESPI_STR;
    u8 addr_mode = ESPI_1_LINE;
    u8 alby_mode = 0;
    u8 alby_data = 0;
    u8 read_dummy = 0;
    u8 dat_mode = ESPI_1_LINE;
    u8 addr_and_mode[5] = {0};
    u8 flash_read_cmd = WINBOND_FAST_READ_DATA;
    if (flash_info.read_mode == 0) {
        flash_read_cmd |= (0x18 << (flash_info.read_width / 2));  //width: 0, 1, 2, 4
        if (flash_read_cmd == 0x1b) {
            flash_read_cmd = 0x0b;
        }
        read_dummy = 8;
        addr_mode = ESPI_1_LINE;
        alby_mode = 0;
        dat_mode = flash_info.read_width == 4 ? 3 : flash_info.read_width;
        if (dtr_en) {
            flash_read_cmd = 0x0d;
            flash_mode = ESPI_DTR;//0,1
            read_dummy = 6;
        }
    } else {
        ASSERT(flash_info.read_width > 1, "espi cpu read width error!");
        flash_read_cmd |= (0x2e << flash_info.read_width);    //width: 2, 4
        addr_mode = flash_info.read_width == 4 ? 3 : flash_info.read_width;
        alby_mode = flash_info.read_width == 4 ? 3 : flash_info.read_width;
        alby_data = 0xa0 * (flash_info.read_mode - 1);
        dat_mode = flash_info.read_width == 4 ? 3 : flash_info.read_width;
        if (flash_info.read_width == 4) {
            read_dummy = 4;
        }

        if (dtr_en) {
            SFR(flash_read_cmd, 1, 2, 2);
            flash_mode = ESPI_DTR;//0,1
            if (flash_info.read_width == 2) {
                read_dummy = 4;
            } else {
                read_dummy = 7;
            }
        }
    }
    /* addr += get_device_offset(); */
    const struct espi_io_t inst = {
        .data = flash_read_cmd,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    const struct espi_io_t e_addr = {
        .data = addr,
        .len = 3 + flash_info.is4byte_mode,
        .dtr = flash_mode,
        .line = addr_mode,
    };
    const struct espi_io_t alby = {
        .data = alby_data,
        .len = 1,
        .dtr = flash_mode,
        .line = alby_mode,//ESPI_1_LINE
    };
    struct espi_data_t rx_data = {
        .dtr = flash_mode,
        .line = dat_mode,
    };
    rx_data.len = len > 8 ? 8 : len;

    espi_hw_cs_io_cfg(0);
    espi_hw_manual_cs_out(0);
    espi_hw_ioctrl(&inst, &e_addr, &alby, NULL, &rx_data, read_dummy);
    memcpy(buf, rx_data.data, rx_data.len);
    espi_hw_transfer_data_cfg(&rx_data, 1);
    len -= rx_data.len;
    buf += rx_data.len;
    while (len) {
        rx_data.len = len > 8 ? 8 : len;
        espi_hw_transfer_data((u32 *)buf, rx_data.len, 1);
        len -= rx_data.len;
        buf += rx_data.len;
    }
    espi_hw_manual_cs_out(1);
    espi_hw_cs_io_cfg(1);
    return len;
}
#endif

static void norflash_espi_cpu_write_page_no_wait_ok(u8 *buf, u32 addr, u32 len)//len<8)
{
    u8 dat_mode = ESPI_1_LINE;
    u8 w_cmd = WINBOND_PAGE_PROGRAM_X1;
    if (flash_info.read_width == 4) {
        dat_mode = ESPI_4_LINE;
        w_cmd = WINBOND_PAGE_PROGRAM_X4;
    }
    const struct espi_io_t inst = {
        .data = w_cmd,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    const struct espi_io_t e_addr = {
        .data = addr,
        .len = 3 + flash_info.is4byte_mode,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    struct espi_data_t tx_data = {
        .dtr = ESPI_STR,
        .line = dat_mode,
    };
    tx_data.len = len > 8 ? 8 : len;
    memcpy(tx_data.data, buf, tx_data.len);
    espi_hw_clk_change(0, ESPI_NORFLASH_WRITE_FREQ);

    norflash_espi_send_write_enable();

    espi_hw_cs_io_cfg(0);
    espi_hw_manual_cs_out(0);
    espi_hw_ioctrl_write(&inst, &e_addr, &tx_data, 0);
    espi_hw_transfer_data_cfg(&tx_data, 0);
    len -= tx_data.len;
    buf += tx_data.len;
    while (len) {
        tx_data.len = len > 8 ? 8 : len;
        espi_hw_transfer_data((u32 *)buf, tx_data.len, 0);
        len -= tx_data.len;
        buf += tx_data.len;
    }
    espi_hw_manual_cs_out(1);
    espi_hw_cs_io_cfg(1);
    espi_hw_clk_change(1, ESPI_NORFLASH_READ_FREQ);
}


u32 sfc1_flash_addr2cpu_addr(u32 offset)
{
    return ESPI_FLASH_ADDR2MMP_CACHE_ADDR(offset);
}

u32 sfc1_cpu_addr2flash_addr(u32 offset)
{
    return ESPI_MMP_CACHE_ADDR2FLASH_ADDR(offset);
}



extern void sfc_drop_cache(void *ptr, u32 len);
static u8 norflash_espi_cpu_write_page(u8 *buf, u32 addr, u32 len)
{
    norflash_espi_cpu_write_page_no_wait_ok(buf, addr, len);
    sfc_drop_cache((u8 *)(ESPI_FLASH_ADDR2MMP_CACHE_ADDR(addr)), len);
    u8 r = norflash_espi_wait_ok(2 * 60000);//2min
    return r;
}
static u32 norflash_espi_cpu_write(u8 *buf, u32 addr, u32 len)
{
    /* if ((flash_support_continue_read_mode) && (norflash_already_in_continue_mode == 1)) { */
    /*     norflash_exit_continue_mode(); */
    /* } */
    /* log_debug("w %x %d\n", addr, len); */
    u32 _len = len;
    u32 first_page_len = 256 - (addr % 256);
    u32 cnt = len > first_page_len ? first_page_len : len;
    while (len) {
        norflash_espi_cpu_write_page(buf, addr, cnt);
        addr += cnt;
        buf += cnt;
        len -= cnt;
        cnt = len > 256 ? 256 : len;
    }
    return _len;
}
static u8 norflash_espi_cpu_eraser(u32 eraser_cmd, u32 addr)
{
    u32 len = 0;
    switch (eraser_cmd) {
    case WINBOND_PAGE_ERASE:
    case 256:
        len = 1 * 256;
        eraser_cmd = WINBOND_PAGE_ERASE;
        break;
    case WINBOND_SECTOR_ERASE:
    case 4096:
        len = 4 * 1024;
        eraser_cmd = WINBOND_SECTOR_ERASE;
        break;
    case WINBOND_BLOCK_ERASE:
    case 65536:
        len = 64 * 1024;
        eraser_cmd = WINBOND_BLOCK_ERASE;
        break;
    case WINBOND_CHIP_ERASE:
        len = flash_info.capacity;
        eraser_cmd = WINBOND_CHIP_ERASE;
        break;
    }
    addr &= ~(len - 1);
    /* addr += get_device_offset(); */
    const struct espi_io_t inst = {
        .data = eraser_cmd,
        .len = 1,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };
    const struct espi_io_t e_addr = {
        .data = addr,
        .len = 3 + flash_info.is4byte_mode,
        .dtr = ESPI_STR,
        .line = ESPI_1_LINE,
    };

    norflash_espi_send_write_enable();
    if (eraser_cmd != WINBOND_CHIP_ERASE) {
        espi_hw_ioctrl_write(&inst, &e_addr, NULL, 0);
    } else {
        espi_hw_send_cmd(&inst);
    }
    sfc_drop_cache((u8 *)(ESPI_FLASH_ADDR2MMP_CACHE_ADDR(addr)), len);
    u32 ret = norflash_espi_wait_ok(2 * 60000);
    return ret;
}


void norflash_espi_enter_4byte_addr_reinit()
{
    if (flash_info.is4byte_mode == 1) {
        enter_espi_cpu_code();
        norflash_espi_enter_4byte_addr();
        exit_espi_cpu_code();
    }
}


void espi_norflash_init(u8 read_mode, u8 width)
{

    norflash_espi_mutex_init();

#if (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT)
    hw_open_espi_power(TCFG_EX_FLASH_POWER_IO, FLASH_POWER_ON_LEVEL, FLASH_POWERON_TIME, width);
#endif


    espi_hw_init(ESPI_NORFLASH_READ_FREQ, ESPI_TSHSL, 0x1f, width);
    flash_info.read_mode = read_mode;
    flash_info.read_width = width;
    log_info("--espi_f_read_mode:%d,espi_f_read_width:%d\n", flash_info.read_mode, flash_info.read_width);
#if 1
    norflash_espi_mutex_enter();
    norflash_espi_powerdown_release(10);
    norflash_espi_soft_reset(10);
    norflash_espi_wait_ok(200);//200ms
    /* } */
    udelay(30);
    norflash_espi_read_id();
    flash_info.is4byte_mode = 0;
    if (flash_info.capacity > 16 * 1024 * 1024) {
        norflash_espi_enter_4byte_addr();
        flash_info.is4byte_mode = 1;
    }
    log_info("is4byte_mode = %d\n", flash_info.is4byte_mode);
    norflash_espi_read_uuid();

    if (width == 4) {
        norflash_espi_set_quad(1);
    }
    norflash_espi_mutex_exit();

    struct espi_control r_cmd;
    struct espi_control w_cmd;

    if (width == 4) {
        r_cmd.cmd = 0xeb;
        r_cmd.dummy = 4;
        r_cmd.width[0] = ESPI_1_LINE;//cmd
        r_cmd.width[1] = ESPI_4_LINE;//addr
        r_cmd.width[2] = ESPI_4_LINE;//alby
        r_cmd.width[3] = ESPI_4_LINE;//data

        w_cmd.cmd = 0x32;
        w_cmd.dummy = 0;
        w_cmd.width[0] = ESPI_1_LINE;//cmd
        w_cmd.width[1] = ESPI_1_LINE;//addr
        w_cmd.width[2] = ESPI_STAGE_DIS;//alby
        w_cmd.width[3] = ESPI_4_LINE;//data

    } else if (width == 2) {
        r_cmd.cmd = 0x3b;
        r_cmd.dummy = 8;
        r_cmd.width[0] = ESPI_1_LINE;//cmd
        r_cmd.width[1] = ESPI_1_LINE;//addr
        r_cmd.width[2] = ESPI_STAGE_DIS;//alby
        r_cmd.width[3] = ESPI_2_LINE;//data

        w_cmd.cmd = 0x02;
        w_cmd.dummy = 0;
        w_cmd.width[0] = ESPI_1_LINE;//cmd
        w_cmd.width[1] = ESPI_1_LINE;//addr
        w_cmd.width[2] = ESPI_STAGE_DIS;//alby
        w_cmd.width[3] = ESPI_1_LINE;//data

    } else if (width <= 1) {
        r_cmd.cmd = 0x0b;
        r_cmd.dummy = 8;
        r_cmd.width[0] = ESPI_1_LINE;//cmd
        r_cmd.width[1] = ESPI_1_LINE;//addr
        r_cmd.width[2] = ESPI_STAGE_DIS;//alby
        r_cmd.width[3] = ESPI_1_LINE;//data

        w_cmd.cmd = 0x02;
        w_cmd.dummy = 0;
        w_cmd.width[0] = ESPI_1_LINE;//cmd
        w_cmd.width[1] = ESPI_1_LINE;//addr
        w_cmd.width[2] = ESPI_STAGE_DIS;//alby
        w_cmd.width[3] = ESPI_1_LINE;//data

    }

    espi_mmp_mode_xn_init(&r_cmd, &w_cmd, flash_info.is4byte_mode); //1,2,4
#endif
}

u32 espi_norflash_read(u8 *buf, u32 addr, u32 len)//len:any//no continue,no 0单线 //no bbh
{

    norflash_espi_mutex_enter();
    __norflash_flash_power_check();
    ASSERT(espi_get_cpu_mode() == 0, "espi:cpu mode!");
#if ESPI_FLASH_CPU_READ_MODE_EN
    enter_espi_cpu_code();
    norflash_espi_cpu_read(buf, addr, len, 0);
    exit_espi_cpu_code();
#else
    espi_wait_pnd();
    memcpy(buf, (u8 *)ESPI_FLASH_ADDR2MMP_CACHE_ADDR(addr), len);
#endif
    norflash_espi_mutex_exit();

    return len;
}
u32 espi_norflash_write(u8 *buf, u32 addr, u32 len)
{
    norflash_espi_mutex_enter();
    __norflash_flash_power_check();
    enter_espi_cpu_code();
    norflash_espi_cpu_write(buf, addr, len);
    exit_espi_cpu_code();
    norflash_espi_mutex_exit();
    return len;
}
static int espi_norflash_eraser(u32 cmd, u32 addr)
{
    u32 len;
    if (cmd == IOCTL_ERASE_SECTOR) {
        cmd = WINBOND_SECTOR_ERASE;//0x20;
        len = 4 * 1024;
    } else if (cmd == IOCTL_ERASE_BLOCK) {
        cmd = WINBOND_BLOCK_ERASE;//0xD8;
        len = 64 * 1024;
    } else if (cmd == IOCTL_ERASE_PAGE) {
        cmd = WINBOND_PAGE_ERASE;//0x81;
        len = 256;
    } else if (cmd == IOCTL_ERASE_CHIP) {
        cmd = WINBOND_CHIP_ERASE;
        len = flash_info.capacity;
    } else {
        return -1;
    }
    addr &= ~(len - 1);
    if (addr >= flash_info.capacity) {
        return -1;
    }

    enter_espi_cpu_code();
    norflash_espi_cpu_eraser(cmd, addr);
    exit_espi_cpu_code();
    return 0;
}
void espi_norflash_enter_powerdown()
{
    /* norflash_espi_mutex_enter(); */
    enter_espi_cpu_code();
    norflash_espi_enter_powerdown();
    exit_espi_cpu_code();
    /* norflash_espi_mutex_exit(); */
}
u8 espi_norflash_powerdown_release(u32 delay_us)
{
    /* norflash_espi_mutex_enter(); */
    enter_espi_cpu_code();
    u8 ret = norflash_espi_powerdown_release(delay_us ? delay_us : 10); //10>3us
    exit_espi_cpu_code();
    /* norflash_espi_mutex_exit(); */
    return ret;
}

int espi_norflash_ioctl(u32 cmd, u32 arg)
{
    int ret = 0;
    norflash_espi_mutex_enter();
    __norflash_flash_power_check();

    switch (cmd) {
    case IOCTL_GET_CAPACITY:
        *((u32 *)arg) = flash_info.capacity;
        break;
    case IOCTL_GET_ID:
        *((u32 *)arg) = flash_info.id;
        break;

    case IOCTL_GET_SECTOR_SIZE:
        *((u32 *)arg) = 4096;
        break;
    case IOCTL_GET_BLOCK_SIZE:
        *((u32 *)arg) = 64 * 1024;
        break;
    case IOCTL_ERASE_PAGE:
    case IOCTL_ERASE_SECTOR:
    case IOCTL_ERASE_BLOCK:
    case IOCTL_ERASE_CHIP:
        ret = espi_norflash_eraser(cmd, arg);
        break;

    case IOCTL_CMD_FLASH_SET_OTP_LOCK:
        break;
    case IOCTL_CMD_FLASH_CFG_WPS:
        break;
    case IOCTL_POWER_RESUME:
        espi_norflash_powerdown_release(arg);
        break;
    case IOCTL_POWER_SUSPEND:
        espi_norflash_enter_powerdown();
        break;
    case IOCTL_CMD_FLASH_SET_WPS_LOCK:
        u32 *data = (u32 *)arg;
        break;
    default :
        ret = -ENOTTY;
        break;
    }

    norflash_espi_mutex_exit();
    return ret;
}



/*
 * 对ops的读写单位有另外需求，或者驱动内部是否支持擦除，可以参照上面的ops，
 * 不同条件自由组合，建立新的ops
 */

#define EX_FLASH_POWER_WORK     (0)
#define EX_FLASH_POWER_CLOSE    (1)
#define EX_FLASH_POWER_CLOSING  (2)
#define EX_FLASH_POWER_OPENING  (3)

#define USER_MASK_TYPE       (BIT(16)|BIT(17))//删消息池使用
static u16 last_id = 1;//删消息池使用


static volatile u8 extern_flash_status = EX_FLASH_POWER_WORK;


extern void udelay(u32 us);

static void norflash_flash_poweroff(int priv)
{
#if (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT)
    os_mutex_pend(&flash_info.mutex, 0);
    if (extern_flash_status != EX_FLASH_POWER_CLOSING) {
        os_mutex_post(&flash_info.mutex);
        return;
    }
    hw_close_espi_power(TCFG_EX_FLASH_POWER_IO, !FLASH_POWER_ON_LEVEL, FLASH_POWEROFF_TIME);
    extern_flash_status = EX_FLASH_POWER_CLOSE;
    os_mutex_post(&flash_info.mutex);
#else
    os_mutex_pend(&flash_info.mutex, 0);
    if (extern_flash_status != EX_FLASH_POWER_CLOSING) {
        os_mutex_post(&flash_info.mutex);
        return;
    }
    //1是操作外挂
    espi_norflash_enter_powerdown();
    extern_flash_status = EX_FLASH_POWER_CLOSE;
    os_mutex_post(&flash_info.mutex);
#endif
}





static void __norflash_flash_power_check()
{
#if (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT)

    u8 width = flash_info.read_width;

    if (!extern_flash_status) {
        return;
    }
    local_irq_disable();
    if (extern_flash_status == EX_FLASH_POWER_CLOSING) {
        os_taskq_del_type("app_core", Q_CALLBACK | last_id | USER_MASK_TYPE);
        extern_flash_status = EX_FLASH_POWER_WORK;
    } else if (extern_flash_status == EX_FLASH_POWER_CLOSE) {

        hw_open_espi_power(TCFG_EX_FLASH_POWER_IO, FLASH_POWER_ON_LEVEL, FLASH_POWERON_TIME, width);

        norflash_espi_enter_4byte_addr_reinit();//32m flash使用
        extern_flash_status = EX_FLASH_POWER_WORK;
    }
    local_irq_enable();
#else

    if (!extern_flash_status) {
        return;
    }

    local_irq_disable();
    if (extern_flash_status == EX_FLASH_POWER_CLOSING) {
        os_taskq_del_type("app_core", Q_CALLBACK | last_id | USER_MASK_TYPE);
        extern_flash_status = EX_FLASH_POWER_WORK;
    } else if (extern_flash_status == EX_FLASH_POWER_CLOSE) {
        local_irq_enable();
        espi_norflash_powerdown_release(FLASH_EXIT_LOWPOWER_TIME);
        local_irq_disable();
        extern_flash_status = EX_FLASH_POWER_WORK;
    }
    local_irq_enable();

#endif
}



void ex_norflash_poweroff(void)
{
    os_mutex_pend(&flash_info.mutex, 0);

    if (extern_flash_status == EX_FLASH_POWER_CLOSE) {
        os_mutex_post(&flash_info.mutex);
        return;
    }

    extern_flash_status = EX_FLASH_POWER_CLOSING;
    norflash_flash_poweroff(0);

    os_mutex_post(&flash_info.mutex);
}



void ex_norflash_poweron(void)
{
    os_mutex_pend(&flash_info.mutex, 0);
    __norflash_flash_power_check();
    os_mutex_post(&flash_info.mutex);
}



static u8 extern_flash_handler(u32 timeout)
{
    int msg[3];
    ASSERT(extern_flash_status != EX_FLASH_POWER_OPENING);
    if (extern_flash_status == EX_FLASH_POWER_CLOSE) {
        return 0;
    }
    if (extern_flash_status == EX_FLASH_POWER_CLOSING) {
        return 1;
    }
    extern_flash_status = EX_FLASH_POWER_CLOSING;
    msg[0] = (int)norflash_flash_poweroff;
    msg[1] = 1;
    msg[2] = (int)NULL;
    last_id++;
    if (!last_id) {
        last_id = 1;
    }
    os_taskq_post_type("app_core", Q_CALLBACK | last_id | USER_MASK_TYPE, 3, msg);

    return 1;
}

static u8 extern_flash_handler1(u32 timeout)
{
    return 0;
}

//低功耗线程请求所有模块关闭，由对应线程处理
REGISTER_LP_REQUEST(power_flash_target) = {
    .name = "extern_flash",
    .request_enter = extern_flash_handler,
    .request_exit = extern_flash_handler1,
};



#if 0
void norflash_espi_test()
{
    log_info("**********espi test**************");
    espi_norflash_init(1, 4);
    espi_hw_info_dump();

    /* espi_hw_dtr_cfg(1); */
    u8 rx_buf[300];
    u8 tx_buf[300];
    u32 test_addr = 10;
    u32 test_len = 280;
    memset(rx_buf, 0, sizeof(rx_buf));
    espi_norflash_read(rx_buf, 5, test_len);
    printf_buf(rx_buf, test_len);
#if 0
    espi_norflash_eraser(IOCTL_ERASE_SECTOR, test_addr);
    memset(rx_buf, 0, sizeof(rx_buf));
    espi_norflash_read(rx_buf, test_addr, test_len);
    printf_buf(rx_buf, test_len);

    for (u16 i = 0; i < sizeof(tx_buf); i++) {
        tx_buf[i] = i;
    }
    espi_norflash_write(tx_buf, test_addr, test_len);
    memset(rx_buf, 0, sizeof(rx_buf));
    espi_norflash_read(rx_buf, 0, sizeof(rx_buf));
    printf_buf(rx_buf, sizeof(rx_buf));
#else
    u8 *copy_test_addr = (u8 *)(ESPI_FLASH_ADDR2MMP_CACHE_ADDR(0));
    printf_buf(copy_test_addr + 0x0a, 60);
    printf("\n--func=%s(),%d\n", __FUNCTION__, __LINE__);

    espi_norflash_eraser(IOCTL_ERASE_SECTOR, test_addr);
    memset(rx_buf, 0, sizeof(rx_buf));
    memcpy(rx_buf, copy_test_addr + test_addr, test_len);
    printf_buf(rx_buf, test_len);

    for (u16 i = 0; i < sizeof(tx_buf); i++) {
        tx_buf[i] = i;
    }
    espi_norflash_write(tx_buf, test_addr, test_len);
    memset(rx_buf, 0, sizeof(rx_buf));
    memcpy(rx_buf, copy_test_addr + 0, sizeof(rx_buf));
    printf_buf(rx_buf, sizeof(rx_buf));

#endif
    if (memcmp(rx_buf + test_addr, tx_buf, 255)) {
        log_info("psram test error----------------------------------------:\n");
    } else {
        log_info("succ\n");
    }
}
#endif

#endif

