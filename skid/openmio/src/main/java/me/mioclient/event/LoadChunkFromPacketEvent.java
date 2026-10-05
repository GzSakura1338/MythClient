package me.mioclient.event;

import net.minecraft.world.chunk.WorldChunk;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/LoadChunkFromPacketEvent.class */
public class LoadChunkFromPacketEvent extends Event {
    public final WorldChunk worldChunk;

    public LoadChunkFromPacketEvent(WorldChunk worldChunk) {
        this.worldChunk = worldChunk;
    }

    public WorldChunk getWorldChunk2555() {
        return this.worldChunk;
    }
}
