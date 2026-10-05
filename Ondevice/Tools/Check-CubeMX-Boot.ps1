[CmdletBinding()]
param([string]$ProjectDirectory = (Join-Path $PSScriptRoot '..'))
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-Source([string]$RelativePath) {
    $path = Join-Path $ProjectDirectory $RelativePath
    if (-not (Test-Path -LiteralPath $path)) { throw "Boot guard: missing $RelativePath" }
    return Get-Content -LiteralPath $path -Raw
}
function Require-Match([string]$Text, [string]$Pattern, [string]$Reason) {
    if ($Text -notmatch $Pattern) { throw "Boot guard: $Reason" }
}
function User-Section([string]$Text, [string]$Name) {
    $escaped = [regex]::Escape($Name)
    $match = [regex]::Match($Text, "(?s)/\* USER CODE BEGIN $escaped \*/(.*?)/\* USER CODE END $escaped \*/")
    if (-not $match.Success) { throw "Boot guard: missing USER CODE section $Name" }
    return $match.Groups[1].Value
}

$ioc = Read-Source 'Ondevice.ioc'
Require-Match $ioc '(?m)^ProjectManager.KeepUserCode=true\s*$' 'CubeMX Keep User Code must be enabled'
$fsbl = Read-Source 'FSBL/Core/Src/main.c'
$includes = User-Section $fsbl 'Includes'
foreach ($header in @('nor_flash.h','fsbl_app.h')) {
    Require-Match $includes ('#include\s+"' + [regex]::Escape($header) + '"') "$header must be included inside USER CODE Includes"
}
$load = User-Section $fsbl '2'
Require-Match $load 'FSBL_LoadApplications\s*\(' 'FSBL application loading hook lost'
Require-Match $load 'if\s*\(\s*fsbl_app_status\s*==\s*FSBL_APP_READY\s*\)' 'FSBL READY condition lost'
Require-Match $load 'FSBL_JumpToSecure\s*\(' 'Secure handoff hook lost'
$msp = User-Section (Read-Source 'FSBL/Core/Src/stm32n6xx_hal_msp.c') 'MspInit 1'
Require-Match $msp 'HAL_PWREx_ConfigVddIORange\s*\(\s*PWR_VDDIO3\s*,\s*PWR_VDDIO_RANGE_1V8\s*\)' 'NOR VDDIO3 1.8V hook lost'
$xspi = Read-Source 'FSBL/Core/Src/xspi.c'
foreach ($setting in @('MemoryType\s*=\s*HAL_XSPI_MEMTYPE_MACRONIX', 'MemorySelect\s*=\s*HAL_XSPI_CSSEL_NCS1',
                       'IOPort\s*=\s*HAL_XSPIM_IOPORT_2', 'Xspi2ClockSelection\s*=\s*RCC_XSPI2CLKSOURCE_IC3',
                       'ClockSelection\s*=\s*RCC_ICCLKSOURCE_PLL1', 'ClockDivider\s*=\s*32', 'ClockPrescaler\s*=\s*0')) {
    Require-Match $xspi $setting "NOR XSPI setting changed: $setting"
}
$divider = [regex]::Match($ioc, '(?m)^RCC.HPRE_Div=(\w+)\s*$')
if ($divider.Success) {
    Require-Match $fsbl ('AHBCLKDivider\s*=\s*' + [regex]::Escape($divider.Groups[1].Value) + '\s*;') 'AHB divider differs between source and .ioc'
}
$secure = Read-Source 'AppliSecure/Core/Src/main.c'
Require-Match (User-Section $secure 'Includes') '#include\s+"secure_boot.h"' 'Secure boot include hook lost'
Require-Match (User-Section $secure 'Init') 'secure_boot_stage\s*=\s*1U' 'Secure Init diagnostics hook lost'
Require-Match (User-Section $secure '2') 'Secure_BootEnterNonSecure\s*\(' 'Secure project-owned handoff hook lost'
$regions = User-Section $secure 'RIF_Init 1'
foreach ($region in @('RISAF3','RISAF2','RISAF7')) {
    Require-Match $regions ("HAL_RIF_RISAF_ConfigBaseRegion\s*\(\s*$region\s*,") "$region memory attribution hook lost"
}
$null = Read-Source 'AppliSecure/Core/Src/secure_boot.c'
$partition = Read-Source 'AppliSecure/Core/Inc/partition_stm32n657xx.h'
foreach ($definition in @('SAU_INIT_CTRL_ENABLE\s+1', 'SAU_INIT_REGION0\s+1', 'SAU_INIT_NSC0\s+1',
                          'SAU_INIT_REGION1\s+1', 'SAU_INIT_START1\s+0x24100000', 'SAU_INIT_END1\s+0x241FFFFF',
                          'SAU_INIT_REGION2\s+1', 'SAU_INIT_START2\s+0x40000000', 'SAU_INIT_END2\s+0x4FFFFFFF')) {
    Require-Match $partition ('#define\s+' + $definition + '\b') "SAU layout changed: $definition"
}
Require-Match $partition '#define\s+SAU_INIT_START0\s+\(\(uint32_t\)\s*&_sNSCVeneer\)' 'NSC start must use linker veneer symbol'
Require-Match $partition '#define\s+SAU_INIT_END0\s+\(\(uint32_t\)\s*&_eNSCVeneer\)' 'NSC end must use linker veneer symbol'
$null = Read-Source 'FSBL/Core/Src/fsbl_app.c'
$null = Read-Source 'FSBL/Core/Src/nor_flash.c'
if ($ioc -match '(?m)^XSPI1.MemoryType=') {
    foreach ($setting in @('MemoryType=HAL_XSPI_MEMTYPE_APMEM_16BITS',
                           'MemorySize=HAL_XSPI_SIZE_256MB', 'ClockPrescaler=3',
                           'Refresh=196', 'ChipSelectBoundary=HAL_XSPI_BONDARYOF_16KB')) {
        Require-Match $ioc ('(?m)^XSPI1\.' + [regex]::Escape($setting) + '\s*$') "PSRAM setting changed: $setting"
    }
    Require-Match $ioc '(?m)^PO0.Signal=XSPIM_P1_NCS1\s*$' 'PSRAM must use PO0/NCS1'
    Require-Match $ioc '(?m)^PO1.Signal=GPIO_Output\s*$' 'LED1 PO1 must remain GPIO'
    $ramXspi = Read-Source 'AppliSecure/Core/Src/xspi.c'
    Require-Match $ramXspi 'nCSOverride\s*=\s*HAL_XSPI_CSSEL_OVR_NCS1' 'PSRAM CS override must be NCS1'
    Require-Match $ramXspi 'IOPort\s*=\s*HAL_XSPIM_IOPORT_1' 'PSRAM must use Port1'
    Require-Match $ramXspi 'Xspi1ClockSelection\s*=\s*RCC_XSPI1CLKSOURCE_HCLK' 'PSRAM must use HCLK'
    Require-Match (Read-Source 'AppliSecure/Core/Src/main.c') 'MX_XSPI1_Init\s*\(\s*\)' 'Generated PSRAM controller init missing'
    $ramMsp = User-Section (Read-Source 'AppliSecure/Core/Src/stm32n6xx_hal_msp.c') 'MspInit 1'
    Require-Match $ramMsp 'HAL_PWREx_ConfigVddIORange\s*\(\s*PWR_VDDIO2\s*,\s*PWR_VDDIO_RANGE_1V8' 'PSRAM 1.8V hook lost'
    $ramDriver = Read-Source 'AppliSecure/Core/Src/ext_ram_secure.c'
    if ($ramDriver -match 'HAL_XSPI_Init\s*\(|HAL_GPIO_Init\s*\(') {
        throw 'Boot guard: PSRAM driver duplicates CubeMX peripheral initialization'
    }
    if ($ramXspi -match '__HAL_RCC_XSPIM_FORCE_RESET') {
        throw 'Boot guard: shared XSPIM reset would affect NOR'
    }
}
Write-Host 'PASS: CubeMX boot hooks, NOR settings and SAU layout preserved.'
