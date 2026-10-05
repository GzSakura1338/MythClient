package me.mioclient.event;

import net.minecraft.client.input.Input;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/TickEvent_2.class */
public class TickEvent_2 extends Event {
    public final Input input;
    public final float val;

    public TickEvent_2(Input input, float f) {
        this.input = input;
        this.val = f;
    }

    public Input getInput806() {
        return this.input;
    }

    public float get807() {
        return this.val;
    }

    public boolean is808() {
        return this.val != -1.0f;
    }

    public void reset() {
        Input input = this.input;
        Input input2 = this.input;
        Input input3 = this.input;
        Input input4 = this.input;
        Input input5 = this.input;
        this.input.sneaking = false;
        input5.jumping = false;
        input4.pressingRight = false;
        input3.pressingLeft = false;
        input2.pressingBack = false;
        input.pressingForward = false;
        this.input.movementForward = 0.0f;
        this.input.movementSideways = 0.0f;
    }
}
