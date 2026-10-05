$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$output = "$client/build-vs2026/test-elytrafly"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = Get-Content "$client/src/minecraft/modules/movement/elytrafly/ElytraFly.cpp" -Raw
$source = $source -replace '(?m)^#include[^\r\n]*\r?\n',''
$fixture = Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$output/runtime.cpp", $fixture.Replace('// @PRODUCTION_MODULE@', $source))
$fireSource = Get-Content "$client/src/minecraft/modules/movement/elytrafly/AutoFire.cpp" -Raw
$fireSource = $fireSource -replace '(?m)^#include[^\r\n]*\r?\n',''
$fireFixture = Get-Content "$PSScriptRoot/AutoFireRuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$output/autofire.cpp", $fireFixture.Replace('// @PRODUCTION_AUTOFIRE@', $fireSource))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/Test.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" autofire.cpp /Fe:autofire.exe
if errorlevel 1 exit /b 1
autofire.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$output/build.cmd", $commands)
Push-Location $output
try {
    & ./build.cmd
    if ($LASTEXITCODE) { throw "ElytraFly regression failed: $LASTEXITCODE" }
} finally { Pop-Location }
