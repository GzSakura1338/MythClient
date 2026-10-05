$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-antikb-mix'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
$runtime = (Get-Content "$clientRoot/src/minecraft/modules/combat/antikb/mix/MixRuntime.cpp" -Raw) -replace '(?m)^#include.*\r?\n', ''
$events = (Get-Content "$clientRoot/src/minecraft/events/GameEvents.h" -Raw) -replace '(?m)^#.*\r?\n', ''
$fixture = Get-Content "$PSScriptRoot/MixRuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$testBuild/runtime.cpp", $fixture.Replace('// @EVENTS@', $events).Replace('// @RUNTIME@', $runtime))
$base = Get-Content "$PSScriptRoot/../autoblock/HypixelPropertiesTest.cpp" -Raw
$base = $base.Substring(0, $base.IndexOf('// @AUTOBLOCK_SETTINGS@'))
$properties = (Get-Content "$clientRoot/src/minecraft/modules/combat/antikb/mix/MixModuleBase.h" -Raw) -replace '(?m)^#.*\r?\n', ''
$propertyFixture = Get-Content "$PSScriptRoot/MixPropertiesTest.cpp" -Raw
[IO.File]::WriteAllText("$testBuild/properties.cpp", "#include `"minecraft/modules/combat/antikb/mix/MixVelocity.h`"`n" + $base + $propertyFixture.Replace('// @MIX_PROPERTIES@', $properties))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" runtime.cpp "$clientRoot/src/minecraft/events/EventBus.cpp" /Fe:runtime.exe
if not %ERRORLEVEL%==0 exit /b 1
runtime.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" properties.cpp "$clientRoot/src/utils/Json.cpp" /Fe:properties.exe
if not %ERRORLEVEL%==0 exit /b 1
properties.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$testBuild/build.cmd", $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "Mix verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
