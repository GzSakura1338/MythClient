package me.mioclient;

import me.mioclient.feature.Stopwatch;
import me.mioclient.module.combat.AutoMine;
import me.mioclient.module.player.SpeedMine;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpeedMineHelper.class */
public final class SpeedMineHelper implements SearchHelper_4 {
    public static SpeedMine speedmine = (SpeedMine) BaritoneHelper_3.baritoneHelper_4.getModule117(SpeedMine.class);
    public final Stopwatch stopwatch = new Stopwatch();
    public final AutoMine autoMine;
    public BlockPos blockPos;

    public SpeedMineHelper(AutoMine autoMine) {
        this.autoMine = autoMine;
    }

    public boolean is1225() {
        BlockPos blockPos1226 = getBlockPos1226();
        return ((blockPos1226 == null || minecraftClient.world.isAir(blockPos1226)) && speedmine.getSpeedMineSearchHelper41059() == null) ? false : true;
    }

    public BlockPos getBlockPos386() {
        SpeedMineSearchHelper4 speedMineSearchHelper41059 = speedmine.getSpeedMineSearchHelper41059();
        if (speedMineSearchHelper41059 == null) {
            return null;
        }
        return speedMineSearchHelper41059.getBlockPos386();
    }

    public BlockPos getBlockPos1226() {
        if (this.stopwatch.is419(200L)) {
            return null;
        }
        return this.blockPos;
    }

    public void do1227(BlockPos blockPos) {
        this.blockPos = blockPos;
        this.stopwatch.reset();
    }

    public boolean is1228(long j) {
        return this.stopwatch.is419(j);
    }
}
