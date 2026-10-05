$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-hypixel-autoplay'
New-Item -ItemType Directory -Force $out | Out-Null
& python "$PSScriptRoot/verify_mappings.py"
if ($LASTEXITCODE -ne 0) { throw 'AutoPlay mapping verification failed' }
$source = Get-Content "$client/src/minecraft/modules/player/hypixelautoplay/HypixelAutoPlay.cpp" -Raw -Encoding UTF8
$source = $source -replace '(?m)^#include[^\r\n]*\r?\n', ''
$source = $source -replace '(?s)long long NowMillis\(\) \{.*?\n\}', 'long long NowMillis() { return mock::now; }'
$fixture = Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/runtime.cpp", $fixture.Replace('// @PRODUCTION_MODULE@', $source))
$chat = Get-Content "$client/src/minecraft/java/game/GameChat.cpp" -Raw -Encoding UTF8
$chat = $chat -replace '(?m)^#include "(?:nova/|module/|minecraft/compat/minecraft263/)[^\r\n]*\r?\n', ''
$fixture = Get-Content "$PSScriptRoot/ChatAdapterTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/chat.cpp", $fixture.Replace('// @PRODUCTION_CHAT@', $chat))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" chat.cpp /Fe:chat.exe
if errorlevel 1 exit /b 1
chat.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'AutoPlay native verification failed' } }
finally { Pop-Location }
