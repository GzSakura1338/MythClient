package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.wrapmethod.WrapMethod;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.myth.Myth;
import dev.myth.event.impl.RotationAnimationEvent;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.entity.LivingEntityRenderer;
import net.minecraft.client.renderer.entity.state.LivingEntityRenderState;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

@Mixin(LivingEntityRenderer.class)
public abstract class MixinLivingEntityRenderer {
    @WrapMethod(method = "extractRenderState(Lnet/minecraft/world/entity/LivingEntity;Lnet/minecraft/client/renderer/entity/state/LivingEntityRenderState;F)V")
    private void myth$rotationContext(LivingEntity entity, LivingEntityRenderState state,
                                      float partialTick, Operation<Void> original) {
        Entity previous = RotationAnimationEvent.currentEntity;
        RotationAnimationEvent.currentEntity = entity;
        try {
            original.call(entity, state, partialTick);
        } finally {
            RotationAnimationEvent.currentEntity = previous;
        }
    }

    @WrapOperation(method = "extractRenderState(Lnet/minecraft/world/entity/LivingEntity;Lnet/minecraft/client/renderer/entity/state/LivingEntityRenderState;F)V",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/util/Mth;rotLerp(FFF)F", ordinal = 0))
    private float myth$renderYaw(float delta, float start, float end, Operation<Float> original) {
        if (RotationAnimationEvent.currentEntity == Minecraft.getInstance().player) {
            RotationAnimationEvent event = Myth.EVENT_MANAGER.call(new RotationAnimationEvent(end, start, 0, 0));
            return original.call(delta, event.getLastYaw(), event.getYaw());
        }
        return original.call(delta, start, end);
    }

    @WrapOperation(method = "extractRenderState(Lnet/minecraft/world/entity/LivingEntity;Lnet/minecraft/client/renderer/entity/state/LivingEntityRenderState;F)V",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/world/entity/LivingEntity;getXRot(F)F"))
    private float myth$renderPitch(LivingEntity entity, float delta, Operation<Float> original) {
        if (entity == Minecraft.getInstance().player) {
            RotationAnimationEvent event = Myth.EVENT_MANAGER.call(
                    new RotationAnimationEvent(0, 0, entity.getXRot(), entity.xRotO));
            return Mth.lerp(delta, event.getLastPitch(), event.getPitch());
        }
        return original.call(entity, delta);
    }
}
