package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.CustomPayload;
import net.minecraft.network.packet.c2s.common.CustomPayloadC2SPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({CustomPayloadC2SPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckCustomPayloadC2SPacket.class */
public interface DuckCustomPayloadC2SPacket {
    @Accessor("payload")
    @Mutable
    void setCustomPayload(CustomPayload customPayload);
}
