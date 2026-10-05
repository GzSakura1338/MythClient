$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-scaffold'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
$source = Get-Content (Join-Path $clientRoot 'src/minecraft/modules/player/scaffold/Scaffold.cpp') -Raw
function Extract-Method([string]$signature, [string]$sourceText = $source) {
    $start = $sourceText.IndexOf($signature)
    if ($start -lt 0) { throw "Missing production method: $signature" }
    $open = $sourceText.IndexOf('{', $start)
    $depth = 1
    $end = $open + 1
    while ($depth -gt 0 -and $end -lt $sourceText.Length) {
        if ($sourceText[$end] -eq '{') { ++$depth }
        if ($sourceText[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Unterminated production method: $signature" }
    return $sourceText.Substring($start, $end - $start) + "`n"
}
$helperMethods = ''
foreach ($signature in @('inline float AngleDelta(', 'inline float SnapTellyYaw(',
                         'inline rotation::Rotation StepTellyRotation(')) {
    $helperMethods += Extract-Method $signature
}
$helperMethods += "template <typename Matches>`n" + (Extract-Method 'bool FindWatchdogPlacement(')
$wireBegin = $source.IndexOf('// @SNAP_WIRE_BEGIN@')
$wireEnd = $source.IndexOf('// @SNAP_WIRE_END@')
if ($wireBegin -lt 0 -or $wireEnd -le $wireBegin) { throw 'Missing Snap packet history' }
$wire = $source.Substring($wireBegin, $wireEnd - $wireBegin)
$helpers = "#include <mutex>`nnamespace nova::module::scaffold { namespace {`n" + $wire + $helperMethods + "`n} }`n"
$watchdog = Get-Content (Join-Path $PSScriptRoot 'WatchdogTest.cpp') -Raw
[IO.File]::WriteAllText((Join-Path $testBuild 'watchdog.cpp'), $watchdog.Replace('// @HYPIXEL_HELPERS@', $helpers))
$methods = ''
foreach ($signature in @('bool SnapActive()', 'void ResetSnap()', 'bool BeginSnapTick()',
                         'bool FindSnapRotation(',
                         'bool SyncSnapPacketHistory()', 'void OnSnapMotion(', 'void HandleSnap()', 'rotation::Vec3 HitVectorForSnap()',
                         'bool TellyActive()', 'bool HypixelTelly()', 'bool UseBoundedRotation()',
                         'void ReleaseSlot()', 'bool SelectSlot(', 'void OnPreTick(', 'void OnMoveInput(', 'void HandleNormal()', 'void HandleTelly()', 'void HandleWatchdogTelly()', 'void ComputeRotation()', 'void Place(')) {
    $methods += Extract-Method $signature
}
$fixture = Get-Content (Join-Path $PSScriptRoot 'TellyTest.cpp') -Raw
$fixture = $fixture.Replace('// @HYPIXEL_HELPERS@', $helpers)
$rotationSource = Get-Content (Join-Path $clientRoot 'src/minecraft/utils/rotation/RotationManager.cpp') -Raw
$fixture = $fixture.Replace('// @PRODUCTION_JUMP@', (Extract-Method 'void OnJump(' $rotationSource))
[IO.File]::WriteAllText((Join-Path $testBuild 'telly.cpp'), $fixture.Replace('// @PRODUCTION_METHODS@', $methods))
$placement = Get-Content (Join-Path $PSScriptRoot 'PlacementTest.cpp') -Raw
$placement = $placement.Replace('// @HYPIXEL_HELPERS@', $helpers)
$fallMethods = ''
foreach ($signature in @('int GetYLevel()', 'bool OnAir()', 'rotation::Rotation GetRotation()', 'void ComputeRotation()', 'void HandleTelly()', 'void Place(')) {
    $fallMethods += Extract-Method $signature
}
$placement = $placement.Replace('// @FALL_METHODS@', $fallMethods)
$raySource = Get-Content (Join-Path $clientRoot 'src/minecraft/utils/rotation/RayCastUtil.cpp') -Raw
$rayMethods = ''
foreach ($signature in @('bool OverBlock(const Rotation& rotation, BlockFace facing,',
                         'bool OverBlock(const Rotation& rotation, const java::BlockPos& pos)',
                         'bool OverBlock(const Rotation& rotation, const java::BlockPos& pos, BlockFace facing)')) {
    $rayMethods += Extract-Method $signature $raySource
}
$placement = $placement.Replace('// @OVER_BLOCK@', $rayMethods)
$search = Extract-Method 'void GetBlockInfo('
$placement = $placement.Replace('// @PLACEMENT_SEARCH@', $search)
$placement = $placement.Replace('// @HIT_VECTOR@', (Extract-Method 'rotation::Vec3 HitVector('))
$placement = $placement.Replace('// @CALCULATE_FACE@', (Extract-Method 'rotation::Rotation CalculateFace('))
$placement = $placement.Replace('// @NORMAL_ROTATION@', ((Extract-Method 'bool TracePlacement(') + (Extract-Method 'void ComputeHypixelRotation()')))
$placement = $placement.Replace('// @SNAP_ROTATION@', ((Extract-Method 'bool FindSnapRotation(') + (Extract-Method 'rotation::Vec3 HitVectorForSnap()')))
$snapLifecycle = ''
foreach ($signature in @('void ResetSnap()', 'bool BeginSnapTick()', 'bool SyncSnapPacketHistory()', 'void OnSnapMotion(', 'void HandleSnap()')) {
    $snapLifecycle += Extract-Method $signature
}
$placement = $placement.Replace('// @SNAP_LIFECYCLE@', $snapLifecycle)
[IO.File]::WriteAllText((Join-Path $testBuild 'placement.cpp'), $placement)
$placeSource = Get-Content (Join-Path $clientRoot 'src/minecraft/java/game/GamePlace.cpp') -Raw
$origin = Get-Content (Join-Path $PSScriptRoot 'LegacyOriginTest.cpp') -Raw
$origin = $origin.Replace('// @EXCLUSION_METHODS@', ((Extract-Method 'bool ScaffoldExcludedRegistryName(' $placeSource) + (Extract-Method 'bool ScaffoldExcludedObject(' $placeSource)))
$runtime = Get-Content (Join-Path $clientRoot 'src/minecraft/compat/lunar189/LunarRuntime.cpp') -Raw
$originBegin = $runtime.IndexOf('case BlockProperty::ScaffoldExcluded:') + 'case BlockProperty::ScaffoldExcluded:'.Length
$originEnd = $runtime.IndexOf('case BlockProperty::CollisionEmpty:', $originBegin)
if ($originBegin -lt 40 -or $originEnd -lt $originBegin) { throw 'Missing production origin adapter' }
$origin = $origin.Replace('// @ORIGIN_ADAPTER@', $runtime.Substring($originBegin, $originEnd - $originBegin))
[IO.File]::WriteAllText((Join-Path $testBuild 'legacy-origin.cpp'), $origin)
$reserveFixture = @'
#include <array>
#include <cassert>
#include <iostream>
std::array<int, 9> counts{};
std::array<bool, 9> allowed{};
bool HotbarSlotPlaceable(int slot) { return allowed[slot] && counts[slot] > 0; }
int HotbarSlotCount(int slot) { return counts[slot]; }
// @SELECT@
int main() {
    allowed.fill(true);
    counts.fill(1);
    assert(FindPlaceableBlockSlot(1) == -1);
    counts[4] = 2;
    assert(FindPlaceableBlockSlot(1) == 4);
    counts[4] = 1;
    counts[8] = 64;
    assert(FindPlaceableBlockSlot(1) == 8);
    allowed[8] = false;
    assert(FindPlaceableBlockSlot(1) == -1);
    assert(FindPlaceableBlockSlot(0) == 0);
    counts.fill(0);
    assert(FindPlaceableBlockSlot(1) == -1);
    std::cout << "PASS: reserved last block, next stack, exhausted hotbar, invalid blocks\n";
}
'@
[IO.File]::WriteAllText((Join-Path $testBuild 'reserve.cpp'), $reserveFixture.Replace('// @SELECT@', (Extract-Method 'int FindPlaceableBlockSlot(' $placeSource)))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" watchdog.cpp "$clientRoot/src/minecraft/utils/rotation/RotationUtils.cpp" /Fe:watchdog.exe
if errorlevel 1 exit /b 1
watchdog.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" telly.cpp "$clientRoot/src/minecraft/utils/rotation/MovementFix.cpp" "$clientRoot/src/minecraft/utils/rotation/RotationUtils.cpp" /Fe:telly.exe
if errorlevel 1 exit /b 1
telly.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" placement.cpp "$clientRoot/src/minecraft/utils/rotation/RotationUtils.cpp" "$clientRoot/src/minecraft/utils/rotation/MovementFix.cpp" /Fe:placement.exe
if errorlevel 1 exit /b 1
placement.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 reserve.cpp /Fe:reserve.exe
if errorlevel 1 exit /b 1
reserve.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 legacy-origin.cpp /Fe:legacy-origin.exe
if errorlevel 1 exit /b 1
legacy-origin.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText((Join-Path $testBuild 'build.cmd'), $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "Scaffold verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
