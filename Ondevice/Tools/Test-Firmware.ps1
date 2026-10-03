[CmdletBinding()]
param([string]$Bundle)
. "$PSScriptRoot/Firmware.Common.ps1"
if (-not $Bundle) { $Bundle = Join-Path $ProjectRoot 'Firmware/baseline' }
$manifest = Read-ValidatedBundle $Bundle
Write-Host 'PASS: three images, headers, memory bounds, payload checksums and SHA256.'
$testRoot = Join-Path $ProjectRoot ('Build/tests/package-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $testRoot | Out-Null
foreach ($image in $manifest.images) { Copy-Item -LiteralPath (Join-Path $Bundle $image.file) -Destination $testRoot }
$manifest | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $testRoot 'manifest.json') -Encoding utf8
$path = Join-Path $testRoot 'NonSecure-trusted.bin'
(Get-Item -LiteralPath $path).IsReadOnly = $false
$bytes = [IO.File]::ReadAllBytes($path)
$bytes[$bytes.Length-1] = $bytes[$bytes.Length-1] -bxor 1
[IO.File]::WriteAllBytes($path,$bytes)
$rejected = $false
try { $null = Read-ValidatedBundle $testRoot } catch { $rejected = $true }
if (-not $rejected) { throw 'Corrupt package accepted.' }
Write-Host 'PASS: corrupted payload rejected before programming.'
Copy-Item -LiteralPath (Join-Path $Bundle 'NonSecure-trusted.bin') -Destination $path -Force
$manifest.images[1].address = '0x70000000'
$manifest | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $testRoot 'manifest.json') -Encoding utf8
$rejected = $false
try { $null = Read-ValidatedBundle $testRoot } catch { $rejected = $true }
if (-not $rejected) { throw 'Wrong Flash address accepted.' }
Write-Host 'PASS: incorrect slot rejected before programming.'
& "$PSScriptRoot/Program-Firmware.ps1" -Bundle $Bundle -PlanOnly
