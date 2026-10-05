@echo off
setlocal
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "NOVA_CLIENT=%~dp0..\..\.."
set "NOVA_OUT=%NOVA_CLIENT%\build-vs2026\ui-catalog-test"
if not exist "%NOVA_OUT%" mkdir "%NOVA_OUT%"
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD ^
 /I"%NOVA_CLIENT%/src" /I"%NOVA_CLIENT%/src/third_party/fatalwidgets/imgui" ^
 /I"%NOVA_CLIENT%/src/third_party/fatalwidgets/include" ^
 "%~dp0CatalogTest.cpp" "%NOVA_CLIENT%/src/ui/clickgui/FeatureTree.cpp" ^
 "%NOVA_CLIENT%/src/utils/Json.cpp" "%NOVA_CLIENT%/src/utils/Color.cpp" ^
 /Fo"%NOVA_OUT%\\" /Fe"%NOVA_OUT%/catalog.exe" || exit /b 1
"%NOVA_OUT%/catalog.exe"
exit /b %ERRORLEVEL%
