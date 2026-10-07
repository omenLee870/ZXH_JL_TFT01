@echo off
if exist "CMakeLists.txt" (
	DEL /Q "CMakeLists.txt"
)
if exist "CMakeListsUI.txt" (
	COPY CMakeListsUI.txt CMakeLists.txt
)
if exist "bin\main.exe" (
	DEL /Q "bin\main.exe"
)

cmake -G "CodeBlocks - MinGW Makefiles" -B cmake-build-ui-mingw
cmake.exe --build cmake-build-ui-mingw -j 8
