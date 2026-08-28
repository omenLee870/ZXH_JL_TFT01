// pmc 的sram给主系统使用
#include "app_config.h"
#include "audio_config_def.h"

PMC_RAM_BEG = 0x7E40000;
PMC_RAM_END = 0x7E60000;
#if (PMC_CPU_CORE_NUM == 0)
PMC_RAM_SIZE = PMC_RAM_END - PMC_RAM_BEG;
#else
PMC_RAM_SIZE = 0;
#endif

MEMORY {
    pmc_ram(rwx)         : ORIGIN = PMC_RAM_BEG,  LENGTH = PMC_RAM_SIZE
    }

    SECTIONS {
    . = ORIGIN(pmc_ram);
    .pmc_ram_data_code ALIGN(32):
    {
        *(.pmc_ram_data)
        * (.pmc_ram_code)
        . = ALIGN(4);
    } > pmc_ram

    .pmc_ram_bss ALIGN(32):
    {
        *(.pmc_bss)
        . = ALIGN(4);
    } > pmc_ram

    .pmc_ram_noinit ALIGN(32):
    {
        *(.pmc_noinit)
        . = ALIGN(4);
    } > pmc_ram

    _PMC_RAM_HEAP_BEGIN = .;
    _PMC_RAM_HEAP_END = PMC_RAM_END;
}

pmc_ram_bss_begin = ADDR(.pmc_ram_bss);
pmc_ram_bss_size  = SIZEOF(.pmc_ram_bss);
pmc_ram_bss_end    = pmc_ram_bss_begin + pmc_ram_bss_size;
ASSERT((pmc_ram_bss_size % 4) == 0, "!!! pmc_ram_bss_size Not Align 4 Bytes !!!");

pmc_ram_data_code_begin = icache_ram_data_code_begin + icache_ram_data_code_size;
pmc_ram_data_code_addr = ADDR(.pmc_ram_data_code);
pmc_ram_data_code_size = SIZEOF(.pmc_ram_data_code);
ASSERT((pmc_ram_data_code_size % 4) == 0, "!!! pmc_ram_data_code_size Not Align 4 Bytes !!!");

