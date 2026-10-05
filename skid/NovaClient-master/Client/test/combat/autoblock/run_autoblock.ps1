$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-autoblock-vanilla'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
& python (Join-Path $PSScriptRoot 'verify_slinky_bindings.py')
if ($LASTEXITCODE -ne 0) { throw 'Slinky mapping verification failed' }
$prefix = Get-Content (Join-Path $PSScriptRoot 'AutoBlockTest.prefix.cpp') -Raw
$suffix = Get-Content (Join-Path $PSScriptRoot 'AutoBlockTest.suffix.cpp') -Raw
$hypixel = (Get-Content (Join-Path $clientRoot 'src/minecraft/modules/combat/autoblock/HypixelAutoBlock.h') -Raw) -replace '(?m)^#.*\r?\n', ''
$prefix += "`n" + $hypixel
$prefix += "`n" + ((Get-Content "$clientRoot/src/minecraft/modules/combat/autoblock/LagNewAutoBlock.h" -Raw) -replace '(?m)^#.*\r?\n', '')
$header = (Get-Content (Join-Path $clientRoot 'src/minecraft/modules/combat/autoblock/AutoBlock.h') -Raw) -replace '(?m)^#.*\r?\n', ''
$source = (Get-Content (Join-Path $clientRoot 'src/minecraft/modules/combat/autoblock/AutoBlock.cpp') -Raw) -replace '(?m)^#include.*\r?\n', ''
$source += (Get-Content (Join-Path $clientRoot 'src/minecraft/modules/combat/autoblock/AutoBlockSlinky.cpp') -Raw) -replace '(?m)^#include.*\r?\n', ''
$source += @'
namespace nova::module {
void AutoBlock::ResetHypixel() {}
void AutoBlock::ResetLagNew() { lagNew_ = {}; }
void AutoBlock::TickLagNew(bool, bool) { ++mock::lagNewTicks; }
void AutoBlock::PrepareHypixelInput() { ++mock::hypixelPrepares; }
bool AutoBlock::VelocityCanReduce(int) const { return true; }
bool AutoBlock::VelocityShouldBlock() const { return false; }
}
'@
[IO.File]::WriteAllText((Join-Path $testBuild 'test.cpp'), $prefix + "`n" + $header + "`n" + $source + "`n" + $suffix)
$slinkyTests = Get-Content (Join-Path $PSScriptRoot 'SlinkyAutoBlockTest.cpp') -Raw
[IO.File]::WriteAllText((Join-Path $testBuild 'slinky.cpp'), $prefix + "`n" + $header + "`n" + $source + "`n" + $slinkyTests)
$runtimeSource = Get-Content (Join-Path $clientRoot 'src/minecraft/modules/combat/autoblock/AutoBlockModule.cpp') -Raw -Encoding UTF8
$runtimeStart = $runtimeSource.IndexOf('enum class Owner')
$runtimeEnd = $runtimeSource.IndexOf('class AutoBlockModule final', $runtimeStart)
$apiStart = $runtimeSource.IndexOf('namespace autoblock {', $runtimeEnd)
$apiEnd = $runtimeSource.IndexOf('std::unique_ptr<Module> MakeAutoBlockModule', $apiStart)
$runtime = "namespace nova::module { namespace {`n" + $runtimeSource.Substring($runtimeStart, $runtimeEnd - $runtimeStart) +
    "}`n" + $runtimeSource.Substring($apiStart, $apiEnd - $apiStart) + "}`n"
$runtimeMocks = @'
namespace nova::clickgui { bool open = false; bool IsOpen() { return open; } }
namespace nova::module::scaffold { bool IsEnabled() { return false; } }
namespace nova::module::antikb { int AttackQueue() { return 0; } }
namespace nova::module::killaura { bool blockTarget = false; bool HasTargets() { return false; } bool HasBlockTarget() { return blockTarget; } }
'@
$runtimeTests = Get-Content (Join-Path $PSScriptRoot 'AutoBlockRuntimeTest.cpp') -Raw -Encoding UTF8
[IO.File]::WriteAllText((Join-Path $testBuild 'runtime.cpp'),
    $prefix + "`n" + $header + "`n" + $source + "`n" + $runtimeMocks + "`n" + $runtime + "`n" + $runtimeTests)
$nativeSource = Get-Content "$clientRoot/src/minecraft/compat/lunar189/LunarRuntime.cpp" -Raw -Encoding UTF8
$hooks = foreach ($signature in @('void JNICALL BlockVanillaUseHook(', 'jboolean JNICALL BlockManualSwordUseHook(', 'void JNICALL BlockMouseKeyHook(')) {
    $start = $nativeSource.IndexOf($signature)
    if ($start -lt 0) { throw "Missing native hook: $signature" }
    $open = $nativeSource.IndexOf('{', $start)
    $depth = 1; $end = $open + 1
    while ($depth -gt 0 -and $end -lt $nativeSource.Length) {
        if ($nativeSource[$end] -eq '{') { ++$depth }
        if ($nativeSource[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Unclosed native hook: $signature" }
    $nativeSource.Substring($start, $end - $start)
}
$hooksFixture = Get-Content "$PSScriptRoot/ManualBlockHooksTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText((Join-Path $testBuild 'manual-hooks.cpp'),
    $hooksFixture.Replace('// @PRODUCTION_MANUAL_BLOCK_HOOKS@', ($hooks -join "`n")))
$commands = @'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /O2 test.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /O2 runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 slinky.cpp /Fe:slinky.exe
if errorlevel 1 exit /b 1
slinky.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 manual-hooks.cpp /Fe:manual-hooks.exe
if errorlevel 1 exit /b 1
manual-hooks.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "../../src" "../../test/combat/autoblock/LagNewAutoBlockTest.cpp" /Fe:lag-new.exe
if errorlevel 1 exit /b 1
lag-new.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText((Join-Path $testBuild 'build.cmd'), $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "AutoBlock verification failed ($LASTEXITCODE)" }
} finally {
    Pop-Location
}
