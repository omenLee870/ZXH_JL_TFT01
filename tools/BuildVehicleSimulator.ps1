param([string]$Python = 'python')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$projectName = [string]::Concat([char]0x4e0a,[char]0x4f4d,[char]0x673a)
$desktopRoot = Join-Path (Split-Path $taskRoot -Parent) $projectName
$buildScript = Join-Path $desktopRoot 'build.ps1'
if (!(Test-Path -LiteralPath $buildScript)) { throw "Desktop project not found: $desktopRoot" }
& $buildScript -Python $Python
if ($LASTEXITCODE) { throw 'Desktop packaging failed.' }
