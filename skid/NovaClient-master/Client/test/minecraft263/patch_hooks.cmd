@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "ROOT=%~dp0..\.."
set "OUT=%ROOT%\build-vs2026\mc263-hook-test"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /W4 /I"%ROOT%/include" /I"%ROOT%/src" "%~dp0patch_hooks.cpp" "%ROOT%/src/minecraft/java/bytecode/ClassEdit.cpp" "%ROOT%/src/minecraft/java/bytecode/ClassFile.cpp" "%ROOT%/src/minecraft/hooks/Edits.cpp" /Fo"%OUT%\\" /Fe"%OUT%/patch_hooks.exe" || exit /b 1
"%OUT%/patch_hooks.exe" "%ROOT%/build-vs2026/mc263-classes" "%OUT%/patched"
if errorlevel 1 exit /b 1
javac -d "%OUT%" "%~dp0VerifyHooks.java" || exit /b 1
java -cp "%OUT%" VerifyHooks "%OUT%/patched" "D:/AI/VibeCoding/shader/skid/minecraft-26.3-client.jar" "%APPDATA%/.minecraft/libraries"
exit /b %ERRORLEVEL%
