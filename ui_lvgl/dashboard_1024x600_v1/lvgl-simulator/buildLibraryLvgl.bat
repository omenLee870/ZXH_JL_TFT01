if exist "CMakeLists.txt" (
	DEL /Q "CMakeLists.txt"
)
if exist "CMakeListsLvgl.txt" (
	COPY CMakeListsLvgl.txt CMakeLists.txt
)
if exist "bin\liblvgl.a" (
	DEL /Q "bin\liblvgl.a"
)

cmake -G "CodeBlocks - MinGW Makefiles" -B cmake-build-lvgl-mingw
cmake.exe --build cmake-build-lvgl-mingw -j 16

if not exist bin (
    mkdir bin
)
COPY cmake-build-lvgl-mingw\liblvgl.a bin\liblvgl.a

if exist "CMakeLists.txt" (
	DEL /Q "CMakeLists.txt"
)
