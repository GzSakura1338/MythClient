$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out=Join-Path $client 'build-vs2026/test-panel'
New-Item -ItemType Directory -Force $out | Out-Null
$sources=@('ui/clickgui/Fonts.cpp','ui/clickgui/FeatureTree.cpp','ui/backend/blur/Blur.cpp','ui/backend/blur/BlurDraw.cpp','minecraft/modules/Module.cpp','minecraft/modules/ModuleManager.cpp','bind/BindProperty.cpp','bind/BindableManager.cpp','bind/BindCapture.cpp','bind/InputManager.cpp','bind/KeyName.cpp','utils/Color.cpp','utils/Json.cpp')
$sources+=Get-ChildItem "$client/src/ui/clickgui/panel/*.cpp" | ForEach-Object { $_.FullName.Substring(("$client/src/").Length) }
$args=$sources | ForEach-Object { '"'+(Join-Path "$client/src" $_)+'"' }
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /Gy /I "$client/src" /I "$client/include" /I "$client/src/third_party/fatalwidgets/include" /I "$client/src/third_party/fatalwidgets/imgui" "$PSScriptRoot/Preview.cpp" $($args -join ' ') "$client/build-vs2026/Release/NovaFatalWidgets.lib" /Fe:preview.exe /link /opt:ref user32.lib gdi32.lib opengl32.lib imm32.lib dwmapi.lib
if errorlevel 1 exit /b 1
preview.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & .\build.cmd; if($LASTEXITCODE -ne 0){throw 'Panel preview failed'} }
finally {Pop-Location}
