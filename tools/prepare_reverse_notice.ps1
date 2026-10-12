param([string]$OutputDirectory, [string]$Python)
$ErrorActionPreference = 'Stop'
$reverseRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $OutputDirectory) { $OutputDirectory = $env:PROJ_DOWNLOAD_PATH }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $reverseRoot 'output' }
if (-not $Python) { $Python = $env:ZXH_PYTHON }
if (-not $Python) {
    $reverseCommand = Get-Command python.exe -ErrorAction SilentlyContinue
    if ($reverseCommand -and $reverseCommand.Source -notmatch 'WindowsApps') { $Python = $reverseCommand.Source }
}
if (-not $Python) {
    $reverseBundled = Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    if (Test-Path -LiteralPath $reverseBundled) { $Python = $reverseBundled }
}
if (-not $Python -or -not (Test-Path -LiteralPath $Python)) {
    throw 'Reverse notice: Python 3 required. Set ZXH_PYTHON or pass -Python.'
}
& $Python (Join-Path $PSScriptRoot 'pack_reverse_notice.py') --output-directory $OutputDirectory
if ($LASTEXITCODE -ne 0) { throw 'Reverse notice: resource packing failed; download stopped.' }
