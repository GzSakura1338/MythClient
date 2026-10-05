package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.s2c.play.EntityStatusS2CPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({EntityStatusS2CPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckEntityStatusS2CPacket.class */
public interface DuckEntityStatusS2CPacket {
    @Accessor("entityId")
    int getId();
}
