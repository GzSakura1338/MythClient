package net.caffeinemc.mods.sodium.api.vertex.buffer;
import org.lwjgl.system.MemoryStack;
import net.caffeinemc.mods.sodium.api.vertex.format.VertexFormatDescription;
/** Stub of Sodium 0.6 API (optional soft-dependency; not bundled). */
public interface VertexBufferWriter {
    void push(MemoryStack stack, long ptr, int count, VertexFormatDescription format);
    boolean canUseIntrinsics();
}
