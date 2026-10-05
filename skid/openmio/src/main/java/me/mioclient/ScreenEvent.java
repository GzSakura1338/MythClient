package me.mioclient;

import me.mioclient.event.Event;
import net.minecraft.client.gui.screen.Screen;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ScreenEvent.class */
public class ScreenEvent extends Event {
    public final Screen screen;
    public final Screen screen2;

    public ScreenEvent(Screen screen, Screen screen2) {
        this.screen = screen;
        this.screen2 = screen2;
    }

    public Screen getScreen247() {
        return this.screen2;
    }

    public Screen getScreen2883() {
        return this.screen;
    }
}
