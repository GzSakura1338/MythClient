$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-bridgeassist'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
$prefix = Get-Content (Join-Path $PSScriptRoot 'Test.prefix.cpp') -Raw -Encoding UTF8
$suffix = Get-Content (Join-Path $PSScriptRoot 'Test.suffix.cpp') -Raw -Encoding UTF8
$source = (Get-Content (Join-Path $clientRoot 'src/minecraft/modules/player/bridgeassist/BridgeAssist.cpp') -Raw -Encoding UTF8) -replace '(?m)^#include.*\r?\n', ''
[IO.File]::WriteAllText((Join-Path $testBuild 'test.cpp'), $prefix + "`n" + $source + "`n" + $suffix)
$commands = @'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I..\..\src /I..\..\include test.cpp ..\..\src\utils\Json.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I..\..\src /I..\..\include ..\..\test\player\bridgeassist\SimulationTest.cpp /Fe:simulation.exe
if errorlevel 1 exit /b 1
simulation.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText((Join-Path $testBuild 'build.cmd'), $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "BridgeAssist verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
