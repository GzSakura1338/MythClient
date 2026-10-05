$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-backtrack'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
$prefix = Get-Content (Join-Path $PSScriptRoot 'Test.prefix.cpp') -Raw
$suffix = Get-Content (Join-Path $PSScriptRoot 'Test.suffix.cpp') -Raw
$events = (Get-Content (Join-Path $clientRoot 'src/minecraft/events/GameEvents.h') -Raw) -replace '(?m)^#.*\r?\n', ''
$source = (Get-Content (Join-Path $clientRoot 'src/minecraft/modules/combat/backtrack/Backtrack.cpp') -Raw) -replace '(?m)^#include.*\r?\n', ''
$source = $source.Replace('using Clock = std::chrono::steady_clock;', 'using Clock = mock::Clock;')
[IO.File]::WriteAllText((Join-Path $testBuild 'test.cpp'), $prefix + "`n" + $events + "`n" + $source + "`n" + $suffix)
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /O2 /I..\..\src test.cpp ..\..\src\minecraft\events\EventBus.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /O2 /I..\..\src "$PSScriptRoot/EspClipTest.cpp" /Fe:esp.exe
if errorlevel 1 exit /b 1
esp.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText((Join-Path $testBuild 'build.cmd'), $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "Backtrack verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
