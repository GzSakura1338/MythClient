package me.mioclient.mixin.ducks;

import net.minecraft.entity.Entity;
import net.minecraft.util.math.BlockPos;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

/* compiled from: 0.java */
@Mixin({Entity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckEntity.class */
public interface DuckEntity {
    @Invoker("getVelocityAffectingPos")
    BlockPos mio$getVelocityAffectingPos();
}
