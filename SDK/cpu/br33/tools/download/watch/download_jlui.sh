

echo SDK BR33			

echo ${PWD}
cd ./download/watch/
echo ${PWD}


rm cfg_tool.bin
rm stream.bin
rm tone_en.cfg
rm tone_zh.cfg

mkdir ui_upgrade
mkdir sleepaid
mkdir fat2_dir
mkdir ui_res


cp ../../script.ver .
cp ../../uboot.boot .
cp ../../ota.bin .
cp ../../app.bin .
cp ../../br33loader.bin .
cp ../../isd_config.ini .
cp ../../p11_code.bin .
cp ../../user_api.bin .
cp ../../flash_params_v2.bin .
cp ../../flash_params_v3.bin .
cp ../../json.txt .

cp ../../ui_resource/*.*   .

cp ../../ui_upgrade/*.*   ./ui_upgrade/

cp ../../sleepaid/*.*   ./sleepaid/
cp ../../fat2_dir/*.*	./fat2_dir/


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


../../packres -keep-suffix-case JL.sty JL.res JL.str JL.tab -n JL -o JL
../../packres -keep-suffix-case F_UNIC.PIX F_ASCII.PIX -n font -o font
../../packres -keep-suffix-case watch.sty watch.res watch.str watch.json watch.view -n res -o watch
../../packres -keep-suffix-case watch1.sty watch1.res watch1.str watch1.json watch1.view -n res -o watch1
../../packres -keep-suffix-case watch2.sty watch2.res watch2.str watch2.json watch2.view -n res -o watch2
../../packres -keep-suffix-case watch3.sty watch3.res watch3.str watch3.json watch3.view -n res -o watch3


cat JL sidebar font > mode.bin

../../fat_comm -pad-backup2 -force-align-fat -out watch.bin -image-size 8 -filelist  watch watch1 -remove-empty -remove-bpb -mark-bad-after 0xD8000  -address 0


if [[ ${EXT_FLASH_ENABLE} -eq 1 ]]; then
    ../../fat_comm -pad-backup2 -force-align-fat -out ex_res.bin -image-size 16 -filelist watch1 -remove-empty -remove-bpb -mark-bad-after 0x1000000  -address 0
    ../../packres -n res -o res.bin ex_res.bin 0 mode.bin 0xa58000 -normal
    EX_FLASH_CMD="-ex_flash res.bin "
fi
if [[ ${DOUBLE_EXT_FLASH_ENABLE} -eq 1 ]]; then
    ../../fat_comm -pad-backup2 -force-align-fat -out ex_res.bin -image-size 16 -filelist watch1 -remove-empty -remove-bpb -mark-bad-after 0x1000000  -address 0
fi

if [[ ${NAND_FLASH_ENABLE} -eq 1 ]]; then
	cp JL ./ui_res/JL
	cp font ./ui_res/font
	cp watch ./ui_res/watch
	../../fat_image_tool --size 32M --sectors-per-cluster 0x40 --lfn true --n-root 512 --volume-name FAT2_IMG --output ex_res.bin --fat-dir ui_res
	../../fat_image_tool --size 90M --sectors-per-cluster 0x40 --lfn true --n-root 512 --volume-name FAT2_IMG --output jl_fat2.bin --fat-dir fat2_dir
	NANDFLASh_CFG=" -gen2 -output-nandflash-fw jl_isd.fw -output-nandflash-ufw update.ufw "
fi

CHIP_CMD="-tonorflash -dev br33 -div8 -wait 300 -uboot uboot.boot -app app.bin"
RES_FILE_CMD="-res sleepaid ui_upgrade cfg_tool.bin p11_code.bin stream.bin config.dat"
OUTPUT_CMD="-output-fw jl_isd.fw -output-ufw update.ufw"
FLASH_CMD="-flash-params flash_params_v3.bin"

echo ${CHIP_CMD}
echo ${TONE_FILES}
echo ${RES_FILE_CMD}
echo ${KEY_FILE}
echo ${FLASH_CMD}
echo ${OUTPUT_CMD}
echo ${FORMAT}

../../isd_download ${CHIP_CMD} -tone ${TONE_FILES} ${RES_FILE_CMD} ${KEY_FILE} -ex_api_bin user_api.bin ${EX_FLASH_CMD} ${FLASH_CMD} ${OUTPUT_CMD} ${NANDFLASh_CFG} ${FORMAT}




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


