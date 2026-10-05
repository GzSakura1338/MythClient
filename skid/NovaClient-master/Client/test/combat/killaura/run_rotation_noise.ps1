$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-killaura-noise"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$client/src/minecraft/modules/combat/killaura/KillAura.cpp" -Raw -Encoding UTF8
$start=$source.IndexOf('rotation::Rotation NoisyAim(')
if($start -lt 0){throw 'Missing production noise method'}
$end=$source.IndexOf('{',$start)+1
$depth=1
while($depth -gt 0 -and $end -lt $source.Length){
    if($source[$end] -eq '{'){++$depth}
    if($source[$end] -eq '}'){--$depth}
    ++$end
}
if($depth){throw 'Unclosed noise method'}
$fixture=Get-Content "$PSScriptRoot/RotationNoiseTest.cpp" -Raw
[IO.File]::WriteAllText("$out/test.cpp",$fixture.Replace('// @PRODUCTION_NOISY_AIM@',$source.Substring($start,$end-$start)))
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
try { & ./build.cmd; if($LASTEXITCODE){throw 'KillAura rotation noise verification failed'} } finally { Pop-Location }
