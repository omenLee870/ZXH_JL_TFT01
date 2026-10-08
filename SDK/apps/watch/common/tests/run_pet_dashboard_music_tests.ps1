param([string]$Compiler)

$ErrorActionPreference = 'Stop'
$petRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../..'))
if (-not $Compiler) {
    $Compiler = [System.IO.Path]::GetFullPath((Join-Path $petRoot '../../380tft/jluitool/JieLiGuiBuilder/resources/publish/tools/environment/mingw730_32/bin/gcc.exe'))
}
if (-not (Test-Path -LiteralPath $Compiler)) { throw '找不到 MinGW gcc，请通过 -Compiler 指定路径。' }
$petUi = Join-Path $petRoot 'ui_lvgl/dashboard_1024x600_v1'
$petOutput = Join-Path $petRoot 'output/pet_music_verify'
$petLink = Join-Path $petUi 'lvgl-simulator/cmake-build-ui-mingw/CMakeFiles/main.dir/linklibs.rsp'
if (-not (Test-Path -LiteralPath $petLink)) { throw '请先在 JieLiUI 编译一次预览程序。' }
New-Item -ItemType Directory -Path $petOutput -Force | Out-Null
$petOldPath = $env:Path
$env:Path = "$(Split-Path -Parent $Compiler);$petUi/lvgl-simulator/bin;$env:Path"
Push-Location $petRoot
try {
    # 状态机测试使用真实应用源码，蓝牙输入由测试提供。
    & $Compiler -std=c11 -Wall -Wextra -Werror -DPET_DASHBOARD_MUSIC_SIMULATOR=1 `
        -I SDK/apps/watch/common SDK/apps/watch/common/pet_dashboard_music.c `
        SDK/apps/watch/common/tests/pet_dashboard_music_test.c -o "$petOutput/state-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '状态机测试编译失败。' }
    & "$petOutput/state-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '状态机测试失败。' }

    # 联动测试使用真实 LVGL 控件、显示适配和小狗点击回调。
    & $Compiler -std=gnu11 -Wno-attributes -DLV_CONF_INCLUDE_SIMPLE `
        -DLV_USE_GUIBUILDER_SIMULATOR=1 -DPET_DASHBOARD_MUSIC_SIMULATOR=1 `
        -DPET_DASHBOARD_MUSIC_LINKED=1 -I "$petUi/lvgl" -I "$petUi/lvgl-simulator" `
        -I "$petUi/generated" -I "$petUi/custom" -I SDK/apps/watch/common `
        SDK/apps/watch/common/pet_dashboard_music.c SDK/apps/watch/common/tests/pet_dashboard_music_ui_test.c `
        "$petUi/custom/chongwuyibiao_yinyue.c" "$petUi/custom/chongwuyibiao_donghua.c" `
        "$petUi/custom/chongwuyibiao_yinyue_donghua.c" `
        "@$petLink" -o "$petOutput/ui-test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'LVGL 联动测试编译失败。' }
    & "$petOutput/ui-test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'LVGL 联动测试失败。' }
}
finally {
    Pop-Location
    $env:Path = $petOldPath
}
