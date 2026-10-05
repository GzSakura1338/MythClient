$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-autoreplenish"
New-Item -ItemType Directory -Force $out | Out-Null
$fixture=Get-Content "$PSScriptRoot/../../combat/autototem/RuntimeTest.cpp" -Raw
$prefix=$fixture.Substring(0,$fixture.IndexOf('// @PRODUCTION_MODULE@'))
$prefix=$prefix.Replace('namespace nova::module::autoreplenish{bool IsWorking(){return false;}}','namespace nova::module::autototem{bool IsWorking(){return mock::totemBusy;}}')
$prefix=$prefix.Replace('bool cursor=true,supported=true,accept=true;','bool cursor=true,supported=true,accept=true; bool totemBusy=false;')
$prefix=$prefix.Replace('enum class Category{Combat};','enum class Category{Combat,Player};')
$prefix=$prefix.Replace('button==40','button>=0&&button<9')
$prefix=$prefix.Replace('std::swap(mock::offhand,mock::slots[slot>=36?slot-36:slot])','std::swap(mock::slots[button],mock::slots[slot])')
$source=Get-Content "$client/src/minecraft/modules/player/autoreplenish/AutoReplenish.cpp" -Raw
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
try{& ./build.cmd;if($LASTEXITCODE){throw 'AutoReplenish runtime regression failed'}}finally{Pop-Location}
