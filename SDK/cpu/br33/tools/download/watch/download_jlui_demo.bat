@echo off

@echo ********************************************************************************
@echo 			SDK BR33			
@echo ********************************************************************************
@echo %date%

cd %~dp0

mkdir ui_upgrade
mkdir sleepaid


copy ..\..\script.ver .
copy ..\..\uboot.boot .
copy ..\..\ota.bin .
copy ..\..\cfg_tool.bin .
copy ..\..\app.bin .
copy ..\..\br33loader.bin .
copy ..\..\isd_config.ini .
copy ..\..\p11_code.bin .
copy ..\..\stream.bin .
copy ..\..\user_api.bin .
copy ..\..\flash_params_v2.bin .
copy ..\..\flash_params_v3.bin .
copy ..\..\json.txt .

copy ..\..\ui_resource\*.*   .

copy ..\..\ui_upgrade\*.*   .\ui_upgrade\

copy ..\..\sleepaid\*.*   .\sleepaid\

copy ..\..\gpu_demo\*.*   .

..\..\json_to_res.exe json.txt

if not %KEY_FILE_PATH%A==A set KEY_FILE=-key %KEY_FILE_PATH%

if %PROJ_DOWNLOAD_PATH%A==A set PROJ_DOWNLOAD_PATH=..\..\..\..\..\..\output
copy %PROJ_DOWNLOAD_PATH%\*.bin .
if exist %PROJ_DOWNLOAD_PATH%\tone_en.cfg copy %PROJ_DOWNLOAD_PATH%\tone_en.cfg .
if exist %PROJ_DOWNLOAD_PATH%\tone_zh.cfg copy %PROJ_DOWNLOAD_PATH%\tone_zh.cfg .

if %TONE_EN_ENABLE%A==1A (
    if not exist tone_en.cfg copy ..\..\tone_en.cfg tone_en.cfg
    set TONE_FILES=tone_en.cfg
)
if %TONE_ZH_ENABLE%A==1A (
    if not exist tone_zh.cfg copy ..\..\tone_zh.cfg tone_zh.cfg
    set TONE_FILES=%TONE_FILES% tone_zh.cfg
)

if %FORMAT_VM_ENABLE%A==1A set FORMAT=-format vm
if %FORMAT_ALL_ENABLE%A==1A set FORMAT=-format all



..\..\packres.exe -keep-suffix-case JL.sty JL.res JL.str JL.tab -n JL -o JL
::..\..\packres.exe -keep-suffix-case sidebar.sty sidebar.res sidebar.str sidebar.tab -n sidebar -o sidebar
..\..\packres.exe -keep-suffix-case F_UNIC.PIX F_ASCII.PIX -n font -o font
..\..\packres.exe -keep-suffix-case pic.bin rate.bin jpg_test.jpg -n demo -o demo
..\..\packres.exe -keep-suffix-case watch.sty watch.res watch.str watch.json watch.view -n res -o watch
..\..\packres.exe -keep-suffix-case watch1.sty watch1.res watch1.str watch1.json watch1.view -n res -o watch1
..\..\packres.exe -keep-suffix-case watch2.sty watch2.res watch2.str watch2.json watch2.view -n res -o watch2
..\..\packres.exe -keep-suffix-case watch3.sty watch3.res watch3.str watch3.json watch3.view -n res -o watch3
..\..\packres.exe -keep-suffix-case ui.res ui.str -n res -o ui

copy /B JL+sidebar+font+demo mode.bin

..\..\fat_comm.exe -pad-backup2 -force-align-fat -out watch.bin -image-size 8 -filelist ui -remove-empty -remove-bpb -mark-bad-after 0xD8000  -address 0


set CHIP_CMD=-tonorflash -dev br33 -boot 0x7f02600 -div8 -wait 300 -uboot uboot.boot -app app.bin
set RES_FILE_CMD=-res sleepaid ui_upgrade cfg_tool.bin p11_code.bin stream.bin config.dat
set OUTPUT_CMD=-output-fw jl_isd.fw -output-ufw update.ufw
set FLASH_CMD=-flash-params flash_params_v3.bin

..\..\isd_download.exe %CHIP_CMD% -tone %TONE_FILES% %RES_FILE_CMD% %KEY_FILE% -ex_api_bin user_api.bin %FLASH_CMD% %OUTPUT_CMD% %FORMAT%




@rem 删除临时文件
if exist *.mp3 del *.mp3 
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty
if exist *.str del *.str
if exist *.anim del *.anim
if exist *.view del *.view
if exist *.json del *.json



copy update.ufw %PROJ_DOWNLOAD_PATH%\update.ufw
copy jl_isd.bin %PROJ_DOWNLOAD_PATH%\jl_isd.bin
copy jl_isd.fw %PROJ_DOWNLOAD_PATH%\jl_isd.fw


@rem 常用命令说明
@rem -format vm        //擦除VM 区域
@rem -format cfg       //擦除BT CFG 区域
@rem -format 0x3f0-2   //表示从第 0x3f0 个 sector 开始连续擦除 2 个 sector(第一个参数为16进制或10进制都可，第二个参数必须是10进制)

ping /n 2 127.1>null
IF EXIST null del null
::pause
