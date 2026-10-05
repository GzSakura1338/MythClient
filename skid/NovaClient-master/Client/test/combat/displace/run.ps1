$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-displace'
New-Item -ItemType Directory -Force $out | Out-Null
& python "$PSScriptRoot/verify_mappings.py"
if ($LASTEXITCODE -ne 0) { throw 'Displace mapping verification failed' }
$source = (Get-Content "$client/src/minecraft/modules/combat/displace/DisplacePackets.cpp" -Raw) -replace '(?m)^#include.*\r?\n', ''
$fixture = (Get-Content "$PSScriptRoot/DisplacePacketsTest.cpp" -Raw).Replace('// @PRODUCTION_PACKETS@', $source)
[IO.File]::WriteAllText("$out/packets.cpp", $fixture)
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I "$client/src" /I "$client/include" "$PSScriptRoot/DisplaceTest.cpp" /Fe:test.exe
if not %ERRORLEVEL%==0 exit /b 1
test.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD packets.cpp /Fe:packets.exe
if not %ERRORLEVEL%==0 exit /b 1
packets.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I "$client/src" /I "$client/include" "$PSScriptRoot/DisplaceHookTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if not %ERRORLEVEL%==0 exit /b 1
hooks.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'Displace verification failed' } }
finally { Pop-Location }
