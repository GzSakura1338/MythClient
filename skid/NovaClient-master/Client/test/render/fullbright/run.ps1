$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-fullbright"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$client/src/minecraft/modules/render/fullbright/FullBright.cpp" -Raw -Encoding UTF8
$start=$source.IndexOf('namespace nova::module {')
if($start -lt 0){throw 'Missing FullBright implementation'}
$body=$source.Substring($start).Replace('using Clock = std::chrono::steady_clock;','using Clock = TestClock;')
$fixture=Get-Content "$PSScriptRoot/FullBrightTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/test.cpp",$fixture.Replace('// @PRODUCTION_MODULE@',$body))
$commands=@'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 test.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & ./build.cmd; if($LASTEXITCODE){throw 'FullBright verification failed'} }
finally { Pop-Location }
