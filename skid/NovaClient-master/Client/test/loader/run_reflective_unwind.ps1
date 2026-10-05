param(
    [string]$VcVars = 'D:/VS2026/VS2026/VC/Auxiliary/Build/vcvars64.bat',
    [switch]$ProductionDll
)
$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output = "$client/build-vs2026/reflective-unwind"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$build = @"
@echo off
call "$VcVars" >nul
if errorlevel 1 exit /b 1
cl /nologo /MD /O2 /GS- /DWIN_X64 /DREFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR /I "$client/include" /c "$client/src/core/ReflectiveLoader.c" /Fo:loader.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MD /O2 /I "$client/src" /c "$client/src/core/SelfUnload.cpp" /Fo:unload.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MD /O2 /LD /I "$client/src" "$PSScriptRoot/ReflectiveUnwindProbe.cpp" loader.obj unload.obj /Fo:probe.obj /Fe:probe.dll
if errorlevel 1 exit /b 1
cl /nologo /MD /O2 /GS- /DWIN_X64 /DREFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR /DREFLECTIVEDLLINJECTION_CUSTOM_DLLMAIN /I "$client/include" /c "$client/src/core/ReflectiveLoader.c" /Fo:reject-loader.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MD /O2 /LD /DPROBE_REJECT_ENTRY /I "$client/src" "$PSScriptRoot/ReflectiveUnwindProbe.cpp" reject-loader.obj unload.obj /Fo:reject-probe.obj /Fe:reject-probe.dll
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /c "$client/src/third_party/minhook/src/buffer.c" "$client/src/third_party/minhook/src/hde/hde64.c" "$client/src/third_party/minhook/src/hook.c" "$client/src/third_party/minhook/src/trampoline.c"
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MT /O2 /I "$client/src" /I "$client/src/third_party/minhook/include" "$PSScriptRoot/ReflectiveUnwindHost.cpp" buffer.obj hde64.obj hook.obj trampoline.obj /Fo:host.obj /Fe:host.exe
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath "$output/build.cmd" -Value $build -Encoding ascii
Push-Location $output
try {
    & ./build.cmd
    if ($LASTEXITCODE) { throw 'Reflective unwind test build failed' }
    $arguments = @("$output/probe.dll", "$output/reject-probe.dll")
    if ($ProductionDll) { $arguments += "$client/build-vs2026/Release/NovaClientBase.dll" }
    $previousLocal = $env:LOCALAPPDATA
    try {
        # Keep the production failed-startup log separate from real game logs.
        $env:LOCALAPPDATA = "$output/local-appdata"
        & ./host.exe @arguments
        if ($LASTEXITCODE) { throw "Reflective unwind regression failed: $LASTEXITCODE" }
    } finally { $env:LOCALAPPDATA = $previousLocal }
} finally { Pop-Location }
