@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
set "ROOT=%~dp0..\..\.."
set "OUT=%ROOT%\build-vs2026\test-backtrack"
if not exist "%OUT%" mkdir "%OUT%"
python "%~dp0audit_mapping.py"
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /O2 /I"%ROOT%/include" /I"%ROOT%/src" "%~dp0attack_hook.cpp" "%ROOT%/src/minecraft/java/bytecode/ClassEdit.cpp" "%ROOT%/src/minecraft/java/bytecode/ClassFile.cpp" "%ROOT%/src/minecraft/hooks/Edits.cpp" /Fo"%OUT%\\" /Fe"%OUT%/attack_hook.exe"
if errorlevel 1 exit /b 1
"%OUT%/attack_hook.exe" forge "%OUT%/forge-controller.class"
exit /b %ERRORLEVEL%
