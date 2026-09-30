

echo SDK BR33			

echo ${PWD}
cd ./download/watch/
echo ${PWD}


rm cfg_tool.bin
rm stream.bin
rm tone_en.cfg
rm tone_zh.cfg

cp ../../script.ver .
cp ../../uboot.boot .
cp ../../ota.bin .
cp ../../app.bin .
cp ../../br33loader.bin .
cp ../../isd_config.ini .
cp ../../p11_code.bin .
cp ../../flash_params_v2.bin .
cp ../../flash_params_v3.bin .
cp ../../json_watch_lvgl.txt json.txt
cp ../../cfg_tool.bin watch.bin


../../json_to_res json.txt

# remove and recreate ui_lvgl_res / ui_dial_res (if they exist)
rm -rf ui_lvgl_res
rm -rf ui_dial_res
mkdir ui_lvgl_res
cp ../../ui_lvgl_res/*.*   ./ui_lvgl_res/

# ============================================================
# Split dial resources (.dyn / .json) out of ui_lvgl_res and
# move them into ui_dial_res, so they go into watch.bin
# instead of the lvgl pack.
# ============================================================
HAS_DIAL_RES=$(find ui_lvgl_res -maxdepth 1 \( -name "*.dyn" -o -name "*.json" \) 2>/dev/null | head -n 1)

if [ -n "${HAS_DIAL_RES}" ]; then
    mkdir ui_dial_res
    find ui_lvgl_res -maxdepth 1 -name "*.dyn"  -exec mv -f {} ui_dial_res/ \;
    find ui_lvgl_res -maxdepth 1 -name "*.json" -exec mv -f {} ui_dial_res/ \;
fi

if [[ -n "$KEY_FILE_PATH" ]]; then
	echo find KEY_FILE_PATH
	KEY_FILE="-key ${KEY_FILE_PATH}"
else
	echo no find KEY_FILE_PATH
fi

if [[ -z "$PROJ_DOWNLOAD_PATH" ]]; then
	echo no find PROJ_DOWNLOAD_PATH
	PROJ_DOWNLOAD_PATH=../../../../../../output
fi

cp ${PROJ_DOWNLOAD_PATH}/*.bin .

cp ${PROJ_DOWNLOAD_PATH}/tone_en.cfg .
cp ${PROJ_DOWNLOAD_PATH}/tone_zh.cfg .

if [[ ${TONE_EN_ENABLE} -eq 1 ]]; then
	if [[ ! -f "tone_en.cfg" ]]; then
		cp ../../tone_en.cfg tone_en.cfg
	fi
    TONE_FILES="tone_en.cfg "
fi

if [[ ${TONE_ZH_ENABLE} -eq 1 ]]; then
	if [[ ! -f "tone_zh.cfg" ]]; then
		cp ../../tone_zh.cfg tone_zh.cfg
	fi
    TONE_FILES=${TONE_FILES}"tone_zh.cfg "
fi

if [[ ! -f "cfg_tool.bin" ]]; then
	cp ../../cfg_tool.bin cfg_tool.bin
fi
if [[ ! -f "stream.bin" ]]; then
	cp ../../stream.bin stream.bin
fi

if [[ ${FORMAT_ALL_ENABLE} -eq 1 ]]; then
	FORMAT="-format all"
elif [[ ${FORMAT_VM_ENABLE} -eq 1 ]]; then
	FORMAT="-format vm"
fi

../../md5sum app.bin md5.bin

../../packres -keep-suffix-case ui_lvgl_res -n lvgl -o lvgl

# ============================================================
# Pack watch1 / watch2 into watch.bin
# sources are in ui_dial_res/
# ============================================================
if [ -n "${HAS_DIAL_RES}" ]; then
    ../../packres -keep-suffix-case ui_dial_res/watch1.dyn ui_dial_res/watch1.json -n res -o watch1
    ../../packres -keep-suffix-case ui_dial_res/watch2.dyn ui_dial_res/watch2.json -n res -o watch2
    #../../packres -keep-suffix-case ui_dial_res/watch3.dyn ui_dial_res/watch3.json -n res -o watch3
    ../../fat_comm -pad-backup2 -force-align-fat -out watch.bin -image-size 8 -filelist watch1 watch2 -remove-empty -remove-bpb -mark-bad-after 0x528000  -address 0
fi

cp lvgl mode.bin

../../packres -n resfs -o resfs.bin -keep-suffix-case watch -aligned 4096 -normal

if [[ ${EXT_FLASH_ENABLE} -eq 1 ]]; then
    ../../fat_comm -pad-backup2 -force-align-fat -out ex_res.bin -image-size 16 -filelist dyn1 -remove-empty -remove-bpb -mark-bad-after 0x400000 -address 0
	../../packres -n res -o res.bin ex_res.bin 0 mode.bin 0x400000 -normal
    EX_FLASH_CMD="-ex_flash res.bin"
fi

CHIP_CMD="-tonorflash -dev br33 -div8 -wait 300 -uboot uboot.boot -app app.bin"
RES_FILE_CMD="-res cfg_tool.bin p11_code.bin stream.bin config.dat"
OUTPUT_CMD="-output-fw jl_isd.fw -output-ufw update.ufw"
FLASH_CMD="-flash-params flash_params_v3.bin"

echo ${CHIP_CMD}
echo ${TONE_FILES}
echo ${RES_FILE_CMD}
echo ${KEY_FILE}
echo ${FLASH_CMD}
echo ${OUTPUT_CMD}
echo ${FORMAT}

../../isd_download ${CHIP_CMD} -tone ${TONE_FILES} ${RES_FILE_CMD} ${KEY_FILE} ${EX_FLASH_CMD} ${FLASH_CMD} ${OUTPUT_CMD} ${FORMAT}




rm *.mp3 
rm *.PIX
rm *.TAB
rm *.res
rm *.sty
rm *.str
rm *.anim
rm *.view
rm *.json



cp update.ufw ${PROJ_DOWNLOAD_PATH}/update.ufw
cp jl_isd.bin ${PROJ_DOWNLOAD_PATH}/jl_isd.bin
cp jl_isd.fw ${PROJ_DOWNLOAD_PATH}/jl_isd.fw


