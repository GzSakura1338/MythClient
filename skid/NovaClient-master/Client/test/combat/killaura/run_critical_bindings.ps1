param(
    [string]$ForgeJar="$env:APPDATA/.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar",
    [string]$ModernJar="$env:APPDATA/.minecraft/versions/26.3/26.3.jar"
)
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$output="$client/build-vs2026/test-killaura-critical"
New-Item -ItemType Directory -Force $output | Out-Null
Add-Type -Path "$client/test/lunar12111/Audit.cs"
Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach($profile in @(@('forge','forge1201.inc',$ForgeJar),@('modern','minecraft263.inc',$ModernJar))) {
    $text=Get-Content "$client/src/minecraft/compat/mappings/$($profile[1])" -Raw
    $section=($text -split '#if defined\(NOVA_CRITICAL_MAPPING\)',2)[1].Split('#elif')[0]
    $rows=foreach($m in [regex]::Matches($section,'NOVA_CRITICAL\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)')) {
        $id,$owner,$name,$desc=$m.Groups[1..4].Value
        @("extra:$id",$owner,$name,$desc,$owner,$name,$desc) -join "`t"
    }
    if($rows.Count -ne 14){throw 'Incomplete critical binding catalog'}
    $rows += @('extra:web','','','','net.minecraft.world.level.block.WebBlock','','') -join "`t"
    $bindings="$output/$($profile[0]).tsv"
    [IO.File]::WriteAllLines($bindings,$rows)
    if([LunarClassAudit]::Check($profile[2],$bindings,"$client/src/minecraft","$output/no-exclusions")){throw "$($profile[0]) critical bindings failed"}
    $zip=[IO.Compression.ZipFile]::OpenRead($profile[2])
    try {
        $entry=$zip.GetEntry('net/minecraft/client/player/LocalPlayer.class')
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,"$output/$($profile[0]).class",$true)
    } finally { $zip.Dispose() }
}
$build=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/CriticalHookTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe forge forge.class
if errorlevel 1 exit /b 1
hooks.exe modern modern.class
exit /b %ERRORLEVEL%
"@
Set-Content "$output/build.cmd" $build -Encoding ascii
Push-Location $output
try { & ./build.cmd; if($LASTEXITCODE){throw 'Critical hook validation failed'} }
finally { Pop-Location }
