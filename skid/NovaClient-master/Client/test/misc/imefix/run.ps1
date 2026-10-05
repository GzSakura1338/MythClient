$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out = "$client/build-vs2026/test-imefix"
New-Item -ItemType Directory -Force $out | Out-Null
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /W4 /O2 /MD /I "$client/src" "$PSScriptRoot/Test.cpp" "$client/src/minecraft/host/input/ImeInput.cpp" /Fe:test.exe /link user32.lib imm32.lib
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /W4 /O2 /MD /I "$client/src" "$PSScriptRoot/ChatAdapterTest.cpp" "$client/src/minecraft/modules/misc/imefix/ChatInput.cpp" "$client/src/minecraft/compat/lunar12111/Mapping.cpp" /Fe:chat.exe
if errorlevel 1 exit /b 1
chat.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try {
    & ./build.cmd
    if ($LASTEXITCODE) { throw 'IME Fix native verification failed' }
} finally { Pop-Location }
