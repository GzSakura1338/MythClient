$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out=Join-Path $client 'build-vs2026/test-esp2d'
New-Item -ItemType Directory -Force $out | Out-Null
# Select the actual profile jar, not the most recently modified Lunar cache:
# that cache can now belong to 1.7 or 1.21 and cannot validate 1.8 mappings.
Add-Type -Path "$client/test/lunar12111/Audit.cs"
foreach($profile in @(
    @('lunar',"$env:USERPROFILE/.lunarclient/offline/multiver/cache/72c13d03/484384a2/bake.zip",'Lunar189.inc'),
    @('forge',"$env:APPDATA/.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar",'forge1201.inc'),
    @('263',"$env:APPDATA/.minecraft/versions/26.3/26.3.jar",'minecraft263.inc')
)){
    $name,$jar,$mapping=$profile
    $source=Get-Content "$client/src/minecraft/compat/mappings/$mapping" -Raw
    $lines=foreach($m in [regex]::Matches($source,'NOVA_ESP2D\((\w+),\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)')){
        $id,$owner,$member,$desc=$m.Groups[1..4].Value
        @("extra:$id",$owner,$member,$desc,$owner,$member,$desc) -join "`t"
    }
    Set-Content "$out/$name.tsv" $lines
    if([LunarClassAudit]::Check($jar,"$out/$name.tsv","$client/src/minecraft","$out/no-exclusions")){throw "$name 2DESP mapping verification failed"}
}
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" "$PSScriptRoot/Esp2DTest.cpp" "$client/src/utils/Color.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & .\build.cmd; if($LASTEXITCODE -ne 0){throw '2DESP verification failed'} }
finally { Pop-Location }
