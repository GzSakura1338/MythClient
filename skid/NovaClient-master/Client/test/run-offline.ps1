param([string]$StartAt = '')
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
& python (Join-Path $repoRoot 'tools/check-client-layout.py')
if ($LASTEXITCODE -ne 0) { throw 'Client layout verification failed' }

# These runners compile native fixtures and inspect local class files only.
# Game attach/injection probes have separate entry points.
$suites = @(
    'loader/run_reflective_unwind.ps1',
    'runtime/run.cmd',
    'network/lunar-incoming/run.ps1',
    'combat/aimassist/run.ps1',
    'combat/autoclicker/run.ps1',
    'combat/autopot/run.ps1',
    'combat/displace/run.ps1',
    'combat/antikb/run_antikb_delay.ps1',
    'combat/antikb/run_watchdog_slinky.ps1',
    'combat/antikb/run_prediction.ps1',
    'combat/antikb/run_jump_reset.cmd',
    'combat/autoblock/run_autoblock.ps1',
    'combat/autoblock/run_hypixel.ps1',
    'combat/autoblock/run_block_coverage.cmd',
    'combat/killaura/run_killaura_cps.ps1',
    'combat/keepsprint/run.ps1',
    'combat/backtrack/run.ps1',
    'combat/throwableaura/run.ps1',
    'player/bridgeassist/run.ps1',
    'player/refill/run.ps1',
    'player/breaker/run.ps1',
    'player/scaffold/run.ps1',
    'player/scaffold/run_slotspoof.ps1',
    'player/scaffold/run_fov.ps1',
    'misc/teams/run.ps1',
    'misc/antibot/run.ps1',
    'misc/ghosthand/run.ps1',
    'render/nametags/run.ps1',
    'render/freelook/run.ps1',
    'render/surface/run.ps1',
    'render/notifications/run.cmd'
)
if ($StartAt) {
    $start = [Array]::IndexOf($suites, $StartAt)
    if ($start -lt 0) { throw "Unknown offline suite: $StartAt" }
    $suites = $suites[$start..($suites.Count - 1)]
}
foreach ($suite in $suites) {
    Write-Host "Running $suite"
    & (Join-Path $PSScriptRoot $suite)
    if ($LASTEXITCODE -ne 0) { throw "Offline suite failed: $suite ($LASTEXITCODE)" }
}
Write-Host "PASS: $($suites.Count) offline regression suites"
