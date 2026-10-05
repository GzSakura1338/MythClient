package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.s2c.play.EntityVelocityUpdateS2CPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({EntityVelocityUpdateS2CPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckEntityVelocityUpdateS2CPacket.class */
public interface DuckEntityVelocityUpdateS2CPacket {
    @Accessor("velocityX")
    @Mutable
    void setX(int i);

    @Accessor("velocityY")
    @Mutable
    void setY(int i);

    @Accessor("velocityZ")
    @Mutable
    void setZ(int i);
}
