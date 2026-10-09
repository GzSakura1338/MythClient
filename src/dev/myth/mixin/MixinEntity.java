package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.myth.Myth;
import dev.myth.event.impl.RayTraceEvent;
import dev.myth.event.impl.StrafeEvent;
import dev.myth.event.impl.StuckInBlockEvent;
import dev.myth.model.render.NoRender;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArg;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Entity.class)
public abstract class MixinEntity {
    @Shadow private Vec3 stuckSpeedMultiplier;

    @WrapOperation(method = "getViewVector", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/Entity;calculateViewVector(FF)Lnet/minecraft/world/phys/Vec3;"))
    private Vec3 myth$rayTrace(Entity entity, float pitch, float yaw, Operation<Vec3> original) {
        if (entity == Minecraft.getInstance().player) {
            RayTraceEvent event = Myth.EVENT_MANAGER.call(new RayTraceEvent(entity, yaw, pitch));
            return original.call(entity, event.getPitch(), event.getYaw());
        }
        return original.call(entity, pitch, yaw);
    }

    @Inject(method = "makeStuckInBlock", at = @At("RETURN"))
    private void myth$stuckInBlock(BlockState state, Vec3 multiplier, CallbackInfo ci) {
        if ((Object) this == Minecraft.getInstance().player) {
            StuckInBlockEvent event = Myth.EVENT_MANAGER.call(new StuckInBlockEvent(state, multiplier));
            stuckSpeedMultiplier = event.isCancelled() ? Vec3.ZERO : event.getStuckSpeedMultiplier();
        }
    }

    @ModifyArg(method = "moveRelative", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/Entity;getInputVector(Lnet/minecraft/world/phys/Vec3;FF)Lnet/minecraft/world/phys/Vec3;"), index = 2)
    private float myth$strafe(float yaw) {
        return (Object) this == Minecraft.getInstance().player
                ? Myth.EVENT_MANAGER.call(new StrafeEvent(yaw)).getYaw() : yaw;
    }

    /** NoRender Interpolation, cancelled for every entity exactly as Mio does it. */
    @Inject(method = "lerpPositionAndRotationStep", at = @At("HEAD"), cancellable = true)
    private void myth$interpolation(int stepsToTarget, double targetX, double targetY, double targetZ,
                                    double targetYRot, double targetXRot, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.interpolation.get()) ci.cancel();
    }
}
