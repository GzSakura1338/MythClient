$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output = "$client/build-vs2026/test-config"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/UnloadSaveTest.cpp" "$client/src/config/Config.cpp" "$client/src/config/ConfigManager.cpp" "$client/src/config/PropertyCodec.cpp" "$client/src/utils/Json.cpp" shell32.lib /Fe:config.exe
if errorlevel 1 exit /b 1
config.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$output/build.cmd",$commands)
Push-Location $output
try { & ./build.cmd; if($LASTEXITCODE){throw "Config regression failed: $LASTEXITCODE"} }
finally { Pop-Location }
