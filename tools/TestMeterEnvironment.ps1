param([string]$Compiler)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
if (!$Compiler) { $Compiler = [IO.Path]::GetFullPath((Join-Path $taskRoot '../../380tft/jluitool/JieLiGuiBuilder/resources/publish/tools/environment/mingw730_32/bin/gcc.exe')) }
if (!(Test-Path -LiteralPath $Compiler)) { throw 'Use -Compiler to specify a native gcc.' }
$uiRoot = Join-Path $taskRoot 'ui_lvgl/dashboard_1024x600_v1'
$env:Path = "$(Split-Path -Parent $Compiler);$uiRoot/lvgl-simulator/bin;$env:Path"
$link = Join-Path $uiRoot 'lvgl-simulator/cmake-build-ui-mingw/CMakeFiles/main.dir/linklibs.rsp'
$exe = Join-Path $taskRoot 'output/environment_ui_test.exe'
New-Item -ItemType Directory -Force -Path (Join-Path $taskRoot 'output') | Out-Null
Push-Location $taskRoot
try {
    & $Compiler -std=gnu11 -Wno-attributes -DLV_CONF_INCLUDE_SIMPLE -DLV_USE_GUIBUILDER_SIMULATOR=1 `
        -I "$uiRoot/lvgl" -I "$uiRoot/lvgl-simulator" -I "$uiRoot/generated" -I "$uiRoot/custom" `
        -I "$taskRoot/SDK/apps/watch/common" -I "$taskRoot/SDK/apps/watch/include" `
        -I "$taskRoot/SDK/apps/watch/wifi/ctp" "$taskRoot/tests/environment/ui_test.c" "@$link" -o $exe
    if ($LASTEXITCODE) { throw 'Environment UI test compilation failed.' }
    & $exe
    if ($LASTEXITCODE) { throw 'Environment tests failed.' }
} finally { Pop-Location }
