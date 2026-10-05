package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.myth.Myth;
import dev.myth.event.impl.AttackSlowdownEvent;
import dev.myth.event.impl.AttackYawEvent;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

@Mixin(Player.class)
public abstract class MixinPlayer {
    @ModifyExpressionValue(method = {"causeExtraKnockback", "doSweepAttack"},
            at = @At(value = "INVOKE", target = "Lnet/minecraft/world/entity/player/Player;getYRot()F"))
    private float myth$attackYaw(float yaw) {
        return (Object) this == Minecraft.getInstance().player
                ? Myth.EVENT_MANAGER.call(new AttackYawEvent(yaw)).getYaw() : yaw;
    }

    @WrapOperation(method = "causeExtraKnockback", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/player/Player;setDeltaMovement(Lnet/minecraft/world/phys/Vec3;)V"))
    private void myth$attackMovement(Player player, Vec3 movement, Operation<Void> original) {
        if (player != Minecraft.getInstance().player
                || !Myth.EVENT_MANAGER.call(new AttackSlowdownEvent()).isCancelled()) {
            original.call(player, movement);
        }
    }

    @WrapOperation(method = "causeExtraKnockback", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/player/Player;setSprinting(Z)V"))
    private void myth$attackSprint(Player player, boolean sprinting, Operation<Void> original) {
        if (player != Minecraft.getInstance().player
                || !Myth.EVENT_MANAGER.call(new AttackSlowdownEvent()).isCancelled()) {
            original.call(player, sprinting);
        }
    }
}
