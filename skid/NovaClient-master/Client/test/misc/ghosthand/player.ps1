$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-ghosthand-player"
New-Item -ItemType Directory -Force $out | Out-Null
Add-Type -Path "$client/test/lunar12111/Audit.cs"
Add-Type -AssemblyName System.IO.Compression.FileSystem
$profiles=@(
    @('lunar',"$env:USERPROFILE/.lunarclient/offline/multiver/cache/72c13d03/484384a2/bake.zip",'Lunar189.inc','net.minecraft.client.renderer.EntityRenderer'),
    @('1710',"$env:USERPROFILE/.lunarclient/offline/multiver/cache/10d9b7f1/aed3ca70/bake.zip",'Lunar189.inc','net.minecraft.client.renderer.EntityRenderer'),
    @('forge',"$env:APPDATA/.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar",'forge1201.inc','net.minecraft.client.renderer.GameRenderer'),
    @('easecation','E:/MCLDownload/Game/.minecraft/libraries/net/neoforged/neoforge/21.8.52/neoforge-21.8.52-client.jar','minecraft263.inc','net.minecraft.client.renderer.GameRenderer'),
    @('263',"$env:APPDATA/.minecraft/versions/26.3/26.3.jar",'minecraft263.inc','net.minecraft.client.Minecraft')
)
foreach($profile in $profiles){
    $name,$jar,$mapping,$owner=$profile
    $source=Get-Content "$client/src/minecraft/compat/mappings/$mapping" -Raw
    $lines=foreach($m in [regex]::Matches($source,'NOVA_GHOSTHAND\((\w+),\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)')){
        $id,$type,$member,$desc=$m.Groups[1..4].Value
        if($id -eq 'riderInteract'){continue} # Optional Forge extension, probed at runtime.
        if($name -eq '1710'){
            if($id -eq 'view'){$member='renderViewEntity';$desc='Lnet/minecraft/entity/EntityLivingBase;'}
            if($id -eq 'look'){$type='net.minecraft.entity.EntityLivingBase'}
            if($id -eq 'canCollide'){$member='canStopRayTrace';$desc='(IZ)Z'}
        }
        @("extra:ghosthand:$id",$type,$member,$desc,$type,$member,$desc) -join "`t"
    }
    foreach($classes in [regex]::Matches($source,'NOVA_GHOSTHAND_CLASSES\(([^\r\n]+)\)')){
        foreach($entry in [regex]::Matches($classes.Groups[1].Value,'"([^"]*)"')){
            $type=$entry.Groups[1].Value
            if(!$type){continue}
            # 26.3 removed HoeItem; the adapter reads its stable untranslated item key.
            if($name -eq '263' -and $type -eq 'net.minecraft.world.item.HoeItem'){continue}
            $lines+=@("extra:ghosthand:class:$type",$type,'','',$type,'','') -join "`t"
        }
    }
    Set-Content "$out/$name.tsv" $lines
    if([LunarClassAudit]::Check($jar,"$out/$name.tsv","$client/src/minecraft","$out/no-exclusions")){throw "$name GhostHand mapping audit failed"}
    $zip=[IO.Compression.ZipFile]::OpenRead($jar)
    try{
        $entry=$zip.GetEntry($owner)
        if(!$entry){$entry=$zip.GetEntry($owner.Replace('.','/')+'.class')}
        if(!$entry){throw "Missing $owner in $jar"}
        $dest="$out/classes/$name/$($owner.Replace('.','/')).class"
        New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($dest)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$dest,$true)
    }finally{$zip.Dispose()}
}
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/PlayerTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:player-test.exe
if errorlevel 1 exit /b 1
"@
foreach($profile in $profiles){$name=$profile[0];$commands+="`r`nplayer-test.exe $name classes/$name`r`nif errorlevel 1 exit /b 1"}
$commands+="`r`nexit /b 0`r`n"
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try{& ./build.cmd;if($LASTEXITCODE){throw 'GhostHand Player verification failed'}}finally{Pop-Location}
