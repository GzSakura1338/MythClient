@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\smtc-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I"%NOVA_CLIENT%/src" "%~dp0model.cpp" "%NOVA_CLIENT%/src/smtc/Lyrics.cpp" /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/model.exe" || exit /b 1
"%NOVA_OUT%/model.exe" || exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /I"%NOVA_CLIENT%/src" "%~dp0NetEaseLayoutTest.cpp" /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/layout.exe" || exit /b 1
"%NOVA_OUT%/layout.exe" || exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /DNOMINMAX /I"%NOVA_CLIENT%/src" "%~dp0probe.cpp" "%NOVA_CLIENT%/src/smtc/Lyrics.cpp" "%NOVA_CLIENT%/src/smtc/backend/NetEaseLyrics.cpp" "%NOVA_CLIENT%/src/smtc/SmtcService.cpp" "%NOVA_CLIENT%/src/smtc/backend/WindowsSession.cpp" "%NOVA_CLIENT%/src/smtc/backend/NetEaseTimeline.cpp" "%NOVA_CLIENT%/src/utils/Json.cpp" "%NOVA_CLIENT%/src/smtc/backend/ArtworkDecoder.cpp" /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/probe.exe" /link runtimeobject.lib windowscodecs.lib shcore.lib ole32.lib shell32.lib winhttp.lib || exit /b 1
"%NOVA_OUT%/probe.exe"
exit /b %ERRORLEVEL%
