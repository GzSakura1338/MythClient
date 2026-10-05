param(
    [string]$RuntimeDirectory = 'E:/MCLDownload/ext/jre-v64-220420/jdk22/bin',
    [string]$VcVars = 'D:/VS2026/VS2026/VC/Auxiliary/Build/vcvars64.bat'
)
$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output = "$client/build-vs2026/mutex-compat"
New-Item -ItemType Directory -Force "$output/old", "$output/current" | Out-Null
foreach ($name in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    Copy-Item -LiteralPath "$RuntimeDirectory/$name" -Destination "$output/old/$name"
}
$build = @"
@echo off
call "$VcVars" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MD /O2 "$PSScriptRoot/MutexCompatibility.cpp" /Fo:baseline.obj /Fe:old/baseline.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MT /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/RuntimeImportsTest.cpp" /Fo:imports.obj /Fe:imports.exe
if errorlevel 1 exit /b 1
cl /nologo /MD /O2 /GS- /DWIN_X64 /DREFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR /I "$client/include" /c "$client/src/core/ReflectiveLoader.c" /Fo:loader.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MD /O2 /LD "$PSScriptRoot/ReflectiveRuntimeProbe.cpp" loader.obj /Fo:probe.obj /Fe:probe.dll
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MT /O2 "$PSScriptRoot/ReflectiveRuntimeHost.cpp" /Fo:host.obj /Fe:host.exe
exit /b %ERRORLEVEL%
"@
Set-Content "$output/build.cmd" $build -Encoding ascii
Push-Location $output
try {
    & ./build.cmd
    if ($LASTEXITCODE) { throw 'Mutex compatibility test build failed' }
    & ./old/baseline.exe
    $baselineExit = $LASTEXITCODE
    Write-Output "Unpatched runtime probe exit: $baselineExit"
    if ($baselineExit -ne 0 -and $baselineExit -ne -1073741819) {
        throw "Unexpected baseline failure: $baselineExit"
    }
    Copy-Item ./old/baseline.exe ./current/baseline.exe
    & ./current/baseline.exe
    if ($LASTEXITCODE) { throw 'Current compiler mutex failed with system runtime' }
    & ./imports.exe "$output/old/msvcp140.dll"
    if ($LASTEXITCODE) { throw 'Profile-specific import resolution failed' }
    foreach ($profile in 0..8) {
        & ./host.exe "$output/old" "$output/probe.dll" $profile
        if ($LASTEXITCODE) { throw "Reflective runtime profile $profile failed" }
    }
} finally {
    Pop-Location
}
