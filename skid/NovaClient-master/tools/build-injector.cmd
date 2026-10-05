@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
pushd "%~dp0.." || exit /b 1
if not exist "out\injector\native" mkdir "out\injector\native"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\package-browser.ps1
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\generate-winmm.ps1
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo"out\injector\native\winmm-stubs.obj" "out\injector\native\winmm-stubs.asm"
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /MT /utf-8 /LD /I"out\injector\native" /Fo"out\injector\native\winmm.obj" injector\winmm.cpp out\injector\native\winmm-stubs.obj /link /DEF:out\injector\native\winmm.def /OUT:out\injector\native\winmm.dll /IMPLIB:out\injector\native\winmm.lib
if errorlevel 1 exit /b 1
cl /nologo /O2 /W3 /MT /DWIN_X64 /Fe:"out\injector\native\NovaInjector.exe" /Fo:"out\injector\native\\" injector\cli.c injector\Inject.c injector\RemoteThreadLoad.c injector\LoadLibraryR.c injector\GetProcAddressR.c
if errorlevel 1 exit /b 1
"D:\VS2026\VS2026\MSBuild\Current\Bin\MSBuild.exe" injector\ui\Injector.WinUI.csproj /restore /t:Publish /p:Configuration=Release /p:Platform=x64 /p:PublishProfile=Standalone /p:OutputPath="%CD%\out\injector\ui/" /p:IntermediateOutputPath="%CD%\injector\ui\obj\single-file/" /nologo /v:minimal %*
set NOVA_BUILD_RESULT=%ERRORLEVEL%
popd
exit /b %NOVA_BUILD_RESULT%
