#ifndef  __TZFLASH_API_H__
#define  __TZFLASH_API_H__

#include "device.h"

struct flash_rw {
    u32 addr;
    u32 len;
    void *buffer;
};

struct flash_wps_area_cfg {
    u32 start_addr;//任意地址,非对齐时不包含地址所在块
    u32 end_addr;//任意地址,非对齐时不包含地址所在块
    u8 lock_en;//1:保护, 0:解保护
} __attribute__((packed)); //size(1byte)

struct flash_info_t {
    u32 capacity;
    u32 flash_id;
    u8 uuid[16];
    u8 data_width;
    u8 read_mode;
    u8 async_mode: 1;
    u8 recover_continue_mode: 1;
    u8 res: 6;

#ifndef ROM_SECURE_BOOT
    //loader add:
    u8 is4byte_mode;
    /* u8 flash_idx; */
    u8 spi_idx;
    u8 cs_io;
    // struct device norflash_dev_t;//load
#endif
};




struct tzspi_target {
    void (*suspend)(u32 id);
    void (*resume)(u32 id);
};

#define REGISTER_TZSPI_TARGET(target) \
        const struct tzspi_target target SEC_USED(.tzspi_target)

extern const struct tzspi_target tzspi_target_begin[];
extern const struct tzspi_target tzspi_target_end[];

#define list_for_each_tzspi_target(p) \
    for (p = tzspi_target_begin; p < tzspi_target_end; p++)

void tzflash_init();
// #ifdef ROM_SECURE_BOOT
extern void *tzflash_flash2cpu_addr(u32 phy_addr);
#define tzflash_open(x,y,z)       tzflash_init()
// #endif
extern int tzflash_ioctl(void *device, u32 cmd, u32 arg);//return:0:ok; <0:error
extern int tzflash_write(void *device, const void *buffer, u32 len, u32 addr);
extern int tzflash_read(void *device, void *buffer, u32 len, u32 addr);

u32 tzflash_get_flash_capacity();
u8 *tzflash_get_flash_uuid();
u8 *get_norflash_uuid(void);
u8 *tzflash_get_uuid(void);
u32 tzflash_get_flash_id();

void tzflash_dump();

u32 syscfg_read_otp(u32 id, u8 *buf, u32 len);
// u8 *tzflash_get_uuid(void);


//写保护区间缓存:
#define FLASH_WPS_AREA_GROUP_SIZE  1
// 获取flash写保护模式
//return <0:error;  0:no; 1:wps,2:wp
int tzflash_get_write_protect_mode();
//开关写保护功能(wps/wp)
//lock:1:开写保护, 0:关保护
//saddr:wps模式有效
//return <0:error,实际地址:ok
int tzflash_set_write_protect(u8 lock, u32 saddr, u32 eaddr);
//设置wps保护区域
//return <0:error,0:ok
int tzflash_cfg_wps_area(struct flash_wps_area_cfg *wps_area, u32 area_group_size);
//获取wps状态
enum tzflash_wps_status_mode tzflash_get_wps_status();
void tzflash_change_mode();//dtr(clk),continue

void votp_cfg_addr_patch(void);

//获取认证码(三元组、序列号)信息
void *rsv_auth_analysis(void);

#endif

