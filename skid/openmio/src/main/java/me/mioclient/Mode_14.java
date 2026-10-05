package me.mioclient;

import java.util.function.Function;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_14.class */
public enum Mode_14 implements SearchHelper_4 {
    DISTANCE(livingEntity -> {
        return Double.valueOf(livingEntity.squaredDistanceTo(SearchHelper_4.minecraftClient.player));
    }),
    HEALTH(livingEntity2 -> {
        return Double.valueOf(SearchHelper_3.get644((Entity) livingEntity2));
    });

    public final Function<LivingEntity, Double> function;

    Mode_14(Function<LivingEntity, Double> function) {
        this.function = function;
    }

    public Function<LivingEntity, Double> getFunction2892() {
        return this.function;
    }
}
