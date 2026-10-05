package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Direction;
import org.jetbrains.annotations.Nullable;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AnvilAuraData.class */
public final class AnvilAuraData {
    public final PlayerEntity playerEntity;
    public final AnvilAuraMode anvilAuraMode;

    @Nullable
    public final Direction direction;

    public AnvilAuraData(PlayerEntity playerEntity, AnvilAuraMode anvilAuraMode, @Nullable Direction direction) {
        this.playerEntity = playerEntity;
        this.anvilAuraMode = anvilAuraMode;
        this.direction = direction;
    }




    public PlayerEntity getPlayerEntity2247() {
        return this.playerEntity;
    }

    public AnvilAuraMode getAnvilAuraMode2248() {
        return this.anvilAuraMode;
    }

    @Nullable
    public Direction getDirection842() {
        return this.direction;
    }
}
