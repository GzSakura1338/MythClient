$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-freecam"
New-Item -ItemType Directory -Force $out | Out-Null
Add-Type -Path "$client/test/lunar12111/Audit.cs"
Add-Type -AssemblyName System.IO.Compression.FileSystem
$profiles=@(
    @('lunar',"$env:USERPROFILE/.lunarclient/offline/multiver/cache/72c13d03/484384a2/bake.zip",'Lunar189.inc','net.minecraft.client.entity.EntityPlayerSP'),
    @('1710',"$env:USERPROFILE/.lunarclient/offline/multiver/cache/10d9b7f1/aed3ca70/bake.zip",'Lunar189.inc','net.minecraft.client.entity.EntityClientPlayerMP'),
    @('forge',"$env:APPDATA/.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar",'forge1201.inc','net.minecraft.client.player.LocalPlayer'),
    @('easecation','E:/MCLDownload/Game/.minecraft/libraries/net/neoforged/neoforge/21.8.52/neoforge-21.8.52-client.jar','minecraft263.inc','net.minecraft.client.player.LocalPlayer'),
    @('263',"$env:APPDATA/.minecraft/versions/26.3/26.3.jar",'minecraft263.inc','net.minecraft.client.player.LocalPlayer')
)
foreach($profile in $profiles){
    $name,$jar,$mapping,$player=$profile
    $source=Get-Content "$client/src/minecraft/compat/mappings/$mapping" -Raw
    $keyIds=@([regex]::Matches($source,'NOVA_MOVEMENT_KEY\((\w+),') | ForEach-Object {$_.Groups[1].Value})
    foreach($required in @('options','jump','sneak','forward','back','left','right','sprint','drop','attack','use','clicks','pressed','keyCode')){
        if($required -notin $keyIds){throw "$name missing required movement mapping: $required"}
    }
    $lines=foreach($m in [regex]::Matches($source,'NOVA_(?:FREECAM|MOVEMENT_KEY)\((\w+),\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)')){
        $id,$owner,$member,$desc=$m.Groups[1..4].Value
        # 1.7 uses the renderViewEntity field and has no camera-based send gate.
        if($name -eq '1710' -and $id -in @('controlled','view','setView')){continue}
        # GLFW profiles read physical keys through GLFW, not the 26.3 input API.
        if($name -eq 'easecation' -and $id -eq 'physicalKey'){continue}
        @("extra:$id",$owner,$member,$desc,$owner,$member,$desc) -join "`t"
    }
    Set-Content "$out/$name.tsv" $lines
    if([LunarClassAudit]::Check($jar,"$out/$name.tsv","$client/src/minecraft","$out/no-exclusions")){throw "$name FreeCam mapping audit failed"}
    $zip=[IO.Compression.ZipFile]::OpenRead($jar)
    $owners=@('net.minecraft.client.Minecraft',$player)
    if($name -in @('lunar','1710')){$owners+='net.minecraft.client.renderer.EntityRenderer'}
    try{foreach($owner in $owners){
        $entry=$zip.GetEntry($owner)
        if(!$entry){$entry=$zip.GetEntry($owner.Replace('.','/')+'.class')}
        $dest="$out/classes/$name/$($owner.Replace('.','/')).class"
        New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($dest)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$dest,$true)
    }}finally{$zip.Dispose()}
}
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/AccessTest.cpp" "$client/src/minecraft/compat/minecraft263/Minecraft263Access.cpp" /Fe:access-test.exe
if errorlevel 1 exit /b 1
access-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/FreeCamTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe lunar classes/lunar
if errorlevel 1 exit /b 1
test.exe 1710 classes/1710
if errorlevel 1 exit /b 1
test.exe forge classes/forge
if errorlevel 1 exit /b 1
test.exe easecation classes/easecation
if errorlevel 1 exit /b 1
test.exe 263 classes/263
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try{& ./build.cmd;if($LASTEXITCODE){throw 'FreeCam native tests failed'}}finally{Pop-Location}
