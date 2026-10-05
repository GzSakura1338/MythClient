package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.entity.player.PlayerEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalData_4.class */
public final class AutoCrystalData_4 {
    public final PlayerEntity playerEntity;
    public final double val;
    public final AutoCrystalDataMode autoCrystalDataMode;
    public final AutoCrystalMode_7 autoCrystalMode_7;

    public AutoCrystalData_4(PlayerEntity playerEntity, double d, AutoCrystalDataMode autoCrystalDataMode, AutoCrystalMode_7 autoCrystalMode_7) {
        this.playerEntity = playerEntity;
        this.val = d;
        this.autoCrystalDataMode = autoCrystalDataMode;
        this.autoCrystalMode_7 = autoCrystalMode_7;
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

    public AutoCrystalMode_7 getAutoCrystalMode_72504() {
        return this.autoCrystalMode_7;
    }
}
