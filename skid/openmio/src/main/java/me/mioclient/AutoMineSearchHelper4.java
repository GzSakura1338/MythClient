package me.mioclient;

import java.util.Comparator;
import me.mioclient.module.combat.AutoMine;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoMineSearchHelper4.class */
public final class AutoMineSearchHelper4 implements SearchHelper_4 {
    public final AutoMine autoMine;

    public AutoMineSearchHelper4(AutoMine autoMine) {
        this.autoMine = autoMine;
    }

    public PlayerEntity getPlayerEntity886() {
        return (PlayerEntity) minecraftClient.world.getPlayers().stream().filter(abstractClientPlayerEntity -> {
            return abstractClientPlayerEntity.isAlive() && abstractClientPlayerEntity != minecraftClient.player && minecraftClient.player.distanceTo((Entity) abstractClientPlayerEntity) <= this.autoMine.get1965() && !BaritoneHelper_3.searchHelper4_14.is520((PlayerEntity) abstractClientPlayerEntity);
        }).filter(abstractClientPlayerEntity2 -> {
            return !this.autoMine.ignoreNaked.getValue().booleanValue() || HoleSnapSearchHelper4.is2013((LivingEntity) abstractClientPlayerEntity2);
        }).min(Comparator.comparing(abstractClientPlayerEntity3 -> {
            return Float.valueOf(MathHelper.angleBetween(minecraftClient.player.getYaw(), SearchHelper4_8.getFloatArray2483((Entity) abstractClientPlayerEntity3)[0]));
        })).orElse(null);
    }

    public Box getBox2244(PlayerEntity playerEntity) {
        return playerEntity.getBoundingBox().expand(-SearchHelper.val, 0.0d, -SearchHelper.val).withMaxY(playerEntity.getY());
    }
}
