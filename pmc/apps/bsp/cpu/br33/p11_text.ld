
        *startup.o(.start_text)
		*(.text*)
		*(.*.text)
		*(.*.const)
    	*(.cmac_version)

        *(.ipc_spin_lock.text.cache.L1)
        *(.memory.text.cache.L1)
        *(.os_code)
        *(.os_const)
        *(.os_critical_code)
        *(.os.text.cache.L1)
        *(.volatile_ram_code)
        *(.os.text.str)
        *(.os_str)
        *(.cache)

        *(.*.text.cache.L1)
        *(.*.const.cache.L1)
        *(.*.text.const.cache.L1)

        *(.debug_code)
        *(.debug_const)

		. = ALIGN(4);
	    _lib_version_begin = .;
	    PROVIDE(lib_version_begin = .);
	    KEEP(*(.lib_version))
	    _lib_version_end = .;
	    PROVIDE(lib_version_end = .);

		. = ALIGN(4);
	    sdk_version_begin = .;
	    PROVIDE(sdk_version_begin = .);
	    KEEP(*(.sys.version))
	    sdk_version_end = .;
	    PROVIDE(sdk_version_end = .);

		. = ALIGN(4);
		_initcall_begin = .;
		PROVIDE(initcall_begin = .);
	    KEEP(*(.initcall))
		_initcall_end = .;
		PROVIDE(initcall_end = .);

		_early_initcall_begin = .;
		PROVIDE(early_initcall_begin = .);
		KEEP(*(.early.initcall))
		_early_initcall_end = .;
		PROVIDE(early_initcall_end = .);

		_late_initcall_begin = .;
		PROVIDE(late_initcall_begin = .);
		KEEP(*(.late.initcall))
		_late_initcall_end = .;
		PROVIDE(late_initcall_end = .);

		_platform_initcall_begin = .;
		PROVIDE(platform_initcall_begin = .);
		KEEP(*(.platform.initcall))
		_platform_initcall_end = .;
		PROVIDE(platform_initcall_end = .);

		_module_initcall_begin = .;
		PROVIDE(module_initcall_begin = .);
		KEEP(*(.module.initcall))
		_module_initcall_end = .;
		PROVIDE(module_initcall_end = .);

		platform_uninitcall_begin = .;
		KEEP(*(.platform.uninitcall))
		platform_uninitcall_end = .;

     
		. = ALIGN(4);
		sensor_dev_begin = .;
		KEEP(*(.sensor_dev))
		sensor_dev_end = .;

		. = ALIGN(4);
		sensor_hal_begin = .;
		KEEP(*(.sensor_hal))
		sensor_hal_end	 = .;

		. = ALIGN(4);
		sensor_service_begin = .;
		KEEP(*(.sensor_service))
		sensor_service_end	 = .;

		. = ALIGN(4);
		sport_health_module_begin = .;
		KEEP(*(.sport_health_module))
		sport_health_module_end = .;
		. = ALIGN(4);
        . = ALIGN(4);
        *(.rodata*)

//---------------------------------------------------------
		. = ALIGN(4);
		*(.power_driver.text)
		*(.power_driver.text.const)

		. = ALIGN(4);
		lp_target_begin = .;
	    PROVIDE(lp_target_begin = .);
	    KEEP(*(.lp_target))
	    lp_target_end = .;
	    PROVIDE(lp_target_end = .);

		. = ALIGN(4);
	    lp_request_begin = .;
	    PROVIDE(lp_request_begin = .);
	    KEEP(*(.lp_request))
	    lp_request_end = .;
	    PROVIDE(lp_request_end = .);

		. = ALIGN(4);
		p2m_msg_handler_begin = .;
		PROVIDE(p2m_msg_handler_begin = .);
		KEEP(*(.p2m_msg_handler))
		PROVIDE(p2m_msg_handler_end = .);
		p2m_msg_handler_end = .;

		. = ALIGN(4);
		PROVIDE(m2p_msg_handler_begin = .);
		KEEP(*(.m2p_msg_handler))
		PROVIDE(m2p_msg_handler_end = .);

		. = ALIGN(4);
		pdrv_text_cache_l2_begin = .;
        PROVIDE(pdrv_text_cache_l2_begin = .);

		deepsleep_target_begin = .;
        PROVIDE(deepsleep_target_begin = .);
        KEEP(*(.deepsleep_target))
        deepsleep_target_end = .;
        PROVIDE(deepsleep_target_end = .);
     	*(.power_driver.text.cache.fetch)

		pdrv_text_cache_l2_end = .;
        PROVIDE(pdrv_text_cache_l2_end = .);
//---------------------------------------------------------

