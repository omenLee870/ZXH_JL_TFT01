#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".norflash_sfc.data.bss")
#pragma data_seg(".norflash_sfc.data")
#pragma const_seg(".norflash_sfc.text.const")
#pragma code_seg(".norflash_sfc.text")
#endif
#include "system/includes.h"
#include "app_config.h"
#include "norflash_sfc.h"
#include "power/power_manage.h"
#include "asm/power/p33/p33_api.h"
#include "device/sdmmc/sdmmc.h"

#if (defined(TCFG_NORFLASH_SFC_DEV_ENABLE) && TCFG_NORFLASH_SFC_DEV_ENABLE)

#undef LOG_TAG_CONST
#define LOG_TAG     "[FLASH_SFC]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#define FLASH_POWERON_TIME       (1000)//1ms  flash 上电使用延时
#define FLASH_EXIT_LOWPOWER_TIME  (200)//200us  退出低功耗使用延时
#define FLASH_POWEROFF_TIME      (200)//200us  掉电延时

#if CONFIG_EX_FLASH_POWER_IO_CTRL
#define FLASH_POWER_ON_LEVEL      1//默认是1上电，如果不是修改这个
#else
#define FLASH_POWER_ON_LEVEL      0
#endif

static void __norflash_sfc_power_check(void);

#define MAX_NORFLASH_PART_NUM       4

struct norflash_partition {
    const char *name;
    u32 start_addr;
    u32 size;
    struct device device;
};

static struct norflash_partition nor_part[MAX_NORFLASH_PART_NUM];

struct norflash_info {
    u32 flash_id[2];
    u32 flash_capacity[2];
    int spi_num;
    u8 flash_cs_sel;//双flash,sfc1_cs_sel
    u8 spi_cs_io;
    u8 spi_r_width;
    u8 part_num;
    u8 open_cnt;
    u8 mutex_cunt;
    struct norflash_partition *const part_list;
    OS_MUTEX mutex;
    u32 max_end_addr;
    struct sfc_spi_platform_data *sfc_spi_data_ptr;
};

static struct norflash_info _norflash = {
    .spi_num = (int) - 1,
    .part_list = nor_part,
};

#define norflash_sfc_mutex_init()   os_mutex_create(&_norflash.mutex)

void norflash_sfc_mutex_enter()
{
    ASSERT((0 == cpu_in_irq() && 0 == cpu_irq_disabled()));
    os_mutex_pend(&_norflash.mutex, 0);
    _norflash.mutex_cunt++;
}

void norflash_sfc_mutex_exit()
{
    ASSERT(_norflash.mutex_cunt);
    _norflash.mutex_cunt--;
    os_mutex_post(&_norflash.mutex);
}



static int _norflash_read(u32 addr, u8 *buf, u32 len, u8 cache);
static int _norflash_eraser(u32 eraser, u32 addr);
static void _norflash_cache_sync_timer(void *priv);
static int _norflash_write_pages(u32 addr, u8 *buf, u32 len);

static u8 fspi1_width = 0;//双向
#define spi_cs_h()                  sfc1_spi_cs(1)
#define spi_cs_l()                  sfc1_spi_cs(0)
#define spi_read_byte()             sfc1_spi_read_byte(fspi1_width)
#define spi_write_byte(x)           sfc1_spi_write_byte(x)
#define spi_dma_read(buf, len)      sfc1_spi_dma_read_Xn(buf, len, fspi1_width)
#define spi_dma_write(buf, len)     sfc1_spi_dma_write_Xn(buf, len, 1)
#define spi_set_width(x)            fspi1_width = x;

static struct norflash_partition *norflash_find_part(const char *name)
{
    struct norflash_partition *part = NULL;
    u32 idx;
    for (idx = 0; idx < MAX_NORFLASH_PART_NUM; idx++) {
        part = &_norflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if (!strcmp(part->name, name)) {
            return part;
        }
    }
    return NULL;
}

static struct norflash_partition *norflash_new_part(const char *name, u32 addr, u32 size)
{
    struct norflash_partition *part;
    u32 idx;
    for (idx = 0; idx < MAX_NORFLASH_PART_NUM; idx++) {
        part = &_norflash.part_list[idx];
        if (part->name == NULL) {
            break;
        }
    }
    if (part->name != NULL) {
        log_error("create norflash part fail\n");
        return NULL;
    }
    memset(part, 0, sizeof(*part));
    part->name = name;
    part->start_addr = addr;
    part->size = size;
    if (part->start_addr + part->size > _norflash.max_end_addr) {
        _norflash.max_end_addr = part->start_addr + part->size;
    }
    _norflash.part_num++;
    return part;
}

static void norflash_delete_part(const char *name)
{
    struct norflash_partition *part;
    u32 idx;
    for (idx = 0; idx < MAX_NORFLASH_PART_NUM; idx++) {
        part = &_norflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if (!strcmp(part->name, name)) {
            part->name = NULL;
            _norflash.part_num--;
        }
    }
}

static int norflash_verify_part(struct norflash_partition *p)
{
    struct norflash_partition *part = NULL;
    u32 idx;
    for (idx = 0; idx < MAX_NORFLASH_PART_NUM; idx++) {
        part = &_norflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if ((p->start_addr >= part->start_addr) && (p->start_addr < part->start_addr + part->size)) {
            if (strcmp(p->name, part->name) != 0) {
                return -1;
            }
        }
    }
    return 0;
}



#define FLASH_CACHE_ENABLE  		1

#if FLASH_CACHE_ENABLE
static u32 flash_cache_addr;
static u8 *flash_cache_buf = NULL; //缓存4K的数据，与flash里的数据一样。
static u8 flash_cache_is_dirty;
static u16 flash_cache_timer;

#define FLASH_CACHE_SYNC_T_INTERVAL     60

static int _check_0xff(u8 *buf, u32 len)
{
    for (u32 i = 0; i < len; i ++) {
        if ((*(buf + i)) != 0xff) {
            return 1;
        }
    }
    return 0;
}
#endif


static u32 _pow(u32 num, int n)
{
    u32 powint = 1;
    int i;
    for (i = 1; i <= n; i++) {
        powint *= num;
    }
    return powint;
}

static int _norflash_wait_ok()
{
    u32 timeout = 50000;
    while (timeout--) {
        spi_cs_l();
        spi_write_byte(WINBOND_READ_SR1);
        u8 reg_1 = spi_read_byte();
        spi_cs_h();
        if (!(reg_1 & BIT(0))) {
            break;
        }
        udelay(10);
    }
    if (timeout == 0) {
        log_error("norflash_wait_ok timeout!\r\n");
        return 1;
    }
    return 0;
}

static void _norflash_send_write_enable()
{
    spi_cs_l();
    spi_write_byte(WINBOND_WRITE_ENABLE);
    spi_cs_h();
}
static u8 is4byte_mode[2];
u8 _norflash_get_is4byte_mode(u8 flash_index)
{
    return is4byte_mode[flash_index];
}
static void _norflash_send_addr(u32 addr)
{
    u8 is4byte = is4byte_mode[0];
    if (_norflash.flash_cs_sel) {
        is4byte = is4byte_mode[1];
    }
    if (is4byte) {
        spi_write_byte(addr >> 24);
    }
    spi_write_byte(addr >> 16);
    spi_write_byte(addr >> 8);
    spi_write_byte(addr);
}

static u32 ex_norflash_read_id()
{
    u8 id[3];
    sfc1_spi_enter_spi_mode();
    spi_cs_l();
    spi_write_byte(WINBOND_JEDEC_ID);
    for (u8 i = 0; i < sizeof(id); i++) {
        id[i] = spi_read_byte();
    }
    spi_cs_h();
    sfc1_spi_exit_spi_mode();
    return id[0] << 16 | id[1] << 8 | id[2];
}

static void ex_norflash_enter_4byte_addr()
{
    sfc1_spi_enter_spi_mode();
    spi_cs_l();
    spi_write_byte(0xb7);
    spi_cs_h();
    sfc1_spi_exit_spi_mode();
}

static void ex_norflash_exit_4byte_addr()
{
    sfc1_spi_enter_spi_mode();
    spi_cs_l();
    spi_write_byte(0xe9);
    spi_cs_h();
    sfc1_spi_exit_spi_mode();
}
static int ex_norflash_set_quad()
{
    sfc1_spi_enter_spi_mode();
    //read sta:
    spi_cs_l();
    spi_write_byte(WINBOND_READ_SR1);
    u8 reg_1 = spi_read_byte();
    spi_cs_h();
    spi_cs_l();
    spi_write_byte(WINBOND_READ_SR2);
    u8 reg_2 = spi_read_byte();
    spi_cs_h();

    /* printf("****norflash sta:0x%x,0x%x\n", reg_1, reg_2); */
    //write sta:
    _norflash_send_write_enable();
    reg_2 |= BIT(1);
    spi_cs_l();
    spi_write_byte(WINBOND_WRITE_SR2);
    spi_write_byte(reg_2);
    spi_cs_h();
    int reg = _norflash_wait_ok();

    sfc1_spi_exit_spi_mode();
    if (reg) {
        return -1;//error
    }
    return 0;//ok
}

/* ========== Flash 低功耗命令函数 ========== */

static void norflash_sfc_enter_powerdown(void)
{
    sfc1_spi_enter_spi_mode();
    spi_cs_l();
    spi_write_byte(WINBOND_POWER_DOWN);  // 0xB9
    spi_cs_h();
    sfc1_spi_exit_spi_mode();
}

static u8 norflash_sfc_powerdown_release(u32 delay_us)
{
    sfc1_spi_enter_spi_mode();
    spi_cs_l();
    spi_write_byte(WINBOND_RELEASE);    // 0xAB
    spi_read_byte();  // dummy byte 1
    spi_read_byte();  // dummy byte 2
    u8 dev_id = spi_read_byte();  // Device ID
    spi_cs_h();
    sfc1_spi_exit_spi_mode();
    udelay(delay_us);
    return dev_id;
}

static void norflash_sfc_soft_reset(u32 delay_us)
{
    sfc1_spi_enter_spi_mode();
    spi_cs_l();
    spi_write_byte(0x66);   // Enable Reset
    spi_cs_h();
    spi_cs_l();
    spi_write_byte(0x99);   // Reset
    spi_cs_h();
    sfc1_spi_exit_spi_mode();
    udelay(delay_us);
}

/* ========== 硬件电源 IO 控制 ========== */

#if (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT)
/* 外挂 flash 电源控制，按 TCFG_EX_FLASH_POWER_IO 配置选择供电方式:
 *   IO_PORT_LDO28  -> LDO28 供电(电压固定 3.1V)
 *   SD_PWR_SDPG    -> SDPG 供电
 *   普通 GPIO      -> GPIO 强驱供电(高/低电平使能见 CONFIG_EX_FLASH_POWER_IO_CTRL)
 */
static void sfc_flash_power_set(u8 enable)
{
#if (TCFG_EX_FLASH_POWER_IO == IO_PORT_LDO28)
    /* LDO28 供电 3.1V */
    iovdd28_config(enable, IOVDD28_VOL_3P1V);
#elif (TCFG_EX_FLASH_POWER_IO == SD_PWR_SDPG)
    /* SDPG 供电 */
    if (enable) {
        sdpg_config(SDPG_IO_OUTPUT_H_HD_HD0_SDPG_USER);
    } else {
        sdpg_config(SDPG_IO_HI_Z);
    }
#elif (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT)
    /* 普通 GPIO 强驱供电 */
    if (enable) {
        gpio_set_drive_strength(IO_PORT_SPILT(TCFG_EX_FLASH_POWER_IO), PORT_DRIVE_STRENGT_8p0mA);
        gpio_set_mode(IO_PORT_SPILT(TCFG_EX_FLASH_POWER_IO), CONFIG_EX_FLASH_POWER_IO_CTRL);
        udelay(200);
        gpio_set_drive_strength(IO_PORT_SPILT(TCFG_EX_FLASH_POWER_IO), PORT_DRIVE_STRENGT_64p0mA);
    } else {
        gpio_set_mode(IO_PORT_SPILT(TCFG_EX_FLASH_POWER_IO), !CONFIG_EX_FLASH_POWER_IO_CTRL);
    }
#endif
}
#endif

static int _norflash_init(const char *name, struct norflash_sfc_dev_platform_data *pdata)
{
    log_info("norflash_sfc_init >>>>");

    if (_norflash.spi_num == (int) - 1) {
        _norflash.spi_num = pdata->sfc_spi_pdata->spi_hw_index;
        _norflash.flash_id[0] = 0;
        _norflash.flash_capacity[0] = 0;
        if (pdata->sfc_spi_pdata->sfc_cs1_en) {
            _norflash.flash_id[1] = 0;
            _norflash.flash_capacity[1] = 0;
        }
        _norflash.flash_cs_sel = 0;
        norflash_sfc_mutex_init();
        sfc_flash_power_set(1);

        _norflash.max_end_addr = 0;
        _norflash.part_num = 0;
        _norflash.spi_r_width = pdata->sfc_spi_pdata->sfc_data_width;
        _norflash.sfc_spi_data_ptr = (struct sfc_spi_platform_data *)pdata->sfc_spi_pdata;
        spi_set_width(pdata->sfc_spi_pdata->sfc_data_width);
        fspi1_init(SFC_FSPI1_FREQ, pdata->sfc_spi_pdata);
    }
    ASSERT(_norflash.spi_r_width == pdata->sfc_spi_pdata->sfc_data_width);

    struct norflash_partition *part;
    part = norflash_find_part(name);
    if (!part) {
        part = norflash_new_part(name, pdata->start_addr, pdata->size);
        ASSERT(part, "not enough norflash partition memory in array\n");
        ASSERT(norflash_verify_part(part) == 0, "norflash partition %s overlaps\n", name);
        log_info("norflash new partition %s\n", part->name);
    } else {
        ASSERT(0, "norflash partition name already exists\n");
    }
    return 0;
}

static void clock_critical_enter()
{

}
static void clock_critical_exit()
{
    if (!(_norflash.flash_id[0] == 0 || _norflash.flash_id[0] == 0xffff)) {
        JL_FSPI1->BAUD = clk_get("sys") / SFC_FSPI1_FREQ;
    }
}
HSB_CRITICAL_HANDLE_REG(sfc1_fspi1_norflash, clock_critical_enter, clock_critical_exit);
/* CLOCK_CRITICAL_HANDLE_REG(sfc1_fspi1_norflash, clock_critical_enter, clock_critical_exit); */

//最好放在保护区间，避免被篡改
//只在open,写擦操作cs切换(open未放中断); 读切换4addr
static void _norflash_cs_sel(u8 flash1_cs_en)//spi cs sel
{
    if (flash1_cs_en) {
        _norflash.flash_cs_sel = 1;
    } else {
        _norflash.flash_cs_sel = 0;
    }
}
u8 _norflash_cs_get()//spi cs sel
{
    return _norflash.flash_cs_sel;
}
static int _norflash_open(void *arg)
{

    int reg = 0;
    os_mutex_pend(&_norflash.mutex, 0);
    log_info("norflash open\n");
    if (!_norflash.open_cnt) {
        u8 flash_num = 1;
        if (_norflash.sfc_spi_data_ptr->sfc_cs1_en) {
            flash_num++;
        }
        for (u8 i = 0; i < flash_num; i++) {
            if (i == 1) {
                _norflash_cs_sel(1);//spi cs sel
            }
            _norflash.flash_id[i] = ex_norflash_read_id();

            log_info("ex_norflash%d id: 0x%x\n", i, _norflash.flash_id[i]);
            if ((_norflash.flash_id[i] == 0) || (_norflash.flash_id[i] == 0xffffff)) {
                log_error("read exnorflash%d id error !\n", i);
                reg = -ENODEV;
                goto __exit;
            }
            u32 capacity_id = _norflash.flash_id[i] & 0xff;
            if (capacity_id >= 0x30) {
                capacity_id = capacity_id - 0x30 + 0x10;
            } else if (capacity_id >= 0x20) {
                capacity_id = capacity_id - 0x20 + 0x1a;
            }
            //cppcheck-suppress integerOverflowCond
            _norflash.flash_capacity[i] = 64 * 1024 << ((capacity_id & 0xff) - 0x10);
            log_info("exnorflash%d capacity: %dK\n", i, _norflash.flash_capacity[i] / 1024);

            is4byte_mode[i] = 0;
            if (_norflash.flash_capacity[i] > 16 * 1024 * 1024) {
                is4byte_mode[i] = 1;
                ex_norflash_enter_4byte_addr();
            }
            if (_norflash.spi_r_width == 4) {
                ex_norflash_set_quad();
            }
            if ((i == 1) && (_norflash.flash_capacity[0] < 1 * 1024 * 1024)) {
                log_error("flash0 cap:%dk <1M,no SUPPORT flash1", _norflash.flash_capacity[0] / 1024);
            }
        }
        sfc_spi_init(_norflash.sfc_spi_data_ptr);

#if FLASH_CACHE_ENABLE
        flash_cache_buf = (u8 *)malloc(4096);
        ASSERT(flash_cache_buf, "flash_cache_buf is not ok\n");
        flash_cache_addr = 4096;//先给一个大于4096的数
        _norflash_read(0, flash_cache_buf, 4096, 1);
        flash_cache_addr = 0;
#endif
        log_info("norflash open success !\n");
    }
    if (_norflash.flash_id[0] == 0 || _norflash.flash_id[0] == 0xffffff)  {
        log_error("re-open exnorflash0 id error !\n");
        reg = -EFAULT;
        goto __exit;
    }
    if (_norflash.sfc_spi_data_ptr->sfc_cs1_en == 0) {
        ASSERT(_norflash.max_end_addr <= _norflash.flash_capacity[0], "max partition end address is greater than flash capacity\n");
    } else {
        ASSERT(_norflash.max_end_addr <= _norflash.flash_capacity[0] + _norflash.flash_capacity[1], "max partition end address is greater than flash capacity\n");
    }
    _norflash.open_cnt++;

__exit:
    os_mutex_post(&_norflash.mutex);
    return reg;
}
u32 _norflash_get_cap(u8 flash_index)
{
    return _norflash.flash_capacity[flash_index];
}

static int _norflash_close(void)
{
    os_mutex_pend(&_norflash.mutex, 0);
    log_info("norflash close\n");
    if (_norflash.open_cnt) {
        _norflash.open_cnt--;
    }
    if (!_norflash.open_cnt) {
#if FLASH_CACHE_ENABLE
        if (flash_cache_is_dirty) {
            flash_cache_is_dirty = 0;
            _norflash_eraser(IOCTL_ERASE_SECTOR, flash_cache_addr);
            _norflash_write_pages(flash_cache_addr, flash_cache_buf, 4096);
        }
        free(flash_cache_buf);
        flash_cache_buf = NULL;
#endif
        log_info("norflash close done\n");
    }
    sfc1_spi_deinit();
    os_mutex_post(&_norflash.mutex);
    return 0;
}

static int _norflash_read(u32 addr, u8 *buf, u32 len, u8 cache)
{
    int ret = 0;

    __norflash_sfc_power_check();
    ret = sfc_spi_read(addr, buf, len);
    if (ret != len) {
        ret = -1;
    } else {
        ret = 0;
    }

    return ret;
}

int __norflash_write_page(u32 addr, u8 *buf, u32 len)
{
    sfc1_spi_enter_spi_mode();
    //支持跨flash写
    u32 exflashx_addr = addr;
    if (addr < _norflash.flash_capacity[0]) {
        _norflash_cs_sel(0);//spi cs sel
    } else {
        exflashx_addr = addr - _norflash.flash_capacity[0];
        _norflash_cs_sel(1);//spi cs sel
    }
    _norflash_send_write_enable();
    spi_cs_l();
    spi_write_byte(WINBOND_PAGE_PROGRAM);
    _norflash_send_addr(exflashx_addr);
    spi_dma_write(buf, len);
    spi_cs_h();
    sfc_drop_cache((void *)sfc1_flash_addr2cpu_addr(addr), len);
    int reg = _norflash_wait_ok();
    sfc1_spi_exit_spi_mode();
    if (reg) {
        return -1;
    }
    return 0;//ok
}
static int _norflash_write_pages(u32 addr, u8 *buf, u32 len)
{
    /* y_printf("flash write addr = %d, num = %d\n", addr, len); */
    int ret = 0;
    ret = sfc_spi_write_pages(addr, buf, len);
    if (ret != len) {
        ret = -1;
    } else {
        ret = 0;
    }

    return ret;
}

#if FLASH_CACHE_ENABLE
static void _norflash_cache_sync_timer(void *priv)
{
    int reg;
    os_mutex_pend(&_norflash.mutex, 0);
    if (flash_cache_is_dirty) {
        flash_cache_is_dirty = 0;
        reg = _norflash_eraser(IOCTL_ERASE_SECTOR, flash_cache_addr);
        if (reg) {
            goto __exit;
        }
        _norflash_write_pages(flash_cache_addr, flash_cache_buf, 4096);
    }
    if (flash_cache_timer) {
        sys_timeout_del(flash_cache_timer);
        flash_cache_timer = 0;
    }
__exit:
    os_mutex_post(&_norflash.mutex);
}
#endif

static int _norflash_write(u32 addr, void *buf, u32 len, u8 cache)
{
    int reg = 0;
    os_mutex_pend(&_norflash.mutex, 0);
    __norflash_sfc_power_check();

    u8 *w_buf = (u8 *)buf;
    u32 w_len = len;
    /* y_printf("flash write addr = %d, num = %d\n", addr, len); */
#if FLASH_CACHE_ENABLE
    if (!cache) {
        reg = _norflash_write_pages(addr, w_buf, w_len);
        goto __exit;
    }
    u32 align_addr = addr / 4096 * 4096;
    u32 align_len = 4096 - (addr - align_addr);
    align_len = w_len > align_len ? align_len : w_len;
    if (align_addr != flash_cache_addr) {
        if (flash_cache_is_dirty) {
            flash_cache_is_dirty = 0;
            reg = _norflash_eraser(IOCTL_ERASE_SECTOR, flash_cache_addr);
            if (reg) {
                line_inf;
                goto __exit;
            }
            reg = _norflash_write_pages(flash_cache_addr, flash_cache_buf, 4096);
            if (reg) {
                line_inf;
                goto __exit;
            }
        }
        _norflash_read(align_addr, flash_cache_buf, 4096, 0);
        flash_cache_addr = align_addr;
    }
    memcpy(flash_cache_buf + (addr - align_addr), w_buf, align_len);
    if ((addr + align_len) % 4096) {
        flash_cache_is_dirty = 1;
        if (flash_cache_timer) {
            sys_timer_re_run(flash_cache_timer);
        } else {
            flash_cache_timer = sys_timeout_add(0, _norflash_cache_sync_timer, FLASH_CACHE_SYNC_T_INTERVAL);
        }
    } else {
        flash_cache_is_dirty = 0;
        reg = _norflash_eraser(IOCTL_ERASE_SECTOR, align_addr);
        if (reg) {
            line_inf;
            goto __exit;
        }
        reg = _norflash_write_pages(align_addr, flash_cache_buf, 4096);
        if (reg) {
            line_inf;
            goto __exit;
        }
    }
    addr += align_len;
    w_buf += align_len;
    w_len -= align_len;
    while (w_len) {
        u32 cnt = w_len > 4096 ? 4096 : w_len;
        _norflash_read(addr, flash_cache_buf, 4096, 0);
        flash_cache_addr = addr;
        memcpy(flash_cache_buf, w_buf, cnt);
        if ((addr + cnt) % 4096) {
            flash_cache_is_dirty = 1;
            if (flash_cache_timer) {
                sys_timer_re_run(flash_cache_timer);
            } else {
                flash_cache_timer = sys_timeout_add(0, _norflash_cache_sync_timer, FLASH_CACHE_SYNC_T_INTERVAL);
            }
        } else {
            flash_cache_is_dirty = 0;
            reg = _norflash_eraser(IOCTL_ERASE_SECTOR, addr);
            if (reg) {
                line_inf;
                goto __exit;
            }
            reg = _norflash_write_pages(addr, flash_cache_buf, 4096);
            if (reg) {
                line_inf;
                goto __exit;
            }
        }
        addr += cnt;
        w_buf += cnt;
        w_len -= cnt;
    }
#else
    reg = _norflash_write_pages(addr, w_buf, w_len);
#endif
__exit:
    os_mutex_post(&_norflash.mutex);
    return reg;
}

static int _norflash_eraser(u32 eraser, u32 addr)
{
    u8 eraser_cmd;
    u32 len;
    switch (eraser) {
    case FLASH_PAGE_ERASER:
    case IOCTL_ERASE_PAGE:
    case 256:
        eraser_cmd = WINBOND_PAGE_ERASE;
        addr = addr / 256 * 256;
        len = 256;
        break;
    case FLASH_SECTOR_ERASER:
    case IOCTL_ERASE_SECTOR:
    case 4096:
        eraser_cmd = WINBOND_SECTOR_ERASE;
        //r_printf(">>>[test]:addr = %d\n", addr);
        addr = addr / 4096 * 4096;
        len = 4096;
        break;
    case FLASH_BLOCK_ERASER:
    case IOCTL_ERASE_BLOCK:
    case 65536:
        eraser_cmd = WINBOND_BLOCK_ERASE;
        addr = addr / 65536 * 65536;
        len = 65536;
        break;
    case FLASH_CHIP_ERASER:
    case IOCTL_ERASE_CHIP:
        eraser_cmd = WINBOND_CHIP_ERASE;
        if (_norflash.flash_cs_sel == 0) { //不处理跨flash擦,仅cache缓存不准
            len = _norflash.flash_capacity[0];
        } else {
            len = _norflash.flash_capacity[1];
        }
        break;
    default:
        return -1;
    }

    __norflash_sfc_power_check();
    sfc1_spi_enter_spi_mode();
    u32 exflashx_addr = addr;
    if (addr < _norflash.flash_capacity[0]) {
        _norflash_cs_sel(0);//spi cs sel
    } else {
        exflashx_addr = addr - _norflash.flash_capacity[0];
        _norflash_cs_sel(1);//spi cs sel
    }

    _norflash_send_write_enable();
    spi_cs_l();
    spi_write_byte(eraser_cmd);
    if (eraser_cmd != WINBOND_CHIP_ERASE) {
        _norflash_send_addr(exflashx_addr);
    }
    spi_cs_h();
    sfc_drop_cache((void *)sfc1_flash_addr2cpu_addr(addr), len);
    int ret = _norflash_wait_ok();
    sfc1_spi_exit_spi_mode();
    return ret;
}

static int _norflash_ioctl(u32 cmd, u32 arg, u32 unit, void *_part)
{
    int reg = 0;
    struct norflash_partition *part = _part;
    os_mutex_pend(&_norflash.mutex, 0);
    __norflash_sfc_power_check();
    switch (cmd) {
    case IOCTL_GET_STATUS:
        *(u32 *)arg = 1;
        break;
    case IOCTL_GET_ID://默认只输出flash0
        *((u32 *)arg) = _norflash.flash_id[0];//只获取0flash
        break;
    case IOCTL_GET_CAPACITY://默认只输出flash0
        if (_norflash.flash_capacity[0] == 0)  {
            *(u32 *)arg = 0;
        } else if (_norflash.part_num == 1 && part->start_addr == 0) {
            *(u32 *)arg = _norflash.flash_capacity[0] / unit;
        } else {
            *(u32 *)arg = part->size / unit;
        }
        break;
    case IOCTL_GET_BLOCK_SIZE:
        *(u32 *)arg = 512;
        break;
    case IOCTL_ERASE_PAGE:
        reg = _norflash_eraser(IOCTL_ERASE_PAGE, arg * unit + part->start_addr);
        break;
    case IOCTL_ERASE_SECTOR:
        reg = _norflash_eraser(IOCTL_ERASE_SECTOR, arg * unit + part->start_addr);
        break;
    case IOCTL_ERASE_BLOCK:
        reg = _norflash_eraser(IOCTL_ERASE_BLOCK, arg * unit + part->start_addr);
        break;
    case IOCTL_ERASE_CHIP:
        reg = _norflash_eraser(IOCTL_ERASE_CHIP, 0);
        break;
    case IOCTL_FLUSH:
        break;
    case IOCTL_CMD_RESUME:
        norflash_sfc_powerdown_release(FLASH_EXIT_LOWPOWER_TIME);
        break;
    case IOCTL_CMD_SUSPEND:
        norflash_sfc_enter_powerdown();
        break;
    case IOCTL_POWER_RESUME:
        norflash_sfc_powerdown_release(arg ? arg : FLASH_EXIT_LOWPOWER_TIME);
        break;
    case IOCTL_POWER_SUSPEND:
        norflash_sfc_enter_powerdown();
        break;
    case IOCTL_GET_PART_INFO:
        u32 *info = (u32 *)arg;
        u32 *start_addr = &info[0];
        u32 *part_size = &info[1];
        *start_addr = part->start_addr;
        *part_size = part->size;
        break;
    default:
        reg = -EINVAL;
        break;
    }
//__exit:
    os_mutex_post(&_norflash.mutex);
    return reg;
}


/*************************************************************************************
 *                                  挂钩 device_api
 ************************************************************************************/

static int norflash_sfc_dev_init(const struct dev_node *node, void *arg)
{
    struct norflash_sfc_dev_platform_data *pdata = arg;

    return _norflash_init(node->name, pdata);
}

static int norflash_sfc_dev_open(const char *name, struct device **device, void *arg)
{
    struct norflash_partition *part;
    part = norflash_find_part(name);
    if (!part) {
        log_error("no norflash partition is found\n");
        return -ENODEV;
    }
    *device = &part->device;
    (*device)->private_data = part;
    if (atomic_read(&part->device.ref)) {
        return 0;
    }
    return _norflash_open(arg);
}
static int norflash_sfc_dev_close(struct device *device)
{
    return _norflash_close();
}
static int norflash_sfc_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    int reg;
    /* printf("flash read sector = %d, num = %d\n", offset, len); */
    offset = offset * 512;
    len = len * 512;
    struct norflash_partition *part;
    part = (struct norflash_partition *)device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = _norflash_read(offset, buf, len, 1);
    if (reg) {
        r_printf(">>>[r error]:\n");
        len = 0;
    }

    len = len / 512;
    return len;
}
static int norflash_sfc_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    /* printf("flash write sector = %d, num = %d\n", offset, len); */
    int reg = 0;
    offset = offset * 512;
    len = len * 512;
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = _norflash_write(offset, buf, len, 1);
    if (reg) {
        r_printf(">>>[w error]:\n");
        len = 0;
    }
    len = len / 512;
    return len;
}
static bool norflash_sfc_dev_online(const struct dev_node *node)
{
    return 1;
}
static int norflash_sfc_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    return _norflash_ioctl(cmd, arg, 512, part);
}

/*
 * 1. 外部调用时以512字节为单位的地址和长度，且需要驱动write自己处理擦除，
 * 请使用norflash_sfc_dev_ops接口，否则使用本文件内的其他ops
 *
 * 2. 如果不需要驱动自己处理擦除，可以把宏FLASH_CACHE_ENABLE清零，或者把
 * norflash_sfc_dev_read()里面调用的_norflash_read()的实参cache填0，
 * norflash_sfc_dev_write()同理
 *
 * 3. norflash_sfc_dev_ops可以被多个设备名注册，每个设备名被认为是不同分区，所以
 * 需要填不同的分区起始地址和大小，若分区地址有重叠或者最大分区结束地址大于
 * flash容量，会触发ASSERT()
 *
 * 4. 关于IOCTL_GET_CAPACITY，有多个分区注册时返回分区的大小，如果只注册了1
 * 个分区，分区起始地址 == 0时返回flash容量，起始地址 != 0时返回分区大小，
 * norflash_sfc_dev_ops返回的长度以512字节为单位
 *
 * 5. 本文件内的各个ops可以同时使用
 */
//按512字节单位读写
const struct device_operations norflash_sfc_dev_ops = {
    .init   = norflash_sfc_dev_init,
    .online = norflash_sfc_dev_online,
    .open   = norflash_sfc_dev_open,
    .read   = norflash_sfc_dev_read,
    .write  = norflash_sfc_dev_write,
    .ioctl  = norflash_sfc_dev_ioctl,
    .close  = norflash_sfc_dev_close,
};


static int norflash_sfc_fs_dev_init(const struct dev_node *node, void *arg)
{
    struct norflash_sfc_dev_platform_data *pdata = arg;

    return _norflash_init(node->name, pdata);
}

static int norflash_sfc_fs_dev_open(const char *name, struct device **device, void *arg)
{
    struct norflash_partition *part;
    part = norflash_find_part(name);
    if (!part) {
        log_error("no norflash partition is found\n");
        return -ENODEV;
    }
    *device = &part->device;
    (*device)->private_data = part;
    if (atomic_read(&part->device.ref)) {
        return 0;
    }
    return _norflash_open(arg);
}
static int norflash_sfc_fs_dev_close(struct device *device)
{
    return _norflash_close();
}
static int norflash_sfc_fs_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    int reg;
    /* printf("flash read sector = %d, num = %d\n", offset, len); */
    struct norflash_partition *part;
    part = (struct norflash_partition *)device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = _norflash_read(offset, buf, len, 0);
    if (reg) {
        r_printf(">>>[r error]:\n");
        len = 0;
    }

    return len;
}

static int norflash_sfc_fs_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    //printf("flash write addr = 0x%x, len = 0x%x\n", offset, len);
    int reg = 0;
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = _norflash_write(offset, buf, len, 0);
    if (reg) {
        r_printf(">>>[w error]:\n");
        len = 0;
    }
    return len;
}
static bool norflash_sfc_fs_dev_online(const struct dev_node *node)
{
    return 1;
}

static int norflash_sfc_fs_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    return _norflash_ioctl(cmd, arg, 1, part);
}

/*
 * 1. 外部调用时以1字节为单位的地址和长度，且驱动write自己不处理擦除，
 * 请使用norflash_sfc_fs_dev_ops接口，否则使用本文件内的其他ops。注意：有些文件系
 * 统需要满足这个条件的驱动，如果期望修改成驱动内部处理擦除，需要测试所
 * 有关联的文件系统是否支持，或者新建一个符合需求的ops
 *
 * 2. 如果需要驱动自己处理擦除，需要把宏FLASH_CACHE_ENABLE置1，且
 * norflash_sfc_fs_dev_read()里面调用的_norflash_read()的实参cache填1，
 * norflash_sfc_fs_dev_write()同理
 *
 * 3. norflash_sfc_fs_dev_ops可以被多个设备名注册，每个设备名被认为是不同分区，所以
 * 需要填不同的分区起始地址和大小，若分区地址有重叠或者最大分区结束地址大于
 * flash容量，会触发ASSERT()
 *
 * 4. 关于IOCTL_GET_CAPACITY，有多个分区注册时返回分区的大小，如果只注册了1
 * 个分区，分区起始地址 == 0时返回flash容量，起始地址 != 0时返回分区大小
 * norflash_sfc_fs_dev_ops返回的长度以1字节为单位
 *
 * 5. 本文件内的各个ops可以同时使用
 */
//按1字节单位读写
const struct device_operations norflash_sfc_fs_dev_ops = {
    .init   = norflash_sfc_fs_dev_init,
    .online = norflash_sfc_fs_dev_online,
    .open   = norflash_sfc_fs_dev_open,
    .read   = norflash_sfc_fs_dev_read,
    .write  = norflash_sfc_fs_dev_write,
    .ioctl  = norflash_sfc_fs_dev_ioctl,
    .close  = norflash_sfc_fs_dev_close,
};

/*
 * 对ops的读写单位有另外需求，或者驱动内部是否支持擦除，可以参照上面的ops，
 * 不同条件自由组合，建立新的ops
 */

/*************************************************************************************
 *                                 低功耗管理
 ************************************************************************************/

#define EX_FLASH_POWER_WORK     (0)
#define EX_FLASH_POWER_CLOSE    (1)
#define EX_FLASH_POWER_CLOSING  (2)
#define EX_FLASH_POWER_OPENING  (3)

#define USER_MASK_TYPE       (BIT(16)|BIT(17))//删消息池使用
static u16 last_id = 1;//删消息池使用


static volatile u8 extern_flash_status = EX_FLASH_POWER_WORK;


extern void udelay(u32 us);

static void norflash_sfc_poweroff(int priv)
{
#if (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT)
    os_mutex_pend(&_norflash.mutex, 0);
    if (extern_flash_status != EX_FLASH_POWER_CLOSING) {
        os_mutex_post(&_norflash.mutex);
        return;
    }

#if FLASH_CACHE_ENABLE
    // 断电前同步脏缓存
    if (flash_cache_is_dirty) {
        flash_cache_is_dirty = 0;
        _norflash_eraser(IOCTL_ERASE_SECTOR, flash_cache_addr);
        _norflash_write_pages(flash_cache_addr, flash_cache_buf, 4096);
    }
    if (flash_cache_timer) {
        sys_timeout_del(flash_cache_timer);
        flash_cache_timer = 0;
    }
#endif

    _norflash_close();
    udelay(FLASH_POWEROFF_TIME);
    sfc_flash_power_set(0);
    extern_flash_status = EX_FLASH_POWER_CLOSE;
    os_mutex_post(&_norflash.mutex);
#else
    os_mutex_pend(&_norflash.mutex, 0);
    if (extern_flash_status != EX_FLASH_POWER_CLOSING) {
        os_mutex_post(&_norflash.mutex);
        return;
    }

    // 发送 Power Down 命令（双 flash 都发送）
    _norflash_cs_sel(0);
    norflash_sfc_enter_powerdown();
    if (_norflash.sfc_spi_data_ptr->sfc_cs1_en) {
        _norflash_cs_sel(1);
        norflash_sfc_enter_powerdown();
        _norflash_cs_sel(0);  // 恢复默认 cs
    }

    extern_flash_status = EX_FLASH_POWER_CLOSE;
    os_mutex_post(&_norflash.mutex);
#endif
}





static void __norflash_sfc_power_check(void)
{
#if (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT)

    if (!extern_flash_status) {
        return;
    }

    local_irq_disable();
    if (extern_flash_status == EX_FLASH_POWER_CLOSING) {
        os_taskq_del_type("app_core", Q_CALLBACK | last_id | USER_MASK_TYPE);
        extern_flash_status = EX_FLASH_POWER_WORK;
    } else if (extern_flash_status == EX_FLASH_POWER_CLOSE) {
        // 硬件 IO 上电 + 重新初始化
        local_irq_enable();
        sfc_flash_power_set(1);
        udelay(FLASH_POWERON_TIME);
        _norflash_open(NULL);
        local_irq_disable();
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
        norflash_sfc_powerdown_release(FLASH_EXIT_LOWPOWER_TIME);
        local_irq_disable();
        extern_flash_status = EX_FLASH_POWER_WORK;
    }
    local_irq_enable();

#endif
}



void ex_norflash_sfc_poweroff(void)
{
    os_mutex_pend(&_norflash.mutex, 0);

    if (extern_flash_status == EX_FLASH_POWER_CLOSE) {
        os_mutex_post(&_norflash.mutex);
        return;
    }

    extern_flash_status = EX_FLASH_POWER_CLOSING;
    norflash_sfc_poweroff(0);

    os_mutex_post(&_norflash.mutex);
}



void ex_norflash_sfc_poweron(void)
{
    os_mutex_pend(&_norflash.mutex, 0);
    __norflash_sfc_power_check();
    os_mutex_post(&_norflash.mutex);
}



static u8 extern_flash_suspend(u32 timeout)
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
    msg[0] = (int)norflash_sfc_poweroff;
    msg[1] = 1;
    msg[2] = (int)NULL;
    last_id++;
    if (!last_id) {
        last_id = 1;
    }
    os_taskq_post_type("app_core", Q_CALLBACK | last_id | USER_MASK_TYPE, 3, msg);

    return 1;
}

static u8 extern_flash_resume(u32 timeout)
{
    // 出低功耗且操作flash时再恢复供电
    return 0;
}

//低功耗线程请求所有模块关闭，由对应线程处理
REGISTER_LP_REQUEST(power_sfc_flash_target) = {
    .name = "extern_flash_sfc",
    .request_enter = extern_flash_suspend,
    .request_exit = extern_flash_resume,
};

#endif /* #if (defined(TCFG_NORFLASH_SFC_DEV_ENABLE) && TCFG_NORFLASH_SFC_DEV_ENABLE) */


