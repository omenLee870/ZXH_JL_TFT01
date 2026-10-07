@echo off

DEL /Q "bin\liblvgl.a"
DEL /Q "bin\libdrv.a"
DEL /Q "bin\libmain.dll.a"
DEL /Q "bin\main.exe"
DEL /Q "CMakeLists.txt"
RD /S /Q "..\generated"
RD /S /Q "..\import\bin"

RD /S /Q "cmake-build-lvgl-mingw"
RD /S /Q "cmake-build-drv-mingw"
RD /S /Q "cmake-build-ui-mingw"
RD /S /Q "board"
