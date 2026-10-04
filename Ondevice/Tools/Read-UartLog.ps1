[CmdletBinding()]
param([string]$Port, [ValidateRange(1,3600)][int]$Seconds = 30)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $Port) {
    $matches = @(Get-CimInstance Win32_SerialPort | Where-Object { $_.Name -match 'STLink Virtual COM Port' })
    if ($matches.Count -ne 1) { throw 'Connect one ST-LINK VCP or pass -Port COMx.' }
    $Port = $matches[0].DeviceID
}
$serial = [IO.Ports.SerialPort]::new($Port, 115200, [IO.Ports.Parity]::None, 8, [IO.Ports.StopBits]::One)
$serial.Handshake = [IO.Ports.Handshake]::None
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$serial.ReadTimeout = 1000
$serial.NewLine = "`n"
try {
    $serial.Open()
    Write-Host "Listening on $Port, 115200 8N1 for $Seconds seconds. Close other serial terminals first."
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    $received = 0
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $line = $serial.ReadLine().TrimEnd("`r")
            if ($line.Length -gt 0) { Write-Host $line; $received++ }
        } catch [TimeoutException] {}
    }
    if ($received -eq 0) { throw "No UART log received on $Port. Check normal L/L boot and running firmware." }
    Write-Host "Received $received lines. Serial port closed on exit."
} finally {
    $serial.Dispose()
}
