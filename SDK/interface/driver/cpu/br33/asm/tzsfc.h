#ifndef  __TZSFC_H__
#define  __TZSFC_H__


#define    TZFLASH_SFC       PMC_SFC0
#define    TZFLASH_SFCDEC    PMC_SFCDEC0
#define    TZFLASH_BOOTMMU   PMC_BOOTMMU0
#define    TZFLASH_SPI       PMC_FSPI0

/*********************spi********************/
#ifdef CONFIG_FPGA_ENABLE
#define SPI0_BAUD_FREQ    12000000
#else
#define SPI0_BAUD_FREQ    24000000
#endif
#define SPI0_CLK_SRC_HSB  0
#if SPI0_CLK_SRC_HSB
#define SPI0_SOURCE_FREQ  clk_get("sys")//br35:spi0 src:hsb
#else
#define SPI0_SOURCE_FREQ  clk_get("lsb")//br35:spi0 src:hsb
#endif


void tzspi_clock_critical_exit();

#define SPI0_DODI_DISABLE()   TZFLASH_SPI->CON1 &=~ BIT(2)


#define TZFLASH_LOADER2CACHE_MODE_EN    1//loader cache mode


/*********************sfc********************/
#define FLASH_CPU_ADDR_END 		(0xFFFFFFFUL)
//tzsfc
#define SFC_SUPPORT_DTR_MODE          1
#define SFC_SUPPORT_PART_NDEC_MODE    1
#define  CACHE_LINE_COUNT  32
enum sfcx_mode {
    NORMAL_READ = 0,//old:0b0000  //CMD 1bit, ADDR 1bit clk < 30MHz
    FAST_READ = 1,      //old:0b0001  //CMD 1bit, ADDR 1bit
    DUAL_WIRE_SEND_COMMAND_MODE = 2,//CMD 2 WIRE, ADDR 2 WIRE
    QUAD_WIRE_SEND_COMMAND_MODE = 3,//CMD 4 WIRE, ADDR 4 WIRE
    DUAL_FAST_READ_OUTPUT = 4,      //CMD 1 WIRE, ADDR 1 WIRE  //old:0b0010
    QUAD_FAST_READ_OUTPUT = 5,      //CMD 1 WIRE, ADDR 1 WIRE  //old:0b0011
    DUAL_FAST_READ_IO = 6,          //CMD 1 WIRE, ADDR 2 WIRE  //old:0b0100
    QUAD_FAST_READ_IO = 7,          //CMD 1 WIRE, ADDR 4 WIRE  //old:0b0101

    DUAL_FAST_READ_IO_CONTINUE = 0x16, //CMD 1 WIRE,no cmd ADDR 2 WIRE  //old:0b0110
    QUAD_FAST_READ_IO_CONTINUE = 0x17, //CMD 1 WIRE,no cmd ADDR 4 WIRE//old:0b0111
    QUAD_WIRE_SEND_COMMAND_IO_CONTINUE = 0x13,//CMD 4 WIRE,no cmd ADDR 4 WIRE
};
extern u32 _SFC_TRIM_CODE_START;
extern u32 _SFC_TRIM_CODE_END;
extern u32 _SFC_MEMORY_START_ADDR[];
extern u32 _SFC_MEMORY_CACHE_SIZE[];
extern void sfc_resume(u32 disable_spi);
extern void sfc_suspend(u32 enable_spi);
extern u8 get_sfc_read_mode(void);
extern u8 get_sfc_data_width(void);
extern const u8 sfc0_dtr_mode_en;
extern const s8 sfc0_dtr_dummy_num;
extern const u32 sfc0_dtr_clk_freq;//查看flash文档*2
extern const u8 sfc0_continue_mode_en;
extern const u8 tzflash_write_protect_mode_en;
extern void sfc_drop_cache(void *ptr, u32 len);
enum sfcx_mode  sfc0_get_mode();
u32 sfc0_is_dtr_mode();
u32 sfc0_is_addr_4byte_mode();
u32 sfc0_is_continue_mode();
void sfc0_continue_switch(u8 continue_en);
void sfc0_dtr_init(s8 dummy_num);//base:ini
void sfc0_dtr_continue_init(u8 dtr_mode, u8 continue_mode, s8 dummy_num);//base:ini

u32 sfc0_get_offset_addr();
u32 sfc0_get_enc_zone(u32 *saddr, u32 *eaddr);
void sfc0_set_enc_zone(u32 saddr, u32 eaddr);
#endif

