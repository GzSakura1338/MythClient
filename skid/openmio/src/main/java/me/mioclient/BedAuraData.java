package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BedAuraData.class */
public final class BedAuraData {
    public final BlockPos blockPos;
    public final BlockPos blockPos2;
    public final float val;
    public final float val2;
    public final BlockHitResult blockHitResult;

    public BedAuraData(BlockPos blockPos, BlockPos blockPos2, float f, float f2, BlockHitResult blockHitResult) {
        this.blockPos = blockPos;
        this.blockPos2 = blockPos2;
        this.val = f;
        this.val2 = f2;
        this.blockHitResult = blockHitResult;
    }




    public BlockPos getBlockPos153() {
        return this.blockPos;
    }

    public BlockPos getBlockPos154() {
        return this.blockPos2;
    }

    public float get155() {
        return this.val;
    }

    public float get156() {
        return this.val2;
    }

    public BlockHitResult getBlockHitResult157() {
        return this.blockHitResult;
    }
}
