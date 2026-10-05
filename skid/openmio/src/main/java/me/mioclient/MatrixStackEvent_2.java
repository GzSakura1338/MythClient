package me.mioclient;

import me.mioclient.event.Event;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MatrixStackEvent_2.class */
public class MatrixStackEvent_2 extends Event {
    public static final MatrixStackEvent_2 matrixStackEvent_2 = new MatrixStackEvent_2();
    public MatrixStack matrixStack;
    public DrawContext drawContext;
    public float val;

    public static MatrixStackEvent_2 getMatrixStackEvent_21234(MatrixStack matrixStack, DrawContext drawContext, float f) {
        matrixStackEvent_2.matrixStack = matrixStack;
        matrixStackEvent_2.val = f;
        matrixStackEvent_2.drawContext = drawContext;
        matrixStackEvent_2.do2402();
        return matrixStackEvent_2;
    }

    public MatrixStack getMatrixStack472() {
        return this.matrixStack;
    }

    public float get473() {
        return this.val;
    }

    public DrawContext getDrawContext474() {
        return this.drawContext;
    }
}
