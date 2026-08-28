// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__

##!/bin/sh

#ifdef CONFIG_EXTRAM_CODE_ENABLE
${OBJDUMP} -D -address-mask=0x1fffffff -print-imm-hex -print-dbg -mcpu=r3 $1.elf > $1.lst
#else
${OBJDUMP} -D -address-mask=0xfffffff -print-imm-hex -print-dbg -mcpu=r3 $1.elf > $1.lst
#endif
${OBJCOPY} -O binary -j .startup_entry $1.elf startup_entry.bin
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data  $1.elf data.bin
${OBJCOPY} -O binary -j .data_code $1.elf data_code.bin
${OBJCOPY} -O binary -j .overlay_aec $1.elf aec.bin
${OBJCOPY} -O binary -j .overlay_aac $1.elf aac.bin
${OBJCOPY} -O binary -j .ps_ram_data_code $1.elf psr_data_code.bin
${OBJCOPY} -O binary -j .dcache_ram_data $1.elf d_ram_data.bin
${OBJCOPY} -O binary -j .icache_ram_data_code $1.elf i_ram_data_code.bin
${OBJCOPY} -O binary -j .pmc_ram_data_code $1.elf pmc_ram_data_code.bin

${OBJDUMP} -section-headers -address-mask=0x7ffffff $1.elf > segment_list.txt
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt


cat segment_list.txt

/opt/utils/report_segment_usage --sdk_path ${ROOT} \
--tbl_file symbol_tbl.txt \
--seg_file segment_list.txt \
--map_file $1.map \
--module_depth "{\"apps\":1,\"lib\":2,\"[lib]\":2}"

#ifdef BUILD_FW_APP_PROJ_ENABLE
cat text.bin data.bin > fw_app.bin
host-client -project ${NICKNAME}$2 -f fw_app.bin
exit 0
#endif

#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH)
case_suffix="_watch"
#elif (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_AI_GLASSES)
case_suffix="_ai_glasses"
#elif (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_DASHBOARD)
case_suffix="_dashboard"
#elif (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_KID_CAMERA)
case_suffix="_kid_camera"
#endif
cp pmc_code${case_suffix}.bin pmc_code.bin
cp p11_code${case_suffix}.bin p11_code.bin
cp p11_code${case_suffix}.lz4 p11_code.lz4


cat startup_entry.bin pmc_code.bin text.bin data.bin data_code.bin aec.bin aac.bin psr_data_code.bin d_ram_data.bin i_ram_data_code.bin pmc_ram_data_code.bin > app.bin

#ifndef CONFIG_RELEASE_ENABLE

/* if [ -f version ]; then */
    /* host-client -project ${NICKNAME}$2 -f app.bin version $1.elf p11_code.bin br28loader.bin br28loader.uart uboot.boot uboot.boot_debug ota.bin ota_debug.bin isd_config.ini */
/* else */
    /* host-client -project ${NICKNAME}$2 -f app.bin $1.elf  p11_code.bin br28loader.bin br28loader.uart uboot.boot uboot.boot_debug ota.bin ota_debug.bin isd_config.ini */

/* fi */

/* /opt/utils/strip-ini -i isd_config.ini -o isd_config.ini */

#if CPU_CORE_NUM > 1
bin_suffix="_d"
#else
bin_suffix="_s"
#endif


cp br33loader${bin_suffix}.bin br33loader.bin
cp br33loader${bin_suffix}.uart br33loader.uart
cp uboot${bin_suffix}.boot uboot.boot


if [ -d "${ROOT}/git_pmc/C51" ]; then
    pmc_path=${ROOT}/git_pmc/C51
	echo "find pmc path"

elif [ -d "${ROOT}/git_pmc" ]; then
    pmc_path=${ROOT}/git_pmc
	echo "find pmc path"

fi

if [ -e "$pmc_path" ]; then
	cp ${ROOT}/cpu/br33/msys_pmc_app.h ${pmc_path}/br33_pmc_q32f/apps/bsp/cpu/br33/config/
	cp ${ROOT}/interface/driver/cpu/br33/asm/power/pmc/pmc_mmap.h ${pmc_path}/br33_pmc_q32f/include/driver/cpu/br33/power/pmc/
	cp ${ROOT}/interface/driver/cpu/br33/asm/power/pmc/pmc_mmap.h ${pmc_path}/br33_pmc_q32s/include/driver/cpu/br33/power/pmc/
	{
	echo "#ifndef MSYS_SDK_CONFIG_H"
	echo "#define MSYS_SDK_CONFIG_H"
	echo ""
	sed -n "/^#define SUB_CASE_TYPE_/p" ${ROOT}/apps/watch/board/br33/board_config.h
	echo ""
	grep "#define TCFG_SUB_CASE_TYPE" ${ROOT}/apps/watch/board/br33/sdk_config.h
	echo ""
	echo "#endif"
	} > ${ROOT}/cpu/br33/msys_sdk_config.h
	cp ${ROOT}/cpu/br33/msys_sdk_config.h ${pmc_path}/br33_pmc_q32f/apps/bsp/cpu/br33/config/
	rm ${ROOT}/cpu/br33/msys_sdk_config.h
else
    echo -e "\n\033[31m"
    echo -e "ERROR : no find pmc_path"
    echo -e "\033[0m\n"
fi


if [ -d "${ROOT}/update_ui/loader_ui" ]; then
    ota_ui_path=${ROOT}/update_ui/loader_ui
	echo "find ota ui path"

elif [ -d "${ROOT}/update_ui" ]; then
    ota_ui_path=${ROOT}/update_ui
	echo "find ota ui path"

fi

if [ -e "$ota_ui_path" ]; then
	{
	echo "#ifndef MSYS_SDK_CONFIG_H"
	echo "#define MSYS_SDK_CONFIG_H"
	echo ""
	sed -n "/^#define SUB_CASE_TYPE_/p" ${ROOT}/apps/watch/board/br33/board_config.h
	echo ""
	grep "#define TCFG_SUB_CASE_TYPE" ${ROOT}/apps/watch/board/br33/sdk_config.h
	echo ""
	grep -E "#define TCFG_PSRAM_DEV_ENABLE|#define TCFG_PSRAM_SIZE" ${ROOT}/apps/watch/board/br33/sdk_config.h
	echo ""
	grep "#define TCFG_CLOCK_OSC_HZ" ${ROOT}/apps/watch/board/br33/sdk_config.h
	echo ""
	sed -n "/^#define TCFG_UI_ENABLE\b/,/^#endif \/\/ TCFG_UI_ENABLE/p" ${ROOT}/apps/watch/board/br33/sdk_config.h
	echo ""
	echo "#endif"
	} > ${ROOT}/cpu/br33/msys_sdk_config.h
	cp ${ROOT}/cpu/br33/msys_sdk_config.h ${ota_ui_path}/apps/watch/board/br33/
	rm ${ROOT}/cpu/br33/msys_sdk_config.h
else
    echo -e "\n\033[31m"
    echo -e "ERROR : no find ota ui path"
    echo -e "\033[0m\n"
fi

files="app.bin ota*.bin p11_code.bin p11_code${case_suffix}.bin isd_config.ini uboot.boot br33loader.uart br33loader.bin flash_params_v*.bin isd_download.exe"
/* files="app.bin br33loader.uart uboot.boot isd_config.ini" */


host-client -project ${NICKNAME}$2 -f ${files} $1.elf

#else /* #ifndef CONFIG_RELEASE_ENABLE */

#if TCFG_TONE_EN_ENABLE
TONE_EN_ENABLE=1
#else
TONE_EN_ENABLE=0
#endif

#if TCFG_TONE_ZH_ENABLE
TONE_ZH_ENABLE=1
#else
TONE_ZH_ENABLE=0
#endif


#if ((defined(TCFG_NORFLASH_SFC_DEV_ENABLE)) && TCFG_NORFLASH_SFC_DEV_ENABLE)
EXT_FLASH_ENABLE=1
#else
EXT_FLASH_ENABLE=0
#endif
#if ((defined(TCFG_NORFLASH_SFC_DEV1_ENABLE)) && TCFG_NORFLASH_SFC_DEV1_ENABLE)
DOUBLE_EXT_FLASH_ENABLE=1
#else
DOUBLE_EXT_FLASH_ENABLE=0
#endif

#if ((defined(TCFG_NANDFLASH_DEV_ENABLE)) && TCFG_NANDFLASH_DEV_ENABLE)
NAND_FLASH_ENABLE=1
#else
NAND_FLASH_ENABLE=0
#endif

cp user_api${case_suffix}.bin user_api.bin

cp cfg_tool${case_suffix}.bin cfg_tool.bin
cp stream${case_suffix}.bin stream.bin
cp tone_en${case_suffix}.cfg tone_en.cfg
cp tone_zh${case_suffix}.cfg tone_zh.cfg

source download_befor.sh

#if CONFIG_JL_UI_ENABLE
#if GPU_DEMO_ENABLE
source download/watch/download_jlui_demo.sh
#elif TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
source download/watch/download_jlui_csc.sh
#else
source download/watch/download_jlui.sh
#endif
#elif CONFIG_LVGL_UI_ENABLE
source download/watch/download_lvgl.sh
#else
source download/watch/download.sh
#endif

#endif /* #ifndef CONFIG_RELEASE_ENABLE */


#else // #ifdef __SHELL__

@echo off
@echo ********************************************************************************
@echo           SDK BR33
@echo ********************************************************************************
@echo %date%


cd /d %~dp0


set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJSIZEDUMP=C:\JL\pi32\bin\llvm-objsizedump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf
set LZ4_PACKET=.\lz4_packet.exe

#ifdef CONFIG_EXTRAM_CODE_ENABLE
%OBJDUMP% -D -address-mask=0x1fffffff -print-imm-hex -mcpu=r3 -print-dbg sdk.elf > sdk.lst
#else
%OBJDUMP% -D -address-mask=0xfffffff -print-imm-hex -mcpu=r3 -print-dbg sdk.elf > sdk.lst
#endif
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
REM %OBJDUMP% -t %ELFFILE% >  symbol_tbl.txt
%OBJSIZEDUMP% -lite -skip-zero -enable-dbg-info %ELFFILE% | sort >  symbol_tbl.txt


#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH)
set case_suffix=_watch
#elif (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_AI_GLASSES)
set case_suffix=_ai_glasses
#elif (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_DASHBOARD)
set case_suffix=_dashboard
#elif (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_KID_CAMERA)
set case_suffix=_kid_camera
#endif
copy pmc_code%case_suffix%.bin pmc_code.bin
copy p11_code%case_suffix%.bin p11_code.bin
copy p11_code%case_suffix%.lz4 p11_code.lz4


copy /b startup_entry.bin + pmc_code.bin + text.bin + data.bin + data_code.bin + aec.bin + aac.bin + psr_data_code.bin + d_ram_data.bin + i_ram_data_code.bin + pmc_ram_data_code.bin app.bin


#if TCFG_TONE_EN_ENABLE
set TONE_EN_ENABLE=1
#else
set TONE_EN_ENABLE=0
#endif

#if TCFG_TONE_ZH_ENABLE
set TONE_ZH_ENABLE=1
#else
set TONE_ZH_ENABLE=0
#endif


#if ((defined(TCFG_NORFLASH_SFC_DEV_ENABLE)) && TCFG_NORFLASH_SFC_DEV_ENABLE)
set EXT_FLASH_ENABLE=1
#else
set EXT_FLASH_ENABLE=0
#endif
#if ((defined(TCFG_NORFLASH_SFC_DEV1_ENABLE)) && TCFG_NORFLASH_SFC_DEV1_ENABLE)
set DOUBLE_EXT_FLASH_ENABLE=1
#else
set DOUBLE_EXT_FLASH_ENABLE=0
#endif


#if ((defined(TCFG_NANDFLASH_DEV_ENABLE)) && TCFG_NANDFLASH_DEV_ENABLE)
set NAND_FLASH_ENABLE=1
#else
set NAND_FLASH_ENABLE=0
#endif


#ifdef CONFIG_WATCH_CASE_ENABLE

copy user_api%case_suffix%.bin user_api.bin

copy cfg_tool%case_suffix%.bin cfg_tool.bin
copy stream%case_suffix%.bin stream.bin
copy tone_en%case_suffix%.cfg tone_en.cfg
copy tone_zh%case_suffix%.cfg tone_zh.cfg


call download_befor.bat

#if CONFIG_JL_UI_ENABLE
#if GPU_DEMO_ENABLE
call download/watch/download_jlui_demo.bat
#elif TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
call download/watch/download_jlui_csc.bat
#else
call download/watch/download_jlui.bat
#endif
#elif CONFIG_LVGL_UI_ENABLE
call download/watch/download_lvgl.bat
#else
call download/watch/download.bat
#endif
#elif (defined CONFIG_SERIAL_HMI_DISPLAY_CASE_ENABLE)
call download/serial_hmi_display/download_lvgl.bat
#elif (defined CONFIG_SOUNDBOX_CASE_ENABLE)
call download/soundbox/download.bat
#elif (defined CONFIG_EARPHONE_CASE_ENABLE)
call download/earphone/download.bat
#else // #ifdef CONFIG_WATCH_CASE_ENABLE
//to do other case
#endif  //endif app_case

#endif // #ifdef __SHELL__

