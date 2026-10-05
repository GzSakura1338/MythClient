param()
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$output="$client/build-vs2026/test-worldcamera"
New-Item -ItemType Directory -Force $output | Out-Null
$archives=@{
    forge='C:/Users/13366/AppData/Roaming/.minecraft/libraries/net/minecraftforge/forge/1.20.1-47.4.23/forge-1.20.1-47.4.23-client.jar'
    easecation='E:/MCLDownload/Game/.minecraft/libraries/net/neoforged/neoforge/21.8.52/neoforge-21.8.52-client.jar'
    lunar12111='C:/Users/13366/.lunarclient/offline/multiver/cache/d1937f9/76a6f84/bake.zip'
    lunar189='C:/Users/13366/.lunarclient/offline/multiver/cache/72c13d03/d3098b28/bake.zip'
    lunar1710='C:/Users/13366/.lunarclient/offline/multiver/cache/10d9b7f1/aed3ca70/bake.zip'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$classes=@('net/minecraft/client/Minecraft','net/minecraft/client/renderer/GameRenderer',
    'net/minecraft/client/renderer/LevelRenderer','net/minecraft/client/renderer/ActiveRenderInfo',
    'net/minecraft/class_310','net/minecraft/class_757','net/minecraft/class_761')
foreach($profile in $archives.Keys) {
    $zip=[IO.Compression.ZipFile]::OpenRead($archives[$profile])
    try {
        foreach($name in $classes) {
            $entry=$zip.GetEntry("$name.class")
            if(!$entry){$entry=$zip.GetEntry($name.Replace('/','.'))}
            if(!$entry){continue}
            $target="$output/$profile/$name.class"
            New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($target)) | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$true)
        }
    } finally {$zip.Dispose()}
}
$build=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/src/third_party/fatalwidgets/imgui" "$PSScriptRoot/ProjectionTest.cpp" "$client/src/minecraft/host/render/WorldProjection.cpp" "$client/src/minecraft/java/game/GameWorldCamera.cpp" /Fe:projection.exe
if errorlevel 1 exit /b 1
projection.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /DNOVA_TEST_LUNAR12111 /I "$client/include" /I "$client/src" "$PSScriptRoot/HooksTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" "$client/src/minecraft/hooks/Edits.cpp" "$client/src/minecraft/compat/lunar12111/Mapping.cpp" "$client/src/minecraft/compat/lunar12111/LunarEdits.cpp" /Fe:hooks.exe
exit /b %ERRORLEVEL%
"@
Set-Content "$output/build.cmd" $build -Encoding ascii
Push-Location $output
try {
    & ./build.cmd
    if($LASTEXITCODE){throw 'World camera build or projection regression failed'}
    foreach($profile in $archives.Keys) {
        & ./hooks.exe $profile "$output/$profile"
        if($LASTEXITCODE){throw "$profile camera hook regression failed"}
    }
} finally {Pop-Location}
