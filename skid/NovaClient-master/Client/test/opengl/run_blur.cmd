@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\opengl-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I"%NOVA_CLIENT%/src" /I"%NOVA_CLIENT%/src/third_party/fatalwidgets/imgui" "%~dp0blur.cpp" "%NOVA_CLIENT%/src/ui/backend/blur/OpenGLBlur.cpp" "%NOVA_CLIENT%/src/ui/backend/blur/BlurDraw.cpp" /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/blur.exe" /link "%NOVA_CLIENT%/build-vs2026/Release/NovaFatalWidgets.lib" user32.lib gdi32.lib opengl32.lib || exit /b 1
"%NOVA_OUT%/blur.exe"
exit /b %ERRORLEVEL%
