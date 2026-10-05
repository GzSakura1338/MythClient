package me.mioclient.mixin.ducks;

import net.minecraft.network.packet.s2c.play.ChunkDeltaUpdateS2CPacket;
import net.minecraft.util.math.ChunkSectionPos;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({ChunkDeltaUpdateS2CPacket.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckChunkDeltaUpdateS2CPacket.class */
public interface DuckChunkDeltaUpdateS2CPacket {
    @Accessor("sectionPos")
    ChunkSectionPos getSection();
}
