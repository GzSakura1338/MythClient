$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-breaker'
& python (Join-Path $PSScriptRoot 'verify.py')
if ($LASTEXITCODE -ne 0) { throw 'Breaker mapping verification failed' }
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/LogicTest.cpp" /Fe:logic.exe
if errorlevel 1 exit /b 1
logic.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/HooksTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe lunar lunar.class
if errorlevel 1 exit /b 1
hooks.exe forge forge.class
if errorlevel 1 exit /b 1
hooks.exe 263 263.class
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw "Breaker tests failed ($LASTEXITCODE)" } }
finally { Pop-Location }
