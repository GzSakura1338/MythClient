$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = "$client/build-vs2026/test-lazy-targets"
$jvm = 'D:/Program Files/Java/jdk-26.0.2.1/bin/server/jvm.dll'
New-Item -ItemType Directory -Force $out | Out-Null
$build = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/LazyTargets.cpp" "$client/src/minecraft/hooks/Transformer.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" /Fe:lazy-targets.exe
exit /b %ERRORLEVEL%
"@
Set-Content "$out/build.cmd" $build -Encoding ascii
Push-Location $out
try {
    & ./build.cmd
    if ($LASTEXITCODE) { throw 'Lazy target test compilation failed' }
    foreach ($case in @('normal','missing','broken')) {
        # Separate fixture directories keep negative cases independent of old files.
        $run = Join-Path $out ($case + '-' + [Guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory $run | Out-Null
        Push-Location $run
        try {
            & "$out/lazy-targets.exe" $jvm $case
            if ($LASTEXITCODE) { throw "Lazy target case failed: $case" }
        } finally { Pop-Location }
    }
} finally { Pop-Location }
