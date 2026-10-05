$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-automlg"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$client/src/minecraft/modules/player/automlg/AutoMLG.cpp" -Raw
$source=$source -replace '(?m)^#include[^\r\n]*\r?\n',''
$source=$source.Replace('using Clock=std::chrono::steady_clock;','using Clock=mock::Clock;')
$fixture=Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$out/runtime.cpp",$fixture.Replace('// @PRODUCTION_MODULE@',$source))
Add-Type -AssemblyName System.IO.Compression.FileSystem
$jar="$env:APPDATA/.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar"
$zip=[IO.Compression.ZipFile]::OpenRead($jar)
try{[IO.Compression.ZipFileExtensions]::ExtractToFile($zip.GetEntry('net/minecraft/client/Minecraft.class'),"$out/Minecraft.class",$true)}finally{$zip.Dispose()}
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client" /I "$client/src" /I "$client/include" "$PSScriptRoot/ForgeHookTest.cpp" "$client/src/minecraft/hooks/Edits.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" /Fe:forge-hook.exe
if errorlevel 1 exit /b 1
forge-hook.exe Minecraft.class
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try {& ./build.cmd;if($LASTEXITCODE){throw 'AutoMLG regression failed'}}finally{Pop-Location}
