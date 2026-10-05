package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalData_2.class */
public final class AutoCrystalData_2 implements SearchHelper_4 {
    public final PlayerEntity playerEntity;
    public final double val;

    public AutoCrystalData_2(PlayerEntity playerEntity, double d) {
        this.playerEntity = playerEntity;
        this.val = d;
    }

    public static boolean is625(PlayerEntity playerEntity, double d) {
        return playerEntity == null || playerEntity == minecraftClient.player || !playerEntity.isAlive() || BaritoneHelper_3.searchHelper4_14.is520(playerEntity) || ((AutoCrystalHelper_4) playerEntity).isServerSideDead() || playerEntity.isSpectator() || playerEntity.getAbilities().creativeMode || ((double) minecraftClient.player.distanceTo((Entity) playerEntity)) > d;
    }




    public PlayerEntity getPlayerEntity626() {
        return this.playerEntity;
    }

    public double get14() {
        return this.val;
    }
}
