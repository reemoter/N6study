[CmdletBinding()]
param([string]$ToolchainBin, [string]$ProgrammerBin)
. "$PSScriptRoot/Firmware.Common.ps1"
$ToolchainBin = Find-ArmToolchain $ToolchainBin
$ProgrammerBin = Find-Programmer $ProgrammerBin
$gcc = Join-Path $ToolchainBin 'arm-none-eabi-gcc.exe'
$output = Join-Path $ProjectRoot 'Build/Debug'
$bundle = Join-Path $ProjectRoot 'Build/bundle'
New-Item -ItemType Directory -Force $bundle | Out-Null
# A failed rebuild must not leave an apparently complete deployment package.
$manifestPath = Join-Path $bundle 'manifest.json'
if (Test-Path $manifestPath) { Remove-Item -LiteralPath $manifestPath }
& "$PSScriptRoot/Sync-HalLinks.ps1"
& "$PSScriptRoot/Sync-FreeRTOS.ps1"
& "$PSScriptRoot/Check-CubeMX-Boot.ps1"
$projects = @(
    @{folder='FSBL'; kind='FSBL'; ld='STM32N657X0HXQ_AXISRAM2_fsbl.ld'; address='0x70000000'},
    @{folder='AppliSecure'; kind='Secure'; ld='STM32N657X0HXQ_LRUN_s.ld'; address='0x70100000'},
    @{folder='AppliNonSecure'; kind='NonSecure'; ld='STM32N657X0HXQ_LRUN.ld'; address='0x70180000'}
)
$images = @()
foreach ($project in $projects) {
    $root = Join-Path $ProjectRoot $project.folder
    $out = Join-Path $output $project.folder
    New-Item -ItemType Directory -Force $out | Out-Null
    [xml]$definition = Get-Content -LiteralPath (Join-Path $root '.project') -Raw
    $sources = @((Get-ChildItem (Join-Path $root 'Core/Src') -Filter '*.c' | Sort-Object Name).FullName)
    $sources += @((Get-ChildItem (Join-Path $root 'Core/Startup') -Filter '*.s' | Sort-Object Name).FullName)
    foreach ($link in $definition.projectDescription.linkedResources.link) {
        $uri = [string]$link.locationURI
        if (-not $uri.StartsWith('PARENT-1-PROJECT_LOC/')) { throw "Unsupported linked source: $uri" }
        $sources += Join-Path $ProjectRoot $uri.Substring(21)
    }
    $flags = @('-mcpu=cortex-m55','-mfpu=fpv5-d16','-mfloat-abi=hard','-mthumb',
        '-g3','-O0','-DDEBUG','-DUSE_HAL_DRIVER','-DSTM32N657xx','-ffunction-sections','-fdata-sections')
    if ($project.kind -ne 'NonSecure') { $flags += '-mcmse' }
    $includes = @("$root/Core/Inc","$ProjectRoot/Secure_nsclib",
        "$ProjectRoot/Drivers/STM32N6xx_HAL_Driver/Inc", "$ProjectRoot/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy",
        "$ProjectRoot/Drivers/CMSIS/Device/ST/STM32N6xx/Include","$ProjectRoot/Drivers/CMSIS/Include")
    $objects = @()
    if ($project.kind -ne 'FSBL' -and (Test-Path "$root/Core/Inc/FreeRTOSConfig.h")) {
        $rtosRoot = "$ProjectRoot/Middlewares/Third_Party/FreeRTOS/Source"
        $includes += @("$ProjectRoot/Middlewares/Third_Party/CMSIS/RTOS2/Include", "$rtosRoot/include", "$rtosRoot/CMSIS_RTOS_V2", "$rtosRoot/portable/GCC/ARM_CM55/secure")
        if ($project.kind -eq 'NonSecure') { $includes += "$rtosRoot/portable/GCC/ARM_CM55/non_secure" }
    }
    foreach ($source in $sources) {
        if (-not (Test-Path $source)) { throw "Missing source: $source" }
        $relative = [IO.Path]::GetRelativePath($ProjectRoot,$source)
        $object = Join-Path $out ($relative + '.o')
        New-Item -ItemType Directory -Force (Split-Path $object) | Out-Null
        $arguments = $flags + @('-c',$source,'-o',$object)
        if ([IO.Path]::GetExtension($source) -eq '.c') { $arguments += @('-std=gnu11','-Wall','-Wextra','--specs=nano.specs') }
        $arguments += @($includes | ForEach-Object { '-I' + $_ })
        Invoke-Native $gcc $arguments
        $objects += $object
    }
    $elf = Join-Path $out ($project.kind + '.elf')
    $raw = Join-Path $out ($project.kind + '.bin')
    $map = Join-Path $out ($project.kind + '.map')
    $response = Join-Path $out 'objects.rsp'
    [IO.File]::WriteAllLines($response, @($objects | ForEach-Object { '"' + $_.Replace('\','/') + '"' }))
    $arguments = @('@' + $response) + $flags[0..3] + @('-o',$elf,'-T' + (Join-Path $root $project.ld),
        '--specs=nosys.specs','--specs=nano.specs',"-Wl,-Map=$map",'-Wl,--gc-sections','-static')
    if ($project.kind -ne 'NonSecure') {
        $arguments += @('-Wl,--cmse-implib',"-Wl,--out-implib=$out/secure_nsclib.o")
    } else {
        $arguments += Join-Path $output 'AppliSecure/secure_nsclib.o'
    }
    $arguments += @('-Wl,--start-group','-lc','-lm','-Wl,--end-group')
    Invoke-Native $gcc $arguments
    Invoke-Native (Join-Path $ToolchainBin 'arm-none-eabi-objcopy.exe') @('-O','binary',$elf,$raw)
    Invoke-Native (Join-Path $ToolchainBin 'arm-none-eabi-size.exe') @($elf)
    $signed = Join-Path $bundle ($project.kind + '-trusted.bin')
    if (Test-Path $signed) { Remove-Item -LiteralPath $signed -Force }
    Invoke-Native (Join-Path $ProgrammerBin 'STM32_SigningTool_CLI.exe') @('-bin',$raw,'-nk','-of','0x80000000','-t','fsbl','-hv','2.3','-align','-o',$signed)
    $info = Get-ImageInfo $signed $project.kind
    $info.address = $project.address
    $images += $info
}
# Verify every imported Secure API resolves to the freshly built Secure address.
$secureSymbols = & (Join-Path $ToolchainBin 'arm-none-eabi-nm.exe') (Join-Path $output 'AppliSecure/secure_nsclib.o')
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect Secure import library.' }
$nsSymbols = & (Join-Path $ToolchainBin 'arm-none-eabi-nm.exe') (Join-Path $output 'AppliNonSecure/NonSecure.elf')
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect Non-secure symbols.' }
foreach ($line in $secureSymbols) {
    if ($line -match '^([0-9a-fA-F]+)\s+\w\s+((?:SECURE_|SecureContext_|SecureInit_)\w+)$') {
        $name = $Matches[2]; $address = $Matches[1]
        $actual = @($nsSymbols | Where-Object { $_ -match ("^" + $address + '\s+\w\s+' + [regex]::Escape($name) + '$') })
        # APIs unused by NS may be removed by --gc-sections.
        $used = @($nsSymbols | Where-Object { $_ -match ('\s' + [regex]::Escape($name) + '$') })
        if ($used.Count -gt 0 -and $actual.Count -ne 1) { throw "Secure import mismatch: $name" }
    }
}
$manifest = [ordered]@{schema=1; provenance='built-from-source'; toolchain=(& $gcc -dumpfullversion); images=$images}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding utf8
$null = Read-ValidatedBundle $bundle
Write-Host "Validated bundle: $bundle"
