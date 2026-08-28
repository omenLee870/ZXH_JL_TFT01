#include "app_config.h"

SECTIONS
{
    .text : ALIGN(4)
    {
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

        imusensor_dev_begin = .;
        KEEP(*(.imusensor_dev))
        imusensor_dev_end = .;

        fm_dev_begin = .;
        KEEP(*(.fm_dev))
        fm_dev_end = .;

        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;

        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

		. = ALIGN(4);
    	key_ops_begin = .;
        KEEP(*(.key_operation))
    	key_ops_end = .;

        . = ALIGN(4);
        key_callback_begin = .;
		KEEP(*(.key_cb))
        key_callback_end = .;

        . = ALIGN(4);
		_watch_syscfg_begin = .;
		PROVIDE(watch_syscfg_begin = .);
		KEEP(*(.watch_syscfg))
		_watch_syscfg_end = .;
		PROVIDE(watch_syscfg_end = .);

        . = ALIGN(4);
        flashdb_partition_begin = .;
		KEEP(*(.flashdb_partition))
        flashdb_partition_end = .;

		. = ALIGN(4);
        sport_health_module_begin = .;
		KEEP(*(.sport_health_module))
        sport_health_module_end = .;

		. = ALIGN(4);
        sensor_dev_begin = .;
		KEEP(*(.sensor_dev))
        sensor_dev_end = .;

		. = ALIGN(4);
        sensor_hal_begin = .;
		KEEP(*(.sensor_hal))
        sensor_hal_end = .;

		. = ALIGN(4);
        ui_effect_module_begin = .;
		KEEP(*(.ui_effect_module))
        ui_effect_module_end = .;

		. = ALIGN(4);
        charge_ic_begin = .;
        KEEP(*(.charge_ic))
        charge_ic_end = .;

		. = ALIGN(4);
	    pt_module_begin = .;
	    PROVIDE(pt_module_begin = .);
	    KEEP(*(.pt_module))
	    pt_module_end = .;
	    PROVIDE(pt_module_end = .);

		. = ALIGN(4);
        camera_device_module_begin = .;
        KEEP(*(.camera_device))
        camera_device_module_end = .;

		. = ALIGN(4);
        jlstm_tx_response_begin = .;
        KEEP(*(.jlstm_tx_response_operation))
        jlstm_tx_response_end = .;

		. = ALIGN(4);
        jlstm_rx_response_begin = .;
        KEEP(*(.jlstm_rx_response_operation))
        jlstm_rx_response_end = .;

		. = ALIGN(4);
        jlstm_rx_push_begin = .;
        KEEP(*(.jlstm_rx_push_operation))
        jlstm_rx_push_end = .;



		. = ALIGN(32);
    }

	.data ALIGN(32):
	{
	} > ram0

	.bss ALIGN(32):
    {
    } > ram0

	.data_code ALIGN(32):
	{
	} > ram0

}
