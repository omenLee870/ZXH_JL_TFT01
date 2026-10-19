param([string]$Compiler, [string]$Python = 'python', [string]$DesktopDirectory)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
if (!$Compiler) {
    $Compiler = [IO.Path]::GetFullPath((Join-Path $taskRoot '../../380tft/jluitool/JieLiGuiBuilder/resources/publish/tools/environment/mingw730_32/bin/gcc.exe'))
}
if (!(Test-Path -LiteralPath $Compiler)) { throw 'Use -Compiler to specify a native gcc.' }
$env:Path = "$(Split-Path -Parent $Compiler);$env:Path"
New-Item -ItemType Directory -Force -Path (Join-Path $taskRoot 'output') | Out-Null
$exe = Join-Path $taskRoot 'output/shenlong_protocol_test.exe'
& $Compiler -std=gnu11 -Wall -Wextra -Werror -I "$taskRoot/SDK/apps/watch/common" `
    "$taskRoot/SDK/apps/watch/common/shenlong_protocol.c" `
    "$taskRoot/tests/shenlong/protocol_test.c" -o $exe
if ($LASTEXITCODE) { throw 'Shenlong protocol test compilation failed.' }
& $exe
if ($LASTEXITCODE) { throw 'Shenlong protocol tests failed.' }
$runtimeExe = Join-Path $taskRoot 'output/shenlong_runtime_test.exe'
& $Compiler -std=gnu11 -Wall -Wextra -Werror -I "$taskRoot/SDK/apps/watch/common" `
    "$taskRoot/SDK/apps/watch/common/shenlong_protocol.c" `
    "$taskRoot/SDK/apps/watch/common/vehicle_storage.c" `
    "$taskRoot/SDK/apps/watch/common/vehicle_core.c" `
    "$taskRoot/SDK/apps/watch/common/vehicle_test_control.c" `
    "$taskRoot/tests/shenlong/runtime_test.c" -o $runtimeExe
if ($LASTEXITCODE) { throw 'Shenlong runtime test compilation failed.' }
& $runtimeExe
if ($LASTEXITCODE) { throw 'Shenlong runtime tests failed.' }
$env:SHENLONG_TEST_EXE = $exe
& (Join-Path $PSScriptRoot 'TestVehicleControl.ps1') -Compiler $Compiler
$env:VEHICLE_CONTROL_TEST_EXE = Join-Path $taskRoot 'output/vehicle_control_test.exe'
if (!$DesktopDirectory) {
    $projectName = [string]::Concat([char]0x4e0a,[char]0x4f4d,[char]0x673a)
    $DesktopDirectory = Join-Path (Split-Path $taskRoot -Parent) $projectName
}
Push-Location $DesktopDirectory
try {
    & $Python -m unittest -v test_simulator test_canvas test_protocol_features test_control_channel
    if ($LASTEXITCODE) { throw 'Desktop simulator tests failed.' }
} finally { Pop-Location }
