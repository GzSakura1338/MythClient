$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-fov'
New-Item -ItemType Directory -Force $out | Out-Null
& python "$PSScriptRoot/extract_slot_classes.py"
if ($LASTEXITCODE -ne 0) { throw 'Camera extraction failed' }
$source = Get-Content "$client/src/minecraft/modules/player/scaffold/Scaffold.cpp" -Raw
$methods = ''
foreach ($signature in @('bool IsEnabled()', 'double AnimateKeepFov(', 'double AnimatedWorldFov(', 'jfloat JNICALL KeepFovFloatHook(', 'jdouble JNICALL KeepFovDoubleHook(', 'jfloat JNICALL KeepCameraFovHook(')) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing $signature" }
    $end = $source.IndexOf('{', $start) + 1
    $depth = 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    $methods += $source.Substring($start, $end - $start) + "`n"
}
$fixture = (Get-Content "$PSScriptRoot/FovTest.cpp" -Raw).Replace('// @FOV_METHODS@', $methods)
$fixture = $fixture.Replace('"../../bytecode/audit.h"', '"' + $client + '/test/bytecode/audit.h"')
[IO.File]::WriteAllText("$out/test.cpp", $fixture)
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" test.cpp "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe forge "$client/build-vs2026/test-slotspoof/classes/forge"
if errorlevel 1 exit /b 1
test.exe 263 "$client/build-vs2026/test-slotspoof/classes/263"
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd", $commands)
Push-Location $out
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw 'FOV verification failed' }
} finally { Pop-Location }
