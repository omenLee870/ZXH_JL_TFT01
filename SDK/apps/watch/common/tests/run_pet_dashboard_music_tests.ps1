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
    & $Compiler -std=c11 -Wall -Wextra -Werror -DTONE_WAV_PSRAM_TEST=1 -DTCFG_PSRAM_DEV_ENABLE=1 `
        SDK/audio/interface/player/tone_wav_psram.c SDK/apps/watch/common/tests/tone_wav_psram_test.c `
        -o "$petOutput/wav-psram-test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'WAV PSRAM allocation test build failed.' }
    & "$petOutput/wav-psram-test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'WAV PSRAM allocation/release test failed.' }

    & $Compiler -std=c11 -Wall -Wextra -Werror -DPET_REVERSE_NOTICE_TEST=1 `
        -DPET_DASHBOARD_MUSIC_SIMULATOR=1 -DPET_DASHBOARD_BT_LYRICS_MODE=0 `
        -I SDK/apps/watch/common SDK/apps/watch/common/pet_reverse_notice.c `
        SDK/apps/watch/common/pet_dashboard_music.c SDK/apps/watch/common/tests/pet_reverse_notice_test.c `
        -o "$petOutput/reverse-notice-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '倒车播报测试编译失败。' }
    & "$petOutput/reverse-notice-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '倒车播报测试失败。' }

    # 普通模式保留既有歌曲信息和状态机检查。
    & $Compiler -std=c11 -Wall -Wextra -Werror -DPET_DASHBOARD_MUSIC_SIMULATOR=1 `
        -DPET_DASHBOARD_BT_LYRICS_MODE=0 `
        -I SDK/apps/watch/common SDK/apps/watch/common/pet_dashboard_music.c `
        SDK/apps/watch/common/tests/pet_dashboard_music_test.c -o "$petOutput/state-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '状态机测试编译失败。' }
    & "$petOutput/state-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '状态机测试失败。' }

    # 固定歌词配置保留原有边界检查。
    & $Compiler -std=c11 -Wall -Wextra -Werror -DPET_DASHBOARD_MUSIC_SIMULATOR=1 `
        -DPET_DASHBOARD_BT_LYRICS_MODE=1 -I SDK/apps/watch/common `
        SDK/apps/watch/common/pet_dashboard_music.c `
        SDK/apps/watch/common/tests/pet_dashboard_music_lyrics_test.c -o "$petOutput/lyrics-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '歌词模式测试编译失败。' }
    & "$petOutput/lyrics-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '歌词模式测试失败。' }

    # 不覆盖默认值，重放普通信息先到、歌词晚到及同曲信息交替补发。
    & $Compiler -std=c11 -Wall -Wextra -Werror -DPET_DASHBOARD_MUSIC_SIMULATOR=1 `
        -I SDK/apps/watch/common SDK/apps/watch/common/pet_dashboard_music.c `
        SDK/apps/watch/common/tests/pet_dashboard_music_metadata_test.c -o "$petOutput/metadata-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '混合元数据测试编译失败。' }
    & "$petOutput/metadata-test.exe"
    if ($LASTEXITCODE -ne 0) { throw '混合元数据测试失败。' }

    # 编译实际 BIP 接收文件，只替换线程锁和 SDK 命令出口。
    foreach ($petPsram in @(0, 1)) {
        & $Compiler -std=c11 -Wall -Wextra -Werror -DLV_USE_GUIBUILDER_SIMULATOR=0 `
            -DPET_DASHBOARD_BIP_TEST=1 "-DTCFG_PSRAM_DEV_ENABLE=$petPsram" `
            -I SDK/apps/watch/common -I SDK/apps/watch/common/tests `
            "$petUi/custom/ui_action_music_bg.c" SDK/apps/watch/common/tests/pet_dashboard_bip_test.c `
            -o "$petOutput/bip-test-$petPsram.exe"
        if ($LASTEXITCODE -ne 0) { throw "BIP 接收测试编译失败，PSRAM=$petPsram。" }
        & "$petOutput/bip-test-$petPsram.exe"
        if ($LASTEXITCODE -ne 0) { throw "BIP 接收测试失败，PSRAM=$petPsram。" }
    }

    # 联动测试使用真实 LVGL 控件、显示适配和小狗点击回调。
    & $Compiler -std=gnu11 -Wno-attributes -DLV_CONF_INCLUDE_SIMPLE `
        -DLV_USE_GUIBUILDER_SIMULATOR=1 -DPET_DASHBOARD_MUSIC_SIMULATOR=1 `
        -DPET_DASHBOARD_MUSIC_LINKED=1 -I "$petUi/lvgl" -I "$petUi/lvgl-simulator" `
        -I "$petUi/generated" -I "$petUi/custom" -I SDK/apps/watch/common `
        -I SDK/apps/common/ui/lvgl_v9/examples/porting `
        SDK/apps/watch/common/pet_dashboard_music.c SDK/apps/watch/common/tests/pet_dashboard_music_ui_test.c `
        "$petUi/custom/chongwuyibiao_yinyue.c" "$petUi/custom/chongwuyibiao_donghua.c" `
        "$petUi/custom/chongwuyibiao_yinyue_donghua.c" `
        "$petUi/custom/ui_pet_hero.c" `
        "@$petLink" -o "$petOutput/ui-test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'LVGL 联动测试编译失败。' }
    & "$petOutput/ui-test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'LVGL 联动测试失败。' }
}
finally {
    Pop-Location
    $env:Path = $petOldPath
}
