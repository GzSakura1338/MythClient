$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$output = "$client/build-vs2026/test-autopearl"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = Get-Content "$client/src/minecraft/modules/player/autopearl/AutoPearl.cpp" -Raw
$source = $source -replace '(?m)^#include[^\r\n]*\r?\n',''
$fixture = Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$output/runtime.cpp", $fixture.Replace('// @PRODUCTION_MODULE@', $source))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$output/build.cmd", $commands)
Push-Location $output
try { & ./build.cmd; if ($LASTEXITCODE) { throw "AutoPearl regression failed: $LASTEXITCODE" } }
finally { Pop-Location }
