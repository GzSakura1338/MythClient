$ErrorActionPreference='Stop'
$vendor=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../vendor'))
New-Item -ItemType Directory -Force $vendor | Out-Null
$runtime=Join-Path $vendor 'webview2-fixed/Microsoft.WebView2.FixedVersionRuntime.154.0.4258.37.x64'
$sdk=Join-Path $vendor 'webview2-sdk'
function Get-VerifiedArchive($Url,$Path,$Hash) {
    if(!(Test-Path -LiteralPath $Path) -or (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $Hash){Invoke-WebRequest $Url -OutFile $Path}
    if((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $Hash){throw "Checksum mismatch: $Path"}
}
if(!(Test-Path "$sdk/build/native/include/WebView2.h")){
    $archive=Join-Path $vendor 'webview2-sdk.zip'
    Get-VerifiedArchive 'https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/1.0.4191.47/microsoft.web.webview2.1.0.4191.47.nupkg' $archive 'F492BBF547D0DA329553B6727435B677579B1E9F91CC9E4A1AD029366D5F23D0'
    Expand-Archive -LiteralPath $archive -DestinationPath $sdk -Force
}
if(!(Test-Path "$runtime/msedgewebview2.exe")){
    $archive=Join-Path $vendor 'webview2-fixed-x64.cab'
    Get-VerifiedArchive 'https://msedge.sf.dl.delivery.mp.microsoft.com/filestreamingservice/files/b82d47e8-d146-4563-94d1-3a3176b25c0a/Microsoft.WebView2.FixedVersionRuntime.154.0.4258.37.x64.cab' $archive '143DA7F7C4939FDDD3875ED918E44022D7EB87063BF912FE3E32DF37C6B0B8C3'
    New-Item -ItemType Directory -Force (Split-Path $runtime) | Out-Null
    & expand.exe $archive '-F:*' (Split-Path $runtime) | Out-Null
    if($LASTEXITCODE -ne 0){throw 'Fixed runtime extraction failed'}
}
$signature=Get-AuthenticodeSignature "$runtime/msedgewebview2.exe"
if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation'){throw 'Microsoft runtime signature validation failed'}
Write-Output "Verified fixed x64 WebView2 runtime: $runtime"
