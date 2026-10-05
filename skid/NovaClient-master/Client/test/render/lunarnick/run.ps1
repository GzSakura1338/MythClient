$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-lunar-nick"
New-Item -ItemType Directory -Force $out | Out-Null
$mapping=Get-Content "$client/src/minecraft/compat/mappings/LunarNick.h" -Raw
$classes=[regex]::Matches($mapping,'const char\* \w+="([^"]+)";') | ForEach-Object { $_.Groups[1].Value }
$archives=@{
    lunar189='C:/Users/13366/.lunarclient/offline/multiver/cache/72c13d03/d3098b28/bake.zip'
    lunar12111='C:/Users/13366/.lunarclient/offline/multiver/cache/d1937f9/76a6f84/bake.zip'
    lunar12111October='C:/Users/13366/.lunarclient/offline/multiver/cache/d1937f9/cc81b79c/bake.zip'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach($profile in $archives.Keys){$zip=[IO.Compression.ZipFile]::OpenRead($archives[$profile]);try{
    foreach($name in $classes){$entry=$zip.GetEntry($name);if(!$entry){continue}
        $target="$out/$profile/$($name.Replace('.','/')).class";New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($target)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$true)
    }
}finally{$zip.Dispose()}}
foreach($profile in @('lunar189','lunar1710')){
    $path=if($profile -eq 'lunar1710'){'C:/Users/13366/.lunarclient/offline/multiver/cache/10d9b7f1/aed3ca70/bake.zip'}else{$archives.lunar189}
    $zip=[IO.Compression.ZipFile]::OpenRead($path)
    try{$entry=$zip.GetEntry('net.minecraft.client.gui.FontRenderer');$target="$out/$profile/net/minecraft/client/gui/FontRenderer.class"
        New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($target)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$true)
    }finally{$zip.Dispose()}
}
$build=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" "$PSScriptRoot/Test.cpp" "$client/src/minecraft/java/game/GamePresentationName.cpp" /Fe:test.exe
if errorlevel 1 exit /b 1
test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" "$PSScriptRoot/MappingTest.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" /Fe:mapping.exe
exit /b %ERRORLEVEL%
"@
Set-Content "$out/build.cmd" $build -Encoding ascii
Push-Location $out
try{& ./build.cmd;if($LASTEXITCODE){throw 'Lunar nick runtime test failed'}
    foreach($profile in $archives.Keys){& ./mapping.exe "$out/$profile";if($LASTEXITCODE){throw "$profile nick mapping failed"}}
    foreach($profile in @('lunar189','lunar1710')){& ./mapping.exe "$out/$profile" legacy;if($LASTEXITCODE){throw "$profile legacy nick mapping failed"}}
}finally{Pop-Location}
