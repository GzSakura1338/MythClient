@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
pushd "%~dp0..\..\..\build-vs2026"
cl /nologo /std:c++20 /EHsc /O2 /I..\src ..\test\combat\antikb\JumpResetTest.cpp /FoJumpResetTest.obj /FeJumpResetTest.exe
if errorlevel 1 goto failed
JumpResetTest.exe
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
