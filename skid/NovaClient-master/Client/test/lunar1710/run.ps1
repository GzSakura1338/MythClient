param([string]$Bake = 'C:/Users/13366/.lunarclient/offline/multiver/cache/10d9b7f1/aed3ca70/bake.zip')
$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output = "$client/build-vs2026/test-lunar1710"
New-Item -ItemType Directory -Force "$output/classes" | Out-Null
python "$PSScriptRoot/audit.py" $Bake Lunar1710.inc
if ($LASTEXITCODE) { throw 'Lunar 1.7 JNI mapping audit failed' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($Bake)
try {
    foreach ($entry in $zip.Entries) {
        if ($entry.FullName.StartsWith('net.minecraft.')) {
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, "$output/classes/$($entry.FullName)", $true)
        }
    }
} finally { $zip.Dispose() }
$build = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" /I "$client/test/bytecode" "$PSScriptRoot/Hooks.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe
exit /b %ERRORLEVEL%
"@
Set-Content "$output/build.cmd" $build -Encoding ascii
Push-Location $output
try { & ./build.cmd; if ($LASTEXITCODE) { throw 'Lunar 1.7 hook rewrite audit failed' } }
finally { Pop-Location }
$jvm = Get-ChildItem "$env:USERPROFILE/.lunarclient/jre" -Filter jvm.dll -Recurse | Where-Object FullName -Match 'zulu17' | Select-Object -First 1
if (!$jvm) { throw 'Lunar Java 17 runtime not found for isolated enum verification' }
$facingBuild = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" /I "$client/test/bytecode" /I "$client/src/third_party/jni" "$PSScriptRoot/Facing.cpp" /Fe:facing.exe
if errorlevel 1 exit /b 1
facing.exe "$($jvm.FullName)"
exit /b %ERRORLEVEL%
"@
Set-Content "$output/facing-build.cmd" $facingBuild -Encoding ascii
Push-Location $output
try { & ./facing-build.cmd; if ($LASTEXITCODE) { throw 'Lunar 1.7 facing semantics failed' } }
finally { Pop-Location }
