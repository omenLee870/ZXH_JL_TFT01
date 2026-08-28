// *INDENT-OFF*
/* #include "p11_rom_stubs.ld" */
#include "maskrom_stubs_q32f.ld"

#include "power/pmc/pmc_mmap.h"

EXTERN(
_start1
#include "p11_used_list.c"
);

/*
p11 memory地址图

0xf28000, end
0xf27fe0, poff ram, 0x20
0xf27f00, m2p_message, 0xe0
0xf27ec0, p2m_message, 0x40
0xf2xxxx, stack, 0x600
0xf2xxxx, bss
0xf20080, code/data
0xf20000, isr_vertor, 0x80
*/

PMC_F_RAM_LIMIT_L     = P11_RAM1_BEGIN;
PMC_F_RAM_LIMIT_H     = _MASK_EXPORT_MEM_BEGIN;
PMC_F_PHY_RAM_SIZE    = PMC_F_RAM_LIMIT_H - PMC_F_RAM_LIMIT_L;
ASSERT((_RAM_LIMIT_H == P11_RAM1_END),"!!! mask addr error !!!");

// 需要和主系统中的保持一致（sdk_ld.c）
CODE_ENTRY_BEG  = 0x8000100;
CODE_ENTRY_SIZE	= 8;
PMC_CODE_BEG  	= CODE_ENTRY_BEG + CODE_ENTRY_SIZE;
PMC_CODE_SIZE	= TCFG_PMC_CODE_SIZE;

PMC_RAM2_BEG 	= 0x7ec0000;
PMC_RAM2_SIZE 	= TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE ? TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE : 4;
PMC_RAM2_END 	= PMC_RAM2_BEG + PMC_RAM2_SIZE;

#if CONFIG_PMU_DEBUG_ENABLE
PMC_RAM0_SIZE 	= 4;//1024*2;
#else
PMC_RAM0_SIZE 	= 1024*8;
#endif
PMC_RAM0_END 	= P11_RAM1_BEGIN;
PMC_RAM0_BEG 	= PMC_RAM0_END - PMC_RAM0_SIZE;

MEMORY
{
    p11_message_ram(rw)		:	ORIGIN = P2M_MESSAGE_RAM_BEGIN, LENGTH = (P2M_MESSAGE_SIZE+M2P_MESSAGE_SIZE)
	msys_poweroff_ram(rw)	:	ORIGIN = MSYS_POFF_RAM_BEGIN, LENGTH = MSYS_POFF_RAM_SIZE
	p11_ram0(rw)			:	ORIGIN = PMC_RAM0_BEG, LENGTH = PMC_RAM0_SIZE
	p11_ram1(rw)			:	ORIGIN = PMC_F_RAM_LIMIT_L, LENGTH = PMC_F_PHY_RAM_SIZE
	p11_ram1_reset(rw)      :   ORIGIN = PMC1_RESET_BEGIN, LENGTH = PMC1_RESET_SIZE
	p11_ram2(rw)			:	ORIGIN = PMC_RAM2_BEG, LENGTH = PMC_RAM2_SIZE
	code0(rw)      			:   ORIGIN = PMC_CODE_BEG, LENGTH = PMC_CODE_SIZE
}

ENTRY(_start1)

SECTIONS
{
	. = ORIGIN(p11_ram1_reset);

	.reset ALIGN(4):
	{
        *startup1.o(.text)
    } > p11_ram1_reset

#if TCFG_PMC_CODE_SIZE
	. = ORIGIN(code0);
    .flash_code ALIGN(4):
    {
		flash_code_begin = .;
#ifndef CONFIG_OTA_LOADER_ENABLE

#if 1 //开启后，未指定段的代码和字符串将放在flash中
		*(.text*)
		*(.rodata*)
#endif

		*(.*.text)
		*(.*.const)

		*(.sensor_const)
		*(.*.sensor_const)
		*(.sensor_code.L2)
		*(.*.sensor_code.L2)
#endif
		flash_code_end = .;
		flash_free_start = .;
		. = ORIGIN(code0) + LENGTH(code0) - 4;
		flash_free_end = .;
		LONG(0xffffffff);
    } > code0
#else

	flash_code_begin = 0;
	flash_code_end = 0;
	flash_free_start = 0;
	flash_free_end = 0;
#endif

	. = ORIGIN(p11_ram1);

	/*
	   这里要4byte对齐,否则其实地址对不到204rst_addr
	 */
    .text ALIGN(4):
    {

#if ((TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE==0) || defined(CONFIG_OTA_LOADER_ENABLE))
#include "p11_text.ld"
#endif
		. = ALIGN(4);
		*(.sensor_code.L1)
		*(.*.sensor_code.L1)
        *(.iic.text.cache.L1)
		*(.jl_motion_code)
		*(.jl_motion_const)
		. = ALIGN(4);
    } > p11_ram1

	.data ALIGN(4):
	{
		*(.*.data)
		. = ALIGN(4);
        *(.data*)
		. = ALIGN(4);
        *(.os_data)
		. = ALIGN(4);
		*(.sensor_data.L1)
		*(.*.sensor_data.L1)
		. = ALIGN(4);
		*(.sensor_data.L2)
		*(.*.sensor_data.L2)
		. = ALIGN(4);
	} > p11_ram1

    .bss ALIGN(4):
    {
        *(.bss)
		*(.*.bss)
		*(.os_bss)
        *(COMMON)
		*(.jl_motion_bss)
		*(.sensor_bss.L1)
		*(.*.sensor_bss.L1)
		*(.sensor_bss.L2)
		*(.*.sensor_bss.L2)


        . = ALIGN(4);
        timer_pool_ram_start = .;
        . = . + (TIMER_POOL_NUM_CONFIG * 28);
        . = ALIGN(4);
        timer_pool_ram_end = .;

    } > p11_ram1

	.stack_bss ALIGN(32):
    {
		stack_magic0_start = .;
        *(.stack_magic0)
		. = stack_magic0_start + 4;
        PROVIDE(_stack_begin = .);
        PROVIDE(_sstack_begin = .);
        *(.sstack)
        PROVIDE(_sstack_end = .);

        PROVIDE(_ustack_begin = .);
        *(.ustack)
        PROVIDE(_ustack_end = .);

        PROVIDE(_stack_end = .);
		stack_magic1_start = .;
        *(.stack_magic1)
		. = stack_magic1_start + 4;
    } > p11_ram1



#if 0
	PROVIDE(overlay_demo_begin = .);
	OVERLAY : AT(0x200000) SUBALIGN(4)
	{
		.overlay_bank_demo_init
		{
            LONG(0xffffffff);
            *(.demo.code.bank.0)
            . = ALIGN(4);
		}
		.overlay_bank_demo_normal0
		{
            LONG(0xffffffff);
            *(.demo.code.bank.1)
		}

		.overlay_bank_demo_normal1
		{
            LONG(0xffffffff);
            *(.demo.code.bank.2)
		}
	} > p11_ram1
	PROVIDE(overlay_demo_end = .);


	PROVIDE(overlay_sensor_begin = .);
	OVERLAY : AT(0x210000) SUBALIGN(4)
	{
		/* TODO: */
		.overlay_bank_sensor_init
		{
            LONG(0xffffffff);
            *(.sensor.code.bank.0)
            . = ALIGN(4);
		}
		.overlay_bank_sensor_normal0
		{
            LONG(0xffffffff);
            *(.sensor.code.bank.1)
		}
		.overlay_bank_sensor_normal1
		{
            LONG(0xffffffff);
            *(.sensor.code.bank.2)
		}

	} > p11_ram1
	PROVIDE(overlay_sensor_end = .);

	PROVIDE(overlay_sys_begin = .);
	OVERLAY : AT(0x220000) SUBALIGN(4)
	{
		/* TODO: */
		.overlay_bank_sys_init
		{
            LONG(0xffffffff);
            *(.sys.code.bank.0)
            . = ALIGN(4);
		}
		.overlay_bank_sys_normal0
		{
            LONG(0xffffffff);
            *(.sys.code.bank.1)
		}
	} > p11_ram1
	PROVIDE(overlay_sys_end = .);
#endif

	/* PROVIDE(p11_heap_begin = .); */
     /* p11_heap_end = P2M_MESSAGE_RAM_BEGIN; */
	p11_free_ram_start = .;
	p11_free_ram_end = PMC_F_RAM_LIMIT_H;

	. = ORIGIN(p11_message_ram);
    .p11_message ALIGN(32):
    {
        *(.p2m_data)
        *(.m2p_data)
    } > p11_message_ram

	. = ORIGIN(msys_poweroff_ram);
    .p11_poweroff ALIGN(32):
    {
        *(.msys_poff_data)
    } > msys_poweroff_ram


	. = ORIGIN(p11_ram2);
	.ram2_text ALIGN(4):
    {
#if (TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE!=0)
#include "p11_text.ld"
#endif

		*(.text*)
		*(.*.text)
		*(.*.const)
		LONG(0xffffffff); // 避免bank批处理出差
    } > p11_ram2

	.ram2_data ALIGN(4):
    {
		. = ALIGN(4);
        *(.data*)
    } > p11_ram2

	.ram2_bss ALIGN(4):
    {
		. = ALIGN(4);
        *(.bss*)
    } > p11_ram2

	ram2_free_ram_start = .;
	ram2_free_ram_end = PMC_RAM2_END;


	. = ORIGIN(p11_ram0);
    .ram0_bss ALIGN(4):
    {
        *(.pmu_free_bss)
		. = ORIGIN(p11_ram0) + LENGTH(p11_ram0) - 4;
		LONG(0xffffffff);
    } > p11_ram0

}

/* flash_code_begin  = ADDR(.flash_code); */
/* flash_code_size   = SIZEOF(.flash_code); */
/* flash_code_end    = flash_code_begin + flash_code_size; */

text_begin  = ADDR(.text);
text_size   = SIZEOF(.text);
text_end    = text_begin + text_size;

bss_begin = ADDR(.bss);
bss_size  = SIZEOF(.bss);

p11_message_begin = ADDR(.p11_message);
p11_message_size = SIZEOF(.p11_message);

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data);

text2_begin  = ADDR(.ram2_text);
text2_size   = SIZEOF(.ram2_text);
text2_end    = text2_begin + text2_size;

bss2_begin = ADDR(.ram2_bss);
bss2_size  = SIZEOF(.ram2_bss);

data2_addr = ADDR(.ram2_data);
data2_begin = text2_begin + text2_size;
data2_size =  SIZEOF(.ram2_data);

pmu_free_begin = ADDR(.ram0_bss);
pmu_free_size  = SIZEOF(.ram0_bss);

/* _HEAP_BEGIN = p11_heap_begin; */
/* _HEAP_END = p11_heap_end; */
/* PROVIDE(HEAP_BEGIN = _HEAP_BEGIN); */
/* PROVIDE(HEAP_END = _HEAP_END); */
/* _MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN; */
/* PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN); */

p11_free_ram_size = p11_free_ram_end - p11_free_ram_start;
p11_msys_free_ram_size = ram2_free_ram_end - ram2_free_ram_start;

flash_free_size = flash_free_end - flash_free_start;


