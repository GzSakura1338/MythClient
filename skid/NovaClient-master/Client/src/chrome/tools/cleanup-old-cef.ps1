[CmdletBinding(SupportsShouldProcess=$true)]
param()
$ErrorActionPreference='Stop'
$project=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../'))
# Explicitly limited to the abandoned CEF work. No new WebView2 runtime,
# user profile, game file, or repository source is included.
$relativePaths=@(
    'Client/src/chrome/vendor/source-build',
    'Client/src/chrome/vendor/cef.tar.bz2',
    'Client/src/chrome/vendor/cef_binary_154.0.28+g564dd6c+chromium-154.0.8037.58_windows64_minimal',
    'Client/build-browser-kernel',
    'Client/build-vs2026/codec-validation',
    'Client/build-vs2026/browser'
)
$targets=@($relativePaths | ForEach-Object {[IO.Path]::GetFullPath((Join-Path $project $_))})
foreach($target in $targets){
    if(!$target.StartsWith($project.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){throw "Refusing path outside project: $target"}
    if(!(Test-Path -LiteralPath $target)){continue}
    $item=Get-Item -LiteralPath $target -Force
    if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Refusing redirected cleanup target: $target"}
    Write-Output "Old CEF artifact: $target"
    if($PSCmdlet.ShouldProcess($target,'Remove abandoned CEF source/cache/runtime')){Remove-Item -LiteralPath $target -Recurse -Force}
}
