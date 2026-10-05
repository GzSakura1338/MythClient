param([string]$RuntimeDirectory = 'D:/MCLDownload/ext/jre-v64-220420/jdk17/bin')
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = "$repo/out/injector/test-remote-thread"
New-Item -ItemType Directory -Force "$out/old" | Out-Null
foreach ($name in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    Copy-Item -LiteralPath "$RuntimeDirectory/$name" -Destination "$out/old/$name"
}
$build = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD /LD /I "$repo/Client/src" /I "$repo/Client/include" "$PSScriptRoot/RemoteThreadProbe.cpp" /Fe:remote-probe.dll
if errorlevel 1 exit /b 1
cl /nologo /O2 /MT /LD "$PSScriptRoot/EmptyJvm.c" /Fe:jvm.dll
if errorlevel 1 exit /b 1
cl /nologo /O2 /W3 /MT /DWIN_X64 /c "$repo/injector/Inject.c" "$repo/injector/RemoteThreadLoad.c" "$repo/injector/LoadLibraryR.c" "$repo/injector/GetProcAddressR.c"
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MT "$PSScriptRoot/RemoteThreadHost.cpp" Inject.obj RemoteThreadLoad.obj LoadLibraryR.obj GetProcAddressR.obj /Fe:remote-host.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $build)
Push-Location $out
try {
    & ./build.cmd
    if ($LASTEXITCODE) { throw 'Remote-thread tests failed to build' }
    New-Item -ItemType Directory -Force "$out/log/.minecraft" | Out-Null
    $previousAppData = $env:APPDATA
    try {
        $env:APPDATA = "$out/log"
        & ./remote-host.exe "$out/old" "$out/remote-probe.dll" "$repo/Client/build-vs2026/Release/NovaClientBase.dll"
        if ($LASTEXITCODE) { throw "Remote-thread tests failed: $LASTEXITCODE" }
    } finally { $env:APPDATA = $previousAppData }
} finally { Pop-Location }
