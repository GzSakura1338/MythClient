$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-hypixel'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
& python (Join-Path $PSScriptRoot 'verify_hypixel_bindings.py')
if ($LASTEXITCODE -ne 0) { throw 'Hypixel mapping verification failed' }
$core = (Get-Content "$clientRoot/src/minecraft/modules/combat/autoblock/HypixelAutoBlock.h" -Raw) -replace '(?m)^#.*\r?\n', ''
$header = (Get-Content "$clientRoot/src/minecraft/modules/combat/autoblock/AutoBlock.h" -Raw) -replace '(?m)^#.*\r?\n', ''
# Only settings types are needed by the real property definitions.
$header = $header.Substring(0, $header.IndexOf('class AutoBlock {')) + "`n}`n"
$settings = (Get-Content "$clientRoot/src/minecraft/modules/combat/autoblock/AutoBlockModuleBase.h" -Raw) -replace '(?m)^#.*\r?\n', ''
$fixture = Get-Content "$PSScriptRoot/HypixelPropertiesTest.cpp" -Raw
[IO.File]::WriteAllText((Join-Path $testBuild 'properties.cpp'),
    $fixture.Replace('// @AUTOBLOCK_SETTINGS@', $core + "`n" + $header + "`n" + $settings))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" "$PSScriptRoot/HypixelAutoBlockTest.cpp" /Fe:hypixel.exe
if errorlevel 1 exit /b 1
hypixel.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/include" /I "$clientRoot/src" /I "$clientRoot/test/bytecode" "$PSScriptRoot/HypixelHooksTest.cpp" "$clientRoot/src/minecraft/java/bytecode/ClassEdit.cpp" "$clientRoot/src/minecraft/java/bytecode/ClassFile.cpp" "$clientRoot/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" properties.cpp "$clientRoot/src/utils/Json.cpp" /Fe:properties.exe
if errorlevel 1 exit /b 1
properties.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText((Join-Path $testBuild 'build.cmd'), $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "Hypixel verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
