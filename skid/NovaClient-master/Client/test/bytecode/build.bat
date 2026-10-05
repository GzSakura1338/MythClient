@echo off
rem Builds the rewriter harness. Needs the VS 2026 environment, same as the
rem client itself -- see the note in the project memory about the bundled cmake.
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

set ROOT=%~dp0..\..
if not exist "%~dp0build" mkdir "%~dp0build"

cl -nologo -std:c++20 -EHsc -utf-8 -O2 -W3 ^
   -I"%ROOT%" -I"%ROOT%\include" -I"%ROOT%\src" ^
   "%~dp0patch_test.cpp" "%ROOT%\src\minecraft\java\bytecode\ClassEdit.cpp" "%ROOT%\src\minecraft\java\bytecode\ClassFile.cpp" ^
   "%ROOT%\src\minecraft\hooks\Edits.cpp" ^
   -Fo"%~dp0build\\" -Fe"%~dp0build\patch_test.exe"
exit /b %ERRORLEVEL%
