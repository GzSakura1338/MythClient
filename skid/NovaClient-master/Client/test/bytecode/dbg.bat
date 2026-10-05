@echo off
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul
set ROOT=%~dp0..\..
cl -nologo -std:c++20 -EHsc -utf-8 -O0 -Zi -I"%ROOT%" -I"%ROOT%\include" -I"%ROOT%\src" ^
   "C:\Users\13366\AppData\Local\Temp\dbg.cpp" "%ROOT%\src\minecraft\java\bytecode\ClassEdit.cpp" "%ROOT%\src\minecraft\java\bytecode\ClassFile.cpp" ^
   -Fo"%~dp0build\dbg\\" -Fe"%~dp0build\dbg.exe"
