package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({PlayerMoveC2SPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckPlayerMoveC2SPacket.class */
public interface DuckPlayerMoveC2SPacket {
    @Accessor("onGround")
    @Mutable
    void setOnGround(boolean z);

    @Accessor("changePosition")
    boolean isChangePosition();

    @Accessor("y")
    @Mutable
    void setY(double d);

    @Accessor("x")
    @Mutable
    void setX(double d);

    @Accessor("z")
    @Mutable
    void setZ(double d);

    @Accessor("yaw")
    @Mutable
    void setYaw(float f);

    @Accessor("pitch")
    @Mutable
    void setPitch(float f);
}
