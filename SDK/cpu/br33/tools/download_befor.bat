
@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\..\..\..\tools\prepare_reverse_notice.ps1"
if errorlevel 1 exit /b 1
@echo copy head

if exist "..\..\..\..\pmc\apps\bsp\cpu\br33\" (
    @echo find path
    copy ..\..\..\cpu\br33\msys_pmc_app.h ..\..\..\..\pmc\apps\bsp\cpu\br33\config\msys_pmc_app.h
    (
    echo #ifndef MSYS_SDK_CONFIG_H
    echo #define MSYS_SDK_CONFIG_H
    echo.
    findstr /R /C:"^#define SUB_CASE_TYPE_" ..\..\..\apps\watch\board\br33\board_config.h
    echo.
    findstr /C:"#define TCFG_SUB_CASE_TYPE" ..\..\..\apps\watch\board\br33\sdk_config.h
    echo.
    echo #endif
    ) > ..\..\..\cpu\br33\msys_sdk_config.h
    copy ..\..\..\cpu\br33\msys_sdk_config.h ..\..\..\..\pmc\apps\bsp\cpu\br33\config\msys_sdk_config.h
    del ..\..\..\cpu\br33\msys_sdk_config.h
)

if exist "..\..\..\..\loader\update_ui\apps\watch\board\br33\" (
    @echo find ota ui path
    powershell -Command "$sc=Get-Content -Encoding UTF8 ..\..\..\apps\watch\board\br33\sdk_config.h; $ui=$false; $ui_lines=@(); foreach($l in $sc){ if($l -match '^#define TCFG_UI_ENABLE\b'){$ui=$true}; if($ui){$ui_lines+=$l}; if($l -match '^#endif // TCFG_UI_ENABLE'){$ui=$false} }; [System.IO.File]::WriteAllLines('..\..\..\cpu\br33\msys_tmp_ui.h', $ui_lines)"
    (
    echo #ifndef MSYS_SDK_CONFIG_H
    echo #define MSYS_SDK_CONFIG_H
    echo.
    findstr /R /C:"^#define SUB_CASE_TYPE_" ..\..\..\apps\watch\board\br33\board_config.h
    echo.
    findstr /C:"#define TCFG_SUB_CASE_TYPE" ..\..\..\apps\watch\board\br33\sdk_config.h
    echo.
    findstr /C:"#define TCFG_PSRAM_DEV_ENABLE" ..\..\..\apps\watch\board\br33\sdk_config.h
    findstr /C:"#define TCFG_PSRAM_SIZE" ..\..\..\apps\watch\board\br33\sdk_config.h
    echo.
    findstr /C:"#define TCFG_CLOCK_OSC_HZ" ..\..\..\apps\watch\board\br33\sdk_config.h
    echo.
    type ..\..\..\cpu\br33\msys_tmp_ui.h
    echo.
    echo #endif
    ) > ..\..\..\cpu\br33\msys_sdk_config.h
    copy ..\..\..\cpu\br33\msys_sdk_config.h ..\..\..\..\loader\update_ui\apps\watch\board\br33\msys_sdk_config.h
    del ..\..\..\cpu\br33\msys_sdk_config.h
    del ..\..\..\cpu\br33\msys_tmp_ui.h
)

::pause

