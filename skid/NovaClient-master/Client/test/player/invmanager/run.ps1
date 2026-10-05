$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-invmanager'
New-Item -ItemType Directory -Force $out | Out-Null
$source = Get-Content "$client/src/minecraft/modules/player/invmanager/InvManager.cpp" -Raw -Encoding UTF8
if ($source -notmatch 'drop_ = &addBool\("Drop", false\)') { throw 'Drop must default off' }
$methods = foreach ($signature in @('void AutoArmor(', 'void ThrowItem(')) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing method: $signature" }
    $open = $source.IndexOf('{', $start)
    $depth = 1; $end = $open + 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Unclosed method: $signature" }
    $source.Substring($start, $end - $start)
}
$fixture = Get-Content "$PSScriptRoot/DropTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/test.cpp", $fixture.Replace('// @PRODUCTION_INVENTORY_METHODS@', ($methods -join "`n")))
$commands = @'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 test.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'InvManager native verification failed' } }
finally { Pop-Location }
