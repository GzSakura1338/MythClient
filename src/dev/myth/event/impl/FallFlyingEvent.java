package dev.myth.event.impl;

import dev.myth.event.api.Event;

public class FallFlyingEvent implements Event {
    private float pitch;

    public FallFlyingEvent(float pitch) {
        this.pitch = pitch;
    }

    public float getPitch() {
        return pitch;
    }

    public void setPitch(float pitch) {
        this.pitch = pitch;
    }
}
