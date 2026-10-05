package me.mioclient;

import me.mioclient.event.Event;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MatrixStackEvent.class */
public class MatrixStackEvent extends Event {
    public MatrixStack matrixStack;
    public float val;
    public DrawContext drawContext;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/MatrixStackEvent$Inner.class */
    public static class Inner extends MatrixStackEvent {
        public static final Inner inner = new Inner();

        public static Inner getInner933(MatrixStack matrixStack, float f) {
            inner.matrixStack = matrixStack;
            inner.val = f;
            inner.do2402();
            return inner;
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/MatrixStackEvent$Inner_2.class */
    public static class Inner_2 extends MatrixStackEvent {
        public static final Inner_2 inner_2 = new Inner_2();

        public static Inner_2 getInner_23016(MatrixStack matrixStack, float f) {
            inner_2.matrixStack = matrixStack;
            inner_2.val = f;
            inner_2.drawContext = Inner_3.inner_3.getDrawContext474();
            inner_2.do2402();
            return inner_2;
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/MatrixStackEvent$Inner_3.class */
    public static class Inner_3 extends MatrixStackEvent {
        public static final Inner_3 inner_3 = new Inner_3();

        public static Inner_3 getInner_3133(MatrixStack matrixStack, float f) {
            inner_3.matrixStack = matrixStack;
            inner_3.val = f;
            inner_3.drawContext = createDrawContext(matrixStack);
            inner_3.do2402();
            return inner_3;
        }

        private static java.lang.reflect.Constructor<DrawContext> drawContextCtor;

        private static DrawContext createDrawContext(MatrixStack matrixStack) {
            try {
                if (drawContextCtor == null) {
                    drawContextCtor = DrawContext.class.getDeclaredConstructor(MinecraftClient.class, MatrixStack.class, net.minecraft.client.render.VertexConsumerProvider.Immediate.class);
                    drawContextCtor.setAccessible(true);
                }
                return drawContextCtor.newInstance(MinecraftClient.getInstance(), matrixStack, MinecraftClient.getInstance().getBufferBuilders().getEntityVertexConsumers());
            } catch (Exception e) {
                throw new java.lang.RuntimeException(e);
            }
        }
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
