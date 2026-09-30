#include "includes.h"
#include "app_config.h"
#include "norflash_sfc.h"
#include "tzflash_api.h"

#undef LOG_TAG_CONST
#define LOG_TAG     "[FLASH_INDIDE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

/* extern void sfc_protect_lock(u8 index); */
/* norflash 分区访问互斥锁；tzflash 内部对写操作另有互斥保护 */
static OS_MUTEX nor_mutex;

static int norflash_mutex_init(void)
{
    os_mutex_create(&nor_mutex);
    return 0;
}
late_initcall(norflash_mutex_init);

static void norflash_mutex_enter(void)
{
    os_mutex_pend(&nor_mutex, 0);
}

static void norflash_mutex_exit(void)
{
    os_mutex_post(&nor_mutex);
}
extern void ble_busy_wait_idle(u8 type);
static struct list_head head = LIST_HEAD_INIT(head);

struct norflash_partition {
    struct list_head entry;
    const char *name;
    u32 start_addr;
    u32 size;
    u8  igore_overlaps;     //忽略分区检查
    struct device device;
    u32 flash_cache_addr;       /* 缓存对应的 flash 绝对地址 */
    u8 *flash_cache_buf;        /* 4K 缓存，内容与 flash 中保持一致 */
    u8 flash_cache_is_dirty;    /* 缓存中存在未落盘的修改 */
    u16 flash_cache_timer;      /* 脏数据延时回写定时器 */
};

static struct norflash_partition *norflash_find_part(const char *name)
{
    struct norflash_partition *p, *n;
    local_irq_disable();
    list_for_each_entry_safe(p, n, &head, entry) {
        if (!strcmp(p->name, name)) {
            local_irq_enable();
            return p;
        }
    }
    local_irq_enable();
    return NULL;
}

static struct norflash_partition *norflash_new_part(const char *name, u32 addr, u32 size, u8 igore_overlap)
{
    struct norflash_partition *part = zalloc(sizeof(struct norflash_partition));
    ASSERT(part, "norflash_partition malloc err");
    part->name = name;
    part->start_addr = addr;
    part->size = size;
    part->igore_overlaps = igore_overlap;
    local_irq_disable();
    list_add_tail(&part->entry, &head);
    local_irq_enable();
    return part;
}

static void norflash_delete_part(const char *name)
{
    struct norflash_partition *p, *n;
    local_irq_disable();
    list_for_each_entry_safe(p, n, &head, entry) {
        if (!strcmp(p->name, name)) {
            list_del(&p->entry);
            free(p);
            local_irq_enable();
            return ;
        }
    }
    local_irq_enable();
    return ;
}

static int norflash_verify_part(struct norflash_partition *p)
{

    if (p->igore_overlaps) {
        return 0;
    }
    struct norflash_partition *part, *n;
    local_irq_disable();
    list_for_each_entry_safe(part, n, &head, entry) {
        if (part->igore_overlaps) {
            continue;
        }
        if ((p->start_addr >= part->start_addr) && (p->start_addr < part->start_addr + part->size)) {
            if (strcmp(p->name, part->name) != 0) {
                local_irq_enable();
                return -1;
            }
        }
    }
    local_irq_enable();
    return 0;
}


extern u32 sdfile_flash_addr2cpu_addr(u32 offset);
extern u32 sdfile_cpu_addr2flash_addr(u32 offset);

static int _norflash_init(const char *name, struct norflash_sfc_dev_platform_data *pdata)
{
    log_info("norflash_sfc_init >>>>");
    if (pdata->path) {
        FILE *fp = fopen((const char *)pdata->path, "r");
        if (!fp) {
            log_error("fopen err :%s \n", pdata->path);
            ASSERT(0);
            return false;
        }
        struct vfs_attr attr = {0};
        fget_attrs(fp, &attr);
        u32 part_addr = attr.sclust;
        /* 文件系统里分区的实际大小，必须能覆盖平台数据配置的大小 */
        u32 part_len = attr.fsize;
        part_addr = sdfile_cpu_addr2flash_addr(part_addr);
        ASSERT(pdata->start_addr == part_addr, "%x %x\n", pdata->start_addr, part_addr);
        ASSERT(part_len >= pdata->size, "%x ,%x\n", pdata->start_addr, part_addr);
        fclose(fp);
    }


    struct norflash_partition *part;
    part = norflash_find_part(name);

    if (!part) {
        part = norflash_new_part(name, pdata->start_addr, pdata->size, pdata->igore_overlaps);
        ASSERT(part, "not enough norflash partition memory in array\n");
        ASSERT(norflash_verify_part(part) == 0, "norflash partition %s overlaps\n", name);
        log_info("norflash new partition %s\n", part->name);
    } else {
        ASSERT(0, "norflash partition name already exists\n");
    }
    return 0;
}


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
    return 0;//_norflash_open(arg);
}
static int norflash_sfc_fs_dev_close(struct device *device)
{
    return 0;//_norflash_close();
}



static int norflash_sfc_fs_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    int reg;
    struct norflash_partition *part;
    part = (struct norflash_partition *)device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    if (offset + len > part->size) {
        log_error("__func__ %s %x %x %x\n", __func__, rets, offset, len);
        ASSERT(0);
    }
    offset += part->start_addr;
    /* norflash_mutex_enter(); */
    u32 cpu_addr = sdfile_flash_addr2cpu_addr(offset);
    memcpy((u8 *)buf, (u8 *)cpu_addr, len);
    /* norflash_mutex_exit(); */
    return len;
}

static int norflash_sfc_fs_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    int reg = 0;
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    if (offset + len > part->size) {
        log_error("__func__ %s %x %x %x\n", __func__, rets, offset, len);
        ASSERT(0);
    }
    offset += part->start_addr;
    /* norflash_mutex_enter(); */
    ASSERT(!cpu_in_irq());
    ASSERT(!cpu_irq_disabled());
    reg =  tzflash_write(NULL, buf, len, offset);//写操作里面已经集合的互斥,这里仅仅检查中断
    /* norflash_mutex_exit(); */
    return reg;
}

static bool norflash_sfc_fs_dev_online(const struct dev_node *node)
{
    return 1;
}

static int norflash_sfc_fs_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    struct norflash_partition *part = device->private_data;
    struct sfc_no_enc_wr   *no_enc_wr;
    int reg = 0;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }

    switch (cmd) {
    case IOCTL_GET_PART_INFO:
        u32 *info = (u32 *)arg;
        u32 *start_addr = &info[0];
        u32 *part_size = &info[1];
        *start_addr = part->start_addr;
        *part_size = part->size;
        break;
    case IOCTL_GET_CAPACITY:
        *(u32 *)arg = part->size;
        break;
    case IOCTL_ERASE_PAGE:
        ASSERT(!cpu_in_irq());
        ASSERT(!cpu_irq_disabled());
        //写操作里面已经集合的互斥,这里仅仅检查中断
#if TCFG_FLASH_ERASE_WAIT_BLE
        //擦除前需等待ble idle
        ble_busy_wait_idle(IOCTL_ERASE_PAGE);
#endif
        return  tzflash_ioctl(NULL, cmd, arg + part->start_addr);
        break;
    case IOCTL_ERASE_SECTOR:
        if ((arg * 1) + 4096 > part->size) {
            log_error("__func__ %s %x %x %x\n", __func__, rets, arg, 1);
            ASSERT(0);
        }

        ASSERT(!cpu_in_irq());
        ASSERT(!cpu_irq_disabled());
#if TCFG_FLASH_ERASE_WAIT_BLE
        //擦除前需等待ble idle
        ble_busy_wait_idle(IOCTL_ERASE_SECTOR);
#endif
        //写操作里面已经集合的互斥,这里仅仅检查中断
        return  tzflash_ioctl(NULL, cmd, arg + part->start_addr);
        break;
    case IOCTL_ERASE_BLOCK:
        if ((arg * 1) + (64 * 1024) > part->size) {
            log_error("__func__ %s %x %x %x\n", __func__, rets, arg, 1);
            ASSERT(0);
        }

        ASSERT(!cpu_in_irq());
        ASSERT(!cpu_irq_disabled());
#if TCFG_FLASH_ERASE_WAIT_BLE
        //擦除前需等待ble idle
        ble_busy_wait_idle(IOCTL_ERASE_BLOCK);
#endif
        //写操作里面已经集合的互斥,这里仅仅检查中断
        return  tzflash_ioctl(NULL, cmd, arg + part->start_addr);
        break;
#if 0
    case IOCTL_SFC_NORFLASH_READ_NO_ENC:
        no_enc_wr = (struct sfc_no_enc_wr *)arg;
        norflash_mutex_enter();
        u32 cpu_addr = sdfile_flash_addr2cpu_addr(no_enc_wr->addr);
        memcpy((u8 *)no_enc_wr->buf, (u8 *)cpu_addr, no_enc_wr->len);
        norflash_mutex_exit();
        reg = 0;
        break;
    case IOCTL_SFC_NORFLASH_WRITE_NO_ENC:
        no_enc_wr = (struct sfc_no_enc_wr *)arg;
        /* norflash_mutex_enter(); */
        ASSERT(!cpu_in_irq());
        ASSERT(!cpu_irq_disabled());
        //写操作里面已经集合的互斥,这里仅仅检查中断
        reg =  tzflash_write(NULL, no_enc_wr->buf, no_enc_wr->len, no_enc_wr->addr);
        /* norflash_mutex_exit(); */
        break;
#endif
    case IOCTL_FLUSH:
        break;
    default:
        return  tzflash_ioctl(NULL, cmd, arg);

    }

    return  reg;
}

//按1字节单位读写
const struct device_operations inside_norflash_fs_dev_ops = {
    .init   = norflash_sfc_fs_dev_init,
    .online = norflash_sfc_fs_dev_online,
    .open   = norflash_sfc_fs_dev_open,
    .read   = norflash_sfc_fs_dev_read,
    .write  = norflash_sfc_fs_dev_write,
    .ioctl  = norflash_sfc_fs_dev_ioctl,
    .close  = norflash_sfc_fs_dev_close,
};


/* flash 4K 缓存开关；开启后读写以 512 字节为单位，写操作经缓存合并后再落盘 */
#define FLASH_CACHE_ENABLE              1
#define FLASH_CACHE_SIZE                4096
#define FLASH_CACHE_INVALID_ADDR        0xffffffff
#define FLASH_CACHE_SYNC_T_INTERVAL     100

static int norflash_fat_fs_dev_init(const struct dev_node *node, void *arg)
{
    struct norflash_sfc_dev_platform_data *pdata = arg;

    return _norflash_init(node->name, pdata);
}

static int norflash_fat_fs_dev_open(const char *name, struct device **device, void *arg)
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
#if FLASH_CACHE_ENABLE
    /* 缓存按需分配，先置为无效地址 */
    part->flash_cache_addr = FLASH_CACHE_INVALID_ADDR;
#endif

    return 0;
}

static int norflash_fat_fs_dev_close(struct device *device)
{
#if FLASH_CACHE_ENABLE
    norflash_mutex_enter();
    struct norflash_partition *part = device->private_data;
    if (part->flash_cache_is_dirty) {
        part->flash_cache_is_dirty = 0;
        norflash_sfc_fs_dev_ioctl(device, IOCTL_ERASE_SECTOR, part->flash_cache_addr);
        norflash_sfc_fs_dev_write(device, part->flash_cache_buf, FLASH_CACHE_SIZE, part->flash_cache_addr);
    }
    if (part->flash_cache_buf) {
        free(part->flash_cache_buf);
        part->flash_cache_buf = NULL;
    }
    part->flash_cache_addr = FLASH_CACHE_INVALID_ADDR;
    norflash_mutex_exit();
#endif

    return 0;
}

static int norflash_fat_fs_dev_read(struct device *device, void *buf, u32 len, u32 addr)
{
    int org_len = len;
    /* 本设备层按 512 字节为单位，底层按字节访问，先统一换算 */
    addr = addr * 512;
    len = len * 512;

#if FLASH_CACHE_ENABLE
    norflash_mutex_enter();

    struct norflash_partition *part = device->private_data;
    u32 r_len = FLASH_CACHE_SIZE - (addr % FLASH_CACHE_SIZE);
    if (part->flash_cache_buf && (addr >= part->flash_cache_addr)
        && (addr < (part->flash_cache_addr + FLASH_CACHE_SIZE))) {
        if (len <= r_len) {
            memcpy(buf, part->flash_cache_buf + (addr - part->flash_cache_addr), len);
            norflash_mutex_exit();
            goto __exit;
        } else {
            memcpy(buf, part->flash_cache_buf + (addr - part->flash_cache_addr), r_len);
            addr += r_len;
            buf = (u8 *)buf + r_len;
            len -= r_len;
        }
    }
    norflash_mutex_exit();
#endif
    norflash_sfc_fs_dev_read(device, buf, len, addr);
__exit:
    return org_len;
}


#if FLASH_CACHE_ENABLE

static void _norflash_cache_sync_timer(struct device *device)
{
    norflash_mutex_enter();

    struct norflash_partition *part = device->private_data;
    int reg = 0;
    if (part->flash_cache_is_dirty && part->flash_cache_buf) {
        part->flash_cache_is_dirty = 0;
        reg = norflash_sfc_fs_dev_ioctl(device, IOCTL_ERASE_SECTOR, part->flash_cache_addr);
        if (reg == -EFAULT) {
            ASSERT(0);
            goto __exit;
        }
        reg = norflash_sfc_fs_dev_write(device, part->flash_cache_buf, FLASH_CACHE_SIZE, part->flash_cache_addr);
        free(part->flash_cache_buf);
        part->flash_cache_buf = NULL;
        part->flash_cache_addr = FLASH_CACHE_INVALID_ADDR;
    }

    if (part->flash_cache_timer) {
        sys_timeout_del(part->flash_cache_timer);
        part->flash_cache_timer = 0;
    }
__exit:
    norflash_mutex_exit();
    return;
}
#endif

static int norflash_fat_fs_dev_write(struct device *device, void *buf, u32 len, u32 addr)
{
    norflash_mutex_enter();

    int org_len = len;
    /* 本设备层按 512 字节为单位，底层按字节访问，先统一换算 */
    u32 w_len = len * 512;
    addr = addr * 512;

#if FLASH_CACHE_ENABLE
    struct norflash_partition *part = device->private_data;
    u8 *w_buf = (u8 *)buf;
    int reg;

    u32 align_addr = addr / FLASH_CACHE_SIZE * FLASH_CACHE_SIZE;
    u32 align_len = FLASH_CACHE_SIZE - (addr - align_addr);
    align_len = w_len > align_len ? align_len : w_len;

    if (!part->flash_cache_buf) {
        /* 没有缓存时不应存在待回写的脏数据 */
        ASSERT(!part->flash_cache_is_dirty, "cache dirty but buf is null\n");
        part->flash_cache_buf = malloc(FLASH_CACHE_SIZE);
        ASSERT(part->flash_cache_buf, "flash cache malloc err\n");
    }

    if (align_addr != part->flash_cache_addr) {
        if (part->flash_cache_is_dirty) {
            part->flash_cache_is_dirty = 0;
            reg = norflash_sfc_fs_dev_ioctl(device, IOCTL_ERASE_SECTOR, part->flash_cache_addr);
            if (reg == -EFAULT) {
                line_inf;
                goto __exit;
            }
            reg = norflash_sfc_fs_dev_write(device, part->flash_cache_buf, FLASH_CACHE_SIZE, part->flash_cache_addr);
            if (reg != FLASH_CACHE_SIZE) {
                line_inf;
                ASSERT(0);
                goto __exit;
            }
        }
        norflash_sfc_fs_dev_read(device, part->flash_cache_buf, FLASH_CACHE_SIZE, align_addr);
        part->flash_cache_addr = align_addr;
    }

    memcpy(part->flash_cache_buf + (addr - align_addr), w_buf, align_len);

    if ((addr + align_len) % FLASH_CACHE_SIZE) {
        part->flash_cache_is_dirty = 1;
        if (part->flash_cache_timer) {
            sys_timer_re_run(part->flash_cache_timer);
        } else {
            part->flash_cache_timer = sys_timeout_add(device,
                                      (void (*)(void *))_norflash_cache_sync_timer,
                                      FLASH_CACHE_SYNC_T_INTERVAL);
        }
    } else {
        part->flash_cache_is_dirty = 0;
        reg = norflash_sfc_fs_dev_ioctl(device, IOCTL_ERASE_SECTOR, align_addr);
        if (reg == -EFAULT) {
            line_inf;
            ASSERT(0);
            goto __exit;
        }

        reg = norflash_sfc_fs_dev_write(device, part->flash_cache_buf, FLASH_CACHE_SIZE, align_addr);
        if (reg != FLASH_CACHE_SIZE) {
            line_inf;
            goto __exit;
        }
    }

    addr += align_len;
    w_buf += align_len;
    w_len -= align_len;

    while (w_len) {
        u32 cnt = w_len > FLASH_CACHE_SIZE ? FLASH_CACHE_SIZE : w_len;

        norflash_sfc_fs_dev_read(device, part->flash_cache_buf, FLASH_CACHE_SIZE, addr);
        part->flash_cache_addr = addr;
        memcpy(part->flash_cache_buf, w_buf, cnt);
        if ((addr + cnt) % FLASH_CACHE_SIZE) {
            part->flash_cache_is_dirty = 1;
            if (part->flash_cache_timer) {
                sys_timer_re_run(part->flash_cache_timer);
            } else {
                part->flash_cache_timer = sys_timeout_add(device,
                                          (void (*)(void *))_norflash_cache_sync_timer,
                                          FLASH_CACHE_SYNC_T_INTERVAL);
            }
        } else {
            part->flash_cache_is_dirty = 0;
            reg = norflash_sfc_fs_dev_ioctl(device, IOCTL_ERASE_SECTOR, addr);
            if (reg == -EFAULT) {
                line_inf;
                goto __exit;
            }
            reg = norflash_sfc_fs_dev_write(device, part->flash_cache_buf, FLASH_CACHE_SIZE, addr);
            if (reg != FLASH_CACHE_SIZE) {
                line_inf;
                goto __exit;
            }
        }
        addr += cnt;
        w_buf += cnt;
        w_len -= cnt;
    }
#else
    int ret = norflash_sfc_fs_dev_write(device, buf, w_len, addr);
    ASSERT(ret == w_len, "norflash write err: %d\n", ret);
    (void)ret;
#endif

__exit:
    norflash_mutex_exit();

    return org_len;
}

static bool norflash_fat_fs_dev_online(const struct dev_node *node)
{
    return 1;
}

static int norflash_fat_fs_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    struct norflash_partition *part = device->private_data;

    switch (cmd) {
    case IOCTL_CHECK_WRITE_PROTECT:
        *(u32 *)arg = 0;
        break;

    case IOCTL_GET_STATUS:
        *(u32 *)arg = 1;
        break;

    case IOCTL_SET_ASYNC_MODE:
        break;

    case IOCTL_ERASE_SECTOR:
        return norflash_sfc_fs_dev_ioctl(device, IOCTL_ERASE_SECTOR, arg * 512);

    case IOCTL_FLUSH:
        norflash_mutex_enter();
        if (part->flash_cache_is_dirty) {
            part->flash_cache_is_dirty = 0;
            norflash_sfc_fs_dev_ioctl(device, IOCTL_ERASE_SECTOR, part->flash_cache_addr);
            norflash_sfc_fs_dev_write(device, part->flash_cache_buf, FLASH_CACHE_SIZE, part->flash_cache_addr);
        }
        norflash_mutex_exit();
        return 0;

    case IOCTL_GET_CAPACITY:
        *(u32 *)arg = part->size / 512;
        break;

    case IOCTL_GET_BLOCK_SIZE:
        *(u32 *)arg = 512;
        break;

    case IOCTL_CMD_RESUME:
        break;

    case IOCTL_CMD_SUSPEND:
        break;

    default:
        ASSERT(0, "%d", cmd);
        break;
    }
    return 0;
}

/* 按512字节单位读写 */
const struct device_operations inside_norflash_fat_fs_dev_ops = {
    .init   = norflash_fat_fs_dev_init,
    .online = norflash_fat_fs_dev_online,
    .open   = norflash_fat_fs_dev_open,
    .read   = norflash_fat_fs_dev_read,
    .write  = norflash_fat_fs_dev_write,
    .ioctl  = norflash_fat_fs_dev_ioctl,
    .close  = norflash_fat_fs_dev_close,
};
