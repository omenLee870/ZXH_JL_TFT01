@echo off
cd ../../../../

del /f /q cpu\br33\tools\sdk.elf 2>nul
rd /s /q objs 2>nul
exit /b 0
