package dev.myth.mixin;

import dev.myth.Myth;
import dev.myth.event.impl.MoveInputEvent;
import dev.myth.access.MovementInputAccess;
import net.minecraft.client.Minecraft;
import net.minecraft.client.player.ClientInput;
import net.minecraft.client.player.KeyboardInput;
import net.minecraft.world.entity.ai.attributes.Attributes;
import net.minecraft.world.entity.player.Input;
import net.minecraft.world.phys.Vec2;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(KeyboardInput.class)
public abstract class MixinKeyboardInput extends ClientInput implements MovementInputAccess {
    @Unique private double myth$sneakMultiplier = 0.3;
    @Unique private float myth$forwardImpulse;
    @Unique private float myth$leftImpulse;

    @Inject(method = "tick", at = @At("RETURN"))
    private void myth$moveInput(CallbackInfo ci) {
        var player = Minecraft.getInstance().player;
        double vanillaMultiplier = player == null ? 0.3 : player.getAttributeValue(Attributes.SNEAKING_SPEED);
        // Starain dispatches raw key impulses before diagonal normalization and sneak scaling.
        float forward = keyPresses.forward() == keyPresses.backward() ? 0 : keyPresses.forward() ? 1 : -1;
        float strafe = keyPresses.left() == keyPresses.right() ? 0 : keyPresses.left() ? 1 : -1;
        MoveInputEvent event = Myth.EVENT_MANAGER.call(new MoveInputEvent(
                forward, strafe, keyPresses.jump(), keyPresses.shift(), vanillaMultiplier));
        myth$sneakMultiplier = event.getSneakSlowDownMultiplier();
        boolean changeForward = event.getForward() != forward;
        boolean changeStrafe = event.getStrafe() != strafe;
        keyPresses = new Input(
                changeForward ? event.getForward() > 0 : keyPresses.forward(),
                changeForward ? event.getForward() < 0 : keyPresses.backward(),
                changeStrafe ? event.getStrafe() > 0 : keyPresses.left(),
                changeStrafe ? event.getStrafe() < 0 : keyPresses.right(),
                event.isJump(), event.isSneak(), keyPresses.sprint());
        myth$forwardImpulse = event.getForward();
        myth$leftImpulse = event.getStrafe();
        Vec2 corrected = new Vec2(myth$leftImpulse, myth$forwardImpulse);
        // 26.2 transports movement as a vector; retain its vanilla unit-length limit.
        moveVector = corrected.lengthSquared() > 1 ? corrected.normalized() : corrected;
    }

    @Override
    public double myth$getSneakMultiplier() {
        return myth$sneakMultiplier;
    }

    @Override public float myth$getForwardImpulse() {
        var player = Minecraft.getInstance().player;
        return player != null && player.isMovingSlowly()
                ? (float) (myth$forwardImpulse * myth$sneakMultiplier) : myth$forwardImpulse;
    }

    @Override public float myth$getLeftImpulse() {
        var player = Minecraft.getInstance().player;
        return player != null && player.isMovingSlowly()
                ? (float) (myth$leftImpulse * myth$sneakMultiplier) : myth$leftImpulse;
    }
}
