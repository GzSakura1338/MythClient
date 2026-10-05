package me.mioclient;

import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BreakingProgressHelper.class */
public interface BreakingProgressHelper {
    boolean isBreakingBlock();

    void setBreakingBlock(boolean z);

    float getBreakingProgress();

    void setBreakingProgress(float f);

    BlockPos getCurrentBreakingBlock();

    void setCurrentBreakingBlock(BlockPos blockPos);
}
