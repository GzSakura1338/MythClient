param([switch]$RuntimeOnly)
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$out=Join-Path $client 'build-vs2026/test-slotspoof'
New-Item -ItemType Directory -Force $out | Out-Null
if(!$RuntimeOnly) {
& python "$PSScriptRoot/extract_slot_classes.py"
if($LASTEXITCODE -ne 0){throw 'Slot hook class extraction failed'}
Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach($profile in @(@('lunar','72c13d03/484384a2'),@('1710','10d9b7f1/aed3ca70'))){
    $zip=[IO.Compression.ZipFile]::OpenRead("$env:USERPROFILE/.lunarclient/offline/multiver/cache/$($profile[1])/bake.zip")
    try{foreach($name in @('net.minecraft.entity.player.InventoryPlayer','net.minecraft.client.multiplayer.PlayerControllerMP','net.minecraft.client.renderer.ItemRenderer')){
        $dest="$out/classes/$($profile[0])/$($name.Replace('.','/')).class"
        New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($dest)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($zip.GetEntry($name),$dest,$true)
    }}finally{$zip.Dispose()}
}
}
$source=Get-Content "$client/src/minecraft/java/game/GameSlotSpoof.cpp" -Raw
$methods=''
foreach($signature in @('void ReleaseSilentSlot(', 'bool SetSilentSlot(', 'bool AcquirePhysicalSlot(', 'bool OwnsPhysicalSlot(', 'bool OwnsSlot(', 'void YieldSlot(', 'bool CanChangePhysicalSlot(', 'bool SetOwnedPhysicalSlot(', 'bool SetScaffoldSlot(', 'void ReleaseScaffoldSlot(', 'bool EffectiveSelectedSlot(', 'jint JNICALL SpoofSlotHook(', 'jobject JNICALL VisualInventoryHandHook(', 'jobject JNICALL VisualPlayerHandHook(')){
    $start=$source.IndexOf($signature)
    if($start -lt 0){throw "Missing $signature"}
    $end=$source.IndexOf('{',$start)+1
    $depth=1
    while($depth -gt 0 -and $end -lt $source.Length){
        if($source[$end] -eq '{'){++$depth}
        if($source[$end] -eq '}'){--$depth}
        ++$end
    }
    if($depth -ne 0){throw 'Invalid method'}
    $methods+=$source.Substring($start,$end-$start)+"`n"
}
$fixture=Get-Content "$PSScriptRoot/SlotSpoofTest.cpp" -Raw
[IO.File]::WriteAllText("$out/test.cpp",$fixture.Replace('// @SLOT_METHODS@',$methods))
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 test.cpp /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
if "$RuntimeOnly"=="True" exit /b 0
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/include" /I "$client/src" "$PSScriptRoot/SlotHooksTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe forge classes/forge
if errorlevel 1 exit /b 1
hooks.exe 263 classes/263
if errorlevel 1 exit /b 1
hooks.exe lunar classes/lunar
if errorlevel 1 exit /b 1
hooks.exe 1710 classes/1710
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try{& .\build.cmd; if($LASTEXITCODE -ne 0){throw "Slot spoof tests failed ($LASTEXITCODE)"}}
finally{Pop-Location}
