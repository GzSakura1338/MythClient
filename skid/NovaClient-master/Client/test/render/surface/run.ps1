$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-surface'
New-Item -ItemType Directory -Force $out | Out-Null
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I "$client/src" /I "$client/src/third_party/fatalwidgets/imgui" "$PSScriptRoot/SurfaceTest.cpp" "$client/src/ui/clickgui/Fonts.cpp" "$client/src/ui/skin/SkinTextures.cpp" "$client/src/ui/hud/musichud/CoverTexture.cpp" "$client/src/ui/backend/blur/OpenGLBlur.cpp" "$client/build-vs2026/Release/NovaFatalWidgets.lib" /Fe:surface.exe /link user32.lib gdi32.lib opengl32.lib comctl32.lib dwmapi.lib
if errorlevel 1 exit /b 1
surface.exe
if errorlevel 1 exit /b 1
surface.exe core
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I "$client/src" /I "$client/src/third_party/fatalwidgets/imgui" "$PSScriptRoot/ThreadedSurfaceTest.cpp" "$client/build-vs2026/Release/NovaFatalWidgets.lib" /Fe:threaded.exe /link user32.lib gdi32.lib comctl32.lib dwmapi.lib
if errorlevel 1 exit /b 1
threaded.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'Render surface verification failed' } }
finally { Pop-Location }
