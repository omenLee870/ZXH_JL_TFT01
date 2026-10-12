param(
    [Parameter(Mandatory=$true)][string]$Archive,
    [Parameter(Mandatory=$true)][string]$Output,
    [Parameter(Mandatory=$true)][string]$Compiler
)

$ErrorActionPreference = 'Stop'
$wavArchive = [System.IO.Path]::GetFullPath($Archive)
$wavOutput = [System.IO.Path]::GetFullPath($Output)
$wavTools = Split-Path -Parent $Compiler
$wavAr = Join-Path $wavTools 'llvm-ar.exe'
$wavRename = Join-Path $wavTools 'llvm-bcrename.exe'
$wavNm = Join-Path $wavTools 'llvm-nm.exe'
foreach ($wavTool in @($wavAr, $wavRename, $wavNm)) {
    if (-not (Test-Path -LiteralPath $wavTool)) { throw "Missing tool: $wavTool" }
}

# Work on an extracted copy; never alter the vendor archive. The same symbol
# wav_dec_plug is linked before media.a, so its original member is not loaded.
$wavWork = Join-Path (Split-Path -Parent $wavOutput) 'wav_psram_work'
New-Item -ItemType Directory -Path $wavWork -Force | Out-Null
Push-Location $wavWork
try {
    & $wavAr x $wavArchive wav_dec.c.o
    if ($LASTEXITCODE -ne 0) { throw 'WAV adapter extraction failed.' }
    $wavInput = Join-Path $wavWork 'wav_dec.c.o'
    $wavSymbols = & $wavNm $wavInput
    if ($LASTEXITCODE -ne 0) { throw 'WAV adapter symbol inspection failed.' }
    foreach ($wavName in @('malloc', 'zalloc', 'free', 'wav_dec_plug')) {
        if (-not ($wavSymbols | Select-String -Pattern "\s$wavName`$")) {
            throw "Vendor WAV adapter changed: missing $wavName"
        }
    }
    $wavTemp = "$wavOutput.tmp"
    & $wavRename $wavInput "-o=$wavTemp" '-rename-list=malloc:tone_wav_psram_malloc,zalloc:tone_wav_psram_zalloc,free:tone_wav_psram_free'
    if ($LASTEXITCODE -ne 0) { throw 'WAV allocator redirection failed.' }
    $wavSymbols = & $wavNm $wavTemp
    if ($LASTEXITCODE -ne 0) { throw 'PSRAM WAV adapter symbol inspection failed.' }
    foreach ($wavName in @('tone_wav_psram_malloc', 'tone_wav_psram_zalloc', 'tone_wav_psram_free')) {
        if (-not ($wavSymbols | Select-String -Pattern "\s$wavName`$")) {
            throw "PSRAM allocator reference missing: $wavName"
        }
    }
    if ($wavSymbols | Select-String -Pattern '\s(malloc|zalloc|free)$') {
        throw 'WAV adapter still references the RAM heap.'
    }
    Move-Item -LiteralPath $wavTemp -Destination $wavOutput -Force
} finally {
    Pop-Location
}
