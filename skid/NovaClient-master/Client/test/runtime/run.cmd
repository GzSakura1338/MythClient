@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\runtime-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I"%NOVA_CLIENT%/src" ^
 "%~dp0RuntimeTest.cpp" "%NOVA_CLIENT%/src/core/Loader.cpp" ^
 "%NOVA_CLIENT%/src/core/RuntimeServices.cpp" "%NOVA_CLIENT%/src/core/Hosts.cpp" ^
 /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/runtime.exe" /link user32.lib || exit /b 1
for %%s in (abi size magic unknown services legacy failed 1 2 3 4 5 6) do (
 "%NOVA_OUT%/runtime.exe" %%s
 if errorlevel 1 exit /b 1
)
exit /b 0
