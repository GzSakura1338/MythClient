package me.mioclient;

import net.caffeinemc.mods.sodium.api.vertex.format.VertexFormatDescription;
import org.lwjgl.system.MemoryStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/VertexBufferWriter.class */
public class VertexBufferWriter extends TrajectoriesVertexConsumer implements net.caffeinemc.mods.sodium.api.vertex.buffer.VertexBufferWriter {
    public final boolean flag;

    public VertexBufferWriter(int i) {
        super(i);
        this.flag = this.bufferBuilder instanceof net.caffeinemc.mods.sodium.api.vertex.buffer.VertexBufferWriter;
    }

    public void push(MemoryStack memoryStack, long j, int i, VertexFormatDescription vertexFormatDescription) {
        if (this.flag) {
            ((net.caffeinemc.mods.sodium.api.vertex.buffer.VertexBufferWriter)(Object) this.bufferBuilder).push(memoryStack, j, i, vertexFormatDescription);
        }
    }

    public boolean canUseIntrinsics() {
        return this.flag;
    }
}
