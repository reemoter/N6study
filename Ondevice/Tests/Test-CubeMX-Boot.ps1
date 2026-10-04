Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$guard = Join-Path $root 'Tools/Check-CubeMX-Boot.ps1'
$scratch = Join-Path $root ('Build/tests/cubemx-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $scratch | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'Ondevice.ioc') -Destination $scratch
foreach ($project in @('FSBL','AppliSecure','AppliNonSecure')) {
    $target = Join-Path $scratch $project
    New-Item -ItemType Directory -Force $target | Out-Null
    Copy-Item -LiteralPath (Join-Path $root "$project/Core") -Destination $target -Recurse
}
& $guard -ProjectDirectory $scratch
$cases = @(
    @{path='FSBL/Core/Src/main.c'; old='#include "fsbl_app.h"'; new=''; name='lost FSBL include'},
    @{path='FSBL/Core/Src/stm32n6xx_hal_msp.c'; old='PWR_VDDIO_RANGE_1V8'; new='PWR_VDDIO_RANGE_3V3'; name='wrong NOR voltage'},
    @{path='AppliSecure/Core/Src/main.c'; old='Secure_BootEnterNonSecure();'; new=''; name='lost Secure handoff'},
    @{path='AppliSecure/Core/Inc/partition_stm32n657xx.h'; old='#define SAU_INIT_REGION1    1'; new='#define SAU_INIT_REGION1    0'; name='disabled NS SRAM attribution'}
)
foreach ($case in $cases) {
    $path = Join-Path $scratch $case.path
    $original = Get-Content -LiteralPath $path -Raw
    if (-not $original.Contains($case.old)) { throw "Test fixture not found: $($case.name)" }
    try {
        [IO.File]::WriteAllText($path, $original.Replace($case.old, $case.new), [Text.UTF8Encoding]::new($false))
        $rejected = $false
        try { & $guard -ProjectDirectory $scratch } catch { $rejected = $true }
        if (-not $rejected) { throw "Boot guard accepted: $($case.name)" }
        Write-Host "PASS: rejected $($case.name)"
    } finally {
        [IO.File]::WriteAllText($path, $original, [Text.UTF8Encoding]::new($false))
    }
}
& $guard -ProjectDirectory $scratch
