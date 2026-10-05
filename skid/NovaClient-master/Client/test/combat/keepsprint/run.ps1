$ErrorActionPreference = 'Stop'
$client = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out = Join-Path $client 'build-vs2026/test-keepsprint'
& python (Join-Path $PSScriptRoot 'verify.py')
if ($LASTEXITCODE -ne 0) { throw 'KeepSprint mapping / oracle generation failed' }
function Extract([string]$source,[string]$signature) {
    $start=$source.IndexOf($signature)
    if($start -lt 0){throw "Missing $signature"}
    $end=$source.IndexOf('{',$start)+1;$depth=1
    while($depth -gt 0 -and $end -lt $source.Length){
        if($source[$end] -eq '{'){++$depth}
        if($source[$end] -eq '}'){--$depth}
        ++$end
    }
    if($depth){throw "Unclosed $signature"}
    $source.Substring($start,$end-$start)
}
$game=(Get-Content "$client/src/minecraft/java/game/GameKeepSprint.cpp" -Raw) -replace '(?m)^#include (?!"minecraft/compat/mappings/).*\r?\n',''
$lunar=Get-Content "$client/src/minecraft/compat/lunar189/LunarRuntime.cpp" -Raw
$methods=foreach($signature in @('void JNICALL AttackMotionXHook(', 'void JNICALL AttackMotionZHook(', 'void JNICALL AttackSprintingHook(')){Extract $lunar $signature}
$fixture=Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$out/runtime.cpp",$fixture.Replace('// @GAME_PRODUCTION@',$game).Replace('// @LUNAR_PRODUCTION@',($methods -join "`n")))
$module=(Get-Content "$client/src/minecraft/modules/combat/keepsprint/KeepSprint.cpp" -Raw) -replace '(?m)^#include.*\r?\n',''
$properties=Get-Content "$PSScriptRoot/PropertiesTest.cpp" -Raw
[IO.File]::WriteAllText("$out/properties.cpp",$properties.Replace('// @MODULE_PRODUCTION@',$module))
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" state.cpp /Fe:state.exe
if errorlevel 1 exit /b 1
state.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" properties.cpp "$client/src/utils/Json.cpp" "$client/src/minecraft/utils/rotation/MovementFix.cpp" /Fe:properties.exe
if errorlevel 1 exit /b 1
properties.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/Hooks.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe lunar net/minecraft/entity/player/EntityPlayer lunar-0.class
if errorlevel 1 exit /b 1
hooks.exe lunar net/minecraft/client/multiplayer/PlayerControllerMP lunar-1.class
if errorlevel 1 exit /b 1
hooks.exe forge net/minecraft/world/entity/player/Player forge-0.class
if errorlevel 1 exit /b 1
hooks.exe forge net/minecraft/client/multiplayer/MultiPlayerGameMode forge-1.class
if errorlevel 1 exit /b 1
hooks.exe 263 net/minecraft/world/entity/player/Player 263-0.class
if errorlevel 1 exit /b 1
hooks.exe 263 net/minecraft/client/multiplayer/MultiPlayerGameMode 263-1.class
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & .\build.cmd; if($LASTEXITCODE -ne 0){throw "KeepSprint native tests failed ($LASTEXITCODE)"} }
finally { Pop-Location }
