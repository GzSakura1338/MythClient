package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
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
