param(
    [string]$LunarJar='C:/Users/13366/.lunarclient/offline/multiver/cache/d1937f9/2b715c4b/bake.zip',
    [string]$ModernJar='D:/AI/VibeCoding/shader/skid/minecraft-26.3-client.jar'
)
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-automace-mapping"
New-Item -ItemType Directory -Force $out,"$out/modern","$out/lunar" | Out-Null
$mapping=Get-Content "$client/src/minecraft/compat/mappings/minecraft263.inc" -Raw
$guard=[regex]::Match($mapping,'(?s)constexpr Guard mc263GameModeGuards\[\]\{.*?\n\};').Value
if(!$guard){throw 'Missing production attack guard'}
[IO.File]::WriteAllText("$out/modern-guard.inc",$guard)
$section=[regex]::Match($mapping,'(?s)#if defined\(NOVA_MACE_MAPPING\)(.*?)#elif').Groups[1].Value
$owners=@([regex]::Matches($section,'NOVA_MACE\([^,]+,"([^"]+)"') | ForEach-Object {$_.Groups[1].Value})
$owners+= 'net.minecraft.client.multiplayer.MultiPlayerGameMode'
$owners=$owners | Select-Object -Unique
$lunarMap=Get-Content "$client/src/minecraft/compat/mappings/Lunar12111.inc" -Raw
$classes=@{}
foreach($entry in [regex]::Matches($lunarMap,'NOVA_LUNAR12111_CLASS\("([^"]+)", "([^"]+)"\)')){
    $classes[$entry.Groups[1].Value]=$entry.Groups[2].Value
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach($profile in @(@($ModernJar,'modern'),@($LunarJar,'lunar'))){
    $zip=[IO.Compression.ZipFile]::OpenRead($profile[0])
    try{
        $profileOwners=$owners
        if($profile[1] -eq 'modern'){$profileOwners+= 'net.minecraft.world.item.ItemInstance','net.minecraft.core.TypedInstance'}
        foreach($owner in $profileOwners){
            $type=if($profile[1] -eq 'lunar'){$classes[$owner]}else{$owner}
            if(!$type){throw "Missing mapping for $owner"}
            $entry=$zip.GetEntry($type.Replace('.','/')+'.class')
            if(!$entry){$entry=$zip.GetEntry($type)} # Lunar bake uses dotted entry names.
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
try{& ./build.cmd;if($LASTEXITCODE){throw 'AutoMace mapping regression failed'}}finally{Pop-Location}
