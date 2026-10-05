$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-feetplace"
New-Item -ItemType Directory -Force $out | Out-Null
$fixture=Get-Content "$PSScriptRoot/../legitcrystalaura/RuntimeTest.cpp" -Raw
$prefix=$fixture.Substring(0,$fixture.IndexOf('// @PRODUCTION_MODULE@'))
$source=Get-Content "$client/src/minecraft/modules/combat/feetplace/FeetPlace.cpp" -Raw
$source=$source -replace '(?m)^#include[^\r\n]*\r?\n',''
$test=Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$out/runtime.cpp",$prefix+$test.Replace('// @PRODUCTION_MODULE@',$source))
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try{& ./build.cmd;if($LASTEXITCODE){throw 'FeetPlace runtime regression failed'}}finally{Pop-Location}
