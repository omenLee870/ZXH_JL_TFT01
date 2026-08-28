#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rdeckey_config.data.bss")
#pragma data_seg(".rdeckey_config.data")
#pragma const_seg(".rdeckey_config.text.const")
#pragma code_seg(".rdeckey_config.text")
#endif
#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "rdec.h"
#include "gpio.h"
#include "key_driver.h"
#include "rdec_key.h"

#if TCFG_QDEC_ENABLE


struct rdeckey_info {
    u8  len;
    u16 key_uuid;
    u16 ioA_uuid;
    u16 ioB_uuid;
} __attribute__((packed));


static struct rdec_device rdeckey_ports[RDEC_HW_MAX_NUM];
static struct rdec_platform_data platform_data;

static const u16 key_uuid_table[][2] = {
    { 0xC849, KEY_QDEC_NUM0 },
    { 0xC84A, KEY_QDEC_NUM1 },
    { 0xC84B, KEY_QDEC_NUM2 },
    { 0xC84C, KEY_QDEC_NUM3 },
    { 0xC84D, KEY_QDEC_NUM4 },
    { 0xC84E, KEY_QDEC_NUM5 },
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

const struct rdec_platform_data  *get_rdeckey_platform_data()
{
    struct rdeckey_info info[RDEC_HW_MAX_NUM];

    if (platform_data.enable) {
        return &platform_data;
    }

#ifdef CONFIG_FPGA_ENABLE
    printf("[%s] line:%d fpag warn: FIXME", __func__, __LINE__);
    platform_data.num       = 1;
    platform_data.rdec      = rdeckey_ports;
    platform_data.enable    = 1;

    /*设置io配置*/
    rdeckey_ports[0].index = 0;
    rdeckey_ports[0].sin_portA = TCFG_RDEC0_ECODEA_PORT;
    rdeckey_ports[0].sin_portA_io_mode = PORT_INPUT_PULLUP_10K;
    rdeckey_ports[0].sin_portB = TCFG_RDEC0_ECODEB_PORT;
    rdeckey_ports[0].sin_portB_io_mode = PORT_INPUT_PULLUP_10K;
    rdeckey_ports[0].scan_time = 100;
    rdeckey_ports[0].phase_mode = RDEC_PHASE_1;

#else
    int len = syscfg_read(CFG_RDEC_ID, info, sizeof(info));
    if (len <= 0) {
        puts("ERR:Can not read the rdeckey config,total rdeckey should <= RDEC_HW_MAX_NUM \n");
        return NULL;
    }
    printf("rdeckey_len: %d, %x, %x, %x\n", len, info[0].key_uuid, info[0].ioA_uuid, info[0].ioB_uuid);

    platform_data.num       = len / sizeof(struct rdeckey_info);
    platform_data.rdec      = rdeckey_ports;
    platform_data.enable    = 1;

    printf("rdeckey numbers: %d\n", platform_data.num);

    for (int i = 0; i < platform_data.num; i++) {
        rdeckey_ports[i].index = i;
        rdeckey_ports[i].sin_portA = uuid2gpio(info[i].ioA_uuid);
        rdeckey_ports[i].sin_portA_io_mode = PORT_INPUT_PULLUP_10K;
        rdeckey_ports[i].sin_portB = uuid2gpio(info[i].ioB_uuid);
        rdeckey_ports[i].sin_portB_io_mode = PORT_INPUT_PULLUP_10K;
        rdeckey_ports[i].scan_time = 100;
        rdeckey_ports[i].phase_mode = RDEC_PHASE_1;

        printf("rdeckey:%d,protA:%d,protB:%d\n", i, rdeckey_ports[i].sin_portA, rdeckey_ports[i].sin_portB);
    }
#endif

    return &platform_data;
}

#endif
