[CmdletBinding()]
param([string]$ProjectDirectory = (Join-Path $PSScriptRoot '..'))
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$driverRoot = Join-Path $ProjectDirectory 'Drivers/STM32N6xx_HAL_Driver/Src'
foreach ($project in @('FSBL','AppliSecure','AppliNonSecure')) {
    $root = Join-Path $ProjectDirectory $project
    $config = Get-Content -LiteralPath (Join-Path $root 'Core/Inc/stm32n6xx_hal_conf.h') -Raw
    $modules = [regex]::Matches($config, '(?m)^\s*#define\s+HAL_([A-Z0-9]+)_MODULE_ENABLED\b') |
        ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() } | Sort-Object -Unique
    $projectFile = Join-Path $root '.project'
    [xml]$xml = Get-Content -LiteralPath $projectFile -Raw
    $links = $xml.projectDescription.linkedResources
    if (-not $links) { throw "Missing linkedResources in $projectFile" }
    $existing = @($links.link | ForEach-Object { [string]$_.name })
    $changed = $false
    foreach ($module in $modules) {
        foreach ($fileName in @("stm32n6xx_hal_$module.c", "stm32n6xx_hal_${module}_ex.c")) {
            if (-not (Test-Path -LiteralPath (Join-Path $driverRoot $fileName))) { continue }
            $linkName = "Drivers/STM32N6xx_HAL_Driver/$fileName"
            if ($existing -contains $linkName) { continue }
            $link = $xml.CreateElement('link')
            foreach ($property in @(@('name',$linkName), @('type','1'),
                    @('locationURI',"PARENT-1-PROJECT_LOC/Drivers/STM32N6xx_HAL_Driver/Src/$fileName"))) {
                $node = $xml.CreateElement($property[0])
                $node.InnerText = $property[1]
                $null = $link.AppendChild($node)
            }
            $null = $links.AppendChild($link)
            $existing += $linkName
            $changed = $true
            Write-Host "HAL link added: $project/$fileName"
        }
    }
    if ($changed) {
        $settings = [Xml.XmlWriterSettings]::new()
        $settings.Indent = $true
        $settings.IndentChars = "`t"
        $settings.Encoding = [Text.UTF8Encoding]::new($false)
        $writer = [Xml.XmlWriter]::Create($projectFile, $settings)
        try { $xml.Save($writer) } finally { $writer.Dispose() }
    }
}
