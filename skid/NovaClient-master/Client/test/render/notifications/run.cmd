@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\notifications-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I"%NOVA_CLIENT%/src" "%~dp0queue.cpp" "%NOVA_CLIENT%/src/ui/notifications/Notifications.cpp" /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/queue.exe" || exit /b 1
"%NOVA_OUT%/queue.exe"
exit /b %ERRORLEVEL%
