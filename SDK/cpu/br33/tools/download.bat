











@echo off
@echo ********************************************************************************
@echo SDK BR33
@echo ********************************************************************************
@echo %date%


cd /d %~dp0


set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJSIZEDUMP=C:\JL\pi32\bin\llvm-objsizedump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf
set LZ4_PACKET=.\lz4_packet.exe




%OBJDUMP% -D -address-mask=0xfffffff -print-imm-hex -mcpu=r3 -print-dbg sdk.elf > sdk.lst

%OBJCOPY% -O binary -j .startup_entry %ELFFILE% startup_entry.bin
%OBJCOPY% -O binary -j .text %ELFFILE% text.bin
%OBJCOPY% -O binary -j .data %ELFFILE% data.bin
%OBJCOPY% -O binary -j .data_code %ELFFILE% data_code.bin
%OBJCOPY% -O binary -j .overlay_aec %ELFFILE% aec.bin
%OBJCOPY% -O binary -j .overlay_aac %ELFFILE% aac.bin
%OBJCOPY% -O binary -j .ps_ram_data_code %ELFFILE% psr_data_code.bin
%OBJCOPY% -O binary -j .dcache_ram_data %ELFFILE% d_ram_data.bin
%OBJCOPY% -O binary -j .icache_ram_data_code %ELFFILE% i_ram_data_code.bin
%OBJCOPY% -O binary -j .pmc_ram_data_code %ELFFILE% pmc_ram_data_code.bin

%OBJDUMP% -section-headers -address-mask=0x7ffffff %ELFFILE%
REM %OBJDUMP% -t %ELFFILE% > symbol_tbl.txt
%OBJSIZEDUMP% -lite -skip-zero -enable-dbg-info %ELFFILE% | sort > symbol_tbl.txt







set case_suffix=_dashboard



copy pmc_code%case_suffix%.bin pmc_code.bin
copy p11_code%case_suffix%.bin p11_code.bin
copy p11_code%case_suffix%.lz4 p11_code.lz4


copy /b startup_entry.bin + pmc_code.bin + text.bin + data.bin + data_code.bin + aec.bin + aac.bin + psr_data_code.bin + d_ram_data.bin + i_ram_data_code.bin + pmc_ram_data_code.bin app.bin



set TONE_EN_ENABLE=1





set TONE_ZH_ENABLE=1
set EXT_FLASH_ENABLE=0




set DOUBLE_EXT_FLASH_ENABLE=0






set NAND_FLASH_ENABLE=0





copy user_api%case_suffix%.bin user_api.bin

copy cfg_tool%case_suffix%.bin cfg_tool.bin
copy stream%case_suffix%.bin stream.bin
copy tone_en%case_suffix%.cfg tone_en.cfg
copy tone_zh%case_suffix%.cfg tone_zh.cfg


call download_befor.bat
if errorlevel 1 exit /b 1
call download/watch/download_lvgl.bat
