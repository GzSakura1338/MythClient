$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-targetesp"
New-Item -ItemType Directory -Force $out | Out-Null
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I "$client/src" /I "$client/src/third_party/fatalwidgets/imgui" "$PSScriptRoot/Test.cpp" "$client/src/ui/hud/targetesp/TargetRing.cpp" "$client/build-vs2026/Release/NovaFatalWidgets.lib" /Fe:test.exe /link user32.lib gdi32.lib opengl32.lib imm32.lib dwmapi.lib
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & ./build.cmd; if($LASTEXITCODE){throw 'TargetESP verification failed'} }
finally {Pop-Location}
