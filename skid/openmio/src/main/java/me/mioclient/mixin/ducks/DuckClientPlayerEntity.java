package me.mioclient.mixin.ducks;

import net.minecraft.client.network.ClientPlayerEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({ClientPlayerEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckClientPlayerEntity.class */
public interface DuckClientPlayerEntity {
    @Accessor("lastSprinting")
    boolean lastSprinting();

    @Accessor("lastSprinting")
    void lastSprinting(boolean z);

    @Accessor("lastSneaking")
    boolean lastSneaking();

    @Accessor("lastSneaking")
    void lastSneaking(boolean z);

    @Accessor("lastOnGround")
    boolean lastOnGround();

    @Accessor("lastOnGround")
    void lastOnGround(boolean z);

    @Accessor("lastX")
    double lastX();

    @Accessor("lastX")
    void lastX(double d);

    @Accessor("lastBaseY")
    double lastBaseY();

    @Accessor("lastBaseY")
    void lastBaseY(double d);

    @Accessor("lastZ")
    double lastZ();

    @Accessor("lastZ")
    void lastZ(double d);

    @Accessor("lastYaw")
    float lastYaw();

    @Accessor("lastYaw")
    void lastYaw(float f);

    @Accessor("lastPitch")
    float lastPitch();

    @Accessor("lastYaw")
    void lastPitch(float f);

    @Accessor("autoJumpEnabled")
    boolean autoJumpEnabled();

    @Accessor("autoJumpEnabled")
    void autoJumpEnabled(boolean z);

    @Accessor("ticksSinceLastPositionPacketSent")
    int ticksSinceLastPositionPacketSent();

    @Accessor("ticksSinceLastPositionPacketSent")
    void ticksSinceLastPositionPacketSent(int i);
}
