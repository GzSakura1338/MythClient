package me.mioclient;

import java.util.Collections;
import java.util.Iterator;
import net.minecraft.block.BlockState;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HoleSnapHelper.class */
public interface HoleSnapHelper {
    public static final HoleSnapHelper holeSnapHelper = (blockPos, blockState) -> {
        return ReachMode.NONE;
    };
    public static final HoleSnapHelper holeSnapHelper2 = (blockPos, blockState) -> {
        return blockState.getBlock().getBlastResistance() < 600.0f ? ReachMode.IGNORE : ReachMode.NONE;
    };

    ReachMode test(BlockPos blockPos, BlockState blockState);

    static HoleSnapHelper getHoleSnapHelper1674(BlockPos blockPos) {
        return getHoleSnapHelper1675(Collections.singleton(blockPos));
    }

    static HoleSnapHelper getHoleSnapHelper1675(Iterable<BlockPos> iterable) {
        return (blockPos, blockState) -> {
            Iterator it = iterable.iterator();
            while (it.hasNext()) {
                if (blockPos.equals((BlockPos) it.next())) {
                    return ReachMode.IGNORE;
                }
            }
            return ReachMode.NONE;
        };
    }

    static HoleSnapHelper getHoleSnapHelper1676(BlockPos blockPos) {
        return getHoleSnapHelper1677(Collections.singleton(blockPos));
    }

    static HoleSnapHelper getHoleSnapHelper1677(Iterable<BlockPos> iterable) {
        return (blockPos, blockState) -> {
            Iterator it = iterable.iterator();
            while (it.hasNext()) {
                if (blockPos.equals((BlockPos) it.next())) {
                    return ReachMode.SOLID;
                }
            }
            return ReachMode.NONE;
        };
    }

    static HoleSnapHelper getHoleSnapHelper1678(HoleSnapHelper... holeSnapHelperArr) {
        return (blockPos, blockState) -> {
            for (HoleSnapHelper holeSnapHelper3 : holeSnapHelperArr) {
                ReachMode test = holeSnapHelper3.test(blockPos, blockState);
                if (test != ReachMode.NONE) {
                    return test;
                }
            }
            return ReachMode.NONE;
        };
    }
}
