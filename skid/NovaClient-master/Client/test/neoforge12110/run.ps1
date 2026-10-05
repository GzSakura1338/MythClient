param(
    [string]$Jar='C:/Users/13366/AppData/Roaming/.minecraft/libraries/net/neoforged/minecraft-client-patched/21.10.64/minecraft-client-patched-21.10.64.jar',
    [string]$RuntimeDumpDirectory=''
)
$ErrorActionPreference='Stop'
# Reuse the read-only JNI/bytecode audit harness, select the independent table.
& "$PSScriptRoot/../easecation/run.ps1" -Jar $Jar -GameVersion '1.21.10' -RuntimeDumpDirectory $RuntimeDumpDirectory
