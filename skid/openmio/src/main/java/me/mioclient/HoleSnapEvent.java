package me.mioclient;

import me.mioclient.event.Event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HoleSnapEvent.class */
public final class HoleSnapEvent extends Event implements SearchHelper_4 {
    public float get751() {
        return minecraftClient.player.getYaw();
    }

    public float get752() {
        return minecraftClient.player.getPitch();
    }
}
