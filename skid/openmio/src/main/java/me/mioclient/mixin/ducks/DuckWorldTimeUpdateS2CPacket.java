package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.s2c.play.WorldTimeUpdateS2CPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({WorldTimeUpdateS2CPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckWorldTimeUpdateS2CPacket.class */
public interface DuckWorldTimeUpdateS2CPacket {
    @Accessor("time")
    @Mutable
    void setTime(long j);

    @Accessor("timeOfDay")
    @Mutable
    void setTimeOfDay(long j);
}
