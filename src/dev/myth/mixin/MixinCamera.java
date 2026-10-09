package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import dev.myth.model.render.NoRender;
import net.minecraft.client.Camera;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.Pose;
import net.minecraft.world.entity.player.Player;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;

/**
 * NoRender NewSneaking: the pre-1.14 crouch eye height. Mio replaced the whole eye height update
 * to reproduce the old easing; the 26.2 lerp already converges on the target, so only the target moves.
 */
@Mixin(Camera.class)
public abstract class MixinCamera {
    @Shadow private Entity entity;

    @ModifyExpressionValue(method = "tick", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/Entity;getEyeHeight()F"))
    private float myth$newSneaking(float original) {
        NoRender noRender = NoRender.active();
        if (noRender == null || !noRender.newSneaking.get()) return original;
        return entity instanceof Player && entity.getPose() == Pose.CROUCHING ? 1.54f : original;
    }
}
