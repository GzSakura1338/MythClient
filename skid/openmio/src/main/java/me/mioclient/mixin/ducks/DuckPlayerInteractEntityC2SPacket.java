package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({PlayerInteractEntityC2SPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckPlayerInteractEntityC2SPacket.class */
public interface DuckPlayerInteractEntityC2SPacket {
    @Accessor("entityId")
    int getEntityId();
}
