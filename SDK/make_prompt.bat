SET PROJ_DOWNLOAD_PATH=%1
SET SCRIPT_PATH=%~dp0%
set PATH=%SCRIPT_PATH%tools\utils;C:\Windows\System32;%PATH%

@fc %PROJ_DOWNLOAD_PATH%\sdk_config.c apps\watch\board\br33\sdk_config.c
@if errorlevel==1 copy %PROJ_DOWNLOAD_PATH%\sdk_config.c apps\watch\board\br33\sdk_config.c
@fc %PROJ_DOWNLOAD_PATH%\sdk_config.h apps\watch\board\br33\sdk_config.h
@if errorlevel==1 copy %PROJ_DOWNLOAD_PATH%\sdk_config.h apps\watch\board\br33\sdk_config.h


