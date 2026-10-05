@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\smtc-cover-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /DNOMINMAX /I"%NOVA_CLIENT%/src" /I"%NOVA_CLIENT%/src/third_party/fatalwidgets/imgui" "%~dp0cover_gl.cpp" "%NOVA_CLIENT%/src/ui/hud/musichud/CoverTexture.cpp" /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/cover_gl.exe" /link "%NOVA_CLIENT%/build-vs2026/Release/NovaFatalWidgets.lib" user32.lib gdi32.lib opengl32.lib || exit /b 1
"%NOVA_OUT%/cover_gl.exe"
exit /b %ERRORLEVEL%
