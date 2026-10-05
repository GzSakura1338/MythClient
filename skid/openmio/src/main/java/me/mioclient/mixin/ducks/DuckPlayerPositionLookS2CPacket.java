package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.s2c.play.PlayerPositionLookS2CPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({PlayerPositionLookS2CPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckPlayerPositionLookS2CPacket.class */
public interface DuckPlayerPositionLookS2CPacket {
    @Accessor("yaw")
    @Mutable
    void setYaw(float f);

    @Accessor("pitch")
    @Mutable
    void setPitch(float f);
}
