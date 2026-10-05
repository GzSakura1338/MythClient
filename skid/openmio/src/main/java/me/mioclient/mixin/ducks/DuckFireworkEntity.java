package me.mioclient.mixin.ducks;

import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.projectile.FireworkRocketEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({FireworkRocketEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckFireworkEntity.class */
public interface DuckFireworkEntity {
    @Accessor("life")
    void setLife(int i);

    @Accessor("life")
    int getLife();

    @Accessor("lifeTime")
    int getLifeTime();

    @Accessor("shooter")
    LivingEntity mio$getShooter();
}
