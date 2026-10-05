package me.mioclient;

import java.util.function.Consumer;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoMineHelper.class */
public class AutoMineHelper {
    public BlockPos blockPos;
    public BlockPos blockPos2;
    public boolean flag;
    public int num;

    public void reset() {
        this.blockPos = null;
        this.flag = false;
    }

    public void do2896() {
        this.flag = true;
    }

    public boolean is2897() {
        return this.flag;
    }

    public BlockPos getBlockPos386() {
        return this.blockPos;
    }

    public void do667(BlockPos blockPos) {
        this.blockPos = blockPos;
    }

    public BlockPos getBlockPos2898() {
        return this.blockPos2;
    }

    public void do2899(BlockPos blockPos) {
        this.blockPos2 = blockPos;
    }

    public int get888() {
        return this.num;
    }

    public void do2900(int i) {
        this.num = i;
    }

    public void do2901(int i, Consumer<AutoMineHelper> consumer) {
        if (this.num <= i || getBlockPos386() == null || is2897()) {
            consumer.accept(this);
            this.num = i;
        }
    }
}
