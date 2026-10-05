package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.c2s.play.ChatMessageC2SPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({ChatMessageC2SPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckChatMessageC2SPacket.class */
public interface DuckChatMessageC2SPacket {
    @Accessor("chatMessage")
    @Mutable
    void setChatMessage(String str);
}
