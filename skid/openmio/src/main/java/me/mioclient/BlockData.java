package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.block.Block;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BlockData.class */
public final class BlockData {
    public final long num;
    public final Block block;

    public BlockData(long j, Block block) {
        this.num = j;
        this.block = block;
    }

    public static BlockData getBlockData2589(Block block) {
        return new BlockData(System.currentTimeMillis(), block);
    }




    public long get798() {
        return this.num;
    }

    public Block getBlock2590() {
        return this.block;
    }
}
