param(
    [string]$Bake="$env:USERPROFILE/.lunarclient/offline/multiver/cache/41523644/2b715c4b/bake.zip",
    [string]$RuntimeDumpDirectory
)
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$client/build-vs2026/test-lunar12111"
New-Item -ItemType Directory -Force "$output/classes","$output/obj" | Out-Null
Add-Type -Path "$PSScriptRoot/Audit.cs"
[LunarClassAudit]::GenerateProbes("$client/src/minecraft","$output/extra.inc")
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead($Bake)
try {
    foreach($entry in $zip.Entries) {
        if($entry.FullName -match '^net\.minecraft\.class_[\w$]+$' -or $entry.FullName.StartsWith('com.moonsworth.')) {
            $target="$output/classes/$($entry.FullName.Replace('.','/')).class"
            New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($target)) | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$true)
        }
    }
    $accountBuild=if($zip.GetEntry('com.moonsworth.lunar.client.IOHOHIHHHHORRCCRHCRCHCRIOIHRHH')){'0930'}else{''}
    if($accountBuild -eq '0930') {
        $stream=$zip.GetEntry('com.moonsworth.lunar.client.IOHOHIHHHHORRCCRHCRCHCRIOIHRHH').Open()
        $bytes=[IO.MemoryStream]::new()
        try {$stream.CopyTo($bytes); if([Text.Encoding]::ASCII.GetString($bytes.ToArray()).Contains('CCCHRRHIOIIOICCIRHRIRCRRHIIIRR')){$accountBuild='1002'}}
        finally {$stream.Dispose();$bytes.Dispose()}
    }
} finally { $zip.Dispose() }
$build=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$output" /I "$client/src" "$PSScriptRoot/Mappings.cpp" "$client/src/minecraft/compat/lunar12111/Mapping.cpp" /Fe:mappings.exe
if errorlevel 1 exit /b 1
mappings.exe > mappings.tsv
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /DNOVA_TEST_LUNAR12111 /I "$output" /I "$client/include" /I "$client/src" "$client/test/easecation/Hooks.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" "$client/src/minecraft/hooks/Edits.cpp" "$client/src/minecraft/compat/lunar12111/Mapping.cpp" "$client/src/minecraft/compat/lunar12111/LunarEdits.cpp" /Fo"obj/" /Fe:hooks.exe
if errorlevel 1 exit /b 1
hooks.exe "$output/classes" "$output/rewritten" $accountBuild
exit /b %ERRORLEVEL%
"@
Set-Content "$output/build.cmd" $build -Encoding ascii
Push-Location $output
try { & ./build.cmd; if($LASTEXITCODE){throw 'Lunar 1.21.11 hook rewrite audit failed'} }
finally { Pop-Location }
if([LunarClassAudit]::Check($Bake,"$output/mappings.tsv","$client/src/minecraft","$PSScriptRoot/adapted.txt")) { throw 'Lunar JNI catalog audit failed' }
if($RuntimeDumpDirectory) {
    & "$output/hooks.exe" $RuntimeDumpDirectory "$output/runtime-rewritten"
    if($LASTEXITCODE){throw 'Lunar runtime Mixin hook rewrite audit failed'}
}
