$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
Add-Type -AssemblyName System.Runtime.WindowsRuntime
$null = [Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager, Windows.Media.Control, ContentType = WindowsRuntime]
$mediaAwait = [System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
    $_.Name -eq 'AsTask' -and $_.IsGenericMethod -and $_.GetGenericArguments().Count -eq 1 -and
    $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
} | Select-Object -First 1
function Receive-MediaOperation($operation, [Type]$resultType) {
    $mediaTask = $mediaAwait.MakeGenericMethod($resultType).Invoke($null, @($operation))
    $mediaTask.Wait()
    return $mediaTask.Result
}
$mediaManager = Receive-MediaOperation ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager]::RequestAsync()) ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager])
$mediaSession = $mediaManager.GetCurrentSession()
if ($null -eq $mediaSession) { exit }
$mediaProperties = Receive-MediaOperation ($mediaSession.TryGetMediaPropertiesAsync()) ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionMediaProperties, Windows.Media.Control, ContentType = WindowsRuntime])
$mediaPlayback = $mediaSession.GetPlaybackInfo()
@{artist = [string]$mediaProperties.Artist; title = [string]$mediaProperties.Title; playing = [int]$mediaPlayback.PlaybackStatus -eq 4} | ConvertTo-Json -Compress
