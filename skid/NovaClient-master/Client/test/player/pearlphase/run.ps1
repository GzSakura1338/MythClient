$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-pearlphase"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$client/src/minecraft/modules/player/pearlphase/PearlPhase.cpp" -Raw -Encoding UTF8
$source=$source -replace '(?m)^#include[^\r\n]*\r?\n',''
$fixture=Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/runtime.cpp",$fixture.Replace('// @PRODUCTION_MODULE@',$source))
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & ./build.cmd; if($LASTEXITCODE){throw 'PearlPhase regression failed'} } finally {Pop-Location}
