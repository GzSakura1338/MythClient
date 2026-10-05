param([string]$BuildDirectory=(Join-Path $PSScriptRoot '../../../build-vs2026/browser-webview2/Release'),
      [string]$TargetDirectory=(Join-Path $env:LOCALAPPDATA 'Nova/Browser/runtime/webview2-154.0.4258.37-v1'))
$ErrorActionPreference='Stop'
$target=$TargetDirectory
if(!(Test-Path (Join-Path $BuildDirectory 'NovaBrowserHost.exe'))) {throw 'Build NovaBrowserHost Release first'}
New-Item -ItemType Directory -Force $target | Out-Null
Get-ChildItem $BuildDirectory -File | Where-Object {$_.Extension -notin @('.lib','.exp','.pdb')} | Copy-Item -Destination $target -Force
Copy-Item (Join-Path $BuildDirectory 'runtime') $target -Recurse -Force
Write-Output "Browser runtime ready: $target"
