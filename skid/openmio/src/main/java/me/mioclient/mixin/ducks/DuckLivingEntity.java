package me.mioclient.mixin.ducks;

import net.minecraft.entity.LivingEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
import org.spongepowered.asm.mixin.gen.Invoker;

/* compiled from: 0.java */
@Mixin({LivingEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckLivingEntity.class */
public interface DuckLivingEntity {
    @Accessor("itemUseTimeLeft")
    void setItemUseTimeLeft(int i);

    @Accessor("leaningPitch")
    void mio$setLeaningPitch(float f);

    @Accessor("lastLeaningPitch")
    void mio$setLastLeaningPitch(float f);

    @Accessor("leaningPitch")
    float mio$getLeaningPitch();

    @Accessor("leaningPitch")
    float mio$getLastLeaningPitch();

    @Accessor("serverX")
    double mio$getServerX();

    @Accessor("serverY")
    double mio$getServerY();

    @Accessor("serverZ")
    double mio$getServerZ();

    @Invoker("getHandSwingDuration")
    int getSwingDuration();
}
