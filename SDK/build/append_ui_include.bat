@echo off
setlocal enabledelayedexpansion

set "INC_FILE=build\include_dir.txt"
if not exist "%INC_FILE%" (
    echo %INC_FILE% not found, creating...
    type nul > "%INC_FILE%"
)

set "INC1=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1"
set "INC2=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom"
set "INC3=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated"
set "INC4=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_events"
set "INC5=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_fonts"
set "INC6=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_group"
set "INC7=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_images"
set "INC8=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_msg"
set "INC9=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_res"
set "INC10=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr"
set "INC11=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_timelines"
set "INC12=-Iapps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/i18n"


findstr /x /c:"%INC1%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC1%>> "%INC_FILE%"
    echo Added %INC1%
)


findstr /x /c:"%INC2%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC2%>> "%INC_FILE%"
    echo Added %INC2%
)


findstr /x /c:"%INC3%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC3%>> "%INC_FILE%"
    echo Added %INC3%
)


findstr /x /c:"%INC4%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC4%>> "%INC_FILE%"
    echo Added %INC4%
)


findstr /x /c:"%INC5%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC5%>> "%INC_FILE%"
    echo Added %INC5%
)


findstr /x /c:"%INC6%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC6%>> "%INC_FILE%"
    echo Added %INC6%
)


findstr /x /c:"%INC7%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC7%>> "%INC_FILE%"
    echo Added %INC7%
)


findstr /x /c:"%INC8%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC8%>> "%INC_FILE%"
    echo Added %INC8%
)


findstr /x /c:"%INC9%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC9%>> "%INC_FILE%"
    echo Added %INC9%
)


findstr /x /c:"%INC10%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC10%>> "%INC_FILE%"
    echo Added %INC10%
)


findstr /x /c:"%INC11%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC11%>> "%INC_FILE%"
    echo Added %INC11%
)

findstr /x /c:"%INC12%" "%INC_FILE%" >nul 2>&1
if errorlevel 1 (
    echo %INC12%>> "%INC_FILE%"
    echo Added %INC12%
)

endlocal
exit /b 0
