$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-throwableaura'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
$prefix = Get-Content "$PSScriptRoot/Test.prefix.cpp" -Raw
$suffix = Get-Content "$PSScriptRoot/Test.suffix.cpp" -Raw
$source = (Get-Content "$clientRoot/src/minecraft/modules/combat/throwableaura/ThrowableAura.cpp" -Raw) -replace '(?m)^#include.*\r?\n', ''
$source = $source.Replace('private:', 'public:')
$rotationHeader = (Get-Content "$clientRoot/src/minecraft/utils/rotation/RotationManager.h" -Raw) -replace '(?m)^#.*\r?\n', ''
$rotationSource = (Get-Content "$clientRoot/src/minecraft/utils/rotation/RotationManager.cpp" -Raw) -replace '(?m)^#include.*\r?\n', ''
[IO.File]::WriteAllText("$testBuild/test.cpp", $prefix + "`n" + $rotationHeader + "`n" + $rotationSource + "`n" + $source + "`n" + $suffix)
$runtime = Get-Content "$clientRoot/src/minecraft/compat/lunar189/LunarRuntime.cpp" -Raw
$start = $runtime.IndexOf('bool ThrowHeldItem()')
$open = $runtime.IndexOf('{', $start)
$depth = 1; $end = $open + 1
while ($depth -gt 0 -and $end -lt $runtime.Length) {
 if ($runtime[$end] -eq '{') { ++$depth }
 if ($runtime[$end] -eq '}') { --$depth }
 ++$end
}
if ($start -lt 0 -or $depth -ne 0) { throw 'Cannot extract ThrowHeldItem' }
$native = Get-Content "$PSScriptRoot/NativeTest.cpp" -Raw
[IO.File]::WriteAllText("$testBuild/native.cpp", $native.Replace('// @THROW_PRODUCTION@', $runtime.Substring($start, $end-$start)))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" test.cpp "$clientRoot/src/minecraft/utils/rotation/RotationUtils.cpp" "$clientRoot/src/minecraft/utils/rotation/MovementFix.cpp" /Fe:test.exe
if not %ERRORLEVEL%==0 exit /b 1
test.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 native.cpp /Fe:native.exe
if not %ERRORLEVEL%==0 exit /b 1
native.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" "$PSScriptRoot/TrajectoryTest.cpp" /Fe:trajectory.exe
if not %ERRORLEVEL%==0 exit /b 1
trajectory.exe
if not %ERRORLEVEL%==0 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$clientRoot/src" "$PSScriptRoot/PredictionTest.cpp" /Fe:prediction.exe
if not %ERRORLEVEL%==0 exit /b 1
prediction.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$testBuild/build.cmd", $commands)
Push-Location $testBuild
try {
 & .\build.cmd
 if ($LASTEXITCODE -ne 0) { throw "ThrowableAura verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
