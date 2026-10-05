$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-lunar-incoming'
$jvm = Get-ChildItem "$env:USERPROFILE/.lunarclient/jre" -Filter jvm.dll -Recurse | Where-Object FullName -Match 'zulu17' | Select-Object -First 1
if (!$jvm) { throw 'Lunar Java 17 runtime not found for isolated bytecode verification' }
& python (Join-Path $PSScriptRoot 'verify.py')
if ($LASTEXITCODE -ne 0) { throw 'Lunar incoming mapping verification failed' }
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" /I "$client/src/third_party/jni" "$PSScriptRoot/HooksTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe PacketTask.class "$($jvm.FullName)"
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw "Lunar incoming tests failed ($LASTEXITCODE)" } }
finally { Pop-Location }
