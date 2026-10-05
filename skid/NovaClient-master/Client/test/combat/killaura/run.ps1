$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-killaura-targets'
New-Item -ItemType Directory -Force $out | Out-Null
$source = Get-Content "$client/src/minecraft/modules/combat/killaura/KillAura.cpp" -Raw -Encoding UTF8
if (!$source.Contains('CollectTargets(self, frame.eye, playerYaw,') -or
    !$source.Contains('SortTargets(self, frame, playerYaw,')) { throw 'KillAura target selection must receive camera yaw' }
$methods = foreach ($signature in @('bool CollectTargets(', 'void SortTargets(')) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing method: $signature" }
    $end = $source.IndexOf('{', $start) + 1
    $depth = 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Unclosed method: $signature" }
    $source.Substring($start, $end - $start)
}
$fixture = Get-Content "$PSScriptRoot/TargetFovTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/targets.cpp", $fixture.Replace('// @PRODUCTION_TARGET_METHODS@', ($methods -join "`n")))
$commands = @'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 targets.cpp /Fe:targets.exe
if errorlevel 1 exit /b 1
targets.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'KillAura target verification failed' } }
finally { Pop-Location }
