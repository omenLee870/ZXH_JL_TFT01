

#ifdef CONFIG_AUDIO_ENABLE
		#include "media/media_lib_data_text.ld"
#endif

		*(.os_critical_code)
		*(.os.text*)

		*(.volatile_ram_code)
		*(.chargebox_code)
		*(.movable.stub.1)

		*(.*.text.cache.L1)
#ifndef CONFIG_OS_MEMORY_CODE_IN_FLASH
		*(.memory.text.cache.L1)
#endif
		*(.clock.text.cache.L1)
		*(.sfc.text.cache.L1)
		*(.movable.text.cache.L1)
		*(.usb.text.cache.L1)
		*(.gptimer.text.cache.L1)
		*(.ipc_spin_lock.text.cache.L1)

        *(.ui_ram)
        *(.math_fast_funtion_code)


		data_code_no_overlay_pc_limit_end = .;
        /* overlay的部分不允许放上面 */

         . = ALIGN(4);
         __fm_movable_slot_start = .;
         *(.movable.slot.1);
         __fm_movable_slot_end = .;

         __bt_movable_slot_start = .;
         *(.movable.slot.2);
         __bt_movable_slot_end = .;

         . = ALIGN(4);
         __aac_movable_slot_start = .;
         *(.movable.slot.3)
         __aac_movable_slot_end = .;

         . = ALIGN(4);
         __aec_movable_slot_start = .;
         *(.movable.slot.4)
         __aec_movable_slot_end = .;

		 . = ALIGN(4);
         __mic_eff_movable_slot_start = .;
         *(.movable.slot.5)
         __mic_eff_movable_slot_end = .;

         . = ALIGN(4);
        *(.movable.stub.1)
        *(.movable.stub.2)
        *(.movable.stub.3)
        *(.movable.stub.4)
        *(.movable.stub.5)

