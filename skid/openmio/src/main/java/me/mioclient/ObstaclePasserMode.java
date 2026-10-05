package me.mioclient;

import java.util.function.Function;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ObstaclePasserMode.class */
public enum ObstaclePasserMode implements SearchHelper_4 {
    XP(num -> {
        return BlockPos.ofFloored(minecraftClient.player.getX() + num.intValue(), 0.0d, 0.0d);
    }),
    XN(num2 -> {
        return BlockPos.ofFloored(minecraftClient.player.getX() - num2.intValue(), 0.0d, 0.0d);
    }),
    ZP(num3 -> {
        return BlockPos.ofFloored(0.0d, 0.0d, minecraftClient.player.getZ() + num3.intValue());
    }),
    ZN(num4 -> {
        return BlockPos.ofFloored(0.0d, 0.0d, minecraftClient.player.getZ() - num4.intValue());
    }),
    XP_ZP(num5 -> {
        return BlockPos.ofFloored(get476() + num5.intValue(), 0.0d, get477() + num5.intValue());
    }),
    XN_ZP(num6 -> {
        return BlockPos.ofFloored(get476() - num6.intValue(), 0.0d, get477() + num6.intValue());
    }),
    XP_ZN(num7 -> {
        return BlockPos.ofFloored(get476() + num7.intValue(), 0.0d, get477() - num7.intValue());
    }),
    XN_ZN(num8 -> {
        return BlockPos.ofFloored(get476() - num8.intValue(), 0.0d, get477() - num8.intValue());
    });

    public final Function<Integer, BlockPos> function;

    ObstaclePasserMode(Function<Integer, BlockPos> function) {
        this.function = function;
    }

    public static int get475() {
        return (int) Math.max(Math.abs(minecraftClient.player.getX()), Math.abs(minecraftClient.player.getZ()));
    }

    public static int get476() {
        return (int) (get475() * Math.signum(minecraftClient.player.getX()));
    }

    public static int get477() {
        return (int) (get475() * Math.signum(minecraftClient.player.getZ()));
    }

    public static ObstaclePasserMode getObstaclePasserMode478(float f) {
        switch (SearchHelper4_8.get2488(f)) {
            case 0:
                return ZP;
            case 1:
                return XN_ZP;
            case 2:
                return XN;
            case 3:
                return XN_ZN;
            case 4:
                return ZN;
            case 5:
                return XP_ZN;
            case 6:
                return XP;
            case 7:
                return XP_ZP;
            default:
                return null;
        }
    }
}
