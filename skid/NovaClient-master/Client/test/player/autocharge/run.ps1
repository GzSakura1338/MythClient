param([ValidateSet('All','Airborne','Runtime','Target')][string]$Suite='All')
$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$output = "$client/build-vs2026/test-autocharge"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = Get-Content "$client/src/minecraft/modules/player/autocharge/AutoCharge.cpp" -Raw
$source = $source -replace '(?m)^#include[^\r\n]*\r?\n',''
$fixture = Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$output/runtime.cpp", $fixture.Replace('// @PRODUCTION_MODULE@', $source))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
if "$Suite"=="Target" goto target
if "$Suite"=="Runtime" goto runtime
if "$Suite"=="Airborne" goto airborne
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/TrajectoryTest.cpp" "$client/src/minecraft/utils/rotation/RotationUtils.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/TwentyBlocksTest.cpp" "$client/src/minecraft/utils/rotation/RotationUtils.cpp" /Fe:twenty.exe
if errorlevel 1 exit /b 1
twenty.exe
if errorlevel 1 exit /b 1
:airborne
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/AirborneTest.cpp" "$client/src/minecraft/utils/rotation/RotationUtils.cpp" /Fe:airborne.exe
if errorlevel 1 exit /b 1
airborne.exe
if errorlevel 1 exit /b 1
:runtime
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp "$client/src/minecraft/utils/rotation/RotationUtils.cpp" /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
if errorlevel 1 exit /b 1
:target
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/TargetTest.cpp" "$client/src/minecraft/utils/rotation/RotationUtils.cpp" /Fe:target.exe
if errorlevel 1 exit /b 1
target.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$output/build.cmd", $commands)
Push-Location $output
try {
    & ./build.cmd
    if ($LASTEXITCODE) { throw "AutoCharge regression failed: $LASTEXITCODE" }
} finally { Pop-Location }
