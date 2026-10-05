$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-freelook'
New-Item -ItemType Directory -Force $out | Out-Null
& python "$PSScriptRoot/verify_mappings.py"
if ($LASTEXITCODE -ne 0) { throw 'FreeLook mapping verification failed' }
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/FreeLookTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe lunar classes/lunar
if errorlevel 1 exit /b 1
test.exe forge classes/forge
if errorlevel 1 exit /b 1
test.exe 263 classes/263
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'FreeLook native verification failed' } }
finally { Pop-Location }
