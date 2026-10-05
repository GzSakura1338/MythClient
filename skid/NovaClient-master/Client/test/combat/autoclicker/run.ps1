$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-autoclicker'
New-Item -ItemType Directory -Force $out | Out-Null
$source = Get-Content "$client/src/minecraft/modules/combat/autoclicker/AutoClicker.cpp" -Raw -Encoding UTF8
$start = $source.IndexOf('class AutoClickerModule final')
$end = $source.IndexOf('};', $start)
# Find the class closing brace, rather than a lambda/member initializer.
$open = $source.IndexOf('{', $start)
$depth = 1; $end = $open + 1
while ($depth -gt 0 -and $end -lt $source.Length) {
    if ($source[$end] -eq '{') { ++$depth }
    if ($source[$end] -eq '}') { --$depth }
    ++$end
}
if ($start -lt 0 -or $depth -ne 0 -or $source[$end] -ne ';') { throw 'AutoClicker class not found' }
$fixture = Get-Content "$PSScriptRoot/InventoryTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/test.cpp", $fixture.Replace('// @PRODUCTION_AUTOCLICKER_CLASS@', $source.Substring($start, $end - $start + 1)))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /MD test.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try { & .\build.cmd; if ($LASTEXITCODE -ne 0) { throw 'AutoClicker inventory verification failed' } }
finally { Pop-Location }
