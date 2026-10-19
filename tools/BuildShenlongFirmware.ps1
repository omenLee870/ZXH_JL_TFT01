param([string]$Make, [switch]$OneWire, [int]$Jobs = 8)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$sdkRoot = Join-Path $taskRoot 'SDK'
if (!$Make) { $Make = Join-Path $sdkRoot 'tools/utils/make.exe' }
if (!(Test-Path -LiteralPath $Make)) { throw 'Use -Make to specify GNU Make 4.x.' }
$env:Path = "$sdkRoot/tools/utils;C:/JL/pi32/bin;$env:Path"
$out = Join-Path $taskRoot 'output'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$mode = if ($OneWire) { 'one-wire' } else { 'shenlong-uart' }
$stamp = Join-Path $out 'firmware-build-mode.txt'
# GNU Make does not track compiler flag changes. Invalidate users of the board
# header when switching -D selection, so an old object cannot silently survive.
if (!(Test-Path -LiteralPath $stamp) -or [IO.File]::ReadAllText($stamp) -ne $mode) {
    (Get-Item -LiteralPath (Join-Path $sdkRoot 'apps/watch/board/br33/board_jl380n_demo/board_jl380n_demo_cfg.h')).LastWriteTime = Get-Date
}
Push-Location $sdkRoot
try {
    $argsList = @('-f','Makefile','-f','../tools/FirmwareCompileOnly.mk','compile-only',"-j$Jobs")
    if ($OneWire) { $argsList += 'EXT_CFLAGS=-DTCFG_SHENLONG_UART_ENABLE=0' }
    & $Make @argsList
    if ($LASTEXITCODE) { throw 'Firmware compilation/link failed. No download was executed.' }
    [IO.File]::WriteAllText($stamp, $mode)
} finally { Pop-Location }
