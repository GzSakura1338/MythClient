package me.mioclient.mixin;

import me.mioclient.Helper_3;
import me.mioclient.feature.Stopwatch;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.mob.MobEntity;
import net.minecraft.entity.mob.ZombifiedPiglinEntity;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({ZombifiedPiglinEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinZombifiedPiglinEntity.class */
public abstract class MixinZombifiedPiglinEntity extends MobEntity implements Helper_3 {

    @Unique
    private Stopwatch stopwatch;

    protected MixinZombifiedPiglinEntity(EntityType<? extends MobEntity> entityType, World world) {
        super(entityType, world);
    }

    @Inject(method = {"<init>"}, at = {@At("TAIL")})
    private void initHook(EntityType<?> entityType, World world, CallbackInfo callbackInfo) {
        this.stopwatch = new Stopwatch();
    }

    @Override // me.mioclient.Helper_3
    public boolean mio$isAttacking() {
        if (isAttacking()) {
            this.stopwatch.reset();
        }
        return !this.stopwatch.is419(1000L);
    }
}
