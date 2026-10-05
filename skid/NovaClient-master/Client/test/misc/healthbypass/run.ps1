$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-healthbypass"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$client/src/minecraft/modules/misc/healthbypass/HealthBypass.cpp" -Raw -Encoding UTF8
$body=$source.Substring($source.IndexOf('namespace nova::module {'))
$actions=Get-Content "$client/src/minecraft/compat/minecraft263/Minecraft263Actions.cpp" -Raw -Encoding UTF8
$start=$actions.IndexOf('bool ReadHealthScore(')
if($start -lt 0){throw 'Missing modern score decoder'}
$end=$actions.IndexOf('{',$start)+1
$depth=1
while($depth -gt 0 -and $end -lt $actions.Length) {
    if($actions[$end] -eq '{'){++$depth}
    if($actions[$end] -eq '}'){--$depth}
    ++$end
}
if($depth -ne 0){throw 'Unclosed modern score decoder'}
$fixture=Get-Content "$PSScriptRoot/HealthBypassTest.cpp" -Raw -Encoding UTF8
[IO.File]::WriteAllText("$out/test.cpp",$fixture.Replace('// @PRODUCTION_MODULE@',$body).Replace('// @PRODUCTION_DECODER@',$actions.Substring($start,$end-$start)))
$commands=@'
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 test.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
'@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & ./build.cmd; if($LASTEXITCODE){throw 'HealthBypass verification failed'} }
finally { Pop-Location }
