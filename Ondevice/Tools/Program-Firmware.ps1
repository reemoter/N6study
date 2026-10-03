[CmdletBinding()]
param(
    [string]$Bundle,
    [string]$ProgrammerBin,
    [string]$SerialNumber,
    [switch]$IncludeFsbl,
    [switch]$PlanOnly
)
. "$PSScriptRoot/Firmware.Common.ps1"
if (-not $Bundle) { $Bundle = Join-Path $ProjectRoot 'Build/bundle' }
$Bundle = [IO.Path]::GetFullPath($Bundle)
$manifest = Read-ValidatedBundle $Bundle
$selected = @($manifest.images | Where-Object { $_.kind -ne 'FSBL' })
if ($IncludeFsbl) { $selected += @($manifest.images | Where-Object { $_.kind -eq 'FSBL' }) }
foreach ($image in $selected) { Write-Host "$($image.kind): $($image.file) -> $($image.address), $($image.size) bytes" }
if ($PlanOnly) { Write-Host 'Plan only: no target connection or writes.'; return }
$ProgrammerBin = Find-Programmer $ProgrammerBin
$cli = Join-Path $ProgrammerBin 'STM32_Programmer_CLI.exe'
$loader = Join-Path $ProgrammerBin 'ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr'
if (-not (Test-Path $loader)) { throw "Missing ST external loader: $loader" }
if (-not $SerialNumber) {
    $probes = & $cli -l stlink
    if ($LASTEXITCODE -ne 0) { throw 'Cannot enumerate ST-LINK.' }
    $serials = @($probes | ForEach-Object { if ($_ -match 'ST-LINK SN\s*:\s*(\S+)') { $Matches[1] } })
    if ($serials.Count -ne 1) { throw 'Connect exactly one ST-LINK or pass -SerialNumber.' }
    $SerialNumber = $serials[0]
}
Write-Host 'Required hardware state: BOOT0=L, BOOT1=H; power-cycle; CubeIDE/CubeProgrammer sessions closed.'
$connect = @('-c','port=SWD',"sn=$SerialNumber",'mode=HOTPLUG','ap=1','-el',$loader)
if ($IncludeFsbl) {
    $backupRoot = Join-Path $ProjectRoot 'Build/backups'
    New-Item -ItemType Directory -Force $backupRoot | Out-Null
    $backup = Join-Path $backupRoot ('fsbl-sector-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N') + '.bin')
    Invoke-Native $cli ($connect + @('-r','0x70000000','0x10000',$backup))
    if ((Get-Item $backup).Length -ne 65536) { throw 'Incomplete FSBL backup; refusing to replace boot image.' }
    Write-Host "Previous boot sector preserved: $backup"
}
foreach ($image in $selected) {
    # Recheck between writes as well; do not deploy a package changed mid-operation.
    $null = Read-ValidatedBundle $Bundle
    Invoke-Native $cli ($connect + @('-w',(Join-Path $Bundle $image.file),$image.address,'-v'))
}
Write-Host 'Downloads verified. Power off, set BOOT0=L / BOOT1=L, then power on without a debugger.'
Write-Host 'Baseline behavior: LED1 on, LED2 blinking while the non-secure loop runs.'
