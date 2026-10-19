param([string]$Compiler)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
if (!$Compiler) { $Compiler = [IO.Path]::GetFullPath((Join-Path $taskRoot '../../380tft/jluitool/JieLiGuiBuilder/resources/publish/tools/environment/mingw730_32/bin/gcc.exe')) }
$env:Path = "$(Split-Path -Parent $Compiler);$env:Path"
$exe = Join-Path $taskRoot 'output/vehicle_control_test.exe'
& $Compiler -std=gnu11 -Wall -Wextra -Werror -I "$taskRoot/SDK/apps/watch/common" `
    "$taskRoot/SDK/apps/watch/common/vehicle_core.c" "$taskRoot/SDK/apps/watch/common/shenlong_protocol.c" `
    "$taskRoot/SDK/apps/watch/common/vehicle_test_control.c" "$taskRoot/tests/shenlong/control_test.c" -o $exe
if ($LASTEXITCODE) { throw 'AT control compilation failed.' }
& $exe
if ($LASTEXITCODE) { throw 'AT control tests failed.' }
