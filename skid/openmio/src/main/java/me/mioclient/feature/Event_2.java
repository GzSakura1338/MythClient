package me.mioclient.feature;

import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Event_2.class */
public class Event_2 extends me.mioclient.event.Event {
    public final Screen screen;
    public DrawContext drawContext;
    public float x;
    public float y;
    public Runnable runnable = null;

    public Event_2(Screen screen) {
        this.screen = screen;
    }

    public Screen getScreen247() {
        return this.screen;
    }

    public MatrixStack getMatrixStack472() {
        return this.drawContext.getMatrices();
    }

    public float get123() {
        return this.x;
    }

    public float get124() {
        return this.y;
    }

    public DrawContext getDrawContext474() {
        return this.drawContext;
    }

    public void do2379(DrawContext drawContext) {
        this.drawContext = drawContext;
    }

    public void setX(float f) {
        this.x = f;
    }

    public void setY(float f) {
        this.y = f;
    }

    public void do2380(Runnable runnable) {
        this.runnable = runnable;
    }

    public void do1457() {
        if (this.runnable != null) {
            this.runnable.run();
        }
    }
}
