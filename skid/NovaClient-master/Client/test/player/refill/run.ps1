$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-refill'
New-Item -ItemType Directory -Force $out | Out-Null
& python "$client/test/combat/autopot/verify_mappings.py" NOVA_INVENTORY_SCREEN
if ($LASTEXITCODE -ne 0) { throw 'Inventory screen mapping verification failed' }
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I "$client/src" "$PSScriptRoot/RefillTest.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'Refill native verification failed' } }
finally { Pop-Location }
