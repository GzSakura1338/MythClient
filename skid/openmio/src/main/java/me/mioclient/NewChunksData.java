package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.ChunkPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NewChunksData.class */
public final class NewChunksData {
    public final int num;
    public final int num2;
    public static final int num3 = 9;

    public NewChunksData(int i, int i2) {
        this.num = i;
        this.num2 = i2;
    }

    public static NewChunksData getNewChunksData2274(ChunkPos chunkPos) {
        return new NewChunksData(chunkPos.getStartX() >> 9, chunkPos.getStartZ() >> 9);
    }

    public static NewChunksData getNewChunksData2275(BlockPos blockPos) {
        return new NewChunksData(blockPos.getX() >> 9, blockPos.getZ() >> 9);
    }

    public NewChunksData getNewChunksData2276(int i, int i2) {
        return new NewChunksData(get2277() + i, get2278() + i2);
    }

    @Override // java.lang.Record
    public int hashCode() {
        return (int) ((31 * ((31 * 17) + this.num)) + this.num2);
    }



    public int get2277() {
        return this.num;
    }

    public int get2278() {
        return this.num2;
    }
}
