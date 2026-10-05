@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\test-nametags"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I"%NOVA_CLIENT%/src" ^
 "%~dp0AppearanceTest.cpp" "%NOVA_CLIENT%/src/config/PropertyCodec.cpp" ^
 "%NOVA_CLIENT%/src/utils/Json.cpp" "%NOVA_CLIENT%/src/utils/Color.cpp" ^
 /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/appearance.exe" || exit /b 1
"%NOVA_OUT%/appearance.exe"
exit /b %ERRORLEVEL%
