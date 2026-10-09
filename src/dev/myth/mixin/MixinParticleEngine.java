package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.particle.ParticleEngine;
import net.minecraft.core.particles.ParticleOptions;
import net.minecraft.world.entity.Entity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** The explosion emitter never goes through ClientLevel, so it needs its own filter. */
@Mixin(ParticleEngine.class)
public abstract class MixinParticleEngine {
    @Inject(method = "createTrackingEmitter(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/core/particles/ParticleOptions;)V",
            at = @At("HEAD"), cancellable = true)
    private void myth$emitter(Entity entity, ParticleOptions particle, CallbackInfo ci) {
        if (NoRender.hidesParticleType(particle)) ci.cancel();
    }

    @Inject(method = "createTrackingEmitter(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/core/particles/ParticleOptions;I)V",
            at = @At("HEAD"), cancellable = true)
    private void myth$emitterWithLifetime(Entity entity, ParticleOptions particle, int lifeTime, CallbackInfo ci) {
        if (NoRender.hidesParticleType(particle)) ci.cancel();
    }
}
