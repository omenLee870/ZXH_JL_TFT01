@echo off

@echo ********************************************************************************
@echo 			SDK BR33			
@echo ********************************************************************************
@echo %date%

cd %~dp0

del cfg_tool.bin
del stream.bin
del tone_en.cfg
del tone_zh.cfg

@rem 检查并删除 ui_lvgl_res、ui_dial_res 文件夹（如果存在）
if exist ui_lvgl_res (
    rmdir /s /q ui_lvgl_res
)
if exist ui_dial_res (
    rmdir /s /q ui_dial_res
)

mkdir ui_lvgl_res

copy ..\..\script.ver .
copy ..\..\uboot.boot .
copy ..\..\ota.bin .
copy ..\..\app.bin .
copy ..\..\br33loader.bin .
copy ..\..\isd_config.ini .
copy ..\..\p11_code.bin .
copy ..\..\flash_params_v2.bin .
copy ..\..\flash_params_v3.bin .
copy ..\..\json.txt json.txt
copy ..\..\cfg_tool.bin watch.bin

@rem 先把外部 ui_lvgl_res 资源全部复制进来
copy ..\..\ui_lvgl_res\*.*   .\ui_lvgl_res\

@rem ============================================================
@rem 检测 ui_lvgl_res 下是否有 .dyn / .json 文件
@rem 有则创建 ui_dial_res，并以剪切方式移入
@rem ============================================================
set HAS_DIAL_RES=
for %%F in (ui_lvgl_res\*.dyn ui_lvgl_res\*.json) do (
    if exist "%%F" set HAS_DIAL_RES=1
)

if defined HAS_DIAL_RES (
    mkdir ui_dial_res
    move /y ui_lvgl_res\*.dyn ui_dial_res\ >nul 2>nul
    move /y ui_lvgl_res\*.json ui_dial_res\ >nul 2>nul
    copy ..\..\json_watch_lvgl.txt json.txt
)


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

if not exist cfg_tool.bin copy ..\..\cfg_tool.bin cfg_tool.bin
if not exist stream.bin copy ..\..\stream.bin stream.bin

if %FORMAT_VM_ENABLE%A==1A set FORMAT=-format vm
if %FORMAT_ALL_ENABLE%A==1A set FORMAT=-format all

..\..\md5sum.exe app.bin md5.bin

..\..\packres.exe -keep-suffix-case ui_lvgl_res -n lvgl -o lvgl

@rem ============================================================
@rem 仅当 HAS_DIAL_RES 存在时，才执行表盘资源打包与 watch.bin 生成
@rem ============================================================
if defined HAS_DIAL_RES (
    ..\..\packres.exe -keep-suffix-case ui_dial_res\watch1.dyn ui_dial_res\watch1.json -n res -o watch1
    ..\..\packres.exe -keep-suffix-case ui_dial_res\watch2.dyn ui_dial_res\watch2.json -n res -o watch2
    ::..\..\packres.exe -keep-suffix-case ui_dial_res\watch3.dyn ui_dial_res\watch3.json -n res -o watch3
    ..\..\fat_comm.exe -pad-backup2 -force-align-fat -out watch.bin -image-size 8 -filelist watch1 watch2 -remove-empty -remove-bpb -mark-bad-after 0x528000  -address 0
)

copy /B lvgl mode.bin
..\..\packres.exe -n resfs -o resfs.bin -keep-suffix-case watch -aligned 4096 -normal

if %EXT_FLASH_ENABLE%A==1A (
    ..\..\fat_comm.exe -pad-backup2 -force-align-fat -out ex_res.bin -image-size 16 -filelist dyn1 -remove-empty -remove-bpb -mark-bad-after 0x400000 -address 0
	..\..\packres.exe -n res -o res.bin ex_res.bin 0 mode.bin 0x400000 -normal
    set EX_FLASH_CMD=-ex_flash res.bin
)

::set CHIP_CMD=-tonorflash -dev br33 -boot 0x7f02600 -div8 -wait 300 -uboot uboot.boot -app app.bin
set CHIP_CMD=-tonorflash -dev br33 -div8 -wait 300 -uboot uboot.boot -app app.bin
set RES_FILE_CMD=-res cfg_tool.bin p11_code.bin stream.bin config.dat
set OUTPUT_CMD=-output-fw jl_isd.fw -output-ufw update.ufw
set FLASH_CMD=-flash-params flash_params_v3.bin

..\..\isd_download.exe %CHIP_CMD% -tone %TONE_FILES% %RES_FILE_CMD% %KEY_FILE% %EX_FLASH_CMD% %FLASH_CMD% %OUTPUT_CMD% %FORMAT%

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