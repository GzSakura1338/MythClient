package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalData.class */
public final class AutoCrystalData {
    public final BlockPos blockPos;
    public final PlayerEntity playerEntity;
    public final double val;
    public final AutoCrystalDataMode autoCrystalDataMode;

    public AutoCrystalData(BlockPos blockPos, PlayerEntity playerEntity, double d, AutoCrystalDataMode autoCrystalDataMode) {
        this.blockPos = blockPos;
        this.playerEntity = playerEntity;
        this.val = d;
        this.autoCrystalDataMode = autoCrystalDataMode;
    }

    public AutoCrystalData getAutoCrystalData11(AutoCrystalData autoCrystalData) {
        if (autoCrystalData != null && this.autoCrystalDataMode.ordinal() >= autoCrystalData.autoCrystalDataMode.ordinal()) {
            if (this.autoCrystalDataMode.ordinal() <= autoCrystalData.autoCrystalDataMode.ordinal() && autoCrystalData.val <= this.val) {
                return this;
            }
            return autoCrystalData;
        }
        return this;
    }




    public BlockPos getBlockPos12() {
        return this.blockPos;
    }

    public PlayerEntity getPlayerEntity13() {
        return this.playerEntity;
    }

    public double get14() {
        return this.val;
    }

    public AutoCrystalDataMode getAutoCrystalDataMode15() {
        return this.autoCrystalDataMode;
    }
}
