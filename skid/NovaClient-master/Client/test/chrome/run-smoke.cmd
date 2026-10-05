@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul
if not %ERRORLEVEL%==0 exit /b 1
cd /d "%~dp0..\..\build-vs2026"
if not exist test-chrome mkdir test-chrome
cd test-chrome
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MT /I"..\..\src" "..\..\test\chrome\Smoke.cpp" "..\..\src\chrome\client\BrowserService.cpp" /Fe:smoke.exe /link user32.lib advapi32.lib ole32.lib shell32.lib imm32.lib
if not %ERRORLEVEL%==0 exit /b 1
smoke.exe
exit /b %ERRORLEVEL%
