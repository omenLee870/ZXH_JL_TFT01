
@echo off

:: 定义源目录，假设 A 是根目录
set "sourceDir=.\ui_res"

:: 定义目标目录 B
set "targetDir=.\..\..\..\SDK\cpu\br33\tools\ui_lvgl_res"

:: 检查源目录是否存在
if not exist "%sourceDir%\" (
    echo 源目录不存在：%sourceDir%
    exit /b 1
)

:: 创建目标目录
if not exist "%targetDir%\" (
    mkdir "%targetDir%"
) else (
	del /q "%targetDir%\*.*"
)

:: 复制源目录中的所有文件到目标目录，并强制覆盖同名文件
echo 正在将所有文件复制到目标目录...
for /r "%sourceDir%" %%F in (*) do (
    :: 检查文件名是否为 copy_ui_res.bat，如果是则跳过复制操作
    if /I "%%~nxF"==".gitkeep" (
        echo 跳过复制文件：%%F
    ) else (
        :: 如果文件名不是 copy_ui_res.bat，则执行复制操作
        copy /y "%%F" "%targetDir%\"
    )
)

:: 定义动态页面原始的路径 C
set "dyn_SourceDir=.\dyn\page"
:: 定义动态页面目标存放的路径 D
set "dyn_targetDir=.\..\..\..\SDK\cpu\br33\tools\download\watch\"

:: 切换到SDK根目录，更新cbp文件
cd .\..\..\..\SDK
powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File update_cbp.ps1

exit /b 0
