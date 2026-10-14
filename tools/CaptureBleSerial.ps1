param([int]$Seconds=75,[string]$Port='COM12')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'output/ble-firmware'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$path=Join-Path $out ('serial-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.log')
$serial=[IO.Ports.SerialPort]::new($Port,1000000,[IO.Ports.Parity]::None,8,[IO.Ports.StopBits]::One)
$serial.DtrEnable=$false;$serial.RtsEnable=$false
$serial.Encoding=[Text.Encoding]::UTF8
$writer=$null
try{
    $serial.Open()
    $writer=[IO.StreamWriter]::new($path,$false,[Text.UTF8Encoding]::new($false))
    $end=(Get-Date).AddSeconds($Seconds)
    Write-Output "Capturing $Port at 1000000 baud for $Seconds seconds"
    while((Get-Date) -lt $end){$chunk=$serial.ReadExisting();if($chunk){$writer.Write($chunk);$writer.Flush()};Start-Sleep -Milliseconds 20}
}finally{if($null -ne $writer){$writer.Dispose()};if($serial.IsOpen){$serial.Close()};$serial.Dispose()}
Write-Output $path
