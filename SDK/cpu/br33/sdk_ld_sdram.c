// *INDENT-OFF*
#include "app_config.h"
#include "audio_config_def.h"

/* ==================================  BR33 SDK memory ============================================

 _______________
|    update     |
|_______________|___ RAM_LIMIT_H
|     HEAP      |
|_______________|___ data_code_pc_limit_H
| audio overlay |
|_______________|
|   data_code   |
|_______________|___ data_code_pc_limit_L
|     bss       |
|_______________|
|     data      |
|_______________|
|     TLB       |
|_______________|
|   irq_stack   |
|_______________|
|   boot info   |
|_______________|___ RAM_LIMIT_L
|rom export ram |
|_______________|___ 0x100200
|   isr base    |
|_______________|___ 0x100000

 =============================================================================================== */

#if CPU_CORE_NUM > 1
#include "maskrom_stubs_d.ld"
#else
#include "maskrom_stubs_s.ld"
#endif

EXTERN(
_start
#include "sdk_used_list.c"
#ifdef BUILD_BSP_PROJ_ENABLE
#include "bsp/bsp_api.c"
#endif
);

SRAM_ACTUAL_BEG = _RAM_LIMIT_L + TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE;
SRAM_ACTUAL_SIZE = CONFIG_PHYSIC_RAM_SIZE - TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE;

UPDATA_SIZE     = 0x200;
/* UPDATA_BEG      = _RAM_LIMIT_H - UPDATA_SIZE; */
// 由于不同版本sof不同,所以ram大小跟据FPGA_TYPE自动配置,不使用maskrom的
UPDATA_BEG      = (SRAM_ACTUAL_BEG + SRAM_ACTUAL_SIZE) - UPDATA_SIZE;
UPDATA_BREDR_BASE_BEG = 0x4000000;

RAM_LIMIT_L     = SRAM_ACTUAL_BEG;
RAM_LIMIT_H     = UPDATA_BEG;
PHY_RAM_SIZE    = RAM_LIMIT_H - RAM_LIMIT_L;

//from mask export
ISR_BASE       = _IRQ_MEM_ADDR;
ROM_RAM_SIZE   = _MASK_EXPORT_MEM_SIZE;// _MASK_MEM_SIZE;
ROM_RAM_BEG    = _MASK_EXPORT_MEM_BEGIN;//_MASK_MEM_BEGIN;

RAM0_BEG 		= RAM_LIMIT_L;
RAM0_END 		= RAM_LIMIT_H;
RAM0_SIZE 		= RAM0_END - RAM0_BEG;

RAM1_BEG 		= RAM_LIMIT_L;
RAM1_END 		= RAM0_BEG;
RAM1_SIZE 		= RAM1_END - RAM1_BEG;

// 需要和PMC中的保持一致（p11_ld.c）
CODE_ENTRY_BEG  = 0x14000100;
CODE_ENTRY_SIZE	= 8;
PMC_CODE_BEG  	= CODE_ENTRY_BEG + CODE_ENTRY_SIZE;
PMC_CODE_SIZE	= TCFG_PMC_CODE_SIZE;
CODE_BEG        = PMC_CODE_BEG + PMC_CODE_SIZE;
CODE_SIZE		= TCFG_PSRAM_SIZE - CODE_ENTRY_SIZE - PMC_CODE_SIZE;

PSRAM_BEGIN     = 0x14000000;

#if !TCFG_PSRAM_DEV_ENABLE
FREE_DACHE_WAY = TCFG_FREE_DCACHE_WAY;
FREE_IACHE_WAY = TCFG_FREE_ICACHE_WAY;
DCACHE_RAM_SIZE = FREE_DACHE_WAY*8K;
ICACHE_RAM_SIZE = FREE_IACHE_WAY*8K;
PSRAM_SIZE      = 0;
#else
// 使能psram时,默认不将cache当ram用
FREE_DACHE_WAY = 0;
FREE_IACHE_WAY = 0;
DCACHE_RAM_SIZE = 0;
ICACHE_RAM_SIZE = 0;
PSRAM_SIZE      = 0;//TCFG_PSRAM_SIZE;
#endif

PSRAM_END       = PSRAM_BEGIN + PSRAM_SIZE;

//=============== About BT RAM ===================
//CONFIG_BT_RX_BUFF_SIZE = (1024 * 18);

MEMORY
{
	code_entry(rx) 	  : ORIGIN = CODE_ENTRY_BEG,  LENGTH = CODE_ENTRY_SIZE
	code0(rx)    	  : ORIGIN = CODE_BEG,  LENGTH = CODE_SIZE
	ram0(rwx)         : ORIGIN = RAM0_BEG,  LENGTH = RAM0_SIZE

    //ram1 - 用于volatile-heap
	//ram1(rwx)         : ORIGIN = RAM1_BEG,  LENGTH = RAM1_SIZE

    psram(rwx)        : ORIGIN = PSRAM_BEGIN, LENGTH = PSRAM_SIZE
    dcache_ram(rw)    : ORIGIN =  0x7D70000+((4-FREE_DACHE_WAY)*8K), LENGTH = DCACHE_RAM_SIZE
    icache_ram(rw)    : ORIGIN =  0x7DC0000+((4-FREE_IACHE_WAY)*8K), LENGTH = ICACHE_RAM_SIZE
}


ENTRY(_start1)

SECTIONS
{
    . = ORIGIN(ram0);

	//TLB 起始需要16K 对齐；
    .mmu_tlb ALIGN(0x4000):
    {
        *(.mmu_tlb_segment);
        _mmu_tlb_begin = .;
        . = . + (TCFG_MMU_MEMORY_SIZE / 256) * 2;
        _mmu_tlb_end = .;
    } > ram0

	.boot_info ALIGN(32):
	{
        *(.boot_info)
        . = ALIGN(256);
        // 需要避免与uboot和maskrom冲突
        *(.debug_record)
        . = ALIGN(4);
	} > ram0

	.irq_stack ALIGN(32):
    {
        _cpu0_sstack_begin = .;
        *(.cpu0_stack)
        _cpu0_sstack_end = .;
		. = ALIGN(4);

        _cpu1_sstack_begin = .;
        *(.cpu1_stack)
        _cpu1_sstack_end = .;
		. = ALIGN(4);

    } > ram0

    // os bss依然存放在bss段
    //.os_data_bss ALIGN(4):
    //{
    //    *(.os.data.bss)
    //} > ram0

	.bss ALIGN(32):SUBALIGN(4)
    {
        . = ALIGN(4);
		/* #include "system/system_lib_bss.ld" */

        . = ALIGN(4);
        *(.bss)

        . = ALIGN(4);
        *(.volatile_ram)

		*(.btstack_pool)

        *(.mem_heap)
#if	!TCFG_PSRAM_DEV_ENABLE
        *(.memp_memory_x)
#endif
        . = ALIGN(4);
		lp_data_save_bss_begin = .;
        *(.power_driver.data.bss.overlay)
#if DAC_OBUF_OVERLAY_LP_BSS_EN
		lp_data_save_bss_src_end = .;
		. = (lp_data_save_bss_begin + 4800); // 可以在dac_overlay_malloc()中打印获取size
#endif
		lp_data_save_bss_end = .;
		lp_data_save_bss_size = lp_data_save_bss_end - lp_data_save_bss_begin;

        . = ALIGN(4);
        *(.usb.data.bss.exchange)

        . = ALIGN(4);
		*(.non_volatile_ram)

		*(.cmac_sch_bss)
		*(.cmac_lib_bss)
		*(.cmac_app_bss)
		*(.bt_lbuf_bss)
		*(.ble_rf_bss)
		*(.cmac_lbuf_pool)


        . = ALIGN(4);
#if ((!TCFG_ICACHE_RUN_BT_STATIC_RAM) && (!TCFG_DCACHE_RUN_BT_STATIC_RAM))
        #include "btctrler/btctler_lib_bss.ld"
#endif
        #include "btctrler/crypto/bss.ld"
        . = ALIGN(32);

    } > ram0

	.data ALIGN(32):SUBALIGN(4)
	{
        //common bank code addr
        common_code_run_addr = .;

		//cpu start
        . = ALIGN(4);
        *(.data_magic)

         . = ALIGN(4);
         __a2dp_movable_slot_start = .;
         *(.movable.slot.1);
         __a2dp_movable_slot_end = .;

		. = ALIGN(4);
        app_mode_begin = .;
		KEEP(*(.app_mode))
        app_mode_end = .;

		*(.cmac_sch_data)
		*(.cmac_lib_data)
		*(.cmac_app_data)
		*(.bt_lbuf_data)
		*(.ble_rf_data)


		. = ALIGN(4);
        /* #include "system/system_lib_data.ld" */
        #include "btctrler/crypto/data.ld"

        . = ALIGN(4);
        #include "btctrler/btctler_lib_data.ld"

		. = ALIGN(4);
	} > ram0


	.data_code ALIGN(32):SUBALIGN(4)
	{
		data_code_pc_limit_begin = .;
        *(.must_ram_code)
        *(.cache)
		*(.flushinv_icache)

		*(.lz4_code)

		///<--- cmac 代码放ram
		*(.cmac_ram_const)
		*(.cmac_ram_code)
		*(.cmac_sch_ram_code)

		*(.bt_lbuf_code)
		*(.bt_lbuf_const)


#if (!TCFG_ICACHE_RUN_DATA_CODE)
    	#include "sdk_ram_code.ld"
#endif
		. = ALIGN(4);
	} > ram0

	__report_overlay_begin = .;
    #include "app_overlay.ld"


    //bank code addr
    bank_code_run_addr = .;

    OVERLAY : AT(0x300000) SUBALIGN(4)
    {
        .overlay_bank0
        {
            *(.bank.code.0*)
            *(.bank.const.0*)
            . = ALIGN(4);
        }
        .overlay_bank1
        {
            *(.bank.code.1*)
            *(.bank.const.1*)
            . = ALIGN(4);
        }
        .overlay_bank2
        {
            *(.bank.code.2*)
            *(.bank.const.2*)
            *(.bank.ecdh.*)
            . = ALIGN(4);
        }
        .overlay_bank3
        {
            *(.bank.code.3*)
            *(.bank.const.3*)
            *(.bank.enc.*)
            . = ALIGN(4);
        }
        .overlay_bank4
        {
            *(.bank.code.4*)
            *(.bank.const.4*)
            . = ALIGN(4);
        }
        .overlay_bank5
        {
            *(.bank.code.5*)
            *(.bank.const.5*)
            . = ALIGN(4);
        }
        .overlay_bank6
        {
            *(.bank.code.6*)
            *(.bank.const.6*)
            . = ALIGN(4);
        }
        .overlay_bank7
        {
            *(.bank.code.7*)
            *(.bank.const.7*)
            . = ALIGN(4);
        }
        .overlay_bank8
        {
            *(.bank.code.8*)
            *(.bank.const.8*)
            . = ALIGN(4);
        }
        .overlay_bank9
        {
            *(.bank.code.9*)
            *(.bank.const.9*)
            . = ALIGN(4);
        }
    } > ram0


	data_code_pc_limit_end = .;
	__report_overlay_end = .;

	_HEAP_BEGIN = . ;
#if CONFIG_LCD_BUF_STATIC_RAM_LEN
	_HEAP_END = RAM0_END - CONFIG_LCD_BUF_STATIC_RAM_LEN;
#else
	_HEAP_END = RAM0_END;
#endif
	_LCD_BUF_STATIC_START = _HEAP_END;
	_LCD_BUF_STATIC_END = RAM0_END;

	. = ORIGIN(code_entry);
	.startup_entry ALIGN(4):
	{
        *(.startup_entry.text)
		. = ALIGN(4);
    } > code_entry

    . = ORIGIN(code0);
    .text ALIGN(4):SUBALIGN(4)
    {
        PROVIDE(text_rodata_begin = .);

        *(.startup.text)
        . = ALIGN(4);
        _SFC_TRIM_CODE_START = . ;
        *(.sfc_trim_code)
        . = ALIGN(4);
        _SFC_TRIM_CODE_END = . ;
        . = ALIGN(32);
        *(.sfc_trim_verify_data)

        *(.startup_func.text)

	    bsp_export_begin = .;
        KEEP(*(.sec.bsp_export))
	    bsp_export_end = .;

		*(.text)
		. = ALIGN(4);

		*(.*.cache.L2*)
		. = ALIGN(4);

	    update_target_begin = .;
	    PROVIDE(update_target_begin = .);
	    KEEP(*(.update_target))
	    update_target_end = .;
	    PROVIDE(update_target_end = .);

		. = ALIGN(4); // must at tail, make rom_code size align 4
        PROVIDE(text_rodata_end = .);



        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;

        hsb_critical_handler_begin = .;
        KEEP(*(.hsb_critical_txt))
        hsb_critical_handler_end = .;

        lsb_critical_handler_begin = .;
        KEEP(*(.lsb_critical_txt))
        lsb_critical_handler_end = .;

        chargestore_handler_begin = .;
        KEEP(*(.chargestore_callback_txt))
        chargestore_handler_end = .;

        . = ALIGN(4);
        app_msg_handler_begin = .;
		KEEP(*(.app_msg_handler))
        app_msg_handler_end = .;

        . = ALIGN(4);
        app_msg_prob_handler_begin = .;
		KEEP(*(.app_msg_prob_handler))
        app_msg_prob_handler_end = .;

        . = ALIGN(4);
        app_charge_handler_begin = .;
		KEEP(*(.app_charge_handler.0))
		KEEP(*(.app_charge_handler.1))
        app_charge_handler_end = .;

        . = ALIGN(4);
        scene_ability_begin = .;
		KEEP(*(.scene_ability))
        scene_ability_end = .;

	    . = ALIGN(4);
        _ctp_mapping_tab_begin = .;
        PROVIDE(ctp_mapping_tab_begin = .);

        . = ALIGN(4);
        _ctp_mapping_tab_system_cmd_begin = .;
        PROVIDE(ctp_mapping_tab_system_cmd_begin = .);
        KEEP(*(.ctp_system_cmd))
        KEEP(*(.ctp_system_cmd.*))
        _ctp_mapping_tab_system_cmd_end = .;
        PROVIDE(ctp_mapping_tab_system_cmd_end = .);

        . = ALIGN(4);
        _ctp_mapping_tab_video_cmd_begin = .;
        PROVIDE(ctp_mapping_tab_video_cmd_begin = .);
        KEEP(*(.ctp_video_cmd))
        KEEP(*(.ctp_video_cmd.*))
        _ctp_mapping_tab_video_cmd_end = .;
        PROVIDE(ctp_mapping_tab_video_cmd_end = .);

        . = ALIGN(4);
        _ctp_mapping_tab_photo_cmd_begin = .;
        PROVIDE(ctp_mapping_tab_photo_cmd_begin = .);
        KEEP(*(.ctp_photo_cmd))
        KEEP(*(.ctp_photo_cmd.*))
        _ctp_mapping_tab_photo_cmd_end = .;
        PROVIDE(ctp_mapping_tab_photo_cmd_end = .);

		. = ALIGN(4);
        _ctp_mapping_tab_screen_mirror_cmd_begin = .;
        PROVIDE(ctp_mapping_tab_screen_mirror_cmd_begin = .);
        KEEP(*(.ctp_screen_mirror_cmd))
        KEEP(*(.ctp_screen_mirror_cmd.*))
        _ctp_mapping_tab_screen_mirror_cmd_end = .;
        PROVIDE(ctp_mapping_tab_screen_mirror_cmd_end = .);

        . = ALIGN(4);
        _ctp_mapping_tab_end = .;
        PROVIDE(ctp_mapping_tab_end = .);

         #include "media/framework/section_text.ld"

		. = ALIGN(4);
		tool_interface_begin = .;
		KEEP(*(.tool_interface))
		tool_interface_end = .;

		. = ALIGN(4);
        effects_online_adjust_begin = .;
        KEEP(*(.effects_online_adjust))
        effects_online_adjust_end = .;

        . = ALIGN(4);
        tws_tone_cb_begin = .;
        KEEP(*(.tws_tone_callback))
        tws_tone_cb_end = .;

        . = ALIGN(4);
        vm_reg_id_begin = .;
        KEEP(*(.vm_manage_id_text))
        vm_reg_id_end = .;

		. = ALIGN(4);
        *(.gpio.text.cache.L2)
		*(.LED_code)
		*(.LED_const)
		. = ALIGN(4);

        __fm_movable_region_start = .;
        *(.movable.region.1)
        __fm_movable_region_end = .;
        __fm_movable_region_size = ABSOLUTE(__fm_movable_region_end - __fm_movable_region_start);

		. = ALIGN(4);
        __bt_movable_region_start = .;
        *(.movable.region.2)
        __bt_movable_region_end = .;
        __bt_movable_region_size = ABSOLUTE(__bt_movable_region_end - __bt_movable_region_start);

		. = ALIGN(4);
        __aac_movable_region_start = .;
        *(.movable.region.3)
        __aac_movable_region_end = .;
        __aac_movable_region_size = ABSOLUTE(__aac_movable_region_end - __aac_movable_region_start);
        . = ALIGN(4);
        *(.bt_aac_dec_const)
        *(.bt_aac_dec_sparse_const)

		. = ALIGN(4);
        __aec_movable_region_start = .;
        *(.movable.region.4)
        __aec_movable_region_end = .;
        __aec_movable_region_size = ABSOLUTE(__aec_movable_region_end - __aec_movable_region_start);

		. = ALIGN(4);
        __mic_eff_movable_region_start = .;
        *(.movable.region.5)
        __mic_eff_movable_region_end = .;
        __mic_eff_movable_region_size = ABSOLUTE(__mic_eff_movable_region_end - __mic_eff_movable_region_start);

		/********maskrom arithmetic ****/
        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.bfilt_code)
        *(.bfilt_const)
		/********maskrom arithmetic end****/

		*(.cmac_lib_code)
		*(.cmac_lib_const)
		*(.cmac_app_code)
		*(.cmac_app_const)


  		cmac_sch_begin = .;
		*(.cmac_sch_code)
		*(.cmac_sch_const)
  		cmac_sch_end = .;
		cmac_sch_size = cmac_sch_end - cmac_sch_begin;

		*(.ble_ll_code)
		*(.ble_rf_code)


		. = ALIGN(4);
		__VERSION_BEGIN = .;
        KEEP(*(.sys.version))
        __VERSION_END = .;

        *(.noop_version)

		. = ALIGN(4);
         __a2dp_text_cache_L2_start = .;
         *(.movable.region.1);
         . = ALIGN(4);
         __a2dp_text_cache_L2_end = .;
         . = ALIGN(4);
		/* #include "system/system_lib_text.ld" */
        #include "btctrler/crypto/text.ld"
        #include "btctrler/btctler_lib_text.ld"

        . = ALIGN(4);
		_record_handle_begin = .;
		PROVIDE(record_handle_begin = .);
		KEEP(*(.debug_record_handle_ops))
		_record_handle_end = .;
		PROVIDE(record_handle_end = .);

        . = ALIGN(4);
		wireless_trans_ops_begin = .;
		KEEP(*(.wireless_trans))
		wireless_trans_ops_end = .;

        . = ALIGN(32);
	  } > code0
#if defined CONFIG_CXX_SUPPORT
    PROVIDE(ctors_count = _ctors_end - _ctors_begin);
#endif

    .ps_ram_data_code ALIGN(32):
    {
        *(.psram_data)
        *(.psram_code)
        . = ALIGN(4);
    } > code0

    .ps_ram_bss ALIGN(32):
    {
        *(.psram_bss)
#if	TCFG_PSRAM_DEV_ENABLE
		*(.memp_memory_x)
#endif
        . = ALIGN(4);
    } > code0

    .ps_ram_noinit ALIGN(32):
    {
        *(.psram_noinit)
        . = ALIGN(4);
    } > code0

    _PSRAM_HEAP_BEGIN = .;
    _PSRAM_HEAP_END = PSRAM_END;

    . = ORIGIN(dcache_ram);
    .dcache_ram_data ALIGN(32):SUBALIGN(4)
    {
   		*(.dch_ram_data)
		. = ALIGN(4);
    } > dcache_ram

    .dcache_ram_bss ALIGN(32):SUBALIGN(4)
    {
#if (TCFG_DCACHE_RUN_BT_STATIC_RAM)
        #include "btctrler/btctler_lib_bss.ld"
#endif
        *(.dcache_bss)
   		*(.dch_ram_bss)
		. = ALIGN(4);

		// 放最后面
		dcache_ram_bss_remain_begin = .;
		. = ORIGIN(dcache_ram) + LENGTH(dcache_ram);
		dcache_ram_bss_remain_end = .;
		. = ALIGN(4);
    } > dcache_ram

    . = ORIGIN(icache_ram);
    .icache_ram_data_code ALIGN(32):SUBALIGN(4)
    {
		icache_ram_data_code_pc_limit_begin = .;
   		*(.ich_ram_code)
#if (TCFG_ICACHE_RUN_DATA_CODE)
    	#include "sdk_ram_code.ld"
   		*(.power_driver.text.cache.L1)
#endif
		. = ALIGN(4);
        *(.icache_code)
		. = ALIGN(4);
		icache_ram_data_code_pc_limit_end = .;
    } > icache_ram
    .icache_ram_bss ALIGN(32):SUBALIGN(4)
    {
   		*(.ich_ram_bss)
#if (TCFG_ICACHE_RUN_BT_STATIC_RAM)
        #include "btctrler/btctler_lib_bss.ld"
#endif
		. = ALIGN(4);

		// 放最后面
		icache_ram_bss_remain_begin = .;
		. = ORIGIN(icache_ram) + LENGTH(icache_ram);
		icache_ram_bss_remain_end = .;
		. = ALIGN(4);
    } > icache_ram
}

#include "app.ld"
#include "update/update.ld"
#include "btstack/btstack_lib.ld"
#include "driver/cpu/br33/driver_lib.ld"
#include "utils/utils_lib.ld"
#include "ui/ui.ld"
#ifdef CONFIG_AUDIO_ENABLE
#include "cvp/audio_cvp_lib.ld"
#include "media/media_lib.c"
#endif

#if CONFIG_JL_UI_ENABLE
/* #include "jlui/ui.ld" */
#endif

#include "system/port/br33/system_lib.ld" //Note: 为保证各段对齐, 系统ld文件必须放在最后include位置

//================== mmu tlb addr check ======================
// mmu tlb的位置不合理,可能会由于对齐产生较大的内存空隙(空隙无法使用)
ASSERT(ADDR(.mmu_tlb) <= (0x7EC4000+TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE),"mmu tlb location unreasonable !!!");

//================== cache ram size check ======================
ASSERT(FREE_DACHE_WAY <= 3,"cache ram size config err !!!");
ASSERT(FREE_IACHE_WAY <= 3,"cache ram size config err !!!");
// 不能关闭太多icache,否则会导致无法使用spi code load to cache功能(icache空间不够装spi代码)

//================== Section Info Export ====================//
text_begin  = ADDR(.text);
text_size   = SIZEOF(.text);
text_end    = text_begin + text_size;
ASSERT((text_size % 4) == 0,"!!! text_size Not Align 4 Bytes !!!");

_sdk_text_addr = text_begin;
_sdk_text_size = text_size;
_sdk_text_addr_end = _sdk_text_addr + _sdk_text_size;


bss_begin = ADDR(.bss);
bss_size  = SIZEOF(.bss);
bss_end    = bss_begin + bss_size;
ASSERT((bss_size % 4) == 0,"!!! bss_size Not Align 4 Bytes !!!");

boot_info_addr = ADDR(.boot_info);
boot_info_size = SIZEOF(.boot_info);
/* ASSERT(boot_info_size >= 128,"!!! boot_info_size must larger than 128 Bytes !!!"); */

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data);
ASSERT((data_size % 4) == 0,"!!! data_size Not Align 4 Bytes !!!");

data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size =  SIZEOF(.data_code);
ASSERT((data_code_size % 4) == 0,"!!! data_code_size Not Align 4 Bytes !!!");

//================ OVERLAY Code Info Export ==================//
#ifndef CONFIG_CODE_MOVABLE_ENABLE
aec_addr = ADDR(.overlay_aec);
aec_begin = data_code_begin + data_code_size;
aec_size =  SIZEOF(.overlay_aec);

aac_addr = ADDR(.overlay_aac);
aac_begin = aec_begin + aec_size;
aac_size =  SIZEOF(.overlay_aac);
#endif

/*
lc3_addr = ADDR(.overlay_lc3);
lc3_begin = aac_begin + aac_size;
lc3_size = SIZEOF(.overlay_lc3);
*/

//================ psram ==================//
#ifndef CONFIG_CODE_MOVABLE_ENABLE
ps_ram_data_code_begin = aac_begin + aac_size;
#else
ps_ram_data_code_begin = data_code_begin + data_code_size;
#endif
ps_ram_data_code_addr = ADDR(.ps_ram_data_code);
ps_ram_data_code_size = SIZEOF(.ps_ram_data_code);

//================ dcache ==================//
dcache_ram_bss_begin = ADDR(.dcache_ram_bss);
dcache_ram_bss_size  = SIZEOF(.dcache_ram_bss);
dcache_ram_bss_end    = dcache_ram_bss_begin + dcache_ram_bss_size;
ASSERT((dcache_ram_bss_size % 4) == 0,"!!! dcache_ram_bss_size Not Align 4 Bytes !!!");

dcache_ram_data_addr = ADDR(.dcache_ram_data);
dcache_ram_data_begin = ps_ram_data_code_begin + ps_ram_data_code_size;
dcache_ram_data_size =  SIZEOF(.dcache_ram_data);
ASSERT((dcache_ram_data_size % 4) == 0,"!!! dcache_ram_data_size Not Align 4 Bytes !!!");

//================ icache ==================//
icache_ram_bss_begin = ADDR(.icache_ram_bss);
icache_ram_bss_size  = SIZEOF(.icache_ram_bss);
icache_ram_bss_end    = icache_ram_bss_begin + icache_ram_bss_size;
ASSERT((icache_ram_bss_size % 4) == 0,"!!! icache_ram_bss_size Not Align 4 Bytes !!!");

icache_ram_data_code_addr = ADDR(.icache_ram_data_code);
icache_ram_data_code_begin = dcache_ram_data_begin + dcache_ram_data_size;
icache_ram_data_code_size =  SIZEOF(.icache_ram_data_code);
ASSERT((icache_ram_data_code_size % 4) == 0,"!!! icache_ram_data_code_size Not Align 4 Bytes !!!");


//================ BANK ==================//
bank_code_load_addr = icache_ram_data_code_begin + icache_ram_data_code_size;

/* moveable_addr = ADDR(.overlay_moveable) ; */
/* moveable_size = SIZEOF(.overlay_moveable) ; */
//===================== HEAP Info Export =====================//

ASSERT(CONFIG_FLASH_SIZE > text_size,"check sdk_config.h CONFIG_FLASH_SIZE < text_size");
ASSERT(_HEAP_BEGIN >= bss_begin,"_HEAP_BEGIN < bss_begin");
ASSERT(_HEAP_BEGIN >= data_addr,"_HEAP_BEGIN < data_addr");
ASSERT(_HEAP_BEGIN >= data_code_addr,"_HEAP_BEGIN < data_code_addr");
//ASSERT(_HEAP_BEGIN >= moveable_slot_addr,"_HEAP_BEGIN < moveable_slot_addr");
//ASSERT(_HEAP_BEGIN >= __report_overlay_begin,"_HEAP_BEGIN < __report_overlay_begin");

PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);

ASSERT(MALLOC_SIZE  >= 0x8000, "heap space too small !")

//============================================================//
//=== report section info begin:
//============================================================//
report_text_beign = ADDR(.text);
report_text_size = SIZEOF(.text);
report_text_end = report_text_beign + report_text_size;

report_mmu_tlb_begin = ADDR(.mmu_tlb);
report_mmu_tlb_size = SIZEOF(.mmu_tlb);
report_mmu_tlb_end = report_mmu_tlb_begin + report_mmu_tlb_size;

report_boot_info_begin = ADDR(.boot_info);
report_boot_info_size = SIZEOF(.boot_info);
report_boot_info_end = report_boot_info_begin + report_boot_info_size;

report_irq_stack_begin = ADDR(.irq_stack);
report_irq_stack_size = SIZEOF(.irq_stack);
report_irq_stack_end = report_irq_stack_begin + report_irq_stack_size;

report_data_begin = ADDR(.data);
report_data_size = SIZEOF(.data);
report_data_end = report_data_begin + report_data_size;

report_bss_begin = ADDR(.bss);
report_bss_size = SIZEOF(.bss);
report_bss_end = report_bss_begin + report_bss_size;

report_data_code_begin = ADDR(.data_code);
report_data_code_size = SIZEOF(.data_code);
report_data_code_end = report_data_code_begin + report_data_code_size;

report_overlay_begin = __report_overlay_begin;
report_overlay_size = __report_overlay_end - __report_overlay_begin;
report_overlay_end = __report_overlay_end;

report_heap_beign = _HEAP_BEGIN;
report_heap_size = _HEAP_END - _HEAP_BEGIN;
report_heap_end = _HEAP_END;

ps_ram_bss_addr = ADDR(.ps_ram_bss);
ps_ram_bss_size = SIZEOF(.ps_ram_bss);

ps_ram_noinit_addr = ADDR(.ps_ram_noinit);
ps_ram_noinit_size = SIZEOF(.ps_ram_noinit);

ps_ram_size = PSRAM_SIZE;

PROVIDE(PSRAM_HEAP_BEGIN = _PSRAM_HEAP_BEGIN);
PROVIDE(PSRAM_HEAP_END = _PSRAM_HEAP_END);
_PSRAM_MALLOC_SIZE = _PSRAM_HEAP_END - _PSRAM_HEAP_BEGIN;
PROVIDE(PSRAM_MALLOC_SIZE = _PSRAM_HEAP_END - _PSRAM_HEAP_BEGIN);


br33_PHY_RAM_SIZE = PHY_RAM_SIZE;
br33_SDK_RAM_SIZE = report_mmu_tlb_size + \
					report_boot_info_size + \
					report_irq_stack_size + \
					report_data_size + \
					report_bss_size + \
					report_overlay_size + \
					report_data_code_size + \
					report_heap_size;
//============================================================//
//=== report section info end
//============================================================//

