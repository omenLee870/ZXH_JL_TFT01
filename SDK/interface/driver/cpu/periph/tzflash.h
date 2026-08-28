#ifndef  __TZFLASH_H__
#define  __TZFLASH_H__


#include "asm/tzsfc.h"
////////////////////////////////////////////////////////////////////////////////
#define NORFLASH_WRITE_ENABLE		                        0x06
#define NORFLASH_WRITE_SR_ENABLE		                    0x50
#define NORFLASH_READ_SR1			                        0x05
#define NORFLASH_READ_SR2			                        0x35
#define NORFLASH_READ_SR3			                        0x15
#define NORFLASH_WRITE_SR1			                        0x01
#define NORFLASH_WRITE_SR2			                        0x31
#define NORFLASH_WRITE_SR3			                        0x11
#define NORFLASH_READ_DATA		                            0x03
#define NORFLASH_READ_FAST		                            0x0B
#define NORFLASH_PAGE_PROGRAM	                            0x02
#define NORFLASH_PAGE_ERASE                                 0x81
#define NORFLASH_SECTOR_ERASE		                        0x20
#define NORFLASH_BLOCK_ERASE		                        0xD8
#define NORFLASH_CHIP_ERASE		                            0xC7
#define NORFLASH_RELEASE_POWRDOWN           			    0xAB
#define NORFLASH_JEDEC_ID                                   0x9F
#define NORFLASH_JEDEC_UID                                  0x4b
#define NORFLASH_ERASE_OTP                                  0x44
#define NORFLASH_WRITE_OTP                                  0x42
#define NORFLASH_READ_OTP                                   0x48

#define NORFLASH_CMD_POWER_DOWN              0xb9
#define NORFLASH_CMD_POWER_DOWN_RELEASE      0xab
#define NORFLASH_CMD_ENTER_4_ADDR         0xb7//>16M
#define NORFLASH_CMD_EXIT_4_ADDR         0xe9//>16M
//lock
#define NORFLASH_CMD_INDI_BS_LOCK       0x36//WEL+adr
#define NORFLASH_CMD_INDI_BS_UNLOCK     0x39//WEL+adr
#define NORFLASH_CMD_READ_BS_LOCK       0x3d//adr
#define NORFLASH_CMD_GLOBAL_BS_LOCK     0x7e//WEL
#define NORFLASH_CMD_GLOBAL_BS_UNLOCK   0x98//WEL
#define NORFLASH_LOCK_SECTOR_SIZE       0x00001000//写保护锁命令扇区大小(顶底扇区锁)
#define NORFLASH_LOCK_BLOCK_SIZE        0x00010000//写保护锁命令块大小

#define     NORFLASH_BLOCK_SIZEE        (64*1024)
#define     NORFLASH_SECTOR_SIZEE       (4*1024)
#define     NORFLASH_PAGE_SIZEE         (256)
////////////////////////////////////////////////////////////////////////////////
#ifdef TZFLASH_WRITE_PROTECT_VER_V1
#define TZFLASH_DTR_FUNC_EN             0
#define TZFLASH_WPS_FUNC_EN             0
#define TZFLASH_WPS_OP_FUNC_EN          0
#define TZFLASH_WP_FUNC_EN              0
#define TZFLASH_WP_V1_FUNC_EN           1//old cpu:1
#else
#define TZFLASH_DTR_FUNC_EN             1
#define TZFLASH_WPS_FUNC_EN             1
#define TZFLASH_WPS_OP_FUNC_EN          0
#define TZFLASH_WP_FUNC_EN              1
#define TZFLASH_WP_V1_FUNC_EN           0//old cpu:1
#endif
#define TZFLASH_OTP_ERASE_WRITE_EN      0//otp擦写使能


#define TZFLASH_ROM_WPS_PDOWN_EN   1

#if TZFLASH_LOADER2CACHE_MODE_EN
#define AT_SPI_CODE                 AT(.tzflash_text_cache)
#else
#define AT_SPI_CODE                 AT(.volatile_ram_code)//AT(.tzflash.text.cache.L1)
#endif
extern u32 _TZSPI_CODE_START;
extern u32 _TZSPI_CODE_END;
// #define AT_VOLATILE_RAM_CODE        AT(.volatile_ram_code)
#define AT_SPI_CODE_RAM     AT(.volatile_ram_code)




#define  TZFLASH_SFC_EN 1//sfc读
#define  TZFLASH_SFC_DEC_EN 0//sfc加密读

extern u32 _SFC_MEMORY_START_ADDR[];
extern u32 _SFC_MEMORY_END_ADDR[];
extern u32 _SFC_MEMORY_CACHE_SIZE[];
struct read_mode_t {
    union {
        struct {
            u32 read_mode: 16;
            u32 dummy: 4;
            u32 indly: 4;
            u32 res: 7;
            u32 dtr_en: 1;
        };
        u32 value;
    };
};

struct tzspi_config_data {
    u8 data_width: 3; //0 1 2 4
    u8 port: 1; //0 1
    u8 baud: 4; //0~15

    u8 rst: 1;// 0 1
    u8 rst_delay_ms: 4; // 0~15
    u8 mode: 3;
};
// extern u8 get_sfc_data_width(void);
extern void sfc_suspend(u32 enable_spi);
extern void sfc_resume(u32 disable_spi);
extern int tzflash_spi_init(const struct tzspi_config_data *pdata);
extern void tzflash_sfc_init(u32 data_width, u32 mode, u32 base_addr, u32 capacity);
extern void tzflash_sfc_reinit(u32 data_width, u32 value);





struct flash_reg {
    u8 cmd[4];
    u8 sr_value[4];
    u8 sr_mask[4];
    u8 num_of_reg: 2;
    u8 wr_en_mode: 1;
    u8 continue_mode: 4;
    u8 rev: 1;
};
// void spi_quad_io_init();
extern void spi_port_io_uninit(u32 port, u32 width);

// void tzspi_enter_spi_mode();
// void tzspi_exit_spi_mode();
// void tzflash_release_power_donw();
// void tzflash_enter_4byte_addr();
// void tzflash_exit_4byte_addr();
extern int tzflash_write_reg(const struct flash_reg *reg);
extern int tzflash_read_reg(struct flash_reg *reg);
extern int tzflash_update_reg(struct flash_reg *reg_wr, struct  flash_reg *reg_rd);
extern int tzflash_cpu2flash_addr(void *logic_addr);
extern void *tzflash_flash2cpu_addr(u32 phy_addr);

extern int tzflash_read(void *device, void *buffer, u32 len, u32 addr);
int tzflash_origin_read(void *buffer, u32 addr, u32 len);
extern int tzflash_write(void *device, const void *buffer, u32 len, u32 addr);
extern int tzflash_erase(void *device, u32 cmd, u32 addr);
extern int tzflash_erase_zone(void *device, u32 addr, u32 len);
extern int tzflash_ioctl(void *device, u32 cmd, u32 arg);


int tzflash_set_wps_all_lock(void *device, u32 cmd);
int tzflash_set_wps_area_lock(void *device, u32 cmd, u32 addr);
int tzflash_read_wps_area_lock(void *device, u32 cmd, u32 addr);

#ifdef ROM_SECURE_BOOT

struct jlfs_image_head_t {
    const struct sdfile_file_head *flash_head;
    const struct sdfile_file_head *uboot;
    const struct sdfile_file_head *isd_config;
    const struct sdfile_file_head *votp_cfg;
    const struct sdfile_file_head *app_dir_head;
    const struct sdfile_file_head *app_a_head; //app.bin
    const struct sdfile_file_head *app_dir_head2;
    const struct sdfile_file_head *app_b_head; //app.bin
    const void *uboot_entry;
    const void *ini_entry;
    const void *app_a_entry;
    const void *app_b_entry;
    u32 app_mmu_entry;
    u32 uboot_mmu_entry;
    u32 base_addr;
};
extern struct jlfs_image_head_t jlfs_image_head;

u32 tzflash_check_wire_mode(u32 baud, u32 port);
extern void boot_mmu_init(u32 app_mmu_entry, u32 app_real_entry);
//mc03,sh59 tzflash_sfc_reinit函数错误重新写
#endif


/*********************flash wp********************/
struct tzflash_operation {
    u32 addr;
    u8 *data_buf;
    u32 data_len;
    u32 cmd_width: 4; //0,1,2,4 //0:表示没有该项
    u32 addr_width: 4; //0,1,2,4

    u32 mode_width: 4; //0,1,2,4
    u32 dummy_width: 4; //0,1,2,4
    u32 dummy_dir: 1; //0:写, 1:读

    u32 write_en_sel: 2; //0:没有，1：non volatile，2：volatile

    u32 data_dir: 1; //0:写, 1:读
    u32 data_width: 4; //0,1,2,4

    u32 cmd : 8;
    u8 mode;
    u8 dummy_len;//byte len
    u8 addr_4byte_en;
};

struct flash_func_len {
    u16 all_len;
    u8 version;
    u8 func_len;//lb
    u8 sta_len;//lb
    u8 func_otp;//lb
    u8 func_dtr;
    u8 func_wp;//bp,tb,sec,cmp
    u8 func_wps;
    u8 func_wps_op;
    u8 func_qe;
    u8 func_srp;
    u8 func_sus;
    u8 func_dc;
    u8 func_drv;
    u8 func_mpm;//dp

    u8 func_qpi;
    u8 func_init;
    u8 func_read_continue;
} __attribute__((packed));

#define TZFLASH_STA_NUM 4
struct flash_sta_cmd {
    u8 sta_cmd_r[TZFLASH_STA_NUM];
    u8 sta_cmd_w[TZFLASH_STA_NUM];
} __attribute__((packed));

struct flash_otp_cfg {//flash otp信息
    u32 otp_offset[5];//otp page的偏移地址组数
    u16 otp_page_size;//otp的page大小
    u8 otp_NumberOfpage;//otp的page数量
    // struct flash_reg lock_cfg[2];//read,write
    u8 wr_en_cmd;//0x50h:sr,0x06h:wr_en
    u8 sr_mask[TZFLASH_STA_NUM];
    u8 sr_value[TZFLASH_STA_NUM];//注意:锁定后不可擦写,sr的值也常是1
} __attribute__((packed));

struct flash_wps_cfg {//flash wps信息
    u8 wr_en_cmd;//0x50h:sr,0x06h:wr_en
    u8 sr_mask[TZFLASH_STA_NUM];
    u8 sr_value[TZFLASH_STA_NUM];//注意:锁定后不可擦写,sr的值也常是1
    // struct flash_reg r_reg_cfg;//read
    // struct flash_reg w_reg_cfg;//write
} __attribute__((packed));

struct flash_wps_op_cfg {
    u8	r_cmd[2];
    u8	w_cmd[2];
    u8	cfg_mask;
    u8	cfg_value;
    struct tzflash_operation read_op;
    struct tzflash_operation write_op;
} __attribute__((packed));

struct flash_wp_cfg {   //写保护配置信息
    u8 numOfwp_array;//写保护参数的个数
    u8 wr_en_cmd;//0x50h:sr,06h:
    u8 sr_mask[TZFLASH_STA_NUM]; //sr要保留或修改的bit
    struct {
        u8 sr_value[TZFLASH_STA_NUM]; //写保护sr取值
        u16 wp_addr;//写保护结束地址,单位K
    } wp_array[0]; //写保护的组数，修改可变长
} __attribute__((packed));

struct flash_dtr_cfg {
    s8 x1_dummy;//-1:使用默认值
    s8 x2_dummy;//-1:使用默认值
    s8 x4_dummy;//-1:使用默认值,-2:不支持
} __attribute__((packed));


enum tzflash_wps_status_mode {
    NORFLASH_WPS_NO_INIT = 0,//未初始化 或 不支持wps
    NORFLASH_WPS_NO_SUPPORT,
    NORFLASH_WPS_OPEN,
    NORFLASH_WPS_POFF,
};

#if TZFLASH_WP_V1_FUNC_EN
#include "inside_flash.h"
#endif
#endif  /*TZFLASH_H*/

