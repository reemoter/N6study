[CmdletBinding()]
param([string]$ProjectDirectory = (Join-Path $PSScriptRoot '..'))
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$path = Join-Path $ProjectDirectory 'AppliNonSecure/STM32N657X0HXQ_LRUN.ld'
$original = Get-Content -LiteralPath $path -Raw
$updated = $original
if ($updated -notmatch '(?m)^\s*PSRAM\s*\(') {
    $updated = [regex]::Replace($updated, '(?s)(MEMORY\s*\{.*?)(\r?\n\})', '$1' + "`n  PSRAM (rw) : ORIGIN = 0x90000000, LENGTH = 32M" + '$2', 1)
}
if ($updated -notmatch '\.external_ram\s*\(NOLOAD\)') {
    $section = @'
  /* Project-owned PSRAM: excluded from startup zeroing and binary payload. */
  .external_ram (NOLOAD) :
  {
    . = ALIGN(64);
    __external_ram_start = .;
    KEEP(*(SORT_BY_NAME(.external_ram.*)))
    . = ALIGN(64);
    __external_ram_end = .;
  } >PSRAM
  ASSERT(SIZEOF(.external_ram) <= LENGTH(PSRAM), "PSRAM buffers exceed 32MiB")

'@
    $updated = $updated.Replace('  /* Remove information from the compiler libraries */', $section + "`n  /* Remove information from the compiler libraries */")
}
if ($updated -notmatch 'PSRAM\s*\(rw\)' -or $updated -notmatch '\.external_ram\s*\(NOLOAD\)') {
    throw 'Cannot integrate PSRAM layout into regenerated linker script'
}
if ($updated -ne $original) { Set-Content -LiteralPath $path -Value $updated -Encoding utf8 }
Write-Host 'PASS: external RAM NOLOAD layout present.'
