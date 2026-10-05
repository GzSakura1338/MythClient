$ErrorActionPreference = 'Stop'
$clientRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outDir = Join-Path $clientRoot 'build-vs2026/test-antibot'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$source = Get-Content (Join-Path $clientRoot 'src/minecraft/modules/misc/antibot/AntiBot.cpp') -Raw -Encoding UTF8
$start = $source.IndexOf('namespace antibot {')
$end = $source.IndexOf('std::unique_ptr<Module> MakeAntiBotModule()', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Cannot locate AntiBot implementation' }
$fixture = Get-Content (Join-Path $PSScriptRoot 'AntiBotTest.cpp') -Raw -Encoding UTF8
[IO.File]::WriteAllText((Join-Path $outDir 'test.cpp'), $fixture.Replace('// @PRODUCTION_ANTIBOT_API@', $source.Substring($start, $end - $start)))
$commands = @'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul || exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I../../src /I../../include test.cpp /Fe:test.exe || exit /b 1
test.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText((Join-Path $outDir 'build.cmd'), $commands)
Push-Location $outDir
try {
    & .\build.cmd
    if ($LASTEXITCODE -ne 0) { throw "AntiBot verification failed ($LASTEXITCODE)" }
} finally { Pop-Location }
