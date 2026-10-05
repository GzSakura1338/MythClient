$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-watchdog-slinky'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
function Extract-Definition([string]$source, [string]$signature) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing production definition: $signature" }
    $open = $source.IndexOf('{', $start)
    $depth = 1; $end = $open + 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth) { throw "Unclosed production definition: $signature" }
    return $source.Substring($start, $end - $start)
}
$runtime = Get-Content "$clientRoot/src/minecraft/compat/lunar189/LunarRuntime.cpp" -Raw
$start = $runtime.IndexOf('struct BlockPacket {')
$blink = $runtime.Substring($start, $runtime.IndexOf('bool Good(', $start) - $start)
foreach ($signature in @('bool SendDisplacePacket(', 'bool DispatchClientOutput(', 'void DrainBlockPackets(', 'void SendOldestBlockPacket(', 'bool SlinkyPacketFlush(',
    'bool HoldBlockPacket(', 'bool SendWatchdogPacket(', 'void DrainWatchdogPackets(',
    'void ClearWatchdogPhase(', 'void MaintainWatchdogPhase(', 'bool StageWatchdogPacket(', 'bool BeginSlinkyBlockLag(', 'void EndSlinkyBlockLag(',
    'void DrainSlinkyBlockLag()', 'bool SlinkyBlockLagging()', 'bool ConsumeSlinkyAttackRelease()',
    'void BeginBlockLag(', 'void BeginHypixelBlockLag(', 'void EndHypixelBlockLag(', 'bool HypixelBlockLagging()', 'bool BeginPredictionBlink(', 'void EndPredictionBlink(')) {
    $blink += "`n" + (Extract-Definition $runtime $signature)
}
$blinkFixture = Get-Content "$PSScriptRoot/PredictionBlinkTest.cpp" -Raw
$blinkFixture = $blinkFixture.Substring(0, $blinkFixture.IndexOf('int main()'))
$blinkFixture = $blinkFixture -replace '(?m)^#include.*\r?\n', ''
$blinkFixture = $blinkFixture.Replace('// @BLINK_PRODUCTION@', $blink)
$prefix = Get-Content "$PSScriptRoot/AntiKbDelayTest.prefix.cpp" -Raw
$events = (Get-Content "$clientRoot/src/minecraft/events/GameEvents.h" -Raw) -replace '(?m)^#.*\r?\n', ''
$antiKb = (Get-Content "$clientRoot/src/minecraft/modules/combat/antikb/AntiKB.cpp" -Raw) -replace '(?m)^#include.*\r?\n', ''
$helpers = Get-Content "$PSScriptRoot/AntiKbDelayTest.suffix.cpp" -Raw
$helpers = $helpers.Substring(0, $helpers.IndexOf('int main()'))
$cases = Get-Content "$PSScriptRoot/WatchdogSlinkyTest.cpp" -Raw
$slinkySource = Get-Content "$clientRoot/src/minecraft/modules/combat/autoblock/AutoBlockPacketSync.cpp" -Raw
$slinkyPolicy = "namespace nova::module::autoblock { java::EntityHandle delayPlayer, delayWorld;`n" + (Extract-Definition $slinkySource 'void SynchronizePacketDelays(') + "`n}`n"
$source = "#include <cassert>`n#include <cstdint>`n#include <iostream>`n" + $prefix + "`n" + $events + "`n" + $antiKb + "`n" + $helpers + "`nnamespace blink {`n" + $blinkFixture + "`n}`n" + $slinkyPolicy + $cases
[IO.File]::WriteAllText("$testBuild/test.cpp", $source)
$phaseCases = Get-Content "$PSScriptRoot/WatchdogPacketPhaseTest.cpp" -Raw
[IO.File]::WriteAllText("$testBuild/phase.cpp", $source.Substring(0, $source.IndexOf('int main()')) + $phaseCases)
$lagIo = Extract-Definition (Get-Content "$clientRoot/src/minecraft/modules/combat/autoblock/AutoBlockLagNew.cpp" -Raw) 'struct LagNewIO {'
$lagCases = (Get-Content "$PSScriptRoot/LagNewWatchdogTest.cpp" -Raw).Replace('// @LAG_NEW_IO@', $lagIo + ';')
[IO.File]::WriteAllText("$testBuild/lag-new.cpp", $source.Substring(0, $source.IndexOf('int main()')) + $lagCases)
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /O2 /I "$clientRoot/src" test.cpp "$clientRoot/src/minecraft/events/EventBus.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /O2 /I "$clientRoot/src" phase.cpp "$clientRoot/src/minecraft/events/EventBus.cpp" /Fe:phase.exe
if errorlevel 1 exit /b 1
phase.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /O2 /I "$clientRoot/src" lag-new.cpp "$clientRoot/src/minecraft/events/EventBus.cpp" /Fe:lag-new.exe
if errorlevel 1 exit /b 1
lag-new.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$testBuild/build.cmd", $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "Watchdog/Slinky verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
