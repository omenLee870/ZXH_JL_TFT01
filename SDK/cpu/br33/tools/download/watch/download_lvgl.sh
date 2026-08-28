

echo SDK BR33			

echo ${PWD}
cd ./download/watch/
echo ${PWD}

rm -r ui_lvgl_res

mkdir ui_lvgl_res

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
cp ../../json.txt .
cp ../../cfg_tool.bin watch.bin


cp ../../ui_lvgl_res/*.*   ./ui_lvgl_res/


../../json_to_res json.txt

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

cp lvgl mode.bin

../../packres -n resfs -o resfs.bin -keep-suffix-case watch -aligned 4096 -normal

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

../../isd_download ${CHIP_CMD} -tone ${TONE_FILES} ${RES_FILE_CMD} ${KEY_FILE} ${FLASH_CMD} ${OUTPUT_CMD} ${FORMAT}




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


