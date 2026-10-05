$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-playerlist"
New-Item -ItemType Directory -Force $out | Out-Null
$sources=@('ui/clickgui/Fonts.cpp','ui/hud/playerlist/PlayerListHud.cpp','ui/overlay/ClientOverlay.cpp','ui/text/ImTextRenderer.cpp','utils/Color.cpp','utils/Json.cpp')
$arguments=$sources | ForEach-Object { '"'+(Join-Path "$client/src" $_)+'"' }
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I "$client/src" /I "$client/include" /I "$client/src/third_party/fatalwidgets/imgui" /I "$client/src/third_party/fatalwidgets/imgui/backends" "$PSScriptRoot/Test.cpp" $($arguments -join ' ') "$client/build-vs2026/Release/NovaFatalWidgets.lib" /Fe:test.exe /link user32.lib gdi32.lib opengl32.lib imm32.lib dwmapi.lib
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & ./build.cmd; if($LASTEXITCODE){throw 'Player List verification failed'} }
finally {Pop-Location}
