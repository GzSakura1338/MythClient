$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out = "$client/build-vs2026/test-speed"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source = (Get-Content "$client/src/minecraft/modules/movement/speed/Speed.cpp" -Raw -Encoding UTF8) -replace '(?m)^#include[^\r\n]*\r?\n',''
$fixture = Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/runtime.cpp",$fixture.Replace('// @PRODUCTION_MODULE@',$source))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" runtime.cpp "$client/src/minecraft/events/EventBus.cpp" "$client/src/minecraft/utils/rotation/RotationManager.cpp" "$client/src/minecraft/utils/rotation/RotationUtils.cpp" "$client/src/minecraft/utils/rotation/MovementFix.cpp" /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & ./build.cmd; if ($LASTEXITCODE) { throw "Speed regression failed ($LASTEXITCODE)" } }
finally { Pop-Location }
