@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\common-hud-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD ^
 /I"%NOVA_CLIENT%/src" /I"%NOVA_CLIENT%/src/third_party/fatalwidgets/imgui" /I"%NOVA_CLIENT%/src/third_party/fatalwidgets/include" ^
 "%~dp0CommonHudTest.cpp" "%NOVA_CLIENT%/src/ui/hud/arraylist/ArrayListHud.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/autoblockhud/AutoBlockHud.cpp" "%NOVA_CLIENT%/src/ui/hud/compass/CompassHud.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/bedplates/BedPlateHud.cpp" "%NOVA_CLIENT%/src/ui/hud/itemesp/ItemLabel.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/esp2d/Esp2DView.cpp" "%NOVA_CLIENT%/src/ui/hud/watermark/WatermarkHud.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/targethud/TargetHud.cpp" "%NOVA_CLIENT%/src/ui/hud/nametags/NameTagView.cpp" ^
 "%NOVA_CLIENT%/src/ui/hud/breaker/BreakerHud.cpp" ^
 "%NOVA_CLIENT%/src/ui/clickgui/panel/ClientSettings.cpp" "%NOVA_CLIENT%/src/ui/clickgui/panel/Theme.cpp" ^
 "%NOVA_CLIENT%/src/ui/skin/SkinTextures.cpp" ^
 "%NOVA_CLIENT%/src/ui/notifications/NotificationHud.cpp" "%NOVA_CLIENT%/src/ui/notifications/Notifications.cpp" ^
 "%NOVA_CLIENT%/src/ui/overlay/ClientOverlay.cpp" "%NOVA_CLIENT%/src/ui/overlay/WindowPlacement.cpp" ^
 "%NOVA_CLIENT%/src/ui/text/ImTextRenderer.cpp" "%NOVA_CLIENT%/src/ui/clickgui/Fonts.cpp" ^
 "%NOVA_CLIENT%/src/utils/Json.cpp" "%NOVA_CLIENT%/src/utils/Color.cpp" ^
 /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/hud.exe" ^
 /link "%NOVA_CLIENT%/build-vs2026/Release/NovaFatalWidgets.lib" user32.lib gdi32.lib opengl32.lib || exit /b 1
"%NOVA_OUT%/hud.exe"
exit /b %ERRORLEVEL%
