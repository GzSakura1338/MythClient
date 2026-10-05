package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.s2c.play.ExplosionS2CPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({ExplosionS2CPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckExplosionS2CPacket.class */
public interface DuckExplosionS2CPacket {
    @Accessor("playerVelocityX")
    @Mutable
    void setX(float f);

    @Accessor("playerVelocityY")
    @Mutable
    void setY(float f);

    @Accessor("playerVelocityZ")
    @Mutable
    void setZ(float f);
}
