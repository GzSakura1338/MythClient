param(
    [string]$Jar='E:/MCLDownload/Game/.minecraft/libraries/net/neoforged/neoforge/21.8.52/neoforge-21.8.52-client.jar',
    [string]$RuntimeDumpDirectory='',
    [string]$GameVersion=''
)
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead($Jar)
try {
    $entry=$archive.GetEntry('version.json')
    if($entry) {
        $reader=[IO.StreamReader]::new($entry.Open())
        try {
            $detectedVersion=($reader.ReadToEnd() | ConvertFrom-Json).id
            if($GameVersion -and $GameVersion -ne $detectedVersion){throw 'JAR version does not match requested version'}
            $GameVersion=$detectedVersion
        } finally {$reader.Dispose()}
    } elseif(!$GameVersion -and [IO.Path]::GetFileName($Jar) -match '^neoforge-21\.8\.\d+-client\.jar$') {
        $GameVersion='1.21.8' # The old NeoForge joined JAR omits version.json.
    }
} finally {$archive.Dispose()}
if($gameVersion -notin @('1.21.8','1.21.10')){throw "Unsupported NeoForge version: $gameVersion"}
$output=if($gameVersion -eq '1.21.10'){"$client/build-vs2026/test-neoforge12110"}else{"$client/build-vs2026/test-easecation"}
$versionFlag=if($gameVersion -eq '1.21.10'){'/DNOVA_TEST_NEOFORGE12110'}else{''}
New-Item -ItemType Directory -Force "$output/classes" | Out-Null
python "$PSScriptRoot/audit.py" $Jar "$output/classes" $GameVersion
$mappingFailed=$LASTEXITCODE -ne 0
$build=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /DNOVA_TEST_EASECATION_PROXY $versionFlag /I "$output/classes" /I "$client/include" /I "$client/src" "$PSScriptRoot/Hooks.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" "$client/src/minecraft/hooks/Edits.cpp" "$client/src/minecraft/proxy/ProxyEdits.cpp" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe "$output/classes"
exit /b %ERRORLEVEL%
"@
Set-Content "$output/build.cmd" $build -Encoding ascii
Push-Location $output
try { & ./build.cmd; if($LASTEXITCODE){throw 'EaseCation hook rewrite audit failed'} }
finally { Pop-Location }
if($RuntimeDumpDirectory){
    & "$output/hooks.exe" $RuntimeDumpDirectory "$output/runtime-rewritten"
    if($LASTEXITCODE){throw 'EaseCation runtime hook rewrite audit failed'}
}
if($mappingFailed){throw 'EaseCation JNI mapping audit failed (hook results shown above)'}
