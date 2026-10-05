@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set NOVA_CLIENT=%~dp0..\..
set NOVA_TEST_BUILD=%NOVA_CLIENT%\build-vs2026\vulkan-test
if not exist "%NOVA_TEST_BUILD%" mkdir "%NOVA_TEST_BUILD%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /DNOVA_VULKAN_TEST /I"%NOVA_CLIENT%\src" /I"%NOVA_CLIENT%\include" /I"%NOVA_CLIENT%" /I"%NOVA_CLIENT%\src\third_party\vulkan\include" /I"%NOVA_CLIENT%\src\third_party\fatalwidgets\imgui" "%~dp0smoke.cpp" "%NOVA_CLIENT%/src/ui/hud/musichud/CoverTexture.cpp" "%NOVA_CLIENT%\src\ui\backend\blur\BlurDraw.cpp" "%NOVA_CLIENT%\src\ui\backend\VulkanOverlay.cpp" "%NOVA_CLIENT%\src\ui\skin\SkinTextures.cpp" "%NOVA_CLIENT%\src\third_party\minhook\src\buffer.c" "%NOVA_CLIENT%\src\third_party\minhook\src\hook.c" "%NOVA_CLIENT%\src\third_party\minhook\src\trampoline.c" "%NOVA_CLIENT%\src\third_party\minhook\src\hde\hde64.c" /Fo"%NOVA_TEST_BUILD%\\" /Fe"%NOVA_TEST_BUILD%\smoke.exe" /link "%NOVA_CLIENT%\build-vs2026\Release\NovaFatalWidgets.lib" user32.lib
if errorlevel 1 exit /b 1
"%NOVA_TEST_BUILD%\smoke.exe"
exit /b %ERRORLEVEL%
