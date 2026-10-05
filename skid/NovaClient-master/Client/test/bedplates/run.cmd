@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\bedplates-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I"%NOVA_CLIENT%/src" /I"%NOVA_CLIENT%/include" "%~dp0model.cpp" /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/model.exe" || exit /b 1
"%NOVA_OUT%/model.exe"
exit /b %ERRORLEVEL%
