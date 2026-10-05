$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-prediction'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
$runtime = Get-Content "$clientRoot/src/minecraft/compat/lunar189/LunarRuntime.cpp" -Raw
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
$blinkStart = $runtime.IndexOf('struct BlockPacket {')
$blinkEnd = $runtime.IndexOf('bool Good(', $blinkStart)
$blink = $runtime.Substring($blinkStart, $blinkEnd - $blinkStart)
foreach ($signature in @('bool SendDisplacePacket(', 'bool DispatchClientOutput(', 'void DrainBlockPackets(', 'void SendOldestBlockPacket(', 'bool SlinkyPacketFlush(',
    'bool HoldBlockPacket(', 'bool SendWatchdogPacket(', 'void DrainWatchdogPackets(',
    'void ClearWatchdogPhase(', 'void MaintainWatchdogPhase(', 'bool StageWatchdogPacket(', 'bool BeginSlinkyBlockLag(', 'void EndSlinkyBlockLag(',
    'void DrainSlinkyBlockLag()', 'bool SlinkyBlockLagging()', 'bool ConsumeSlinkyAttackRelease()', 'void BeginHypixelBlockLag(',
    'void EndHypixelBlockLag(', 'bool HypixelBlockLagging()', 'bool BeginPredictionBlink(', 'void EndPredictionBlink(',
    'void ReleaseBlockLag(', 'void EndBlockLag(', 'void JNICALL AttackMotionXHook(', 'void JNICALL AttackMotionZHook(')) {
    $blink += "`n" + (Extract-Definition $runtime $signature)
}
$fixture = Get-Content "$PSScriptRoot/PredictionBlinkTest.cpp" -Raw
[IO.File]::WriteAllText("$testBuild/blink.cpp", $fixture.Replace('// @BLINK_PRODUCTION@', $blink))
$incomingSource = Get-Content "$clientRoot/src/minecraft/modules/combat/antikb/prediction/PredictionRuntime.cpp" -Raw
$incomingStart = $incomingSource.IndexOf('struct Runtime {')
$incomingEnd = $incomingSource.IndexOf('    struct IO {', $incomingStart)
$incoming = $incomingSource.Substring($incomingStart, $incomingEnd - $incomingStart)
foreach ($signature in @('void Publish(', 'void Drain(', 'bool ReleaseExpiredDelay(', 'void Reset(', 'bool Ready(', 'bool Keep(', 'void Packet(')) {
    $incoming += "`n" + (Extract-Definition $incomingSource $signature)
}
$incoming += "`n};`n"
$incomingFixture = Get-Content "$PSScriptRoot/PredictionIncomingTest.cpp" -Raw
[IO.File]::WriteAllText("$testBuild/incoming.cpp", $incomingFixture.Replace('// @INCOMING_PRODUCTION@', $incoming))
$core = (Get-Content "$clientRoot/src/minecraft/modules/combat/antikb/prediction/PredictionVelocity.h" -Raw) -replace '(?m)^#.*\r?\n', ''
$properties = (Get-Content "$clientRoot/src/minecraft/modules/combat/antikb/prediction/PredictionModuleBase.h" -Raw) -replace '(?m)^#.*\r?\n', ''
$propertyFixture = Get-Content "$PSScriptRoot/PredictionPropertiesTest.cpp" -Raw
$propertyBase = Get-Content "$PSScriptRoot/../autoblock/HypixelPropertiesTest.cpp" -Raw
$propertyBase = $propertyBase.Substring(0, $propertyBase.IndexOf('// @AUTOBLOCK_SETTINGS@'))
[IO.File]::WriteAllText("$testBuild/properties.cpp", $propertyBase + "`n" + $core + "`n" + $properties + "`n" + $propertyFixture)
# Exercise production adapters rather than substituting boolean IO outcomes.
$events = Get-Content "$clientRoot/src/minecraft/events/GameEvents.h" -Raw
$nativeFixture = Get-Content "$PSScriptRoot/PredictionNativeTest.cpp" -Raw
$nativeFixture = $nativeFixture.Replace('// @MOVE_EVENT@', (Extract-Definition $events 'class MoveInputEvent :') + ';')
$nativeFixture = $nativeFixture.Replace('// @MOVE_CALLBACK@', (Extract-Definition $incomingSource 'void MoveInput('))
$nativeDefinitions = ''
foreach ($signature in @('bool PhysicalUseDown(', 'bool PlayerReportedRotation(', 'int PredictionIncomingKind(', 'void JNICALL InputHook(')) {
    $nativeDefinitions += "`n" + (Extract-Definition $runtime $signature)
}
[IO.File]::WriteAllText("$testBuild/native.cpp", $nativeFixture.Replace('// @NATIVE_PRODUCTION@', $nativeDefinitions))
$raySource = Get-Content "$clientRoot/src/minecraft/utils/rotation/RayCastUtil.cpp" -Raw
$rayDefinitions = ''
foreach ($signature in @('static HitResult RayCastWithRules(const Rotation& rotation',
    'HitResult RiseRayCast(', 'HitResult LeaderRayCast(', 'HitResult RayCast(const Rotation& rotation, double range, float expand,',
    'HitResult RayCast(const Rotation& rotation, double range)')) {
    $rayDefinitions += "`n" + (Extract-Definition $raySource $signature)
}
$rayFixture = Get-Content "$PSScriptRoot/LeaderRayAdapterTest.cpp" -Raw
$rayFixture = $rayFixture.Replace('// @RAY_PRODUCTION@', $rayDefinitions)
$auraSource = Get-Content "$clientRoot/src/minecraft/modules/combat/killaura/KillAura.cpp" -Raw
$rayFixture = $rayFixture.Replace('// @BLOCK_TARGET@', (Extract-Definition $auraSource 'bool HasBlockTarget(double'))
$predictionIO = ''
foreach ($signature in @('void ComputeRotation(', 'void Rotate(', 'bool RayPlayer(')) {
    $predictionIO += "`n" + (Extract-Definition $incomingSource $signature)
}
$rayFixture = $rayFixture.Replace('// @PREDICTION_IO@', $predictionIO)
$hypixelSource = Get-Content "$clientRoot/src/minecraft/modules/combat/autoblock/AutoBlockHypixel.cpp" -Raw
$geometry = ''
foreach ($signature in @('void CaptureGeometry(', 'double DistanceSq(', 'bool InSwingRange(', 'bool Ray(', 'void InteractBlock(')) {
    $geometry += "`n" + (Extract-Definition $hypixelSource $signature)
}
[IO.File]::WriteAllText("$testBuild/ray.cpp", $rayFixture.Replace('// @HYPIXEL_GEOMETRY@', $geometry))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" "$PSScriptRoot/PredictionVelocityTest.cpp" /Fe:prediction.exe
if not %ERRORLEVEL%==0 exit /b 1
prediction.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 blink.cpp /Fe:blink.exe
if not %ERRORLEVEL%==0 exit /b 1
blink.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" incoming.cpp /Fe:incoming.exe
if not %ERRORLEVEL%==0 exit /b 1
incoming.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" properties.cpp "$clientRoot/src/utils/Json.cpp" /Fe:properties.exe
if not %ERRORLEVEL%==0 exit /b 1
properties.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" native.cpp /Fe:native.exe
if not %ERRORLEVEL%==0 exit /b 1
native.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" ray.cpp /Fe:ray.exe
if not %ERRORLEVEL%==0 exit /b 1
ray.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText((Join-Path $testBuild 'build.cmd'), $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "Prediction verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
