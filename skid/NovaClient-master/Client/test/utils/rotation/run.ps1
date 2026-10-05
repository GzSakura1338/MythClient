$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$output = "$client/build-vs2026/test-rotation-continuity"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" "$PSScriptRoot/ContinuityTest.cpp" "$client/src/minecraft/utils/rotation/RotationManager.cpp" "$client/src/minecraft/utils/rotation/RotationUtils.cpp" "$client/src/minecraft/utils/rotation/MovementFix.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$output/build.cmd", $commands)
Push-Location $output
try {
    & ./build.cmd
    if ($LASTEXITCODE) { throw "Rotation continuity regression failed: $LASTEXITCODE" }
} finally { Pop-Location }
