param([string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$sdk=Join-Path $root 'SDK/cpu/br33/tools'
$out=if($OutputDirectory){[IO.Path]::GetFullPath((Join-Path $root $OutputDirectory))}else{Join-Path $root 'output/ble-firmware'}
$copy='C:/JL/pi32/bin/llvm-objcopy.exe'
$elf=Join-Path $sdk 'sdk.elf'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sections=[ordered]@{
    'startup_entry'='.startup_entry';'text'='.text';'data'='.data';'data_code'='.data_code'
    'aec'='.overlay_aec';'aac'='.overlay_aac';'psr_data_code'='.ps_ram_data_code'
    'd_ram_data'='.dcache_ram_data';'i_ram_data_code'='.icache_ram_data_code';'pmc_ram_data_code'='.pmc_ram_data_code'
}
foreach($s in $sections.GetEnumerator()){
    & $copy -O binary -j $s.Value $elf (Join-Path $out ($s.Key+'.bin'))
    if($LASTEXITCODE -ne 0){throw "objcopy failed: $($s.Value)"}
}
Copy-Item -LiteralPath (Join-Path $sdk 'pmc_code_dashboard.bin') -Destination (Join-Path $out 'pmc_code.bin')
$parts='startup_entry','pmc_code','text','data','data_code','aec','aac','psr_data_code','d_ram_data','i_ram_data_code','pmc_ram_data_code'
$app=Join-Path $out 'app.bin'
$stream=[IO.File]::Create($app)
try{foreach($part in $parts){$bytes=[IO.File]::ReadAllBytes((Join-Path $out ($part+'.bin')));$stream.Write($bytes,0,$bytes.Length)}}finally{$stream.Dispose()}
# Reuse the previously prepared UI and boot resources without deleting or repacking them.
$resources='uboot.boot','br33loader.bin','isd_config.ini','flash_params_v3.bin','cfg_tool.bin','p11_code.bin','stream.bin','config.dat','tone_en.cfg','tone_zh.cfg','ota.bin','mode.bin','script.ver'
foreach($name in $resources){Copy-Item -LiteralPath (Join-Path $sdk ('download/watch/'+$name)) -Destination (Join-Path $out $name)}
& (Join-Path $sdk 'md5sum.exe') $app (Join-Path $out 'md5.bin')
if($LASTEXITCODE -ne 0){throw 'md5sum failed'}
Get-Item -LiteralPath $app | Select-Object FullName,Length
Get-FileHash -LiteralPath $app,$elf -Algorithm SHA256
