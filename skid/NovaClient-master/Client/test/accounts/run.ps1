param([switch]$PortraitNetwork,[string]$LunarJar)
$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$client/build-vs2026/test-accounts"
New-Item -ItemType Directory -Force $out | Out-Null
$lunarCommands=''
if($LunarJar) {
    # Include inheritance targets for checking all account JNI members.
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $LunarJar))
    try {
        $build=if($zip.GetEntry('com.moonsworth.lunar.client.IOHOHIHHHHORRCCRHCRCHCRIOIHRHH')){'0930'}else{'0927'}
        if($build -eq '0930') {
            $stream=$zip.GetEntry('com.moonsworth.lunar.client.IOHOHIHHHHORRCCRHCRCHCRIOIHRHH').Open()
            $bytes=[IO.MemoryStream]::new()
            try {$stream.CopyTo($bytes); if([Text.Encoding]::ASCII.GetString($bytes.ToArray()).Contains('CCCHRRHIOIIOICCIRHRIRCRRHIIIRR')){$build='1002'}}
            finally {$stream.Dispose();$bytes.Dispose()}
        }
        $legacy=[bool]$zip.GetEntry('net.minecraft.util.Session')
        $sample=if($legacy){"lunar189-$build"}else{"lunar-$build"}
        foreach($entry in $zip.Entries) {
            if(!$entry.FullName.StartsWith('com.moonsworth.') -and !($legacy -and $entry.FullName.StartsWith('net.minecraft.'))){continue}
            $target=Join-Path "$out/$sample" ($entry.FullName.Replace('.','/')+'.class')
            New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($target)) | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$true)
        }
    } finally {$zip.Dispose()}
    $lunarCommands=@"
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/LunarRedirectTest.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" /Fe:lunar-redirect.exe
if errorlevel 1 exit /b 1
lunar-redirect.exe "$out/$sample" $build
"@
    if($legacy){$lunarCommands+=@"

if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /DNOVA_TEST_LUNAR189 /I "$client/src" "$client/test/easecation/Hooks.cpp" "$client/src/minecraft/java/bytecode/ClassEdit.cpp" "$client/src/minecraft/java/bytecode/ClassFile.cpp" "$client/src/minecraft/hooks/Edits.cpp" /Fe:lunar189-hooks.exe
if errorlevel 1 exit /b 1
lunar189-hooks.exe "$out/$sample" "$out/rewritten189" $build
"@}
}
$portraitCommands=if($PortraitNetwork){@"
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /DNOMINMAX /I "$client/src" /I "$client/src/third_party/fatalwidgets/imgui" "$PSScriptRoot/PortraitTest.cpp" "$client/src/minecraft/accounts/AccountPortraits.cpp" /Fe:portrait.exe /link winhttp.lib windowscodecs.lib ole32.lib
if errorlevel 1 exit /b 1
portrait.exe
"@}else{''}
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" "$PSScriptRoot/BoundaryTest.cpp" "$client/src/ui/clickgui/accounts/AccountService.cpp" /Fe:boundary.exe
if errorlevel 1 exit /b 1
boundary.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /DNOMINMAX /I "$client/src" "$PSScriptRoot/MicrosoftTest.cpp" "$client/src/minecraft/accounts/MicrosoftAuth.cpp" "$client/src/minecraft/accounts/CredentialStore.cpp" "$client/src/utils/Json.cpp" /Fe:microsoft.exe /link winhttp.lib bcrypt.lib crypt32.lib ws2_32.lib shell32.lib
if errorlevel 1 exit /b 1
microsoft.exe
$lunarCommands
$portraitCommands
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try { & ./build.cmd; if($LASTEXITCODE){throw 'Account boundary tests failed'} } finally { Pop-Location }
