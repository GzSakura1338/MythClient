package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AnchorAuraData.class */
public final class AnchorAuraData {
    public final BlockPos blockPos;
    public final Direction direction;
    public final AutoCrystalData_2 autoCrystalData_2;

    public AnchorAuraData(BlockPos blockPos, Direction direction, AutoCrystalData_2 autoCrystalData_2) {
        this.blockPos = blockPos;
        this.direction = direction;
        this.autoCrystalData_2 = autoCrystalData_2;
    }

    public AnchorAuraData getAnchorAuraData2056(AnchorAuraData anchorAuraData) {
        if (anchorAuraData != null && anchorAuraData.autoCrystalData_2.get14() > this.autoCrystalData_2.get14()) {
            return anchorAuraData;
        }
        return this;
    }




    public BlockPos getBlockPos12() {
        return this.blockPos;
    }

    public Direction getDirection1462() {
        return this.direction;
    }

    public AutoCrystalData_2 getAutoCrystalData_22057() {
        return this.autoCrystalData_2;
    }
}
