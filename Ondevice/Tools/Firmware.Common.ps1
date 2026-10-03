Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))

function Invoke-Native([string]$Tool, [string[]]$Arguments) {
    & $Tool @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Command failed ($LASTEXITCODE): $Tool" }
}

function Find-ArmToolchain([string]$Bin) {
    if (-not $Bin) { $Bin = $env:ST_GCC_BIN }
    if (-not $Bin) {
        $plugins = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins'
        $match = Get-ChildItem $plugins -Directory -Filter '*gnu-tools-for-stm32*' -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending | Select-Object -First 1
        if ($match) { $Bin = Join-Path $match.FullName 'tools/bin' }
    }
    if (-not $Bin -or -not (Test-Path (Join-Path $Bin 'arm-none-eabi-gcc.exe'))) {
        throw 'Set ST_GCC_BIN or pass -ToolchainBin to the STM32 GNU tools bin directory.'
    }
    return [IO.Path]::GetFullPath($Bin)
}

function Find-Programmer([string]$Bin) {
    if (-not $Bin) { $Bin = $env:ST_PROGRAMMER_BIN }
    if (-not $Bin) { $Bin = Join-Path $env:ProgramFiles 'STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin' }
    if (-not (Test-Path (Join-Path $Bin 'STM32_Programmer_CLI.exe'))) {
        throw 'Set ST_PROGRAMMER_BIN or pass -ProgrammerBin to the CubeProgrammer bin directory.'
    }
    return [IO.Path]::GetFullPath($Bin)
}

function Get-U32([byte[]]$Bytes, [int]$Offset) { return [BitConverter]::ToUInt32($Bytes, $Offset) }

function Get-ImageInfo([string]$Path, [string]$Kind) {
    $b = [IO.File]::ReadAllBytes($Path)
    if ($b.Length -lt 1032) { throw "Truncated image: $Path" }
    if ([Text.Encoding]::ASCII.GetString($b,0,4) -ne 'STM2' -or
        (Get-U32 $b 104) -ne 0x20300 -or (Get-U32 $b 132) -ne 0x80000000L -or
        (Get-U32 $b 136) -ne 416 -or (Get-U32 $b 140) -ne 16 -or
        (Get-U32 $b 152) -ne 0 -or (Get-U32 $b 160) -ne 0xFFFF5453L -or
        (Get-U32 $b 164) -ne 416) { throw "Unsupported v2.3 -nk -align header: $Path" }
    $size = 576L + (Get-U32 $b 108)
    if ($size -ne $b.Length) { throw "Header/file length mismatch: $Path" }
    $limits = @{
        FSBL = @(0x34180400L,0x341C0000L,0x34200000L,0x40000L)
        Secure = @(0x34000400L,0x34064000L,0x34100000L,0x64000L)
        NonSecure = @(0x24100400L,0x24180000L,0x24200000L,0x80000L)
    }
    $limit = $limits[$Kind]
    if (-not $limit) { throw "Unknown image kind: $Kind" }
    $msp = Get-U32 $b 1024
    $reset = Get-U32 $b 1028
    $code = [long]$reset -band 0xFFFFFFFEL
    if ($size -gt $limit[3] -or $msp -le $limit[1] -or $msp -gt $limit[2] -or
        ($msp -band 7) -ne 0 -or ($reset -band 1) -ne 1 -or
        $reset -ne (Get-U32 $b 112) -or $code -lt $limit[0] -or
        $code -gt $limit[0] + $size - 1024 - 2) { throw "Invalid memory/vector bounds: $Path" }
    [uint64]$sum = 0
    for ($i = 576; $i -lt $b.Length; $i++) { $sum += $b[$i] }
    if (($sum -band 0xFFFFFFFFL) -ne (Get-U32 $b 100)) { throw "Payload checksum mismatch: $Path" }
    return [ordered]@{
        kind=$Kind; file=[IO.Path]::GetFileName($Path); size=$size
        msp=('0x{0:X8}' -f $msp); reset=('0x{0:X8}' -f $reset)
        sha256=(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    }
}

function Read-ValidatedBundle([string]$Directory) {
    $directoryPath = [IO.Path]::GetFullPath($Directory)
    $manifest = Get-Content -LiteralPath (Join-Path $directoryPath 'manifest.json') -Raw | ConvertFrom-Json
    if ($manifest.schema -ne 1 -or @($manifest.images).Count -ne 3) { throw 'Invalid bundle schema.' }
    $expected = @{ FSBL='0x70000000'; Secure='0x70100000'; NonSecure='0x70180000' }
    $seen = @{}
    foreach ($entry in $manifest.images) {
        if (-not $expected.ContainsKey($entry.kind) -or $seen.ContainsKey($entry.kind) -or
            $entry.address -ne $expected[$entry.kind] -or
            $entry.file -ne [IO.Path]::GetFileName($entry.file)) { throw 'Invalid image kind, file or Flash address.' }
        $seen[$entry.kind] = $true
        $info = Get-ImageInfo (Join-Path $directoryPath $entry.file) $entry.kind
        if ($info.sha256 -ne $entry.sha256 -or $info.size -ne $entry.size -or
            $info.msp -ne $entry.msp -or $info.reset -ne $entry.reset) { throw "Bundle hash/metadata mismatch: $($entry.kind)" }
    }
    return $manifest
}
