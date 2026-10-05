[CmdletBinding()]
param([string]$ProjectDirectory = (Join-Path $PSScriptRoot '..'))
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$relativeRoot = 'Middlewares/Third_Party/FreeRTOS/Source'
if (-not (Test-Path (Join-Path $ProjectDirectory 'AppliNonSecure/Core/Inc/FreeRTOSConfig.h'))) { return }
$definitions = @{
    AppliSecure = @('portable/GCC/ARM_CM55/secure/secure_context.c','portable/GCC/ARM_CM55/secure/secure_context_port.c',
                   'portable/GCC/ARM_CM55/secure/secure_heap.c','portable/GCC/ARM_CM55/secure/secure_init.c')
    AppliNonSecure = @('tasks.c','list.c','queue.c','timers.c','event_groups.c','stream_buffer.c',
                      'portable/MemMang/heap_4.c','CMSIS_RTOS_V2/cmsis_os2.c',
                      'portable/GCC/ARM_CM55/non_secure/port.c','portable/GCC/ARM_CM55/non_secure/portasm.c')
}
foreach ($project in @('AppliSecure','AppliNonSecure')) {
    $projectFile = Join-Path $ProjectDirectory "$project/.project"
    [xml]$xml = Get-Content -LiteralPath $projectFile -Raw
    $existing = @($xml.projectDescription.linkedResources.link | ForEach-Object { [string]$_.name })
    $changed = $false
    foreach ($source in $definitions[$project]) {
        $relative = "$relativeRoot/$source"
        if (-not (Test-Path (Join-Path $ProjectDirectory $relative))) { throw "Missing FreeRTOS source: $relative" }
        if ($existing -contains $relative) { continue }
        $link = $xml.CreateElement('link')
        foreach ($property in @(@('name',$relative),@('type','1'),@('locationURI',"PARENT-1-PROJECT_LOC/$relative"))) {
            $element = $xml.CreateElement($property[0]); $element.InnerText = $property[1]
            $null = $link.AppendChild($element)
        }
        $null = $xml.projectDescription.linkedResources.AppendChild($link)
        $changed = $true
    }
    if ($changed) { $xml.Save($projectFile) }
    $cprojectFile = Join-Path $ProjectDirectory "$project/.cproject"
    [xml]$cproject = Get-Content -LiteralPath $cprojectFile -Raw
    $changed = $false
    # CubeIDE invokes GCC from Appli*/Debug or Appli*/Release.
    # Shared middleware is two levels above that build directory.
    $includePaths = @('../../Middlewares/Third_Party/CMSIS/RTOS2/Include', "../../$relativeRoot/include", "../../$relativeRoot/CMSIS_RTOS_V2",
                      "../../$relativeRoot/portable/GCC/ARM_CM55/secure")
    if ($project -eq 'AppliNonSecure') { $includePaths += "../../$relativeRoot/portable/GCC/ARM_CM55/non_secure" }
    foreach ($option in $cproject.SelectNodes('//option[@valueType="includePath"]')) {
        foreach ($oldEntry in @($option.SelectNodes('listOptionValue[starts-with(@value,"../Middlewares/Third_Party/")]'))) {
            $null = $option.RemoveChild($oldEntry); $changed = $true
        }
        $existing = @($option.listOptionValue | ForEach-Object { [string]$_.value })
        foreach ($path in $includePaths) {
            if ($existing -contains $path) { continue }
            $entry = $cproject.CreateElement('listOptionValue')
            $entry.SetAttribute('builtIn','false'); $entry.SetAttribute('value',$path)
            $null = $option.AppendChild($entry); $changed = $true
        }
    }
    foreach ($sources in $cproject.SelectNodes('//sourceEntries')) {
        if ($sources.SelectSingleNode('entry[@name="Middlewares"]')) { continue }
        $entry = $cproject.CreateElement('entry')
        $entry.SetAttribute('flags','VALUE_WORKSPACE_PATH|RESOLVED')
        $entry.SetAttribute('kind','sourcePath'); $entry.SetAttribute('name','Middlewares')
        $null = $sources.AppendChild($entry); $changed = $true
    }
    if ($changed) { $cproject.Save($cprojectFile) }
}
Write-Host 'PASS: FreeRTOS CM55 NS kernel and Secure support linked.'
