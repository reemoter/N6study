[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('BuildAndTest', 'ProgramApps', 'ProgramPlan', 'ValidateBundle')]
    [string]$Action
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$transcribing = $false
try {
    if ($PSVersionTable.PSVersion.Major -lt 7) { throw 'PowerShell 7 is required.' }
    $logRoot = Join-Path $PSScriptRoot '../Build/logs'
    New-Item -ItemType Directory -Force $logRoot | Out-Null
    $logPath = Join-Path $logRoot ($Action + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N') + '.log')
    Start-Transcript -Path $logPath | Out-Null
    $transcribing = $true
    Write-Host "Log: $logPath"
    $bundlePath = Join-Path $PSScriptRoot '../Build/bundle'
    switch ($Action) {
        'BuildAndTest' {
            & "$PSScriptRoot/Build-Firmware.ps1"
            & "$PSScriptRoot/Test-Firmware.ps1" -Bundle $bundlePath
            Write-Host 'SUCCESS: build and package tests passed. NOR has not been changed.'
            Write-Host 'Next: terminate debugging, power-cycle with BOOT0=L / BOOT1=H, then run 02 Program Apps.'
            Write-Host 'The debugger ELF has changed: program this bundle before debugging.'
        }
        'ProgramApps' {
            & "$PSScriptRoot/Program-Firmware.ps1" -Bundle $bundlePath
        }
        'ProgramPlan' {
            & "$PSScriptRoot/Program-Firmware.ps1" -Bundle $bundlePath -PlanOnly
        }
        'ValidateBundle' {
            & "$PSScriptRoot/Test-Firmware.ps1" -Bundle $bundlePath
        }
    }
    exit 0
} catch {
    Write-Error -Message $_.Exception.Message -ErrorAction Continue
    exit 1
} finally {
    if ($transcribing) { Stop-Transcript | Out-Null }
}
