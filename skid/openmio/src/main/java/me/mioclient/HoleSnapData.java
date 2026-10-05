package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HoleSnapData.class */
public final class HoleSnapData {
    public final HoleSnapDataMode holeSnapDataMode;
    public final BlockPos blockPos;
    public final Box box;
    public final boolean flag;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/HoleSnapData$HoleSnapDataMode.class */
    public enum HoleSnapDataMode {
        SAFE,
        UNSAFE
    }

    public HoleSnapData(HoleSnapDataMode holeSnapDataMode, BlockPos blockPos, Box box, boolean z) {
        this.holeSnapDataMode = holeSnapDataMode;
        this.blockPos = blockPos;
        this.box = box;
        this.flag = z;
    }




    public HoleSnapDataMode getHoleSnapDataMode2170() {
        return this.holeSnapDataMode;
    }

    public BlockPos getBlockPos12() {
        return this.blockPos;
    }

    public Box getBox799() {
        return this.box;
    }

    public boolean is2171() {
        return this.flag;
    }
}
