package me.mioclient;

import java.awt.Color;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.Objects;
import me.mioclient.module.exploit.NewChunks;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.ChunkPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NewChunksHelperSearchHelper4.class */
public final class NewChunksHelperSearchHelper4 implements SearchHelper_4 {
    public final ChunkPos chunkPos;
    public final StashFinderMode stashFinderMode;
    public final NewChunksHelperMode newChunksHelperMode;

    public NewChunksHelperSearchHelper4(ChunkPos chunkPos, StashFinderMode stashFinderMode, NewChunksHelperMode newChunksHelperMode) {
        this.chunkPos = chunkPos;
        this.stashFinderMode = stashFinderMode;
        this.newChunksHelperMode = newChunksHelperMode;
    }

    public static NewChunksHelperSearchHelper4 getNewChunksHelperSearchHelper42464(ChunkPos chunkPos, NewChunksHelperMode newChunksHelperMode) {
        return new NewChunksHelperSearchHelper4(chunkPos, SearchHelper4_7.getStashFinderMode2438(), newChunksHelperMode);
    }

    public boolean is2465(NewChunksHelperMode newChunksHelperMode) {
        return newChunksHelperMode == this.newChunksHelperMode;
    }

    public void do2466(MatrixStackEvent matrixStackEvent, NewChunks newChunks) {
        Color[] colorArray671 = this.newChunksHelperMode.getColorArray671(newChunks);
        if (colorArray671 == null) {
            return;
        }
        BlockPos blockPos = minecraftClient.gameRenderer.getCamera().getBlockPos();
        int bottomY = minecraftClient.world.getBottomY() + newChunks.height.getValue().intValue();
        if (blockPos.isWithinDistance(this.chunkPos.getCenterAtY(blockPos.getY()), 16 * newChunks.distance.getValue().intValue())) {
            Box box = new Box(this.chunkPos.getStartX(), bottomY, this.chunkPos.getStartZ(), this.chunkPos.getStartX() + 16, bottomY, this.chunkPos.getStartZ() + 16);
            if (SearchHelper4_8.is2492(box)) {
                PhaseESPSearchHelper4.do1590(matrixStackEvent.getMatrixStack472(), box, colorArray671[0]);
                PhaseESPSearchHelper4.do1593(matrixStackEvent.getMatrixStack472(), box, colorArray671[1], newChunks.lineWidth.getValue().floatValue());
            }
        }
    }

    @Override // java.lang.Record
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || getClass() != obj.getClass()) {
            return false;
        }
        NewChunksHelperSearchHelper4 newChunksHelperSearchHelper4 = (NewChunksHelperSearchHelper4) obj;
        return Objects.equals(this.chunkPos, newChunksHelperSearchHelper4.chunkPos) && this.newChunksHelperMode == newChunksHelperSearchHelper4.newChunksHelperMode && this.stashFinderMode == newChunksHelperSearchHelper4.stashFinderMode;
    }

    @Override // java.lang.Record
    public int hashCode() {
        return Objects.hash(this.chunkPos, this.stashFinderMode, this.newChunksHelperMode);
    }


    public ChunkPos getChunkPos2467() {
        return this.chunkPos;
    }

    public StashFinderMode getStashFinderMode800() {
        return this.stashFinderMode;
    }

    public NewChunksHelperMode getNewChunksHelperMode2468() {
        return this.newChunksHelperMode;
    }
}
