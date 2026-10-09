package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.renderer.fog.environment.MobEffectFogEnvironment;
import net.minecraft.core.Holder;
import net.minecraft.world.effect.MobEffect;
import net.minecraft.world.effect.MobEffects;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.level.material.FogType;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/** Mio cancels the effect lookup itself; 26.2 keeps blindness and darkness in their own fog. */
@Mixin(MobEffectFogEnvironment.class)
public abstract class MixinMobEffectFogEnvironment {
    @Shadow public abstract Holder<MobEffect> getMobEffect();

    @Inject(method = "isApplicable", at = @At("HEAD"), cancellable = true)
    private void myth$effectFog(FogType fogType, Entity entity, CallbackInfoReturnable<Boolean> cir) {
        NoRender noRender = NoRender.active();
        if (noRender == null) return;
        Holder<MobEffect> effect = getMobEffect();
        if (effect.equals(MobEffects.BLINDNESS) ? noRender.blindness.get()
                : effect.equals(MobEffects.DARKNESS) && noRender.darkness.get()) cir.setReturnValue(false);
    }
}
