package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.block.BlockState;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BlockStateSearchHelper4.class */
public final class BlockStateSearchHelper4 implements SearchHelper_4 {
    public final BlockState blockState;
    public final BlockPos blockPos;
    public final long num;

    public BlockStateSearchHelper4(BlockState blockState, BlockPos blockPos, long j) {
        this.blockState = blockState;
        this.blockPos = blockPos;
        this.num = j;
    }

    public static BlockStateSearchHelper4 getBlockStateSearchHelper42779(BlockPos blockPos) {
        return new BlockStateSearchHelper4(minecraftClient.world.getBlockState(blockPos), blockPos, System.currentTimeMillis());
    }

    public boolean is1775() {
        return System.currentTimeMillis() - this.num >= 500;
    }




    public BlockState getBlockState2780() {
        return this.blockState;
    }

    public BlockPos getBlockPos12() {
        return this.blockPos;
    }

    public long get2781() {
        return this.num;
    }
}
