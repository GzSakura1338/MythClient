$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-cswin-victory"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$client/src/minecraft/modules/render/cswin/CSWin.cpp" -Raw -Encoding UTF8
$source=$source -replace '(?m)^#include[^\r\n]*\r?\n',''
$source=$source.Replace('using Clock=std::chrono::steady_clock;','using Clock=mock::Clock;')
$source=$source.Replace('std::mt19937 random_{std::random_device{}()};','std::mt19937 random_{12345};')
$fixture=Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/runtime.cpp",$fixture.Replace('// @PRODUCTION_MODULE@',$source))
foreach($track in @('Inhuman','EZ4ENCE')) {
    & 'D:/VS2026/VS2026/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' "-DINPUT=$client/src/ui/hud/cswin/assets/$track.wav" "-DTRACK=$track" "-DOUTPUT=$out/Music$track.cpp" -P "$client/src/ui/hud/cswin/EmbedMusic.cmake"
    if($LASTEXITCODE){throw 'CSWin test asset embedding failed'}
}
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/VictoryTextTest.cpp" /Fe:text.exe
if errorlevel 1 exit /b 1
text.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/AudioTest.cpp" MusicInhuman.cpp MusicEZ4ENCE.cpp /Fe:audio.exe
if errorlevel 1 exit /b 1
audio.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & ./build.cmd; if($LASTEXITCODE){throw 'CSWin victory regression failed'} }
finally { Pop-Location }
