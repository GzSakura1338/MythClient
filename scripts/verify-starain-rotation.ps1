param([string] $Workspace = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$referenceRoot = Join-Path $Workspace 'skid/Starain-1.20.1-main/src/main/java/today/starain/utils'
$targetRoot = Join-Path $Workspace 'src/dev/myth/utils'
$sourceFiles = @(
    'rotation/Rotation.java',
    'rotation/RotationManager.java',
    'rotation/RotationUtils.java',
    'rotation/RayCastUtil.java',
    'player/MoveUtils.java',
    'math/MathConst.java',
    'vector/Vector2f.java'
)

# Exact Starain parity remains enforced for the five unchanged utility classes.
# The two Nova continuity revisions below are pinned separately; see docs/nova-yaw-continuity.md.
$allowedChanges = @{
    'rotation/RotationManager.java' = @(
        @('import net.minecraft.network.protocol.game.ServerboundMovePlayerPacket.PosRot;',
          "import net.minecraft.network.protocol.game.ServerboundUseItemPacket;`nimport dev.myth.access.MinecraftAccess;"),
        @('mc.gameRenderer.pick(1.0F);', '((MinecraftAccess) mc).myth$pick(1.0F);'),
        @('PosRot packet = (PosRot) e.getPacket();', 'ServerboundUseItemPacket packet = (ServerboundUseItemPacket) e.getPacket();'),
        @('PosRot newPacket = new PosRot(packet.getX(0.0), packet.getY(0.0), packet.getZ(0.0), rotations.getYaw(), pitch, packet.isOnGround());',
          'ServerboundUseItemPacket newPacket = new ServerboundUseItemPacket(packet.getHand(), packet.getSequence(), rotations.getYaw(), pitch);')
    )
    'rotation/RotationUtils.java' = @(
        @('enumFacing.getNormal().getX()', 'enumFacing.getStepX()'),
        @('enumFacing.getNormal().getY()', 'enumFacing.getStepY()'),
        @('enumFacing.getNormal().getZ()', 'enumFacing.getStepZ()')
    )
    'rotation/RayCastUtil.java' = @(
        ,@('mc.getFrameTime()', 'mc.getDeltaTracker().getGameTimeDeltaPartialTick(false)')
    )
    'player/MoveUtils.java' = @(
        @('import net.minecraft.client.Minecraft;', "import net.minecraft.client.Minecraft;`nimport dev.myth.access.MovementInputAccess;"),
        @('mc.player.input.forwardImpulse', 'MovementInputAccess.forwardImpulse(mc.player.input)'),
        @('mc.player.input.leftImpulse', 'MovementInputAccess.leftImpulse(mc.player.input)')
    )
}

$novaContinuityRevisions = @{
    'rotation/RotationManager.java' = @{
        Reference = 'D76EA170584AB7F74918D817B5524D642E15FFE076CED7F2834349285F5DC84E'
        Revision = '789838CFFE4B284B4EDF356CF3AB011934DD4C8768B889ED5D099A4AD9BE80C0'
    }
    'rotation/RotationUtils.java' = @{
        Reference = '353DCDA7193110D8B6468ECDB855469EEAA767F4773B76ED0255A247664F0815'
        Revision = '643526AC935BA5CB690B6458E42B19A3444B6891468C7B9E9A4F5FF98217E770'
    }
}

function Get-TextDigest([string] $Text) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Text))).Replace('-', '') }
    finally { $sha.Dispose() }
}

$lineCount = 0
foreach ($relativePath in $sourceFiles) {
    $reference = [System.IO.File]::ReadAllText((Join-Path $referenceRoot $relativePath)).Replace("`r`n", "`n")
    $actual = [System.IO.File]::ReadAllText((Join-Path $targetRoot $relativePath)).Replace("`r`n", "`n")
    if ($novaContinuityRevisions.ContainsKey($relativePath)) {
        $revision = $novaContinuityRevisions[$relativePath]
        if ((Get-TextDigest $reference) -cne $revision.Reference) { throw "Starain reference changed: $relativePath" }
        if ((Get-TextDigest $actual) -cne $revision.Revision) { throw "Unreviewed change to Nova continuity revision: $relativePath" }
        Write-Output "PASS $relativePath (reviewed Nova continuity revision; no longer exact Starain parity)"
        continue
    }
    $expected = $reference.Replace('today.starain.', 'dev.myth.')
    foreach ($replacement in $allowedChanges[$relativePath]) {
        if (!$expected.Contains($replacement[0])) { throw "Reference changed; adaptation no longer matches: $relativePath / $($replacement[0])" }
        $expected = $expected.Replace($replacement[0], $replacement[1])
    }
    if ($expected -cne $actual) {
        $expectedLines = $expected.Split("`n")
        $actualLines = $actual.Split("`n")
        for ($i = 0; $i -lt [Math]::Max($expectedLines.Length, $actualLines.Length); $i++) {
            if ($expectedLines[$i] -cne $actualLines[$i]) {
                throw "Unlisted difference at ${relativePath}:$($i + 1). Expected: $($expectedLines[$i]) Actual: $($actualLines[$i])"
            }
        }
        throw "Unexpected text difference: $relativePath"
    }
    $fileLines = ($reference.TrimEnd("`n").Split("`n")).Length
    $lineCount += $fileLines
    Write-Output "PASS $relativePath ($fileLines original lines)"
}
Write-Output "PASS 5 Starain utilities ($lineCount original lines) plus 2 pinned Nova continuity revisions."
