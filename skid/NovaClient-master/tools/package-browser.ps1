$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$source=Join-Path $root 'Client/build-vs2026/browser-webview2/Release'
$out=Join-Path $root 'out/browser'
if(!(Test-Path (Join-Path $source 'NovaBrowserHost.exe')) -or !(Test-Path (Join-Path $source 'runtime/msedgewebview2.exe'))) {throw 'Build the fixed WebView2 NovaBrowserHost Release before publishing the injector'}
New-Item -ItemType Directory -Force $out | Out-Null
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=Join-Path $out 'runtime.zip'
$stream=[IO.File]::Create($zip)
try {
    $archive=[IO.Compression.ZipArchive]::new($stream,[IO.Compression.ZipArchiveMode]::Create,$true)
    try {
        Get-ChildItem $source -Recurse -File | Where-Object {$_.Extension -notin @('.lib','.exp','.pdb')} | ForEach-Object {
            $relative=$_.FullName.Substring($source.TrimEnd('\').Length+1).Replace('\','/')
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$_.FullName,$relative,[IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
    } finally {$archive.Dispose()}
} finally {$stream.Dispose()}
$hasher=[Security.Cryptography.SHA256]::Create()
$inputStream=[IO.File]::OpenRead($zip)
try {$digest=([BitConverter]::ToString($hasher.ComputeHash($inputStream))).Replace('-','').ToLowerInvariant()}
finally {$inputStream.Dispose();$hasher.Dispose()}
[IO.File]::WriteAllText((Join-Path $out 'runtime.sha256'),$digest,[Text.Encoding]::ASCII)
Write-Output "Bundled fixed WebView2: $zip"
