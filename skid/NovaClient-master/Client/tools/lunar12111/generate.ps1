param([string]$MappingJar="$env:USERPROFILE/.lunarclient/offline/multiver/lunar-platform-mappings-v1_21_11.jar")
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
Add-Type -Path "$PSScriptRoot/MappingTool.cs"
$mapping=[LunarMappingTool]::Read($MappingJar)
$mapping.Generate("$client/src/minecraft","$client/src/minecraft/compat/mappings/Lunar12111.inc")
Write-Output "Parsed $($mapping.Classes.Count) classes and $($mapping.Members.Count) members from $MappingJar"
Get-Item "$client/src/minecraft/compat/mappings/Lunar12111.inc" | Select-Object FullName,Length
