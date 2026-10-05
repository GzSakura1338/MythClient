@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\smtc-hud-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /DNOMINMAX ^
 /I"%NOVA_CLIENT%/src" /I"%NOVA_CLIENT%/src/third_party/fatalwidgets/imgui" ^
 "%~dp0hud.cpp" "%NOVA_CLIENT%/src/smtc/Lyrics.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/lyrics/LyricsOverlay.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/musichud/MusicHud.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/musichud/CoverTexture.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/musichud/SourcePicker.cpp" ^
 "%NOVA_CLIENT%/src/ui/clickgui/panel/ClientSettings.cpp" "%NOVA_CLIENT%/src/ui/clickgui/panel/Theme.cpp" ^
 "%NOVA_CLIENT%/src/ui/overlay/ClientOverlay.cpp" "%NOVA_CLIENT%/src/ui/overlay/WindowPlacement.cpp" ^
 "%NOVA_CLIENT%/src/ui/text/ImTextRenderer.cpp" "%NOVA_CLIENT%/src/ui/clickgui/Fonts.cpp" ^
 "%NOVA_CLIENT%/src/ui/backend/blur/BlurDraw.cpp" "%NOVA_CLIENT%/src/utils/Json.cpp" "%NOVA_CLIENT%/src/utils/Color.cpp" ^
 /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/hud.exe" ^
 /link "%NOVA_CLIENT%/build-vs2026/Release/NovaFatalWidgets.lib" user32.lib gdi32.lib opengl32.lib || exit /b 1
"%NOVA_OUT%/hud.exe" "%NOVA_OUT%/preview.bmp"
exit /b %ERRORLEVEL%
