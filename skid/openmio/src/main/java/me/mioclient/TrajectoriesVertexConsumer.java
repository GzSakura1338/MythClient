package me.mioclient;

import net.minecraft.client.render.BufferBuilder;
import net.minecraft.client.render.BufferRenderer;
import net.minecraft.client.render.BuiltBuffer;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.render.VertexFormat;
import net.minecraft.client.util.BufferAllocator;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/TrajectoriesVertexConsumer.class */
public class TrajectoriesVertexConsumer implements VertexConsumer {
    public static final boolean flag;
    public final BufferAllocator bufferAllocator;
    public BufferBuilder bufferBuilder;
    public VertexFormat.DrawMode drawMode;
    public VertexFormat vertexFormat;

    public static TrajectoriesVertexConsumer getTrajectoriesVertexConsumer2592() {
        return getTrajectoriesVertexConsumer2593(786432);
    }

    public static TrajectoriesVertexConsumer getTrajectoriesVertexConsumer2593(int i) {
        return flag ? Helper_21.getTrajectoriesVertexConsumer2400(i) : new TrajectoriesVertexConsumer(i);
    }

    public TrajectoriesVertexConsumer(int i) {
        this.bufferAllocator = new BufferAllocator(i);
    }

    public TrajectoriesVertexConsumer getTrajectoriesVertexConsumer2594(VertexFormat.DrawMode drawMode, VertexFormat vertexFormat) {
        this.drawMode = drawMode;
        this.vertexFormat = vertexFormat;
        return this;
    }

    public VertexConsumer vertex(float f, float f2, float f3) {
        return getBufferBuilder2596().vertex(f, f2, f3);
    }

    public VertexConsumer color(int i, int i2, int i3, int i4) {
        return getBufferBuilder2596().color(i, i2, i3, i4);
    }

    public VertexConsumer texture(float f, float f2) {
        return getBufferBuilder2596().texture(f, f2);
    }

    public VertexConsumer overlay(int i, int i2) {
        return getBufferBuilder2596().overlay(i, i2);
    }

    public VertexConsumer light(int i, int i2) {
        return getBufferBuilder2596().light(i, i2);
    }

    public VertexConsumer normal(float f, float f2, float f3) {
        return getBufferBuilder2596().normal(f, f2, f3);
    }

    public boolean is1662() {
        return this.bufferBuilder != null;
    }

    public BufferBuilder getBufferBuilder2595(VertexFormat.DrawMode drawMode, VertexFormat vertexFormat) {
        this.bufferBuilder = new BufferBuilder(this.bufferAllocator, drawMode, vertexFormat);
        return this.bufferBuilder;
    }

    public BufferBuilder getBufferBuilder2596() {
        if (this.bufferBuilder == null) {
            if (this.drawMode == null || this.vertexFormat == null) {
                throw new IllegalArgumentException("Tried to invoke empty not caching buffer");
            }
            this.bufferBuilder = new BufferBuilder(this.bufferAllocator, this.drawMode, this.vertexFormat);
        }
        return this.bufferBuilder;
    }

    public BuiltBuffer getBuiltBuffer2597() {
        if (this.bufferBuilder == null) {
            throw new IllegalArgumentException("Tried to end empty buffer");
        }
        BuiltBuffer endNullable = this.bufferBuilder.endNullable();
        this.bufferBuilder = null;
        return endNullable;
    }

    public void do865() {
        if (this.bufferBuilder == null) {
            return;
        }
        do2599(this.bufferBuilder);
        this.bufferBuilder = null;
    }

    public static void do2598(TrajectoriesVertexConsumer trajectoriesVertexConsumer) {
        if (trajectoriesVertexConsumer.is1662()) {
            do2599(trajectoriesVertexConsumer.bufferBuilder);
            trajectoriesVertexConsumer.bufferBuilder = null;
        }
    }

    public static void do2599(BufferBuilder bufferBuilder) {
        BuiltBuffer endNullable;
        if (bufferBuilder == null || (endNullable = bufferBuilder.endNullable()) == null) {
            return;
        }
        BufferRenderer.drawWithGlobalProgram(endNullable);
    }

    static {
        boolean z;
        try {
            Class.forName("net.caffeinemc.mods.sodium.api.vertex.buffer.VertexBufferWriter");
            z = true;
        } catch (ClassNotFoundException e) {
            z = false;
        }
        flag = z;
    }
}
