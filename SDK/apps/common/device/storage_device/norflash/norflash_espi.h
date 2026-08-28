#ifndef _ESPI_NOR_FLASH_H_
#define _ESPI_NOR_FLASH_H_

//!!!根据701客户历史反馈情况，不建议直接使用这部分接口,建议使用分区dev接口，避免用户读写异常破坏了整个flash

#define ESPI_NORFLASH_READ_FREQ   85000000
#define ESPI_NORFLASH_WRITE_FREQ  85000000

extern void espi_norflash_init(u8 read_mode, u8 width);
extern u32 espi_norflash_read(u8 *buf, u32 addr, u32 len);//len:any//no continue,no 0单线 //no bbh
extern u32 espi_norflash_write(u8 *buf, u32 addr, u32 len);
extern int espi_norflash_ioctl(u32 cmd, u32 arg);


extern void norflash_espi_mutex_enter();
extern void norflash_espi_mutex_exit();
//外挂flash 直接cpu 寻址请注意加上互斥

#endif

