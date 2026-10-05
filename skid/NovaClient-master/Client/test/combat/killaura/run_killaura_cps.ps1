$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$testBuild = Join-Path $clientRoot 'build-vs2026/test-killaura-cps'
New-Item -ItemType Directory -Force -Path $testBuild | Out-Null
$source = Get-Content (Join-Path $clientRoot 'src/minecraft/modules/combat/killaura/KillAura.cpp') -Raw -Encoding UTF8
$fixture = Get-Content (Join-Path $PSScriptRoot 'KillAuraCpsTest.cpp') -Raw -Encoding UTF8
$critical = Get-Content (Join-Path $clientRoot 'src/minecraft/java/api/game_critical.h') -Raw -Encoding UTF8
$state = [regex]::Match($critical, '(?s)struct CriticalAttackState \{.*?\n\};').Value
if (!$state) { throw 'Missing critical attack state' }
$fixture = $fixture.Replace('// @CRITICAL_STATE@', $state)
function Extract([string]$text, [string]$signature) {
    $start=$text.IndexOf($signature)
    if($start -lt 0){throw "Missing production declaration: $signature"}
    $end=$text.IndexOf('{',$start)+1; $depth=1
    while($depth -gt 0 -and $end -lt $text.Length){
        if($text[$end] -eq '{'){++$depth}
        if($text[$end] -eq '}'){--$depth}
        ++$end
    }
    if($depth){throw "Unclosed production declaration: $signature"}
    $text.Substring($start,$end-$start)
}
$criticalSource=Get-Content "$clientRoot/src/minecraft/java/game/GameCritical.cpp" -Raw
$criticalMethods=@(
    (Extract $criticalSource 'struct CriticalAttackScope') + ';'
    (Extract $criticalSource 'bool CriticalSprintInterrupted()')
    (Extract $criticalSource 'bool AttackWithCriticalWait(')
)
$fixture=$fixture.Replace('// @CRITICAL_METHODS@',($criticalMethods -join "`n"))
# Extract real production methods rather than duplicating scheduling logic.
$methods = foreach ($signature in @('bool UsesAttackCooldown() const', 'bool CriticalSprintEnabled() const', 'void OnCriticalSprintTick(', 'void OnCriticalSprintInput(', 'void OnPlayerTick()')) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing production method: $signature" }
    $open = $source.IndexOf('{', $start)
    $depth = 1
    $end = $open + 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Unclosed production method: $signature" }
    $source.Substring($start, $end - $start)
}
[IO.File]::WriteAllText((Join-Path $testBuild 'test.cpp'),
    $fixture.Replace('// @KILLAURA_METHODS@', ($methods -join "`n")))
$commands = @'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "@CLIENT@/src" test.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText((Join-Path $testBuild 'build.cmd'), $commands.Replace('@CLIENT@',$clientRoot))
Push-Location $testBuild
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "KillAura CPS verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
