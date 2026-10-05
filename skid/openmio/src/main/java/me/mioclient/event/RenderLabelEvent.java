package me.mioclient.event;

import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/RenderLabelEvent.class */
public class RenderLabelEvent extends Event {
    public final MatrixStack matrixStack;
    public final Entity entity;

    public RenderLabelEvent(MatrixStack matrixStack, Entity entity) {
        this.matrixStack = matrixStack;
        this.entity = entity;
    }

    public MatrixStack getMatrixStack472() {
        return this.matrixStack;
    }

    public Entity getEntity181() {
        return this.entity;
    }
}
