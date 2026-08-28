#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iokey_config.data.bss")
#pragma data_seg(".iokey_config.data")
#pragma const_seg(".iokey_config.text.const")
#pragma code_seg(".iokey_config.text")
#endif
#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "key/iokey.h"
#include "key_driver.h"


struct iokey_info {
    u8  len;
    u16 key_uuid;
    u16 io_uuid;
    u8 detect;
    u8 long_press_enable;
    u8 long_press_time;
    u8 press_wakeup_enable;
} __attribute__((packed));


static struct iokey_port iokey_ports[CONFIG_IOKEY_MAX_NUM];
static struct iokey_platform_data platform_data;

static const u16 key_uuid_table[][2] = {
    { 0x7684, KEY_IO_NUM0 },
    { 0x7685, KEY_IO_NUM1 },
    { 0x7686, KEY_IO_NUM2 },
    { 0x7687, KEY_IO_NUM3 },
    { 0x7688, KEY_IO_NUM4 },
    { 0x7689, KEY_IO_NUM5 },
    { 0x768A, KEY_IO_NUM6 },
    { 0x768B, KEY_IO_NUM7 },
    { 0x768C, KEY_IO_NUM8 },
    { 0x768D, KEY_IO_NUM9 },
    { 0x4755, KEY_IO_NUM10 },
    { 0x4756, KEY_IO_NUM11 },
    { 0x4757, KEY_IO_NUM12 },
    { 0x4758, KEY_IO_NUM13 },
    { 0x4759, KEY_IO_NUM14 },
};

static u8 uuid2keyValue(u16 uuid)
{
    for (int i = 0; i < ARRAY_SIZE(key_uuid_table); i++) {
        if (key_uuid_table[i][0] == uuid) {
            return key_uuid_table[i][1];
        }
    }

    return 0xff;
}

const struct iokey_platform_data *get_iokey_platform_data()
{
    struct iokey_info info[CONFIG_IOKEY_MAX_NUM];

    if (platform_data.enable) {
        return &platform_data;
    }

#ifdef CONFIG_FPGA_ENABLE
    printf("[%s] line:%d fpag warn: FIXME", __func__, __LINE__);
    platform_data.num       = 1;
    platform_data.port      = iokey_ports;
    platform_data.enable    = 1;

    /*设置io配置*/
    iokey_ports[0].connect_way = ONE_PORT_TO_LOW;
    iokey_ports[0].key_type.one_io.port = IO_PORTE_04;
    iokey_ports[0].key_value = KEY_IO_NUM0;

    /*设置io的长按复位功能*/
    platform_data.long_press_enable = 0;
    /* platform_data.long_press_time = info[i].long_press_time; */
    /* platform_data.long_press_port = uuid2gpio(info[i].io_uuid); */
    /* platform_data.long_press_level = (info[i].detect == 1) ? 0 : 1; */

#else
    int len = syscfg_read(CFG_IOKEY_ID, info, sizeof(info));
    if (len <= 0) {
        puts("ERR:Can not read the iokey config,total iokeys should <= CONFIG_IOKEY_MAX_NUM\n");
        return NULL;
    }
    printf("iokey_len: %d, %x, %x\n", len, info[0].key_uuid, info[0].io_uuid);

    platform_data.num       = len / sizeof(struct iokey_info);
    platform_data.port      = iokey_ports;
    platform_data.enable    = 1;

    printf("iokey numbers: %d\n", platform_data.num);

    for (int i = 0; i < platform_data.num; i++) {
        if (info[i].detect == 1) {
            iokey_ports[i].connect_way = ONE_PORT_TO_LOW;
        } else {
            iokey_ports[i].connect_way = ONE_PORT_TO_HIGH;
        }
        iokey_ports[i].key_type.one_io.port = uuid2gpio(info[i].io_uuid);
        iokey_ports[i].key_value = uuid2keyValue(info[i].key_uuid);
        if (info[i].long_press_enable) {
            platform_data.long_press_enable = 1;
            platform_data.long_press_time = info[i].long_press_time;
            platform_data.long_press_port = uuid2gpio(info[i].io_uuid);
            platform_data.long_press_level = (info[i].detect == 1) ? 0 : 1;
        }
        platform_data.press_wakeup_enable[i] = info[i].press_wakeup_enable;
        register_key_for_hold(iokey_ports[i].key_value);
        printf("iokey:%d,prot:%d,value:%d,c_way:%d long_press_en:%d time:%ds\n", i, iokey_ports[i].key_type.one_io.port, iokey_ports[i].key_value, iokey_ports[i].connect_way, info[i].long_press_enable, info[i].long_press_time);
    }
#endif

    return &platform_data;
}

bool is_iokey_press_down()
{
#if TCFG_IOKEY_ENABLE
    int value = 0;
    u8  power_key_num = 0; // 默认设置第0个按键为唤醒键，其他请自行更换
    if (platform_data.enable == 0) {
        return false;
    }

    value = gpio_read(iokey_ports[power_key_num].key_type.one_io.port);
    if (iokey_ports[power_key_num].connect_way == ONE_PORT_TO_LOW) {
        return value == 0 ? true : false;
    }
    return value == 1 ? true : false;
#endif
    return false;
}

int get_iokey_power_io()
{
#if TCFG_IOKEY_ENABLE
    if (platform_data.enable) {
        return iokey_ports[0].key_type.one_io.port;
    }
#endif
    return -1;
}

