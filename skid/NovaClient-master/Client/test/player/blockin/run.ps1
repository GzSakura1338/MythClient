$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out=Join-Path $client 'build-vs2026/test-blockin'
New-Item -ItemType Directory -Force $out | Out-Null
& python "$PSScriptRoot/verify_mappings.py"
if($LASTEXITCODE -ne 0){throw 'BlockIn mapping verification failed'}
$source=Get-Content "$client/src/minecraft/modules/player/blockin/BlockIn.cpp" -Raw -Encoding UTF8
$methods=foreach($signature in @('bool Allowed()', 'void ReleaseRotation()', 'void Reset()', 'void Place()')) {
    $start=$source.IndexOf($signature)
    if($start -lt 0){throw "Missing production method: $signature"}
    $open=$source.IndexOf('{',$start); $end=$open+1; $depth=1
    while($depth -gt 0 -and $end -lt $source.Length) {
        if($source[$end] -eq '{'){++$depth}
        if($source[$end] -eq '}'){--$depth}
        ++$end
    }
    if($depth -ne 0){throw "Unclosed production method: $signature"}
    $source.Substring($start,$end-$start)
}
$fixture=Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/runtime.cpp",$fixture.Replace('// @PRODUCTION_RUNTIME@',($methods -join "`n")))
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" "$PSScriptRoot/PlannerTest.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & .\build.cmd; if($LASTEXITCODE -ne 0){throw 'BlockIn verification failed'} }
finally { Pop-Location }
