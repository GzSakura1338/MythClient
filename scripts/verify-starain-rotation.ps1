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

# These are the complete allowed changes, apart from the package/import prefix.
# Any other changed statement, comment, annotation or line makes this check fail.
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

$lineCount = 0
foreach ($relativePath in $sourceFiles) {
    $reference = [System.IO.File]::ReadAllText((Join-Path $referenceRoot $relativePath)).Replace("`r`n", "`n")
    $actual = [System.IO.File]::ReadAllText((Join-Path $targetRoot $relativePath)).Replace("`r`n", "`n")
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
Write-Output "PASS all $($sourceFiles.Length) files, $lineCount original lines; only the declared namespace and 26.2 API adaptations differ."
