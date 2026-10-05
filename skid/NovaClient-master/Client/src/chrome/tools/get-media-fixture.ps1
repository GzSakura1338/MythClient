$ErrorActionPreference='Stop'
$fixtures=Join-Path $PSScriptRoot '../../../test/chrome/fixtures'
New-Item -ItemType Directory -Force $fixtures | Out-Null
$file=Join-Path $fixtures 'bear.mp4'
Invoke-WebRequest 'https://raw.githubusercontent.com/chromium/chromium/154.0.8037.58/media/test/data/bear.mp4' -OutFile $file
if((Get-FileHash $file -Algorithm SHA256).Hash -ne 'AA2786362E3D61646390BA23CB0639B6AE9C9F48B58A882A7622D94C8AFE7FE1'){throw 'Chromium media fixture checksum mismatch'}
Write-Output "H.264/AAC test fixture ready: $file"
