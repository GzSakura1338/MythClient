$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-nametags'
New-Item -ItemType Directory -Force $out | Out-Null
& python "$PSScriptRoot/verify_mappings.py"
if ($LASTEXITCODE -ne 0) { throw 'NameTags mapping verification failed' }
$imgui = "$client/src/third_party/fatalwidgets/imgui"
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$imgui" "$PSScriptRoot/ClassicTest.cpp" "$client/src/ui/clickgui/Fonts.cpp" "$imgui/imgui.cpp" "$imgui/imgui_draw.cpp" "$imgui/imgui_widgets.cpp" "$imgui/imgui_tables.cpp" /Fe:classic.exe /link user32.lib gdi32.lib imm32.lib
if errorlevel 1 exit /b 1
classic.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'Classic native verification failed' } }
finally { Pop-Location }
