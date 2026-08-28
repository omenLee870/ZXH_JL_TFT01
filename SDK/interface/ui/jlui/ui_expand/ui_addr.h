/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#ifndef __UI_ADDR_H__
#define __UI_ADDR_H__

#include "system/malloc.h"
#include "asm/psram_api.h"


/* ================================= FLASH 地址处理 ================================= */

/* 判断地址是否在 flash 范围内 */
#define UI_ADDR_IN_FLASH(addr) 			(memory_in_flash((const void *)(addr)))

/* 判断地址是否在 flash cache 范围内 */
#define UI_ADDR_IN_FLASH_CACHE(addr) 	(memory_in_flash_cache((const void *)(addr)))

/* 判断地址是否在 flash nocache 范围内 */
#define UI_ADDR_IN_FLASH_NOCACHE(addr) 	(memory_in_flash_nocache((const void *)(addr)))


/* =================================  SRAM 地址处理 ================================= */

/* 判断地址是否在物理 RAM 范围内 */
#define UI_ADDR_IN_PHY_RAM(addr) 		(memory_in_phy((const void *)(addr)))

/* 判断地址是否在虚拟 RAM 范围内 */
#define UI_ADDR_IN_VLT_RAM(addr) 		(memory_in_vtl((const void *)(addr)))

/* 判断地址是否为 RAM 地址 */
#define UI_ADDR_IN_RAM(addr)			(UI_ADDR_IN_PHY_RAM(addr) || UI_ADDR_IN_VLT_RAM(addr))



/* ================================= PSRAM 地址处理 ================================= */

/* 判断地址是否在 PSRAM 范围 */
#define UI_ADDR_IN_PSRAM(addr) 			(UI_ADDR_IN_PSRAM_NOCACHE(addr) || UI_ADDR_IN_PSRAM_CACHE(addr))

/* 判断地址是否在 PSRAM nocache 范围*/
#define UI_ADDR_IN_PSRAM_NOCACHE(addr) 	(psram_no_cache_check((void *)(addr)))

/* 判断地址是否在 PSRAM cache 范围 */
#define UI_ADDR_IN_PSRAM_CACHE(addr) 	(psram_cache_check((void *)(addr)))

/* PSRAM cache 地址转 nocache*/
#define UI_PSRAM_CACHE_TO_NOCACHE(addr) (UI_ADDR_IN_PSRAM_CACHE(addr) ? ((void *)psram_cache_2_no_cache((void *)(addr))) : (void *)(addr))

/* PSRAM nocache 地址转 cache */
#define UI_PSRAM_NOCACHE_TO_CACHE(addr) (UI_ADDR_IN_PSRAM_NOCACHE(addr) ? ((void *)psram_no_cache_2_cache((void *)(addr))) : (void *)(addr))

#endif

