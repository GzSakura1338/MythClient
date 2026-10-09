package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.ModifyReturnValue;
import dev.myth.Myth;
import dev.myth.event.impl.FallFlyingEvent;
import dev.myth.event.impl.JumpEvent;
import dev.myth.event.impl.RotationAnimationEvent;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.LivingEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

@Mixin(LivingEntity.class)
public abstract class MixinLivingEntity {
    /**
     * Starain's Animations reflects on a LivingEntity.swingDuration field to scale the swing. 26.2
     * has no such field - the duration is computed here from the item's swing animation - so the
     * setting is applied to this return instead, for the local player only.
     */
    @ModifyReturnValue(method = "getCurrentSwingDuration", at = @At("RETURN"))
    private int myth$swingSpeed(int duration) {
        if ((Object) this != Minecraft.getInstance().player) return duration;
        float speed = dev.myth.model.render.Animations.swingSpeedClamped();
        return Math.abs(speed - 1) < 1e-3f ? duration
                : net.minecraft.util.Mth.clamp(Math.round(duration / speed), 1, 100);
    }

    @ModifyExpressionValue(method = "jumpFromGround", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/LivingEntity;getYRot()F"))
    private float myth$jumpYaw(float yaw) {
        return (Object) this == Minecraft.getInstance().player
                ? Myth.EVENT_MANAGER.call(new JumpEvent(yaw)).getYaw() : yaw;
    }

    @ModifyExpressionValue(method = "updateFallFlyingMovement", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/LivingEntity;getXRot()F"))
    private float myth$fallFlyingPitch(float pitch) {
        return (Object) this == Minecraft.getInstance().player
                ? Myth.EVENT_MANAGER.call(new FallFlyingEvent(pitch)).getPitch() : pitch;
    }

    @ModifyExpressionValue(method = "tickHeadTurn", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/LivingEntity;getYRot()F"))
    private float myth$headYaw(float yaw) {
        return (Object) this == Minecraft.getInstance().player
                ? Myth.EVENT_MANAGER.call(new RotationAnimationEvent(yaw, 0, 0, 0)).getYaw() : yaw;
    }
}
