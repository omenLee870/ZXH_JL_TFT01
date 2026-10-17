param([string]$Compiler)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
if (!$Compiler) {
    $Compiler = [IO.Path]::GetFullPath((Join-Path $taskRoot '../../380tft/jluitool/JieLiGuiBuilder/resources/publish/tools/environment/mingw730_32/bin/gcc.exe'))
}
if (!(Test-Path -LiteralPath $Compiler)) { throw 'Use -Compiler to specify a native gcc.' }
$env:Path = "$(Split-Path -Parent $Compiler);$env:Path"
New-Item -ItemType Directory -Force -Path (Join-Path $taskRoot 'output') | Out-Null
$exe = Join-Path $taskRoot 'output/youyibao_protocol_test.exe'
& $Compiler -std=gnu11 -Wall -Wextra -Werror -I "$taskRoot/SDK/apps/watch/common" `
    "$taskRoot/SDK/apps/watch/common/youyibao_sif_protocol.c" `
    "$taskRoot/SDK/apps/watch/common/vehicle_core.c" `
    "$taskRoot/SDK/apps/watch/common/vehicle_storage.c" `
    "$taskRoot/tests/youyibao_sif/protocol_test.c" -o $exe
if ($LASTEXITCODE) { throw 'SIF protocol test compilation failed.' }
& $exe
if ($LASTEXITCODE) { throw 'SIF protocol tests failed.' }

$uiRoot = Join-Path $taskRoot 'ui_lvgl/dashboard_1024x600_v1'
$link = Join-Path $uiRoot 'lvgl-simulator/cmake-build-ui-mingw/CMakeFiles/main.dir/linklibs.rsp'
$env:Path = "$uiRoot/lvgl-simulator/bin;$env:Path"
$uiExe = Join-Path $taskRoot 'output/youyibao_ui_test.exe'
Push-Location $taskRoot
try {
    & $Compiler -std=gnu11 -Wno-attributes -DLV_CONF_INCLUDE_SIMPLE `
        -DLV_USE_GUIBUILDER_SIMULATOR=1 -DTCFG_YYB_SIF_ENABLE=1 `
        -DPET_REVERSE_NOTICE_TEST=1 -DVEHICLE_CONTROLLER_TEST=1 `
        -I "$uiRoot/lvgl" -I "$uiRoot/lvgl-simulator" -I "$uiRoot/generated" `
        -I "$uiRoot/custom" -I "$taskRoot/SDK/apps/watch/common" `
        -I "$taskRoot/SDK/apps/watch/wifi/ctp" `
        "$taskRoot/tests/youyibao_sif/ui_test.c" "$taskRoot/SDK/apps/watch/common/vehicle_core.c" `
        "$taskRoot/SDK/apps/watch/common/vehicle_controller.c" "@$link" -o $uiExe
    if ($LASTEXITCODE) { throw 'SIF UI test compilation failed.' }
    & $uiExe
    if ($LASTEXITCODE) { throw 'SIF UI tests failed.' }
} finally { Pop-Location }
