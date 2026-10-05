package dev.myth.access;

import net.minecraft.client.player.ClientInput;

public interface MovementInputAccess {
    double myth$getSneakMultiplier();
    float myth$getForwardImpulse();
    float myth$getLeftImpulse();

    static float forwardImpulse(ClientInput input) {
        return input instanceof MovementInputAccess access ? access.myth$getForwardImpulse() : input.getMoveVector().y;
    }

    static float leftImpulse(ClientInput input) {
        return input instanceof MovementInputAccess access ? access.myth$getLeftImpulse() : input.getMoveVector().x;
    }
}
