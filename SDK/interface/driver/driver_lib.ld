SECTIONS
{
    .data ALIGN(4):SUBALIGN(4)
    {
        driver_data_start = .;

        CLOCK_DATA_START = .;
        *(.clock_data)
        CLOCK_DATA_SIZE = ABSOLUTE(. - CLOCK_DATA_START);

        *(.debug_data)

        *(.uart_data)

        *(.power_driver.data)

        driver_data_end = .;
        . = ALIGN(32);
    } > ram0

    .data_code ALIGN(4):SUBALIGN(4)
    {
        *(.power_driver.text.cache.L1)

    } > ram0

    .bss ALIGN(4):SUBALIGN(4)
    {
        driver_bss_start = .;

        CLOCK_BSS_START = .;
        *(.clock_bss)
        CLOCK_BSS_SIZE = ABSOLUTE(. - CLOCK_BSS_START);

        *(.debug_bss)

        *(.power_driver.data.bss)

        *(.uart_bss)

        *(.sd_var)

        . = ALIGN(4);
        driver_bss_end = .;
    } > ram0

    .text  ALIGN(4):SUBALIGN(4)
    {
        driver_code_start = .;

        *(.fm_code)

        CLOCK_CODE_START = .;
        *(.clock_code)
        *(.clock_const)
        CLOCK_CODE_SIZE = ABSOLUTE(. - CLOCK_CODE_START);

        *(.debug_code)
        *(.debug_const)

        . = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
        *(.sfc.text.cache.L2*)
        . = ALIGN(4);
        _SPI_CODE_END = . ;
        . = ALIGN(4);

        _TZSPI_CODE_START = . ;
        *(.tzflash_text_cache)
        . = ALIGN(4);
        _TZSPI_CODE_END = . ;
        . = ALIGN(4);

        . = ALIGN(4);
        _SFC_TRIM_CODE_START = . ;
        *(.sfc_trim_code)
        . = ALIGN(4);
        _SFC_TRIM_CODE_END = . ;

        . = ALIGN(4);
        tzspi_target_begin = .;
        PROVIDE(tzspi_target_begin = .);
        KEEP(*(.tzspi_target))
        tzspi_target_end = .;
        PROVIDE(tzspi_target_end = .);

        . = ALIGN(4);
        *(.hsafe.code*)

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

        . = ALIGN(4);
        sleep_target_begin = .;
        PROVIDE(sleep_target_begin = .);
        KEEP(*(.sleep_target))
        sleep_target_end = .;
        PROVIDE(sleep_target_end = .);

        . = ALIGN(4);
        p2m_msg_handler_begin = .;
        PROVIDE(p2m_msg_handler_begin = .);
        KEEP(*(.p2m_msg_handler))
        PROVIDE(p2m_msg_handler_end = .);
        p2m_msg_handler_end = .;

        . = ALIGN(4);
        phw_begin = .;
        PROVIDE(phw_begin = .);
        KEEP(*(.phw_operation))
        PROVIDE(phw_end = .);
        phw_end = .;

        . = ALIGN(4);
        __usb_class_start = .;
        PROVIDE(__usb_class_start = .);
        KEEP(*(.usb.class))
        PROVIDE(__usb_class_end = .);
        __usb_class_end = .;

        . = ALIGN(4);
        __usb_host_class_start = .;
        PROVIDE(__usb_host_class_start = .);
        KEEP(*(.usb.host.class))
        PROVIDE(__usb_host_class_end = .);
        __usb_host_class_end = .;

     . = ALIGN(4);
        *(.uart_code)
        *(.uart_const)

        driver_code_end = .;
        . = ALIGN(4);
    } > code0

    /*代码统计 Code & RAM */
    DRIVER_RAM_TOTAL   = (driver_data_end - driver_data_start) + (driver_bss_end - driver_bss_start);
    DRIVER_CODE_TOTAL  = (driver_code_end - driver_code_start);
}
