$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-aimassist'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
$source = Get-Content (Join-Path $clientRoot 'src/minecraft/utils/rotation/RotationManager.cpp') -Raw -Encoding UTF8
$utils = Get-Content (Join-Path $clientRoot 'src/minecraft/utils/rotation/RotationUtils.cpp') -Raw -Encoding UTF8
$fixture = Get-Content (Join-Path $PSScriptRoot 'LinearTest.cpp') -Raw -Encoding UTF8
$methods = foreach ($signature in @(
    'Rotation RelateToPlayerRotation(', 'void SetActive(', 'void CorrectDisabledRotations()',
    'bool OwnsExclusiveRotations(', 'bool SetExclusiveRotations(', 'void ReleaseExclusiveRotations(',
    'bool SetLinearRotations(', 'void ReturnLinearRotations(', 'void ReleaseLinearRotations()', 'bool SetDirectRotations(', 'void ReleaseDirectRotations()', 'void Smooth()',
    'void UpdateGlobalYaw()', 'void OnMotionPre(', 'void OnRespawn()',
    'void OnMoveInput(', 'void OnStrafe(', 'void SetMoveInputFix(',
    "void SetRotations(const Rotation& rotations, double rotationSpeed,`n")) {
    $normalized = $source.Replace("`r`n", "`n")
    if ($signature -eq 'Rotation RelateToPlayerRotation(') { $normalized = $utils.Replace("`r`n", "`n") }
    $start = $normalized.IndexOf($signature)
    if ($start -lt 0) { throw "Missing production method: $signature" }
    $open = $normalized.IndexOf('{', $start)
    $depth = 1; $end = $open + 1
    while ($depth -gt 0 -and $end -lt $normalized.Length) {
        if ($normalized[$end] -eq '{') { ++$depth }
        if ($normalized[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Unclosed production method: $signature" }
    $normalized.Substring($start, $end - $start)
}
[IO.File]::WriteAllText((Join-Path $testBuild 'test.cpp'),
    $fixture.Replace('// @PRODUCTION_ROTATION_METHODS@', ($methods -join "`n")))
$configSource = Get-Content (Join-Path $clientRoot 'src/minecraft/host/presentation/Config.cpp') -Raw -Encoding UTF8
$configStart = $configSource.IndexOf('nova::json::Value MigrateAimAssistSettings(')
$configEnd = $configSource.IndexOf('nova::json::Value BuildModule(', $configStart)
if ($configStart -lt 0 -or $configEnd -lt 0) { throw 'Missing config row serialization functions' }
$configFixture = Get-Content (Join-Path $PSScriptRoot 'ConfigGroupTest.cpp') -Raw -Encoding UTF8
[IO.File]::WriteAllText((Join-Path $testBuild 'config.cpp'),
    $configFixture.Replace('// @PRODUCTION_CONFIG_ROWS@', $configSource.Substring($configStart, $configEnd - $configStart)))
$commands = @'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I..\..\src /I..\..\include test.cpp ..\..\src\minecraft\utils\rotation\MovementFix.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I..\..\src config.cpp ..\..\src\config\PropertyCodec.cpp ..\..\src\utils\Json.cpp /Fe:config.exe
if errorlevel 1 exit /b 1
config.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText((Join-Path $testBuild 'build.cmd'), $commands)
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "AimAssist verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
