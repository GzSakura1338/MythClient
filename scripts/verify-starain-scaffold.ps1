param([string] $Workspace = (Split-Path -Parent $PSScriptRoot))
$ErrorActionPreference = 'Stop'
$referenceRoot = Join-Path $Workspace 'skid/Starain-1.20.1-main/src/main/java/today/starain'
function Read-Normalized([string] $Path) { return [IO.File]::ReadAllText($Path).Replace("`r`n", "`n") }
function Get-Method([string] $Source, [string] $Name) {
    $match = [regex]::Match($Source, '(?m)^    (?:public|private) (?:static )?[^\n]+\b' + [regex]::Escape($Name) + '\([^\n]*\) \{')
    if (!$match.Success) { throw "Method missing: $Name" }
    $brace = $Source.IndexOf('{', $match.Index)
    $depth = 1
    for ($i = $brace + 1; $i -lt $Source.Length; $i++) {
        if ($Source[$i] -eq '{') { $depth++ }
        if ($Source[$i] -eq '}') { $depth-- }
        if ($depth -eq 0) { return $Source.Substring($match.Index, $i - $match.Index + 1) }
    }
    throw "Unclosed method: $Name"
}
$expected = Read-Normalized (Join-Path $referenceRoot 'module/impl/player/Scaffold.java')
$expected = $expected.Replace('MathUtils.getRandomInRange', 'randomRange').Replace('block instanceof BushBlock', 'block instanceof VegetationBlock').Replace('block instanceof FungusBlock', 'block instanceof NetherFungusBlock').Replace('getInventory().getSelected()', 'getInventory().getSelectedItem()').Replace('rayCast.isMode("Normal")', 'rayCast.get().equals("Normal")').Replace('rayCast.isMode("Strict")', 'rayCast.get().equals("Strict")')
$actual = Read-Normalized (Join-Path $Workspace 'src/dev/myth/model/player/Scaffold.java')
$methods = @('getVec3', 'isValidStack', 'findBlockSlot', 'place', 'getYLevel', 'getBlockInfo', 'checkBlock', 'getRotation', 'onAir', 'isOnBlockEdge')
foreach ($method in $methods) {
    if ((Get-Method $expected $method) -cne (Get-Method $actual $method)) { throw "Undeclared algorithm difference: $method" }
    Write-Output "PASS Scaffold.$method (Starain algorithm + declared API/helper substitutions)"
}
if ($actual -match '(?i)\b(?:espTrail|espColor|espFill|espOutline|onScaffoldEsp|recordEspTrail)\b') { throw 'Unexpected ESP implementation' }
Write-Output 'PASS no Scaffold ESP implementation'
