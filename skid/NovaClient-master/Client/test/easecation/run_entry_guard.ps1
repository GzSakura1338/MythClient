param(
    [string]$JavaHome='E:/MCLDownload/ext/jre-v64-220420/jdk22',
    [string]$Fixture="$PSScriptRoot/../bytecode/build/probe/Probe.class"
)
$ErrorActionPreference='Stop'
if(!(Test-Path -LiteralPath $Fixture)){throw 'Existing compiled Probe.class fixture required; this test creates no Java sources'}
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$client/build-vs2026/test-easecation"
New-Item -ItemType Directory -Force $output | Out-Null
$build=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MT /O2 /I "$client/include" /I "$JavaHome/include" /I "$JavaHome/include/win32" "$PSScriptRoot/EntryGuardJvm.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" /Fe:entry-guard-jvm.exe
exit /b %ERRORLEVEL%
"@
Set-Content "$output/build-entry-guard.cmd" $build -Encoding ascii
Push-Location $output
try{
    & ./build-entry-guard.cmd
    if($LASTEXITCODE){throw 'Entry guard test build failed'}
    & ./entry-guard-jvm.exe "$JavaHome/bin/server/jvm.dll" $Fixture
    if($LASTEXITCODE){throw 'Entry guard JVM verification failed'}
}finally{Pop-Location}
