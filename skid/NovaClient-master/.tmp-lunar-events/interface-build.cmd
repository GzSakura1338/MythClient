@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I ..\Client\include /I ..\Client\test\bytecode interface.cpp ..\Client\src\java\ClassEdit.cpp ..\Client\src\java\ClassFile.cpp /Fe:interface-test.exe
exit /b %ERRORLEVEL%
