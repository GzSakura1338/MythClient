$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-ghosthand'
New-Item -ItemType Directory -Force $out | Out-Null
$source = Get-Content (Join-Path $client 'src/minecraft/modules/misc/ghosthand/GhostHand.cpp') -Raw
$methods = ''
foreach ($signature in @('void OnClick(', 'bool GhostInteractWithChest(')) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing $signature" }
    $end = $source.IndexOf('{', $start) + 1
    $depth = 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw 'Unterminated method' }
    $methods += $source.Substring($start, $end-$start) + "`n"
}
$fixture = Get-Content (Join-Path $PSScriptRoot 'Test.cpp') -Raw
[IO.File]::WriteAllText((Join-Path $out 'test.cpp'), $fixture.Replace('// @METHODS@', $methods))
$commands = @"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" test.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/Hooks.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe forge "$client/test/bytecode/classes/net.minecraft.client.Minecraft.class"
if errorlevel 1 exit /b 1
hooks.exe lunar "$client/build-vs2026/test-hypixel/classes/net_minecraft_client_Minecraft.class"
if errorlevel 1 exit /b 1
hooks.exe 263 "$client/build-vs2026/mc263-classes/net/minecraft/client/Minecraft.class"
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText((Join-Path $out 'build.cmd'), $commands)
Push-Location $out
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "GhostHand tests failed ($LASTEXITCODE)" }
} finally { Pop-Location }
