param(
    [string]$LunarJar='C:/Users/13366/.lunarclient/offline/multiver/cache/d1937f9/2b715c4b/bake.zip',
    [string]$ModernJar='D:/AI/VibeCoding/shader/skid/minecraft-26.3-client.jar',
    [string]$Adapter='GameCrystal.cpp',
    [string]$Macro='NOVA_CRYSTAL',
    [string]$OutputName='test-crystal-mapping',
    [string[]]$MemberKeys=@()
)
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/$OutputName"
New-Item -ItemType Directory -Force $out,"$out/modern","$out/lunar" | Out-Null
$mapping=Get-Content "$client/src/minecraft/compat/mappings/minecraft263.inc" -Raw
$source=Get-Content "$client/src/minecraft/java/game/$Adapter" -Raw
$classes=@{}
foreach($m in [regex]::Matches($mapping,'NOVA_CLASS\((\w+),\s*"([^"]+)"\)')){$classes[$m.Groups[1].Value]=$m.Groups[2].Value}
$members=[Collections.Generic.List[string]]::new()
$owners=[Collections.Generic.HashSet[string]]::new()
if($Macro -eq 'NOVA_STACK') {
    foreach($m in [regex]::Matches($mapping,'NOVA_STACK\(\w+,\s*"([^"]+)",\s*"([^"]+)"\)')) {
        $members.Add('{"net.minecraft.world.item.ItemStack","'+$m.Groups[1].Value+'","'+$m.Groups[2].Value+'"},')
        [void]$owners.Add('net.minecraft.world.item.ItemStack')
    }
}
foreach($m in [regex]::Matches($mapping,([regex]::Escape($Macro)+'\(\w+,"([^"]+)","([^"]+)","([^"]+)"\)'))) {
    $members.Add('{"'+$m.Groups[1].Value+'","'+$m.Groups[2].Value+'","'+$m.Groups[3].Value+'"},')
    [void]$owners.Add($m.Groups[1].Value)
}
foreach($m in [regex]::Matches($mapping,'NOVA_(?:METHOD|FIELD|STATIC_METHOD|STATIC_FIELD)\((\w+),\s*(\w+),\s*"([^"]+)",\s*"([^"]+)"\)')) {
    if($MemberKeys.Count -gt 0) {
        if($m.Groups[1].Value -notin $MemberKeys){continue}
    } elseif($source -notmatch ('\b'+$m.Groups[1].Value+'\b')){continue}
    $owner=$classes[$m.Groups[2].Value]
    if(!$owner){throw "Unknown class for $($m.Value)"}
    $members.Add('{"'+$owner+'","'+$m.Groups[3].Value+'","'+$m.Groups[4].Value+'"},')
    [void]$owners.Add($owner)
}
[IO.File]::WriteAllLines("$out/members.inc",$members)
$lunarMap=Get-Content "$client/src/minecraft/compat/mappings/Lunar12111.inc" -Raw
$translations=@{}
foreach($m in [regex]::Matches($lunarMap,'NOVA_LUNAR12111_CLASS\("([^"]+)", "([^"]+)"\)')){$translations[$m.Groups[1].Value]=$m.Groups[2].Value}
# Also extract known superclasses/interfaces used by the JNI inherited lookup.
foreach($owner in @('net.minecraft.world.item.ItemInstance','net.minecraft.core.TypedInstance','net.minecraft.core.Vec3i','net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase','net.minecraft.world.level.BlockGetter')){[void]$owners.Add($owner)}
Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach($profile in @(@($ModernJar,'modern'),@($LunarJar,'lunar'))) {
    $zip=[IO.Compression.ZipFile]::OpenRead($profile[0])
    try {
        foreach($owner in $owners) {
            $type=if($profile[1] -eq 'lunar'){$translations[$owner]}else{$owner}
            if(!$type){continue}
            $entry=$zip.GetEntry($type.Replace('.','/')+'.class');if(!$entry){$entry=$zip.GetEntry($type)}
            if(!$entry){throw "Missing class $type"}
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,"$out/$($profile[1])/$type.class",$true)
        }
    }finally{$zip.Dispose()}
}
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client" /I "$client/src" /I "$client/include" /I "$out" "$PSScriptRoot/MappingTest.cpp" "$client/src/minecraft/compat/lunar12111/Mapping.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" /Fe:mapping.exe
if errorlevel 1 exit /b 1
mapping.exe modern lunar
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try{& ./build.cmd;if($LASTEXITCODE){throw 'Crystal mapping regression failed'}}finally{Pop-Location}
