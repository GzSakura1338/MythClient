package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.c2s.play.CommandExecutionC2SPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({CommandExecutionC2SPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckCommandExecutionC2SPacket.class */
public interface DuckCommandExecutionC2SPacket {
    @Accessor("command")
    @Mutable
    void setCommand(String str);
}
