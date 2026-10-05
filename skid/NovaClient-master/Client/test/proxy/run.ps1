param([string]$RuntimeDumpDirectory='')
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$client/build-vs2026/test-proxy"
New-Item -ItemType Directory -Force $out | Out-Null
$transformer=Get-Content "$client/src/minecraft/hooks/Transformer.cpp" -Raw
$start=$transformer.IndexOf('bool LoadRequiredTargets(')
if($start -lt 0){throw 'Missing required-target preloading'}
$open=$transformer.IndexOf('{',$start);$depth=1;$end=$open+1
while($depth -gt 0){if($transformer[$end] -eq '{'){$depth++};if($transformer[$end] -eq '}'){$depth--};$end++}
$fixture=Get-Content "$PSScriptRoot/TargetLoadTest.cpp" -Raw
[IO.File]::WriteAllText("$out/target-load.cpp",$fixture.Replace('// @PRODUCTION_LOAD_TARGETS@',$transformer.Substring($start,$end-$start)))
$archives=@{
    '263'='C:/Users/13366/AppData/Roaming/.minecraft/versions/26.3/26.3.jar'
    lunar189='C:/Users/13366/.lunarclient/offline/multiver/cache/72c13d03/d3098b28/bake.zip'
    lunar1710='C:/Users/13366/.lunarclient/offline/multiver/cache/10d9b7f1/aed3ca70/bake.zip'
    lunar12111='C:/Users/13366/.lunarclient/offline/multiver/cache/d1937f9/76a6f84/bake.zip'
    forge='C:/Users/13366/AppData/Roaming/.minecraft/libraries/net/minecraftforge/forge/1.20.1-47.4.23/forge-1.20.1-47.4.23-client.jar'
    easecation='E:/MCLDownload/Game/.minecraft/libraries/net/neoforged/neoforge/21.8.52/neoforge-21.8.52-client.jar'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$classes=@('net/minecraft/network/NetworkManager','net/minecraft/client/multiplayer/GuiConnecting$1',
    'net/minecraft/network/Connection','net/minecraft/client/gui/screens/ConnectScreen$1','net/minecraft/class_2535','net/minecraft/class_412$1')
foreach($profile in $archives.Keys){$zip=[IO.Compression.ZipFile]::OpenRead($archives[$profile]);try{
    foreach($name in $classes){$entry=$zip.GetEntry("$name.class");if(!$entry){$entry=$zip.GetEntry($name.Replace('/','.'))};if(!$entry){continue}
        $path="$out/$profile/$name.class";New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($path)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$path,$true)
    }
}finally{$zip.Dispose()}}
$build=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/ProtocolTest.cpp" "$client/src/proxy/ProxyService.cpp" "$client/src/utils/Json.cpp" /Fe:protocol.exe /link ws2_32.lib iphlpapi.lib crypt32.lib
if errorlevel 1 exit /b 1
protocol.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" "$PSScriptRoot/AdapterTest.cpp" "$client/src/minecraft/proxy/ProxyHooks.cpp" /Fe:adapter.exe
if errorlevel 1 exit /b 1
adapter.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" target-load.cpp /Fe:target-load.exe
if errorlevel 1 exit /b 1
target-load.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" /I "$client/include" "$PSScriptRoot/HooksTest.cpp" "$client/src/minecraft/proxy/ProxyEdits.cpp" "$client/src/minecraft/hooks/Edits.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" /Fe:hooks.exe
exit /b %ERRORLEVEL%
"@
Set-Content "$out/build.cmd" $build -Encoding ascii
Push-Location $out
try{
    & ./build.cmd;if($LASTEXITCODE){throw 'Proxy protocol/build failed'}
    foreach($profile in $archives.Keys){& ./hooks.exe $profile "$out/$profile";if($LASTEXITCODE){throw "$profile proxy hook audit failed"}}
    if($RuntimeDumpDirectory){& ./hooks.exe lunar189 $RuntimeDumpDirectory;if($LASTEXITCODE){throw 'Lunar runtime proxy hook audit failed'}}
}finally{Pop-Location}
